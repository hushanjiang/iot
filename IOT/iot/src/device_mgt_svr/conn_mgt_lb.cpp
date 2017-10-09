
#include "conn_mgt_lb.h"
#include "base/base_logger.h"
#include "base/base_string.h"
#include "base/base_time.h"
#include "base/base_timer_select.h"
#include "conn_timer_handler.h"


extern Logger g_logger;

//--------------- conn mgt ----------------

Conn_Mgt_LB g_conf_mgt_conn;
Conn_Mgt_LB g_uid_conn;
Conn_Mgt_LB g_push_conn;


Conn_Mgt_LB::Conn_Mgt_LB(): _index(0)
{

}



Conn_Mgt_LB::~Conn_Mgt_LB()
{

}




//��ʼ�������ӳ�
int Conn_Mgt_LB::init(std::vector<StSvr> *svrs, Event_Handler *handler, bool asyn, bool do_register)
{
	int nRet = 0;

	_handler = handler;
	_asyn = asyn;
	_do_register = do_register;

	refresh(svrs);

	//��conn mgt  timer��5���ӾͿ��Լ��Զ˷����Ƿ�������
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start conn mgt lb timer ---\n");
	Select_Timer *timer_req = new Select_Timer;
	Conn_Timer_handler *conn_thandler = new Conn_Timer_handler;
	conn_thandler->_conn_mgt = this;
	nRet = timer_req->register_timer_handler(conn_thandler, 5000000);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "register conn mgt lb timer handler falied, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = timer_req->init();
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "int conn mgt lb timer failed, ret:%d\n", nRet);
		return nRet;
	}

	nRet = timer_req->run();
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "conn mgt lb timer run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start conn mgt lb timer ===\n");
	
	return 0;
	
}





void Conn_Mgt_LB::refresh(std::vector<StSvr> *svrs)
{
	Thread_Mutex_Guard guard(_mutex);
	
	//�ͷ�ԭ�����ӳ��е���Ч����
	std::vector<Conn_Ptr>::iterator itr = _conns.begin();
	for(; itr != _conns.end();)
	{
		bool found = false;
		for(unsigned int i=0; i<svrs->size(); i++)
		{
			if(((*svrs)[i]._ip == (*itr)->_svr._ip) &&
			   ((*svrs)[i]._port == (*itr)->_svr._port))
			{
				found = true;
				break;
			}
		}

		//������Ч��conn�رպ��ͷ�
		if(!found)
		{
			std::string id = format("%s_%u", (*itr)->_svr._ip.c_str(), (*itr)->_svr._port);
			XCP_LOGGER_INFO(&g_logger, "**** release useless conn(%s) ****\n", id.c_str());
			printf("**** release useless conn(%s) ****\n", id.c_str());
			
			(*itr)->close();
			_conns.erase(itr); //����Conn��������
		}
		else
		{
			++itr;
		}

	}
	
		
	//����������
	for(unsigned int i=0; i<svrs->size(); i++)
	{
		bool found = false;
		std::vector<Conn_Ptr>::iterator itr = _conns.begin();
		for(; itr != _conns.end(); itr++)
		{
			if(((*svrs)[i]._ip == (*itr)->_svr._ip) &&
			   ((*svrs)[i]._port == (*itr)->_svr._port))
			{
				found = true;
				break;
			}
		
		}

		if(!found)
		{
			std::string id = format("%s_%u", (*svrs)[i]._ip.c_str(), (*svrs)[i]._port);
			XCP_LOGGER_INFO(&g_logger, "**** add new conn(%s) ****\n", id.c_str());
			printf("**** add new conn(%s) ****\n", id.c_str());
			
			Conn_Ptr conn = new Conn((*svrs)[i], _handler, _asyn);
			conn->connect();
			
			//������׷���ں���
			_conns.push_back(conn);
		}
		
	}

	
}





void Conn_Mgt_LB::check()
{
	check_conn();
}




void Conn_Mgt_LB::check_conn()
{
	Thread_Mutex_Guard guard(_mutex);
	
	std::vector<Conn_Ptr>::iterator itr = _conns.begin();
	for(; itr != _conns.end(); itr++)
	{
		(*itr)->connect();
	}
}



bool Conn_Mgt_LB::get_conn(Conn_Ptr &conn)
{
	Thread_Mutex_Guard guard(_mutex);

	if(_conns.empty())
	{
		XCP_LOGGER_ERROR(&g_logger, "conn is empty.\n");
		return false;
	}

	if(_index >= _conns.size())
	{
		_index = 0;
	}
	
	unsigned int tag = _index;
	do
	{
		conn = _conns[_index];
		++_index;
		if(!conn->is_close())
		{
			XCP_LOGGER_ERROR(&g_logger, "get (%s:%u) conn success, index:%u, total:%u\n", 
				conn->_svr._ip.c_str(), conn->_svr._port, (_index-1), _conns.size());
			return true;
		}
		else
		{
			XCP_LOGGER_ERROR(&g_logger, "the conn (%s:%u) is closed, index:%u, total:%u\n", 
				conn->_svr._ip.c_str(), conn->_svr._port, (_index-1), _conns.size());
		}

		if(_index >= _conns.size())
		{
			_index = 0;
		}

	}while(_index != tag);

	XCP_LOGGER_ERROR(&g_logger, "no conn is found.\n");
	return false;

}


