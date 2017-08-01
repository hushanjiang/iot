
#include "protocol.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "base/base_convert.h"
#include "base/base_logger.h"
#include "base/base_base64.h"
#include "comm/common.h"
#include "json-c/json.h"

extern Logger g_logger;

int XProtocol::req_head(const std::string &req, std::string &method, unsigned long long &timestamp, unsigned int &req_id, 
		std::string &msg_tag, std::string &err_info)
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
		msg_tag = "";
	}
	else
	{
		if(!json_object_is_type(_msg_tag, json_type_string))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, msg_tag isn't string, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, msg_tag isn't string.";
			json_object_put(root);
			return -1;
		}
		msg_tag = json_object_get_string(_msg_tag);
	}

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



int XProtocol::get_uuid_req(const std::string &req, std::string &uuid_name, std::string &err_info)
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

	//params
	json_object *params = NULL;
	if(!json_object_object_get_ex(root, "params", &params))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no params, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no params.";
		json_object_put(root);
		return -1;
	}

	//uuid_name
	json_object *_uuid_name = NULL;
	if(!json_object_object_get_ex(params, "uuid_name", &_uuid_name))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no uuid_name, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no uuid_name.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_uuid_name, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, uuid_name isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, uuid_name isn't string.";
		json_object_put(root);
		return -1;

	}
	uuid_name = json_object_get_string(_uuid_name);

	//释放内存
	json_object_put(root);
	
	return 0;

}



std::string XProtocol::get_uuid_rsp(unsigned long long uuid)
{
	char buf[100] = {0};
	snprintf(buf, 100, "{\"uuid\":%llu}", uuid);
	return buf;
}

