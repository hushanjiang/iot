
#include "redis_timer_handler.h"
extern redis_mgt g_redis_mgt;
Redis_Timer_handler::Redis_Timer_handler()
{

}

Redis_Timer_handler::~Redis_Timer_handler()
{

}

int Redis_Timer_handler::handle_timeout(void *args)
{
	g_redis_mgt.check();
	return 0;
}

