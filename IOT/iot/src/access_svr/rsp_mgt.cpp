
#include "comm/common.h"
#include "rsp_mgt.h"
#include "base/base_logger.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;


Rsp_Mgt::Rsp_Mgt(): _cond(_mutex)
{

}


Rsp_Mgt::~Rsp_Mgt()
{

}



unsigned int Rsp_Mgt::size()
{
	Thread_Mutex_Guard guard(_mutex);
	return _items.size();
}



bool Rsp_Mgt::empty()
{
	Thread_Mutex_Guard guard(_mutex);
	return _items.empty();
}



int Rsp_Mgt::push_req(const std::string &key, Request_Ptr req)
{
	int nRet = 0;
	
	Thread_Mutex_Guard guard(_mutex);

	std::map<std::string, Request_Ptr>::iterator itr = _items.find(key);
	if(itr == _items.end())
	{
		_items.insert(std::make_pair(key, req));
		//唤醒所有等待的线程进行处理
		_cond.broadcast();
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "it is timeout rsp, rsp:%s\n", req->to_string().c_str());
		_items.erase(itr);
	}

	return nRet;

}




int Rsp_Mgt::get_req(const std::string &key, Request_Ptr &req, const long long timeout)
{
	int nRet = 0;

	long long _timeout = timeout;
	long long pre = get_millisecond();

	Thread_Mutex_Guard guard(_mutex);

	if(_items.empty())
	{
		nRet = _cond.timed_wait(_timeout);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "it is timeout req1, key:%s\n", key.c_str());
			_items.insert(std::make_pair(key, req));
			return -1;
		}
	}
	
	while(true)
	{		
		std::map<std::string, Request_Ptr >::iterator itr = _items.find(key);
		if(itr != _items.end())
		{
			//在超时时间内找到
			req = itr->second;
			_items.erase(itr);
			return 0;
		}
		else
		{
			//没找到
			long long cur = get_millisecond();
			_timeout -= (cur-pre);
			if(_timeout <= 0)
			{
				XCP_LOGGER_INFO(&g_logger, "it is timeout req2, key:%s\n", key.c_str());
				_items.insert(std::make_pair(key, req));
				return -2;
			}
			pre = cur;
			
			nRet = _cond.timed_wait(_timeout);
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "it is timeout req3, key:%s\n", key.c_str());
				_items.insert(std::make_pair(key, req));
				return -3;
			}
		}
		
	}
	
	return nRet;

}


