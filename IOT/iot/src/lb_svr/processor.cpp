
#include "comm/common.h"
#include "protocol.h"
#include "base/base_net.h"
#include "base/base_string.h"
#include "base/base_args_parser.h"
#include "base/base_logger.h"

#include "processor.h"
#include "req_mgt.h"
#include "msg_oper.h"
#include "conf_mgt.h"
#include "worker_mgt.h"

extern Logger g_logger;

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
	
	XCP_LOGGER_INFO(&g_logger, "prepare to process req from app, %s\n", req->to_string().c_str());

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


	if(method == CMD_LB)
	{
		XCP_LOGGER_INFO(&g_logger, "-------- lb --------\n");
		
		//获取一个负载最低的worker
		Worker_Ptr worker;
		if(PSGT_Worker_Mgt->get_worker_new(worker))
		{
			XCP_LOGGER_INFO(&g_logger, "get valid access svr: %s\n", worker->to_string().c_str());
				
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_SUCCESS, "lb success", 
				XProtocol::lb_rsp(worker->_svr_ip, worker->_svr_port), true);
		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "no access svr is found.\n");
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_NO_WORKER_FOUND, "no access svr is found.");
		}
		
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from app\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;
	
}


