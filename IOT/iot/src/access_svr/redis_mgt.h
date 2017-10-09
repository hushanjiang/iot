
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
 

#ifndef _REDIS_MGT_H
#define _REDIS_MGT_H

#include "base/base_thread_mutex.h"
#include <hiredis/hiredis.h>
#include "base/base_singleton_t.h"
#include "comm/common.h"


USING_NS_BASE;

class Redis_Mgt
{
public:
    Redis_Mgt();
    virtual ~Redis_Mgt();

	int init(const std::string &ip, const unsigned int port, const std::string &auth);

	void release();

	void check();	

	int connect();

	int ping();

public:
	int register_route(const unsigned long long id, const std::string &access_svr_id, std::string &err_info);

	int unregister_route(const unsigned long long id, const std::string &access_svr_id, std::string &err_info);

	int get_client_list(const std::string &access_svr_id, const unsigned long long begin, const unsigned int cnt, std::deque<unsigned long long> &clients, std::string &err_info);

	int get_access_svr_list(std::set<std::string> &svrs, std::string &err_info);
	
	int get_security_channel(const std::string &id, std::string &key, std::string &err_info);	

	int check_client_online(const unsigned long long client_id, std::string &err_info);

private:
	redisContext *_redis;  //需要确认是否是线程安全的
	Thread_Mutex _mutex;
	bool _valid;
	std::string _ip;
	unsigned short _port;
	std::string _auth;
	
};

#define PSGT_Redis_Mgt Singleton_T<Redis_Mgt>::getInstance()

#endif

