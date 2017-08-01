
#include "request.h"
#include "base/base_string.h"
#include "base/base_args_parser.h"
#include "base/base_logger.h"

extern Logger g_logger;

Request::Request(): _msg_tag(""), _req_id(0), _req(""), _fd(-1), _ip(""), _port(0), 
	_stmp(0),_real_id(0), _app_stmp(0), _session_id("")
{

}


Request::~Request()
{

}


void Request::log()
{
	XCP_LOGGER_INFO(&g_logger, "req: req_id=%u, msg_tag=%s, fd=%d, app=(%s:%u), real_id:%llu, stmp:%llu, app_stmp:%llu, session id:%s, req(%u):%s\n", 
		_req_id, _msg_tag.c_str(), _fd, _ip.c_str(), _port, _real_id, _stmp, _app_stmp, _session_id.c_str(), _req.size(), _req.c_str());
}


std::string Request::to_string()
{
	return format("req: req_id=%u, msg_tag=%s, fd=%d, app=(%s:%u), real_id:%llu, stmp:%llu, app_stmp:%llu, session id:%s, req(%u):%s\n", 
		_req_id, _msg_tag.c_str(), _fd, _ip.c_str(), _port, _real_id, _stmp, _app_stmp, _session_id.c_str(), _req.size(), _req.c_str());
}


