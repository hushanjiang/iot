
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "family_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;
extern Conn_Mgt_LB g_push_conn;

#define CHECK_RET(nRet)	\
{ \
	if(nRet != 0) \
	{ \
		XCP_LOGGER_ERROR(&g_logger, "check return failed: %d.\n", nRet); \
		return nRet; \
	} \
}

const std::string REDIS_KEY_FAMILY_INFO_ID		= "id";
const std::string REDIS_KEY_FAMILY_INFO_NAME	= "name";
const std::string REDIS_KEY_FAMILY_INFO_AVATAR	= "avatar";
const std::string REDIS_KEY_FAMILY_INFO_OWNER	= "owner";
const std::string REDIS_KEY_FAMILY_INFO_CREATE	= "created";
const std::string REDIS_KEY_FAMILY_INFO_UPDATE	= "update";

const std::string REDIS_FAMILY_INFO_PRE_HEAD	= "Family:family_";
const std::string REDIS_FAMILY_ROUTER_PRE_HEAD	= "FamilyBind:family_";
const std::string REDIS_FAMILY_LIST_PRE_HEAD	= "UserInFamily:user_";
const std::string REDIS_MEMBER_LIST_PRE_HEAD	= "FamilyAllUser:family_";
const std::string REDIS_FAMILY_CODE_PRE_HEAD	= "FamilyCode:family_";


const unsigned int REDIS_KEY_MAX_LENGTH			= 50;
const unsigned int RANDON_STRING_MAX_LENGTH		= 25;
const unsigned int TOKEN_NONCE_STRING_LENGTH	= 8;
const unsigned int PUSH_MSG_TAG_LENGTH			= 12;
const unsigned int FAMILY_CODE_EXPIRE_SECOND	= 180;

const unsigned int FAMILY_INVITITION_TOKEN_TTL	= 3 * 24 * 3600;

Family_Mgt::Family_Mgt()
{

}


Family_Mgt::~Family_Mgt()
{

}

int Family_Mgt::create_family(const uint64 &userId, const std::string &msg_tag, const StFamilyInfo &info, std::string &token)
{
	// 不需要校验权限
	if(_set_FamilyInfo_to_sql(info) != 0)
	{
		return ERR_GET_MYSQL_CONN_FAILED;
	}

	// 家庭信息写入缓存，忽略失败
	if(_set_FamilyInfo_to_Redis(info))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family(%d) info to cach", userId, info.familyId);
	}

	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	std::ostringstream sql;
	MySQL_Row_Set row_set;
	// 获取出户主信息，用于app同步给路由器
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql << "select F_phone_num, F_password, F_salt from tbl_user_info_" << (userId % 10)
		<< " where F_uid = " << userId << ";";
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get user(%llu) info from sql.\n", userId);
		return -1;
	}

/*	std::string errInfo;
	TokenInfo _token;
	_token.user_id	= userId;
	_token.phone	= row_set[0][0];
	_token.pwd		= row_set[0][1];
	_token.salt		= row_set[0][2];
	_token.token_type	= TokenTypeGetAccount;
	nRet = _generate_token(_token.pwd, _token, token, errInfo);
	CHECK_RET(nRet);
	*/
	return 0;
}

/* "params":{"user_id":123, "family_id":123, "family_name":"", "family_avatar":"" } */
int Family_Mgt::update_family(const uint64 &userId, const std::string &msg_tag, const StFamilyInfo &info)
{
	// 只有户主可以操作
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(info.familyId, members, ownerId);
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}
	
	
	// 写入数据库
	if(_update_FamilyInfo_to_sql(info) != 0)
	{
		return ERR_GET_MYSQL_CONN_FAILED;
	}

	// 刷新缓存，如果失败清除缓存，可以查不到缓存，但是不能查到错误信息
	if(_update_FamilyInfo_to_Redis(info))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot update the family(%d) info to cach", userId, info.familyId);
		_remove_FamilyInfo_from_Redis(info.familyId);
	}

	// 家庭信息更新，需要同步给家庭的用户
	_push_msg(CMD_SYN_UPDATE_FAMILY, msg_tag, userId, info.familyId, userId);
	
	return 0;
}

int Family_Mgt::member_switch_family(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId)
{
	// 切换家庭，前提是用户需要加入家庭
	if(!_is_user_joined_family(userId,familyId))
	{
		XCP_LOGGER_INFO(&g_logger, "cannot allow user(%llu) switch to family(%llu), must join the family first.\n", 
			userId, familyId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}

	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	// update tbl_user_info_0 set F_last_family_id = 444 where F_uid = 111110;
	std::ostringstream sql;
	sql << "update tbl_user_info_"
		<< (userId % 10)
		<< " set F_last_family_id = " << familyId
		<< " where F_uid = " << userId
		<< ";";

	uint64 last_insert_id = 0;
    uint64 affected_rows = 0;
    int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
    return 0;
}

int Family_Mgt::get_family_info(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, StFamilyInfo &result)
{
	// 不需要检查权限

	int nRet = _get_router_id_by_family(familyId, result.routerId);
	CHECK_RET(nRet);

	// 查询缓存，命中，刷新超时时间，返回
	if(_get_FamilyInfo_from_Redis(familyId,result) == 0)
	{
		return 0;
	}
	
	// 未命中缓存，查询数据库，如果查询数据库失败，说明id不存在
	if(_get_FamilyInfo_from_sql(familyId,result) != 0)
	{
		return ERR_AUTHENTIONCATION_BEGIN;
	}
	
	// 写入缓冲，忽略失败
	if(_set_FamilyInfo_to_Redis(result))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family(%d) info to cach", familyId);
	}

	return 0;	
}

int Family_Mgt::get_family_list(const uint64 &userId, const std::string &msg_tag, std::vector<StFamilyInfo> &result)
{
	// 不需要查询用户权限
	
	std::vector<uint64> familyIds;
	_get_family_ids_by_user(userId, familyIds);
	
	_muti_get_family_info(familyIds, result);	
	return 0;
}

