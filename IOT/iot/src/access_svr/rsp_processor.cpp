
#include "rsp_processor.h"
#include "comm/common.h"
#include "protocol.h"
#include "base/base_net.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_uid.h"
#include "base/base_utility.h"

#include "msg_oper.h"
#include "conf_mgt.h"
#include "session_mgt.h"
#include "client_mgt.h"
#include "req_mgt.h"

//geoip
#include "GeoIP.h"
#include "GeoIPCity.h"
GeoIP *g_geoip = NULL;

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Req_Mgt *g_asyn_mgt;


Rsp_Processor::Rsp_Processor()
{

}


Rsp_Processor::~Rsp_Processor()
{

}



int Rsp_Processor::do_init(void *args)
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




int Rsp_Processor::svc()
{
	int nRet = 0;
	std::string err_info = "";
	
	//获取req 并且处理
	Request_Ptr req;
	nRet = g_asyn_mgt->get_req(req);
	if(nRet != 0)
	{
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "prepare to process rsp from svr, %s\n", req->to_string().c_str());
	req->_process_stmp = getTimestamp();

	std::string method = "";
	std::string uuid = "";
	std::string encry = "";
	std::string session_id = "";
	unsigned int req_id = 0;
	nRet = XProtocol::rsp_head(req->_req, method, uuid, encry, session_id, req_id, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, ret:%d, err_info:%s, req:%s\n", nRet, err_info.c_str(), req->to_string().c_str());
		return 0;
	}

	std::string key = "";
	if(encry == "true")
	{
		//获取安全通道key
		nRet = PSGT_Session_Mgt->get_security_channel(session_id, uuid, key, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "get_security_channel failed, ret:%d, err_info:%s, session_id:%s, uuid:%s, req:%s\n", 
				nRet, err_info.c_str(), session_id.c_str(), uuid.c_str(), req->to_string().c_str());
			return 0;
		}
		XCP_LOGGER_INFO(&g_logger, "get_security_channel success, session_id:%s, uuid:%s, key:%s\n", 
			session_id.c_str(), uuid.c_str(), key.c_str());	
	}	

	//获取会话对应客户端fd
	int client_fd = 0;
	if(!PSGT_Session_Mgt->get_fd(session_id, client_fd))
	{
		XCP_LOGGER_INFO(&g_logger, "get fd from session mgt failed, ret:%d, session_id:%s, req:%s\n", 
		nRet, session_id.c_str(), req->to_string().c_str());
		return 0;
	}

	//处理接入服务器方法
	if ((method == CMD_LOGIN_PWD) || (method == CMD_LOGIN_CODE) || (method == CMD_AUTH))
	{
		XCP_LOGGER_INFO(&g_logger, "--- %s ---\n", method.c_str());

		unsigned long long user_id = 0;
		nRet = XProtocol::get_user_login_result(req->_req, user_id, err_info);
		XCP_LOGGER_INFO(&g_logger, "complete to login, ret:%d, user id:%llu\n", nRet, user_id);
		if(nRet == 0)
		{
			//设置并且判断会话是否有效
			if(!PSGT_Session_Mgt->set_client_id(session_id, user_id))
			{
				XCP_LOGGER_INFO(&g_logger, "set client_id failed, session id:%s, user id:%llu, req:%s\n", 
					req->_session_id.c_str(), user_id, req->to_string().c_str());
				return 0;
			}
				
			StClient client;
			client._id = user_id;
			client._type = TT_APP;
			client._session_id = session_id;
			client._uuid = uuid;
			client._key = key;
			client._fd = client_fd;
			get_remote_socket(client_fd, client._ip, client._port);
			client._stmp_create = getTimestamp();
			client._geography = get_geography(client._ip);
			nRet = PSGT_Client_Mgt->register_client(client, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "register client failed, ret:%d, err_info:%s, req:%s\n", 
					nRet, err_info.c_str(), req->to_string().c_str());
				Msg_Oper::send_msg(client_fd, session_id, uuid, "false", "", method, req_id, req->_msg_tag, nRet, err_info);
				return 0;
			}	
		}
	}
	else if(method == CMD_AUTH_ROUTER)
	{
		XCP_LOGGER_INFO(&g_logger, "--- %s ---\n", method.c_str());

		unsigned long long router_id = 0;
		nRet = XProtocol::get_router_login_result(req->_req, router_id, err_info);
		if(nRet == 0)
		{
			//设置并且判断会话是否有效
			if(!PSGT_Session_Mgt->set_client_id(session_id, router_id))
			{
				XCP_LOGGER_INFO(&g_logger, "set client_id failed, session id:%s, router id:%llu\n", 
					req->_session_id.c_str(), router_id);
				return 0;
			}
			
			StClient client;
			client._id = router_id;
			client._type = TT_ROUTER;
			client._session_id = session_id;
			client._uuid = uuid;
			client._key = key;
			client._fd = client_fd;
			get_remote_socket(client_fd, client._ip, client._port);
			client._stmp_create = getTimestamp();
			client._geography = get_geography(client._ip);
			nRet = PSGT_Client_Mgt->register_client(client, err_info);
			if(nRet != 0)
			{
				XCP_LOGGER_INFO(&g_logger, "register client failed, ret:%d, err_info:%s, req:%s\n", 
					nRet, err_info.c_str(), req->to_string().c_str());
				Msg_Oper::send_msg(client_fd, session_id, uuid, "false", "", method, req_id, req->_msg_tag, nRet, err_info);
				return 0;
			}
			
		}		
	}
	else
	{
		//no-todo
	}

	//返回给前端
	Msg_Oper::send_msg(client_fd, session_id, uuid, encry, key, req->_req);

	return 0;
	
}


//获取用户地理位置
const char *_mk_NA(const char *p)
{
    return p ? p : "N/A";
}



std::string Rsp_Processor::get_geography(const std::string &ip)
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

