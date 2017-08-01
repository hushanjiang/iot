
#include "conn_timer_handler.h"

Conn_Timer_handler::Conn_Timer_handler()
{

}

Conn_Timer_handler::~Conn_Timer_handler()
{

}

int Conn_Timer_handler::handle_timeout(void *args)
{
	_conn_mgt->check();
	
	return 0;
}

