
#include "msg_oper.h"
#include "base/base_socket_oper.h"
#include "base/base_logger.h"
#include "protocol.h"


extern Logger g_logger;

Thread_Mutex Msg_Oper::_mutex_msg;


int Msg_Oper::send_msg(int fd, const std::string &method, const unsigned int req_id, const std::string &msg_tag,
		const std::string &msg_encry, const std::string &msg_uuid, const std::string &session_id,
		const int code, const std::string &msg, const std::string &body, bool is_object)
{
	int nRet = 0;

	std::string buf = XProtocol::rsp_msg(method, req_id, msg_tag, msg_encry, msg_uuid, session_id, code, msg, body, is_object);
	unsigned int len = buf.size();
	{
		Thread_Mutex_Guard guard(_mutex_msg);
		nRet = Socket_Oper::send_n(fd, buf.c_str(), len, 0, 300000);
	}
	
	if(nRet == 0)
	{
		//XCP_LOGGER_INFO(&g_logger, "send rsp msg success. fd:%d, buf(%u):%s\n", fd, buf.size(), buf.c_str());
	}
	else if(nRet == 2)
	{
		XCP_LOGGER_ERROR(&g_logger, "send rsp msg timeout, ret:%d, fd:%d, buf(%u):%s\n", 
			nRet, fd, buf.size(), buf.c_str());
	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "send rsp msg to failed, ret:%d, fd:%d, buf(%u):%s\n", 
			nRet, fd, buf.size(), buf.c_str());	
	}
	
	return nRet;

}



int Msg_Oper::send_msg(int fd, const std::string &buf)
{
	int nRet = 0;

	std::string new_req = buf + std::string("\n");
	unsigned int len = new_req.size();
	{
		Thread_Mutex_Guard guard(_mutex_msg);
		nRet = Socket_Oper::send_n(fd, new_req.c_str(), len, 0, 1000000);
	}

	if(nRet == 0)
	{
		//XCP_LOGGER_INFO(&g_logger, "send msg success. fd:%d, buf(%u):%s\n", fd, buf.size(), buf.c_str());
	}
	else if(nRet == 2)
	{
		XCP_LOGGER_ERROR(&g_logger, "send msg timeout, ret:%d, fd:%d, buf(%u):%s\n", 
			nRet, fd, buf.size(), buf.c_str());
	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "send msg to failed, ret:%d, fd:%d, buf(%u):%s\n", 
			nRet, fd, buf.size(), buf.c_str());		
	}
	
	return nRet;

}


