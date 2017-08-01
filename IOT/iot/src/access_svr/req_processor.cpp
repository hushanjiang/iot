
#include "req_processor.h"
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
#include "client_mgt.h"
#include "conn_mgt_lb.h"
#include "redis_mgt.h"

//geoip
#include "GeoIP.h"
#include "GeoIPCity.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Req_Mgt *g_req_mgt;
GeoIP *g_geoip = NULL;

extern Conn_Mgt_LB g_user_mgt_conn;
extern Conn_Mgt_LB g_device_mgt_conn;
extern Conn_Mgt_LB g_family_mgt_conn;


Req_Processor::Req_Processor()
{

}


Req_Processor::~Req_Processor()
{

}



int Req_Processor::do_init(void *args)
{	
	g_geoip = GeoIP_open("./../conf/GeoLiteCity.dat", GEOIP_INDEX_CACHE);
	if (g_geoip == NULL) 
	{
		XCP_LOGGER_INFO(&g_logger, "GeoIP_open ./../conf/GeoLiteCity.dat failed.\n");
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "GeoIP_open ./../conf/GeoLiteCity.dat success.\n");
	}

	return 0;
}




int Req_Processor::svc()
{
	int nRet = 0;
	std::string err_info = "";
	
	//获取req 并且处理
	Request_Ptr req;
	nRet = g_req_mgt->get_req(req);
	if(nRet != 0)
	{
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "prepare to process req from app, %s\n", req->to_string().c_str());

	//---------------
	
	//解析iot_req
	std::string uuid = "";
	std::string encry = "";
	std::string content = "";
	nRet = XProtocol::iot_req(req->_req, uuid, encry, content, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "iot_req faield, ret:%d err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
		return 0;
	}

	
	XCP_LOGGER_INFO(&g_logger, "iot_req, uuid:%s, encry:%s, content:%s\n", 
		uuid.c_str(), encry.c_str(), content.c_str());		

	//---------------

	std::string new_req = "";
	if(encry == "true" && !content.empty())
	{
		new_req = "";
		std::string buf = "";

		//base64解密
		X_BASE64 base64;
		char *tmp = NULL;
		if(!base64.decrypt(content.c_str(), tmp))
		{
			XCP_LOGGER_INFO(&g_logger, "base64 faield, content:%s\n", content.c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, "base64 faield");
			return 0;
		}
		buf = tmp;
		DELETE_POINTER_ARR(tmp);

		//获取安全通道key
		std::string key = "";
		nRet = PSGT_Redis_Mgt->get_security_channel(uuid, key, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "get_security_channel failed, ret:%d, err_info:%s, uuid:%s\n", 
				nRet, err_info.c_str(), uuid.c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
			return 0;
		}

		XCP_LOGGER_INFO(&g_logger, "get_security_channel success, uuid:%s, key:%s\n", uuid.c_str(), key.c_str());		

		//aes cbc 解密
		X_AES_CBC aes;
		if(!aes.decrypt(key, buf, new_req))
		{
			XCP_LOGGER_INFO(&g_logger, "aes cbc failed, ret:%d, key:%s\n", nRet, key.c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, "aes cbc failed");
			return 0;
		}
		
	}
	else
	{
		new_req = content;
	}
	

	//---------------

	XCP_LOGGER_INFO(&g_logger, "prepare to process new req(%u): %s\n", new_req.size(), new_req.c_str());
	req->_req = new_req;
	
	std::string method = "";
	unsigned long long timestamp = 0;
	unsigned int req_id = 0;
	nRet = XProtocol::req_head(req->_req, method, timestamp, req_id, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, ret:%d err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, "null", 0, req->_msg_tag, ERR_INVALID_REQ, err_info);
		return 0;
	}

	//更新req 信息
	req->_req_id = req_id;
	req->_app_stmp = timestamp;
	req->_req = XProtocol::add_msg_tag(req->_req, req->_msg_tag);

	if((method == CMD_REGISTER_USER) ||
		(method == CMD_LOGIN_PWD) || 
		(method == CMD_LOGIN_CODE) ||
		(method == CMD_PHONE_CODE)||
		(method == CMD_AUTH_ROUTER)||	
		(method == CMD_AUTH) ||
		(method == CMD_SET_PWD)||
		(method == CMD_RESET_PWD)||
		(method == CMD_GET_USER_PROFILE)||
		(method == CMD_UPDATE_USER_PROFILE)||
		(method == CMD_REFRESH_USER_TOKEN))  
	{
		//用户管理
		XCP_LOGGER_INFO(&g_logger, "--- user mgt:%s ---\n", method.c_str());

		//获取user mgt conn
		Conn_Ptr user_conn;
		if(!g_user_mgt_conn.get_conn(user_conn))
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "no user mgt conn.");
			return 0;
		}
		
		std::string rsp = "";
		nRet = user_conn->inner_message(req->_msg_tag, req->_req, rsp, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "get rsp from user mgt svr failed, ret:%d err_info:%s\n", nRet, err_info.c_str());
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, nRet, err_info);
			return 0;
			
		}

		//返回给前端
		Msg_Oper::send_msg(req->_fd, req->_session_id, rsp);

		//todo: 对登录和验证成功后需要注册和增加路由操作
		
	
	}
	else if((method == CMD_CREATE_FAMILY)||
	   (method == CMD_UPDATE_FAMILY)||
	   (method == CMD_GET_FAMILY_INFO)||
	   (method == CMD_GET_FAMILY_LIST)||
	   (method == CMD_APPLY_FAMILY)||
	   (method == CMD_ACCEPT_FAMILY)||
	   (method == CMD_GET_APPLY_LIST)||
	   (method == CMD_REMOVE_MEMBER)||
	   (method == CMD_UPDATE_MEMBER)||
	   (method == CMD_REMOVE_MEMBER)||
	   (method == CMD_GET_MEMBER_INFO)||
	   (method == CMD_GET_MEMBER_LIST))
	{
		//家庭管理
		XCP_LOGGER_INFO(&g_logger, "--- family mgt:%s ---\n", method.c_str());

		//获取familiy mgt conn
		Conn_Ptr family_conn;
		if(!g_family_mgt_conn.get_conn(family_conn))
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "no family mgt conn.");
			return 0;
		}
		
		std::string rsp = "";
		nRet = family_conn->inner_message(req->_msg_tag, req->_req, rsp, err_info);
		if(nRet == 0)
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, rsp);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, nRet, err_info);
		}
		
	}
	else if((method == CMD_ADD_ROOM )||
	   (method == CMD_DEL_ROOM)||
	   (method == CMD_UPDATE_ROOM)||
	   (method == CMD_GET_ROOM_LIST)||
	   (method == CMD_UPDATE_ROOM_ORDER)||
	   
	   (method == CMD_BIND_ROUTER)||
	   (method == CMD_UNBIND_ROUTER)||
	   (method == CMD_ROUTER_DEVICES)||
	   (method == CMD_AUTH_ROUTER)||
	   
	   (method == CMD_ADD_DEIVCE)||
	   (method == CMD_DEL_DEVICE)||
	   (method == CMD_UPDATE_DEVICE)||
	   (method == CMD_GET_DEVICE_INFO)||
	   (method == CMD_GET_DEVICE_LIST))
	{
		//家庭管理
		XCP_LOGGER_INFO(&g_logger, "--- device mgt:%s ---\n", method.c_str());

		//获取device mgt conn
		Conn_Ptr device_conn;
		if(!g_device_mgt_conn.get_conn(device_conn))
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_NO_CONN_FOUND, "no family mgt conn.");
			return 0;
		}
		
		std::string rsp = "";
		nRet = device_conn->inner_message(req->_msg_tag, req->_req, rsp, err_info);
		if(nRet == 0)
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, rsp);
		}
		else
		{
			Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, nRet, err_info);
		}
		
		
	}	
	else
	{
		XCP_LOGGER_INFO(&g_logger, "invalid method(%s) from app\n", method.c_str());
		Msg_Oper::send_msg(req->_fd, req->_session_id, method, req_id, req->_msg_tag, ERR_INVALID_REQ, "invalid method.");
	}
	
	return 0;
	
}




