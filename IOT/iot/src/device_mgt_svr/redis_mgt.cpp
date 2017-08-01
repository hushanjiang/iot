
#include "redis_mgt.h"
#include "redis_timer_handler.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_convert.h"

extern Logger g_logger;


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




int Redis_Mgt::create_security_channel(const std::string &id, std::string &key, std::string &err_info)
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
    reply = (redisReply*)redisCommand(_redis, "hmset %s key %s created_at %llu", 
    	id.c_str(), key.c_str(), getTimestamp());
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		err_info = "reply is null, pls check redis server.";
		release();
		return -1;
		
	}
	freeReplyObject(reply);
	

	//设置TTL 
    reply = (redisReply*)redisCommand(_redis, "expire %s %u", id.c_str(), SECURITY_CHANNEL_TTL);
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
			XCP_LOGGER_INFO(&g_logger, "expire security channel id failed, id:%s\n", id.c_str());
			err_info = "expire security channel id failed.";			
		}
	}	
	freeReplyObject(reply);
	
	return nRet;

}




int Redis_Mgt::refresh_security_channel(const std::string &id, std::string &err_info)
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
    reply = (redisReply*)redisCommand(_redis, "expire %s %u", id.c_str(), SECURITY_CHANNEL_TTL);
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
			XCP_LOGGER_INFO(&g_logger, "expire security channel id failed, id:%s\n", id.c_str());
			err_info = "expire security channel id failed.";
			nRet = -1;

		}
	}
	freeReplyObject(reply);
	
	return nRet;

}

int Redis_Mgt::release_security_channel(const std::string &id, std::string &err_info)	
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
    reply = (redisReply*)redisCommand(_redis, "del %s", id.c_str());
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
			XCP_LOGGER_INFO(&g_logger, "release security channel id failed, id:%s\n", id.c_str());
			err_info = "expire security channel id failed.";
			nRet = -1;

		}
	}
	freeReplyObject(reply);
	
	return nRet;

}


int Redis_Mgt::get(const std::string &id, unsigned long long &value)
{
	int nRet = 0;
	
	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "get %s", id.c_str());
	if(reply && (reply->type == REDIS_REPLY_STRING))
	{
		value = (unsigned long long)atoi(reply->str);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		nRet = -1;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int Redis_Mgt::set(const std::string &id, const unsigned long long &value)
{
	int nRet = 0;
	
	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "set %s %llu", id.c_str(), value);
	if(reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"OK") == 0))
	{
		
	}
	else
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release();
		nRet = -1;
	}
	freeReplyObject(reply);
	
	return nRet;
}


int Redis_Mgt::hset(const std::string &id, const std::string &key, const std::string &value, std::string &err_info)
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
    reply = (redisReply*)redisCommand(_redis, "hset %s %s %s", id.c_str(), key.c_str(), value.c_str());
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
		if((reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"OK") != 0))
		{
			XCP_LOGGER_INFO(&g_logger, "release security channel id failed, id:%s\n", id.c_str());
			err_info = "expire security channel id failed.";
			nRet = -1;

		}
	}
	freeReplyObject(reply);
	
	return nRet;
}


int Redis_Mgt::hget(const std::string &id, const std::string &key, std::string &value, std::string &err_info)
{
	int nRet = -1;
	err_info = "";
	
	Thread_Mutex_Guard guard(_mutex);
	
    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "hget %s %s", id.c_str(), key.c_str(), value.c_str());
	if(reply && reply->type == REDIS_REPLY_STRING)
	{
		value = reply->str;
		nRet = 0;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "radius server cannot find value(id:%s, key:%s)\n", id.c_str(), key.c_str());
		err_info = "id or key is not exists.";
		return -1;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int Redis_Mgt::remove(const std::string &id)
{
	int nRet = 0;
	Thread_Mutex_Guard guard(_mutex);
	XCP_LOGGER_INFO(&g_logger, "remove %s from redis\n", id.c_str());
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "del %s", id.c_str());
	if(reply && (reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
	{
		XCP_LOGGER_INFO(&g_logger, "remove id(%s) from redis", id.c_str());
	}
	else if(reply == NULL)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release();
		return -1;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "reply invalid, type:%d, value:%d.\n", reply->type, reply->integer);
		nRet = -1;
	}
	freeReplyObject(reply);
	
	return nRet;
}


bool Redis_Mgt::isExist(const std::string &id)
{
	int bRet = false;
	
	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "exists %s", id.c_str());
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release();
		return -1;
		
	}
	else
	{
		if((reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
		{
			bRet = true;
		}
	}
	freeReplyObject(reply);
	
	return bRet;
}

bool Redis_Mgt::isExist(const std::string &id, const std::string &key)
{
	int bRet = false;
	
	Thread_Mutex_Guard guard(_mutex);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "hexists %s %s", id.c_str(), key.c_str());
	if(!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release();
		return -1;
		
	}
	else
	{
		if((reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
		{
			bRet = true;
		}
	}
	freeReplyObject(reply);
	
	return bRet;
}


