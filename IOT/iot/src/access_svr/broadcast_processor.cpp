
#include "comm/common.h"
#include "base/base_signal.h"
#include "base/base_logger.h"
#include "base/base_string.h"
#include "base/base_time.h"
#include "broadcast_processor.h"
#include "client_mgt.h"
#include "req_mgt.h"
#include "msg_oper.h"

extern Logger g_logger;
extern Req_Mgt *g_broadcast_mgt;

Broadcast_Processor::Broadcast_Processor()
{

}



Broadcast_Processor::~Broadcast_Processor()
{

}



int Broadcast_Processor::do_init(void *args)
{
	int nRet = 0;

	return nRet;
}



/*
经过测试在通知所有人消息很多的情况下
Notify_Everyone_Processor 执行消耗CPU 比较高
为了降低CPU占用率，每发送一个在线客户3毫秒
6W在线需要180秒
*/
int Broadcast_Processor::svc()
{
	int nRet = 0;
	std::string err_info = "";
	
	//获取req 并且处理
	Request_Ptr req;
	nRet = g_broadcast_mgt->get_req(req);
	if(nRet != 0)
	{
		return 0;
	}
	
	XCP_LOGGER_INFO(&g_logger, "prepare to process broagcast:%s\n", req->to_string().c_str());
	
	std::string msg_type = req->_msg_type;
	if(msg_type == MT_BROADCAST)
	{
		std::map<unsigned long long, std::map<std::string, StClient> > apps;
		PSGT_Client_Mgt->get_all_app(apps);

		std::map<unsigned long long, std::map<std::string, StClient> > routers;
		PSGT_Client_Mgt->get_all_router(routers);
		
		std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr = apps.begin();
		for(; itr != apps.end(); itr++)
		{
			std::map<std::string, StClient>::iterator itr1 = itr->second.begin();
			for(; itr1 != itr->second.end(); itr1++)
			{
				Msg_Oper::send_msg(itr1->second._fd, itr1->second._session_id, itr1->second._uuid, req->_encry, itr1->second._key, req->_req);
				::usleep(3000);
			}
		}
		
		std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr2 = routers.begin();
		for(; itr2 != routers.end(); itr2++)
		{
			std::map<std::string, StClient>::iterator itr3 = itr2->second.begin();
			for(; itr3 != itr2->second.end(); itr3++)
			{
				Msg_Oper::send_msg(itr3->second._fd, itr3->second._session_id, itr3->second._uuid, req->_encry, itr3->second._key, req->_req);
				::usleep(3000);
			}
		}

	}
	else if(msg_type == MT_BROADCAST_APP)
	{
		std::map<unsigned long long, std::map<std::string, StClient> > apps;
		PSGT_Client_Mgt->get_all_app(apps);
		
		std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr = apps.begin();
		for(; itr != apps.end(); itr++)
		{
			std::map<std::string, StClient>::iterator itr1 = itr->second.begin();
			for(; itr1 != itr->second.end(); itr1++)
			{
				Msg_Oper::send_msg(itr1->second._fd, itr1->second._session_id, itr1->second._uuid, req->_encry, itr1->second._key, req->_req);
				::usleep(3000);
			}
		}

	}
	else if(msg_type == MT_BROADCAST_ROUTER)
	{
		std::map<unsigned long long, std::map<std::string, StClient> > routers;
		PSGT_Client_Mgt->get_all_router(routers);
		
		std::map<unsigned long long, std::map<std::string, StClient> >::iterator itr = routers.begin();
		for(; itr != routers.end(); itr++)
		{
			std::map<std::string, StClient>::iterator itr1 = itr->second.begin();
			for(; itr1 != itr->second.end(); itr1++)
			{
				Msg_Oper::send_msg(itr1->second._fd, itr1->second._session_id, itr1->second._uuid, req->_encry, itr1->second._key, req->_req);
				::usleep(3000);
			}
		}

	}
	else
	{
		//no-todo
	}

	return 0;
	
}


