
#include "redis_mgt.h"
#include "base/base_convert.h"
#include "redis_timer_handler.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_convert.h"

extern Logger g_logger;
redis_mgt g_redis_mgt;

//redis conn
redis_mgt::redis_mgt()
{

}

redis_mgt::~redis_mgt()
{

}

//初始化
int redis_mgt::init(const std::string &ip, unsigned int port,const std::string &pwd,const unsigned int cnt)
{
	int nRet = 0;

	//创建mysql连接池
	for (unsigned int i = 0; i < cnt; i++)
	{
		redis_conn_Ptr conn = new redis_conn(i);
		nRet = conn->connect_conn(ip, port,pwd);
		if (nRet != 0)
		{
			printf("redis connect_conn failed, error code:%d\n", nRet);
			return nRet;
		}
		_conn_queue.push_back(conn);
	}

	//启动conn mgt  timer
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start redis mgt timer ---\n");
	Select_Timer *timer = new Select_Timer;
	Redis_Timer_handler *conn_thandler = new Redis_Timer_handler;
	nRet = timer->register_timer_handler(conn_thandler, 5000000);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "register redis mgt timer handler falied, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = timer->init();
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "int redis mgt timer failed, ret:%d\n", nRet);
		return nRet;
	}

	nRet = timer->run();
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "redis mgt timer run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start redis mgt timer ===\n");
	return nRet;

}



bool redis_mgt::get_conn(redis_conn_Ptr &conn)
{
	Thread_Mutex_Guard guard(_mutex);

	bool bRet = false;

	std::vector<redis_conn_Ptr>::iterator itr = _conn_queue.begin();
	for (; itr != _conn_queue.end(); itr++)
	{
		if (!(*itr)->_used && (*itr)->_valid)
		{
			conn = *itr;
			(*itr)->_used = true;
			bRet = true;
			break;
		}
	}

	if (!bRet)
	{
		XCP_LOGGER_ERROR(&g_logger, "redis no free conn is gotten, pls try next time. total_conn_size:%d\n", _conn_queue.size());
	}

	return bRet;

}




void redis_mgt::check()
{
	Thread_Mutex_Guard guard(_mutex);
	int nRet = 0;
	std::vector<redis_conn_Ptr>::iterator itr = _conn_queue.begin();
	for (; itr != _conn_queue.end(); itr++)
	{
		if ((*itr)->_used == false)
		{
			nRet = (*itr)->ping();
			if (nRet == 0)
			{
				XCP_LOGGER_INFO(&g_logger, "redis check,ping ok,seq:%d\n", (*itr)->_seq);
				(*itr)->_valid = true;
			}
			else
			{
				//ping 失败
				nRet = (*itr)->reconnect();
				XCP_LOGGER_INFO(&g_logger, "redis check,reconnect fail!nRet:%d,seq:%d\n",nRet, (*itr)->_seq);
				if (nRet == 0)
				{
					(*itr)->_valid = true;
				}
			}
		}
	}
}



void redis_mgt::release(redis_conn_Ptr &conn)
{
	Thread_Mutex_Guard guard(_mutex);
	conn->_used = false;
}



//----------------------------------------

Redis_Guard::Redis_Guard(redis_conn_Ptr &conn) : _conn(conn)
{

}


Redis_Guard::~Redis_Guard()
{
	PSGT_Redis_Mgt->release(_conn);
}


redis_conn_Ptr& Redis_Guard::operator-> ()
{
	return _conn;
}


