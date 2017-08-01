
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

 

#ifndef _WORKER_H
#define _WORKER_H

#include "base/base_common.h"
#include "base/base_smart_ptr_t.h"
#include "base/base_thread_mutex.h"


USING_NS_BASE;


class Worker : public RefCounter
{
public:
	Worker();

	~Worker();
	
	int send_rsp(const std::string &method, const std::string &msg_tag, const int code, const std::string &msg="");
	
	void log();

	std::string to_string();
	
public:
	Thread_Mutex _mutex;
	
	std::string _id;  
	std::string _new_id; //[svr id]_[ip_port]
	std::string _svr_ip;
	unsigned short _svr_port;	
	int _fd;   //每个woker 有唯一的fd
	std::string _ip;
	unsigned short _port;
	unsigned long long _create_stmp;  //worker 注册时间戳
	unsigned long long _hb_stmp;      //worker 心跳更新时间戳
	unsigned int _client_num;
	
};

typedef Smart_Ptr_T<Worker>  Worker_Ptr;

#endif


