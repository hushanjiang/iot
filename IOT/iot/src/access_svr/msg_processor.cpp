
#include "msg_processor.h"
#include "req_mgt.h"
#include "comm/common.h"
#include "protocol.h"
#include "base/base_net.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_reactor.h"

#include "msg_oper.h"
#include "client_mgt.h"
#include "session_mgt.h"

extern Logger g_logger;
extern Req_Mgt *g_msg_mgt;
extern Req_Mgt *g_broadcast_mgt;
extern Reactor *g_reactor_req;

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
	
	XCP_LOGGER_INFO(&g_logger, "prepare to process msg from mdp:%s\n", req->to_string().c_str());
	req->_process_stmp = getTimestamp();
	
	std::string method = "";
	std::string sys_method = "";
	std::string msg_type = "";
	unsigned long long target_id = 0;
	std::string encry = "";
	nRet = XProtocol::from_mdp_req(req->_req, method, sys_method, msg_type, target_id, encry, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "mdp_msg failed, ret:%d, err_info:%s\n", nRet, err_info.c_str());
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "method:%s, sys_method:%s, msg_type:%s, target_id:%llu, encry:%s\n", 
			method.c_str(), sys_method.c_str(), msg_type.c_str(), target_id, encry.c_str());

	if(method == CMD_MDP_MSG)
	{
		XCP_LOGGER_INFO(&g_logger, "-------- %s --------\n", method.c_str());

		req->_encry = encry;
		if((msg_type == MT_BROADCAST) || (msg_type == MT_BROADCAST_APP) || (msg_type == MT_BROADCAST_ROUTER))
		{
			//广播消息
			if(!(g_broadcast_mgt->full()))
			{	
				req->_msg_type = msg_type;
				nRet = g_broadcast_mgt->push_req(req);
				if(nRet != 0)
				{
					XCP_LOGGER_ERROR(&g_logger, "push into broadcast mgt failed, ret:%d, msg:%s\n", 
						nRet, req->to_string().c_str());
				}
				
			}
			else
			{		
				XCP_LOGGER_ERROR(&g_logger, "req mgt is full, msg:%s\n", req->to_string().c_str());			
			}
			
		}
		else
		{
			//点对点消息

			//获取目标id 对应是client 列表
			std::map<std::string, StClient> clients;
			if(!PSGT_Client_Mgt->get_clients(target_id, clients, err_info))
			{
				XCP_LOGGER_ERROR(&g_logger, "the client(%llu) isn't in the access svr.\n", target_id);	
				return 0;
			}
					
			if(sys_method.empty())
			{
				XCP_LOGGER_INFO(&g_logger, "p2p msg(%llu)\n", target_id); 
				
				//发送给本地客户端
				std::map<std::string, StClient>::iterator itr = clients.begin();
				for(; itr != clients.end(); itr++)
				{
					Msg_Oper::send_msg(itr->second._fd, itr->second._session_id, itr->second._uuid, encry, itr->second._key, req->_req);
				}

			}
			else if(sys_method == CMD_SYS_KICKOFF_CLIENT)
			{
				//踢人
				XCP_LOGGER_INFO(&g_logger, "prepare to kickoff client(%llu)\n", target_id); 
				
				std::map<std::string, StClient>::iterator itr = clients.begin();
				for(; itr != clients.end(); itr++)
				{
					int fd = itr->second._fd;
					std::string session_id = "";
					PSGT_Session_Mgt->remove_session(fd, session_id);
					PSGT_Client_Mgt->unregister_client(session_id, US_KICKOFF);
					
					XCP_LOGGER_INFO(&g_logger, "---prepare to del_fd from reactor, fd:%d, session id:%s\n", 
						fd, session_id.c_str()); 
					g_reactor_req->del_fd(itr->second._fd);
				}
			}
			else
			{
				//no-todo
			}
			

		}
		

	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from mdp\n", method.c_str());
	}
	
	return 0;
	
}


