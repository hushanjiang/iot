

#include "session.h"
#include "base/base_string.h"
#include "base/base_logger.h"

extern Logger g_logger;

Session::Session(): _id(""), _fd(-1), _ip(""), _port(0), _status(false), \
_create_time(0), _access_time(0), _close_time(0), _r_num(0), _w_num(0)
{

}

Session::~Session()
{

}

void Session::log()
{
	XCP_LOGGER_INFO(&g_logger, "[session]: id:%s, fd:%d, ip:%s, port:%u, status:%s, "
		"create time:%llu, access time:%llu, close time:%llu, spend time:%llu, read num:%llu, write num:%llu\n", 
		_id.c_str(), _fd, _ip.c_str(), _port, (_status?"connect":"close"), 
		_create_time, _access_time, _close_time, (_close_time>_create_time?(_close_time-_create_time):0), 
		_r_num, _w_num);
}


std::string Session::to_string()
{
	return format("[session]: id:%s, fd:%d, ip:%s, port:%u, status:%s, "
		"create time:%llu, access time:%llu, close time:%llu, spend time:%llu, read num:%llu, write num:%llu", 
		_id.c_str(), _fd, _ip.c_str(), _port, (_status?"connect":"close"), 
		_create_time, _access_time, _close_time, (_close_time>_create_time?(_close_time-_create_time):0), 
		_r_num, _w_num);
}

