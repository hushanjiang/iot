
#include "redis_mgt.h"
#include "redis_timer_handler.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_convert.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;


Redis_Mgt::Redis_Mgt(): _redis(NULL), _valid(false), _ip(""), _port(0), _auth("") 
{

}


Redis_Mgt::~Redis_Mgt()
{

}



int Redis_Mgt::init(const std::string &ip, const unsigned int port, const std::string &auth)
{
	int nRet = 0;

	_ip = ip;
	_port = port;
	_auth = auth;

	//初始化先连接一下，后面定时器会定时检测redis server的状态
	connect();

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
	
	return 0;
	
}




void Redis_Mgt::release()
{
	//最后释放 redisContext 对象    
	redisFree(_redis);
	_redis = NULL;
	_valid = false;
}


void Redis_Mgt::check()
{
	int nRet = 0;
	
	Thread_Mutex_Guard guard(_mutex);

	if(_valid)
	{
		nRet = ping();
		if(nRet == 0)
		{
			//ping 成功
			return;
		}
		else
		{
			//ping 失败
			connect();
		}

	}
	else
	{
		//重新连接
		connect();
	}
	
}



int Redis_Mgt::connect()
{
	int nRet = 0;
	
	/*
	连接创建以后内部生成一个 redisContext 堆对象，一旦连接断开需要重新连接，
	没有自动重连机制
	*/
    struct timeval timeout = {0, 300000};
    _redis = redisConnectWithTimeout(_ip.c_str(), _port, timeout);
    if (_redis == NULL || _redis->err) 
	{
        if (_redis) 
		{
			XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) failed, err:%s\n", 
				_ip.c_str(), _port, _redis->errstr);
			release();

        } 
		else 
		{
			XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) failed, can't allocate redis context\n", 
				_ip.c_str(), _port);
        }
		
        return -1;
    }

	XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) success.\n", _ip.c_str(), _port);
	
	if(!_auth.empty())
	{
		redisReply *reply = NULL;
		reply = (redisReply*)redisCommand(_redis, "auth %s", _auth.c_str());
		if(!reply)
		{
			//如果redis server 停止，reply is null
			XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
			release();
			return -1;
		}
		else
		{
			if(reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"OK") == 0))
			{
				XCP_LOGGER_INFO(&g_logger, "auth redis server(%s:%u) success.\n", _ip.c_str(), _port);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "auth redis failed, err:%s, pls check redis server(%s:%u), auth:%s\n", 
					reply->str, _ip.c_str(), _port, _auth.c_str());
				release();
				return -1;
			}
		}

	}

	_valid = true;

	return nRet;
	
}



int Redis_Mgt::ping()
{
	int nRet = 0;
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", 
			_ip.c_str(), _port); 

		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "ping");
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u)\n", 
		_ip.c_str(), _port);
		nRet = -2;

		release();
	}
	else
	{
		if(reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"PONG") == 0))
		{
			//XCP_LOGGER_INFO(&g_logger, "ping redis server(%s:%u) success.\n", _ip.c_str(), _port);
		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "ping redis failed, err:%s, pls check redis server(%s:%u)\n", 
				reply->str, _ip.c_str(), _port);
			nRet = -3;

			release();
		}
    }

    freeReplyObject(reply);

	return nRet;
	
}




int Redis_Mgt::register_route(const unsigned long long id, const std::string &access_svr_id, std::string &err_info)
{
	int nRet = 0;
	err_info = "";

	XCP_LOGGER_INFO(&g_logger, "prepare to register route. id:%llu, access_svr_id:%s\n", id, access_svr_id.c_str());
		
	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		err_info = "user redis svr is disconnect.";
		return -1;
	}

    redisReply *reply = (redisReply*)redisCommand(_redis, "sadd MDP:id_%llu %s", id, access_svr_id.c_str());
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}
	freeReplyObject(reply);

    reply = (redisReply*)redisCommand(_redis, "zadd Access_Svr:%s %llu %llu", access_svr_id.c_str(), getTimestamp(), id);
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}
	freeReplyObject(reply);

	XCP_LOGGER_INFO(&g_logger, "complete to register route.\n");

	return nRet;

}



int Redis_Mgt::unregister_route(const unsigned long long id, const std::string &access_svr_id, std::string &err_info)
{
	int nRet = 0;
	err_info = "";

	XCP_LOGGER_INFO(&g_logger, "prepare to unregister route. id:%llu, access_svr_id:%s\n", id, access_svr_id.c_str());
		
	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		err_info = "user redis svr is disconnect.";
		return -1;
	}

    redisReply *reply = (redisReply*)redisCommand(_redis, "srem MDP:id_%llu %s", id, access_svr_id.c_str());
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}
	freeReplyObject(reply);

    reply = (redisReply*)redisCommand(_redis, "zrem Access_Svr:%s %llu", access_svr_id.c_str(), id);
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}
	freeReplyObject(reply);
	
	XCP_LOGGER_INFO(&g_logger, "complete to unregister route.\n");

	return nRet;

}





