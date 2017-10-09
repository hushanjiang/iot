
#include "protocol.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "base/base_convert.h"
#include "base/base_logger.h"
#include "base/base_base64.h"
#include "base/base_cryptopp.h"
#include "base/base_openssl.h"

#include "comm/common.h"
#include "json-c/json.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;

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
		if(!json_object_is_type(_content, json_type_object))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, content isn't object, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, content isn't object.";
			json_object_put(root);
			return -1;
		}
		//content = json_object_to_json_string(_content);
		content = json_object_to_json_string_ext(_content, JSON_C_TO_STRING_PLAIN);
	}

	if(content.empty())
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, content is empty, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, content is empty.";
		json_object_put(root);
		return -1;		
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



int XProtocol::rsp_head(const std::string &req, std::string &method, std::string &uuid, std::string &encry, std::string &session_id, unsigned int &req_id, std::string &err_info)
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
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no method, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, no method.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_method, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, method isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, method isn't string.";
		json_object_put(root);
		return -1;

	}
	method = json_object_get_string(_method);
	
	//msg_uuid
	json_object *_msg_uuid = NULL;
	if(!json_object_object_get_ex(root, "msg_uuid", &_msg_uuid))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no msg_uuid, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, no msg_uuid.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_msg_uuid, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, msg_uuid isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, msg_uuid isn't string.";
		json_object_put(root);
		return -1;

	}
	uuid = json_object_get_string(_msg_uuid);


	//msg_encry
	json_object *_msg_encry = NULL;
	if(!json_object_object_get_ex(root, "msg_encry", &_msg_encry))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no msg_encry, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, no msg_encry.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_msg_encry, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, msg_encry isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, msg_encry isn't string.";
		json_object_put(root);
		return -1;

	}
	encry = json_object_get_string(_msg_encry);


	//session_id
	json_object *_session_id = NULL;
	if(!json_object_object_get_ex(root, "session_id", &_session_id))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no session_id, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, no session_id.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_session_id, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, session_id isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, session_id isn't string.";
		json_object_put(root);
		return -1;

	}
	session_id = json_object_get_string(_session_id);

	//req_id
	json_object *_req_id = NULL;
	if(!json_object_object_get_ex(root, "req_id", &_req_id))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no req_id, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, no req_id.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_req_id, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, req_id isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid rsp, req_id isn't int.";
		json_object_put(root);
		return -1;
	}
	req_id = (unsigned int)json_object_get_int(_req_id);

	//释放内存
	json_object_put(root);
	
	return 0;
}




