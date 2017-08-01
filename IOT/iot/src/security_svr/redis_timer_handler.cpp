
#include "redis_timer_handler.h"

Redis_Timer_handler::Redis_Timer_handler()
{

}

Redis_Timer_handler::~Redis_Timer_handler()
{

}

int Redis_Timer_handler::handle_timeout(void *args)
{
	PSGT_Redis_Mgt->check();
	
	return 0;
}

