
#include "comm/common.h"
#include "base/base_logger.h"
#include "base/base_string.h"
#include "route_release.h"
#include "redis_mgt.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;

Route_Release::Route_Release()
{

}



Route_Release::~Route_Release()
{

}




int Route_Release::do_init(void *args)
{
	int nRet = 0;

	return nRet;
}




int Route_Release::svc()
{
	int nRet = 0;
	std::string err_info = "";

	XCP_LOGGER_INFO(&g_logger, "--- prepare to run Route_Release ---\n");
	
	std::set<std::string> svrs;
	nRet = PSGT_Redis_Mgt->get_access_svr_list(svrs, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "get_access_svr_list faield, ret:%d, err_info:%s\n", nRet, err_info.c_str());
		return -1;
	}

	if(svrs.empty())
	{
		XCP_LOGGER_INFO(&g_logger, "svrs.empty.\n");
		return -1;		
	}

	std::set<std::string>::iterator itr = svrs.find(g_sysInfo._log_id);
	if((itr != svrs.end()) && (svrs.size() == 1))
	{
		XCP_LOGGER_INFO(&g_logger, "only the route of access svr(%s) is exist.\n", g_sysInfo._new_id.c_str());
		return -1;	
	}
	else if((itr != svrs.end()) && (svrs.size() > 1))
	{
		//剔除本服务
		svrs.erase(itr);
	}
	else
	{
		//no-todo
	}
	
	std::set<std::string>::iterator itr1 = svrs.begin();
	for(; itr1 != svrs.end(); itr1++)
	{
		std::string svr = *itr1;
		XCP_LOGGER_INFO(&g_logger, "--- invalid access svr:%s\n", svr.c_str());

		bool bquery = false;
		unsigned long long begin = 0;
		std::deque<unsigned long long> clients;
		while((clients.size() == MAX_SEARCH_CLIENT_CNT) || (!bquery && clients.empty()))
		{
			bquery = true;

			std::string::size_type pos = svr.find(":");
			std::string svr_name = svr.substr(pos+1, svr.size()-pos-1);

			clients.clear();
			nRet = PSGT_Redis_Mgt->get_client_list(svr_name, begin, MAX_SEARCH_CLIENT_CNT, clients, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "get_client_list faield, access svr:%s, ret:%d, err_info:%s\n", 
					svr.c_str(), nRet, err_info.c_str());
				break;
			}

			if(clients.empty())
			{
				XCP_LOGGER_INFO(&g_logger, "clients.empty(), access svr:%s\n", svr.c_str());
				break;
			}

			XCP_LOGGER_INFO(&g_logger, "size of client:%d\n", clients.size());
			
			std::deque<unsigned long long>::iterator itr2 = clients.begin();
			for(; itr2 != clients.end(); itr2++)
			{
				PSGT_Redis_Mgt->unregister_route(*itr2, svr_name, err_info);
			}

			begin = *(clients.end());
			
		}

	}
	

	XCP_LOGGER_INFO(&g_logger, "=== complete to run Route_Release ===\n");

	return -1;
	
}