std::string XProtocol::rsp_msg(const std::string &uuid, const std::string &encry, const std::string &key, const std::string &method, 
	const unsigned int req_id, const std::string &msg_tag, const int code, const std::string &msg, const std::string &body, bool is_object)
{
	std::string rsp = "";

	if(encry == "true")
	{
		//需要加密处理
		
		std::string src = "";
		if(body == "")
		{
			char buf[2048] = {0};
			memset(buf, 0x0, 2048);
			snprintf(buf, 2048, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\"}", 
				method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str());
			src = buf;
		}
		else
		{
			unsigned int len = body.size()+2048;
			char *buf = new char[len];
			memset(buf, 0x0, len);
			if(is_object)
			{
				snprintf(buf, len, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\", \"result\":%s}", 
					uuid.c_str(), method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str(), body.c_str());
			}
			else
			{
				snprintf(buf, len, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\", \"result\":\"%s\"}", 
					uuid.c_str(), method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str(), body.c_str());
			}
			src = buf;
			DELETE_POINTER_ARR(buf);
		}

		//加密
		std::string encrypt = "";
		SSL_AES aes;
		if(!aes.encrypt(key, src, encrypt, AES_CBC_IV))
		{
			XCP_LOGGER_INFO(&g_logger, "aes encrypt failed. key:%s, src:%s\n", key.c_str(), src.c_str());
			return rsp;
		}
		
		//base64
		X_BASE64 base64;
		char *tmp = NULL;
		if(!base64.encrypt(encrypt.c_str(), encrypt.size(), tmp))
		{
			XCP_LOGGER_INFO(&g_logger, "base64 encrypt faield.\n");
			return rsp;
		}
		std::string base64_value = tmp;
		DELETE_POINTER_ARR(tmp);

		//添加安全头
		unsigned int len = base64_value.size()+100;
		char *buf = new char[len];
		memset(buf, 0x0, len);		
		snprintf(buf, len, "{\"uuid\":\"%s\", \"encry\":\"true\", \"content\":\"%s\"}\n", uuid.c_str(), base64_value.c_str());
		rsp = buf;
		DELETE_POINTER_ARR(buf);
		
	}
	else
	{
		//不需要加密
		
		if(body == "")
		{
			char buf[2048] = {0};
			memset(buf, 0x0, 2048);
			snprintf(buf, 2048, "{\"uuid\":\"%s\", \"encry\":\"false\", \"content\":{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\"}}\n", 
				uuid.c_str(), method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str());
			rsp = buf;
		}
		else
		{
			unsigned int len = body.size()+2048;
			char *buf = new char[len];
			memset(buf, 0x0, len);
			if(is_object)
			{
				snprintf(buf, len, "{\"uuid\":\"%s\", \"encry\":\"false\", \"content\":{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\", \"result\":%s}}\n", 
					uuid.c_str(), method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str(), body.c_str());
			}
			else
			{
				snprintf(buf, len, "{\"uuid\":\"%s\", \"encry\":\"false\", \"content\":{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":%u, \"msg_tag\":\"%s\", \"code\":%d, \"msg\":\"%s\", \"result\":\"%s\"}}\n", 
					uuid.c_str(), method.c_str(), getTimestamp(), req_id, msg_tag.c_str(), code, msg.c_str(), body.c_str());
			}
			rsp = buf;
			DELETE_POINTER_ARR(buf);
		}

	}

	return rsp;

}




std::string XProtocol::rsp_msg(const std::string &uuid, const std::string &encry, const std::string &key, const std::string &buf)
{
	std::string rsp = "";

	if(encry == "true")
	{
		//加密
		std::string encrypt = "";
		SSL_AES aes;
		if(!aes.encrypt(key, buf, encrypt, AES_CBC_IV))
		{
			XCP_LOGGER_INFO(&g_logger, "aes encrypt failed. key:%s, buf:%s\n", key.c_str(), buf.c_str());
			return rsp;
		}
		
		//base64
		X_BASE64 base64;
		char *tmp1 = NULL;
		if(!base64.encrypt(encrypt.c_str(), encrypt.size(), tmp1))
		{
			XCP_LOGGER_INFO(&g_logger, "base64 encrypt faield.\n");
			return rsp;
		}
		std::string base64_value = tmp1;
		DELETE_POINTER_ARR(tmp1);

		//添加安全头
		unsigned int len = base64_value.size()+100;
		char *tmp2 = new char[len];
		memset(tmp2, 0x0, len);		
		snprintf(tmp2, len, "{\"uuid\":\"%s\", \"encry\":\"true\", \"content\":\"%s\"}\n", uuid.c_str(), base64_value.c_str());
		rsp = tmp2;
		DELETE_POINTER_ARR(tmp2);
		
	}
	else
	{
		unsigned int len = buf.size()+100;
		char *tmp = new char[len];
		memset(tmp, 0x0, len);
		snprintf(tmp, len, "{\"uuid\":\"%s\", \"encry\":\"false\", \"content\":%s}\n", uuid.c_str(), buf.c_str());
		rsp = tmp;
		DELETE_POINTER_ARR(tmp);	
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



int XProtocol::get_user_login_result(const std::string &req, unsigned long long &user_id, std::string &err_info)
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

	//msg
	json_object *_msg = NULL;
	if(!json_object_object_get_ex(root, "msg", &_msg))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg, req(%u):%s\n", req.size(), req.c_str());
		err_info = "";
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
		err_info = json_object_get_string(_msg);
	}

	if(code == 0)
	{
		//result
		json_object *result = NULL;
		if(!json_object_object_get_ex(root, "result", &result))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, no result, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, no result.";
			json_object_put(root);
			return -1;
		}

		//user_id
		json_object *_user_id = NULL;
		if(!json_object_object_get_ex(result, "user_id", &_user_id))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, no user_id, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, no user_id.";
			json_object_put(root);
			return -1;
		}
		if(!json_object_is_type(_user_id, json_type_int))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, user_id isn't int, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, user_id isn't int.";
			json_object_put(root);
			return -1;
		}
		user_id = (unsigned long long)json_object_get_int64(_user_id);
		if(user_id == 0)
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, user_id is 0, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, user_id is 0.";
			json_object_put(root);
			return -1;
		}

	}
	
	//释放内存
	json_object_put(root);
	
	return code;

}




