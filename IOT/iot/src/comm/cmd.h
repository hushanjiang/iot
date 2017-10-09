
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
 
#ifndef _CMD_H
#define _CMD_H


//公共
const std::string CMD_HB = "hb";                                                                        //心跳
const std::string CMD_REGISTER = "register";                                                            //注册服务
const std::string CMD_UNREGISTER = "unregister";                                                        //注销服务


//负载均衡
const std::string CMD_LB = "lb";                                                                        //负载均衡


//配置服务
const std::string CMD_GET_SERVER_ACCESS = "get_server_access";                                          //获取服务器访问信息
const std::string CMD_GET_USER_RIGHT = "get_user_right";                                                //获取用户权限列表
const std::string CMD_GET_SYS_CONFIG = "get_sys_config";                                                //获取系统配置


//接入服务器
const std::string CMD_LOGOUT = "acc_logout";                                                             //退出客户端(app和router)
const std::string CMD_CHECK_CLIENT_ONLINE = "acc_check_client_online";                                   //判断客户端是否在线


//ID 服务器
const std::string CMD_GET_UUID = "get_uuid";                                                            //获取UUID


//Key 服务器
const std::string CMD_CREATE_SECURITY_CHANNEL = "create_security_channel";                              //创建安全通道


//用户管理
const std::string CMD_REGISTER_USER = "um_register_user";                                               //注册用户
const std::string CMD_LOGIN_PWD = "um_login_pwd";                                                       //登录(密码)
const std::string CMD_LOGIN_CODE = "um_login_code";                                                     //登录(手机验证码)
const std::string CMD_AUTH = "um_auth";                                                                 //验证        
const std::string CMD_GET_PHONE_CODE = "um_get_phone_code";                                             //获取手机验证码
const std::string CMD_CHECK_PHONE_CODE = "um_check_phone_code";                                         //检查手机验证码
const std::string CMD_SET_PWD = "um_set_pwd";                                                           //设置密码
const std::string CMD_RESET_PWD = "um_reset_pwd";                                                       //通过手机验证码重新设置密码
const std::string CMD_GET_USER_PROFILE = "um_get_user_profile";                                         //获取用户信息
const std::string CMD_UPDATE_USER_PROFILE = "um_update_user_profile";                                   //更新用户信息
const std::string CMD_GET_USER_ACCOUNT = "um_get_user_account";                                         //获取用户账户信息


//家庭管理
const std::string CMD_CREATE_FAMILY = "fm_create_family";                                               //创建家庭
const std::string CMD_UPDATE_FAMILY = "fm_update_family";                                               //更新家庭信息
const std::string CMD_SWITCH_FAMILY = "fm_switch_family";                                               //切换家庭
const std::string CMD_GET_FAMILY_INFO = "fm_get_family_info";                                           //获取家庭信息
const std::string CMD_GET_FAMILY_LIST = "fm_get_family_list";                                           //获取家庭列表
const std::string CMD_APPLY_FAMILY = "fm_apply_family";                                                 //申请加入家庭(删除)
const std::string CMD_ACCEPT_FAMILY = "fm_accept_family";                                               //同意加入家庭(删除)
const std::string CMD_GET_APPLY_LIST = "fm_get_apply_list";                                             //获取申请加入家庭列表(删除)
const std::string CMD_GET_APPLY_CNT = "fm_get_apply_cnt";                                               //获取申请加入家庭数(删除)
const std::string CMD_GET_APPLY_CODE = "fm_get_apply_code";                                 			//获取用户申请码
const std::string CMD_GET_INVITATION = "fm_get_invitation";                                 			//获取用户邀请函
const std::string CMD_CREATE_MEMBER = "fm_create_member";											    //创建家庭成员
const std::string CMD_REMOVE_MEMBER = "fm_remove_member";                                               //移除家庭成员
const std::string CMD_UPDATE_MEMBER = "fm_update_member";                                               //更新家庭成员信息
const std::string CMD_GET_MEMBER_INFO = "fm_get_member_info";                                           //获取家庭成员信息
const std::string CMD_GET_MEMBER_LIST = "fm_get_member_list";                                           //获取家庭成员列表
const std::string CMD_GET_MEMBER_ID_LIST = "fm_get_member_id_list";                                     //获取家庭成员ID 列表
const std::string CMD_CHECK_TALK_CONDITION = "fm_check_talk_condition";                                 //判断聊天条件
const std::string CMD_FM_BIND_ROUTERP = "fm_bind_router";                                               //绑定路由器(来自APP)
const std::string CMD_FM_UNBIND_ROUTER = "fm_unbind_router";                                            //解绑定路由器(来自APP)


