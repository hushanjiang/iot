
#include "protocol.h"
#include "base/base_time.h"
#include "base/base_string.h"
#include "base/base_convert.h"
#include "base/base_logger.h"
#include "base/base_base64.h"
#include "comm/common.h"

#include "rapidjson/document.h"        // rapidjson's DOM-style API  
#include "rapidjson/prettywriter.h"    // for stringify JSON  
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

extern Logger g_logger;

#define GET_JSON_STRING_NODE(father, name, result, errInfo, ifNeed) \
{\
    errInfo = ""; \
    if((father).HasMember(name.c_str())) \
    { \
        const rapidjson::Value &node = (father)[name.c_str()]; \
        if(!node.IsString()) \
        { \
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't integer, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
            errInfo.append("it is invalid req, ").append(name).append(" isn't string type"); \
            return -1; \
        } \
        result = node.GetString(); \
    } \
    else if(ifNeed) \
    { \
        XCP_LOGGER_ERROR(&g_logger, "cannot find %s of req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
        errInfo.append("it is invalid req, cannot find ").append(name); \
        return -1; \
    } \
}

#define GET_JSON_INTEGER_NODE(father, name, result, errInfo, ifNeed) \
{\
    errInfo = ""; \
    if((father).HasMember(name.c_str())) \
    { \
        const rapidjson::Value &node = (father)[name.c_str()]; \
        if(!node.IsInt()) \
        { \
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't integer, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
            errInfo.append("it is invalid req, ").append(name).append(" isn't integer type"); \
            return -1; \
        } \
        result = node.GetInt(); \
    } \
    else if(ifNeed) \
    { \
        XCP_LOGGER_ERROR(&g_logger, "cannot find %s of req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
        errInfo.append("it is invalid req, cannot find ").append(name); \
        return -1; \
    } \
}

#define GET_JSON_UNSIGNED_INTEGER_NODE(father, name, result, errInfo, ifNeed) \
{\
    errInfo = ""; \
    if((father).HasMember(name.c_str())) \
    { \
        const rapidjson::Value &node = (father)[name.c_str()]; \
        if(!node.IsUint()) \
        { \
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't integer, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
            errInfo.append("it is invalid req, ").append(name).append(" isn't unsigned integer type"); \
            return -1; \
        } \
        result = node.GetUint(); \
    } \
    else if(ifNeed) \
    { \
        XCP_LOGGER_ERROR(&g_logger, "cannot find %s of req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
        errInfo.append("it is invalid req, cannot find ").append(name); \
        return -1; \
    } \
}


#define GET_JSON_LONG_LONG_NODE(father, name, result, errInfo, ifNeed) \
{\
    errInfo = ""; \
    if((father).HasMember(name.c_str())) \
    { \
        const rapidjson::Value &node = (father)[name.c_str()]; \
        if(!node.IsUint64()) \
        { \
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't integer, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
            errInfo.append("it is invalid req, ").append(name).append(" isn't unsigned long long type"); \
            return -1; \
        } \
        result = node.GetUint64(); \
    } \
    else if(ifNeed) \
    { \
        XCP_LOGGER_ERROR(&g_logger, "cannot find %s of req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
        errInfo.append("it is invalid req, cannot find ").append(name); \
        return -1; \
    } \
}

#define GET_JSON_CHILD_NODE(father, name, node, errInfo, ifNeed) \
{\
    errInfo = ""; \
    if((father).HasMember(name.c_str())) \
    { \
        node = (father)[name.c_str()]; \
        if(!node.IsObject() && !node.IsArray()) \
        { \
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't json type, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
            errInfo.append("it is invalid req, ").append(name).append(" isn't json type"); \
            return -1; \
        } \
    } \
    else if(ifNeed) \
    { \
        XCP_LOGGER_ERROR(&g_logger, "cannot find %s of req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
        errInfo.append("it is invalid req, cannot find ").append(name); \
        return -1; \
    } \
}

#define GET_JSON_ARRAY_NODE(father, name, node, errInfo, ifNeed) \
{\
    errInfo = ""; \
    if((father).HasMember(name.c_str())) \
    { \
        node = (father)[name.c_str()]; \
        if(!node.IsArray()) \
        { \
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't array type, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
            errInfo.append("it is invalid req, ").append(name).append(" isn't array type"); \
            return -1; \
        } \
    } \
    else if(ifNeed) \
    { \
        XCP_LOGGER_ERROR(&g_logger, "cannot find %s of req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
        errInfo.append("it is invalid req, cannot find ").append(name); \
        return -1; \
    } \
}

int XProtocol::req_head(const std::string &req, std::string &method, unsigned long long &timestamp, 
    unsigned int &req_id, unsigned long long &real_id, std::string &msg_tag, std::string &msg_encry, 
    std::string &msg_uuid, std::string &session_id,	std::string &err_info)
{
    err_info = "";
    std::string nodeName;

    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 

    //method
    nodeName = "method";
    GET_JSON_STRING_NODE(root, nodeName, method, err_info, true);

    //timestamp
    nodeName = "timestamp";
    GET_JSON_LONG_LONG_NODE(root, nodeName, timestamp, err_info, true);
    
    //req_id
    nodeName = "req_id";
    GET_JSON_UNSIGNED_INTEGER_NODE(root, nodeName, req_id, err_info, true);	

    //real_id
    nodeName = "real_id";
    GET_JSON_LONG_LONG_NODE(root, nodeName, real_id, err_info, false);

    //msg_tag
    nodeName = "msg_tag";
    GET_JSON_STRING_NODE(root, nodeName, msg_tag, err_info, true);

    //session_id
    nodeName = "session_id";
    GET_JSON_STRING_NODE(root, nodeName, session_id, err_info, false);

    //msg_uuid
    nodeName = "msg_uuid";
    GET_JSON_STRING_NODE(root, nodeName, msg_uuid, err_info, false);

    //msg_encry
    nodeName = "msg_encry";
    GET_JSON_STRING_NODE(root, nodeName, msg_encry, err_info, false);
    
    return 0;	
}

std::string XProtocol::rsp_msg(const std::string &method, const unsigned int req_id, const std::string &msg_tag,  
        const std::string &msg_encry, const std::string &msg_uuid, const std::string &session_id,
        const int code, const std::string &msg, const std::string &body, bool is_object)
{	
    std::ostringstream result;
    result << "{\"method\":\"" << method <<"\""
        << ", \"timestamp\":" << getTimestamp()
        << ", \"req_id\":" << req_id
        << ", \"msg_tag\":\"" << msg_tag << "\""
        << ", \"code\":" << code
        << ", \"msg\":\"" << msg << "\"";
    if(msg_encry != "")
    {
        result << ", \"msg_encry\":\"" << msg_encry << "\"";
    }
    if(msg_uuid != "")
    {
        result << ", \"msg_uuid\":\"" << msg_uuid << "\"";
    }
    if(session_id != "")
    {
        result << ", \"session_id\":\"" << session_id << "\"";
    }
    if(body != "")
    {
        if(is_object)
        {
            result << ", \"result\":" << body;
        }
        else
        {
            result << ", \"result\":\"" << body << "\"";
        }
    }
    result << "}\n";
    return result.str();
}

int XProtocol::admin_head(const std::string &req,
    std::string &method, unsigned long long &timestamp, unsigned long long &realId,
    std::string &msg_tag, int &code, std::string &msg, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 

    //method
    std::string nodeName = "method";
    GET_JSON_STRING_NODE(root, nodeName, method, err_info, true);

    //timestamp
    nodeName = "timestamp";
    GET_JSON_LONG_LONG_NODE(root, nodeName, timestamp, err_info, true);
    
    //real_id
    nodeName = "real_id";
    GET_JSON_LONG_LONG_NODE(root, nodeName, realId, err_info, false);

    //msg_tag
    nodeName = "msg_tag";
    GET_JSON_STRING_NODE(root, nodeName, msg_tag, err_info, true);

    //code
    nodeName = "code";
    GET_JSON_INTEGER_NODE(root, nodeName, code, err_info, true);
    
    //msg
    nodeName = "msg";
    GET_JSON_STRING_NODE(root, nodeName, msg, err_info, false);
    
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
    snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\"}\n", 
        CMD_GET_SERVER_ACCESS.c_str(), getTimestamp(), msg_id.c_str(), ST_UID.c_str());
    
    return msg;
}

int XProtocol::get_server_access_rsp(const std::string &req, std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info)
{
    err_info = "";
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 

    //code
    std::string nodeName = "code";
    int code = 0;
    GET_JSON_INTEGER_NODE(root, nodeName, code, err_info, true);
    if(code != 0)
    {
        XCP_LOGGER_INFO(&g_logger, "get server access failed, req(%u):%s\n", req.size(), req.c_str());
        return code;
    }

    //result
    nodeName = "result";	
    rapidjson::Value result;
    GET_JSON_CHILD_NODE(root, nodeName, result, err_info, true);

    //uid svr
    if(result.HasMember(ST_UID.c_str()))
    {
        const rapidjson::Value &_uid = result[ST_UID.c_str()];
        if(!_uid.IsArray())
        {
              XCP_LOGGER_INFO(&g_logger, "it is invalid req, uid svr isn't array, req(%u):%s\n", req.size(), req.c_str());
            err_info = "it is invalid req, uid svr isn't array.";
            return -1;
        }

        std::vector<StSvr> vec_svr;
        for (Value::ConstValueIterator itr = _uid.Begin(); itr != _uid.End(); ++itr)
        {
            if(!itr->IsObject())
            {
                XCP_LOGGER_INFO(&g_logger, "it is invalid req, uid svr element isn't json object, req(%u):%s\n", req.size(), req.c_str());
                err_info = "it is invalid req, uid svr isn't array.";
                return -1;
            }
            
            StSvr svr;
            //ip
            nodeName = "ip";
            GET_JSON_STRING_NODE(*itr, nodeName, svr._ip, err_info, true);

            //port
            nodeName = "port";
            GET_JSON_INTEGER_NODE(*itr, nodeName, svr._port, err_info, true);

            vec_svr.push_back(svr);
        }
        svrs.insert(std::make_pair(ST_UID, vec_svr));
    }
        
    //push svr
    if(result.HasMember(ST_PUSH.c_str()))
    {
        const rapidjson::Value &_push = result[ST_PUSH.c_str()];
        if(!_push.IsArray())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, push svr isn't array, req(%u):%s\n", req.size(), req.c_str());
              err_info = "it is invalid req, uid svr isn't array.";
            return -1;
        }

        std::vector<StSvr> vec_svr;
        for (Value::ConstValueIterator itr = _push.Begin(); itr != _push.End(); ++itr)
        {
            if(!itr->IsObject())
            {
                XCP_LOGGER_INFO(&g_logger, "it is invalid req, uid svr element isn't json object, req(%u):%s\n", req.size(), req.c_str());
                err_info = "it is invalid req, uid svr isn't array.";
                return -1;
            }
            
            StSvr svr;
            //ip
            nodeName = "ip";
            GET_JSON_STRING_NODE(*itr, nodeName, svr._ip, err_info, true);

            //port
            nodeName = "port";
            GET_JSON_INTEGER_NODE(*itr, nodeName, svr._port, err_info, true);

            vec_svr.push_back(svr);
        }
        svrs.insert(std::make_pair(ST_PUSH, vec_svr));
    }
    
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
    
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 

    //result
    std::string nodeName = "result";
    rapidjson::Value result;
    GET_JSON_CHILD_NODE(root, nodeName, result, err_info, true);

    //uuid
    nodeName = "uuid";
    GET_JSON_LONG_LONG_NODE(result, nodeName, uuid, err_info, true);	

    return 0;
}

int XProtocol::get_rsp_result(const std::string &req, std::string &err_info)
{
    std::string msg = "";
    
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 

    //code
    std::string nodeName = "code";
    int code = 0;
    GET_JSON_INTEGER_NODE(root, nodeName, code, err_info, true);

    //msg
    nodeName = "msg";
    GET_JSON_STRING_NODE(root, nodeName, msg, err_info, false);
    err_info = msg;
    
    return code;
}

int XProtocol::get_special_params(const std::string &req, const std::string &paramName, std::string &result, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    GET_JSON_STRING_NODE(params, paramName, result, err_info, true);

    XCP_LOGGER_INFO(&g_logger, "get param %s success, result:%s\n", paramName.c_str(), result.c_str());
    
    return 0;
}

int XProtocol::get_special_params(const std::string &req, const std::string &paramName, unsigned int &result, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);
    
    GET_JSON_UNSIGNED_INTEGER_NODE(params, paramName, result, err_info, true);

    XCP_LOGGER_INFO(&g_logger, "get param %s success, result:%u\n", paramName.c_str(), result);
    
    return 0;
}

int XProtocol::get_special_params(const std::string &req, const std::string &paramName, unsigned long long &result, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {
        XCP_LOGGER_ERROR(&g_logger, "rapid json parse failed, req:%s\n", req.c_str());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    GET_JSON_LONG_LONG_NODE(params, paramName, result, err_info, true);
    XCP_LOGGER_INFO(&g_logger, "get param %s success, result:%llu\n", paramName.c_str(), result);
    
    return 0;
}

int XProtocol::get_json_object_params(const std::string &req, const std::string &paramName, std::string &result, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    rapidjson::Value paramEle;
    GET_JSON_CHILD_NODE(params, paramName, paramEle, err_info, true);
    
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    paramEle.Accept(writer);
    
    result = buffer.GetString();
    XCP_LOGGER_INFO(&g_logger, "get param %s success, result:%s\n", paramName.c_str(), result.c_str());
    
    return 0;		
}

int XProtocol::get_room_infos(const std::string &req, std::vector<StRoomInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, list element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }

        StRoomInfo info;
        nodeName = "room_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.id, err_info, true);
        nodeName = "user_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.creatorId, err_info, true);
        nodeName = "is_default";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.isDefault, err_info, true);
        nodeName = "order";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.orderNum, err_info, true);
        nodeName = "room_name";
        GET_JSON_STRING_NODE(*itr, nodeName, info.name, err_info, true);
        nodeName = "create_at";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.createAt, err_info, true);
        nodeName = "update_at";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.updateAt, err_info, true);

        infos.push_back(info);
    }
    return 0;
}

