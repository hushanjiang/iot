
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

	//Common -400 ~ -2
	ERR_COMMON_BEGIN = -400,
	ERR_QUEUE_FULL = -399,              //队列满
	ERR_PUSH_QUEUE_FAIL = -398,         //插入队列失败
	ERR_INVALID_REQ = -397,             //请求串格式非法
	ERR_REACH_MAX_MSG = -396,           //请求串大于最大长度
	ERR_REACH_MIN_MSG = -395,           //请求串小于最小长度
	ERR_SEND_FAIL = -394,               //发送失败
	ERR_RCV_FAIL = -393,                //接收失败
	ERR_SESSION_TIMEOUT = -392,         //会话超时
	ERR_NO_CONN_FOUND = -391,           //找不到conn
	ERR_GET_MYSQL_CONN_FAILED = -390,   //获取mysql 链接失败
	ERR_DUPLICATE_KEY = -389,           //重复键
	ERR_TABLE_EXISTS = -388,            //数据表存在
	ERR_REACH_MAX_MSG_CNT = -387,       //达到每秒最多消息个数
	ERR_NO_WORKER_FOUND = -386, 		//没有找到对应的worker
	ERR_WORKER_ID_EXIST = -385,         //worker id 已经存在		
	ERR_WORKER_FD_EXIST = -384, 		//worker fd 已经存在

	//LB
	ERR_LB_BEGIN = -10000,
	
	//UID
	ERR_UID_BEGIN = -11000,

	//Conf
	ERR_CONF_BEGIN = -12000,

	//Security 
	ERR_SECURITY_BEGIN = -13000,

	//Access
	ERR_ACCESS_BEGIN = -14000,
	ERR_DATA_DECRYPT_FAILED = ERR_ACCESS_BEGIN - 10,     //数据解码失败

	//Device Mgt
	ERR_DEVICE_MGT_BEGIN = -15000,

	//Family Mgt
	ERR_AUTHENTIONCATION_BEGIN = -16000,

	//User Mgt
	ERR_USER_MGT_BEGIN = -17000,
	//----UserCommon
	ERR_UNKNOW                              = ERR_USER_MGT_BEGIN - 10, //无知错误
	ERR_LESS_PARAM                          = ERR_USER_MGT_BEGIN - 20, //缺少参数
	ERR_USER_INVALID_CODE                   = ERR_USER_MGT_BEGIN - 30, //code 无效
	ERR_USER_INVALID_SECRET                 = ERR_USER_MGT_BEGIN - 40, //标据无效
	ERR_USER_PWD_COMPLEX                    = ERR_USER_MGT_BEGIN - 50, //密码复杂度不够
	ERR_USER_PHONE_COMPLEX                  = ERR_USER_MGT_BEGIN - 60, //手机号码复杂度不够
	ERR_USER_PHONE_NOT_REGISTE              = ERR_USER_MGT_BEGIN - 70, //手机号码未注册
	ERR_USER_PHONE_REGISTED                 = ERR_USER_MGT_BEGIN - 80, //手机号码已经注册
	ERR_TOKEN_SIG_ERROR                     = ERR_USER_MGT_BEGIN - 90, //token 签名错误
	ERR_TOKEN_GENERATE_ERROR                = ERR_USER_MGT_BEGIN - 100,//token 生成失败
	ERR_TOKEN_INVALID                       = ERR_USER_MGT_BEGIN - 110,//token 无效
	ERR_TOKEN_EXPIRED                       = ERR_USER_MGT_BEGIN - 120,//token 过期
	ERR_SECRET_EXPIRED                      = ERR_USER_MGT_BEGIN - 130,//票据过期
	ERR_CODE_EXPIRED                        = ERR_USER_MGT_BEGIN - 140,//code 无效
	ERR_SECRET_INVALID                      = ERR_USER_MGT_BEGIN - 150,//票据无效
	ERR_USER_ID_NOT_EXSIT                   = ERR_USER_MGT_BEGIN - 160,//用户ID不存在
	ERR_USER_LOGIN_FORBID                   = ERR_USER_MGT_BEGIN - 170,//用户帐号被禁用
	ERR_CODE_NOT_EXIST                      = ERR_USER_MGT_BEGIN - 180,//code不存在,请先发送验证码
	ERR_CODE_NOT_CORRECT                    = ERR_USER_MGT_BEGIN - 190,//code 不正确
	ERR_REDIS_NET_ERROR                     = ERR_USER_MGT_BEGIN - 200,//redis网络错误
	ERR_MYSQL_NET_ERROR                     = ERR_USER_MGT_BEGIN - 210,//mysql网络错误
	ERR_RESPONSE_BUF_ERROR                  = ERR_USER_MGT_BEGIN - 220,//回包错误
	ERR_INNER_SVR_NET_ERROR                 = ERR_USER_MGT_BEGIN - 230,//内部服务网络错误 
	ERR_USER_ID_NOT_EXIST                   = ERR_USER_MGT_BEGIN - 240,//用户ID不存在 

	//----get_phone_code                    
	ERR_GET_PHONE_CODE_LIMIT                = ERR_USER_MGT_BEGIN - 250,//每分钟只能发一次验证码			
	ERR_GET_PHONE_CODE_SEND_SMS_ERROR       = ERR_USER_MGT_BEGIN - 260,//发送验证码失败
	//-----check_phone_code                 
	ERR_CHECK_PHONE_CODE_NOT_CORRECT        = ERR_USER_MGT_BEGIN - 270,//验证码不正确
	ERR_CHECK_PHONE_CODE_NOT_EXIST          = ERR_USER_MGT_BEGIN - 280,//验证码不存在，已经过期

	//-----user_register                    	
	ERR_REGISTER_PHONE_REGISTED             = ERR_USER_MGT_BEGIN - 290,//手机号码已经注册
	ERR_REGISTER_GET_UID_FAIL               = ERR_USER_MGT_BEGIN - 300,//注册时获取UID的接口返回错误
	//------login code                      
	ERR_LOGIN_CODE_TRY_LIMIT                = ERR_USER_MGT_BEGIN - 310,//code登录错误次数超限
	//------login pwd                       
	ERR_LOGIN_PWD_NOT_CORRECT               = ERR_USER_MGT_BEGIN - 320,//密码错误
    ERR_PWD_OLD_NOT_CORRECT                 = ERR_USER_MGT_BEGIN - 321,//老密码不对
	//-------um auth                         
	ERR_AUTH_TOKEN_NOT_MATCH                = ERR_USER_MGT_BEGIN - 330,//token不匹配
	//------set pwd
	//------reset pwd
	//------get profile
	//------set profile

	//MDP
	ERR_MDP_BEGIN = -18000,

	//Push
	ERR_PUSH_BEGIN = -19000,

	//SMS
	ERR_SMS_BEGIN = -20000,
	
	ERR_END
};


#endif


