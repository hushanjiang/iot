
#include "comm/common.h"
#include "req_mgt.h"
#include "base/base_logger.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;

Req_Mgt::Req_Mgt()
{
	_queue = new X_Queue<Request_Ptr>(g_sysInfo._max_queue_size);
}


Req_Mgt::~Req_Mgt()
{


}

int Req_Mgt::push_req(Request_Ptr req)
{
	return _queue->push(req);
}


int Req_Mgt::get_req(Request_Ptr &req)
{
	return _queue->pop(req);	
}


bool Req_Mgt::full()
{
	return _queue->full();
}


//------------------------------

Rsp_Mgt::Rsp_Mgt()
{
	_queue = new M_Queue<Request_Ptr>();
}


Rsp_Mgt::~Rsp_Mgt()
{


}

int Rsp_Mgt::push_req(const std::string &key, Request_Ptr req)
{
	return _queue->push(key, req);
}

int Rsp_Mgt::get_req(const std::string &key, Request_Ptr &req, const long long timeout)
{
	return _queue->pop(key, req, timeout);
}