int XProtocol::get_room_update_infos(const std::string &req, std::vector<StRoomInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, list element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }

        StRoomInfo info;
        nodeName = "room_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.id, err_info, true);
        nodeName = "room_name";
        GET_JSON_STRING_NODE(*itr, nodeName, info.name, err_info, false);
        infos.push_back(info);
    }
    return 0;
}

int XProtocol::get_device_infos(const std::string &req, std::vector<StDeviceInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, list element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        StDeviceInfo info;
        nodeName = "device_uuid";
        GET_JSON_STRING_NODE(*itr, nodeName, info.deviceId, err_info, true);
        nodeName = "device_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.id, err_info, true);
        nodeName = "bussiness_user_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.businessId, err_info, true);
        nodeName = "device_category_id";
        int category_id = 0;
        GET_JSON_INTEGER_NODE(*itr, nodeName, category_id, err_info, true);
        info.productId = (enum DM_DEVICE_CATEGORY)category_id;
        nodeName = "device_name";
        GET_JSON_STRING_NODE(*itr, nodeName, info.name, err_info, true);

        if(itr->HasMember("attribute"))
        {
            const rapidjson::Value &paramAttr = (*itr)["attribute"];
            if(!paramAttr.IsObject() && !paramAttr.IsArray())
            {
                XCP_LOGGER_INFO(&g_logger, "it is invalid req, attribute isn't json type, req(%u):%s\n", req.size(), req.c_str());
                return -1;
            }             
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            paramAttr.Accept(writer);
            info.attr = buffer.GetString();
        }
        else
        {
            XCP_LOGGER_ERROR(&g_logger, "cannot find attribute of req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }

        nodeName = "room_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.roomId, err_info, true);
        nodeName = "user_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.creatorId, err_info, true);
        nodeName = "create_at";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.createAt, err_info, true);
        nodeName = "update_at";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.updateAt, err_info, true);

        infos.push_back(info);
    }
    return 0;
}