int XProtocol::get_router_login_result(const std::string &req, unsigned long long &router_id, std::string &err_info)
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

	//msg
	json_object *_msg = NULL;
	if(!json_object_object_get_ex(root, "msg", &_msg))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg, req(%u):%s\n", req.size(), req.c_str());
		err_info = "";
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
		err_info = json_object_get_string(_msg);
	}

	if(code == 0)
	{
		//result
		json_object *result = NULL;
		if(!json_object_object_get_ex(root, "result", &result))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, no result, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, no result.";
			json_object_put(root);
			return -1;
		}

		//router_id
		json_object *_router_id = NULL;
		if(!json_object_object_get_ex(result, "router_id", &_router_id))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, no router_id, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, no router_id.";
			json_object_put(root);
			return -1;
		}
		if(!json_object_is_type(_router_id, json_type_int))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, router_id isn't int, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, router_id isn't int.";
			json_object_put(root);
			return -1;
		}
		router_id = (unsigned long long)json_object_get_int64(_router_id);
		if(router_id == 0)
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, router_id is 0, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, router_id is 0.";
			json_object_put(root);
			return -1;
		}

	}
	
	//释放内存
	json_object_put(root);
	
	return code;

}



//注册lb svr
std::string XProtocol::register_lb_req(const std::string &msg_tag, const std::string &worker_id, const std::string &ip, const unsigned short port)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"id\":\"%s\", \"ip\":\"%s\", \"port\":%u}}\n", 
		CMD_REGISTER.c_str(), getTimestamp(), msg_tag.c_str(), worker_id.c_str(), ip.c_str(), port);

	return msg;
}




//注册mdp
std::string XProtocol::register_mdp_req(const std::string &msg_tag, const std::string &worker_id)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"id\":\"%s\", \"type\":\"%s\"}}\n", 
		CMD_MDP_REGISTER.c_str(), getTimestamp(), msg_tag.c_str(), worker_id.c_str(), ST_ACCESS.c_str());

	return msg;
}




//lb 心跳
std::string XProtocol::hb_lb_req(const std::string &msg_tag, const unsigned int client_num)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"client_num\":%u}}\n", 
		CMD_HB.c_str(), getTimestamp(), msg_tag.c_str(), client_num);

	return msg;
}


std::string XProtocol::add_inner_head(const std::string &req, const std::string &msg_tag, const unsigned long long real_id, const std::string &uuid, const std::string &encry, 
	const std::string &from_svr, const std::string &session_id)
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
	json_object_object_add(root, "real_id", json_object_new_int64(real_id));
	json_object_object_add(root, "msg_uuid", json_object_new_string(uuid.c_str()));
	json_object_object_add(root, "msg_encry", json_object_new_string(encry.c_str()));
	json_object_object_add(root, "from_svr", json_object_new_string(from_svr.c_str()));
	json_object_object_add(root, "session_id", json_object_new_string(session_id.c_str()));
	new_req = json_object_get_string(root);
	
	//释放内存
	json_object_put(root);

	return new_req;

}




std::string XProtocol::add_mdp_msg_head(const std::string &req, const unsigned long long target_id, const unsigned long long group_id)
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
	
	json_object_object_add(root, "target_id", json_object_new_int64(target_id));
	
	if(group_id != 0)
	{
		json_object_object_add(root, "group_id", json_object_new_int64(group_id));
	}
	
	new_req = json_object_get_string(root);

	//释放内存
	json_object_put(root);

	return new_req;

}