int Redis_Mgt::get_client_list(const std::string &access_svr_id, const unsigned long long begin, const unsigned int cnt, std::deque<unsigned long long> &clients, std::string &err_info)
{
	int bRet = false;

	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", 
			_ip.c_str(), _port);
		err_info = "user redis svr is disconnect.";
		return -1;
	}
	
    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "zrange Access_Svr:%s %llu %llu", access_svr_id.c_str(), begin, (begin+cnt-1));
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}
	
	if((reply->type == REDIS_REPLY_ARRAY) && (reply->elements > 0))
	{
		redisReply *reply_member = NULL;
		for(unsigned int i=0; i<reply->elements; i++)
		{
			reply_member = reply->element[i];
			if(!reply_member)
			{
				//如果redis server 停止，reply is null
				XCP_LOGGER_INFO(&g_logger, "reply_member is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
				err_info = "reply is null, pls check redis server.";
				freeReplyObject(reply);
				release();
				return -1;	
			}
			else
			{
				unsigned long long client_id = 0;
				if((reply_member->type == REDIS_REPLY_STRING))
				{
					client_id = atoll(reply_member->str);
				}
				clients.push_back(client_id);
			}
			
		}
		
	}

    freeReplyObject(reply);

	return bRet;

}






int Redis_Mgt::get_access_svr_list(std::set<std::string> &svrs, std::string &err_info)
{

	int bRet = false;

	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", 
			_ip.c_str(), _port);
		err_info = "user redis svr is disconnect.";
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "keys Access_Svr:%s*", g_sysInfo._log_id.c_str());
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}

	if((reply->type == REDIS_REPLY_ARRAY) && (reply->elements > 0))
	{
		redisReply *reply_member = NULL;
		for(unsigned int i=0; i<reply->elements; i++)
		{
			reply_member = reply->element[i];
			if(!reply_member)
			{
				//如果redis server 停止，reply is null
				XCP_LOGGER_INFO(&g_logger, "reply_member is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
				err_info = "reply is null, pls check redis server.";
				freeReplyObject(reply);
				release();
				return -1;	
			}
			else
			{
				std::string str = "";
				if((reply_member->type == REDIS_REPLY_STRING))
				{
					str = reply_member->str;
					if(str.empty())
					{
						continue;
					}
					svrs.insert(str);
				}
			}
		}
		
	}

    freeReplyObject(reply);

	return bRet;

}





int Redis_Mgt::get_security_channel(const std::string &id, std::string &key, std::string &err_info)
{
	int nRet = 0;
	err_info = "";
	
	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		err_info = "user redis svr is disconnect.";
		return -1;
	}

    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "hmget Security:%s key %s", id.c_str(), key.c_str());
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}
	else
	{
		if((reply->type == REDIS_REPLY_ARRAY))
		{
			if((reply->element[0]->type == REDIS_REPLY_STRING))
			{
				key = reply->element[0]->str;
			}
			
		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "the reply type of security channel id isn't array.\n");
			err_info = "the reply type of security channel id isn't array.";
			freeReplyObject(reply);
			return -1;
		}
	}
	freeReplyObject(reply);
	

	//设置TTL 3天
    reply = (redisReply*)redisCommand(_redis, "expire Security:%s %u", id.c_str(), SECURITY_CHANNEL_TTL);
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
	}
	else
	{
		if((reply->type == REDIS_REPLY_INTEGER) && (reply->integer != 1))
		{
			XCP_LOGGER_INFO(&g_logger, "expire security channel uuid failed, uuid:%s\n", id.c_str());
			err_info = "expire security channel uuid failed.";
			freeReplyObject(reply);
			return -1;
		}
	}
	freeReplyObject(reply);
	
	return nRet;

}





int Redis_Mgt::check_client_online(const unsigned long long client_id, std::string &err_info)
{
	int nRet = 0;
	err_info = "client_id is online.";

	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", 
			_ip.c_str(), _port);
		err_info = "user redis svr is disconnect.";
		return -1;
	}

	//判断该用户 是否在线
    redisReply *reply = (redisReply*)redisCommand(_redis, "exists MDP:id_%llu", client_id);
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
		
	}
	else
	{
		if((reply->type == REDIS_REPLY_INTEGER) && (reply->integer != 1))
		{
			XCP_LOGGER_INFO(&g_logger, "the client(%llu) isn't online.", client_id);
			err_info = "the client isn't online.";
			freeReplyObject(reply);
			return -1;
		}
    }
	freeReplyObject(reply);

	return nRet;

}



