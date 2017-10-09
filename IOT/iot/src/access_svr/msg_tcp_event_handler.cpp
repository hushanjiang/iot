
#include "base/base_socket_oper.h"
#include "base/base_net.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "msg_tcp_event_handler.h"
#include "msg_oper.h"
#include "protocol.h"
#include "req_mgt.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Req_Mgt *g_msg_mgt;

Msg_TCP_Event_Handler::Msg_TCP_Event_Handler(Conn_Mgt_LB *conn_mgt): _buf(""), _conn_mgt(conn_mgt)
{

};


Msg_TCP_Event_Handler::~Msg_TCP_Event_Handler()
{

};


//处理读事件
int Msg_TCP_Event_Handler::handle_input(int fd)
{
	int nRet = 0;

	//获取mdp ip 和port
	std::string ip = "";
	unsigned short port = 0;
	get_remote_socket(fd, ip, port);
	
	char buf[4096];
	unsigned int buf_len = 4095;
	nRet = Socket_Oper::recv(fd, buf, buf_len, 300000);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "rcv close from mdp(fd:%d), ret:%d\n", fd, nRet);
		return -1;
	}
	else if(nRet == 1)
	{
		//XCP_LOGGER_INFO(&g_logger_debug, "rcv success from mdp(fd:%d), req(%u):%s\n", fd, buf_len, buf);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "rcv failed from mdp(fd:%d), ret:%d\n", fd, nRet);
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
		
		//XCP_LOGGER_INFO(&g_logger, "rcv request form mdp, req(%u):%s\n",  req_src.size(), req_src.c_str());
		
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
			std::string err_info = "";
			nRet = XProtocol::admin_head(req_src, method, timestamp, msg_tag, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "admin_head failed, ret:%d, err_info:%s, req(%u):%s\n", 
					nRet, err_info.c_str(), req_src.size(), req_src.c_str());
			}
			else
			{
				if(method == CMD_MDP_REGISTER)
				{
					int code = XProtocol::get_rsp_result(req_src, err_info);
	
					//注册响应消息
					std::string ip = "";
					unsigned short port = 0;
					get_remote_socket(fd, ip, port);
					std::string mdp_id = format("%s_%u", ip.c_str(), port);
	
					Conn_Ptr conn;
					if(!(_conn_mgt->get_conn(mdp_id, conn)))
					{
						XCP_LOGGER_ERROR(&g_logger, "register mdp: no conn is found, fd:%d\n", fd);
					}
					else
					{
						if(code == 0)
						{
							conn->_registered = true;
							XCP_LOGGER_INFO(&g_logger, "register to mdp success.\n");						
						}
						else
						{
							conn->_registered = false;
							XCP_LOGGER_INFO(&g_logger, "register to mdp failed.\n");
						}	
					}	
				}	
				else
				{
					//处理正常请求
					Request *req = new Request;
					req->_rcv_stmp = getTimestamp();
					req->_req = req_src;
					req->_ip = ip;
					req->_port = port;
					req->_fd = fd;
					req->_msg_tag = msg_tag;
					
					//首先先判断msg queue 是否已经满了
					if(!(g_msg_mgt->full()))
					{	
						nRet = g_msg_mgt->push_req(req);
						if(nRet != 0)
						{
							XCP_LOGGER_ERROR(&g_logger, "push into msg mgt failed, ret:%d, req(%u):%s\n", nRet, buf_len, buf);
						}
						
					}
					else
					{		
						XCP_LOGGER_ERROR(&g_logger, "msg mgt is full, req(%u):%s\n", buf_len, buf); 		
					}

				}

			}
			
		}
		
		pos = _buf.find("\n");
		
	}

	return 0;

};




//处理连接关闭事件
int Msg_TCP_Event_Handler::handle_close(int fd)
{
	int nRet = 0;
	
	std::string local_ip = "";
	unsigned short local_port = 0;
	get_local_socket(fd, local_ip, local_port);
	
	std::string remote_ip = "";
	unsigned short remote_port = 0; 
	get_remote_socket(fd, remote_ip, remote_port);
	
	XCP_LOGGER_INFO(&g_logger, "close (fd:%d) from mdp, %s:%u --> %s:%u\n", 
		fd, remote_ip.c_str(), remote_port, local_ip.c_str(), local_port);
	
	//刷新注册标志位
	std::string mdp_id = format("%s_%u", remote_ip.c_str(), remote_port);
	Conn_Ptr conn;
	if(_conn_mgt->get_conn(mdp_id, conn))
	{
		conn->_registered = false;
	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "handle close: no mdp conn is found.\n");
	}
			
	return nRet;
	
};



Event_Handler* Msg_TCP_Event_Handler::renew()
{
	return new Msg_TCP_Event_Handler(_conn_mgt);
};


