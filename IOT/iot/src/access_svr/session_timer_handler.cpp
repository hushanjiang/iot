
#include "session_timer_handler.h"
#include "base/base_logger.h"
#include "session_mgt.h"

Session_Timer_handler::Session_Timer_handler()
{

}

Session_Timer_handler::~Session_Timer_handler()
{

}

int Session_Timer_handler::handle_timeout(void *args)
{
	PSGT_Session_Mgt->check();	
	return 0;
}

