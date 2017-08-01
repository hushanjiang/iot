
#include "protocol.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "base/base_convert.h"
#include "base/base_logger.h"
#include "base/base_base64.h"
#include "comm/common.h"
#include "json-c/json.h"

extern Logger g_logger;


int XProtocol::iot_req(const std::string &req, std::string &uuid, std::string &encry, std::string &content, std::string &err_info)
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


	//uuid
	json_object *_uuid = NULL;
	if(!json_object_object_get_ex(root, "uuid", &_uuid))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no uuid, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no uuid.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_uuid, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, uuid isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, uuid isn't string.";
		json_object_put(root);
		return -1;

	}
	uuid = json_object_get_string(_uuid);


	//encry
	json_object *_encry = NULL;
	if(!json_object_object_get_ex(root, "encry", &_encry))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no encry, req(%u):%s\n", req.size(), req.c_str());
		encry = "false";
	}
	else
	{
		if(!json_object_is_type(_encry, json_type_string))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, encry isn't string, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, encry isn't string.";
			json_object_put(root);
			return -1;
		
		}
		encry = json_object_get_string(_encry);
		if((encry != "true") && (encry != "false"))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, invalid encry, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, invalid encry.";
			json_object_put(root);
			return -1;
		}
	}


	//content
	json_object *_content = NULL;
	if(!json_object_object_get_ex(root, "content", &_content))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no content, req(%u):%s\n", req.size(), req.c_str());
		content = "";
	}
	else
	{
		if(!json_object_is_type(_content, json_type_string) && !json_object_is_type(_content, json_type_object))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, content isn't string, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, content isn't string.";
			json_object_put(root);
			return -1;
		}
		content = json_object_get_string(_content);
	}	

	//释放内存
	json_object_put(root);
	
	return 0;

}





int XProtocol::req_head(const std::string &req, std::string &method, unsigned long long &timestamp, unsigned int &req_id, std::string &err_info)
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




std::string XProtocol::add_msg_tag(const std::string &req, const std::string &msg_tag)
{

	std::string new_req = "";
	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(req.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
			req.c_str(), json_tokener_error_desc(jerr));
		return new_req;
	}
	
	json_object_object_add(root, "msg_tag", json_object_new_string(msg_tag.c_str()));
	new_req = json_object_get_string(root);
	new_req += std::string("\n");

	//释放内存
	json_object_put(root);

	return new_req;

}




std::string XProtocol::add_id(const std::string &req, const unsigned long long real_id)
{
	std::string new_req = "";
	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(req.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
			req.c_str(), json_tokener_error_desc(jerr));
		return new_req;
	}
	
	json_object_object_add(root, "real_id", json_object_new_int64(real_id));
	
	new_req = json_object_get_string(root);
	new_req += std::string("\n");

	//释放内存
	json_object_put(root);

	return new_req;

}



