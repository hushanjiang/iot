
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

 
#ifndef _DEVICE_MGT_H
#define _DEVICE_MGT_H

#include "base/base_common.h"
#include "base/base_singleton_t.h"
#include "base/base_thread_mutex.h"
#include "comm/common.h"
#include "mysql_mgt.h"

USING_NS_BASE;

#define uint64 unsigned long long

typedef struct _StDeviceInfo
{
	uint64 id;
	uint64 routerId;
	enum DM_DEVICE_CATEGORY productId;
	uint64 businessId;
	std::string name;
	std::string deviceId;	// 设备的uuid
	enum DM_DEVICE_STATE state;
	std::string attr;	

	uint64 familyId;
	uint64 roomId;
	uint64 creatorId;
	
	uint64 createAt;
	uint64 updateAt;

	_StDeviceInfo():id(0),routerId(0),productId(DM_DEVICE_CATEGORY_END),businessId(0),name(""),deviceId(""),
		state(DM_DEVICE_STATE_END),attr(""),familyId(0),roomId(0),creatorId(0),createAt(0),updateAt(0)
	{
	}
}StDeviceInfo;

typedef struct _StRoomInfo
{
	uint64 id;
	uint64 familyId;
	uint64 creatorId;
	uint64 createAt;
	uint64 updateAt;
	unsigned int orderNum;
	unsigned int isDefault;
	std::string name;
	_StRoomInfo():id(0),familyId(0),creatorId(0),createAt(0),updateAt(0), orderNum(0),isDefault(0),name("")
	{
	}
}StRoomInfo;

typedef struct _StRouterInfo
{
	uint64 routerId;
	uint64 familyId;
	std::string uuid;
	std::string name;
	std::string snNo;
	std::string mac;
	unsigned int state;
	std::string attr;
	uint64 creatAt;

	_StRouterInfo():routerId(0),familyId(0),uuid(""),name(""),snNo(""),mac(""),state(0),attr(""),creatAt(0)
	{
	}
}StRouterInfo;

typedef struct _StDeviceStatus
{
	uint64 id;
	uint64 deviceId;
	uint64 familyId;
	enum DM_DEVICE_STATUS_TYPE type;
	std::string msg;
	uint64 createAt;
	_StDeviceStatus():id(0),deviceId(0),familyId(0),type(DM_DEVICE_STATUS_TYPE_END),msg(""),createAt(0)
	{
	}
}StDeviceStatus;

typedef struct _StDeviceAlert
{
	uint64 id;
	uint64 deviceId;
	uint64 familyId;
	enum DM_DEVICE_ALERT_TYPE type;
	std::string msg;
	uint64 handler;		//处理人
	enum DM_DEVICE_ALERT_STATUS status;
	uint64 createAt;
	uint64 handleAt;	//处理时间
	_StDeviceAlert():id(0),deviceId(0),familyId(0),type(DM_DEVICE_ALERT_TYPE_END),
		msg(""),handler(0),status(DM_DEVICE_ALERT_STATUS_END),createAt(0),handleAt(0)
	{
	}
}StDeviceAlert;

class Device_Mgt
{
public:
	Device_Mgt();

	~Device_Mgt();

public:
	// 房间管理
	int add_room(const StRoomInfo &info, uint64 &roomId);

	int delete_room(const uint64 &userId, const uint64 &roomId, const uint64 &familyId);

	int update_room(const uint64 &userId, const StRoomInfo &info);

	int get_room_list(const uint64 &familyId, std::vector<StRoomInfo> &infos);

	int update_room_order(const uint64 &userId, const uint64 &familyId, const std::vector<StRoomInfo> &infos);

public:
	// 设备管理
	int add_device(const StDeviceInfo &info);

	int delete_device(const uint64 &userId, const StDeviceInfo &info);

	int update_device(const uint64 &userId, const StDeviceInfo &info);

	int get_device_info(const uint64 &userId, StDeviceInfo &info);

	int get_devices_by_room(const uint64 &userId, const uint64 &familyId, const uint64 &roomId, const uint64 &routerId, enum DM_DEVICE_CATEGORY category,
			std::vector<StDeviceInfo> &infos, const unsigned int &beginAt, const unsigned int &size, unsigned int &left);

	int get_devices_by_family(const uint64 &userId, const uint64 &familyId, const uint64 &routerId, enum DM_DEVICE_CATEGORY category,
			std::vector<StDeviceInfo> &infos, const unsigned int &beginAt, const unsigned int &size, unsigned int &left);
	
public:
	// 设备状态管理
	int report_status(const StDeviceStatus &status, uint64 &statusId);
	
	int report_alert(const StDeviceAlert &alert, uint64 &alertId);

	int get_dev_status_list(const uint64 &userId, const uint64 &deviceId,
			std::vector<StDeviceStatus> &status, const unsigned int &beginAt, const unsigned int &size, unsigned int &left);

	int get_dev_alert_list(const uint64 &userId, const uint64 &deviceId,
			std::vector<StDeviceAlert> &alert, const unsigned int &beginAt, const unsigned int &size, unsigned int &left);

public:
	// 路由器管理
	int router_auth(const std::string &uuid, const std::string &pwd, uint64 &routerId);

	int router_bind(const uint64 userId, const uint64 familyId, const uint64 routerId);

	int router_unbind(const uint64 userId, const uint64 familyId, const uint64 routerId);

	int router_get_info(const uint64 &routerId, StRouterInfo &info);
};

#define PSGT_Device_Mgt Singleton_T<Device_Mgt>::getInstance()

#endif


