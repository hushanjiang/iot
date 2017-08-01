
#include "admin.h"
#include "comm/common.h"
#include "conf_mgt.h"
#include "base/base_signal.h"
#include "base/base_logger.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;

Admin::Admin()
{

}


Admin::~Admin()
{

}


int Admin::do_init(void *args)
{
	int nRet = 0;

	return nRet;
}


int Admin::svc()
{
	int nRet = 0;

	sigset_t sigset;
	add_signal_in_set(sigset, 1, SIGUSR2);

	int signo = 0;	
	while(true) 
	{
		nRet = sigwait(&sigset, &signo);
		if (EINTR == nRet) 
		{
		    continue;
		}
		else if (nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "sigwait failed. errno:%d, errmsg:%s\n", errno, strerror(errno));
		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "--- prepare to Admin refresh ----\n");
			
			nRet = PSGT_Conf_Mgt->refresh();
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "refresh conf failed, ret:%d\n", nRet);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "refresh conf success\n");
			}

			XCP_LOGGER_INFO(&g_logger, "--- complete to Admin refresh ----\n");
		}

		break;
	
	}
	
	return 0;

	
}


