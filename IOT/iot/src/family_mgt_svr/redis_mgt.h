
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
 * Author: 89617663@qq.com
 */
 

#ifndef _REDIS_MGT_H
#define _REDIS_MGT_H

#include "base/base_thread_mutex.h"
#include <hiredis/hiredis.h>
#include "base/base_singleton_t.h"
#include "comm/common.h"
#include "redis_conn.h"


USING_NS_BASE;

class redis_mgt
{
public:
    redis_mgt();
    virtual ~redis_mgt();

	//初始化
	int init(const std::string &ip, unsigned int port, const std::string &pwd,const unsigned int cnt);

	bool get_conn(redis_conn_Ptr &conn);

	//检测redis状态并且按照周期要求创建数据表
	void check();

	void release(redis_conn_Ptr &conn);

private:
	//数据库连接池
	std::vector<redis_conn_Ptr> _conn_queue;
	Thread_Mutex _mutex;

};

#define PSGT_Redis_Mgt Singleton_T<redis_mgt>::getInstance()

//----------------------------
//保护类， 不支持拷贝构造
class Redis_Guard : public noncopyable
{
public:

	//构造MySQL_Guard 对象只能使用下面这种方式
	Redis_Guard(redis_conn_Ptr &conn);

	~Redis_Guard();

	redis_conn_Ptr& operator-> ();

private:
	redis_conn_Ptr _conn;

};



#endif