int Family_Mgt::bind_router(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, uint64 &routerId, const std::string &uuid, const std::string &pwd)
{
	XCP_LOGGER_INFO(&g_logger, "start bind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);

	// 检查是不是户主操作
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	MySQL_Row_Set row_set;
	uint64 last_insert_id = 0, affected_rows = 0;
	std::ostringstream sql;
	
	// 检查路由器信息是否正确
	/* select F_rid from tbl_router_map where F_device_id = '1'; */
	sql.str("");
	sql << "select F_rid from tbl_router_map where F_device_id = '" << uuid << "';";
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find uuid:%s in database\n", uuid.c_str());
		return -1;
	}
	routerId = (uint64)atoi(row_set[0][0].c_str());
	/* select F_device_auth_key,F_device_id,F_family_id from tbl_router_0 where F_router_id = 1; */
	sql.str("");
	sql << "select F_device_auth_key,F_device_id,F_family_id  from tbl_router_" << routerId % 10
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(pwd != row_set[0][0] || uuid != row_set[0][1])
	{
		XCP_LOGGER_ERROR(&g_logger, "pwd or uuid is not correct, %s <---> %s, %s <---> %s\n",
			pwd.c_str(), row_set[0][0].c_str(), uuid.c_str(), row_set[0][1].c_str());
		//XCP_LOGGER_ERROR(&g_logger, "pwd or uuid is not correct\n");
		return -1;
	}
	
	// 如果路由器已经绑定过，需要先解绑路由器，不能重复绑定
	uint64 bindFamilyId =  (uint64)atoi(row_set[0][2].c_str());
	if(bindFamilyId == familyId)
	{
		XCP_LOGGER_INFO(&g_logger, "the family have bind this router before\n", userId, familyId, routerId);
		return 0;
	}
	if(bindFamilyId != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "router have been bind for family:%llu\n", bindFamilyId);
		return -1;
	}

	// 更新家庭表
	/* update tbl_family_0 set F_router_id = 1 where F_family_id = 10; */
	sql.str("");
	sql << "update tbl_family_" << (familyId % 10)
		<< " set F_router_id = " << routerId
		<< " where F_family_id = " << familyId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);

	// 更新路由器信息表
	/* update tbl_router_0 set F_family_id = 2013 where F_router_id = 1001; */
	sql.str("");
	sql << "update tbl_router_" << (routerId % 10)
		<< " set F_family_id = " << familyId
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);

	// 如果路由器有设备，将所有路由器下的设备至为有效
	/* update tbl_device_info_1 set F_device_state = 1 where F_router_id = 0; */
	sql.str("");
	sql << "update tbl_device_info_" << (routerId % 10)
		<< " set F_device_state = 0 where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);

	// 刷新缓存
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_ROUTER_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;
	
	PSGT_Redis_Mgt->set_int(redis_key, routerId);
	
	return 0;
}
	
int Family_Mgt::unbind_router(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId)
{
	uint64 routerId = 0;
	_get_router_id_by_family(familyId, routerId);	
	XCP_LOGGER_INFO(&g_logger, "start unbind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);

	// 检查是不是户主操作；检查家庭是不是绑定的这个路由器
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}
		
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	std::ostringstream sql;
	uint64 last_insert_id = 0, affected_rows = 0;
	
	/* update tbl_family_0 set F_router_id = 1 where F_family_id = 10; */
	sql.str("");
	sql << "update tbl_family_" << (familyId % 10)
		<< " set F_router_id = 0 where F_family_id = " << familyId
		<< ";";
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	
	// 更新路由器信息表
	/* update tbl_router_0 set F_family_id = 0 where F_router_id = 1001; */
	sql.str("");
	sql << "update tbl_router_" << (routerId % 10)
		<< " set F_family_id = 0 where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);

	// 家庭内所有设备至为无效
	/* update tbl_device_info_1 set F_device_state = 1 where F_router_id = 1; */
	sql.str("");
	sql << "update tbl_device_info_" << (routerId % 10)
		<< " set F_device_state = 1 where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);

	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_ROUTER_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	// 删除缓存
	PSGT_Redis_Mgt->set_int(redis_key, 0);
	
	return 0;
}

int Family_Mgt::get_apply_code(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &code, uint64 &timestamp)
{
	// 检查是不是户主操作；检查家庭是不是绑定的这个路由器
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}

	code = _rand_str(RANDON_STRING_MAX_LENGTH);
	timestamp = getTimestamp();

	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_CODE_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	// 超时时间180秒，考虑网络延迟，另多加5秒
	if( PSGT_Redis_Mgt->set_string(redis_key, code, FAMILY_CODE_EXPIRE_SECOND + 5) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family code to cach", familyId);
		return -1;
	}

	XCP_LOGGER_INFO(&g_logger, "generate family code success.\n", familyId);
	return 0;
}


/**
  *1. 如果用户是家庭内的普通成员，那么有可能是APP同步添加成员到路由器失败，需要重新生成token
  *2. 如果用户是家庭的户主，那么不允许操作
  *3. 如果用户不属于家庭，则将成员添加到家庭，且生成token
**/
int Family_Mgt::create_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const std::string &code, std::string &token)
{
	// 检查code是否正确
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_CODE_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf, real_code;
	if( PSGT_Redis_Mgt->get_string(redis_key, real_code) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot get the family code to cach", familyId);
		return -1;
	}
	if(code != real_code)
	{
		XCP_LOGGER_ERROR(&g_logger, "invite code not correct, %s <---> %s", code.c_str(), real_code.c_str());
		return -1;
	}
	// 如果邀请码正确，需要立刻清除
	XCP_LOGGER_INFO(&g_logger, "invite code correct, now it will be removed because it can use only once\n");
	PSGT_Redis_Mgt->remove(redis_key);
	
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	if(userId == ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "family owner cannot invite himself to join\n");
		return -1;
	}
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	std::ostringstream sql;
	MySQL_Row_Set row_set;
	uint64 timeStamp = getTimestamp(), last_insert_id = 0, affected_rows = 0;

	// 查询用户的基本信息
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql << "select F_phone_num, F_password, F_salt from tbl_user_info_" << (userId % 10)
		<< " where F_uid = " << userId << ";";
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get user(%llu) info from sql.\n", userId);
		return -1;
	}

	// 查询出户主的md5密钥
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql.str("");
	sql << "select F_password from tbl_user_info_" << (ownerId % 10)
		<< " where F_uid = " << ownerId<< ";";
	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1);
	CHECK_RET(nRet);
	
	if(row_set1.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get owner(%llu) info from sql.\n", ownerId);
		return -1;
	}
	
	std::vector<uint64>::iterator it = std::find(members.begin(), members.end(), userId);
	if(it != members.end())
	{
		XCP_LOGGER_INFO(&g_logger, "user have included in the family, will generate invition again\n");
	}
	else
	{
		// insert into tbl_family_member_0 (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values (11, 11, 1, 11111,11111);
		sql.str("");
		sql << "insert into tbl_family_member_"
			<< (familyId % 10)
			<< " (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values ("
			<< familyId <<", " << userId <<", " << FAMILY_ROLE_MEMBER <<", " << timeStamp <<", " << timeStamp
			<< ");";
		
		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
		
		_set_family_add_new_member_to_Redis(familyId, userId);
		
		// insert into tbl_user_family_0 (F_uid, F_family_id) values (123, 123);
		sql.str("");
		sql << "insert into tbl_user_family_"
			<< (userId % 10)
			<< " (F_uid, F_family_id) values ("
			<< userId <<", " << familyId
			<< ");";
	    nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	    CHECK_RET(nRet);
		
		_set_user_joined_new_family_to_Redis(userId, familyId);
	}

	std::string errInfo;
	TokenInfo _token;
	_token.user_id	= userId;
	_token.phone	= row_set[0][0];
	_token.pwd		= row_set[0][1];
	_token.salt		= row_set[0][2];
	_token.token_type	= TokenTypeInviteMember;
	nRet = _generate_token(row_set1[0][0], _token, token, errInfo);
	CHECK_RET(nRet);

	// 同步，忽略失败
	nRet = _push_msg(CMD_SYN_ADD_MEMBER, msg_tag, userId, familyId, ownerId);

	return 0;
}