std::string XProtocol::get_server_access_req(const std::string &msg_tag)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\"}}\n", 
		CMD_GET_SERVER_ACCESS.c_str(), getTimestamp(), msg_tag.c_str());
	
	return msg;
}




int XProtocol::get_server_access_rsp(const std::string &rsp, std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info)
{
	err_info = "";
	
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(rsp.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, rsp:%s, err:%s\n", 
			rsp.c_str(), json_tokener_error_desc(jerr));
		err_info = "it is invlid json rsp.";
		return -1;
	}

	//code
	json_object *_code = NULL;
	if(!json_object_object_get_ex(root, "code", &_code))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no code, rsp(%u):%s\n", rsp.size(), rsp.c_str());
		err_info = "it is invalid rsp, no code.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_code, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, code isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
		err_info = "it is invalid rsp, code isn't int.";
		json_object_put(root);
		return -1;
	}	
	int code = json_object_get_int(_code);
	if(code != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "get server access failed, rsp(%u):%s\n", rsp.size(), rsp.c_str());
		return code;
	}

	//result
	json_object *result = NULL;
	if(!json_object_object_get_ex(root, "result", &result))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no result, rsp(%u):%s\n", rsp.size(), rsp.c_str());
		err_info = "it is invalid rsp, no result.";
		json_object_put(root);
		return -1;
	}

	//lb svr
	json_object *_lb = NULL;
	if(!json_object_object_get_ex(result, ST_LB.c_str(), &_lb))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no lb, rsp(%u):%s\n", rsp.size(), rsp.c_str());
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_lb, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, lb svr isn't array, rsp(%u):%s\n", rsp.size(), rsp.c_str());
			err_info = "it is invalid rsp, lb svr isn't array.";
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
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no lb ip, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no lb ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, lb ip isn't string, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, lb ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no lb port, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no lb port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, lb port isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, lb port isn't int.";
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
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no user_mgt, rsp(%u):%s\n", rsp.size(), rsp.c_str());
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_user_mgt, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, user_mgt svr isn't array, rsp(%u):%s\n", rsp.size(), rsp.c_str());
			err_info = "it is invalid rsp, user_mgt svr isn't array.";
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
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no user_mgt ip, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no user_mgt ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, user_mgt ip isn't string, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, user_mgt ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no lb port, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no lb port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, user_mgt port isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, user_mgt port isn't int.";
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
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no device_mgt, rsp(%u):%s\n", rsp.size(), rsp.c_str());
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_device_mgt, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, device_mgt svr isn't array, rsp(%u):%s\n", rsp.size(), rsp.c_str());
			err_info = "it is invalid rsp, device_mgt svr isn't array.";
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
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no device_mgt ip, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no device_mgt ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, device_mgt ip isn't string, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, device_mgt ip isn't string.";
				json_object_put(root);
				return -1;
			}				
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no device_mgt port, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no device_mgt port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, device_mgt port isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, device_mgt port isn't int.";
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
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no family_mgt, rsp(%u):%s\n", rsp.size(), rsp.c_str());
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_family_mgt, json_type_array))
		{
	  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, family_mgt svr isn't array, rsp(%u):%s\n", rsp.size(), rsp.c_str());
			err_info = "it is invalid rsp, family_mgt svr isn't array.";
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
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no family_mgt ip, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no family_mgt ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, family_mgt ip isn't string, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, family_mgt ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no family_mgt port, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no family_mgt port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
		  		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, family_mgt port isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, family_mgt port isn't int.";
				json_object_put(root);
				return -1;
			}			
			svr._port = (unsigned short)json_object_get_int(_port);
			vec_svr.push_back(svr);
			
	    }
		svrs.insert(std::make_pair(ST_FAMILY_MGT, vec_svr));

	}	


	//push svr
	json_object *_push = NULL;
	if(!json_object_object_get_ex(result, ST_PUSH.c_str(), &_push))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no push, rsp(%u):%s\n", rsp.size(), rsp.c_str());
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_push, json_type_array))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, push svr isn't array, rsp(%u):%s\n", rsp.size(), rsp.c_str());
			err_info = "it is invalid rsp, syn svr isn't array.";
			json_object_put(root);
			return -1;
		}

		std::vector<StSvr> vec_svr;
		int size = json_object_array_length(_push);
		for(int i=0; i<size; i++) 
		{
			json_object *_svr_one = json_object_array_get_idx(_push, i);

			StSvr svr;
			
			//ip
			json_object *_ip = NULL;
			if(!json_object_object_get_ex(_svr_one, "ip", &_ip))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no push ip, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no push ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, push ip isn't string, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, push ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no push port, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no spushyn port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, push port isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, push port isn't int.";
				json_object_put(root);
				return -1;
			}			
			svr._port = (unsigned short)json_object_get_int(_port);
			vec_svr.push_back(svr);
			
		}
		svrs.insert(std::make_pair(ST_PUSH, vec_svr));

	}


	//mdp
	json_object *_mdp = NULL;
	if(!json_object_object_get_ex(result, ST_MDP.c_str(), &_mdp))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no mdp, rsp(%u):%s\n", rsp.size(), rsp.c_str());
	}
	else
	{
		//判断svr 是否是数组类型
		if(!json_object_is_type(_family_mgt, json_type_array))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, mdp isn't array, rsp(%u):%s\n", rsp.size(), rsp.c_str());
			err_info = "it is invalid rsp, mdp isn't array.";
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
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no mdp ip, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no mdp ip.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_ip, json_type_string))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, mdp ip isn't string, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, mdp ip isn't string.";
				json_object_put(root);
				return -1;
			}			
			svr._ip = json_object_get_string(_ip);

			//port
			json_object *_port = NULL;
			if(!json_object_object_get_ex(_svr_one, "port", &_port))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, no mdp port, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, no mdp port.";
				json_object_put(root);
				return -1;
			}
			if(!json_object_is_type(_port, json_type_int))
			{
				XCP_LOGGER_INFO(&g_logger, "it is invalid rsp, mdp port isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
				err_info = "it is invalid rsp, mdp port isn't int.";
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





int XProtocol::to_mdp_req(const std::string &req, std::string &method, unsigned long long &target_id, std::string &msg_type, std::string &err_info)
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
	

	//params
	json_object *_params = NULL;
	if(!json_object_object_get_ex(root, "params", &_params))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no params, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no params.";
		json_object_put(root);
		return -1;
	}
	
	//target_id
	json_object *_target_id = NULL;
	if(!json_object_object_get_ex(_params, "target_id", &_target_id))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no target_id, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no target_id.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_target_id, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, target_id isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, target_id isn't int.";
		json_object_put(root);
		return -1;
	}
	target_id = (unsigned long long)json_object_get_int64(_target_id);

	//msg_type
	json_object *_msg_type = NULL;
	if(!json_object_object_get_ex(_params, "msg_type", &_msg_type))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg_type, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no msg_type.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_msg_type, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, msg_type isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, msg_type isn't string.";
		json_object_put(root);
		return -1;
	}
	msg_type = json_object_get_string(_msg_type);
	if(!((msg_type == MT_P2P) || (msg_type == MT_P2R) || (msg_type == MT_P2F) || (msg_type == MT_R2P) || (msg_type == MT_R2F)))
	{
		XCP_LOGGER_INFO(&g_logger, "invalid msg type, req(%u):%s\n", req.size(), req.c_str());
		err_info = "invalid msg type.";
		json_object_put(root);
		return -1;
	}

	//释放内存
	json_object_put(root);
	
	return 0;

}