int XProtocol::get_device_update_infos(const std::string &req, std::vector<StDeviceInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, list element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }

        StDeviceInfo info;
        nodeName = "device_uuid";
        GET_JSON_STRING_NODE(*itr, nodeName, info.deviceId, err_info, true);
        nodeName = "device_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.id, err_info, true);
        nodeName = "bussiness_user_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.businessId, err_info, false);
        nodeName = "device_category_id";
        int category_id = 0;
        GET_JSON_INTEGER_NODE(*itr, nodeName, category_id, err_info, false);
        info.productId = (enum DM_DEVICE_CATEGORY)category_id;
        nodeName = "device_name";
        GET_JSON_STRING_NODE(*itr, nodeName, info.name, err_info, false);

        if(itr->HasMember("attribute"))
        {
            const rapidjson::Value &paramAttr = (*itr)["attribute"];
            if(!paramAttr.IsObject() && !paramAttr.IsArray())
            {
                XCP_LOGGER_INFO(&g_logger, "it is invalid req, attribute isn't json type, req(%u):%s\n", req.size(), req.c_str());
                return -1;
            }             
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            paramAttr.Accept(writer);
            info.attr = buffer.GetString();
        }

        nodeName = "room_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.roomId, err_info, false);
        nodeName = "user_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.creatorId, err_info, false);
        nodeName = "update_at";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.updateAt, err_info, false);

        infos.push_back(info);
    }
    return 0;
}

