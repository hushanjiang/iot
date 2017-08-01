
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
 

#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include "base/base_common.h"
#include "comm/common.h"

USING_NS_BASE;


class XProtocol
{
public:
	static int iot_req(const std::string &req, std::string &uuid, std::string &encry, std::string &content, std::string &err_info);
	
	static int req_head(const std::string &req, std::string &method, unsigned long long &timestamp, unsigned int &req_id, std::string &err_info);

	static std::string rsp_msg(const std::string &method, const unsigned int req_id, const std::string &msg_tag,  
		const int code, const std::string &msg, const std::string &body="", bool is_object = false);

	static int admin_head(const std::string &req, std::string &method, unsigned long long &timestamp, std::string &msg_tag, int &code, std::string &msg, std::string &err_info);

	static std::string set_hb(const std::string &msg_tag, const unsigned int client_num);

	static std::string set_register(const std::string &msg_tag, const std::string &worker_id, 
		const std::string &ip, const std::string &ip_out, const unsigned short port);

	static std::string set_unregister(const std::string &msg_id, const std::string &worker_id);

	static std::string add_msg_tag(const std::string &req, const std::string &msg_tag);

	static std::string add_id(const std::string &req, const unsigned long long real_id);

	static std::string get_server_access_req(const std::string &msg_id);
	static int get_server_access_rsp(const std::string &req, std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info);
	
};


#endif