int XProtocol::from_mdp_req(std::string &req, std::string &method, std::string &sys_method, std::string &msg_type, unsigned long long &target_id, std::string &encry, std::string &err_info)
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

	//encry
	json_object *_encry = NULL;
	if(!json_object_object_get_ex(root, "msg_encry", &_encry))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg_encry, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no msg_encry.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_encry, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, msg_encry isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, msg_encry isn't string.";
		json_object_put(root);
		return -1;

	}
	encry = json_object_get_string(_encry);
	
	//target_id
	json_object *_target_id = NULL;
	if(!json_object_object_get_ex(root, "target_id", &_target_id))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no target_id, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no target_id.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_target_id, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, target_id isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, target_id isn't int.";
		json_object_put(root);
		return -1;
	}
	target_id = (unsigned long long)json_object_get_int64(_target_id);

	//params
	json_object *_params = NULL;
	if(!json_object_object_get_ex(root, "params", &_params))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no params, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no params.";
		json_object_put(root);
		return -1;
	}
	
	//msg_type
	json_object *_msg_type = NULL;
	if(!json_object_object_get_ex(_params, "msg_type", &_msg_type))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg_type, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no msg_type.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_msg_type, json_type_string))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, msg_type isn't string, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, msg_type isn't string.";
		json_object_put(root);
		return -1;

	}
	msg_type = json_object_get_string(_msg_type);
	if(!((msg_type == MT_P2P) || (msg_type == MT_P2R) || (msg_type == MT_P2F) || (msg_type == MT_R2P) || 
	   (msg_type == MT_R2F) || (msg_type == MT_BROADCAST) || (msg_type == MT_BROADCAST_APP) || (msg_type == MT_BROADCAST_ROUTER)))
	{
		XCP_LOGGER_INFO(&g_logger, "invalid msg type, req(%u):%s\n", req.size(), req.c_str());
		err_info = "invalid msg type.";
		json_object_put(root);
		return -1;
	}


	//sys_method
	json_object *_sys_method = NULL;
	if(!json_object_object_get_ex(_params, "sys_method", &_sys_method))
	{
		sys_method = "";
	}
	else
	{
		if(!json_object_is_type(_sys_method, json_type_string))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, sys_method isn't string, req(%u):%s\n", req.size(), req.c_str());
			err_info = "it is invalid req, sys_method isn't string.";
			json_object_put(root);
			return -1;
		
		}
		sys_method = json_object_get_string(_sys_method);
	}	


	//添加目标svr 信息
	json_object_object_add(root, "to_svr", json_object_new_string(g_sysInfo._log_id.c_str()));
	req = json_object_get_string(root);
	
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
	if(!json_object_is_type(_code, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, code isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, code isn't int.";
		json_object_put(root);
		return -1;
	}	
	int code = json_object_get_int(_code);

	
	//msg
	json_object *_msg = NULL;
	if(!json_object_object_get_ex(root, "msg", &_msg))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no msg, req(%u):%s\n", req.size(), req.c_str());
		err_info = "";
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
		err_info = json_object_get_string(_msg);
	}

	//释放内存
	json_object_put(root);
	
	return code;

}




