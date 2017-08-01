
#include "protocol.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "base/base_convert.h"
#include "base/base_logger.h"
#include "base/base_base64.h"
#include "comm/common.h"
#include "json-c/json.h"

extern Logger g_logger;

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




int XProtocol::admin_head(const std::string &req, std::string &method, unsigned long long &timestamp, std::string &msg_tag, std::string &err_info)
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

	//释放内存
	json_object_put(root);
	
	return 0;

}




int XProtocol::worker_info(const std::string &req, std::string &worker_id, std::string &ip, unsigned short &port, std::string &err_info)
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

	//worker id
	json_object *_worker_id = NULL;
	if(!json_object_object_get_ex(params, "id", &_worker_id))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no id, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no id.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_worker_id, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, worker id isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, worker id isn't string.";
		json_object_put(root);
		return -1;
	}	
	worker_id = json_object_get_string(_worker_id);

	//ip_out
	json_object *_ip = NULL;
	if(!json_object_object_get_ex(params, "ip_out", &_ip))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no ip_out, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no ip_out.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_ip, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, worker ip_out isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, worker ip_out isn't string.";
		json_object_put(root);
		return -1;
	}
	ip = json_object_get_string(_ip);
	

	//port
	json_object *_req_port = NULL;
	if(!json_object_object_get_ex(params, "port", &_req_port))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no port, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no port.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_req_port, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, worker port isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, worker port isn't string.";
		json_object_put(root);
		return -1;
	}
	port = (unsigned short)json_object_get_int(_req_port);

	//释放内存
	json_object_put(root);
	
	return 0;

}




int XProtocol::hb_req(const std::string &req, unsigned int &client_num, std::string &err_info)
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

	//client num
	json_object *_client_num = NULL;
	if(!json_object_object_get_ex(params, "client_num", &_client_num))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no client_num, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no client_num.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_client_num, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, client_num isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, client_num isn't int.";
		json_object_put(root);
		return -1;
	}	
	client_num = (unsigned int)json_object_get_int(_client_num);

	return 0;

}



std::string XProtocol::lb_rsp(const std::string &ip, const unsigned short port)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"ip\":\"%s\", \"port\":%u}", ip.c_str(), port);
	return msg;
}