int Family_Mgt::get_invitation(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &token)
{
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	std::vector<uint64>::iterator it = std::find(members.begin(), members.end(), userId);
	if(it == members.end())
	{
		XCP_LOGGER_ERROR(&g_logger, "this user donot include in family\n");
		return -1;
	}
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	std::ostringstream sql;
	MySQL_Row_Set row_set;
	// 查询用户的基本信息
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql << "select F_phone_num, F_password, F_salt from tbl_user_info_" << (userId % 10)
		<< " where F_uid = " << userId << ";";
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get user(%llu) info from sql.\n", userId);
		return -1;
	}

	// 查询出户主的md5密钥
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql.str("");
	sql << "select F_password from tbl_user_info_" << (ownerId % 10)
		<< " where F_uid = " << ownerId<< ";";
	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1);
	CHECK_RET(nRet);
	
	if(row_set1.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get owner(%llu) info from sql.\n", ownerId);
		return -1;
	}
	std::string errInfo;
	TokenInfo _token;
	_token.user_id	= userId;
	_token.phone	= row_set[0][0];
	_token.pwd		= row_set[0][1];
	_token.salt		= row_set[0][2];
	_token.token_type	= TokenTypeInviteMember;
	nRet = _generate_token(row_set1[0][0], _token, token, errInfo);
	CHECK_RET(nRet);
	return 0;
}

int Family_Mgt::member_apply_join(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId)
{
	// 获取该用户加入该家庭申请记录:
	// 1. 如果已申请，但是未审批，则忽略此次请求，刷新"updateAt"；
	// 2. 如果已申请，且已拒绝或同意，则请求有效，入库保存；
	// 3. 如果未申请过，请求有效，入库保存。
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	uint64 timeStamp = getTimestamp();

	// 分成两步，先查记录，再通过判断来更新或插入
	/* select count(*) from tbl_family_apply_0 where F_uid = 111 and F_family_id = 111 and F_state = 0 */
	uint64 last_insert_id = 0;
    uint64 affected_rows = 0;
	std::ostringstream sql;
	sql << "select count(*) from tbl_family_apply_"
		<< (familyId % 10)
		<< " where F_uid = " << userId
		<< " and F_family_id = " << familyId
		<< " and F_state = 0;";
		
	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);
    CHECK_RET(nRet);
	
	int count = atoi(row_set[0][0].c_str());
	
	// 如果可以查到记录，只更新"update_At"字段
	if(count >= 1)
	{
		sql.str("");
		// 更新时间
		/* update tbl_family_apply_0 set F_updated_at = 12345 where F_uid = 111 and F_family_id = 111 and F_state = 0; */
		sql << "update tbl_family_apply_"
		<< (familyId % 10)
		<< " set F_updated_at = " << timeStamp
		<< " where F_uid = " << userId
		<< " and F_family_id = " << familyId
		<< " and F_state = 0;";
		
	    nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
		return 0;
	}

	sql.str("");
	/* insert into tbl_family_apply_0 (F_uid, F_family_id, F_created_at, F_updated_at) values (11, 1111, 123, 234); */
	sql << "insert into tbl_family_apply_"
		<< (familyId % 10)
		<< " (F_uid, F_family_id, F_created_at, F_updated_at) values ("
		<< userId << "," << familyId << "," << timeStamp << "," << timeStamp
		<< ");";
	
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	return 0;	
}

int Family_Mgt::accept_member_join(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, 
								const uint64 &applyId, enum member_join_apply_result applyResult)
{
	// 只允许户主操作
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	uint64 last_insert_id = 0;
    uint64 affected_rows = 0;

	// 修改申请单据，applyId对应单据处理必须是未处理状态，不可以重复处理
	// update tbl_family_apply_3 set F_state = 1 where F_id = 1 and F_state = 0;
	std::ostringstream sql;
	sql << "update tbl_family_apply_"
		<< (familyId % 10)
		<< " set F_state = " << applyResult
		<< " where F_id = " << applyId
		<< " and F_state = 0;";
	
    int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
    CHECK_RET(nRet);
	
	// 如果接受加入，修改家庭成员表，修改用户家庭列表
	if(applyResult == MEMBER_JOIN_ACCEPT && affected_rows >= 1)
	{
		// select F_uid from tbl_family_apply_7 where F_id = 7;
		sql.str("");
		MySQL_Row_Set row_set;
		sql << "select F_uid from tbl_family_apply_"
			<< (familyId % 10)
			<< " where F_id = " << applyId
			<< ";";
		
		nRet = conn->query_sql(sql.str(), row_set);
		CHECK_RET(nRet);
		
		uint64 memberId = (uint64)(atoi(row_set[0][0].c_str()));
		XCP_LOGGER_INFO(&g_logger, "member id is %llu for apply id %llu\n", memberId, applyId);

		uint64 timeStamp = getTimestamp();
		// insert into tbl_family_member_0 (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values (11, 11, 1, 11111,11111);
		sql.str("");
		sql << "insert into tbl_family_member_"
			<< (familyId % 10)
			<< " (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values ("
			<< familyId <<", " << memberId <<", " << FAMILY_ROLE_MEMBER <<", " << timeStamp <<", " << timeStamp
			<< ");";
		
		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
		
		_set_family_add_new_member_to_Redis(familyId, memberId);
		
		// insert into tbl_user_family_0 (F_uid, F_family_id) values (123, 123);
		sql.str("");
		sql << "insert into tbl_user_family_"
			<< (memberId % 10)
			<< " (F_uid, F_family_id) values ("
			<< memberId <<", " << familyId
			<< ");";
	    nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	    CHECK_RET(nRet);
		
		_set_user_joined_new_family_to_Redis(memberId, familyId);
	}

	// 同步，忽略失败
	nRet = _push_msg(CMD_SYN_ADD_MEMBER, msg_tag, userId, familyId, ownerId);
	
	return 0;
}

int Family_Mgt::get_family_apply_number(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, unsigned int &count)
{
	// 不需要检查权限
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	/* select count(*) from tbl_family_apply_3 where F_family_id = 2013 and F_state = 0; */
	std::ostringstream sql;
	MySQL_Row_Set row_set;
	sql << "select count(*) from tbl_family_apply_"
		<< (familyId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_state = 0;";

	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);

	count = (uint64)atoi(row_set[0][0].c_str());
	return 0;
}


int Family_Mgt::get_family_apply_list(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
												std::vector<StMemberApplyInfo> &result)
{
	// 不需要检查权限
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	/* select F_id,F_uid,F_state,F_created_at,F_updated_at from tbl_family_apply_0 where F_family_id = 1111; */
	std::ostringstream sql;
	MySQL_Row_Set row_set;
	sql << "select F_id,F_uid,F_state,F_created_at,F_updated_at from tbl_family_apply_"
		<< (familyId % 10)
		<< " where F_family_id = "
		<< familyId
		<< ";";

	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "query family list for user(%llu), get zero record.\n", userId);
		return 0;
	}

	for(int i=0; i<row_set.row_count(); i++)
	{
		StMemberApplyInfo info;
		info.applyId = (uint64)atoi(row_set[i][0].c_str());
		info.userId = (uint64)atoi(row_set[i][1].c_str());
		info.state = (enum member_join_apply_result)(atoi(row_set[i][2].c_str()) % MEMBER_JOIN_END);
		info.createdAt = (uint64)atoi(row_set[i][3].c_str());
		info.updateAt= (uint64)atoi(row_set[i][4].c_str());
		
		result.push_back(info);
	}
	
	XCP_LOGGER_INFO(&g_logger, "query family list for user(%llu) sucess. total number:%d\n", userId, row_set.row_count());
	
	return nRet;
}

