
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
 

#ifndef _COMMON_H
#define _COMMON_H

#include "base/base_common.h"
#include "cmd.h"
#include "error_code.h"

USING_NS_BASE;


//----------- 系统限制-----------

const unsigned int MIN_MSG_LEN = 35;                  //最小消息长度{"head":{"cmd":"hb", "time":111111}}
const unsigned int MAX_MSG_LEN = 20480;               //最大消息长度20KB
const unsigned int MSG_CNT_PER_SEC = 100;             //每个链接每秒中最多发送几条消息   
const unsigned int SECURITY_CHANNEL_TTL = 259200;     //安全通道key的ttl设置(缺省3天)
const unsigned int MAX_SEARCH_CLIENT_CNT = 500;       //每次从Redis 获取最大客户端数
const unsigned int MAX_CLIENT_CNT = 20000;            //Access Svr 最大并发限制
const std::string  AES_CBC_IV = "1234567890123456";   //AES对称加密解密IV

const unsigned int MAX_UUID_LEN = 64;                 //客户端UUID 长度
const unsigned int MAX_AES_KEY_LEN = 32;              //AES key 长度


//---------------------------  全局变量-------------------------------

//服务类型
const std::string ST_LB = "lb_svr";                   //LB Svr
const std::string ST_ACCESS = "access_svr";           //Access Svr
const std::string ST_USER_MGT = "user_mgt_svr";       //User Mgt Svr
const std::string ST_DEVICE_MGT = "device_mgt_svr";   //Device Mgt Svr
const std::string ST_FAMILY_MGT = "family_mgt_svr";   //Family Mgt Svr
const std::string ST_MDP = "mdp";                     //Mdp
const std::string ST_CONF = "conf_svr";               //Conf Svr
const std::string ST_UID = "uid_svr";                 //Uid Svr
const std::string ST_SECURITY = "security_svr";       //Security Svr
const std::string ST_PUSH = "push_svr";               //Push Svr
const std::string ST_SMS = "sms_svr";                 //SMS Svr
const std::string ST_BUSINESS = "business_svr";       //业务服务器统称


//终端类型
const std::string TT_APP = "APP";                     //APP
const std::string TT_IOS = "IOS";                     //IOS
const std::string TT_ANDRIOD = "Andriod";             //Andriod
const std::string TT_ROUTER = "Router";               //Router
const std::string TT_UNKNOWN = "unknown";             //unknown


//会话类型
const std::string MT_P2P = "P2P";                             //app --> app
const std::string MT_P2R = "P2R";                             //app --> router
const std::string MT_P2F = "P2F";                             //app --> family
const std::string MT_R2P = "R2P";                             //router -->app
const std::string MT_R2F = "R2F";                             //router -->family
const std::string MT_BROADCAST = "broadcast";                 //广播all
const std::string MT_BROADCAST_APP = "broadcast_app";         //广播app
const std::string MT_BROADCAST_ROUTER = "broadcast_router";   //广播路由器


//Method 前缀
const std::string MP_UM = "um_";                      //user mgt svr request
const std::string MP_FM = "fm_";                      //family mgt svr request
const std::string MP_DM = "dm_";                      //device mgt svr request


// 用户状态
const std::string US_ONLINE = "online";          //在线
const std::string US_OFFLINE = "offline";        //离线
const std::string US_KICKOFF = "kickoff";        //踢下线 


//系统配置键值
const std::string SYS_SEND_RATE = "send_rate";          //单个长连接发送速率
const std::string SYS_MAX_CLIENT = "max_client";        //接入服务最大客户端数



//---------------------------  全局枚举-------------------------------

// 家庭成员角色
enum member_role_in_family
{
	FAMILY_ROLE_OWNER = 0,
	FAMILY_ROLE_MEMBER = 1,

	FAMILY_ROLE_END
};



// 性别
enum member_gender
{
	GENDER_MALE = 0,
	GENDER_FEMALE = 1,

	GENDER_END
};


//token的类型
enum TokenType {
	TokenTypeBegin			= 0,
	TokenTypeGetAccount		= 1,
	TokenTypeInviteMember	= 2,
	TokenTypeLogin			= 3,
	TokenTypeUpdatePwd		= 4,
	TokenTypeEnd			= 5
};


// 设备类型
enum DM_DEVICE_CATEGORY
{
	DM_DEVICE_CATEGORY_AIR_CONDITIONER 		= 1,	// 空调
	DM_DEVICE_CATEGORY_WINDOW_CURTAINS 		= 2,	// 窗帘
	DM_DEVICE_CATEGORY_TELEVISION			= 3,	// 电视
	DM_DEVICE_CATEGORY_ELECTRIC_COOKER		= 4,	// 电饭煲
	DM_DEVICE_CATEGORY_LAMP					= 5,	// 灯
	DM_DEVICE_CATEGORY_ELECTRONIC_BALANCE	= 6,	// 电子秤
	DM_DEVICE_CATEGORY_CAMERA				= 7,	// 摄像头
	DM_DEVICE_CATEGORY_INTELLIGENT_SWITCH	= 8,	// 智能开关
	DM_DEVICE_CATEGORY_AIR_QUALITY_DETECTOR	= 9,	// 空气质量检测仪
	DM_DEVICE_CATEGORY_HYGROTHERMOGRAPH		= 10,	// 温湿度计
	DM_DEVICE_CATEGORY_ROUTER				= 11,	// 路由器
	DM_DEVICE_CATEGORY_DOOR_SENSOR			= 12,	// 门窗传感器
	DM_DEVICE_CATEGORY_BODY_SENSOR			= 13,	// 人体传感器
	DM_DEVICE_CATEGORY_VISUAL_INTERCOM		= 14,	// 可视对讲机