int XProtocol::get_device_status(const std::string &req, std::vector<StDeviceStatus> &statues, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, list element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        StDeviceStatus statue;
        nodeName = "device_uuid";
        GET_JSON_STRING_NODE(*itr, nodeName, statue.device_uuid, err_info, true);
        nodeName = "device_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, statue.deviceId, err_info, true);
        nodeName = "device_category_id";
        int category_id = 0;
        GET_JSON_INTEGER_NODE(*itr, nodeName, category_id, err_info, true);
        statue.type = (enum DM_DEVICE_CATEGORY)category_id;

        if(itr->HasMember("attribute"))
        {
            const rapidjson::Value &paramAttr = (*itr)["attribute"];
            if(!paramAttr.IsObject() && !paramAttr.IsArray())
            {
                XCP_LOGGER_INFO(&g_logger, "it is invalid req, attribute isn't json type, req(%u):%s\n", req.size(), req.c_str());
                return -1;
            }             
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            paramAttr.Accept(writer);
            statue.msg = buffer.GetString();
        }
        else
        {
            XCP_LOGGER_ERROR(&g_logger, "cannot find attribute of req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        nodeName = "create_at";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, statue.createAt, err_info, true);

        statues.push_back(statue);
    }
    return 0;
}

int XProtocol::get_shortcut_infos(const std::string &req, std::vector<StShortCutInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, list element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        StShortCutInfo info;
        nodeName = "room_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.roomId, err_info, true);
        nodeName = "shortcut_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.id, err_info, true);
        nodeName = "name";
        GET_JSON_STRING_NODE(*itr, nodeName, info.name, err_info, true);
        nodeName = "order";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.order, err_info, true);
        nodeName = "device_category_id";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.flag, err_info, true);

        if(itr->HasMember("content"))
        {
            const rapidjson::Value &paramAttr = (*itr)["content"];
            if(!paramAttr.IsObject() && !paramAttr.IsArray())
            {
                XCP_LOGGER_INFO(&g_logger, "it is invalid req, attribute isn't json type, req(%u):%s\n", req.size(), req.c_str());
                return -1;
            }             
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            paramAttr.Accept(writer);
            info.data = buffer.GetString();
        }
        else
        {
            XCP_LOGGER_ERROR(&g_logger, "cannot find attribute of req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        infos.push_back(info);
    }
    return 0;
}

