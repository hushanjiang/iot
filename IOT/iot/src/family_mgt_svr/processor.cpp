
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
		err_info = "get uid conn failed";
		return -1;
	}
	
	nRet = uid_conn->get_uuid(msg_tag, uuid, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "get uid failed, ret:%d, err_info:%s\n", nRet, err_info.c_str());
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "get uid success, uuid:%llu\n", uuid);

	return nRet;
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

	if(method == CMD_CREATE_FAMILY)
	{
		XCP_LOGGER_INFO(&g_logger, "--- create family ---\n");

		uint64 uuid;
		if(_get_uuid(req->_msg_tag, err_info, uuid) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_SYSTEM, err_info);
			return 0;
		}
		
		std::string familyName;
		if( XProtocol::get_special_params(req->_req, "family_name", familyName, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
			return 0;
		}
		
		StFamilyInfo info;
		info.familyId = uuid;
		info.familyName = familyName;
		info.createdAt = timestamp;
		info.updateAt = timestamp;
		info.ownerId = realId;

		std::string token;
		nRet = PSGT_Family_Mgt->create_family(realId, msg_tag, info, token, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::add_family_result(uuid, token);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_UPDATE_FAMILY)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update family ---\n");

		uint64 familyId = 0;
		if( XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0 )
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		StFamilyInfo info;
		XProtocol::get_special_params(req->_req, "family_name", info.familyName, err_info);
		XProtocol::get_special_params(req->_req, "family_avatar", info.familyAvatar, err_info);		
		info.familyId = familyId;
		info.updateAt = timestamp;
		
		nRet = PSGT_Family_Mgt->update_family(realId, msg_tag, info, err_info);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		return 0;
	}
	else if(method == CMD_FM_BIND_ROUTERP)
	{
		XCP_LOGGER_INFO(&g_logger, "--- bind router ---\n");
		
		uint64 familyId = 0, routerId = 0;
		std::string uuid, pwd;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
			|| XProtocol::get_special_params(req->_req, "router_uuid", uuid, err_info) != 0
			|| XProtocol::get_special_params(req->_req, "router_pwd", pwd, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		nRet = PSGT_Family_Mgt->bind_router(realId, msg_tag, familyId, routerId, uuid, pwd, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::bind_router_result(routerId);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;		
	}
	else if(method == CMD_FM_UNBIND_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- unbind router ---\n");
		
		uint64 familyId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		nRet = PSGT_Family_Mgt->unbind_router(realId, msg_tag, familyId, err_info);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		return 0;		
	}
	else if(method == CMD_GET_APPLY_CODE)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get apply code ---\n");
		
		uint64 familyId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		std::string code;
		uint64 create_time;
		nRet = PSGT_Family_Mgt->get_apply_code(realId, msg_tag, familyId, code, create_time, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_apply_code_result(code, create_time, create_time + 180);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;		
	}
	else if(method == CMD_CREATE_MEMBER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- create member ---\n");
		
		uint64 familyId = 0;
		std::string code;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
		 || XProtocol::get_special_params(req->_req, "code", code, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		std::string token;
		nRet = PSGT_Family_Mgt->create_member(realId, msg_tag, familyId, code, token, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_creat_member_result(token);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;		
	}
	else if(method == CMD_GET_INVITATION)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get invitation ---\n");
		
		uint64 familyId = 0;
		std::string code;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		std::string token;
		nRet = PSGT_Family_Mgt->get_invitation(realId, msg_tag, familyId, token, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_creat_member_result(token);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;		
	}
	else if(method == CMD_SWITCH_FAMILY)
	{
		XCP_LOGGER_INFO(&g_logger, "--- switch family ---\n");
		
		uint64 familyId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		nRet = PSGT_Family_Mgt->member_switch_family(realId, msg_tag, familyId, err_info);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		return 0;
	}
	
	else if(method == CMD_GET_FAMILY_INFO)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get family info ---\n");
		
		uint64 familyId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		StFamilyInfo info;
		nRet = PSGT_Family_Mgt->get_family_info(realId, msg_tag, familyId, info, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_family_info_result(info);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		
		return 0;		
	}	

	else if(method == CMD_GET_FAMILY_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get family list ---\n");
				
		std::vector<StFamilyInfo> infoList;
		nRet = PSGT_Family_Mgt->get_family_list(realId, msg_tag, infoList, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_family_list_result(infoList);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;
	}
	else if(method == CMD_REMOVE_MEMBER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- remove member ---\n");
		
		uint64 familyId = 0, memberId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
			|| XProtocol::get_special_params(req->_req, "target_user_id", memberId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		nRet = PSGT_Family_Mgt->remove_member(realId, msg_tag, familyId, memberId, err_info);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		return 0;	
	}
	else if(method == CMD_UPDATE_MEMBER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- update member ---\n");
		
		uint64 familyId = 0, memberId = 0;
		std::string nickName;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
			|| XProtocol::get_special_params(req->_req, "target_user_id", memberId, err_info) != 0
			|| XProtocol::get_special_params(req->_req, "user_name", nickName, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}
		
		nRet = PSGT_Family_Mgt->update_member(realId, msg_tag, familyId, memberId, nickName, err_info);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		return 0;
	}
	else if(method == CMD_GET_MEMBER_INFO)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get member info ---\n");
		
		uint64 familyId = 0, memberId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0
			|| XProtocol::get_special_params(req->_req, "target_user_id", memberId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		StFamilyMemberInfo info;
		nRet = PSGT_Family_Mgt->get_member_info(realId, msg_tag, familyId, memberId, info, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_member_info_result(info);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		
		return 0;	
	}
	else if(method == CMD_GET_MEMBER_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get member list ---\n");
		
		uint64 familyId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		std::vector<StFamilyMemberInfo> infoList;
		nRet = PSGT_Family_Mgt->get_member_info_list(realId, msg_tag, familyId, infoList, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_member_list_result(infoList);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;	
	}	
	else if(method == CMD_GET_MEMBER_ID_LIST)
	{
		XCP_LOGGER_INFO(&g_logger, "--- get member id list ---\n");
		
		uint64 familyId = 0;
		if(XProtocol::get_special_params(req->_req, "family_id", familyId, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		std::vector<uint64> ids;
		uint64 routerId = 0;
		nRet = PSGT_Family_Mgt->get_member_id_list(realId, msg_tag, familyId, ids, routerId, err_info);
		if(nRet == 0)
		{
			std::string body = XProtocol::get_member_id_list_result(ids, routerId);
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info, body, true);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		}
		return 0;	
	}	
	else if(method == CMD_CHECK_TALK_CONDITION)
	{
		XCP_LOGGER_INFO(&g_logger, "--- check talk condition ---\n");
		
		uint64 targetId = 0;
		std::string msgType;
		if(XProtocol::get_special_params(req->_req, "target_id", targetId, err_info) != 0
		  || XProtocol::get_special_params(req->_req, "msg_type", msgType, err_info) != 0)
		{
			Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, err_info);
			return 0;
		}

		enum talk_msg_type type = TALK_MSG_TYPE_END;
		if(msgType == MT_P2P)
		{
			type = TALK_MSG_TYPE_P2P;
		}
		else if(msgType == MT_P2R)
		{
			type = TALK_MSG_TYPE_P2R;
		}
		else if(msgType == MT_P2F)
		{
			type = TALK_MSG_TYPE_P2F;
		}
		else if(msgType == MT_R2P)
		{
			type = TALK_MSG_TYPE_R2P;
		}
		else if(msgType == MT_R2F)
		{
			type = TALK_MSG_TYPE_R2F;
		}

		nRet = PSGT_Family_Mgt->check_talk_condition(realId, targetId, msg_tag, type, err_info);
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, nRet, err_info);
		return 0;	
	}	

	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from access svr\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, method, req_id, req->_msg_tag, msg_encry, msg_uuid, session_id, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;

}