	DM_DEVICE_CATEGORY_END
};



// 设备有效无效
enum DM_DEVICE_STATE
{
	DM_DEVICE_STATE_ENABLE = 0,
	DM_DEVICE_STATE_DISNABLE = 1,

	DM_DEVICE_STATE_END
};



// 告警处理状态
enum DM_DEVICE_ALERT_STATUS
{
	DM_DEVICE_ALERT_STATUS_INITIAL = 0,
	DM_DEVICE_ALERT_STATUS_HANDLED = 1,
	DM_DEVICE_ALERT_STATUS_IGNORE = 2,

	DM_DEVICE_ALERT_STATUS_END
};



// 设备状态类型
enum DM_DEVICE_STATUS_TYPE
{
	DM_DEVICE_STATUS_TYPE_TEST = 1,
		
	DM_DEVICE_STATUS_TYPE_END
};



// 设备告警类型
enum DM_DEVICE_ALERT_TYPE
{
	DM_DEVICE_ALERT_TYPE_TEST = 1,
		
	DM_DEVICE_ALERT_TYPE_END
};



//---------------------------  全局结构体-------------------------------


//系统配置结构体
typedef struct StSysInfo
{
	std::string _id;                             //原始ID
	std::string _new_id;                         //新ID
	std::string _log_id;                         //日志ID
	unsigned int _pid;                           //进程ID
	std::string _ip;                             //IP地址
	std::string _ip_out;                         //公网IP地址
	unsigned short _port;                        //独立端口
	unsigned short _worker_port;                 //注册端口
	unsigned short _thr_num;                     //工作线程个数
	unsigned int _max_queue_size;                //请求队列最大尺寸
	unsigned int _rate_limit;                    //速率限制
	std::string _TZ;                             //时区
	
	StSysInfo():_id(""), _new_id(""), _ip(""), _port(0), _worker_port(0),
		_thr_num(0), _max_queue_size(0), _rate_limit(0), _TZ("")
	{
	}
	
}StSysInfo;


//----------------------------------------------


typedef struct StSvr
{
   	std::string _ip;                   //服务IP
   	std::string _ip_out;               //公网IP地址
	unsigned short _port;          	   //服务端口

	StSvr(): _ip(""), _ip_out(""), _port(0)
	{
	}
	
} StSvr;


//----------------------------------------------


//mysql 访问配置信息结构体
typedef struct StMysql_Access
{
	std::string _ip;
	unsigned short _port;
	std::string _db;
	std::string _user;
	std::string _pwd;
	std::string _chars;
	unsigned int _num;
	
	StMysql_Access(): _ip(""), _port(0), _db(""), _user(""), _pwd(""), _chars(""), _num(1)
	{
	}
	
} StMysql_Access;



//Mongo 访问配置信息结构体
typedef struct StMongo_Access
{
	std::string _ip;
	unsigned short _port;
	std::string _server;
	std::string _db;
	std::string _collection;
	
	StMongo_Access(): _ip(""), _port(0), _server(""), _db(""), _collection("")
	{
	}
	
} StMongo_Access;




//Redis 访问配置信息结构体
typedef struct StRedis_Access
{
	std::string _ip;
	unsigned short _port;
	std::string _auth;
	
	StRedis_Access(): _ip(""), _port(0), _auth("")
	{
	}
	
} StRedis_Access;



//----------------------------------------------


/*
客户端信息结构体
4GB 内存最多可以存储4000W客户端记录(每个客户端100字节)
4GB 内存管理2000W 客户端记录部署
*/
typedef struct StClient
{   
	unsigned long long _id;            //客户端ID:  用户ID，Router ID
	std::string _type;                 //客户端类型
	std::string _session_id;           //会话ID

	std::string _uuid;                 //安全通道ID 
	std::string _key;                  //安全通道秘钥

	int _fd;                           //该客户端fd
	std::string _ip;                   //客户端的IP
	unsigned short _port;              //客户端端口
	
	unsigned long long _stmp_create;    //该客户端创建时间戳
	std::string _geography;             //地理位置
	
	StClient(): _id(0), _type(TT_UNKNOWN), _session_id(""), _uuid(""), _key(""), _fd(-1), _ip(""), _port(0), 
		_stmp_create(0), _geography("")
	{
	
	}
	
} StClient;



#endif

