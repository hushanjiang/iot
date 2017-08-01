
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
#include "device_mgt.h"

USING_NS_BASE;


class XProtocol
{
public:
	static int req_head(const std::string &req, std::string &method, unsigned long long &timestamp, unsigned int &req_id, unsigned long long &real_id, std::string &msg_tag, std::string &err_info);

	static std::string rsp_msg(const std::string &method, const unsigned int req_id, const std::string &msg_tag,  
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

	static int get_special_params(const std::string &req, const std::string &paramName, std::string &result);
	static int get_special_params(const std::string &req, const std::string &paramName, unsigned int  &result);
	static int get_special_params(const std::string &req, const std::string &paramName, unsigned long long &result);
	static int get_page_params(const std::string &req, unsigned int &size, unsigned int &beginAt);
	static int get_room_orders_params(const std::string &req, std::vector<StRoomInfo> &infos);

	static std::string add_room_result(const unsigned long long &roomId);
	static std::string add_device_result(const unsigned long long &deviceId);
	static std::string get_room_info_result(const StRoomInfo &info);
	static std::string get_device_info_result(const StDeviceInfo &info);
	static std::string get_device_status_result(const StDeviceStatus &status);
	static std::string get_device_alert_result(const StDeviceAlert &alert);
	static std::string get_router_auth_result(const uint64 &routerId);
	static std::string get_router_info_result(const StRouterInfo &info);
	static std::string get_room_list_result(const std::vector<StRoomInfo> &infos);
	static std::string get_device_list_result(const std::vector<StDeviceInfo> &infos, const unsigned int &left);
	static std::string get_status_list_result(const std::vector<StDeviceStatus> &status, const unsigned int &left);
	static std::string get_alert_list_result(const std::vector<StDeviceAlert> &alerts, const unsigned int &left);

};


#endif


