
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

#include "family_mgt.h"

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
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't string, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
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
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't unsigned integer, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
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
            errInfo.append("it is invalid req, ").append(name).append(" isn't string type"); \
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
        if(!node.IsObject()) \
        { \
            XCP_LOGGER_INFO(&g_logger, "it is invalid req, %s isn't json type, req(%u):%s\n", name.c_str(), req.size(), req.c_str()); \
            errInfo.append("it is invalid req, ").append(name).append(" isn't string type"); \
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
            errInfo.append("it is invalid req, ").append(name).append(" isn't string type"); \
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

std::string XProtocol::get_push_req(const std::string &msg_tag, const std::string &method, const uint64 userId, const uint64 familyId, const uint64 operatorId)
{
    char msg[512] = {0};
    snprintf(msg, 512, "{\"method\":\"%s\", \"timestamp\":%llu, \"req_id\":0, \"msg_tag\":\"%s\", \"params\":{\"method\":\"%s\", \"family_id\":%llu, \"user_id\":%llu, \"operater_id\":%llu}}\n", 
        CMD_PUSH_MSG.c_str(), getTimestamp(), msg_tag.c_str(), method.c_str(), familyId, userId, operatorId);
    return msg;
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


std::string XProtocol::add_family_result(const unsigned long long familyId, const std::string &token)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    //snprintf(buf, 512, "{\"family_id\":%llu, \"token\": \"%s\"}", familyId, token.c_str());
    snprintf(buf, 512, "{\"family_id\":%llu}", familyId);

    return buf;
}

std::string XProtocol::get_family_info_result(const StFamilyInfo &info)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"family_id\":%llu, \"family_name\":\"%s\", \"family_avatar\":\"%s\",\"owner_id\":%llu, \"router_id\":%llu,\"created_at\":%llu,\"updated_at\":%llu}", 
        info.familyId, info.familyName.c_str(), info.familyAvatar.c_str(), info.ownerId, info.routerId, info.createdAt, info.updateAt);

    return buf;
}

std::string XProtocol::get_apply_code_result(const std::string &code, const uint64 createAt, const uint64 expireAt)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"code\":\"%s\", \"created_at\":%llu, \"expire_at\":%llu}", 
        code.c_str(), createAt, expireAt);

    return buf;
}

std::string XProtocol::get_creat_member_result(const std::string &token)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"token\": \"%s\"}", token.c_str());

    return buf;
}

std::string XProtocol::bind_router_result(const uint64 routerId)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"router_id\": %llu}", routerId);

    return buf;
}


std::string XProtocol::get_family_list_result(const std::vector<StFamilyInfo> &list)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::vector<StFamilyInfo>::const_iterator end = list.end();
    for(std::vector<StFamilyInfo>::const_iterator it = list.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        result << get_family_info_result(*it);
        isFirstElement = false;
    }
    
    result << "] }";
    
    return result.str();
}

std::string XProtocol::get_apply_list_result(const std::vector<StMemberApplyInfo> &list)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::vector<StMemberApplyInfo>::const_iterator end = list.end();
    for(std::vector<StMemberApplyInfo>::const_iterator it = list.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ", ";
        }
        
        StMemberApplyInfo info = *it;
        result << "{\"apply_id\": " << info.applyId
            << ", \"user_id\": " << info.userId
            << ", \"status\": " << info.state
            << ", \"created_at\": " << info.createdAt
            << ", \"updated_at\": " << info.updateAt
            << "}" ;
        isFirstElement = false;
    }
    
    result << "] }";
    
    return result.str();
}

std::string XProtocol::get_apply_conut_result(const unsigned int count)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"count\":%llu}", count);

    return buf;
}


std::string XProtocol::get_member_info_result(const StFamilyMemberInfo &info)
{
    char buf[512] = {0};
    memset(buf, 0x0, 512); 
    snprintf(buf, 512, "{\"user_id\":%llu, \"role\":%u, \"user_name\":\"%s\", \"nick_name\":\"%s\",\"gender\":%u,\"created_at\":%llu,\"updated_at\":%llu}", 
        info.memberId, info.role, info.name.c_str(), info.nickName.c_str(), info.gender, info.createdAt, info.updateAt);

    return buf;
}

std::string XProtocol::get_member_list_result(const std::vector<StFamilyMemberInfo> &list)
{
    std::ostringstream result;
    result << "{\"list\":[";

    bool isFirstElement = true;
    std::vector<StFamilyMemberInfo>::const_iterator end = list.end();
    for(std::vector<StFamilyMemberInfo>::const_iterator it = list.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        result << get_member_info_result(*it);
        isFirstElement = false;
    }
    
    result << "] }";
    
    return result.str();
}

std::string XProtocol::get_member_id_list_result(const std::vector<unsigned long long> &list, const uint64 routerId)
{
    std::ostringstream result;
    result << "{\"member_list\":[";

    bool isFirstElement = true;
    std::vector<unsigned long long>::const_iterator end = list.end();
    for(std::vector<unsigned long long>::const_iterator it = list.begin(); it != end; it++)
    {
        if(!isFirstElement)
        {
            result << ",";
        }
        result << *it;
        isFirstElement = false;
    }
    
    result << "], \"router_id\": " << routerId <<"}";
    
    return result.str();
}