int XProtocol::get_shortcut_update_infos(const std::string &req, std::vector<StShortCutInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, list element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        StShortCutInfo info;
        nodeName = "room_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.roomId, err_info, true);
        nodeName = "shortcut_id";
        GET_JSON_LONG_LONG_NODE(*itr, nodeName, info.id, err_info, true);
        nodeName = "name";
        GET_JSON_STRING_NODE(*itr, nodeName, info.name, err_info, false);
        nodeName = "order";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.order, err_info, false);
        nodeName = "device_category_id";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.flag, err_info, false);

        if(itr->HasMember("content"))
        {
            const rapidjson::Value &paramAttr = (*itr)["content"];
            if(!paramAttr.IsObject() && !paramAttr.IsArray())
            {
                XCP_LOGGER_INFO(&g_logger, "it is invalid req, attribute isn't json type, req(%u):%s\n", req.size(), req.c_str());
                return -1;
            }             
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            paramAttr.Accept(writer);
            info.data = buffer.GetString();
        }
        infos.push_back(info);
    }
    return 0;
}

int XProtocol::get_id_lists(const std::string &req, std::vector<unsigned long long> &ids, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value list;
    GET_JSON_ARRAY_NODE(params, nodeName, list, err_info, true);

    for (Value::ConstValueIterator itr = list.Begin(); itr != list.End(); ++itr)
    {
        if(!itr->IsUint64())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, id isn't integer, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        uint64 id = itr->GetUint64(); 

        ids.push_back(id);
    }
    return 0;
}