//获取用户地理位置
const char *_mk_NA(const char *p)
{
    return p ? p : "N/A";
}



std::string Req_Processor::get_geography(const std::string &ip)
{
	std::string geography = ip;

	if(!g_geoip)
	{
		return geography;
	}  

	//通过主机名称返回GeoIPRecord，由内部生成返回， 需要应用通过GeoIPRecord_delete主动释放
	GeoIPRecord *gir = NULL;
	gir = GeoIP_record_by_name(g_geoip, ip.c_str());
	if(gir) 
	{
		//通过公网IP获取Ip对应网络子码信息ret[0]为子码开始地址，ret[1]为子码结束地址
		char **ret = NULL;
		ret = GeoIP_range_by_ip(g_geoip, ip.c_str());
		
		//通过国家和地区号获取时区信息
		const char *time_zone = NULL;
		time_zone = GeoIP_time_zone_by_country_and_region(gir->country_code, gir->region);

		//地理信息大概100Byte
		char buf[1024] = {0};
		snprintf(buf, 1024, "host:%s;country_code:%s;region:%s;region_name:%s;city:%s", 
		ip.c_str(),
		_mk_NA(gir->country_code), //国家代号
	    _mk_NA(gir->region),   //行政地区代号
		_mk_NA(GeoIP_region_name_by_code(gir->country_code, gir->region)),	//获取行政地区名称，比如省名称
	    _mk_NA(gir->city)	//城市名称
	    );

		geography = buf;
			
	   	//释放内存
		GeoIP_range_by_ip_delete(ret);
		GeoIPRecord_delete(gir);
		
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "GeoIP_record_by_name failed, ip:%s\n", ip.c_str());
	}

	return geography;

}


