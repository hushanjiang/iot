
#include "worker_timer_handler.h"
#include "worker_mgt.h"

Worker_Timer_handler::Worker_Timer_handler()
{

}

Worker_Timer_handler::~Worker_Timer_handler()
{

}

int Worker_Timer_handler::handle_timeout(void *args)
{
	PSGT_Worker_Mgt->check();	
	return 0;
}