//房间管理
const std::string CMD_ADD_ROOM = "dm_add_room";                                                         //添加房间
const std::string CMD_DEL_ROOM = "dm_del_room";                                                         //删除房间
const std::string CMD_UPDATE_ROOM = "dm_update_room";                                                   //更新房间信息
const std::string CMD_GET_ROOM_LIST = "dm_get_room_list";                                               //获取房间列表
const std::string CMD_UPDATE_ROOM_ORDER = "dm_update_room_order";                                       //刷新房间顺序


//路由器管理
const std::string CMD_BIND_ROUTER = "dm_bind_router";                                                   //绑定路由器
const std::string CMD_UNBIND_ROUTER = "dm_unbind_router";                                               //解绑定路由器
const std::string CMD_AUTH_ROUTER = "dm_auth_router";                                                   //路由器 校验登录 
const std::string CMD_GET_ROUTER_INFO = "dm_get_router_info";                                           //获取路由器信息
const std::string CMD_CHECK_ROUTER = "dm_check_router";                                                 //验证 路由器 有效性
const std::string CMD_DM_GET_USER_ACCOUNT = "dm_get_user_account";                                      //获取用户账户信息（router调用） 


//设备管理
const std::string CMD_ADD_DEIVCE = "dm_add_device";                                                     //添加设备
const std::string CMD_DEL_DEVICE = "dm_del_device";                                                     //删除设备
const std::string CMD_UPDATE_DEVICE = "dm_update_device";                                               //刷新设备
const std::string CMD_GET_DEVICE_INFO = "dm_get_device_info";                                           //获取设备信息
const std::string CMD_GET_DEVICES_BY_ROOM = "dm_get_devices_by_room";                                   //按照房间获取设备列表
const std::string CMD_GET_DEVICES_BY_FAMILY = "dm_get_devices_by_family";                               //按照家庭获取设备列表
const std::string CMD_REPORT_DEV_STATUS = "dm_report_dev_status";                                       //上报设备状态
const std::string CMD_REPORT_DEV_ALERT = "dm_report_dev_alert";                                         //上报设备告警
const std::string CMD_GET_DEV_STATUS_LIST = "dm_get_dev_status_list";                                   //获取设备状态列表
const std::string CMD_GET_DEV_ALERT_LIST = "dm_get_dev_alert_list";                                     //上报设备告警列表
const std::string CMD_GET_DEV_KEY_PROPERTY = "dm_get_dev_key_property";                                 //获取设备关键属性

//快捷功能
const std::string CMD_ADD_SHORTCUT				= "dm_add_shortcut";                                    //添加快捷
const std::string CMD_DEL_SHORTCUT				= "dm_del_shortcut";                                    //删除快捷
const std::string CMD_GET_SHORTCUT_LIST			= "dm_get_shortcut_list";                               //获取快捷列表
const std::string CMD_UPDATE_SHORTCUT			= "dm_update_shortcut";                                 //更新快捷
const std::string CMD_UPDATE_SHORTCUT_ORDER     = "dm_sort_shortcut";									//修改快捷方式位置
const std::string CMD_GET_SHORTCUT_MODE			= "dm_get_shortcut_mode";                               //获取快捷模式
const std::string CMD_SET_SHORTCUT_MODE			= "dm_set_shortcut_mode";                               //设置快捷模式
const std::string CMD_GET_SHORTCUT_FILTER		= "dm_get_shortcut_filter";                             //获取快捷过滤器


//MDP
const std::string CMD_MDP_MSG = "mdp_msg";                                                              //路由消息
const std::string CMD_MDP_REGISTER = "mdp_register";                                                    //注册到mdp


//数据同步消息
const std::string CMD_SYN_ADD_MEMBER = "syn_add_member"; 
const std::string CMD_SYN_UPDATE_MEMBER = "syn_update_member"; 
const std::string CMD_SYN_DEL_MEMBER = "syn_del_member"; 
const std::string CMD_SYN_UPDATE_FAMILY = "syn_update_family"; 
const std::string CMD_SYN_CLIENT_STATUS = "syn_client_status"; 
const std::string CMD_SYN_APPLY_FAMILY = "syn_apply_family"; 
const std::string CMD_SYN_UPDARE_USER = "syn_update_user";	


//SMS
const std::string CMD_SMS = "sms";                                                                      //短信


//推送服务
const std::string CMD_PUSH_MSG = "push_msg";                                                            //推送消息


//管理消息
const std::string CMD_SYS_KICKOFF_CLIENT = "sys_kickoff_client"; 


#endif

