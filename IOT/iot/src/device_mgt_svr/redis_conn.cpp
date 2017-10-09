
#include <math.h>
#include <sys/time.h>
#include "redis_conn.h"
#include "base/base_convert.h"
#include "base/base_logger.h"
#include "comm/common.h"

extern Logger g_logger;

const unsigned int MAX_REDIS_COMMAND_LENGTH		= 1024;

const char REDIS_SPECIAL_ID_SYMBOL = '@';

//mysql conn
redis_conn::redis_conn(int seq) :_valid(false), _ip(""), _port(6379),_pwd(""),_conn(false), _used(false), _seq(seq)
{

}

redis_conn::~redis_conn()
{


}

int redis_conn::connect_conn(const std::string &ip, unsigned int port, const std::string &pwd)
{
	int nRet = 0;
	_ip = ip;
	_port = port;
	_pwd = pwd;
	/*
	连接创建以后内部生成一个 redisContext 堆对象，一旦连接断开需要重新连接，
	没有自动重连机制
	*/
	struct timeval timeout = { 0, 300000 };
	_redis = redisConnectWithTimeout(_ip.c_str(), _port, timeout);
	if (_redis == NULL || _redis->err)
	{
		if (_redis)
		{
			XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) failed, err:%s\n",
				_ip.c_str(), _port, _redis->errstr);
			release_conn();

		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) failed, can't allocate redis context\n",
				_ip.c_str(), _port);
		}

		return -1;
	}

	XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) success.\n", _ip.c_str(), _port);

	if (!_pwd.empty())
	{
		redisReply *reply = NULL;
		reply = (redisReply*)redisCommand(_redis, "auth %s", _pwd.c_str());
		if (!reply)
		{
			//如果redis server 停止，reply is null
			XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
			release_conn();
			return -2;
		}
		else
		{
			if (reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str, "OK") == 0))
			{
				XCP_LOGGER_INFO(&g_logger, "auth redis server(%s:%u) success.\n", _ip.c_str(), _port);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "auth redis failed, err:%s, pls check redis server(%s:%u), auth:%s\n",
					reply->str, _ip.c_str(), _port, _pwd.c_str());
				release_conn();
				return -3;
			}
		}

	}
	_valid = true;
	return nRet;

}

int redis_conn::reconnect()
{
	
	/*
	连接创建以后内部生成一个 redisContext 堆对象，一旦连接断开需要重新连接，
	没有自动重连机制
	*/
	int nRet = 0;
	struct timeval timeout = { 0, 300000 };
	_redis = redisConnectWithTimeout(_ip.c_str(), _port, timeout);
	if (_redis == NULL || _redis->err)
	{
		if (_redis)
		{
			XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) failed, err:%s\n",
				_ip.c_str(), _port, _redis->errstr);
			release_conn();

		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) failed, can't allocate redis context\n",
				_ip.c_str(), _port);
		}

		return -1;
	}

	XCP_LOGGER_INFO(&g_logger, "connect redis(%s:%u) success.\n", _ip.c_str(), _port);

	if (!_pwd.empty())
	{
		redisReply *reply = NULL;
		reply = (redisReply*)redisCommand(_redis, "auth %s", _pwd.c_str());
		if (!reply)
		{
			//如果redis server 停止，reply is null
			XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
			release_conn();
			return -2;
		}
		else
		{
			if (reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str, "OK") == 0))
			{
				XCP_LOGGER_INFO(&g_logger, "auth redis server(%s:%u) success.\n", _ip.c_str(), _port);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "auth redis failed, err:%s, pls check redis server(%s:%u), auth:%s\n",
					reply->str, _ip.c_str(), _port, _pwd.c_str());
				release_conn();
				return -3;
			}
		}

	}
	_valid = true;
	return nRet;
}

void redis_conn::release_conn()
{
	redisFree(_redis);
	_redis = NULL;
	_valid = false;
}