int Family_Mgt::remove_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const uint64 &memberId)
{
	// 1、只允许户主操作；2、不能删除自己；
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can remove family member\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}
	if(userId == memberId)
	{
		XCP_LOGGER_ERROR(&g_logger, "you cannot remove yourself, because you are the family owner\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	uint64 last_insert_id = 0;
    uint64 affected_rows = 0;

	// delete from tbl_family_member_0 where F_family_id = 11 and F_uid = 11;
	std::ostringstream sql;
	sql << "delete from tbl_family_member_"
		<< (familyId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_uid = " << memberId
		<< ";";
    int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	
	_set_family_remove_member_to_Redis(memberId, familyId);

	// delete from tbl_user_family_0 where F_family_id = 11 and F_uid = 11;
	sql.str("");
	sql << "delete from tbl_user_family_"
		<< (memberId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_uid = " << memberId
		<< ";";

    nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	
	_set_user_quit_family_to_Redis(memberId, familyId);

	// 同步，忽略失败
	nRet = _push_msg(CMD_SYN_DEL_MEMBER, msg_tag, memberId, familyId, userId);
	
	return 0;
}

int Family_Mgt::update_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
						const uint64 &memberId, const std::string &memberName)
{
	// 只允许户主操作；
	uint64 ownerId = 0;
	std::vector<uint64> members;
	_get_members_by_family(familyId, members, ownerId);
	if(userId != ownerId || userId != memberId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) or himself can update family info\n", ownerId);
		return ERR_AUTHENTIONCATION_BEGIN;
	}
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	uint64 last_insert_id = 0;
    uint64 affected_rows = 0;
	uint64 timeStamp = getTimestamp();

	// update tbl_family_member_0 set F_nickname = 'adfa',F_updated_at = 123 where F_family_id = 11 and F_uid = 111;
	std::ostringstream sql;
	sql << "update tbl_family_member_"
		<< (familyId % 10)
		<< "  set F_nickname = '" << memberName << "', "
		<< "F_updated_at = " << timeStamp
		<< " where F_family_id = " << familyId
		<< " and F_uid = " << memberId
		<< ";";
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	
	// 同步，忽略失败
	nRet = _push_msg(CMD_SYN_UPDATE_MEMBER, msg_tag, memberId, familyId, userId);

	return 0;
}

int Family_Mgt::get_member_info(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
						const uint64 memberId, StFamilyMemberInfo &result)
{
	// 不需要判断权限
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	
	MySQL_Row_Set row_set;
	/* select um.F_name,um.F_gender,fm.F_nickname,fm.F_role_id,fm.F_created_at,fm.F_updated_at
		from tbl_family_member_0 fm,tbl_user_info_0 um
		where fm.F_family_id = 111 and fm.F_uid = 111 and um.F_uid = 111; */
	std::ostringstream sql;
	sql << "select um.F_name,um.F_gender,fm.F_nickname,fm.F_role_id,fm.F_created_at,fm.F_updated_at"
		<< " from tbl_family_member_" << (familyId % 10)
		<< " fm,tbl_user_info_" << (memberId % 10)
		<< " um where fm.F_family_id = " << familyId
		<< " and fm.F_uid = " << memberId
		<< " and um.F_uid = " << memberId
		<< ";";
	
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "query family list for user(%llu), get zero record.\n", userId);
		return -1;
	}

	result.memberId = memberId;
	result.name = row_set[0][0];
	result.gender= (enum member_gender)(atoi(row_set[0][1].c_str()) % GENDER_END);
	result.nickName = row_set[0][2];
	result.role = (enum member_role_in_family)(atoi(row_set[0][3].c_str()) % FAMILY_ROLE_END);
	result.createdAt = atoi(row_set[0][4].c_str());
	result.updateAt = atoi(row_set[0][5].c_str());
	
	return 0;
}

int Family_Mgt::get_member_info_list(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
												std::vector<StFamilyMemberInfo> &result)
{
	// 不需要判断权限
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	
	MySQL_Row_Set row_set;
	/* select F_uid,F_nickname,F_role_id,F_created_at,F_updated_at from tbl_family_member_0 where F_family_id = 111; */
	std::ostringstream sql;
	sql << "select F_uid,F_nickname,F_role_id,F_created_at,F_updated_at from tbl_family_member_"
		<< (familyId % 10)
		<< " where F_family_id = " << familyId
		<< ";";
	
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "query family list for user(%llu), get zero record.\n", userId);
		return 0;
	}

	for(int i=0; i<row_set.row_count(); i++)
	{
		StFamilyMemberInfo info;
		info.memberId = atoi(row_set[i][0].c_str());
		info.nickName = row_set[i][1];
		info.role = (enum member_role_in_family)(atoi(row_set[i][2].c_str()) % FAMILY_ROLE_END);
		info.createdAt = atoi(row_set[i][3].c_str());
		info.updateAt = atoi(row_set[i][4].c_str());

		result.push_back(info);
	}
	
	XCP_LOGGER_INFO(&g_logger, "query family list for user(%llu) sucess. total number:%d\n", userId, row_set.row_count());
	
	return 0;
}

int Family_Mgt::get_member_id_list(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
								std::vector<uint64> &result, uint64 &routerId)
{
	// 不需要判断权限 
	
	_get_router_id_by_family(familyId, routerId);

	uint64 ownerId;
	_get_members_by_family(familyId, result, ownerId);
	
	return 0;
}