int XProtocol::get_page_params(const std::string &req, unsigned int &size, unsigned int &beginAt, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "page";
    rapidjson::Value page;
    GET_JSON_CHILD_NODE(params, nodeName, page, err_info, true);

    nodeName = "size";
    GET_JSON_INTEGER_NODE(page, nodeName, size, err_info, true);

    nodeName = "begin";
    GET_JSON_INTEGER_NODE(page, nodeName, beginAt, err_info, true);
    
    XCP_LOGGER_INFO(&g_logger, "get page success, size:%u, begin:%u\n", size, beginAt);
    
    return 0;		
}

int XProtocol::get_room_orders_params(const std::string &req, std::vector<StRoomInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value orders;
    GET_JSON_ARRAY_NODE(params, nodeName, orders, err_info, true);
    
    for (Value::ConstValueIterator itr = orders.Begin(); itr != orders.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, orders element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }

        StRoomInfo info;
        nodeName = "room_id";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.id, err_info, true);
        
        nodeName = "order";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.orderNum, err_info, true);

        infos.push_back(info);
    }
    
    XCP_LOGGER_INFO(&g_logger, "get order list success, size:%u\n", infos.size());
    return 0;
}

int XProtocol::get_shortcut_orders_params(const std::string &req, std::vector<StShortCutInfo> &infos, std::string &err_info)
{
    rapidjson::Document root;
    root.Parse(req.c_str());
    if (root.HasParseError() || !root.IsObject())  
    {  
        XCP_LOGGER_INFO(&g_logger, "json_tokener_parse_verbose failed, req:%s, err:%s\n", 
            req.c_str(), root.GetParseError());
        err_info = "it is invlid json req.";
        return -1;
    } 
    
    //params
    std::string nodeName = "params";
    rapidjson::Value params;
    GET_JSON_CHILD_NODE(root, nodeName, params, err_info, true);

    nodeName = "list";
    rapidjson::Value orders;
    GET_JSON_ARRAY_NODE(params, nodeName, orders, err_info, true);
    
    for (Value::ConstValueIterator itr = orders.Begin(); itr != orders.End(); ++itr)
    {
        if(!itr->IsObject())
        {
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, orders element isn't json object, req(%u):%s\n", req.size(), req.c_str());
            return -1;
        }
        
        StShortCutInfo info;
        nodeName = "shortcut_id";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.id, err_info, true);
        
        nodeName = "order";
        GET_JSON_INTEGER_NODE(*itr, nodeName, info.order, err_info, true);

        infos.push_back(info);
    }
    
    XCP_LOGGER_INFO(&g_logger, "get order list success, size:%u\n", infos.size());
    return 0;
}

