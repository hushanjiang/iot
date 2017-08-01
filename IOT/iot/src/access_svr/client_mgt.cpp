
#include "client_mgt.h"
#include "comm/common.h"
#include "conf_mgt.h"
#include "base/base_logger.h"
#include "base/base_uid.h"
#include "base/base_string.h"
#include "base/base_utility.h"
#include "base/base_convert.h"
#include "base/base_reactor.h"
#include "redis_mgt.h"

extern Logger g_logger;


Client_Mgt::Client_Mgt()
{

}


Client_Mgt::~Client_Mgt()
{


}


void Client_Mgt::register_client(const StClient stClient)
{
	Thread_Mutex_Guard guard(_mutex);
	
	std::map<unsigned long long, std::set<int> >::iterator itr = _ids.find(stClient._id);
	if(itr == _ids.end())
	{
		std::set<int> fds;
		fds.insert(stClient._fd);
		_ids.insert(std::make_pair(stClient._id, fds));
	}
	else
	{
		itr->second.insert(stClient._fd);
	}

	_fds[stClient._id] = stClient;

}



void Client_Mgt::unregister_client(int fd)
{
	Thread_Mutex_Guard guard(_mutex);

	std::map<int, StClient>::iterator itr_fd = _fds.find(fd);
	if(itr_fd != _fds.end())
	{
		std::map<unsigned long long, std::set<int> >::iterator itr_id = _ids.find(itr_fd->second._id);
		if(itr_id != _ids.end())
		{
			std::set<int>::iterator itr = itr_id->second.find(fd);
			if(itr != itr_id->second.end())
			{
				itr_id->second.erase(itr);
				if(itr_id->second.empty())
				{
					_ids.erase(itr_id);
				}
			}
		}

		_fds.erase(itr_fd);
	}

}




unsigned int Client_Mgt::client_num()
{
	return _fds.size();
}