int Family_Mgt::check_talk_condition(const uint64 srcId, const uint64 targetId, const std::string &msg_tag, enum talk_msg_type type, std::string &errInfo)
{
	int nRet = 0;
	errInfo = "success";
	
	switch(type)
	{
		case TALK_MSG_TYPE_P2P:
			// 用户到用户，判断是否属于同一个家庭
			{
				std::vector<uint64> srcFamilyIds, targetFamilyIds;
				_get_family_ids_by_user(srcId, srcFamilyIds);
				_get_family_ids_by_user(targetId, targetFamilyIds);

				nRet = -1;
				errInfo = "this two user donot include in a public family";
				std::vector<uint64>::iterator end = srcFamilyIds.end();
				for(std::vector<uint64>::iterator it = srcFamilyIds.begin(); it != end; it++)
				{
					std::vector<uint64>::iterator inter = std::find(targetFamilyIds.begin(), targetFamilyIds.end(), *it);
					if(inter != targetFamilyIds.end())
					{
						XCP_LOGGER_INFO(&g_logger, "get public family(%llu) for two users\n", *it);
						errInfo = "success";
						nRet = 0;
						break;
					}
				}
			}
			break;
		case TALK_MSG_TYPE_P2R:
			// 判断用户所属于的家庭列表，有没有绑定过该路由器
			{
				std::vector<uint64> srcFamilyIds;
				_get_family_ids_by_user(srcId, srcFamilyIds);

				nRet = -1;
				errInfo = "the user's family donot bind this router";
				std::vector<uint64>::iterator end = srcFamilyIds.end();
				for(std::vector<uint64>::iterator it = srcFamilyIds.begin(); it != end; it++)
				{
					uint64 routerId = 0;
					_get_router_id_by_family(*it, routerId);
					if(routerId == targetId)
					{
						XCP_LOGGER_INFO(&g_logger, "get family(%llu) bind this router\n", *it);
						errInfo = "success";
						nRet = 0;
						break;
					}
				}
			}
			break;
		case TALK_MSG_TYPE_P2F:
			// 判断用户是否属于家庭
			if(!_is_user_joined_family(srcId, targetId))
			{
				XCP_LOGGER_ERROR(&g_logger, "user(%llu) is not join family(%llu)\n", srcId, targetId);
				nRet = -1;
				errInfo = "the user donot join this family";
			}
			break;
		case TALK_MSG_TYPE_R2P:
			// 判断用户所属于的家庭列表，有没有绑定过该路由器，跟TALK_MSG_TYPE_P2R逻辑一样
			{
				std::vector<uint64> targetFamilyIds;
				_get_family_ids_by_user(targetId, targetFamilyIds);

				nRet = -1;
				errInfo = "router bind family donot have this uer";
				std::vector<uint64>::iterator end = targetFamilyIds.end();
				for(std::vector<uint64>::iterator it = targetFamilyIds.begin(); it != end; it++)
				{
					uint64 routerId = 0;
					_get_router_id_by_family(*it, routerId);
					if(routerId == srcId)
					{
						XCP_LOGGER_INFO(&g_logger, "get family(%llu) bind this router\n", *it);
						errInfo = "success";
						nRet = 0;
						break;
					}
				}
			}
			break;
		case TALK_MSG_TYPE_R2F:
			// 查看家庭是否绑定该路由器
			{
				uint64 routerId = 0;
				_get_router_id_by_family(targetId, routerId);
				if(routerId != srcId)
				{
					XCP_LOGGER_ERROR(&g_logger, "family bind router is(%llu)\n", routerId);
					nRet = -1;
					errInfo = "router donot bind this family";
				}
			}
			break;
		default:
			errInfo = "unkown msg type";
			nRet = ERR_INVALID_REQ;
			break;
	}
	return nRet;
}


bool Family_Mgt::_is_user_joined_family(const uint64 userId, const uint64 familyId)
{
	std::vector<uint64> familyIds;
	_get_family_ids_by_user(userId, familyIds);
	
	std::vector<uint64>::iterator it = std::find(familyIds.begin(), familyIds.end(), familyId);
	if(it != familyIds.end())
	{
		return true;
	}

	return false;
}

int Family_Mgt::_get_router_id_by_family(const uint64 familyId, uint64 &routerId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_ROUTER_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;
	
	if(PSGT_Redis_Mgt->exists(redis_key))
	{
		PSGT_Redis_Mgt->get_int(redis_key, routerId);
		XCP_LOGGER_INFO(&g_logger, "get router(%llu) bind family(%llu) from redis.\n", routerId, familyId);
		return 0;
	}	
	XCP_LOGGER_INFO(&g_logger, "query redis of family(%s) failed.\n", redis_key.c_str());
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	/* select F_uid from tbl_family_member_0 where F_family_id = 1111; */
	std::ostringstream sql;
	sql << "select F_router_id from tbl_family_" << (familyId % 10)
		<< " where F_family_id = " << familyId	<< ";";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);	
	CHECK_RET(nRet);

	routerId = (uint64)(atoi(row_set[0][0].c_str()));

	PSGT_Redis_Mgt->set_int(redis_key, routerId);

	return 0;
}

int Family_Mgt::_get_members_by_family(const uint64 familyId, std::vector<uint64> &members, uint64 &ownerId)
{
	// 查询缓存，命中，刷新超时时间，返回
	if(_get_family_all_user_list_from_Redis(familyId, members, ownerId) == 0)
	{
		return 0;
	}
	
	// 未命中缓存，查询数据库，如果查询数据库失败，说明id不存在
	if(_get_family_all_user_list_from_sql(familyId, members, ownerId) != 0)
	{
		return ERR_FM_FAMILY_ID_IS_NOT_EXIST;
	}
	
	// 写入缓冲，忽略失败
	if(_set_family_all_user_list_to_Redis(familyId, members, ownerId))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family(%d) member list to cach", familyId);
	}
	
	return 0;
}

int Family_Mgt::_get_family_ids_by_user(const uint64 userId, std::vector<uint64> &familyIds)
{
	// 查询缓存，命中，刷新超时时间，返回
	if(_get_user_joined_family_list_from_Redis(userId, familyIds) == 0)
	{
		return 0;
	}
	
	// 未命中缓存，查询数据库
	if(_get_user_joined_family_list_from_sql(userId, familyIds) != 0)
	{
		//如果查询数据库失败，返回异常
		return ERR_FM_FAMILY_ID_IS_NOT_EXIST;
	}
	// 写入缓冲，忽略失败
	if(_set_user_joined_family_list_to_Redis(userId, familyIds))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family(%d) list to cach", userId);
	}

	return 0;
}


int Family_Mgt::_muti_get_family_info(const std::vector<uint64> &familyIds, std::vector<StFamilyInfo> &result)
{
	int nRet = 0;
	result.clear();
	
	std::vector<uint64>::const_iterator end = familyIds.end();
	for(std::vector<uint64>::const_iterator it = familyIds.begin(); it != end; it++)
	{
		StFamilyInfo info;
		if(_get_FamilyInfo_from_Redis(*it,info) != 0)
		{
			// 查询缓存失败，查数据库，写缓存
			if(_get_FamilyInfo_from_sql(*it,info) != 0)
			{
				// 查数据库失败，一般不会发生，有则跳过这段记录
				XCP_LOGGER_ERROR(&g_logger, "user join family(%llu), but family id is not exist.\n", *it);
				continue;
			}
			_set_FamilyInfo_to_Redis(info);
		}
		nRet = _get_router_id_by_family(*it, info.routerId);
		CHECK_RET(nRet);
		
		result.push_back(info);
	}
	return 0;
}


int Family_Mgt::_get_FamilyInfo_from_Redis(const uint64 &familyId, StFamilyInfo &result)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_INFO_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	result.familyId = familyId;
	
	if( PSGT_Redis_Mgt->hget_string(redis_key, REDIS_KEY_FAMILY_INFO_NAME, result.familyName) == 0
		&&  PSGT_Redis_Mgt->hget_string(redis_key, REDIS_KEY_FAMILY_INFO_AVATAR, result.familyAvatar) == 0
		&&  PSGT_Redis_Mgt->hget_int(redis_key, REDIS_KEY_FAMILY_INFO_OWNER, result.ownerId) == 0
		&&  PSGT_Redis_Mgt->hget_int(redis_key, REDIS_KEY_FAMILY_INFO_CREATE, result.createdAt) == 0
		&&  PSGT_Redis_Mgt->hget_int(redis_key, REDIS_KEY_FAMILY_INFO_UPDATE, result.updateAt) == 0 )
	{
		// 刷新超时时间
		std::string err_info;
		PSGT_Redis_Mgt->refresh_security_channel(redis_key, err_info);
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "query redis of family(%s) failed.\n", redis_key.c_str());
	return -1;
}

