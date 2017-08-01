
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

 

#ifndef _SESSION_H
#define _SESSION_H

#include "base/base_common.h"
#include "base/base_smart_ptr_t.h"


USING_NS_BASE;

class Session
{
public:
	Session();

	~Session();

	void log();

	std::string to_string();
	
public:
	std::string _id;
	int _fd;
	std::string _ip;
	unsigned short _port;
	bool _status;
	unsigned long long _create_time;  //Œ¢√Î
	unsigned long long _access_time;
	unsigned long long _close_time;
	unsigned long long _r_num;
	unsigned long long _w_num;
		
};
typedef Smart_Ptr_T<Session>  Session_Ptr;

#endif


