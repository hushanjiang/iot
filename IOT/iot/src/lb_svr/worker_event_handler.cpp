
#include "worker_event_handler.h"
#include "base/base_socket_oper.h"
#include "base/base_net.h"
#include "req_mgt.h"
#include "session_mgt.h"
#include "worker_mgt.h"
#include "comm/common.h"
#include "protocol.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "worker_mgt.h"
#include "msg_oper.h"

extern Logger g_logger;

Worker_Event_Handler::Worker_Event_Handler(): _buf("")
{

};


Worker_Event_Handler::~Worker_Event_Handler()
{

};

	
//处理建立连接请求事件
int Worker_Event_Handler::handle_accept(int fd)
{
	int nRet = 0;

	std::string local_ip = "";
	unsigned short local_port = 0;
	get_local_socket(fd, local_ip, local_port);
	
	std::string remote_ip = "";
	unsigned short remote_port = 0; 
	get_remote_socket(fd, remote_ip, remote_port);
	
	XCP_LOGGER_INFO(&g_logger, "accept success(fd:%d) from worker, %s:%u --> %s:%u\n", 
		fd, remote_ip.c_str(), remote_port, local_ip.c_str(), local_port);
	
	return nRet;
};



//处理读事件
int Worker_Event_Handler::handle_input(int fd)
{
	int nRet = 0;
		
	char buf[4096] = {0};
	unsigned int buf_len = 4095;
	nRet = Socket_Oper::recv(fd, buf, buf_len, 300000);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "rcv close from worker(fd:%d), ret:%d\n", fd, nRet);
		return -1;
	}
	else if(nRet == 1)
	{
		//XCP_LOGGER_INFO(&g_logger, "rcv success from worker(fd:%d), req(%u):%s\n", fd, buf_len, buf);
	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "rcv failed from worker(fd:%d), ret:%d\n", fd, nRet);
		return -2;
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

		trim(req_src);
		
		//获取method
		std::string method = "";
		unsigned long long timestamp = 0;
		std::string msg_tag = "";
		std::string err_info = "";	
		nRet = XProtocol::admin_head(req_src, method, timestamp, msg_tag, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "invalid msg from worker, ret:%d, err_info:%s, msg(%u)%s\n", 
				nRet, err_info.c_str(), req_src.size(), req_src.c_str());
		}
		else if(method == CMD_REGISTER)
		{
			XCP_LOGGER_INFO(&g_logger, "rcv register req from worker, req(%u)%s\n", req_src.size(), req_src.c_str());

			//注册worker
			Worker_Ptr worker = new Worker;
			worker->_fd = fd;
			worker->_create_stmp = getTimestamp();
			worker->_hb_stmp = getTimestamp();
			get_remote_socket(fd, worker->_ip, worker->_port);

			std::string id = "";
			std::string svr_ip = "";
			unsigned short svr_port = 0;
			nRet = XProtocol::worker_info(req_src, id, svr_ip, svr_port, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_ERROR(&g_logger, "get worker info failed, ret:%d, err_info:%s, req(%u):%s\n", 
					nRet, err_info.c_str(), req_src.size(), req_src.c_str());
				nRet = ERR_INVALID_REQ;
			}
			else
			{
				//new id 规则: [svr id]_[ip]_[port]
				std::string new_id = format("%s_%s_%u", id.c_str(), svr_ip.c_str(), svr_port);
				worker->_id = id;
				worker->_new_id = new_id;
				worker->_svr_ip = svr_ip;
				worker->_svr_port = svr_port;
				nRet = PSGT_Worker_Mgt->register_worker(worker);
				if(nRet == 0)
				{
					XCP_LOGGER_ERROR(&g_logger, "register worker success, worker:%s\n", worker->to_string().c_str());					
				}
				else
				{
					XCP_LOGGER_ERROR(&g_logger, "register worker failed, ret:%d, worker:%s\n", nRet, worker->to_string().c_str());	
				}
				
			}

			//向worker svr 返回注册结果
			worker->send_rsp(method, msg_tag, nRet);
			
			
		}
		else if(method == CMD_HB)
		{
			//心跳附带上Access Svr 当前客户端长连接数
			unsigned int client_num = 0;
			nRet = XProtocol::hb_req(req_src, client_num, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_ERROR(&g_logger, "hb_req failed, ret:%d, err_info:%s, req(%u):%s\n", 
					nRet, err_info.c_str(), req_src.size(), req_src.c_str());
			}
			else
			{
				//心跳
				Worker_Ptr worker;
				if(!PSGT_Worker_Mgt->get_worker(fd, worker))
				{
					XCP_LOGGER_ERROR(&g_logger, "hb-get worker failed, fd:%d\n", fd);
				}
				else
				{			
					//刷新心跳时间戳
					worker->_hb_stmp = getTimestamp();
					worker->_client_num = client_num;
				}
			}
			
		}		
		else
		{
			XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from worker\n", method.c_str());
		}
		
		pos = _buf.find("\n");
			
	}

	return 0;

};





//处理连接关闭事件
int Worker_Event_Handler::handle_close(int fd)
{
	int nRet = 0;

	Worker *worker = new Worker;
	worker->_fd = fd;
	PSGT_Worker_Mgt->unregister_worker(worker);

	std::string local_ip = "";
	unsigned short local_port = 0;
	get_local_socket(fd, local_ip, local_port);
	
	std::string remote_ip = "";
	unsigned short remote_port = 0; 
	get_remote_socket(fd, remote_ip, remote_port);
	
	XCP_LOGGER_INFO(&g_logger, "close (fd:%d) from worker, %s:%u --> %s:%u\n", 
		fd, remote_ip.c_str(), remote_port, local_ip.c_str(), local_port);
	
	return nRet;
	
};



Event_Handler* Worker_Event_Handler::renew()
{
	return new Worker_Event_Handler;
};