int Family_Mgt::_set_FamilyInfo_to_Redis(const StFamilyInfo &info)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_INFO_PRE_HEAD.c_str(), info.familyId);
	std::string redis_key = buf;
	
	if( PSGT_Redis_Mgt->hset_string(redis_key, REDIS_KEY_FAMILY_INFO_NAME, info.familyName) == 0
		&&  PSGT_Redis_Mgt->hset_string(redis_key, REDIS_KEY_FAMILY_INFO_AVATAR, info.familyAvatar) == 0
		&&  PSGT_Redis_Mgt->hset_int(redis_key, REDIS_KEY_FAMILY_INFO_OWNER, info.ownerId) == 0
		&&  PSGT_Redis_Mgt->hset_int(redis_key, REDIS_KEY_FAMILY_INFO_CREATE, info.createdAt) == 0
		&&  PSGT_Redis_Mgt->hset_int(redis_key, REDIS_KEY_FAMILY_INFO_UPDATE, info.updateAt) == 0 )
	{
		std::string err_info;
		PSGT_Redis_Mgt->refresh_security_channel(redis_key, err_info);
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "set redis of family(%d) failed.\n", info.familyId);
	return -1;
}

int Family_Mgt::_update_FamilyInfo_to_Redis(const StFamilyInfo &info)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_INFO_PRE_HEAD.c_str(), info.familyId);
	std::string redis_key = buf;

	if( PSGT_Redis_Mgt->exists(redis_key))
	{
		if( PSGT_Redis_Mgt->hset_string(redis_key, REDIS_KEY_FAMILY_INFO_NAME, info.familyName) == 0
		 && PSGT_Redis_Mgt->hset_string(redis_key, REDIS_KEY_FAMILY_INFO_AVATAR, info.familyAvatar) == 0
		 && PSGT_Redis_Mgt->hset_int(redis_key, REDIS_KEY_FAMILY_INFO_UPDATE, info.updateAt) == 0 )
		{
			std::string err_info;
			PSGT_Redis_Mgt->refresh_security_channel(redis_key, err_info);
			return 0;
		}
	}
	else
	{
		_set_FamilyInfo_to_Redis(info);
	}

	XCP_LOGGER_INFO(&g_logger, "update redis of family(%d) failed.\n", info.familyId);
	return -1;
}

int Family_Mgt::_remove_FamilyInfo_from_Redis(const uint64 &familyId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_INFO_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;
	
	if( PSGT_Redis_Mgt->exists(redis_key) && PSGT_Redis_Mgt->remove(redis_key) == 0)
	{
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "update redis of family(%d) failed.\n", familyId);
	return -1;
}

int Family_Mgt::_set_user_joined_family_list_to_Redis(const uint64 &userId, const std::vector<uint64> &familyIds)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_LIST_PRE_HEAD.c_str(), userId);
	std::string redis_key = buf;

	if(PSGT_Redis_Mgt->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "redis key %s have exists.\n", redis_key.c_str());
		return 0;
	}
	
	XCP_LOGGER_INFO(&g_logger, "size:%d\n", familyIds.size());
	std::vector<uint64>::const_iterator end = familyIds.end();
	for(std::vector<uint64>::const_iterator it = familyIds.begin(); it != end; it++)
	{
		if(PSGT_Redis_Mgt->sadd_int(redis_key, *it) != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "put user(%d) joined family list to redis failed.\n", userId);

			// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
			PSGT_Redis_Mgt->remove(redis_key);
			
			return -1;
		}
		XCP_LOGGER_INFO(&g_logger, "set redis pair(%s, %llu) success.\n", redis_key.c_str(), *it);
	}

	XCP_LOGGER_INFO(&g_logger, "set user(%d)'s family list to redis success.\n", userId);
	return 0;
}

int Family_Mgt::_set_user_joined_new_family_to_Redis(const uint64 &userId, const uint64 &familyId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_LIST_PRE_HEAD.c_str(), userId);
	std::string redis_key = buf;

	if(PSGT_Redis_Mgt->exists(redis_key))
	{
		if(PSGT_Redis_Mgt->sadd_int(redis_key, familyId) != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "put user(%d) joined family(%d) to redis failed.\n", userId, familyId);
			// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
			PSGT_Redis_Mgt->remove(redis_key);				
			return -1;
		}
		XCP_LOGGER_INFO(&g_logger, "put user(%d) joined family(%d) to redis success.\n", userId, familyId);
	}
	// 缓存中没有用户加入的家庭列表，重新查询数据库
	else
	{
		XCP_LOGGER_INFO(&g_logger, "user(%d) is not exist in redis, will put family list.\n", userId);
		std::vector<uint64> familyIds;
		if(_get_user_joined_family_list_from_sql(userId, familyIds) != 0)
		{
			//如果查询数据库失败，返回异常
			XCP_LOGGER_ERROR(&g_logger, "get family list failed,sql abnormal");
			return -1;
		}
		_set_user_joined_family_list_to_Redis(userId, familyIds);
	}
	
	return 0;
}

int Family_Mgt::_set_user_quit_family_to_Redis(const uint64 &userId, const uint64 &familyId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_LIST_PRE_HEAD.c_str(), userId);
	std::string redis_key = buf;

	if(!PSGT_Redis_Mgt->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "%s is not exists in redis, ignore request.\n", redis_key.c_str());
		return 0;
	}

	if(PSGT_Redis_Mgt->exists(redis_key) && PSGT_Redis_Mgt->sremove_int(redis_key, familyId) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "user(%d) quit family(%d) to redis failed.\n", userId, familyId);
		
		// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
		PSGT_Redis_Mgt->remove(redis_key);
			
		return -1;
	}
	
	XCP_LOGGER_INFO(&g_logger, "user(%d) quit family(%d) to redis success.\n", userId, familyId);
	return 0;
}

int Family_Mgt::_set_family_all_user_list_to_Redis(const uint64 &familyId, const std::vector<uint64> &userIds, const uint64 ownerId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_MEMBER_LIST_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;
	
	if(PSGT_Redis_Mgt->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "%s have exists in redis.\n", redis_key.c_str());
		return 0;
	}
	
	std::vector<uint64>::const_iterator end = userIds.end();
	for(std::vector<uint64>::const_iterator it = userIds.begin(); it != end; it++)
	{
		int nRet = 0;
		if(*it == ownerId)
		{
			nRet = PSGT_Redis_Mgt->sadd_int_with_special_symbol(redis_key, *it);
		}
		else
		{
			nRet = PSGT_Redis_Mgt->sadd_int(redis_key, *it);
		}
		
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "put family(%d) all memeber to redis failed.\n", familyId);

			// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
			PSGT_Redis_Mgt->remove(redis_key);
			
			return -1;
		}
	}

	XCP_LOGGER_INFO(&g_logger, "put family(%d) all memeber to redis success.\n", familyId);
	return 0;

}

