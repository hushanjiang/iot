
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
 

#ifndef _ERROR_CODE_H
#define _ERROR_CODE_H


enum EnErrCode
{ 
	ERR_SUCCESS = 0,
	ERR_SYSTEM = -1,

	//Common
	ERR_COMMON_BEGIN 				         = -400,
	ERR_QUEUE_FULL 					         = -401,    //队列满
	ERR_PUSH_QUEUE_FAIL 			         = -402,    //插入队列失败
	ERR_INVALID_REQ 				         = -403,    //请求串格式非法
	ERR_REACH_MAX_MSG 				         = -404,    //请求串大于最大长度
	ERR_REACH_MIN_MSG 				         = -405,    //请求串小于最小长度
	ERR_SEND_FAIL 					         = -406,    //发送失败
	ERR_RCV_FAIL 					         = -407,    //接收失败
	ERR_SESSION_TIMEOUT 			         = -408,    //会话超时
	ERR_NO_CONN_FOUND 				         = -409,    //找不到conn
	ERR_GET_MYSQL_CONN_FAILED 		         = -410,    //获取mysql 链接失败
	ERR_MYSQL_OPER_EXCEPTION 			     = -411,    //MYSQL操作异常
	ERR_DUPLICATE_KEY 				         = -412,    //重复键
	ERR_TABLE_EXISTS 				         = -413,    //数据表存在
	ERR_REACH_MAX_MSG_CNT 			         = -414,    //达到每秒最多消息个数
	ERR_NO_WORKER_FOUND 			         = -415,    //没有找到对应的worker
	ERR_WORKER_ID_EXIST 			         = -416,    //worker id 已经存在		
	ERR_WORKER_FD_EXIST 			         = -417,    //worker fd 已经存在		
	ERR_BASE64_ENCODE_FAILED		         = -418,    //BASE64 编码失败
	ERR_BASE64_DECODE_FAILED		         = -419,    //BASE64 解码失败
	ERR_HMAC_ENCODE_FAILED			         = -420,    //hmac-sha1编码失败
	ERR_REDIS_OPER_EXCEPTION                 = -421,	//redis操作异常

	//LB
	ERR_LB_BEGIN = -10000,
	
	//UID
	ERR_UID_BEGIN = -11000,

	//Conf
	ERR_CONF_BEGIN = -12000,

	//Security 
	ERR_SECURITY_BEGIN = -13000,

	//Access svr
	ERR_ACCESS_BEGIN                        = -14000,
	ERR_DATA_DECRYPT_FAILED                 = -14001,   //数据解码失败
	ERR_GET_SECURITY_CHANNEL_FAILED         = -14002,   //获取安全通道失败
	ERR_SECURITY_HEAD_DECODE_FAILED         = -14003,   //安全头解码失败
	ERR_CLIENT_NOT_AUTH                     = -14004,   //还没验证登录
	ERR_CHECK_TALK_CONDITION_FAILED         = -14005,   //无法发送通道消息
	ERR_GET_MEMBER_ID_LIST_FAILED           = -14006,   //获取家庭成员ID列表失败

