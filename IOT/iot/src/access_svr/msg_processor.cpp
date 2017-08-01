
#include "msg_processor.h"
#include "req_mgt.h"
#include "comm/common.h"
#include "protocol.h"
#include "base/base_net.h"
#include "base/base_string.h"
#include "base/base_logger.h"

#include "msg_oper.h"
#include "client_mgt.h"

extern Logger g_logger;
extern Req_Mgt *g_msg_mgt;

Msg_Processor::Msg_Processor()
{

}


Msg_Processor::~Msg_Processor()
{

}


int Msg_Processor::do_init(void *args)
{
	int nRet = 0;

	return nRet;
}


int Msg_Processor::svc()
{
	int nRet = 0;
	std::string err_info = "";
	
	//获取req 并且处理
	Request_Ptr req;
	nRet = g_msg_mgt->get_req(req);
	if(nRet != 0)
	{
		return 0;
	}
	
	XCP_LOGGER_INFO(&g_logger, "prepare to process msg:%s\n", req->to_string().c_str());

	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned int req_id = 0;
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%dm err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_app_stmp = timestamp;
	req->_req = XProtocol::add_msg_tag(req->_req, req->_msg_tag);

	if(method == CMD_MESSAGE)
	{
		XCP_LOGGER_INFO(&g_logger, "-------- message --------\n");

	}
	else if(method == CMD_NOTIFY)
	{
		XCP_LOGGER_INFO(&g_logger, "-------- notify --------\n");
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from message svr\n", method.c_str());
	}
	
	return 0;
	
}