//判断用户是否可以聊天
std::string XProtocol::check_talk_condition_req(const std::string &msg_tag, const std::string &from_svr, const std::string &uuid, const std::string &encry, const std::string &session_id,
	const unsigned long long src_id, const unsigned long long target_id, const std::string &msg_type)
{
	char msg[2018] = {0};
	snprintf(msg, 2018, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"from_svr\":\"%s\", \"msg_uuid\":\"%s\", \"msg_encry\":\"%s\", \"session_id\":\"%s\", \"real_id\":%llu, \"params\":{\"target_id\":%llu, \"msg_type\":\"%s\"}}\n", 
		CMD_CHECK_TALK_CONDITION.c_str(), getTimestamp(), msg_tag.c_str(), from_svr.c_str(), uuid.c_str(), encry.c_str(), session_id.c_str(), src_id, target_id, msg_type.c_str());

	return msg;

}



//获取家庭成员id 列表
std::string XProtocol::get_member_id_list_req(const std::string &msg_tag, const std::string &from_svr, const std::string &uuid, const std::string &encry, const std::string &session_id,
	const unsigned long long user_id, const unsigned long long family_id)
{
	char msg[2018] = {0};
	snprintf(msg, 2018, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\",  \"from_svr\":\"%s\", \"msg_uuid\":\"%s\", \"msg_encry\":\"%s\", \"session_id\":\"%s\", \"real_id\":%llu, \"params\":{\"family_id\":%llu}}\n", 
		CMD_GET_MEMBER_ID_LIST.c_str(), getTimestamp(), msg_tag.c_str(), from_svr.c_str(), uuid.c_str(), encry.c_str(), session_id.c_str(), user_id, family_id);

	return msg;
}