int Family_Mgt::_set_family_add_new_member_to_Redis(const uint64 &familyId, const uint64 &userId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_MEMBER_LIST_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;
	
	if(PSGT_Redis_Mgt->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "%s have exists in redis.\n", redis_key.c_str());
		if(PSGT_Redis_Mgt->sadd_int(redis_key, userId) != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "put new member(%d) in family(%d) to redis failed.\n", userId, familyId);
	
			// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
			PSGT_Redis_Mgt->remove(redis_key);
			
			return -1;
		}
		XCP_LOGGER_INFO(&g_logger, "put new member(%d) in family(%d) to redis success.\n", userId, familyId);
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "%s is not exists in redis, will put all member from sql.\n", redis_key.c_str());
		std::vector<uint64> userIds;
		uint64 ownerId = 0;
		if(_get_family_all_user_list_from_sql(familyId, userIds, ownerId) != 0)
		{
			return -1;
		}
		_set_family_all_user_list_to_Redis(familyId, userIds, ownerId);
	}
	
	return 0;

}

int Family_Mgt::_set_family_remove_member_to_Redis(const uint64 &userId, const uint64 &familyId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_MEMBER_LIST_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	if(!PSGT_Redis_Mgt->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "%s is not exists in redis, ignore remove request.\n", redis_key.c_str());
		return 0;
	}

	if(PSGT_Redis_Mgt->sremove_int(redis_key, userId) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "user(%d) quit family(%d) to redis failed.\n", userId, familyId);
		
		// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
		PSGT_Redis_Mgt->remove(redis_key);
			
		return -1;
	}
	
	XCP_LOGGER_INFO(&g_logger, "user(%d) quit family(%d) to redis success.\n", userId, familyId);
	return 0;
}

int Family_Mgt::_get_user_joined_family_list_from_Redis(const uint64 &userId, std::vector<uint64> &familyIds)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_FAMILY_LIST_PRE_HEAD.c_str(), userId);
	std::string redis_key = buf;

	if(!PSGT_Redis_Mgt->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "get user(%d) joined family list from redis but it's not exist.\n", userId);
		return -1;
	}

	uint64 specialId = 0;
	if(PSGT_Redis_Mgt->sget_all(redis_key, familyIds, specialId) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "get user(%d) joined family list from redis failed.\n", userId);
		return -1;
	}
	
	XCP_LOGGER_INFO(&g_logger, "get user(%d) joined family list(%d record) from redis success.\n", userId, familyIds.size());
	return 0;
}

int Family_Mgt::_get_family_all_user_list_from_Redis(const uint64 &familyId, std::vector<uint64> &userIds, uint64 &ownerId)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d",REDIS_MEMBER_LIST_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	if(!PSGT_Redis_Mgt->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "%s is not exist.\n", redis_key.c_str());
		return -1;
	}
	if(PSGT_Redis_Mgt->sget_all(redis_key, userIds, ownerId) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "get member list of family(%s) from redis failed.\n", redis_key.c_str());
		return -1;
	}
	
	XCP_LOGGER_INFO(&g_logger, "get member list of family(%s) from redis success.\n", redis_key.c_str());
	return 0;

}

int Family_Mgt::_get_FamilyInfo_from_sql(const uint64 &familyId, StFamilyInfo &result)
{
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);
	
	std::ostringstream sql;
	sql << "select F_name,F_image,F_owner_id,F_created_at,F_updated_at from tbl_family_"
		<< (familyId % 10)
		<< " where F_family_id = '"
		<< familyId
		<< "';";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "server access isn't found.\n");
		return -1;
	}

	int i = 0;
	result.familyName = row_set[0][i++];
	result.familyAvatar = row_set[0][i++];
	result.ownerId = atoi(row_set[0][i++].c_str());
	result.createdAt = atoi(row_set[0][i++].c_str());
	result.updateAt = atoi(row_set[0][i++].c_str());
	
	XCP_LOGGER_INFO(&g_logger, "query sql sucess.id:%d, name:%s, avatar:%s, ownerId:%d\n",
		familyId, result.familyName.c_str(), result.familyAvatar.c_str(), result.ownerId);
	
	return nRet;
}

int Family_Mgt::_set_FamilyInfo_to_sql(const StFamilyInfo &info)
{
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	std::ostringstream sql;
	sql << "insert into tbl_family_"
		<< (info.familyId % 10)
		<< "(F_family_id, F_name, F_owner_id, F_image, F_created_at, F_updated_at) values("
		<< info.familyId << ", "
		<< "'" << info.familyName<< "', "
		<< info.ownerId << ", "
		<< "'" << info.familyAvatar<< "', "
		<< info.createdAt<< ", "
		<< info.updateAt<< ")"
		<< ";";

	uint64 last_insert_id = 0;
    uint64 affected_rows = 0;
    int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);

	// 同步修改用户家庭表和家庭成员表
	if(affected_rows >= 1)
	{
		uint64 timeStamp = getTimestamp();
		// insert into tbl_family_member_0 (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values (11, 11, 1, 11111,11111);
		sql.str("");
		sql << "insert into tbl_family_member_"
			<< (info.familyId % 10)
			<< " (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values ("
			<< info.familyId <<", " << info.ownerId <<", " << FAMILY_ROLE_OWNER <<", " << timeStamp <<", " << timeStamp
			<< ");";

	    nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
		
		// 刷新缓存，忽略失败
		_set_user_joined_new_family_to_Redis(info.ownerId, info.familyId);

		// insert into tbl_user_family_0 (F_uid, F_family_id) values (123, 123);
		sql.str("");
		sql << "insert into tbl_user_family_"
			<< (info.ownerId% 10)
			<< " (F_uid, F_family_id) values ("
			<< info.ownerId <<", " << info.familyId
			<< ");";
		
	    nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
		
		// 刷新缓存，忽略失败
		_set_family_add_new_member_to_Redis(info.familyId, info.ownerId);
	}
	
    return nRet;
}

int Family_Mgt::_update_FamilyInfo_to_sql(const StFamilyInfo &info)
{
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	// update tbl_family_1 set F_name = '', F_image = '' where F_family_id = 111111;
	std::ostringstream sql;
	sql << "update tbl_family_"
		<< (info.familyId % 10)
		<< " set F_name = '" << info.familyName << "', "
		<< "F_image = '" << info.familyAvatar << "', "
		<< "F_updated_at = " << info.updateAt
		<< " where F_family_id = " << info.familyId
		<< ";";

	uint64 last_insert_id = 0;
    uint64 affected_rows = 0;
    int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);    
	CHECK_RET(nRet);
	
    return nRet;
}

int Family_Mgt::_get_user_joined_family_list_from_sql(const uint64 &userId, std::vector<uint64> &familyIds)
{
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	/* select F_family_id from tbl_user_family_0 WHERE F_uid = 'aaa'; */
	std::ostringstream sql;
	sql << "select F_family_id from tbl_user_family_"
		<< (userId % 10)
		<< " where F_uid = " << userId
		<< ";";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		familyIds.push_back((uint64)atoi(row_set[i][0].c_str()));
	}
	return nRet;
}

