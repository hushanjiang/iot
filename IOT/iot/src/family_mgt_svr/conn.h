
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

 

#ifndef _CONN_H
#define _CONN_H

#include "base/base_common.h"
#include "base/base_tcp_client_epoll.h"
#include "base/base_smart_ptr_t.h"
#include "comm/common.h"

USING_NS_BASE;

/*
Conn 既可以是同步连接也可以是异步连接。
异步连接的好处就是对端关闭，该Conn 会立即知道。
同步连接在对端关闭的情况下，不会立即知道，在使用该Conn发送后回收到对端的RST 报文，
再使用该Conn接收会返回0(表明该Conn 已经关闭)，会从Conn Mgt中标识不可用。后面不会从Conn Mgt中获取到。
*/
class Conn : public RefCounter
{
public:
	Conn(StSvr svr, Event_Handler *handler=NULL, bool asyn=false);

	~Conn();

	int connect();

	int check();

	void close();

	bool is_close();

	int send(const char *buf, unsigned int &len, int flags=0, unsigned int timeout=300000);

	int rcv(char *buf, unsigned int &len, unsigned int timeout=300000);

public:
	int get_server_access(std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info);

	int get_uuid(const std::string &msg_tag, unsigned long long &uuid, std::string &err_info);
	
	int push_msg(const std::string &msg_tag, const std::string &method, const unsigned long long user_id,
		const unsigned long long family_id, const unsigned long long operatorId, std::string &err_info);

		
public:
	StSvr _svr;
	unsigned long long _stmp_create;   //连接创建时间戳
	unsigned long long _stmp_hb;       //最后心跳时间戳
	
	TCP_Client_Epoll *_client;       //长连接这个已经释放了
	Event_Handler *_handler;
	bool _asyn;

	//是否已经注册
	bool _registered;
	
};

typedef Smart_Ptr_T<Conn> Conn_Ptr;

#endif