int XProtocol::get_member_id_list_rsp(const std::string &rsp, std::set<unsigned long long> &members, std::string &err_info)
{
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(rsp.c_str(), &jerr);
	if(jerr != json_tokener_success)
	{
		XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, rsp:%s, err:%s\n", 
			rsp.c_str(), json_tokener_error_desc(jerr));
		err_info = "it is invlid json req.";
		return -1;
	}


	json_object *_result = NULL;
	if(!json_object_object_get_ex(root, "result", &_result))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no result, rsp(%u):%s\n", rsp.size(), rsp.c_str());
		err_info = "it is invalid req, no result.";
		json_object_put(root);
		return -1;
	}


	json_object *_member_list = NULL;
	if(!json_object_object_get_ex(_result, "member_list", &_member_list))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no member_list, rsp(%u):%s\n", rsp.size(), rsp.c_str());
		err_info = "it is invalid req, no member_list.";
		json_object_put(root);
		return -1;
	}

	
	//判断数组类型
	if(!json_object_is_type(_member_list, json_type_array))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, member_list isn't array, rsp(%u):%s\n", rsp.size(), rsp.c_str());
		err_info = "it is invalid req, member_list isn't array.";
		json_object_put(root);
		return -1;
	}
	
	std::string member_one = "";
	int size = json_object_array_length(_member_list);
	for(int i=0; i<size; i++) 
	{
		unsigned long long member_id = 0;
			
		json_object *_member_one = json_object_array_get_idx(_member_list, i);
		if(!json_object_is_type(_member_one, json_type_int))
		{
			XCP_LOGGER_INFO(&g_logger, "it is invalid req, member isn't int, rsp(%u):%s\n", rsp.size(), rsp.c_str());
			err_info = "it is invalid req, member isn't int.";
			json_object_put(root);
			return -1;
		}
		member_id = (unsigned long long)json_object_get_int64(_member_one);

		members.insert(member_id);
	}

	//释放内存
	json_object_put(root);
	
	return 0;

}




std::string XProtocol::notify_client_status_req(const std::string &msg_tag, unsigned long long id, const std::string &type, const std::string &status)
{
	char msg[512] = {0};
	snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"real_id\":0, \"params\":{\"method\":\"%s\", \"client_id\":%llu, \"client_type\":\"%s\", \"status\":\"%s\"}}\n", 
		CMD_PUSH_MSG.c_str(), getTimestamp(), msg_tag.c_str(), CMD_SYN_CLIENT_STATUS.c_str(), id, type.c_str(), status.c_str());

	return msg;
}




//判断客户端是否在线
int XProtocol::check_client_online_req(std::string &req, unsigned long long &client_id, std::string &err_info)
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
	json_object *_params = NULL;
	if(!json_object_object_get_ex(root, "params", &_params))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no params, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no params.";
		json_object_put(root);
		return -1;
	}
	
	//client_id
	json_object *_client_id = NULL;
	if(!json_object_object_get_ex(_params, "client_id", &_client_id))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, no client_id, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, no client_id.";
		json_object_put(root);
		return -1;
	}
	if(!json_object_is_type(_client_id, json_type_int))
	{
		XCP_LOGGER_INFO(&g_logger, "it is invalid req, client_id isn't int, req(%u):%s\n", req.size(), req.c_str());
		err_info = "it is invalid req, client_id isn't int.";
		json_object_put(root);
		return -1;
	}
	client_id = (unsigned long long)json_object_get_int64(_client_id);
	
	//释放内存
	json_object_put(root);

	return 0;

}