std::string XProtocol::process_failed_result(const std::vector<unsigned long long> &failed)
{
    if(failed.empty())
    {
        return "";
    }
    std::ostringstream result;

    result << "{\"failed\":[";
    std::vector<unsigned long long>::const_iterator end = failed.end();
    for (std::vector<unsigned long long>::const_iterator it = failed.begin(); it != end; it++)
    {
        if(it != failed.begin())
        {
            result << ",";
        }
        result << *it;
    }
    result << "] }";

    return result.str();
}

std::string XProtocol::process_failed_result(const std::vector<std::string> &failed)
{
    if(failed.empty())
    {
        return "";
    }
    std::ostringstream result;

    result << "{\"failed\":[";
    std::vector<std::string>::const_iterator end = failed.end();
    for (std::vector<std::string>::const_iterator it = failed.begin(); it != end; it++)
    {
        if(it != failed.begin())
        {
            result << ",";
        }
        result << "\"" << *it << "\"";
    }
    result << "] }";

    return result.str();
}

std::string XProtocol::add_device_result(const unsigned long long &deviceId)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"device_id\":%llu}", deviceId);

    return buf;
}

std::string XProtocol::add_shortcut_result(const unsigned long long &shortcutId)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"shutcut_id\":%llu}", shortcutId);

    return buf;
}

std::string XProtocol::get_room_info_result(const StRoomInfo &info)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"room_id\":%llu, \"family_id\":%llu, \"room_name\": \"%s\", \"order\":%u, \"created_at\":%llu, \"updated_at\":%llu}",
        info.id, info.familyId, info.name.c_str(), info.orderNum, info.createAt, info.updateAt);

    return buf;
}

std::string XProtocol::get_device_info_result(const StDeviceInfo &info)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"device_id\":%llu, \"device_uuid\":\"%s\", \"family_id\": \"%llu\", \"room_id\": \"%llu\", \"device_category_id\":%llu, \"device_name\":\"%s\", \"attribute\":%s, \"created_at\":%llu, \"updated_at\":%llu}",
        info.id, info.deviceId.c_str(), info.familyId, info.roomId, info.productId, info.name.c_str(), info.attr.c_str(), info.createAt, info.updateAt);

    return buf;
}

std::string XProtocol::get_shortcut_info_result(const StShortCutInfo &info)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 	
    snprintf(buf, 512, "{\"shortcut_id\":%llu, \"family_id\":%llu, \"room_id\":%llu, \"name\": \"%s\", \"device_category_id\":%u, \"order\":%u, \"content\": %s}",
        info.id, info.familyId, info.roomId, info.name.c_str(), info.flag, info.order, info.data.c_str());

    return buf;
}

std::string XProtocol::get_device_status_result(const StDeviceStatus &status)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"report_id\":%llu, \"device_category_id\":%u, \"device_id\": %llu, \"device_id\": \"%s\", \"family_id\": %llu, \"report_msg\":%s, \"report_created_at\":%llu}",
        status.id, status.type, status.deviceId, status.device_uuid.c_str(), status.familyId, status.msg.c_str(), status.createAt);

    return buf;
}

std::string XProtocol::get_device_alert_result(const StDeviceAlert &alert)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"alert_id\":%llu, \"alert_type_id\":%u, \"device_id\": \"%llu\", \"family_id\": \"%llu\", \"alert_msg\":\"%s\", \"alert_status\":%u, \"report_created_at\":%llu, \"alert_handler_at\": %llu}",
        alert.id, alert.type, alert.deviceId, alert.familyId, alert.msg.c_str(), alert.status, alert.createAt, alert.handleAt);

    return buf;
}