	//Device Mgt
	ERR_DEVICE_MGT_BEGIN 					= -15000,
	ERR_ROUTER_UUID_NOT_EXIST				= -15001,	// 路由器的uuid不存在
	ERR_ROUTER_PWD_NOT_CORRECT				= -15002,	// 路由器的随机码不正确
	ERR_DEVICE_FAMILY_ID_IS_NOT_EXIST		= -15003,	// 路由器绑定的家庭id不存在
	ERR_DEVICE_USER_IS_NOT_OWNER			= -15004,	// 操作设备权限不足，不是户主
	ERR_ROUTER_UNBIND_FAMILY_ID_NOT_CORRECT	= -15005,	// 解绑操作，家庭id和路由器id不对应
	ERR_ROUTER_CANNOT_GET_USERS_ACCOUNT		= -15006,	// 路由器查询账户信息，然而用户不属于绑定这个路由器的家庭
	ERR_ROUTER_GET_USERS_ACCOUNT_FAILED		= -15007,	// 路由器查询账户信息，查数据库失败
	ERR_ROUTER_GET_BIND_FAMILY_FAILED		= -15008,	// 通过路由器查找绑定的家庭失败
	ERR_DEVICE_ID_NOT_EXIST					= -15009,	// 设备的id不存在
	ERR_ROUTER_ID_NOT_EXIST					= -15010,	// 路由器的id不存在
	ERR_ADD_TOO_MANY_ROOMS					= -15011,	// 一次添加太多房间，最多10个
	ERR_ADD_ROOMS_FAILED					= -15012,	// 批量插入房间失败
	ERR_DELETE_TOO_MANY_ROOMS				= -15013,	// 一次删除太多房间，最多10个
	ERR_DELETE_ROOMS_FAILED					= -15014,	// 批量删除房间失败
	ERR_UPDATE_TOO_MANY_ROOMS				= -15015,	// 一次修改太多房间，最多10个
	ERR_UPDATE_ROOMS_FAILED					= -15016,	// 批量修改房间失败
	ERR_ADD_TOO_MANY_DEVICES				= -15017,	// 一次添加太多设备，最多10个
	ERR_ADD_DEVICES_FAILED					= -15018,	// 批量插入设备失败
	ERR_DELETE_TOO_MANY_DEVICES				= -15019,	// 一次删除太多设备，最多10个
	ERR_DELETE_DEVICES_FAILED				= -15020,	// 批量删除设备失败
	ERR_UPDATE_TOO_MANY_DEVICES				= -15021,	// 一次修改太多设备，最多10个
	ERR_UPDATE_DEVICES_FAILED				= -15022,	// 批量修改设备失败
	ERR_ADD_TOO_MANY_SHORTCUTS				= -15023,	// 一次添加太多快捷方式，最多10个
	ERR_ADD_SHORTCUTS_FAILED				= -15024,	// 批量插入快捷方式失败
	ERR_DELETE_TOO_MANY_SHORTCUTS			= -15025,	// 一次删除太多快捷方式，最多10个
	ERR_DELETE_SHORTCUTS_FAILED				= -15026,	// 批量删除快捷方式失败
	ERR_UPDATE_TOO_MANY_SHORTCUTS			= -15027,	// 一次修改太多快捷方式，最多10个
	ERR_UPDATE_SHORTCUTS_FAILED				= -15028,	// 批量修改快捷方式失败

	//Family Mgt
	ERR_AUTHENTIONCATION_BEGIN 				= -16000,
	ERR_MEMBER_UPDATE_FAMILY_INFO 			= -16001,	// 普通成员不能编辑家庭信息
	ERR_MEMBER_SWITCH_BUT_DONOT_JOIN_FAMILY	= -16002,	// 用户切换家庭前需要先加入家庭
	ERR_MEMBER_BIND_FAMILY					= -16003,	// 普通成员不能绑定家庭
	ERR_ROUTER_UUID_DONOT_EXIST				= -16004,	// 绑定家庭，路由器的uuid不存在
	ERR_PWD_DONOT_CORRECT					= -16005,	// 绑定家庭，路由器的密码不正确
	ERR_ROUTER_HAVE_BIND_OTHER_FAMILY		= -16006,	// 路由器已经绑定了其他家庭
	ERR_MEMBER_UNBIND_FAMILY				= -16007,	// 普通成员不能解绑家庭
	ERR_UNBIND_BUT_DONOT_BIND				= -16008,	// 没用绑定家庭，直接解绑
	ERR_MEMBER_APPLY_CODE					= -16009,	// 普通成员不能获取邀请码
	ERR_CANNOT_WRITER_CODE_TO_REDIS			= -16010,	// 邀请码写入redis失败
	ERR_APPLY_CODE_DONOT_CORRECT			= -16011,	// 邀请码不正确
	ERR_APPLY_CODE_EXPIRE					= -16012,	// 邀请码已经超时
	ERR_OWNER_INVITER_HIS_SELF				= -16013,	// 户主不能邀请自己加入
	ERR_GET_OWNER_ACCOUNT_INFO_FAILED		= -16014,	// 查询户主的个人账号信息失败
	ERR_GET_MEMBER_ACCOUNT_INFO_FAILED		= -16015,	// 查询用户个人账号信息失败
	ERR_GET_INVITATION_BUT_DONOT_JOIN_FAMILY= -16016,	// 用户获取邀请函之前，需要先加入家庭
	ERR_ONLY_OWNER_CAN_ACCEPT_TO_JOIN		= -16017,	// 只有户主可以审批能否加入家庭
	ERR_ONLY_OWNER_CAN_REMOVE_MEMBER		= -16018,	// 只有户主可以删除成员
	ERR_OWNER_REMOVE_HISSELF				= -16019,	// 户主不能删除自己
	ERR_MEMBER_UPDATE_OTHERS_INFO			= -16020,	// 只有户主或成员自己可以编辑成员信息
	ERR_MEMBER_GET_ACCOUT_INFO_FAILED		= -16021,	// 获取成员账户信息失败
	ERR_P2P_TALK_BUT_DONOT_JOIN_SAME_FAMILY	= -16022,	// 用户之间通话，需要加入同一个家庭
	ERR_P2R_USERS_FAMILY_DONOT_BIND_ROUTER	= -16023,	// 用户访问路由器，需要加入绑定路由器的家庭
	ERR_P2F_USERS_DONNOT_JOIN_FAMILY		= -16024,	// 用户访问家庭，需要先加入家庭
	ERR_R2P_USERS_FAMILY_DONOT_BIND_ROUTER	= -16025,	// 路由器通知用户，用户不数据绑定路由器的家庭
	ERR_R2F_FAMILY_DONOT_BIND_ROUTER		= -16026,	// 家庭没有绑定路由器，不能访问
	ERR_FAMILY_ID_IS_NOT_EXIST				= -16027,	// 家庭id不存在

