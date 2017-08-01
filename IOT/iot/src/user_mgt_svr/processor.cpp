
#include "processor.h"
#include "req_mgt.h"
#include "comm/common.h"
#include "protocol.h"
#include "base/base_net.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_url_parser.h"
#include "base/base_uid.h"
#include "base/base_utility.h"
#include "base/base_base64.h"
#include "base/base_cryptopp.h"

#include "msg_oper.h"
#include "conf_mgt.h"
#include "conn_mgt_lb.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Conn_Mgt_LB g_uid_conn;

Processor::Processor()
{

}


Processor::~Processor()
{

}



int Processor::do_init(void *args)
{
	int nRet = 0;

	return nRet;
}




int Processor::svc()
{
	int nRet = 0;
	std::string err_info = "";
	
	//获取req 并且处理
	Request_Ptr req;
	nRet = PSGT_Req_Mgt->get_req(req);
	if(nRet != 0)
	{
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "prepare to process req from access svr:%s\n", req->to_string().c_str());

	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned int req_id = 0;
	std::string msg_tag = "";
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, msg_tag, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, "null", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_app_stmp = timestamp;
	req->_msg_tag = msg_tag;

	if(method == CMD_REGISTER_USER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- register user ---\n");
		
		//获取uuid
		Conn_Ptr uid_conn;
		if(!g_uid_conn.get_conn(uid_conn))
		{
			XCP_LOGGER_INFO(&g_logger, "get uid conn failed.\n");
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "get uid conn failed.");
			return 0;
		}
		
		unsigned long long uuid = 0;
		nRet = uid_conn->get_uuid(req->_msg_tag, uuid, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "get uid failed, ret:%d, err_info:%s\n", nRet, err_info.c_str());
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
		}

		XCP_LOGGER_INFO(&g_logger, "get uid success, uuid:%llu\n", uuid);

		//创建用户存储mysql
		err_info = "register user success!";
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);		
		
		
	}
	else if(method == CMD_LOGIN_PWD)
	{
		XCP_LOGGER_INFO(&g_logger, "--- login_pwd ---\n");

		err_info = "login success!";
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		
	}
	else if(method == CMD_LOGIN_CODE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- login_code ---\n");
	}	

	else if(method == CMD_AUTH)
	{
		XCP_LOGGER_INFO(&g_logger, "--- auth ---\n");
		
	}
	else if(method == CMD_PHONE_CODE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- phone code ---\n");
		
	}
	else if(method == CMD_SET_PWD)
	{
		XCP_LOGGER_INFO(&g_logger, "--- phone code ---\n");
	}
	else if(method == CMD_RESET_PWD)
	{
		XCP_LOGGER_INFO(&g_logger, "--- reset pwd ---\n");
	}
	else if(method == CMD_GET_USER_PROFILE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get user profile ---\n");
	}
	else if(method == CMD_UPDATE_USER_PROFILE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update user profile ---\n");
	}
	else if(method == CMD_REFRESH_USER_TOKEN)
	{
		XCP_LOGGER_INFO(&g_logger, "--- refresh user token ---\n");
	}		
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from access svr\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;

}


