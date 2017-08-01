/*
**		Copyright (C) Roman Ukhov 2000
**		e-mail: r_ukhov@yahoo.com
**		http://rukhov.newmail.ru
*/

#if !defined(_THREAD_HPP_INCLUDE_)
#define _THREAD_HPP_INCLUDE_

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
	

namespace MTNMsgApi_NP
{
	class CThread
	{
	public:
		typedef int (*thread_func)(void*);
		CThread(bool _auto_delete = false);
		virtual ~CThread();

		bool create(thread_func start_addr = 0, 
			void* _param = 0, bool suspended = false);

		bool wait_for_end(unsigned long tiomeout = 0);
		void resume() const;
		void suspend() const;
		bool set_priority(int priority) const;
    	unsigned int GetID(void)
    	{
    		return (unsigned int) m_dwThreadId;
    	}
	protected:
		virtual int thread_function();
	public:
    	pthread_t  m_dwThreadId;
	private:
		bool m_autoDelete;
		int  m_retVal;
		void* m_Param;
		thread_func m_startAddres;
    	bool        m_bStopFlag;//是否已经“取消”该线程
    	bool        m_bStarted;//线程已经启动

    	//
		static void* static_start_func(void *);
	};
	
	
	class sinchronization_object
	{
		public:
			sinchronization_object(){}
			virtual ~sinchronization_object(){}

			virtual void lock() const = 0;
			virtual void unlock() const = 0;
		private:
			sinchronization_object(const sinchronization_object&){}
			const sinchronization_object& operator = (const sinchronization_object&);
	};

	
	class mutex_object : public sinchronization_object
	{
	public:
		mutex_object(){
			pthread_mutex_init(&m_Mutex,NULL);
		}
		virtual ~mutex_object(){
			pthread_mutex_destroy(&m_Mutex);
		}
		virtual void lock() const {
			pthread_mutex_lock(&m_Mutex);
		}
		virtual void unlock() const {
			pthread_mutex_unlock(&m_Mutex);
		}
	private:
		mutex_object(const mutex_object& x){}
		const mutex_object& operator = (const mutex_object& x) const{return *this;}
		mutable pthread_mutex_t  m_Mutex;
	};

	class auto_lock
	{
	public:
		auto_lock(const mutex_object& cs) : m_mutexObj(cs){
			m_mutexObj.lock();
		}
		virtual ~auto_lock(){
			m_mutexObj.unlock();
		}

	private:
		const mutex_object& m_mutexObj;
	};
	
}

#endif //_THREAD_HPP_INCLUDE
	
	