int Family_Mgt::_get_family_all_user_list_from_sql(const uint64 &familyId, std::vector<uint64> &userIds, uint64 &ownerId)
{
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}	
	MySQL_Guard guard(conn);

	/* select F_uid from tbl_family_member_0 where F_family_id = 1111; */
	std::ostringstream sql;
	sql << "select F_uid from tbl_family_member_"
		<< (familyId % 10)
		<< " where F_family_id = '"
		<< familyId
		<< "';";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);	
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		userIds.push_back(atoi(row_set[i][0].c_str()));
	}

	/* select F_owner_id from tbl_family_3 where F_family_id = 2013; */
	sql.str("");
	sql << "select F_owner_id from tbl_family_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";

	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1);	
	CHECK_RET(nRet);

	ownerId = (uint64)atoi(row_set1[0][0].c_str());
	
	return nRet;
}

int Family_Mgt::_push_msg(const std::string &method, const std::string &msg_tag, const uint64 userId, const uint64 familyId, const uint64 operatorId)
{
	Conn_Ptr push_conn;
	if(!g_push_conn.get_conn(push_conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get push conn failed.\n");
		return 0;
	}
	std::string errInfo;
	int nRet = push_conn->push_msg(msg_tag, method, userId, familyId, operatorId, errInfo);
	CHECK_RET(nRet);

	return 0;
}

std::string Family_Mgt::_rand_str(const unsigned int length)
{
	std::string str;
	srand(getTimestamp());

	int size = (length < RANDON_STRING_MAX_LENGTH) ? length : RANDON_STRING_MAX_LENGTH;
    for(int i=0; i<size; i++)
	{
		//string &append(int n,char c); ----在当前字符串结尾添加n个字符c
    	str.append(1, (char)('a' + rand()%26));
	}
	
    return str;
}

int Family_Mgt::_generate_token(const std::string &key, TokenInfo &token_info, std::string &token, std::string &err_info)
{
	if (token_info.token_type <= TokenTypeBegin || token_info.token_type >= TokenTypeEnd)
	{
		err_info = "token_type error!";
		return -1;
	}
	unsigned long long timestamp = getTimestamp();
	token_info.create_time = timestamp;
	token_info.expire_time = timestamp + FAMILY_INVITITION_TOKEN_TTL;
	token_info.nonce = _rand_str(TOKEN_NONCE_STRING_LENGTH);

	if (token_info.user_id <= 0 || token_info.phone.empty() || token_info.create_time <= 0 || token_info.nonce.empty())
	{
		err_info = "generate token fail!less must param";
		return -1;
	}

	if (token_info.token_type == TokenTypeUpdatePwd)
	{
		if (token_info.pwd.empty() || token_info.salt.empty())
		{
			err_info = "generate token fail!less must param about pwd";
			return -1;
		}
	}

	std::string sig;

	int nRet = _get_sig_token(key, token_info, sig);
	CHECK_RET(nRet);

	char src[512] = { 0 };
	if (token_info.token_type == TokenTypeLogin)
	{
		snprintf(src, sizeof(src), "{\"user_id\":%llu,\"phone\":\"%s\",\"token_type\":%d,\"create_time\":%llu,\"expire_time\":%llu,\"from\":\"Server\",\"nonce\":\"%s\",\"sig\":\"%s\"}",
			token_info.user_id, token_info.phone.c_str(),token_info.token_type, token_info.create_time, token_info.expire_time,token_info.nonce.c_str(), sig.c_str());
	}
	else if(token_info.token_type == TokenTypeGetAccount)
	{
		snprintf(src, sizeof(src), "{\"user_id\":%llu,\"phone\":\"%s\",\"token_type\":%d,\"create_time\":%llu,\"expire_time\":%llu,\"from\":\"Server\",\"nonce\":\"%s\",\"salt\":\"%s\",\"password\":\"%s\"}",
			token_info.user_id, token_info.phone.c_str(),token_info.token_type, token_info.create_time, token_info.expire_time,token_info.nonce.c_str(),token_info.salt.c_str(),token_info.pwd.c_str());
	}
	else
	{
		snprintf(src, sizeof(src), "{\"user_id\":%llu,\"phone\":\"%s\",\"token_type\":%d,\"create_time\":%llu,\"expire_time\":%llu,\"from\":\"Server\",\"nonce\":\"%s\",\"sig\":\"%s\",\"salt\":\"%s\",\"password\":\"%s\"}",
			token_info.user_id, token_info.phone.c_str(), token_info.token_type,token_info.create_time, token_info.expire_time, token_info.nonce.c_str(), sig.c_str(),token_info.salt.c_str(),token_info.pwd.c_str());
	}
	//base64
	Base64 base64;
	std::string s_src = src;
	char *tmp = NULL;
	if (base64.encrypt(s_src.c_str(), s_src.size(), tmp) != 0)
	{
		return -2;
	}
	token = tmp;
	XCP_LOGGER_INFO(&g_logger, "src token:%s\n", s_src.c_str());
	return 0;
}

int Family_Mgt::_get_sig_token(const std::string &key, const TokenInfo &token_info, std::string &sig)
{
	std::map<std::string, std::string> m_src;
	std::stringstream ss;
	ss << token_info.user_id;
	m_src.insert(std::make_pair<std::string, std::string>("user_id", ss.str()));
	m_src.insert(std::make_pair<std::string, std::string>("phone", (std::string)token_info.phone));
	ss.str("");
	ss << token_info.token_type;
	m_src.insert(std::make_pair<std::string, std::string>("token_type", (std::string)ss.str()));
	ss.str("");
	ss << token_info.create_time;
	m_src.insert(std::make_pair<std::string, std::string>("create_time", (std::string)ss.str()));

	ss.str("");
	ss << token_info.expire_time;
	m_src.insert(std::make_pair<std::string, std::string>("expire_time", (std::string)ss.str()));
	m_src.insert(std::make_pair<std::string, std::string>("from", (std::string)token_info.from));
	m_src.insert(std::make_pair<std::string, std::string>("nonce", (std::string)token_info.nonce));
	if (token_info.token_type == TokenTypeUpdatePwd)
	{
		m_src.insert(std::make_pair<std::string, std::string>("salt", (std::string)token_info.salt));
		m_src.insert(std::make_pair<std::string, std::string>("password", (std::string)token_info.pwd));
	}

	std::map<std::string, std::string>::iterator it;
	for (it = m_src.begin(); it != m_src.end(); it++)
	{
		ss << it->first
   		<< "="
   		<< it->second
   		<< "&";
	}
	std::string sig_src = ss.str();
	sig_src = sig_src.substr(0, sig_src.length() - 1);

	X_HMACSHA1 hmac;
	if (!hmac.calculate_digest(key, sig_src, sig))
	{
		XCP_LOGGER_ERROR(&g_logger, "src:%s,key:%s,sig:%s\n", sig_src.c_str(),key.c_str(), sig.c_str());
		return -1;
	}
	XCP_LOGGER_INFO(&g_logger, "src:%s,sig:%s\n", sig_src.c_str(), sig.c_str());
	return 0;
}



