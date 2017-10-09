
#include "client_mgt.h"
#include "comm/common.h"
#include "conf_mgt.h"
#include "base/base_logger.h"
#include "base/base_uid.h"
#include "base/base_string.h"
#include "base/base_utility.h"
#include "base/base_convert.h"
#include "base/base_reactor.h"
#include "redis_mgt.h"
#include "conn_mgt_lb.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;

extern Conn_Mgt_LB g_push_conn;


Client_Mgt::Client_Mgt()
{

}


Client_Mgt::~Client_Mgt()
{


}



int Client_Mgt::register_client(const StClient stClient, std::string &err_info)
{
	int nRet = 0;

	XCP_LOGGER_INFO(&g_logger, "--- prepare to register client ---\n");
	
	//注册路由信息
	nRet = PSGT_Redis_Mgt->register_route(stClient._id, g_sysInfo._new_id, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "register route failed, ret:%d\n", nRet);
		return nRet;
	}


	{
		//存储本地客户端
		Thread_Mutex_Guard guard(_mutex);

		//判断该客户端是否已经注册过
		std::map<std::string, StClient>::iterator itr = _clients.find(stClient._session_id);
		if(itr != _clients.end())
		{
			XCP_LOGGER_INFO(&g_logger, "the client(session id:%s) is registered ago\n", stClient._session_id.c_str());
			err_info = "the client is registered ago";
			return -1;
		}
		_clients.insert(std::make_pair(stClient._session_id, stClient));
		
		if(stClient._type == TT_APP)
		{
			std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr = _apps.find(stClient._id);
			if(itr == _apps.end())
			{
				std::map<std::string, StClient> clients;
				clients.insert(std::make_pair(stClient._session_id, stClient));
				_apps.insert(std::make_pair(stClient._id, clients));
			}
			else
			{
				itr->second.insert(std::make_pair(stClient._session_id, stClient));
			}

		}
		else if(stClient._type == TT_ROUTER)
		{
			std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr = _routers.find(stClient._id);
			if(itr == _routers.end())
			{
				std::map<std::string, StClient> clients;
				clients.insert(std::make_pair(stClient._session_id, stClient));
				_routers.insert(std::make_pair(stClient._id, clients));
			}
			else
			{
				itr->second.insert(std::make_pair(stClient._session_id, stClient));
			}

		}
		else
		{
			//no-todo
			return -1;
		}
		
	}

	//将用户上线消息发送给push svr
	Conn_Ptr conn;
	if(!g_push_conn.get_conn(conn))
	{
		XCP_LOGGER_INFO(&g_logger, "no push svr conn.\n");
	}
	else
	{
		conn->notify_client_status(UID::uid_inc(g_sysInfo._log_id), stClient._id, stClient._type, US_LOGIN, err_info); 
	}
	
	return 0;

}




void Client_Mgt::unregister_client(const std::string &session_id, const std::string &status)
{
	XCP_LOGGER_INFO(&g_logger, "--- prepare to unregister client ---\n");

	unsigned long long id  = 0;
	std::string type  = "";
	{
		Thread_Mutex_Guard guard(_mutex);

		std::map<std::string, StClient>::iterator itr_client = _clients.find(session_id);
		if(itr_client != _clients.end())
		{
			id = itr_client->second._id;
			type = itr_client->second._type;
			std::string type = itr_client->second._type;

			if(type == TT_APP)
			{
				std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr_id = _apps.find(id);
				if(itr_id != _apps.end())
				{
					std::map<std::string, StClient>::iterator itr = itr_id->second.find(session_id);
					if(itr != itr_id->second.end())
					{
						itr_id->second.erase(itr);
						if(itr_id->second.empty())
						{
							_apps.erase(itr_id);
						}
					}
				}

			}
			else if(type == TT_ROUTER)
			{
				std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr_id = _routers.find(id);
				if(itr_id != _routers.end())
				{
					std::map<std::string, StClient>::iterator itr = itr_id->second.find(session_id);
					if(itr != itr_id->second.end())
					{
						itr_id->second.erase(itr);
						if(itr_id->second.empty())
						{
							_routers.erase(itr_id);
						}
					}
				}

			}
			else
			{
				//no-todo
				return;
			}

			_clients.erase(itr_client);
			
		}
		else
		{
			//no-todo
			return;
		}
		
	}

	//去注册路由表
	std::string err_info = "";
	PSGT_Redis_Mgt->unregister_route(id, g_sysInfo._new_id, err_info);	

	//将用户下线消息发送给push svr
	Conn_Ptr conn;
	if(!g_push_conn.get_conn(conn))
	{
		XCP_LOGGER_INFO(&g_logger, "no push svr conn.\n");
	}
	else
	{
		conn->notify_client_status(UID::uid_inc(g_sysInfo._log_id), id, type, status, err_info); 
	}

}




bool Client_Mgt::is_auth(const std::string &session_id)
{
	int bRet = false;
	
	Thread_Mutex_Guard guard(_mutex);
	
	std::map<std::string, StClient>::iterator itr = _clients.find(session_id);
	if(itr != _clients.end())
	{
		bRet = true;
		XCP_LOGGER_INFO(&g_logger, "the client(session id:%s) is auth ago.\n", session_id.c_str());
	}

	return bRet;
}




bool Client_Mgt::get_clients(unsigned long long id, std::map<std::string, StClient> &clients, std::string &err_info)
{
	int bRet = false;
	err_info = "";

	Thread_Mutex_Guard guard(_mutex);

	std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr = _apps.find(id);
	if(itr != _apps.end())
	{
		bRet = true;
		clients = itr->second;
	}
	else
	{
		std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr1 = _routers.find(id);
		if(itr1 != _routers.end())
		{
			bRet = true;
			clients = itr1->second;
		}
	}
	
	return bRet;
	
}



void Client_Mgt::get_all_app(std::map<unsigned long long, std::map<std::string, StClient> > &apps)
{
	Thread_Mutex_Guard guard(_mutex);
	apps = _apps;

}




void Client_Mgt::get_all_router(std::map<unsigned long long, std::map<std::string, StClient> > &routers)
{
	Thread_Mutex_Guard guard(_mutex);
	routers = _routers;

}



unsigned int Client_Mgt::client_num()
{
	return _clients.size();
}


