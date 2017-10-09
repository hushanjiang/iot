
#include "req_processor.h"
#include "req_mgt.h"
#include "comm/common.h"
#include "protocol.h"
#include "base/base_net.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_uid.h"
#include "base/base_utility.h"
#include "base/base_base64.h"
#include "base/base_cryptopp.h"
#include "base/base_openssl.h"

#include "msg_oper.h"
#include "conf_mgt.h"
#include "conn_mgt_lb.h"
#include "redis_mgt.h"
#include "session_mgt.h"
#include "client_mgt.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Req_Mgt *g_req_mgt;

extern Conn_Mgt_LB g_user_mgt_conn;
extern Conn_Mgt_LB g_device_mgt_conn;
extern Conn_Mgt_LB g_family_mgt_conn;
extern Conn_Mgt_LB g_mdp_conn;
extern Conn_Mgt_LB g_syn_conn;


Req_Processor::Req_Processor()
{

}


Req_Processor::~Req_Processor()
{

}



int Req_Processor::do_init(void *args)
{
	return 0;
}




int Req_Processor::svc()
{
	int nRet = 0;
	std::string err_info = "";
	
	//获取req 并且处理
	Request_Ptr req;
	nRet = g_req_mgt->get_req(req);
	if(nRet != 0)
	{
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "prepare to process req from app, %s\n", req->to_string().c_str());
	req->_process_stmp = getTimestamp();
		
	//获取该会话客户端id，同时判断该请求所属会话是否有效
	unsigned long long real_id = 0;
	if(!PSGT_Session_Mgt->get_client_id(req->_session_id, real_id))
	{
		XCP_LOGGER_INFO(&g_logger, "get client_id failed, the session is gone, req:%s\n", req->to_string().c_str());
		return 0;
	}
	XCP_LOGGER_INFO(&g_logger, "--- real_id:%llu ---\n", real_id);

	//---------------
	
	//解析iot_req
	std::string uuid = "";
	std::string encry = "";
	std::string content = "";
	nRet = XProtocol::iot_req(req->_req, uuid, encry, content, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "iot_req faield, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", "", 0, req->_msg_tag, ERR_SECURITY_HEAD_DECODE_FAILED, err_info);
		return 0;
	}
	
	XCP_LOGGER_INFO(&g_logger, "iot_req, uuid:%s, encry:%s, content:%s\n", uuid.c_str(), encry.c_str(), content.c_str());		

	//---------------

	std::string key = "";
	std::string new_req = "";
	if(encry == "true")
	{
		new_req = "";
		
		//base64解密
		X_BASE64 base64;
		char *tmp = NULL;
		unsigned int len = 0;
		if(!base64.decrypt(content.c_str(), tmp, len))
		{
			XCP_LOGGER_INFO(&g_logger, "base64 decrypt faield, content:%s, req:%s\n", content.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", "", 0, req->_msg_tag, ERR_BASE64_DECODE_FAILED, "base64 faield");
			return 0;
		}
		std::string buf(tmp, len);
		DELETE_POINTER_ARR(tmp);


		//获取安全通道key
		nRet = PSGT_Session_Mgt->get_security_channel(req->_session_id, uuid, key, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "get_security_channel failed, ret:%d, err_info:%s, uuid:%s, req:%s\n", 
				nRet, err_info.c_str(), uuid.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", "", 0, req->_msg_tag, ERR_GET_SECURITY_CHANNEL_FAILED, err_info);
			return 0;
		}
		XCP_LOGGER_INFO(&g_logger, "get_security_channel success, uuid:%s, key:%s\n", uuid.c_str(), key.c_str());		


		//aes cbc 解密
		SSL_AES aes;
		if(!aes.decrypt(key, buf, new_req, AES_CBC_IV))
		{
			XCP_LOGGER_INFO(&g_logger, "aes decrypt failed, ret:%d, key:%s, req:%s\n", nRet, key.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", "", 0, req->_msg_tag, ERR_DATA_DECRYPT_FAILED, "aes decrypt failed");
			return 0;
		}

		if(new_req.empty())
		{
			XCP_LOGGER_INFO(&g_logger, "aes decrypt exception, ret:%d, key:%s, req:%s\n", nRet, key.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", "", 0, req->_msg_tag, ERR_DATA_DECRYPT_FAILED, "aes decrypt exception");
			return 0;
		}
		
	}
	else
	{
		new_req = content;
	}
	
	//---------------

	XCP_LOGGER_INFO(&g_logger, "prepare to process new req(%u):%s, req:%s\n", new_req.size(), new_req.c_str(), req->to_string().c_str());
	req->_req = new_req;
	
	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned int req_id = 0;
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", "", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
		return 0;
	}
	req->_req_id = req_id;
	req->_app_stmp = timestamp;


	//处理接入服务器方法
	if (method == CMD_LOGOUT)
	{
		//客户端退出	
		XCP_LOGGER_INFO(&g_logger, "--- %s ---\n", method.c_str());
				
		if(!PSGT_Client_Mgt->is_auth(req->_session_id))
		{
			XCP_LOGGER_INFO(&g_logger, "the client isn't auth, req:%s\n", req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, 0, "the client isn't auth.");
			return 0;
		}
		
		Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, encry, key, method, req_id, req->_msg_tag, 0, "logout success.");

		PSGT_Client_Mgt->unregister_client(req->_session_id, US_LOGOUT);
		PSGT_Session_Mgt->set_client_id(req->_session_id, 0);

		return 0;
	}
	else if(method == CMD_CHECK_CLIENT_ONLINE)
	{
		//判断客户端是否在线
		XCP_LOGGER_INFO(&g_logger, "--- %s ---\n", method.c_str());

		unsigned long long client_id = 0;
		nRet = XProtocol::check_client_online_req(req->_req, client_id, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		nRet = PSGT_Redis_Mgt->check_client_online(client_id, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "check_client_online failed, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		}

		//返回给前端
		Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, encry, key, method, req_id, req->_msg_tag, nRet, err_info);

		return 0;
		
	}
	else
	{
		//no-todo
	}
	
	//添加请求头
	req->_req = XProtocol::add_inner_head(req->_req, req->_msg_tag, real_id, uuid, encry, 
		g_sysInfo._new_id, req->_session_id);

	std::string method_prefix = method.substr(0, 3);
	if(method_prefix == MP_UM)
	{
		//用户管理
		XCP_LOGGER_INFO(&g_logger, "--- user mgt:%s ---\n", method.c_str());

		//不允许在同一个长连接进行重复验证操作
		if((method == CMD_LOGIN_PWD) || (method == CMD_LOGIN_CODE) || (method == CMD_AUTH))
		{
			if(PSGT_Client_Mgt->is_auth(req->_session_id))
			{
				XCP_LOGGER_INFO(&g_logger, "the client is auth ago, req:%s\n", req->to_string().c_str());
				Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, 0, "the client is auth ago.");
				return 0;
			}
		}
			
		//获取user mgt conn
		Conn_Ptr user_conn;
		if(!g_user_mgt_conn.get_conn(user_conn))
		{
			XCP_LOGGER_INFO(&g_logger, "get user conn failed, req:%s\n", req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "no user mgt conn.");
			return 0;
		}
		
		nRet = user_conn->inner_message(req->_msg_tag, req->_req, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "send req to user mgt svr failed, ret:%d, err_info:%s, req:%s\n", 
				nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
		}
		XCP_LOGGER_INFO(&g_logger, "send req to user mgt svr success, req:%s\n", req->to_string().c_str());

	}
	else if (method_prefix == MP_FM)
	{
		//家庭管理
		XCP_LOGGER_INFO(&g_logger, "--- family mgt:%s ---\n", method.c_str());

		//获取familiy mgt conn
		Conn_Ptr family_conn;
		if(!g_family_mgt_conn.get_conn(family_conn))
		{
			XCP_LOGGER_INFO(&g_logger, "get family conn failed, req:%s\n", req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "no family mgt conn.");
			return 0;
		}
		
		nRet = family_conn->inner_message(req->_msg_tag, req->_req, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "send req to family mgt svr failed, ret:%d, err_info:%s, req:%s\n", 
				nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
			
		}
		XCP_LOGGER_INFO(&g_logger, "send req to family mgt svr success, req:%s\n", req->to_string().c_str());

	}
	else if (method_prefix == MP_DM)
	{
		//设备管理
		XCP_LOGGER_INFO(&g_logger, "--- device mgt:%s ---\n", method.c_str());

		//不允许在同一个长连接进行重复验证操作
		if(method == CMD_AUTH_ROUTER)
		{
			if(PSGT_Client_Mgt->is_auth(req->_session_id))
			{
				XCP_LOGGER_INFO(&g_logger, "the router is auth ago, req:%s\n", req->to_string().c_str());
				Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, nRet, "the router is auth ago.");
				return 0;
			}
		}

		//获取device mgt conn
		Conn_Ptr device_conn;
		if(!g_device_mgt_conn.get_conn(device_conn))
		{
			XCP_LOGGER_INFO(&g_logger, "get device conn failed, req:%s\n", req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "no device mgt conn.");
			return 0;
		}
		
		nRet = device_conn->inner_message(req->_msg_tag, req->_req, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "send req to device mgt svr failed, ret:%d, err_info:%s, req:%s\n", 
				nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
			
		}
		XCP_LOGGER_INFO(&g_logger, "send req to device mgt svr success, req:%s\n", req->to_string().c_str());

	}
	else if (method == CMD_MDP_MSG)
	{
		//MDP 消息
		XCP_LOGGER_INFO(&g_logger, "--- %s ---\n", method.c_str());

		//1. 检查本地client 是否存在目标id
		unsigned long long target_id = 0;
		std::string msg_type = "";
		nRet = XProtocol::to_mdp_req(req->_req, method, target_id, msg_type, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "mdp_msg_req failed, ret:%d, err_info:%s, req:%s\n", 
				nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
		}

		//2. 判断mdp 消息的有效性
		Conn_Ptr family_conn;
		if(!g_family_mgt_conn.get_conn(family_conn))
		{
			XCP_LOGGER_INFO(&g_logger, "get family conn failed, req:%s\n", req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "no family mgt conn.");
			return 0;
		}
		
		nRet = family_conn->check_talk_condition(req->_msg_tag, g_sysInfo._new_id, uuid, encry, req->_session_id, real_id, target_id, msg_type, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "---> check talk condition failed, ret:%d, err_info:%s, req:%s\n", 
				nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_CHECK_TALK_CONDITION_FAILED, err_info);
			return 0;
		}

		//获取一个mdp 链接
		bool mdp_conn_enable = true;
		Conn_Ptr mdp_conn;
		if(!g_mdp_conn.get_conn(mdp_conn))
		{
			//获取不到mdp conn 就无法路由到其他的access svr 上
			mdp_conn_enable = false;
			XCP_LOGGER_INFO(&g_logger, "get mdp conn failed, req:%s\n", req->to_string().c_str());
		}
		
		//3. 发送消息
		if((msg_type == MT_P2P) || (msg_type == MT_P2R) || (msg_type == MT_R2P))
		{
			//【单播】

			//添加mdp 消息头
			req->_req = XProtocol::add_mdp_msg_head(req->_req, target_id);
			
			//先发送给mdp
			if(mdp_conn_enable)
			{
				std::string _req = req->_req + std::string("\n");
				unsigned int len = _req.size();
				nRet = mdp_conn->send(_req.c_str(), len);
				if(nRet != 0)
				{
					XCP_LOGGER_INFO(&g_logger, "send to mdp failed, ret:%d, req:%s\n", nRet, req->_req.c_str());
				}
			}
			
			//发送给本地客户端
			std::map<std::string, StClient> clients;
			if(PSGT_Client_Mgt->get_clients(target_id, clients, err_info))
			{
				std::map<std::string, StClient>::iterator itr = clients.begin();
				for(; itr != clients.end(); itr++)
				{
					if(itr->second._session_id == req->_session_id)
					{
						//不发送给自己
						continue;
					}
					
					Msg_Oper::send_msg(itr->second._fd, itr->second._session_id, itr->second._uuid, encry, itr->second._key, req->_req);
				}
			}
			
		}
		else if((msg_type == MT_P2F) || (msg_type == MT_R2F))
		{
			//【组播】

			//获取家庭成员id 列表
			std::set<unsigned long long> members;
			nRet = family_conn->get_member_id_list(req->_msg_tag, g_sysInfo._new_id, uuid, encry, req->_session_id, real_id, target_id, members, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "get member id list failed, ret:%d, err_info:%s, req:%s\n", 
					nRet, err_info.c_str(), req->to_string().c_str());
				Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_GET_MEMBER_ID_LIST_FAILED, err_info);
				return 0;
			}

			std::set<unsigned long long>::iterator itr = members.begin();
			for(; itr != members.end(); itr++)
			{
				//添加mdp 消息头
				req->_req = XProtocol::add_mdp_msg_head(req->_req, *itr, target_id);

				//先发送给mdp
				if(mdp_conn_enable)
				{
					std::string _req = req->_req + std::string("\n");
					unsigned int len = _req.size();
					nRet = mdp_conn->send(_req.c_str(), len);
					if(nRet != 0)
					{
						XCP_LOGGER_INFO(&g_logger, "send to mdp failed, ret:%d, req:%s\n", nRet, req->_req.c_str());
					}
				}

				std::map<std::string, StClient> clients;
				if(PSGT_Client_Mgt->get_clients(*itr, clients, err_info))
				{
					std::map<std::string, StClient>::iterator itr1 = clients.begin();
					for(; itr1 != clients.end(); itr1++)
					{
						if(itr1->second._session_id == req->_session_id)
						{
							continue;
						}
						
						Msg_Oper::send_msg(itr1->second._fd, itr1->second._session_id, uuid, encry, key, req->_req);
					}
				}
				
			}
			
		}
		else
		{
			//no-todo
		}
		
	}	
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from app, req:%s\n", method.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, uuid, "false", "", method, req_id, req->_msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;
	
}