//通过ping 对redis 长连接进行检查
bool redis_conn::ping()
{
	int nRet = 0;

	if (!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n",
			_ip.c_str(), _port);

		return -1;
	}

	redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "ping");
	if (!reply)
	{
		//如果redis server 停止，reply is null
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u)\n",
			_ip.c_str(), _port);
		nRet = -2;

		release_conn();
	}
	else
	{
		if (reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str, "PONG") == 0))
		{
			//XCP_LOGGER_INFO(&g_logger, "ping redis server(%s:%u) success.\n", _ip.c_str(), _port);
		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "ping redis failed, err:%s, pls check redis server(%s:%u)\n",
				reply->str, _ip.c_str(), _port);
			nRet = -3;

			release_conn();
		}
	}

	freeReplyObject(reply);

	return nRet;

}

int redis_conn::hget_int(const std::string &id, const std::string &key, unsigned long long &value)
{
	int nRet = 0;
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}
    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "hmget %s %s", id.c_str(), key.c_str());
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -2;
	}
	if(reply && (reply->type == REDIS_REPLY_ARRAY) && (reply->elements >= 1) && (reply->element[0]->type == REDIS_REPLY_STRING))
	{
		value = (unsigned long long)atoi(reply->element[0]->str);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::hget_string(const std::string &id, const std::string &key, std::string &value)
{
	int nRet = 0;
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}
	
    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "hmget %s %s", id.c_str(), key.c_str());
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -2;
	}
	if(reply && (reply->type == REDIS_REPLY_ARRAY) && (reply->elements >= 1) && (reply->element[0]->type == REDIS_REPLY_STRING))
	{
		value = reply->element[0]->str;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "get %s:%s failed\n", id.c_str(), key.c_str());
		nRet = -3;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::get_int(const std::string &id, unsigned long long &value)
{
	int nRet = 0;	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "get %s", id.c_str());
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -2;
	}
	if(reply && (reply->type == REDIS_REPLY_STRING))
	{
		value = (unsigned long long)atoi(reply->str);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "key(%s) is not exist in redis\n", id.c_str());
		nRet = -3;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::set_int(const std::string &id, const unsigned long long &value, const unsigned int ttl)
{
	int nRet = 0;
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "set %s %llu ex %d", id.c_str(), value, ttl);
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"OK") == 0))
	{
	}
	else
	{	
		XCP_LOGGER_ERROR(&g_logger, "redisContext is invalid!(%s:%u).\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::get_string(const std::string &id, std::string &value)
{
	int nRet = 0;
	value = "";
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

	redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "get %s", id.c_str());
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_STRING))
	{
		value = reply->str;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::set_string(const std::string &id, const std::string &value, const unsigned int ttl)
{
	int nRet = 0;
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "set %s %s ex %d", id.c_str(), value.c_str(), ttl);
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"OK") == 0))
	{
		
	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "redisContext is invalid, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}


int redis_conn::hset_int(const std::string &id, const std::string &key, const unsigned long long &value)
{
	int nRet = 0;
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "hmset %s %s %llu", id.c_str(), key.c_str(), value);
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"OK") == 0))
	{
		
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}


