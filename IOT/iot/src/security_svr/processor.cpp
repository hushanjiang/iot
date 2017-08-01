
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
#include "security_mgt.h"
#include "session_mgt.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;

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

	//------------------------------

	
	X_RSA_OAEP rsa_oaep;
	std::string new_req = "";
	if(!rsa_oaep.decrypt(std::string("../conf/iot_private.key"), req->_req, new_req))
	{
		XCP_LOGGER_INFO(&g_logger, "rsa_oaep decrypt failed, req:%s\n", req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, "invalid request, decrypt failed.");
		return 0;
	}
	req->_req = new_req;

	
	//------------------------------
	

	XCP_LOGGER_INFO(&g_logger, "prepare to process req from app, req:%s\n", new_req.c_str());

	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned int req_id = 0;
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_app_stmp = timestamp;

	if(method == CMD_CREATE_SECURITY_CHANNEL)
	{

		XCP_LOGGER_INFO(&g_logger, "--- create security channel ---\n");

		std::string id = "";
		std::string key = "";
		nRet = XProtocol::create_security_channel_req(req->_req, id, key, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_INVALID_REQ, err_info);
			return 0;
		}

		nRet = PSGT_Security_Mgt->create_security_channel(id, key, err_info);
		if(nRet != 0)
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
		}

		XCP_LOGGER_INFO(&g_logger, "seurity channel: %s|%s\n", id.c_str(), key.c_str());

		Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_SUCCESS, "create security channel.");

		
	}
	else if(method == CMD_REFRESH_SECURITY_CHANNEL)
	{

		XCP_LOGGER_INFO(&g_logger, "--- refresh security channel ---\n");

		std::string id = "";
		nRet = XProtocol::refresh_security_channel_req(req->_req, id, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_INVALID_REQ, err_info);
			return 0;
		}

		nRet = PSGT_Security_Mgt->refresh_security_channel(id, err_info);
		if(nRet != 0)
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
		}

		Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_SUCCESS, "refresh security channel.");

	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from app\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;

}


