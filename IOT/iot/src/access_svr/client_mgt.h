
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

 

#ifndef _CLIENT_MGT_H
#define _CLIENT_MGT_H

#include "base/base_common.h"
#include "base/base_queue.h"
#include "base/base_singleton_t.h"
#include "base/base_thread_mutex.h"
#include "comm/common.h"

USING_NS_BASE;

/*
会话结构体Session 是在创建连接后就生成
客户端结构体StClient 在登录验证成功后才生成
会话在前客户端在后
会话和客户端是1:(0|1)的关系
*/

class Client_Mgt
{
public:
	Client_Mgt();

	~Client_Mgt();

	int register_client(const StClient stClient, std::string &err_info);

	//void unregister_client(int fd, const std::string &status=US_OFFLINE);
	void unregister_client(const std::string &session_id, const std::string &status=US_OFFLINE);

	//判断该链接是否已经登录验证过
	//bool is_auth(int fd);
	bool is_auth(const std::string &session_id);

	bool get_clients(unsigned long long id, std::map<std::string, StClient> &clients, std::string &err_info);

	void get_all_app(std::map<unsigned long long, std::map<std::string, StClient> > &apps);

	void get_all_router(std::map<unsigned long long, std::map<std::string, StClient> > &routers);
	
	unsigned int client_num();
		
private:
	Thread_Mutex _mutex;

	//app id   --- (session id --- client)
	std::map<unsigned long long, std::map<std::string, StClient> > _apps;
	//router id  --- (session id --- client)
	std::map<unsigned long long, std::map<std::string, StClient> > _routers;
	
	//session id --- client
	std::map<std::string, StClient> _clients;
	
};

#define PSGT_Client_Mgt Singleton_T<Client_Mgt>::getInstance()

#endif

