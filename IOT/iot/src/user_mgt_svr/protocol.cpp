
#include "protocol.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "base/base_convert.h"
#include "base/base_logger.h"
#include "base/base_base64.h"
#include "comm/common.h"
#include "json-c/json.h"

extern Logger g_logger;

int XProtocol::req_head(const std::string &req, std::string &method, unsigned long long &timestamp, unsigned int &req_id, std::string &msg_tag, std::string &err_info)
{
	err_info = "";
	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(req.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
			req.c_str(), json_tokener_error_desc(jerr));
		err_info = "it is invlid json req.";
		return -1;
	}


	//method
	json_object *_method = NULL;
	if(!json_object_object_get_ex(root, "method", &_method))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no method, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no method.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_method, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, method isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, method isn't string.";
		json_object_put(root);
		return -1;

	}
	method = json_object_get_string(_method);


	//timestamp
	json_object *_timestamp = NULL;
	if(!json_object_object_get_ex(root, "timestamp", &_timestamp))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no timestamp, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no timestamp.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_timestamp, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, timestamp isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, timestamp isn't int.";
		json_object_put(root);
		return -1;
	}
	timestamp = (unsigned long long)json_object_get_int64(_timestamp);

	
	//req_id
	json_object *_req_id = NULL;
	if(!json_object_object_get_ex(root, "req_id", &_req_id))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no req_id, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no req_id.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_req_id, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, req_id isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, req_id isn't int.";
		json_object_put(root);
		return -1;
	}
	req_id = (unsigned int)json_object_get_int(_req_id);


	//msg_tag
	json_object *_msg_tag = NULL;
	if(!json_object_object_get_ex(root, "msg_tag", &_msg_tag))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg_tag, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no msg_tag.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_msg_tag, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, msg_tag isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, msg_tag isn't string.";
		json_object_put(root);
		return -1;
	}
	msg_tag = json_object_get_string(_msg_tag);

	//释放内存
	json_object_put(root);
	
	return 0;
	
}



std::string XProtocol::rsp_msg(const std::string &method, const unsigned int req_id, const std::string &msg_tag,  
		const int code, const std::string &msg, const std::string &body, bool is_object)
{	
	std::string rsp = "";
	
	if(body == "")
	{
		char buf[1024] = {0};
		memset(buf, 0x0, 1024);
		snprintf(buf, 1024, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\"}\n", 
			method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str());
		rsp = buf;
	}
	else
	{
		unsigned int len = body.size()+1024;
		char *buf = new char[len];
		memset(buf, 0x0, len);
		if(is_object)
		{
			snprintf(buf, len, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\", \"result\":%s}\n", 
				method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str(), body.c_str());
		}
		else
		{
			snprintf(buf, len, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\", \"result\":\"%s\"}\n", 
				method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str(), body.c_str());
		}
		rsp = buf;
		DELETE_POINTER_ARR(buf);
	}

	return rsp;
	
}


int XProtocol::admin_head(const std::string &req, std::string &method, unsigned long long &timestamp, std::string &msg_tag, int &code, std::string &msg, std::string &err_info)
{	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(req.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
			req.c_str(), json_tokener_error_desc(jerr));
		err_info = "it is invlid json req.";
		return -1;
	}

	//method
	json_object *_method = NULL;
	if(!json_object_object_get_ex(root, "method", &_method))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no method, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no method.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_method, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, method isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, method isn't string.";
		json_object_put(root);
		return -1;

	}
	method = json_object_get_string(_method);


	//timestamp
	json_object *_timestamp = NULL;
	if(!json_object_object_get_ex(root, "timestamp", &_timestamp))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no timestamp, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no timestamp.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_timestamp, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, timestamp isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, timestamp isn't int.";
		json_object_put(root);
		return -1;
	}
	timestamp = (unsigned long long)json_object_get_int64(_timestamp);


	//msg_tag
	json_object *_msg_tag = NULL;
	if(!json_object_object_get_ex(root, "msg_tag", &_msg_tag))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg_tag, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no msg_tag.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_msg_tag, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, msg_tag isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, msg_tag isn't string.";
		json_object_put(root);
		return -1;
	}
	msg_tag = json_object_get_string(_msg_tag);


	//code
	json_object *_code = NULL;
	if(!json_object_object_get_ex(root, "code", &_code))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no code, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no code.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_code, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, code isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, code isn't int.";
		json_object_put(root);
		return -1;
	}	
	code = json_object_get_int(_code);

	
	//msg
	json_object *_msg = NULL;
	if(!json_object_object_get_ex(root, "msg", &_msg))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg, req(%u):%s\n", req.size(), req.c_str());
		msg = "";
	}
	else
	{
		if(!json_object_is_type(_msg, json_type_string))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, msg isn't string, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, msg isn't string.";
			json_object_put(root);
			return -1;
		}	
		msg = json_object_get_string(_msg);
	}
	

	//释放内存
	json_object_put(root);
	
	return 0;

}




