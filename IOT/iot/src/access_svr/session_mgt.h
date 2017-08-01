
/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: cuipingxu918@qq.com
 */

 

#ifndef _SESSION_MGT_H
#define _SESSION_MGT_H

#include "base/base_common.h"
#include "base/base_singleton_t.h"
#include "base/base_thread_mutex.h"
#include "session.h"

USING_NS_BASE;

class Session_Mgt
{
public:
	Session_Mgt();

	~Session_Mgt();

	void insert_session(int fd);

	void remove_session(int fd);

	bool is_valid_sesssion(const std::string &id);

	void update_status(int fd, bool status);

	void update_read(int fd, unsigned long long num, std::string &id);

	void update_write(int fd, unsigned long long num);

	void check();
		
private:
	Thread_Mutex _mutex;
	std::map<int, Session> _sessions;
	std::map<std::string, int> _fds;
	
};

#define PSGT_Session_Mgt Singleton_T<Session_Mgt>::getInstance()


#endif


