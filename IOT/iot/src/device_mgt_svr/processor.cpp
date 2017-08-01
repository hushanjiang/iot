
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


int Processor::svc()
{
	int nRet = 0;
	std::string err_info = "";
	
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
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, realId, msg_tag, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, "null", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_app_stmp = timestamp;
	req->_msg_tag = msg_tag;

	if(method == CMD_ADD_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- add room ---\n");

		StRoomInfo info;
		if( XProtocol::get_special_params(req->_req, "family_id", info.familyId) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", info.creatorId) != 0
		 || XProtocol::get_special_params(req->_req, "is_default", info.isDefault) != 0
		 || XProtocol::get_special_params(req->_req, "order", info.orderNum) != 0
		 || XProtocol::get_special_params(req->_req, "room_name", info.name) != 0
		 || XProtocol::get_special_params(req->_req, "is_default", info.isDefault) != 0 )
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}
		info.createAt = timestamp;
		info.updateAt = timestamp;
		
		uint64 roomId = 0;
		nRet = PSGT_Device_Mgt->add_room(info, roomId);
		if(nRet == 0)
		{
			std::string body = XProtocol::add_room_result(roomId);
			err_info = "add room success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "add room failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_DEL_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- del room ---\n");

		uint64 userId = 0, familyId = 0, roomId = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId) != 0
		 || XProtocol::get_special_params(req->_req, "room_id", roomId) != 0 )
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->delete_room(userId, roomId, familyId);
		if(nRet == 0)
		{
			err_info = "delete room success!";
		}
		else
		{
			err_info = "delete room failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}	

	else if(method == CMD_UPDATE_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update room ---\n");

		uint64 userId = 0;
		StRoomInfo info;
		if( XProtocol::get_special_params(req->_req, "family_id", info.familyId) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "room_id", info.id) != 0
		 || XProtocol::get_special_params(req->_req, "room_name", info.name) != 0 )
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}
		info.updateAt = timestamp;
		
		nRet = PSGT_Device_Mgt->update_room(userId, info);
		if(nRet == 0)
		{
			err_info = "update room success!";
		}
		else
		{
			err_info = "update room failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}
	else if(method == CMD_GET_ROOM_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get room list ---\n");
		
		uint64 familyId = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}

		std::vector<StRoomInfo> infos;
		nRet = PSGT_Device_Mgt->get_room_list(familyId, infos);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_room_list_result(infos);
			err_info = "get room success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "get room failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_UPDATE_ROOM_ORDER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update room order ---\n");
		
		uint64 userId = 0, familyId = 0;
		std::vector<StRoomInfo> infos;
		if( XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId) != 0
		 || XProtocol::get_room_orders_params(req->_req, infos) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}	
		nRet = PSGT_Device_Mgt->update_room_order(userId, familyId, infos);
		if(nRet == 0)
		{
			err_info = "update room success!";
		}
		else
		{
			err_info = "update room failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}
	
	else if(method == CMD_BIND_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- bind router ---\n");
		
		uint64 userId = 0, familyId = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->router_bind(userId, familyId, realId);
		if(nRet == 0)
		{
			err_info = "bind router success!";
		}
		else
		{
			err_info = "bind router failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}
	else if(method == CMD_UNBIND_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- unbind router ---\n");
		
		uint64 userId = 0, familyId = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->router_unbind(userId, familyId, realId);
		if(nRet == 0)
		{
			err_info = "unbind router success!";
		}
		else
		{
			err_info = "unbind router failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}	
	else if(method == CMD_AUTH_ROUTER || method == CMD_CHECK_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- router auth ---\n");
		
		std::string uuid, pwd;
		if( XProtocol::get_special_params(req->_req, "uuid", uuid) != 0
		 || XProtocol::get_special_params(req->_req, "pwd", pwd) != 0)
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}
		
		nRet = PSGT_Device_Mgt->router_auth(uuid, pwd, realId);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_router_auth_result(realId);
			err_info = "router auth success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "router auth failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}		
	else if(method == CMD_GET_ROUTER_INFO)
	{
		XCP_LOGGER_INFO(&g_logger, "--- router auth ---\n");

		StRouterInfo info;
		nRet = PSGT_Device_Mgt->router_get_info(realId, info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_router_info_result(info);
			err_info = "add device success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "add device failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	
	else if(method == CMD_ADD_DEIVCE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- add device ---\n");

		unsigned long long uuid;
		if(_get_uuid(req->_msg_tag, err_info, uuid) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, "get uid conn failed.");
			return 0;
		}
		
		StDeviceInfo info;
		unsigned int categoryId = DM_DEVICE_CATEGORY_END;
		if( XProtocol::get_special_params(req->_req, "device_uuid", info.deviceId) != 0
		 || XProtocol::get_special_params(req->_req, "bussiness_user_id", info.businessId) != 0
		 || XProtocol::get_special_params(req->_req, "category_id", categoryId) != 0
		 || XProtocol::get_special_params(req->_req, "device_name", info.name) != 0
		 || XProtocol::get_special_params(req->_req, "device_attr_ext", info.attr) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", info.familyId) != 0
		 || XProtocol::get_special_params(req->_req, "room_id", info.roomId) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", info.creatorId) != 0  )
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		info.productId = (enum DM_DEVICE_CATEGORY)(categoryId % DM_DEVICE_CATEGORY_END);
		info.id = uuid;
		info.routerId = realId;
		info.createAt = timestamp;
		info.updateAt = timestamp;
		
		nRet = PSGT_Device_Mgt->add_device(info);
		if(nRet == 0)
		{
			std::string body = XProtocol::add_device_result(uuid);
			err_info = "add device success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "add device failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_DEL_DEVICE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- del device ---\n");
		
		uint64 userId = 0;
		StDeviceInfo info;
		if( XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", info.id) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", info.familyId) != 0 )
	 	{
	 		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
	 		return 0;
	 	}
		info.routerId = realId;
		
		nRet = PSGT_Device_Mgt->delete_device(userId, info);
		if(nRet == 0)
		{
			err_info = "delete device success!";
		}
		else
		{
			err_info = "delete device failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}
	else if(method == CMD_UPDATE_DEVICE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update device ---\n");
		
		StDeviceInfo info;
		unsigned int categoryId = DM_DEVICE_CATEGORY_END;
		uint64 userId = 0;
		if( XProtocol::get_special_params(req->_req, "device_id", info.id) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", info.familyId) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", userId) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		// 非必带参数
		XProtocol::get_special_params(req->_req, "room_id", info.roomId);
		XProtocol::get_special_params(req->_req, "device_uuid", info.deviceId);
		XProtocol::get_special_params(req->_req, "bussiness_user_id", info.businessId);
		XProtocol::get_special_params(req->_req, "device_name", info.name);
		XProtocol::get_special_params(req->_req, "category_id", categoryId);
		XProtocol::get_special_params(req->_req, "device_attr_ext", info.attr);
		
		info.productId = (enum DM_DEVICE_CATEGORY)(categoryId % DM_DEVICE_CATEGORY_END);
		info.updateAt = timestamp;
		
		nRet = PSGT_Device_Mgt->update_device(userId, info);
		if(nRet == 0)
		{
			err_info = "update device success!";
		}
		else
		{
			err_info = "update device failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}
	else if(method == CMD_GET_DEVICE_INFO)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device info ---\n");
		
		StDeviceInfo info;
		uint64 userId = 0;
		if( XProtocol::get_special_params(req->_req, "device_id", info.id) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", info.familyId) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", userId) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		info.routerId = realId;
		
		nRet = PSGT_Device_Mgt->get_device_info(userId, info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_device_info_result(info);
			err_info = "query device success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "query device failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_GET_DEVICES_BY_ROOM)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device list by room ---\n");
		
		uint64 userId = 0, familyId = 0, roomId = 0;
		unsigned int categoryId = DM_DEVICE_CATEGORY_END, size = 0, beginAt = 0, left = 0;
		if( XProtocol::get_special_params(req->_req, "room_id", roomId) != 0
		 || XProtocol::get_special_params(req->_req, "family_id", familyId) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "device_category_id", categoryId) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		enum DM_DEVICE_CATEGORY category = (enum DM_DEVICE_CATEGORY)(categoryId % DM_DEVICE_CATEGORY_END);
		if(categoryId == 0)
		{
			category = DM_DEVICE_CATEGORY_END;
		}

		std::vector<StDeviceInfo> infos;	
		nRet = PSGT_Device_Mgt->get_devices_by_room(userId, familyId, roomId, realId, category, infos, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_device_list_result(infos, left);
			err_info = "query device list by room success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "query device list by room failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_GET_DEVICES_BY_FAMILY)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device list by family ---\n");
		
		uint64 userId = 0, familyId = 0;
		unsigned int categoryId = DM_DEVICE_CATEGORY_END, size = 0, beginAt = 0, left = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId) != 0
		 || XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "device_category_id", categoryId) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		enum DM_DEVICE_CATEGORY category = (enum DM_DEVICE_CATEGORY)(categoryId % DM_DEVICE_CATEGORY_END);
		if(categoryId == 0)
		{
			category = DM_DEVICE_CATEGORY_END;
		}

		std::vector<StDeviceInfo> infos;	
		nRet = PSGT_Device_Mgt->get_devices_by_family(userId, familyId, realId, category, infos, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_device_list_result(infos, left);
			err_info = "query device list by room success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "query device list by room failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}	
	else if(method == CMD_REPORT_DEV_STATUS)
	{
		XCP_LOGGER_INFO(&g_logger, "--- report device status ---\n");
		
		uint64 familyId = 0, statusId = 0;
		StDeviceStatus status;
		unsigned int type = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", status.familyId) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", status.deviceId) != 0
		 || XProtocol::get_special_params(req->_req, "report_type", type) != 0
		 || XProtocol::get_special_params(req->_req, "report_msg", status.msg) != 0
		 || XProtocol::get_special_params(req->_req, "created_at", status.createAt) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		status.type = (enum DM_DEVICE_STATUS_TYPE)(type % DM_DEVICE_STATUS_TYPE_END);

		nRet = PSGT_Device_Mgt->report_status(status, statusId);
		if(nRet == 0)
		{
			err_info = "report device status success!";
		}
		else
		{
			err_info = "report device status failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}
	else if(method == CMD_REPORT_DEV_ALERT)
	{
		XCP_LOGGER_INFO(&g_logger, "--- report device status ---\n");
		
		uint64 familyId = 0, alertId = 0;
		StDeviceAlert alert;
		unsigned int type = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", alert.familyId) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", alert.deviceId) != 0
		 || XProtocol::get_special_params(req->_req, "report_type", type) != 0
		 || XProtocol::get_special_params(req->_req, "report_msg", alert.msg) != 0
		 || XProtocol::get_special_params(req->_req, "created_at", alert.createAt) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		alert.type = (enum DM_DEVICE_ALERT_TYPE)(type % DM_DEVICE_ALERT_TYPE_END);

		nRet = PSGT_Device_Mgt->report_alert(alert, alertId);
		if(nRet == 0)
		{
			err_info = "report device alert success!";
		}
		else
		{
			err_info = "report device alert failed!";
		}
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		return 0;
	}
	else if(method == CMD_GET_DEV_STATUS_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device status list ---\n");
		
		uint64 familyId = 0, userId = 0, deviceId = 0;
		unsigned int size = 0, beginAt = 0, left = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", deviceId) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		
		std::vector<StDeviceStatus> status;	
		nRet = PSGT_Device_Mgt->get_dev_status_list(userId, deviceId, status, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_status_list_result(status, left);
			err_info = "query device list by room success!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "query device list by room failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_GET_DEV_ALERT_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get device alert list ---\n");

		uint64 familyId = 0, userId = 0, deviceId = 0;
		unsigned int size = 0, beginAt = 0, left = 0;
		if( XProtocol::get_special_params(req->_req, "user_id", userId) != 0
		 || XProtocol::get_special_params(req->_req, "device_id", deviceId) != 0
		 || XProtocol::get_page_params(req->_req, size, beginAt) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, -1, "invaild request!");
			return 0;
		}
		
		std::vector<StDeviceAlert> alerts;	
		nRet = PSGT_Device_Mgt->get_dev_alert_list(userId, deviceId, alerts, beginAt, size, left);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_alert_list_result(alerts, left);
			err_info = "query device alert list uccess!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info, body, true);
		}
		else
		{
			err_info = "query device list by room failed!";
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, nRet, err_info);
		}
		return 0;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from access svr\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;

}