int redis_conn::hset_string(const std::string &id, const std::string &key, const std::string &value)
{
	int nRet = 0;
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "hmset %s %s %s", id.c_str(), key.c_str(), value.c_str());
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_STATUS) && (strcasecmp(reply->str,"OK") == 0))
	{
		
	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "redisContext is invalid, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::sadd_int(const std::string &id, const unsigned long long &value)
{
	int nRet = 0;
	XCP_LOGGER_INFO(&g_logger, "add %s:%llu into redis\n", id.c_str(), value);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "sadd %s %llu", id.c_str(), value);
	if(!reply)
	{
		XCP_LOGGER_ERROR(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
	{
		XCP_LOGGER_INFO(&g_logger, "add to redis success\n", id.c_str(), value);
	}else{
		XCP_LOGGER_ERROR(&g_logger, "redisContext is invalid, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::sadd_int_with_special_symbol(const std::string &id, const unsigned long long &value)
{
	int nRet = 0;
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

	char buf[25] = {0};
	memset(buf, 0x0, 25); 
	snprintf(buf, 25, "%c%llu", REDIS_SPECIAL_ID_SYMBOL, value);
	XCP_LOGGER_INFO(&g_logger, "add %s:%s into redis\n", id.c_str(), buf);

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "sadd %s %s", id.c_str(), buf);
	if(!reply)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
	{
		XCP_LOGGER_INFO(&g_logger, "add to redis success\n", id.c_str(), value);
	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "redisContext is invalid, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}


int redis_conn::sremove_int(const std::string &id, const unsigned long long &value)
{
	int nRet = 0;
	XCP_LOGGER_INFO(&g_logger, "remove %s:%llu from redis\n", id.c_str(), value);
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "srem %s %llu", id.c_str(), value);
	if(!reply)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -1;
	}
	if(reply && (reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
	{
		XCP_LOGGER_INFO(&g_logger, "remove success\n", id.c_str(), value);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "reply invalid, type:%d, value:%d.\n", reply->type, reply->integer);
		nRet = -2;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::hmget_array(const std::string &id, const std::vector<std::string> &keys, std::vector<std::string> &values)
{
	int nRet = 0;
	values.clear();
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

	if (id.empty() || keys.size() == 0)
	{
		return -1;
	}

    size_t len = keys.size() + 2;
	char ** argv = new char*[len];
	size_t * argvlen = new size_t[len];

	int j = 0;
	argvlen[j] = 5;
	argv[j] = new char[6];
	memset((void *)argv[j], 0, argvlen[j]);
	memcpy(argv[j], "hmget", 5);
	j++;
	argv[j] = new char[id.size() + 1];
	argvlen[j] = id.size();
	memset((void *)argv[j], 0, argvlen[j]);
	memcpy(argv[j], id.data(), id.size());
	j++;

	for (size_t i = 0; i < keys.size(); ++i)
	{
		argvlen[j] = keys[i].size();
		argv[j] = new char[argvlen[j] + 1];
		memset((void *)argv[j], 0, argvlen[j]);
		memcpy(argv[j], keys[i].data(), keys[i].size());
		j++;
	}
	redisReply *reply = (redisReply *)redisCommandArgv(_redis, len, const_cast<const char**>(argv), argvlen);

	for (size_t i = 0; i < len; i++)
	{
		delete[] argv[i];
		argv[i] = NULL;
	}
	delete[]argv;
	delete[]argvlen;
	argv = NULL;

	if (!reply)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -3;
	}
	if (reply->type == REDIS_REPLY_NIL)
	{
		freeReplyObject(reply);
		return -1;
	}
	bool bExit = false;
	if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 0)
	{

		for (size_t i = 0; i < reply->elements && i < keys.size(); i++)
		{
			std::string value;
			if (reply->element[i]->type == REDIS_REPLY_INTEGER && reply->integer == -1)
			{
				value = "";
			}
			else if (reply->element[i]->type == REDIS_REPLY_STRING)
			{
				value = reply->element[i]->str;
				bExit = true;
			}
			values.push_back(value);
		}

	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "redis response error!reply->type=%d\n", reply->type);
		nRet = -4;
	}
	freeReplyObject(reply);
	
	if (bExit == false)
	{
		return -1;
	}
	
	return nRet;
}

int redis_conn::hmset_array(const std::string &id, const std::vector<std::string> &keys, std::vector<std::string> &values, const unsigned int ttl)
{
	struct timeval begin, end;
	gettimeofday(&begin, NULL);

	int nRet = 0;
	if (id.empty() || keys.size() == 0 || keys.size() != values.size())
	{
		XCP_LOGGER_ERROR(&g_logger, "invalid params, keys size:%d, values size:%d\n", keys.size(), values.size());
		return -1;
	}
	if (!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -3;
	}

	size_t len = keys.size() * 2 + 2;
	char ** argv = new char*[len];
	size_t * argvlen = new size_t[len];

	int j = 0;
	argvlen[j] = 5;
	argv[j] = new char[6];
	memset((void *)argv[j], 0, argvlen[j]);
	memcpy(argv[j], "hmset", 5);
	j++;
	argv[j] = new char[id.size() + 1];
	argvlen[j] = id.size();
	memset((void *)argv[j], 0, argvlen[j]);
	memcpy(argv[j], id.data(), id.size());
	j++;

	for (size_t i = 0; i < keys.size(); ++i)
	{
		argvlen[j] = keys[i].size();
		argv[j] = new char[argvlen[j] + 1];
		memset((void *)argv[j], 0, argvlen[j]);
		memcpy(argv[j], keys[i].data(), keys[i].size());
		j++;

		argvlen[j] = values[i].size();
		argv[j] = new char[argvlen[j] + 1];
		memset((void *)argv[j], 0, argvlen[j]);
		memcpy(argv[j], values[i].data(), values[i].size());
		j++;

	}
	redisReply *reply = (redisReply *)redisCommandArgv(_redis, len, const_cast<const char**>(argv), argvlen);

	for (size_t i = 0; i < len; i++)
	{
		delete[] argv[i];
		argv[i] = NULL;
	}
	delete[]argv;
	delete[]argvlen;
	argv = NULL;

	if (!reply)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -5;

	}
	if ((reply->type == REDIS_REPLY_INTEGER) && (reply->integer != 1))
	{
		XCP_LOGGER_INFO(&g_logger, "redis set fail!id:%s,keys.size()=%zu\n", id.c_str(), keys.size());
		freeReplyObject(reply);
		return -6;
	}

	
	freeReplyObject(reply);
	reply = (redisReply*)redisCommand(_redis, "expire %s %d", id.c_str(), ttl);
	if (!reply)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -6;
	}
	else
	{
		if ((reply->type == REDIS_REPLY_INTEGER) && (reply->integer != 1))
		{
			XCP_LOGGER_INFO(&g_logger, "redis set fail!id:%s\n", id.c_str());
			freeReplyObject(reply);
			return -8;
		}
	}

	freeReplyObject(reply);
	return nRet;
}

int redis_conn::sget_all(const std::string &id, std::vector<unsigned long long> &values, unsigned long long &specialId)
{
	int nRet = 0;
	values.clear();
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "sdiff %s", id.c_str());
	if (!reply)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -2;
	}
	if(reply && (reply->type == REDIS_REPLY_ARRAY))
	{
		for(unsigned int i=0; i<reply->elements; i++)
		{
			char* data = reply->element[i]->str;
			if(data[0] == REDIS_SPECIAL_ID_SYMBOL)
			{
				specialId = (unsigned long long)atoi(data + 1);
				values.push_back(specialId);
			}
			else
			{
				values.push_back((unsigned long long)atoi(data));
			}
		}
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		release_conn();
		nRet = -3;
	}
	freeReplyObject(reply);
	
	return nRet;
}

