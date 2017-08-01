
#include "worker.h"
#include "base/base_logger.h"
#include "base/base_string.h"
#include "base/base_socket_oper.h"
#include "base/base_convert.h"
#include "base/base_uid.h"
#include "conf_mgt.h"
#include "worker_mgt.h"
#include "protocol.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;


Worker::Worker(): _id(""), _new_id(""), _svr_ip(""), _svr_port(0), _fd(-1), _ip(""), _port(0), 
	_create_stmp(0), _hb_stmp(0), _client_num(0)
{

}

Worker::~Worker()
{

}



int Worker::send_rsp(const std::string &method, const std::string &msg_tag, const int code, const std::string &msg)
{
	Thread_Mutex_Guard guard(_mutex);

	int nRet = 0;

	std::string buf = XProtocol::rsp_msg(method, 0, msg_tag, code, msg);
	unsigned int len = buf.size();
	nRet = Socket_Oper::send_n(_fd, buf.c_str(), len, 0, 300000);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send rsp to worker(%s) success, rsp(%u):%s\n", 
			to_string().c_str(), buf.size(), buf.c_str());

	}
	else if(nRet == 2)
	{
		XCP_LOGGER_ERROR(&g_logger, "send rsp to worker(%s) timeout, ret:%d, rsp(%u):%s\n", 
			to_string().c_str(), nRet, buf.size(), buf.c_str());

	}
	else
	{
		XCP_LOGGER_ERROR(&g_logger, "send rsp to worker(%s) failed, ret:%d, rsp(%u):%s\n", 
			to_string().c_str(), _fd, nRet, buf.size(), buf.c_str());
	}

	if(nRet != 0)
	{
		PSGT_Worker_Mgt->unregister_worker(this);
		::close(_fd);
	}
	
	return nRet;

}



void Worker::log()
{
	XCP_LOGGER_INFO(&g_logger, "[worker]: id=%s, new_id:%s, svr(%s:%u), fd=%d, "
		"peer(%s:%u), create_stmp=%llu, hb_stmp=%llu, client_num=%u\n",
		_id.c_str(), _new_id.c_str(), _svr_ip.c_str(), _svr_port, _fd, 
		_ip.c_str(), _port, _create_stmp, _hb_stmp, _client_num);
}


std::string Worker::to_string()
{
	return format("[worker]: id=%s, new_id:%s, svr(%s:%u), fd=%d, "
		"peer(%s:%u), create_stmp=%llu, hb_stmp=%llu, client_num:%u\n",
		_id.c_str(), _new_id.c_str(), _svr_ip.c_str(), _svr_port, _fd, 
		_ip.c_str(), _port, _create_stmp, _hb_stmp, _client_num);
}


