
/**
* Licensed to the Apache Software Foundation (ASF) under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The ASF licenses this file to You under the Apache License, Version 2.0
* (the "License"); you may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: 89617663@qq.com
*/


#ifndef _REDIS_CONN_H
#define _REDIS_CONN_H

#include "base/base_smart_ptr_t.h"
#include <hiredis/hiredis.h>
#include "base/base_singleton_t.h"
#include "comm/common.h"

USING_NS_BASE;

// 默认超时时间为3天
const unsigned int REDIS_COMMON_EXPIRTE	= 3 * 24 * 60 *60;

//mysql 连接类
class redis_conn : public RefCounter
{
public:
	redis_conn(int seq);

	~redis_conn();

	int connect_conn(const std::string &ip, unsigned int port,const std::string &pwd);
	int reconnect();
	void release_conn();

	//通过ping 对mysql 长连接进行检查
	bool ping();
public:
	int set_int(const std::string &id, const unsigned long long &value, const unsigned int ttl = REDIS_COMMON_EXPIRTE);

	int get_int(const std::string &id, unsigned long long &value);
	
	int set_string(const std::string &id, const std::string &value, const unsigned int ttl = REDIS_COMMON_EXPIRTE);
	
	int get_string(const std::string &id, std::string &value);

	int hset_int(const std::string &id, const std::string &key, const unsigned long long &value);
	
	int hset_string(const std::string &id, const std::string &key, const std::string &value);

	int hget_int(const std::string &id, const std::string &key, unsigned long long &value);
	
	int hget_string(const std::string &id, const std::string &key, std::string &value);

	int hmset_array(const std::string &id, const std::vector<std::string> &keys, std::vector<std::string> &values, const unsigned int ttl = REDIS_COMMON_EXPIRTE);

	int hmget_array(const std::string &id, const std::vector<std::string> &keys, std::vector<std::string> &values);

	int sadd_int(const std::string &id, const unsigned long long &value);
	
	int sadd_int_with_special_symbol(const std::string &id, const unsigned long long &value);

	int sremove_int(const std::string &id, const unsigned long long &value);

	int sget_all(const std::string &id, std::vector<unsigned long long> &values, unsigned long long &specialId);

	int remove(const std::string &id);
	
	bool exists(const std::string &id);

public:
	redisContext *_redis;
	bool _valid;
	std::string  _ip;
	int _port;
	std::string  _pwd;
	bool _conn;   //是否已经连接
	bool _used;   //是否正在使用
	int _seq;

};
typedef Smart_Ptr_T<redis_conn>  redis_conn_Ptr;

#endif