	//User Mgt
	ERR_USER_MGT_BEGIN = -17000,
	ERR_USER_INVALID_CODE                   = -17001,   //code 无效
	ERR_USER_INVALID_SECRET                 = -17002,   //标据无效
	ERR_USER_PWD_COMPLEX                    = -17003,   //密码复杂度不够
	ERR_USER_PHONE_COMPLEX                  = -17004,   //手机号码复杂度不够
	ERR_USER_PHONE_NOT_REGISTE              = -17005,   //手机号码未注册
	ERR_USER_PHONE_REGISTED                 = -17006,   //手机号码已经注册
	ERR_TOKEN_SIG_ERROR                     = -17007,   //token 签名错误
	ERR_TOKEN_GENERATE_ERROR                = -17008,   //token 生成失败
	ERR_TOKEN_INVALID                       = -17009,	//token 无效
	ERR_TOKEN_EXPIRED                       = -17010,	//token 过期
	ERR_SECRET_EXPIRED                      = -17011,	//票据过期
	ERR_CODE_EXPIRED                        = -17012,	//code 无效
	ERR_SECRET_INVALID                      = -17013,	//票据无效
	ERR_USER_ID_NOT_EXSIT                   = -17014,	//用户ID不存在
	ERR_USER_LOGIN_FORBID                   = -17015,	//用户帐号被禁用
	ERR_CODE_NOT_EXIST                      = -17016,	//code不存在,请先发送验证码
	ERR_CODE_NOT_CORRECT                    = -17017,	//code 不正确
	ERR_USER_ID_NOT_EXIST                   = -17018,	//用户ID不存在                    
	ERR_GET_PHONE_CODE_LIMIT                = -17019,	//每分钟只能发一次验证码			
	ERR_GET_PHONE_CODE_SEND_SMS_ERROR       = -17020,	//发送验证码失败               
	ERR_CHECK_PHONE_CODE_NOT_CORRECT        = -17021,	//验证码不正确
	ERR_CHECK_PHONE_CODE_NOT_EXIST          = -17022,	//验证码不存在，已经过期                 	
	ERR_REGISTER_PHONE_REGISTED             = -17023,	//手机号码已经注册
	ERR_REGISTER_GET_UID_FAIL               = -17024,	//注册时获取UID的接口返回错误                    
	ERR_LOGIN_CODE_TRY_LIMIT                = -17025,	//code登录错误次数超限                   
	ERR_LOGIN_PWD_NOT_CORRECT               = -17026,	//密码错误
    ERR_PWD_OLD_NOT_CORRECT                 = -17027,	//老密码不对                       
	ERR_AUTH_TOKEN_NOT_MATCH                = -17028,	//token不匹配
	ERR_PHONE_CODE_DAY_LIMIT				= -17029,	//获取验证码超过1天的上限,1天最多20次
	ERR_PHONE_CODE_SLOT_LIMIT				= -17030,   //在当前时间段内超过了上限，15分钟内不超过10次


	//MDP
	ERR_MDP_BEGIN                            = -18000,

	//Push
	ERR_PUSH_BEGIN                           = -19000,

	//SMS
	ERR_SMS_BEGIN                            = -20000,
	
	ERR_END
};


#endif


