
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

#include "family_mgt.h"

USING_NS_BASE;


class XProtocol
{
public:
	static int req_head(const std::string &req, std::string &method, unsigned long long &timestamp, 
		unsigned int &req_id, unsigned long long &real_id, std::string &msg_tag, std::string &msg_encry, 
		std::string &msg_uuid, std::string &session_id,	std::string &err_info);

	static std::string rsp_msg(const std::string &method, const unsigned int req_id, const std::string &msg_tag,  
		const std::string &msg_encry, const std::string &msg_uuid, const std::string &session_id,
		const int code, const std::string &msg, const std::string &body="", bool is_object = false);

	static int admin_head(const std::string &req, std::string &method, unsigned long long &timestamp, unsigned long long &realId, std::string &msg_tag, int &code, std::string &msg, std::string &err_info);

	static std::string set_hb(const std::string &msg_tag, const unsigned int client_num);

	static std::string set_register(const std::string &msg_tag, const std::string &worker_id, 
		const std::string &ip, const std::string &ip_out, const unsigned short port);

	static std::string set_unregister(const std::string &msg_id, const std::string &worker_id);

	static std::string get_server_access_req(const std::string &msg_id);

	static int get_server_access_rsp(const std::string &req, std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info);

	static int get_rsp_result(const std::string &req, std::string &err_info);

	//uuid
	static std::string get_uuid_req(const std::string &msg_tag, std::string &err_info);
	static int get_uuid_rsp(const std::string &req, unsigned long long &uuid, std::string &err_info);

	// push
	static std::string get_push_req(const std::string &msg_tag, const std::string &method, const uint64 userId, const uint64 familyId, const uint64 operatorId);

	// param
	static int get_special_params(const std::string &req, const std::string &paramName, std::string &result, std::string &err_info);
	
	static int get_special_params(const std::string &req, const std::string &paramName, unsigned int &result, std::string &err_info);
	
	static int get_special_params(const std::string &req, const std::string &paramName, unsigned long long &result, std::string &err_info);

	static std::string add_family_result(const unsigned long long familyId, const std::string &token);
	
	static std::string get_family_info_result(const StFamilyInfo &info);

	static std::string get_apply_code_result(const std::string &code, const uint64 createAt, const uint64 expireAt);

	static std::string get_creat_member_result(const std::string &token);
	
	static std::string get_family_list_result(const std::vector<StFamilyInfo> &list);
	
	static std::string get_apply_list_result(const std::vector<StMemberApplyInfo> &list);
	
	static std::string get_apply_conut_result(const unsigned int count);
	
	static std::string get_member_info_result(const StFamilyMemberInfo &info);
	
	static std::string get_member_list_result(const std::vector<StFamilyMemberInfo> &list);
	
	static std::string get_member_id_list_result(const std::vector<unsigned long long> &list, const uint64 routerId);

	static std::string bind_router_result(const uint64 routerId);

};


#endif


