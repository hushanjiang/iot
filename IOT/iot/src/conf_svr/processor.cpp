
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

#include "msg_oper.h"
#include "conf_mgt.h"


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

	XCP_LOGGER_INFO(&g_logger, "prepare to process req from svr, req:%s\n", req->to_string().c_str());

	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned int req_id = 0;
	std::string msg_tag = "";
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, msg_tag, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, "null", 0, "null", ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_msg_tag = msg_tag;
	req->_app_stmp = timestamp;


	if(method == CMD_GET_SERVER_ACCESS)
	{
	
		XCP_LOGGER_INFO(&g_logger, "--- get server access ---\n");

		std::string svr_name = "";
		nRet = XProtocol::get_server_access_req(req->_req, svr_name, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
			Msg_Oper::send_msg(req->_fd, method, req_id, msg_tag, ERR_INVALID_REQ, err_info);
			return 0;
		}

		std::map<std::string, std::vector<StSvr> > svrs;
		nRet = PSGT_Conf_Mgt->get_server_access(svr_name, svrs, err_info);
		if(nRet != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, msg_tag, nRet, err_info);
			return 0;
		}
		
		Msg_Oper::send_msg(req->_fd, method, req_id, msg_tag, ERR_SUCCESS, "get server access success.", 
			XProtocol::get_server_access_rsp(svrs), true);
		
	}
	else if(method == CMD_GET_USER_RIGHT)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get user right ---\n");
		
	}
	else if(method == CMD_GET_SYS_CONFIG)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get system config ---\n");
		
		std::map<std::string, int> configs;
		std::string err_info;
		nRet = PSGT_Conf_Mgt->get_sys_config(configs, err_info);
		if(nRet != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, msg_tag, ERR_SUCCESS, "get system config success.", 
			XProtocol::get_sys_config_result(configs), true);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from app\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, method, req_id, msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;

}

