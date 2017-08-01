
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
	
	XCP_LOGGER_INFO(&g_logger, "prepare to process req from svr, %s\n", req->to_string().c_str());

	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned int req_id = 0;
	std::string msg_tag = "";
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, msg_tag, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, "null", 0, "null", ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_msg_tag = msg_tag;
	req->_app_stmp = timestamp;

	if(method == CMD_SMS)
	{
		XCP_LOGGER_INFO(&g_logger, "--- sms ---\n");
		
	}	
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from svr\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, method, req_id, msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;

}


