
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

 #ifndef _MONGO_MGT_H
 #define _MONGO_MGT_H
 
 #include "base/base_thread_mutex.h"
 #include "base/base_thread.h"
 #include "base/base_queue.h"
 #include "base/base_singleton_t.h"
 #include "comm/common.h"
 
 
 USING_NS_BASE;
 
 typedef struct _StDeviceStatusMsg
 {
	unsigned long long _id;
	std::string _date;
	std::string _msg;
 }StDeviceStatusMsg;
 
 /*
 Mongo_Mgt  是独立的线程负责将异步队列中的消息存储到mongodb中
 */
 class Mongo_Mgt : public Thread
 {
 public:
	 Mongo_Mgt();
	 virtual ~Mongo_Mgt();
	 
	 void update(StMongo_Access stMongo_Access);
 
	 int storage(StDeviceStatusMsg &msg);
 
	 int get_device_status_message(const unsigned long long familyId, const unsigned long long deviceId, const std::string &date,
		const unsigned int skip, const unsigned int limit, std::deque<std::string> &msg_list, long long &total, std::string &err_info);
 	 
 private:
	 virtual int svc();
	 virtual int do_init(void *args);
 
 private:
	 X_Queue<StDeviceStatusMsg> _queue;
	 StMongo_Access _stMongo_Access;
	 
 };
 
 #define PSGT_Mongo_Mgt Singleton_T<Mongo_Mgt>::getInstance()
 
 
 #endif
 
 