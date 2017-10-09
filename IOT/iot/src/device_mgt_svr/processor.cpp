
#include "processor.h"
#include "req_mgt.h"
#include "comm/common.h"
#include "protocol.h"
#include "base/base_net.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_url_parser.h"
#include "base/base_uid.h"
#include "base/base_utility.h"
#include "base/base_base64.h"
#include "base/base_cryptopp.h"

#include "msg_oper.h"
#include "conf_mgt.h"
#include "conn_mgt_lb.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Conn_Mgt_LB g_uid_conn;

Processor::Processor()
{
	_err_msg.insert(std::make_pair<int, std::string>(ERR_SUCCESS, "OK")); //队列满
	_err_msg.insert(std::make_pair<int, std::string>(ERR_SYSTEM, "system error")); //插入队列失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_QUEUE_FULL, "msg queue full")); //请求串格式非法
	_err_msg.insert(std::make_pair<int, std::string>(ERR_PUSH_QUEUE_FAIL, "put msg to queue failed")); //请求串大于最大长度
	_err_msg.insert(std::make_pair<int, std::string>(ERR_INVALID_REQ, "invalid request")); //请求串小于最小长度
	_err_msg.insert(std::make_pair<int, std::string>(ERR_REACH_MAX_MSG, "msg too large")); //发送失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_REACH_MIN_MSG, "msg too small")); //接收失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_SEND_FAIL, "send msg failed")); //会话超时
	_err_msg.insert(std::make_pair<int, std::string>(ERR_RCV_FAIL, "recieve msg failed")); //找不到conn
	_err_msg.insert(std::make_pair<int, std::string>(ERR_SESSION_TIMEOUT, "session timeout")); //获取mysql链接失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_NO_CONN_FOUND, "cannot get connection")); //MYSQL操作异常
	_err_msg.insert(std::make_pair<int, std::string>(ERR_GET_MYSQL_CONN_FAILED, "cannot conect to sql")); //重复键
	_err_msg.insert(std::make_pair<int, std::string>(ERR_MYSQL_OPER_EXCEPTION, "sql abnormal"));	//数据表存在
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DUPLICATE_KEY, "cannot insert dupicate idex"));	//达到每秒最多消息个数
	_err_msg.insert(std::make_pair<int, std::string>(ERR_TABLE_EXISTS, "table exist")); //没有找到对应的worker
	_err_msg.insert(std::make_pair<int, std::string>(ERR_REACH_MAX_MSG_CNT, "too many msg,canot process")); //workerid已经存在
	_err_msg.insert(std::make_pair<int, std::string>(ERR_NO_WORKER_FOUND, "user id is not exist")); //workerfd已经存在
	_err_msg.insert(std::make_pair<int, std::string>(ERR_WORKER_ID_EXIST, "user id have exist")); //BASE64编码失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_WORKER_FD_EXIST, "user fd have exist")); //BASE64解码失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_BASE64_ENCODE_FAILED, "base64 encode failed")); //hmac-sha1编码失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_BASE64_DECODE_FAILED, "base64 decode failed")); //redis操作异常
	_err_msg.insert(std::make_pair<int, std::string>(ERR_HMAC_ENCODE_FAILED, "hmac encode failed")); //路由器的uuid不存在
	_err_msg.insert(std::make_pair<int, std::string>(ERR_REDIS_OPER_EXCEPTION, "redis abnormal")); //路由器的随机码不正确
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ROUTER_UUID_NOT_EXIST, 	"router uuid is not exists")); //路由器绑定的家庭id不存在
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ROUTER_PWD_NOT_CORRECT, "router pwd is not correct")); //操作设备权限不足，不是户主
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DEVICE_FAMILY_ID_IS_NOT_EXIST, "family id is not exists")); //解绑操作，家庭id和路由器id不对应
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DEVICE_USER_IS_NOT_OWNER, "permission denied,only family owner can process")); //路由器查询账户信息，然而用户不属于绑定这个路由器的家庭
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ROUTER_UNBIND_FAMILY_ID_NOT_CORRECT, "router donot bind this family,cannot unbind")); //路由器查询账户信息，查数据库失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ROUTER_CANNOT_GET_USERS_ACCOUNT, "permission denied,user donnot join the bind family")); //通过路由器查找绑定的家庭失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ROUTER_GET_USERS_ACCOUNT_FAILED, "get user account info from sql failed")); //设备的id不存在
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ROUTER_GET_BIND_FAMILY_FAILED, "cannot find bind family")); //路由器的id不存在
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DEVICE_ID_NOT_EXIST, "device id not exists")); //一次添加太多房间，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ROUTER_ID_NOT_EXIST, "router id not exists")); //批量插入房间失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ADD_TOO_MANY_ROOMS, "add too many rooms,max 10")); //一次删除太多房间，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ADD_ROOMS_FAILED, "add rooms failed")); //批量删除房间失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DELETE_TOO_MANY_ROOMS, "delete too many rooms,max 10")); //一次修改太多房间，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DELETE_ROOMS_FAILED, "delete rooms failed")); //批量修改房间失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_UPDATE_TOO_MANY_ROOMS, "update too many rooms,max 10")); //一次添加太多设备，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_UPDATE_ROOMS_FAILED, "update rooms failed")); //批量插入设备失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ADD_TOO_MANY_DEVICES, "add too many device,max10")); //一次删除太多设备，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ADD_DEVICES_FAILED, "add devices failed")); //批量删除设备失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DELETE_TOO_MANY_DEVICES, "delete too many devices,max 10")); //一次修改太多设备，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DELETE_DEVICES_FAILED, "delete devices failed")); //批量修改设备失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_UPDATE_TOO_MANY_DEVICES, "update too many devices,max 10")); //一次添加太多快捷方式，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_UPDATE_DEVICES_FAILED, "update devices failed")); //批量插入快捷方式失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ADD_TOO_MANY_SHORTCUTS, "add too many shortcut,max 10")); //一次删除太多快捷方式，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_ADD_SHORTCUTS_FAILED, "add shortcut failed")); //批量删除快捷方式失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DELETE_TOO_MANY_SHORTCUTS, "delete too many shortcut,max 10"));		//一次修改太多快捷方式，最多10个
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DELETE_SHORTCUTS_FAILED, "delete shortcut failed")); //批量修改快捷方式失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_UPDATE_TOO_MANY_SHORTCUTS, "update too many shortcut,max 10")); //批量修改快捷方式失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_UPDATE_SHORTCUTS_FAILED, "update shortcut failed")); //批量修改快捷方式失败
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DEVICE_ROUTER_HAVE_BIND_FAMILY, "router have been bind other family")); // 路由器已经被别的家庭绑定
	_err_msg.insert(std::make_pair<int, std::string>(ERR_DEVICE_FAMILY_HAVA_BIND_ROUTER, "family have bind other router")); // 家庭已经绑定了其他的路由器
}