std::string XProtocol::get_server_access_req(const std::string &msg_id)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\"}}\n", 
		CMD_GET_SERVER_ACCESS.c_str(), getTimestamp(), msg_id.c_str());
	
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

	//lb svr
	json_object *_lb = NULL;
	if(!json_object_object_get_ex(result, ST_LB.c_str(), &_lb))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no lb, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no lb.";
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_lb, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, lb svr isn't array, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, lb svr isn't array.";
			json_object_put(root);
			return -1;
		}

		std::vector<StSvr> vec_svr;
	    int size = json_object_array_length(_lb);
	    for(int i=0; i<size; i++) 
		{
	        json_object *_svr_one = json_object_array_get_idx(_lb, i);

			StSvr svr;
			
			//ip
			json_object *_ip = NULL;
			if(!json_object_object_get_ex(_svr_one, "ip", &_ip))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no lb ip, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no lb ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, lb ip isn't string, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, lb ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no lb port, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no lb port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, lb port isn't int, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, lb port isn't int.";
				json_object_put(root);
				return -1;
			}	
			svr._port = (unsigned short)json_object_get_int(_port);
			
			vec_svr.push_back(svr);
			
	    }
		svrs.insert(std::make_pair(ST_LB, vec_svr));

	}


	//user mgt svr
	json_object *_user_mgt = NULL;
	if(!json_object_object_get_ex(result, ST_USER_MGT.c_str(), &_user_mgt))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no user_mgt, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no user_mgt.";
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_user_mgt, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, user_mgt svr isn't array, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, user_mgt svr isn't array.";
			json_object_put(root);
			return -1;
		}

		std::vector<StSvr> vec_svr;
	    int size = json_object_array_length(_user_mgt);
	    for(int i=0; i<size; i++) 
		{
	        json_object *_svr_one = json_object_array_get_idx(_user_mgt, i);

			StSvr svr;
			
			//ip
			json_object *_ip = NULL;
			if(!json_object_object_get_ex(_svr_one, "ip", &_ip))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no user_mgt ip, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no user_mgt ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, user_mgt ip isn't string, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, user_mgt ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no lb port, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no lb port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, user_mgt port isn't int, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, user_mgt port isn't int.";
				json_object_put(root);
				return -1;
			}	
			svr._port = (unsigned short)json_object_get_int(_port);
			
			vec_svr.push_back(svr);
			
	    }
		svrs.insert(std::make_pair(ST_USER_MGT, vec_svr));

	}


	//device mgt svr
	json_object *_device_mgt = NULL;
	if(!json_object_object_get_ex(result, ST_DEVICE_MGT.c_str(), &_device_mgt))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no device_mgt, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no device_mgt.";
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_device_mgt, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, device_mgt svr isn't array, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, device_mgt svr isn't array.";
			json_object_put(root);
			return -1;
		}

		std::vector<StSvr> vec_svr;
	    int size = json_object_array_length(_device_mgt);
	    for(int i=0; i<size; i++) 
		{
	        json_object *_svr_one = json_object_array_get_idx(_device_mgt, i);

			StSvr svr;
			
			//ip
			json_object *_ip = NULL;
			if(!json_object_object_get_ex(_svr_one, "ip", &_ip))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no device_mgt ip, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no device_mgt ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, device_mgt ip isn't string, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, device_mgt ip isn't string.";
				json_object_put(root);
				return -1;
			}				
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no device_mgt port, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no device_mgt port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, device_mgt port isn't int, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, device_mgt port isn't int.";
				json_object_put(root);
				return -1;
			}			
			svr._port = (unsigned short)json_object_get_int(_port);
			vec_svr.push_back(svr);
			
	    }

		svrs.insert(std::make_pair(ST_DEVICE_MGT, vec_svr));

	}


	//family mgt svr
	json_object *_family_mgt = NULL;
	if(!json_object_object_get_ex(result, ST_FAMILY_MGT.c_str(), &_family_mgt))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no family_mgt, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no session_mgt.";
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_family_mgt, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, family_mgt svr isn't array, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, family_mgt svr isn't array.";
			json_object_put(root);
			return -1;
		}

		std::vector<StSvr> vec_svr;
	    int size = json_object_array_length(_family_mgt);
	    for(int i=0; i<size; i++) 
		{
	        json_object *_svr_one = json_object_array_get_idx(_family_mgt, i);

			StSvr svr;
			
			//ip
			json_object *_ip = NULL;
			if(!json_object_object_get_ex(_svr_one, "ip", &_ip))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no family_mgt ip, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no family_mgt ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, family_mgt ip isn't string, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, family_mgt ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no family_mgt port, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no family_mgt port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid req, family_mgt port isn't int, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, family_mgt port isn't int.";
				json_object_put(root);
				return -1;
			}			
			svr._port = (unsigned short)json_object_get_int(_port);
			vec_svr.push_back(svr);
			
	    }

		svrs.insert(std::make_pair(ST_FAMILY_MGT, vec_svr));

	}	


	//mdp
	json_object *_mdp = NULL;
	if(!json_object_object_get_ex(result, ST_MDP.c_str(), &_mdp))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no mdp, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no mdp.";
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_family_mgt, json_type_array))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, mdp isn't array, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, mdp isn't array.";
			json_object_put(root);
			return -1;
		}

		std::vector<StSvr> vec_svr;
		int size = json_object_array_length(_mdp);
		for(int i=0; i<size; i++) 
		{
			json_object *_svr_one = json_object_array_get_idx(_mdp, i);

			StSvr svr;
			
			//ip
			json_object *_ip = NULL;
			if(!json_object_object_get_ex(_svr_one, "ip", &_ip))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no mdp ip, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no mdp ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, mdp ip isn't string, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, mdp ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, no mdp port, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, no mdp port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid req, mdp port isn't int, req(%u):%s\n", req.size(), req.c_str());
				err_info = "it is invalid req, mdp port isn't int.";
				json_object_put(root);
				return -1;
			}			
			svr._port = (unsigned short)json_object_get_int(_port);
			vec_svr.push_back(svr);
			
		}

		svrs.insert(std::make_pair(ST_MDP, vec_svr));

	}	


	//释放内存
	json_object_put(root);
	
	return 0;

}



