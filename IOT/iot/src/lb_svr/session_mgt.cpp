
#include "session_mgt.h"
#include "base/base_args_parser.h"
#include "base/base_logger.h"
#include "base/base_net.h"
#include "base/base_uid.h"
#include "base/base_time.h"
#include "base/base_string.h"

extern Logger g_logger;

Session_Mgt::Session_Mgt()
{

}



Session_Mgt::~Session_Mgt()
{

}



void Session_Mgt::insert_session(int fd)
{
	Thread_Mutex_Guard guard(_mutex);

	std::string ip = "";
	unsigned short port = 0; 
	get_remote_socket(fd, ip, port);

	//获取session id
	std::string tmp = format("%s_%u", ip.c_str(), port);
	std::string id = UID::uid_inc(tmp);

	Session session;
	session._id = id;
	session._fd = fd;
	session._status = true;
	session._ip = ip;
	session._port = port;
	unsigned long long ts = get_microsecond();
	session._create_time = ts;
	session._access_time = ts;
	_sessions[fd] = session;
	_fds[id] = fd;

}




void Session_Mgt::remove_session(int fd)
{
	Thread_Mutex_Guard guard(_mutex);

	std::map<int, Session>::iterator itr = _sessions.find(fd);
	if(itr != _sessions.end())
	{
		XCP_LOGGER_INFO(&g_logger, "prepare remove session(fd:%d)\n", fd);
			
		itr->second._close_time = get_microsecond();
		itr->second.log();
		
		_fds.erase(itr->second._id);
		_sessions.erase(itr);
		
	}
}


bool Session_Mgt::is_valid_sesssion(const std::string &id)
{
	Thread_Mutex_Guard guard(_mutex);

	std::map<std::string, int>::iterator itr = _fds.find(id);
	if(itr != _fds.end())
	{
		return true;
	}

	return false;
}



void Session_Mgt::update_status(int fd, bool status)
{
	Thread_Mutex_Guard guard(_mutex);

	std::map<int, Session>::iterator itr = _sessions.find(fd);
	if(itr != _sessions.end())
	{
		itr->second._status = status;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "fd(%d) isn't found in _sessions.\n", fd);
	}
}



void Session_Mgt::update_read(int fd, unsigned long long num, std::string &id)
{
	Thread_Mutex_Guard guard(_mutex);

	std::map<int, Session>::iterator itr = _sessions.find(fd);
	if(itr != _sessions.end())
	{
		if(itr->second._status)
		{
			itr->second._access_time = get_microsecond();
			itr->second._r_num += num;
			id = itr->second._id;
		}
	}
}



void Session_Mgt::update_write(int fd, unsigned long long num)
{
	Thread_Mutex_Guard guard(_mutex);

	std::map<int, Session>::iterator itr = _sessions.find(fd);
	if(itr != _sessions.end())
	{
		if(itr->second._status)
		{
			itr->second._access_time = get_microsecond();
			itr->second._w_num += num;
		}
	}

}


void Session_Mgt::check()
{
	Thread_Mutex_Guard guard(_mutex);
		
	//计算当前时间戳
	unsigned long long cur_stmp = get_microsecond();
	std::map<int, Session>::iterator itr = _sessions.begin();
	for(; itr != _sessions.end();)
	{
		//itr->second.log();

		//会话超过600秒后Server自动关闭连接
		if(cur_stmp - (itr->second._access_time) > 600000000UL)
		{
			int fd = itr->first;
			
			itr->second._close_time = get_microsecond();
			XCP_LOGGER_INFO(&g_logger, "===timeout: the session(%s) is timeout, prepare to close it, cur_stmp:%llu\n", 
				itr->second.to_string().c_str(), cur_stmp);
			
			std::string local_ip = "";
			unsigned short local_port = 0;
			get_local_socket(fd, local_ip, local_port);
			
			std::string remote_ip = "";
			unsigned short remote_port = 0; 
			get_remote_socket(fd, remote_ip, remote_port);
			
			::close(fd);
			
			XCP_LOGGER_INFO(&g_logger, "close to app(fd:%d), %s:%u --> %s:%u\n", 
				fd, local_ip.c_str(), local_port, remote_ip.c_str(), remote_port);
			
			_fds.erase(itr->second._id);
			_sessions.erase(itr++);
			
		}
		else
		{
			++itr;
		}
		
	}

}