Processor::~Processor()
{

}



int Processor::do_init(void *args)
{
	int nRet = 0;

	return nRet;
}


int Processor::_get_uuid(const std::string &msg_tag, std::string &err_info, unsigned long long &uuid)
{
	int nRet = 0;
	err_info = "";
	
	uuid = 0;

	//获取uuid
	Conn_Ptr uid_conn;
	if(!g_uid_conn.get_conn(uid_conn))
	{
		XCP_LOGGER_INFO(&g_logger, "get uid conn failed.\n");
		return -1;
	}
	
	nRet = uid_conn->get_uuid(msg_tag, uuid, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "get uid failed, ret:%d, err_info:%s\n", nRet, err_info.c_str());
		return -1;
	}

	XCP_LOGGER_INFO(&g_logger, "get uid success, uuid:%llu\n", uuid);

	return nRet;
}

std::string Processor::_get_errInfo(const int error_code)
{
	std::map<int, std::string>::iterator it = _err_msg.find(error_code);
	if(it != _err_msg.end())
	{
		return it->second;
	}
	return "Unkown reason";
}


int Processor::svc()
{
	int nRet = 0;
	std::string err_info = "OK";
	
	//获取req 并且处理
	Request_Ptr req;
	nRet = PSGT_Req_Mgt->get_req(req);
	if(nRet != 0)
	{
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "prepare to process req from access svr, access svr:%s\n", req->to_string().c_str());

	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned long long realId = 0;
	unsigned int req_id = 0;
	std::string msg_tag = "";
	std::string msg_encry = "", msg_uuid = "", session_id = "";
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, realId, msg_tag, msg_encry, msg_uuid, session_id, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, "null", 0, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_app_stmp = timestamp;
	req->_msg_tag = msg_tag;

	if(method == CMD_ADD_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- add room ---\n");

		std::vector<StRoomInfo> infos;
		uint64 familyId = 0;
		if( XProtocol::get_room_infos(req->_req, infos, err_info) != 0
		|| XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
		}
		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->add_room(familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_DEL_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- del room ---\n");
		uint64 familyId = 0;
		std::vector<uint64> roomIds;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_id_lists(req->_req, roomIds, err_info) != 0 )
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}
		
		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->delete_room(familyId, roomIds, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_UPDATE_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update room ---\n");
		uint64 familyId = 0;
		std::vector<StRoomInfo> infos;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_room_update_infos(req->_req, infos, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
		 }
		 
		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->update_room(familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_UPDATE_ROOM_NEW)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update room new ---\n");
		uint64 familyId = 0;
		std::vector<StRoomInfo> infos;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_room_infos(req->_req, infos, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
		 }
		 
		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->update_room_new(familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_GET_ROOM_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get room list ---\n");
		uint64 familyId = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}

		std::vector<StRoomInfo> infos;
		nRet = PSGT_Device_Mgt->get_room_list(familyId, infos);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_room_list_result(infos);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}
	else if(method == CMD_UPDATE_ROOM_ORDER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update room order ---\n");
		
		uint64 userId = 0, familyId = 0;
		std::vector<StRoomInfo> infos;
		if( XProtocol::get_special_params(req->_req, "user_id", userId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_room_orders_params(req->_req, infos, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}	
		nRet = PSGT_Device_Mgt->update_room_order(userId, familyId, infos);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		return 0;
	}
	
	else if(method == CMD_BIND_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- bind router ---\n");
		
		uint64 userId = 0, familyId = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->router_bind(userId, familyId, realId);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		return 0;
	}
	else if(method == CMD_UNBIND_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- unbind router ---\n");
		
		uint64 userId = 0, familyId = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->router_unbind(userId, familyId, realId);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		return 0;
	}	
	else if(method == CMD_AUTH_ROUTER || method == CMD_CHECK_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- router auth ---\n");
		
		std::string uuid, pwd;
		if( XProtocol::get_special_params(req->_req, "uuid", uuid, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "pwd", pwd, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->router_auth(uuid, pwd, realId);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_router_auth_result(realId);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}		
	else if(method == CMD_GET_ROUTER_INFO)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get router info ---\n");

		StRouterInfo info;
		nRet = PSGT_Device_Mgt->router_get_info(realId, info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_router_info_result(info);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}
	else if(method == CMD_DM_GET_USER_ACCOUNT)
	{
		XCP_LOGGER_INFO(&g_logger, "--- router get user account ---\n");

		std::vector<std::string> tokens;
		std::vector<uint64> userIds;
		if( XProtocol::get_id_lists(req->_req, userIds, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}

		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->router_get_user_account(realId, userIds, tokens, failed);
		std::string body = XProtocol::get_user_account_result(tokens,failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_ADD_DEIVCE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- add device ---\n");
		
		std::vector<StDeviceInfo> infos;
		uint64 familyId = 0;
		std::vector<std::string> failed;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_device_infos(req->_req, infos, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		nRet = PSGT_Device_Mgt->add_device(realId, familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_DEL_DEVICE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- del device ---\n");
		
		uint64 familyId = 0;
		std::vector<uint64> deviceIds, failed;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0 
		|| XProtocol::get_id_lists(req->_req, deviceIds, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->delete_device(realId, familyId, deviceIds, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_UPDATE_DEVICE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update device ---\n");
		
		std::vector<StDeviceInfo> infos;
		uint64 familyId = 0;
		std::vector<std::string> failed;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_device_update_infos(req->_req, infos, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		nRet = PSGT_Device_Mgt->update_device(realId, familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_UPDATE_DEVICE_NEW)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update device new ---\n");
		
		std::vector<StDeviceInfo> infos;
		uint64 familyId = 0;
		std::vector<std::string> failed;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_device_infos(req->_req, infos, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		nRet = PSGT_Device_Mgt->update_device_new(realId, familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_GET_DEVICE_INFO)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device info ---\n");
		
		StDeviceInfo info;
		uint64 userId = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", info.familyId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", info.id, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "router_id", info.routerId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		//info.routerId = realId;
		
		nRet = PSGT_Device_Mgt->get_device_info(userId, info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_device_info_result(info);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}
	else if(method == CMD_GET_DEVICES_BY_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device list by room ---\n");
		
		uint64 userId = 0, familyId = 0, roomId = 0, routerId = 0;
		unsigned int categoryId = DM_DEVICE_CATEGORY_END, size = 0, beginAt = 0, left = 0;
		if( XProtocol::get_special_params(req->_req, "room_id", roomId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "router_id", routerId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", userId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "device_category_id", categoryId, err_info) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		enum DM_DEVICE_CATEGORY category = (enum DM_DEVICE_CATEGORY)(categoryId);
		if(categoryId == 0)
		{
			category = DM_DEVICE_CATEGORY_END;
		}

		std::vector<StDeviceInfo> infos;	
		nRet = PSGT_Device_Mgt->get_devices_by_room(userId, familyId, roomId, routerId, category, infos, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_device_list_result(infos, left);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}
	else if(method == CMD_GET_DEVICES_BY_FAMILY)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device list by family ---\n");
		
		uint64 userId = 0, familyId = 0, routerId = 0;
		unsigned int categoryId = DM_DEVICE_CATEGORY_END, size = 0, beginAt = 0, left = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", userId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "router_id", routerId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "device_category_id", categoryId, err_info) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		enum DM_DEVICE_CATEGORY category = (enum DM_DEVICE_CATEGORY)(categoryId % DM_DEVICE_CATEGORY_END);
		if(categoryId == 0)
		{
			category = DM_DEVICE_CATEGORY_END;
		}

		std::vector<StDeviceInfo> infos;	
		nRet = PSGT_Device_Mgt->get_devices_by_family(userId, familyId, routerId, category, infos, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_device_list_result(infos, left);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}	
	else if(method == CMD_REPORT_DEV_STATUS)
	{
		XCP_LOGGER_INFO(&g_logger, "--- report device status ---\n");
		
		uint64 familyId = 0;
		std::vector<StDeviceStatus> statues;
		unsigned int type = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_device_status(req->_req, statues, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		nRet = PSGT_Device_Mgt->report_status(familyId, statues);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		return 0;
	}
	else if(method == CMD_REPORT_DEV_ALERT)
	{
		XCP_LOGGER_INFO(&g_logger, "--- report device status ---\n");
		
		uint64 familyId = 0, alertId = 0;
		StDeviceAlert alert;
		unsigned int type = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", alert.familyId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", alert.deviceId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "report_type", type, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "report_msg", alert.msg, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "created_at", alert.createAt, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		alert.type = (enum DM_DEVICE_ALERT_TYPE)(type % DM_DEVICE_ALERT_TYPE_END);

		nRet = PSGT_Device_Mgt->report_alert(alert, alertId);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		return 0;
	}
	else if(method == CMD_GET_DEV_STATUS_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device status list ---\n");
		
		uint64 familyId = 0, deviceId = 0;
		unsigned int size = 0, beginAt = 0, left = 0;
		std::string date;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", deviceId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "date", date, err_info) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		std::deque<std::string> status;
		timestamp = getTimestamp();
		nRet = PSGT_Device_Mgt->get_dev_status_list(familyId, deviceId, date, status, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_status_list_result(status, left);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}
	else if(method == CMD_GET_DEV_ALERT_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device alert list ---\n");

		uint64 familyId = 0, userId = 0, deviceId = 0;
		unsigned int size = 0, beginAt = 0, left = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", deviceId, err_info) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		std::vector<StDeviceAlert> alerts;	
		nRet = PSGT_Device_Mgt->get_dev_alert_list(userId, deviceId, alerts, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_alert_list_result(alerts, left);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}
	else if(method == CMD_ADD_SHORTCUT)
	{
		XCP_LOGGER_INFO(&g_logger, "--- add shortcut ---\n");

		uint64 familyId;
		std::vector<StShortCutInfo> infos;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_shortcut_infos(req->_req, infos, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
		 }
		 
		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->add_shortcut(familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_DEL_SHORTCUT)
	{
		XCP_LOGGER_INFO(&g_logger, "--- del shortcut ---\n");

		uint64 familyId = 0;
		std::vector<uint64> shortcutIds, failed;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0 
		|| XProtocol::get_id_lists(req->_req, shortcutIds, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->remove_shortcut(familyId, shortcutIds, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_UPDATE_SHORTCUT)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update shortcut ---\n");

		uint64 familyId;
		std::vector<StShortCutInfo> infos;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_shortcut_update_infos(req->_req, infos, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
		 }
		 
		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->update_shortcut(familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_UPDATE_SHORTCUT_NEW)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update shortcut new ---\n");

		uint64 familyId;
		std::vector<StShortCutInfo> infos;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_shortcut_infos(req->_req, infos, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
		 }
		 
		std::vector<uint64> failed;
		nRet = PSGT_Device_Mgt->update_shortcut_new(familyId, infos, failed);
		std::string body = XProtocol::process_failed_result(failed);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		return 0;
	}
	else if(method == CMD_GET_SHORTCUT_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get shortcut list ---\n");
		
		uint64 familyId = 0, roomId = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
			|| XProtocol::get_special_params(req->_req, "room_id", roomId, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}

		std::vector<StShortCutInfo> infos;
		nRet = PSGT_Device_Mgt->get_list_shortcut(realId, familyId, roomId, infos);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_shortcut_list_result(infos);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet), body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		}
		return 0;
	}
	else if(method == CMD_UPDATE_SHORTCUT_ORDER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update shortcut order ---\n");
		
		uint64 familyId = 0;
		std::vector<StShortCutInfo> infos;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_shortcut_orders_params(req->_req, infos, err_info) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
	 		return 0;
	 	}	
		nRet = PSGT_Device_Mgt->update_shortcut_order(familyId, infos);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, _get_errInfo(nRet));
		return 0;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from access svr\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;

}