std::string XProtocol::set_hb(const std::string &msg_id, const unsigned int client_num)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"client_num\":%u}}\n", 
		CMD_HB.c_str(), getTimestamp(), msg_id.c_str(), client_num);

	return msg;
}



std::string XProtocol::set_register(const std::string &msg_tag, const std::string &worker_id, 
	const std::string &ip, const std::string &ip_out, const unsigned short port)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"id\":\"%s\", \"ip\":\"%s\",  \"ip_out\":\"%s\", \"port\":%u}}\n", 
		CMD_REGISTER.c_str(), getTimestamp(), msg_tag.c_str(), worker_id.c_str(), ip.c_str(), ip_out.c_str(), port);

	return msg;
}



std::string XProtocol::set_unregister(const std::string &msg_id, const std::string &worker_id)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"id\":\"%s\"}}\n", 
		CMD_REGISTER.c_str(), getTimestamp(), msg_id.c_str(), worker_id.c_str());
	
	return msg;
}




std::string XProtocol::get_server_access_req(const std::string &msg_id)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"svr_name\":\"%s\"}}}\n", 
		CMD_GET_SERVER_ACCESS.c_str(), getTimestamp(), msg_id.c_str(), ST_UID.c_str());
	
	return msg;
}



int XProtocol::get_server_access_rsp(const std::string &req, std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info)
{
	err_info = "";
	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(req.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
			req.c_str(), json_tokener_error_desc(jerr));
		err_info = "it is invlid json req.";
		return -1;
	}

	//code
	json_object *_code = NULL;
	if(!json_object_object_get_ex(root, "code", &_code))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no code, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no code.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_code, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, code isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, code isn't int.";
		json_object_put(root);
		return -1;
	}	
	int code = json_object_get_int(_code);
	if(code != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "get server access failed, req(%u):%s\n", req.size(), req.c_str());
		return code;
	}

	//result
	json_object *result = NULL;
	if(!json_object_object_get_ex(root, "result", &result))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no result, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no result.";
		json_object_put(root);
		return -1;
	}

	//uid svr
	json_object *_uid = NULL;
	if(!json_object_object_get_ex(result, ST_UID.c_str(), &_uid))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no uid, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no uid.";
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_uid, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, uid svr isn't array, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, uid svr isn't array.";
			json_object_put(root);
			return -1;
		}

		std::vector<StSvr> vec_svr;
	    int size = json_object_array_length(_uid);
	    for(int i=0; i<size; i++) 
		{
	        json_object *_svr_one = json_object_array_get_idx(_uid, i);

			StSvr svr;
			
			//ip
			json_object *_ip = NULL;
			if(!json_object_object_get_ex(_svr_one, "ip", &_ip))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no uid ip, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no uid ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, uid ip isn't string, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, uid ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no uid port, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no uid port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, uid port isn't int, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, uid port isn't int.";
				json_object_put(root);
				return -1;
			}	
			svr._port = (unsigned short)json_object_get_int(_port);
			
			vec_svr.push_back(svr);
			
	    }
		svrs.insert(std::make_pair(ST_UID, vec_svr));

	}	

	//释放内存
	json_object_put(root);
	
	return 0;

}




std::string XProtocol::get_uuid_req(const std::string &msg_tag, std::string &err_info)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"uuid_name\":\"mdp_id\"}}\n", 
		CMD_GET_UUID.c_str(), getTimestamp(), msg_tag.c_str());
	return msg;
}




int XProtocol::get_uuid_rsp(const std::string &req, unsigned long long &uuid, std::string &err_info)
{
	err_info = "";
	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(req.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
			req.c_str(), json_tokener_error_desc(jerr));
		err_info = "it is invlid json req.";

		return -1;
	}

	//result
	json_object *_result = NULL;
	if(!json_object_object_get_ex(root, "result", &_result))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no result, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no result.";
		json_object_put(root);
		return -1;
	}

	//uuid
	json_object *_uuid = NULL;
	if(!json_object_object_get_ex(_result, "uuid", &_uuid))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no uuid, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no uuid.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_uuid, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, uuid isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, uuid isn't int.";
		json_object_put(root);
		return -1;
	}
	uuid = (unsigned long long)json_object_get_int64(_uuid);

	//释放内存
	json_object_put(root);
	
	return 0;

}




int XProtocol::get_rsp_result(const std::string &req, std::string &err_info)
{
	err_info = "";
	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(req.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
			req.c_str(), json_tokener_error_desc(jerr));
		err_info = "it is invlid json req.";

		return -1;
	}


	//code
	json_object *_code = NULL;
	if(!json_object_object_get_ex(root, "code", &_code))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no code, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no code.";
		json_object_put(root);
		return -1;
	}
	int code = json_object_get_int(_code);

	
	//msg
	json_object *_msg = NULL;
	if(!json_object_object_get_ex(root, "msg", &_msg))
	{
		err_info = "";
	}
	else
	{
		err_info = json_object_get_string(_msg);
	}

	//释放内存
	json_object_put(root);
	
	return code;

}


