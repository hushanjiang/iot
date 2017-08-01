
#include "conf_timer_handler.h"
#include "base/base_logger.h"
#include "conf_mgt.h"

Conf_Timer_handler::Conf_Timer_handler()
{

}

Conf_Timer_handler::~Conf_Timer_handler()
{

}

int Conf_Timer_handler::handle_timeout(void *args)
{
	PSGT_Conf_Mgt->update_svr();
	
	return 0;
}

