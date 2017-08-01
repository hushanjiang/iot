/*
**		Copyright (C) Roman Ukhov 2000
**		e-mail: r_ukhov@yahoo.com
**		http://rukhov.newmail.ru
*/


#include "thread.h"

namespace MTNMsgApi_NP
{

	CThread::CThread(bool _auto_delete/* = false*/)
		:
	m_dwThreadId(0),
	m_autoDelete(_auto_delete),
	m_retVal(0),
	m_Param(0),
	m_startAddres(0)
	{
	}
	
	CThread::~CThread()
	{
	}
	
	bool CThread::create(thread_func func, void* _param, bool suspended)
	{

		m_startAddres = func;
		m_Param = _param;
		
	    pthread_attr_t attr;
	    pthread_attr_init(&attr);
	    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    	//int err = pthread_create(&m_dwThreadId,NULL,static_start_func,this);
	    int err = pthread_create(&m_dwThreadId,&attr,static_start_func,this);
	    
    	if(err != 0)
    	{
#ifdef EUCPC_DEBUG    		
			printf("can't create thread: %s\n",strerror(err));
#endif			
			return false;
    	}
		return true;
	}

	void*  CThread::static_start_func(void* pThis)
	{
		
		if(reinterpret_cast<CThread*>(pThis)->m_startAddres==0)
		{
			reinterpret_cast<CThread*>(pThis)->m_retVal = reinterpret_cast<CThread*>(pThis)->thread_function();
		}
		else
		{
			reinterpret_cast<CThread*>(pThis)->m_retVal = reinterpret_cast<CThread*>(pThis)->m_startAddres(reinterpret_cast<CThread*>(pThis)->m_Param);
		}

		if(reinterpret_cast<CThread*>(pThis)->m_autoDelete == true)
		{
			delete reinterpret_cast<CThread*>(pThis);
		}

		return &(reinterpret_cast<CThread*>(pThis)->m_retVal);
	}

	int CThread::thread_function()
	{
		return 0;
	}

	bool CThread::wait_for_end(unsigned long tiomeout)
	{

		if(m_dwThreadId == 0) return true;

    	int nRet = pthread_join(m_dwThreadId,NULL);
		if(nRet == 0) 
			return true;
		else
			return false;       

		return true;
	}

	void CThread::resume() const
	{
	}
	
	void CThread::suspend() const
	{
	}

	bool CThread::set_priority(int priority) const
	{
		return true;
	}
}