int redis_conn::remove(const std::string &id)
{
	int nRet = 0;
	XCP_LOGGER_INFO(&g_logger, "remove %s from redis\n", id.c_str());
	
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
    reply = (redisReply*)redisCommand(_redis, "del %s", id.c_str());
	if(reply == NULL)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -2;
	}
	else if((reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
	{
		XCP_LOGGER_INFO(&g_logger, "remove id(%s) from redis\n", id.c_str());
	}
	else if((reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 0))
	{
		XCP_LOGGER_INFO(&g_logger, "remove id(%s) from redis but it is not exist\n", id.c_str());
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "reply invalid, type:%d, value:%d.\n", reply->type, reply->integer);
		nRet = -3;
	}
	freeReplyObject(reply);
	
	return nRet;
}

bool redis_conn::exists(const std::string &id)
{
	if(!_valid || !_redis)
	{
		XCP_LOGGER_INFO(&g_logger, "redisContext is invalid, pls check redis server(%s:%u)\n", _ip.c_str(), _port);
		return -1;
	}

    redisReply *reply = NULL;
	reply = (redisReply*)redisCommand(_redis, "exists %s", id.c_str());
	if(reply == NULL)
	{
		XCP_LOGGER_INFO(&g_logger, "reply is null, pls check redis server(%s:%u).\n", _ip.c_str(), _port);
		release_conn();
		return -2;
	}
	if(reply && (reply->type == REDIS_REPLY_INTEGER) && (reply->integer == 1))
	{
		freeReplyObject(reply);
		return true;
	}
	
	freeReplyObject(reply);
	return false;
}
