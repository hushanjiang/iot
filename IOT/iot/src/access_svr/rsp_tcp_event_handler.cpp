
#include "base/base_socket_oper.h"
#include "base/base_net.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "rsp_tcp_event_handler.h"
#include "msg_oper.h"
#include "protocol.h"
#include "req_mgt.h"
#include "rsp_mgt.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Req_Mgt *g_asyn_mgt;
extern Rsp_Mgt *g_syn_mgt;

RSP_TCP_Event_Handler::RSP_TCP_Event_Handler(const std::string &svr_name): _buf(""), _svr_name(svr_name)
{

};


RSP_TCP_Event_Handler::~RSP_TCP_Event_Handler()
{

};


//处理读事件
int RSP_TCP_Event_Handler::handle_input(int fd)
{
	int nRet = 0;
		
	char buf[4096] = {0};
	unsigned int buf_len = 4095;
	nRet = Socket_Oper::recv(fd, buf, buf_len, 300000);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "rcv close from %s svr(fd:%d), ret:%d\n", _svr_name.c_str(), fd, nRet);
		return -1;
	}
	else if(nRet == 1)
	{
		//XCP_LOGGER_INFO(&g_logger, "rcv success from %s svr(fd:%d), req(%u):%s\n", _svr_name.c_str(), fd, buf_len, buf);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "rcv failed from %s svr(fd:%d), ret:%d\n", _svr_name.c_str(), fd, nRet);
		return 0;
	}
	buf[buf_len] = '\0';

	//追加缓存
	_buf += buf;
	std::string::size_type pos = _buf.find("\n");
	while(pos != std::string::npos)
	{
		//解析完整请求串
		std::string req_src = _buf.substr(0, pos);
		_buf.erase(0, pos+1);
		
		XCP_LOGGER_INFO(&g_logger, "rcv rsp form %s svr(fd:%d), req(%u):%s\n", _svr_name.c_str(), fd, req_src.size(), req_src.c_str());
		
		trim(req_src);
		
		if(req_src.size() < MIN_MSG_LEN)
		{
			//XCP_LOGGER_ERROR(&g_logger_debug, "the req reach min len, req(%u):%s\n", req_src.size(), req_src.c_str());
		}
		else
		{			
			std::string method = "";
			unsigned long long timestamp = 0;
			std::string msg_tag = "";
			int code = 0;
			std::string msg = "";	
			std::string err_info = "";
			nRet = XProtocol::admin_head(req_src, method, timestamp, msg_tag, code, msg, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "admin_head failed, ret:%d, err_info:%s, req(%u):%s\n", 
					nRet, err_info.c_str(), req_src.size(), req_src.c_str());
			}
			else
			{
				//获取对端ip 和port
				std::string ip = "";
				unsigned short port = 0;
				get_remote_socket(fd, ip, port);
	
				Request *req = new Request;
				req->_rcv_stmp = getTimestamp();
				req->_req = req_src;
				req->_msg_tag = msg_tag;
				req->_fd = fd;
				req->_ip = ip;
				req->_port = port;

				if((method == CMD_CHECK_TALK_CONDITION) || (method == CMD_GET_MEMBER_ID_LIST)) 
				{
					//业务服务同步响应
					g_syn_mgt->push_req(msg_tag, req);
				}
				else
				{
					//业务服务异步响应
					g_asyn_mgt->push_req(req);
				}

			}
			
		}
		
		pos = _buf.find("\n");
		
	}

	return 0;

};




//处理连接关闭事件
int RSP_TCP_Event_Handler::handle_close(int fd)
{
	int nRet = 0;
	
	std::string local_ip = "";
	unsigned short local_port = 0;
	get_local_socket(fd, local_ip, local_port);
	
	std::string remote_ip = "";
	unsigned short remote_port = 0; 
	get_remote_socket(fd, remote_ip, remote_port);
	
	XCP_LOGGER_INFO(&g_logger, "close (fd:%d) from %s svr, %s:%u --> %s:%u\n", 
		fd, _svr_name.c_str(), remote_ip.c_str(), remote_port, local_ip.c_str(), local_port);
			
	return nRet;
};



Event_Handler* RSP_TCP_Event_Handler::renew()
{
	return new RSP_TCP_Event_Handler(_svr_name);
};