std::string XProtocol::get_router_auth_result(const uint64 &routerId)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"router_id\":%llu}", routerId);

    return buf;
}

std::string XProtocol::get_router_info_result(const StRouterInfo &info)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    /*{"router_id": 13, "router_uuid": "test123123", "family_id": 1, "name": "", "sn_nuber": "11",
         "mac_address": "11-11-11-11", "state": 1, "device_attr_ext":"", "created_at" : 123456 }*/	
    snprintf(buf, 512, "{\"router_id\":%llu, \"router_uuid\":\"%s\", \"family_id\":%llu, \"name\":\"%s\", \"sn_nuber\":\"%s\", \"mac_address\":\"%s\", \"state\":%llu, \"device_attr_ext\":\"%s\", \"created_at\":%llu}",
        info.routerId, info.uuid.c_str(), info.familyId, info.name.c_str(), info.snNo.c_str(), info.mac.c_str(), info.state, info.attr.c_str(), info.creatAt);
    
    return buf;
}

std::string XProtocol::get_user_account_result(const std::vector<std::string> &tokens, const std::vector<unsigned long long> &failed)
{
    std::ostringstream result;

    if(tokens.empty() && failed.empty())
    {
        return "";
    }
    result << "{";
    if(!tokens.empty())
    {
        result << "\"tokens\":[";
        std::vector<std::string>::const_iterator end = tokens.end();
        for (std::vector<std::string>::const_iterator it = tokens.begin(); it != end; it++)
        {
            if(it != tokens.begin())
            {
                result << ",";
            }
            result << "\"" << *it << "\"";
        }
        result << "]";
    }

    if(!failed.empty())
    {
        if(!tokens.empty())
        {
            result << ",";
        }
        result << "\"failed\":[";
        std::vector<unsigned long long>::const_iterator end = failed.end();
        for (std::vector<unsigned long long>::const_iterator it = failed.begin(); it != end; it++)
        {
            if(it != failed.begin())
            {
                result << ",";
            }
            result << *it;
        }
        result << "]";
    }
    result << "}";

    return result.str();
}

std::string XProtocol::get_room_list_result(const std::vector<StRoomInfo> &infos)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::vector<StRoomInfo>::const_iterator end = infos.end();
    for(std::vector<StRoomInfo>::const_iterator it = infos.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        result << get_room_info_result(*it);
        isFirstElement = false;
    }
    
    result << "] }";
    
    return result.str();
}

std::string XProtocol::get_device_list_result(const std::vector<StDeviceInfo> &infos, const unsigned int &left)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::vector<StDeviceInfo>::const_iterator end = infos.end();
    for(std::vector<StDeviceInfo>::const_iterator it = infos.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        result << get_device_info_result(*it);
        isFirstElement = false;
    }
    
    result << "], \"more\":" << left << " }";
    
    return result.str();
}

std::string XProtocol::get_shortcut_list_result(const std::vector<StShortCutInfo> &infos)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::vector<StShortCutInfo>::const_iterator end = infos.end();
    for(std::vector<StShortCutInfo>::const_iterator it = infos.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        result << get_shortcut_info_result(*it);
        isFirstElement = false;
    }
    
    result << "] }";
    
    return result.str();
}

std::string XProtocol::get_status_list_result(const std::deque<std::string> &status, const unsigned int &left)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::deque<std::string>::const_iterator end = status.end();
    for(std::deque<std::string>::const_iterator it = status.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        // result << get_device_status_result(*it);
        result << *it;
        isFirstElement = false;
    }
    
    result << "], \"more\":" << left << " }";
    
    return result.str();
}

std::string XProtocol::get_alert_list_result(const std::vector<StDeviceAlert> &alerts, const unsigned int &left)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::vector<StDeviceAlert>::const_iterator end = alerts.end();
    for(std::vector<StDeviceAlert>::const_iterator it = alerts.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        result << get_device_alert_result(*it);
        isFirstElement = false;
    }
    
    result << "], \"more\":" << left << " }";
    
    return result.str();
}
