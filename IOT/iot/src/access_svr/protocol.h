
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

	static int rsp_head(const std::string &req, std::string &method, std::string &uuid, std::string &encry, std::string &session_id, unsigned int &req_id, std::string &err_info);

	static std::string rsp_msg(const std::string &uuid, const std::string &encry, const std::string &key, const std::string &method, 
		const unsigned int req_id, const std::string &msg_tag, const int code, const std::string &msg, const std::string &body="", bool is_object = false);

	static std::string rsp_msg(const std::string &uuid, const std::string &encry, const std::string &key, const std::string &buf);

	static int admin_head(const std::string &req, std::string &method, unsigned long long &timestamp, std::string &msg_tag, int &code, std::string &msg, std::string &err_info);

	//通道消息 event handler使用
	static int admin_head(const std::string &req, std::string &method, unsigned long long &timestamp, std::string &msg_tag, std::string &err_info);

	//获取用户登录结果
	static int get_user_login_result(const std::string &req, unsigned long long &user_id, std::string &err_info);

	//获取路由器登录结果
	static int get_router_login_result(const std::string &req, unsigned long long &router_id, std::string &err_info);
		
	//注册lb svr
	static std::string register_lb_req(const std::string &msg_tag, const std::string &worker_id, const std::string &ip, const unsigned short port);

	//注册mdp
	static std::string register_mdp_req(const std::string &msg_tag, const std::string &worker_id);

	//lb 心跳
	static std::string hb_lb_req(const std::string &msg_tag, const unsigned int client_num);

	//每个请求增加内部请求头
	static std::string add_inner_head(const std::string &req, const std::string &msg_tag, const unsigned long long real_id, const std::string &uuid, const std::string &encry, 
		const std::string &from_svr, const std::string &session_id);

	//添加组消息请求头
	static std::string add_mdp_msg_head(const std::string &req, const unsigned long long target_id, const unsigned long long group_id=0);

	//获取服务访问地址请求
	static std::string get_server_access_req(const std::string &msg_tag);
	static int get_server_access_rsp(const std::string &rsp, std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info);

	//客户端发送mdp消息，获取mdp 消息头
	static int to_mdp_req(const std::string &req, std::string &method, unsigned long long &target_id, std::string &msg_type, std::string &err_info);

	//来自mdp的消息
	static int from_mdp_req(std::string &req, std::string &method, std::string &sys_method, std::string &msg_type, unsigned long long &target_id, std::string &encry, std::string &err_info);

	static int get_rsp_result(const std::string &req, std::string &err_info);

	//判断用户是否可以聊天
	static std::string check_talk_condition_req(const std::string &msg_tag, const std::string &from_svr, const std::string &uuid, const std::string &encry, const std::string &session_id, 
		const unsigned long long src_id, const unsigned long long target_id, const std::string &msg_type);

	//获取家庭成员id 列表
	static std::string get_member_id_list_req(const std::string &msg_tag, const std::string &from_svr, const std::string &uuid, const std::string &encry, const std::string &session_id,
		const unsigned long long user_id, const unsigned long long family_id);
	static int get_member_id_list_rsp(const std::string &rsp, std::set<unsigned long long> &members, std::string &err_info);	

	//通知客户端状态
	static std::string notify_client_status_req(const std::string &msg_tag, unsigned long long id, const std::string &type, const std::string &status);

	//判断客户端是否在线
	static int check_client_online_req(std::string &req, unsigned long long &client_id, std::string &err_info);
	
};


#endif


