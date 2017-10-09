
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "family_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;
extern redis_mgt g_redis_mgt;
extern Conn_Mgt_LB g_push_conn;

#define CHECK_RET(nRet)                                                      \
{                                                                     		 \
		if (nRet != 0)                                                       \
		{                                                                    \
			XCP_LOGGER_ERROR(&g_logger, "check return failed: %d.\n", nRet); \
			return nRet;                                                     \
		}                                                                    \
}

#define CHECK_RET_ROLL_BACK(nRet, conn)                                      \
{                                                                     		 \
		if (nRet != 0)                                                       \
		{                                                                    \
			XCP_LOGGER_ERROR(&g_logger, "check return failed: %d.\n", nRet); \
			conn->rollback();                                                \
			conn->autocommit(true);                                          \
			return nRet;                                                     \
		}                                                                    \
}

#define MYSQL_GET_CONN_THEN_LOCK(conn)                           \
	if (!g_mysql_mgt.get_conn(conn))                             \
	{                                                            \
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n"); \
		errInfo = "get mysql conn failed";						 \
		return ERR_GET_MYSQL_CONN_FAILED;                        \
	}                                                            \
	MySQL_Guard guard_mysql(conn);                               \

#define REDIS_GET_CONN_THEN_LOCK(conn)                           \
	if (!g_redis_mgt.get_conn(conn))                             \
	{                                                            \
		XCP_LOGGER_ERROR(&g_logger, "get redis conn failed.\n"); \
		errInfo = "get redis conn failed";						 \
		return ERR_REDIS_OPER_EXCEPTION;                       	 \
	}                                                            \
	Redis_Guard guard_redis(conn);                               \

const std::string REDIS_KEY_FAMILY_INFO_ID = "id";
const std::string REDIS_KEY_FAMILY_INFO_NAME = "name";
const std::string REDIS_KEY_FAMILY_INFO_AVATAR = "avatar";
const std::string REDIS_KEY_FAMILY_INFO_OWNER = "owner";
const std::string REDIS_KEY_FAMILY_INFO_ROUTER = "router";
const std::string REDIS_KEY_FAMILY_INFO_CREATE = "created";
const std::string REDIS_KEY_FAMILY_INFO_UPDATE = "update";

const std::string REDIS_FAMILY_INFO_PRE_HEAD = "Family:family_";
const std::string REDIS_FAMILY_ROUTER_PRE_HEAD = "FamilyBind:family_";
const std::string REDIS_FAMILY_LIST_PRE_HEAD = "UserInFamily:user_";
const std::string REDIS_MEMBER_LIST_PRE_HEAD = "FamilyAllUser:family_";
const std::string REDIS_FAMILY_CODE_PRE_HEAD = "FamilyCode:family_";
const std::string REDIS_FAMILY_MEMBER_PER_HEAD = "FamilyMember:family_";
const std::string REDIS_FAMILY_MEMBER_SECOND_HEAD = ":member_";

const std::string REDIS_FAMILY_CODE_BEEN_USED = "@invalid@";

const unsigned int REDIS_KEY_MAX_LENGTH = 50;
const unsigned int RANDON_STRING_MAX_LENGTH = 25;
const unsigned int TOKEN_NONCE_STRING_LENGTH = 8;
const unsigned int PUSH_MSG_TAG_LENGTH = 12;
const unsigned int FAMILY_CODE_EXPIRE_SECOND = 180;
const unsigned int FAMILY_INVITITION_TOKEN_TTL = 3 * 24 * 3600;

Family_Mgt::Family_Mgt()
{
}

Family_Mgt::~Family_Mgt()
{
}

int Family_Mgt::create_family(const uint64 &userId, const std::string &msg_tag, const StFamilyInfo &info, std::string &token, std::string &errInfo)
{
	// 不需要校验权限
	int nRet = _set_FamilyInfo_to_sql(info, errInfo);
	CHECK_RET(nRet);

	// 删除家庭信息缓存，忽略失败
	/*nRet = _remove_FamilyInfo_from_Redis(info.familyId);
	CHECK_RET(nRet);*/

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::update_family(const uint64 &userId, const std::string &msg_tag, const StFamilyInfo &info, std::string &errInfo)
{
	// 只有户主可以操作
	uint64 ownerId = 0;
	std::vector<uint64> members;
	int nRet = _get_members_by_family(info.familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	if (userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		errInfo = "only family owner can modify family info";
		return ERR_MEMBER_UPDATE_FAMILY_INFO;
	}

	// 写入数据库
	nRet = _update_FamilyInfo_to_sql(info, errInfo) != 0;
	CHECK_RET(nRet);

	// 清除缓存，如果删除失败，忽略
	/*nRet = _remove_FamilyInfo_from_Redis(info.familyId);
	CHECK_RET(nRet);*/

	// 家庭信息更新，需要同步给家庭的用户
	_push_msg(CMD_SYN_UPDATE_FAMILY, msg_tag, userId, info.familyId, userId, errInfo);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::member_switch_family(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &errInfo)
{
	// 切换家庭，前提是用户需要加入家庭
	if (!_is_user_joined_family(userId, familyId))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot allow user(%llu) switch to family(%llu), must join the family first.\n",
						 userId, familyId);
		errInfo = "user donot join the family, then cannot switch to it";
		return ERR_MEMBER_SWITCH_BUT_DONOT_JOIN_FAMILY;
	}

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	// update tbl_user_info_0 set F_last_family_id = 444 where F_uid = 111110;
	std::ostringstream sql;
	sql << "update tbl_user_info_"
		<< (userId % 10)
		<< " set F_last_family_id = " << familyId
		<< " where F_uid = " << userId
		<< ";";

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET(nRet);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::get_family_info(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, StFamilyInfo &result, std::string &errInfo)
{
	// 不需要检查权限
	return _get_familyInfo_by_id(familyId, result, errInfo);
}

int Family_Mgt::get_family_list(const uint64 &userId, const std::string &msg_tag, std::vector<StFamilyInfo> &result, std::string &errInfo)
{
	// 不需要查询用户权限

	std::vector<uint64> familyIds;
	int nRet = _get_family_ids_by_user(userId, familyIds, errInfo);
	CHECK_RET(nRet);

	nRet = _muti_get_family_info(familyIds, result, errInfo);
	CHECK_RET(nRet);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::bind_router(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, uint64 &routerId, const std::string &uuid, const std::string &pwd, std::string &errInfo)
{
	XCP_LOGGER_INFO(&g_logger, "start bind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);

	// 检查是不是户主操作
	uint64 ownerId = 0;
	std::vector<uint64> members;
	int nRet = _get_members_by_family(familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	if (userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can bind family\n", ownerId);
		errInfo = "only family owner can bind family";
		return ERR_MEMBER_BIND_FAMILY;
	}

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	MySQL_Row_Set row_set, row_set1;
	uint64 last_insert_id = 0, affected_rows = 0;
	std::ostringstream sql;

	// 检查路由器信息是否正确
	/* select F_rid from tbl_router_map where F_device_id = '1'; */
	sql.str("");
	sql << "select F_rid from tbl_router_map where F_device_id = '" << uuid << "';";
	nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);
	if (row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find uuid:%s in database\n", uuid.c_str());
		errInfo = "router uuid not exists";
		return ERR_ROUTER_UUID_DONOT_EXIST;
	}
	routerId = (uint64)atoll(row_set[0][0].c_str());
	/* select F_device_auth_key,F_device_id,F_family_id from tbl_router_0 where F_router_id = 1; */
	sql.str("");
	sql << "select F_device_auth_key,F_device_id,F_family_id  from tbl_router_" << routerId % 10
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->query_sql(sql.str(), row_set1, errInfo);
	CHECK_RET(nRet);
	if (pwd != row_set1[0][0] || uuid != row_set1[0][1])
	{
		XCP_LOGGER_ERROR(&g_logger, "pwd or uuid is not correct, %s <---> %s, %s <---> %s\n",
						 pwd.c_str(), row_set1[0][0].c_str(), uuid.c_str(), row_set1[0][1].c_str());
		errInfo = "router pwd not correct";
		return ERR_PWD_DONOT_CORRECT;
	}

	// 如果路由器已经绑定过，需要先解绑路由器，不能重复绑定
	uint64 bindFamilyId = (uint64)atoll(row_set1[0][2].c_str());
	if (bindFamilyId == familyId)
	{
		XCP_LOGGER_INFO(&g_logger, "the family have bind this router before\n", userId, familyId, routerId);
		errInfo = "router have bind this family before";
		return ERR_SUCCESS;
	}
	if (bindFamilyId != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "router have been bind for family:%llu\n", bindFamilyId);
		errInfo = "router have been bind to other family";
		return ERR_ROUTER_HAVE_BIND_OTHER_FAMILY;
	}

	StFamilyInfo info;
	nRet = _get_familyInfo_by_id(familyId, info, errInfo);
	CHECK_RET(nRet);
	if(info.routerId != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "the family have bind other router before\n", userId, familyId, routerId);
		errInfo = "family have bind other router";
		return ERR_FAMILY_HAVE_BIND_OTHER_ROUTER;
	}
	
	// 关闭自动提交，绑定过程出错需要回滚数据
	conn->autocommit(false);

	// 更新家庭表
	/* update tbl_family_0 set F_router_id = 1 where F_family_id = 10; */
	sql.str("");
	sql << "update tbl_family_" << (familyId % 10)
		<< " set F_router_id = " << routerId
		<< " where F_family_id = " << familyId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET_ROLL_BACK(nRet, conn);
	info.routerId = routerId;
	// 清除缓存，忽略失败
	/*nRet = _remove_FamilyInfo_from_Redis(info.familyId);*/
	//CHECK_RET(nRet);

	// 更新路由器信息表
	/* update tbl_router_0 set F_family_id = 2013 where F_router_id = 1001; */
	sql.str("");
	sql << "update tbl_router_" << (routerId % 10)
		<< " set F_family_id = " << familyId
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET_ROLL_BACK(nRet, conn);
	
	conn->commit();
	conn->autocommit(true);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::unbind_router(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &errInfo)
{
	StFamilyInfo info;
	int nRet = _get_familyInfo_by_id(familyId, info, errInfo);
	CHECK_RET(nRet);
	uint64 routerId = info.routerId;
	XCP_LOGGER_INFO(&g_logger, "start unbind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);

	// 检查家庭有没有绑定过路由器
	if (routerId == 0)
	{
		return ERR_UNBIND_BUT_DONOT_BIND;
	}
	// 检查是不是户主操作
	uint64 ownerId = 0;
	std::vector<uint64> members;
	nRet = _get_members_by_family(familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	if (userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		errInfo = "only family owner can unbind router";
		return ERR_MEMBER_UNBIND_FAMILY;
	}

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	uint64 last_insert_id = 0, affected_rows = 0;

	// 关闭自动提交，绑定过程出错需要回滚数据
	conn->autocommit(false);

	/* update tbl_family_0 set F_router_id = 1 where F_family_id = 10; */
	sql.str("");
	sql << "update tbl_family_" << (familyId % 10)
		<< " set F_router_id = 0 where F_family_id = " << familyId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET_ROLL_BACK(nRet, conn);

	// 更新缓存
	info.routerId = 0;
	/*nRet = _remove_FamilyInfo_from_Redis(info.familyId);*/
	//CHECK_RET(nRet);

	// 更新路由器信息表
	/* update tbl_router_0 set F_family_id = 0 where F_router_id = 1001; */
	sql.str("");
	sql << "update tbl_router_" << (routerId % 10)
		<< " set F_family_id = null where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET_ROLL_BACK(nRet, conn);

	conn->commit();
	conn->autocommit(true);

	// 解绑后，删除家庭内所有房间、设备、快捷方式
	/* delete from tbl_room_3 where F_family_id = 2013; */
	sql.str("");
	sql << "delete from tbl_room_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	XCP_LOGGER_INFO(&g_logger, "unbind router, delete %d room record\n", affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "unbind router, delete room data failed, nRet:%d\n", nRet);
	}
	/* delete from tbl_room_device_3 where F_family_id = 2013; */
	sql.str("");
	sql << "delete from tbl_room_device_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	XCP_LOGGER_INFO(&g_logger, "unbind router, delete %d device record\n", affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "unbind router, delete device data failed, nRet:%d\n", nRet);
	}
	/* delete from tbl_shortcut_3 where F_family_id = 2013; */
	sql.str("");
	sql << "delete from tbl_shortcut_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	XCP_LOGGER_INFO(&g_logger, "unbind router, delete %d shortcut record\n", affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "unbind router, delete shortcut data failed, nRet:%d\n", nRet);
	}

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::get_apply_code(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &code, uint64 &timestamp, std::string &errInfo)
{
	// 检查是不是户主操作；检查家庭是不是绑定的这个路由器
	uint64 ownerId = 0;
	std::vector<uint64> members;
	int nRet = _get_members_by_family(familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	if (userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can update family info\n", ownerId);
		errInfo = "only family owner can apply invite code";
		return ERR_MEMBER_APPLY_CODE;
	}

	code = _rand_str(RANDON_STRING_MAX_LENGTH);
	timestamp = getTimestamp();

	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_CODE_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	// 超时时间180秒，考虑网络延迟，另多加5秒
	if (conn_redis->set_string(redis_key, code, FAMILY_CODE_EXPIRE_SECOND + 5) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family code to cach", familyId);
		errInfo = "cannot write the family code to cach";
		return ERR_CANNOT_WRITER_CODE_TO_REDIS;
	}

	XCP_LOGGER_INFO(&g_logger, "generate family code success.\n", familyId);
	errInfo = "OK";
	return ERR_SUCCESS;
}

/**
  *1. 如果用户是家庭内的普通成员，那么有可能是APP同步添加成员到路由器失败，需要重新生成token
  *2. 如果用户是家庭的户主，那么不允许操作
  *3. 如果用户不属于家庭，则将成员添加到家庭，且生成token
**/
int Family_Mgt::create_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const std::string &code, std::string &token, std::string &errInfo)
{
	uint64 ownerId = 0;
	std::vector<uint64> members;
	int nRet = _get_members_by_family(familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	if (userId == ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "family owner cannot invite himself to join\n");
		errInfo = "family owner cannot invite himself to join";
		return ERR_OWNER_INVITER_HIS_SELF;
	}

	// 检查code是否正确
	std::string real_code;
	nRet = _match_invite_code(familyId, code, errInfo);
	CHECK_RET(nRet);

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	MySQL_Row_Set row_set;
	uint64 timeStamp = getTimestamp(), last_insert_id = 0, affected_rows = 0;
	// 查询用户的基本信息
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql << "select F_phone_num, F_password, F_salt from tbl_user_info_" << (userId % 10)
		<< " where F_uid = " << userId << ";";
	nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);

	if (row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get user(%llu) info from sql.\n", userId);
		errInfo = "can not get user account info from sql";
		return ERR_GET_OWNER_ACCOUNT_INFO_FAILED;
	}

	// 查询出户主的md5密钥
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql.str("");
	sql << "select F_password from tbl_user_info_" << (ownerId % 10)
		<< " where F_uid = " << ownerId << ";";
	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1, errInfo);
	CHECK_RET(nRet);

	if (row_set1.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get owner(%llu) info from sql.\n", ownerId);
		errInfo = "can not get family owner account info from sql";
		return ERR_GET_MEMBER_ACCOUNT_INFO_FAILED;
	}

	std::vector<uint64>::iterator it = std::find(members.begin(), members.end(), userId);
	if (it != members.end())
	{
		// 如果用户已经在家庭里面，不用重复添加成员
		XCP_LOGGER_INFO(&g_logger, "user have included in the family, will generate invition again\n");
		/* select F_created_at from tbl_family_member_3 where F_family_id = 2013 and F_uid = 2003;*/
		sql.str("");
		sql << "select F_created_at from tbl_family_member_" << (familyId % 10)
			<< " where F_family_id = " << familyId
			<< " and F_uid = " << userId
			<< ";";
		MySQL_Row_Set row_set2;
		nRet = conn->query_sql(sql.str(), row_set2, errInfo);
		CHECK_RET(nRet);
		errInfo = FormatDateTimeStr(atoll(row_set2[0][0].c_str()), "%Y/%m/%d %H:%M:%S");
		return ERR_USER_HAVE_JOIN_FAMILY;
	}
	else
	{
		// 关闭自动提交，绑定过程出错需要回滚数据
		conn->autocommit(false);

		// insert into tbl_family_member_0 (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values (11, 11, 1, 11111,11111);
		sql.str("");
		sql << "insert into tbl_family_member_"
			<< (familyId % 10)
			<< " (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values ("
			<< familyId << ", " << userId << ", " << FAMILY_ROLE_MEMBER << ", " << timeStamp << ", " << timeStamp
			<< ");";

		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
		CHECK_RET_ROLL_BACK(nRet, conn);

		//nRet = _remove_family_all_user_list_from_Redis(familyId);
		//CHECK_RET_ROLL_BACK(nRet, conn);

		// insert into tbl_user_family_0 (F_uid, F_family_id) values (123, 123);
		sql.str("");
		sql << "insert into tbl_user_family_"
			<< (userId % 10)
			<< " (F_uid, F_family_id) values ("
			<< userId << ", " << familyId
			<< ");";
		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
		CHECK_RET_ROLL_BACK(nRet, conn);

		//nRet = _remove_user_joined_family_from_Redis(userId);
		//CHECK_RET_ROLL_BACK(nRet, conn);

		conn->commit();
		conn->autocommit(true);
	}

	TokenInfo _token;
	_token.user_id = userId;
	_token.phone = row_set[0][0];
	_token.pwd = row_set[0][1];
	_token.salt = row_set[0][2];
	_token.token_type = TokenTypeInviteMember;
	nRet = _generate_token(row_set1[0][0], _token, token, errInfo);
	CHECK_RET(nRet);

	// 同步，忽略失败
	_push_msg(CMD_SYN_ADD_MEMBER, msg_tag, userId, familyId, ownerId, errInfo);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::get_invitation(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &token, std::string &errInfo)
{
	uint64 ownerId = 0;
	std::vector<uint64> members;
	int nRet = _get_members_by_family(familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	std::vector<uint64>::iterator it = std::find(members.begin(), members.end(), userId);
	if (it == members.end())
	{
		XCP_LOGGER_ERROR(&g_logger, "this user donot include in family\n");
		errInfo = "user donot include in family";
		return ERR_GET_INVITATION_BUT_DONOT_JOIN_FAMILY;
	}

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	std::ostringstream sql;
	MySQL_Row_Set row_set;
	// 查询用户的基本信息
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql << "select F_phone_num, F_password, F_salt from tbl_user_info_" << (userId % 10)
		<< " where F_uid = " << userId << ";";
	nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);

	if (row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get user(%llu) info from sql.\n", userId);
		errInfo = "can not get user account info from sql";
		return ERR_GET_MEMBER_ACCOUNT_INFO_FAILED;
	}

	// 查询出户主的md5密钥
	/* select F_uid, F_phone_num, F_password, F_salt from tbl_user_info_3 where F_uid = 2003; */
	sql.str("");
	sql << "select F_password from tbl_user_info_" << (ownerId % 10)
		<< " where F_uid = " << ownerId << ";";
	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1, errInfo);
	CHECK_RET(nRet);

	if (row_set1.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "can not get owner(%llu) info from sql.\n", ownerId);
		return ERR_GET_OWNER_ACCOUNT_INFO_FAILED;
	}
	TokenInfo _token;
	_token.user_id = userId;
	_token.phone = row_set[0][0];
	_token.pwd = row_set[0][1];
	_token.salt = row_set[0][2];
	_token.token_type = TokenTypeInviteMember;
	nRet = _generate_token(row_set1[0][0], _token, token, errInfo);
	CHECK_RET(nRet);
	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::remove_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const uint64 &memberId, std::string &errInfo)
{
	// 1、只允许户主操作；2、不能删除自己；
	uint64 ownerId = 0;
	std::vector<uint64> members;
	int nRet = _get_members_by_family(familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	if (userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) can remove family member\n", ownerId);
		errInfo = "only family owner can remove family member";
		return ERR_ONLY_OWNER_CAN_REMOVE_MEMBER;
	}
	if (userId == memberId)
	{
		XCP_LOGGER_ERROR(&g_logger, "you cannot remove yourself, because you are the family owner\n", ownerId);
		errInfo = "cannot remove yourself cause you are the family owner";
		return ERR_AUTHENTIONCATION_BEGIN;
	}

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	
	// 关闭自动提交，绑定过程出错需要回滚数据
	conn->autocommit(false);

	// delete from tbl_family_member_0 where F_family_id = 11 and F_uid = 11;
	std::ostringstream sql;
	sql << "delete from tbl_family_member_"
		<< (familyId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_uid = " << memberId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET_ROLL_BACK(nRet, conn);

	//nRet = _remove_family_all_user_list_from_Redis(familyId);
	//CHECK_RET_ROLL_BACK(nRet);

	// delete from tbl_user_family_0 where F_family_id = 11 and F_uid = 11;
	sql.str("");
	sql << "delete from tbl_user_family_"
		<< (memberId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_uid = " << memberId
		<< ";";

	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET_ROLL_BACK(nRet, conn);

	/*nRet = _remove_user_joined_family_from_Redis(memberId);*/
	//CHECK_RET_ROLL_BACK(nRet);

	conn->commit();
	conn->autocommit(true);

	// 同步，忽略失败
	_push_msg(CMD_SYN_DEL_MEMBER, msg_tag, memberId, familyId, userId, errInfo);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::update_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
							  const uint64 &memberId, const std::string &memberName, std::string &errInfo)
{
	// 只允许户主操作；
	uint64 ownerId = 0;
	std::vector<uint64> members;
	int nRet = _get_members_by_family(familyId, members, ownerId, errInfo);
	CHECK_RET(nRet);
	if (userId != ownerId && userId != memberId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only owner(id:%llu) or himself can update family member info\n", ownerId);
		errInfo = "only family owner or himself can update member info";
		return ERR_MEMBER_UPDATE_OTHERS_INFO;
	}

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

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
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET(nRet);

	// 同步，忽略失败
	_push_msg(CMD_SYN_UPDATE_MEMBER, msg_tag, memberId, familyId, userId, errInfo);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::get_member_info(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
								const uint64 memberId, StFamilyMemberInfo &result, std::string &errInfo)
{
	// 不需要判断权限
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

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

	int nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);

	if (row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "query member info for user(%llu), get zero record.\n", userId);
		errInfo = "canot find member info from sql";
		return ERR_MEMBER_GET_ACCOUT_INFO_FAILED;
	}

	result.memberId = memberId;
	result.name = row_set[0][0];
	result.gender = (enum member_gender)(atoi(row_set[0][1].c_str()));
	result.nickName = row_set[0][2];
	result.role = (enum member_role_in_family)(atoi(row_set[0][3].c_str()));
	result.createdAt = (uint64)atoll(row_set[0][4].c_str());
	result.updateAt = (uint64)atoll(row_set[0][5].c_str());

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::get_member_info_list(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
									 std::vector<StFamilyMemberInfo> &result, std::string &errInfo)
{
	// 不需要判断权限
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	MySQL_Row_Set row_set;
	/* select F_uid,F_nickname,F_role_id,F_created_at,F_updated_at from tbl_family_member_0 where F_family_id = 111; */
	std::ostringstream sql;
	sql << "select F_uid,F_nickname,F_role_id,F_created_at,F_updated_at from tbl_family_member_"
		<< (familyId % 10)
		<< " where F_family_id = " << familyId
		<< ";";

	int nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);

	for (int i = 0; i < row_set.row_count(); i++)
	{
		uint64 memberId =  atoll(row_set[i][0].c_str());

		StFamilyMemberInfo info;
		info.memberId = memberId;
		info.nickName = row_set[i][1];
		info.role = (enum member_role_in_family)(atoi(row_set[i][2].c_str()));
		info.createdAt = atoll(row_set[i][3].c_str());
		info.updateAt = atoll(row_set[i][4].c_str());

		MySQL_Row_Set row_set1;
		sql.str("");
		sql << "select F_name,F_gender from tbl_user_info_" << (memberId % 10)
			<< " where F_uid = " << memberId
			<< ";";
		int nRet = conn->query_sql(sql.str(), row_set1, errInfo);
		CHECK_RET(nRet);
		info.name = row_set1[0][0];
		info.gender = (enum member_gender)(atoi(row_set1[0][1].c_str()));

		result.push_back(info);
	}
	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::get_member_id_list(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId,
								   std::vector<uint64> &result, uint64 &routerId, std::string &errInfo)
{
	// 不需要判断权限

	StFamilyInfo info;
	int nRet = _get_familyInfo_by_id(familyId, info, errInfo);
	CHECK_RET(nRet);
	routerId = info.routerId;

	uint64 ownerId;
	nRet = _get_members_by_family(familyId, result, ownerId, errInfo);
	CHECK_RET(nRet);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::check_talk_condition(const uint64 srcId, const uint64 targetId, const std::string &msg_tag, enum talk_msg_type type, std::string &errInfo)
{
	int nRet = 0;

	switch (type)
	{
	case TALK_MSG_TYPE_P2P:
		// 用户到用户，判断是否属于同一个家庭
		{
			std::vector<uint64> srcFamilyIds, targetFamilyIds;
			nRet = _get_family_ids_by_user(srcId, srcFamilyIds, errInfo);
			CHECK_RET(nRet);
			nRet = _get_family_ids_by_user(targetId, targetFamilyIds, errInfo);
			CHECK_RET(nRet);

			nRet = ERR_P2P_TALK_BUT_DONOT_JOIN_SAME_FAMILY;
			errInfo = "this two user donot include in a public family";
			std::vector<uint64>::iterator end = srcFamilyIds.end();
			for (std::vector<uint64>::iterator it = srcFamilyIds.begin(); it != end; it++)
			{
				std::vector<uint64>::iterator inter = std::find(targetFamilyIds.begin(), targetFamilyIds.end(), *it);
				if (inter != targetFamilyIds.end())
				{
					XCP_LOGGER_INFO(&g_logger, "get public family(%llu) for two users\n", *it);
					errInfo = "OK";
					nRet = ERR_SUCCESS;
					break;
				}
			}
		}
		break;
	case TALK_MSG_TYPE_P2R:
		// 判断用户所属于的家庭列表，有没有绑定过该路由器
		{
			uint64 familyId = 0;
			nRet = _get_family_id_by_router(targetId, familyId, errInfo);
			CHECK_RET(nRet);
			if (familyId == 0 || !_is_user_joined_family(srcId, familyId))
			{
				XCP_LOGGER_ERROR(&g_logger, "user(%llu) is not join family(%llu)\n", srcId, targetId);
				nRet = ERR_P2R_USERS_FAMILY_DONOT_BIND_ROUTER;
				errInfo = "the user donot join this family";
			}
		}
		break;
	case TALK_MSG_TYPE_P2F:
		// 判断用户是否属于家庭
		if (!_is_user_joined_family(srcId, targetId))
		{
			XCP_LOGGER_ERROR(&g_logger, "user(%llu) is not join family(%llu)\n", srcId, targetId);
			nRet = ERR_P2F_USERS_DONNOT_JOIN_FAMILY;
			errInfo = "the user donot join this family";
		}
		break;
	case TALK_MSG_TYPE_R2P:
		// 判断用户所属于的家庭列表，有没有绑定过该路由器，跟TALK_MSG_TYPE_P2R逻辑一样
		{
			uint64 familyId = 0;
			nRet = _get_family_id_by_router(srcId, familyId, errInfo);
			CHECK_RET(nRet);
			if (familyId == 0 || !_is_user_joined_family(targetId, familyId))
			{
				XCP_LOGGER_ERROR(&g_logger, "user(%llu) is not join family(%llu)\n", targetId, familyId);
				nRet = ERR_P2R_USERS_FAMILY_DONOT_BIND_ROUTER;
				errInfo = "the user donot join this family";
			}
		}
		break;
	case TALK_MSG_TYPE_R2F:
		// 查看家庭是否绑定该路由器
		{
			StFamilyInfo info;
			int nRet = _get_familyInfo_by_id(targetId, info, errInfo);
			CHECK_RET(nRet);

			if(info.routerId != srcId)
			{
				XCP_LOGGER_ERROR(&g_logger, "family bind router is(%llu)\n", info.routerId);
				nRet = ERR_R2F_FAMILY_DONOT_BIND_ROUTER;
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
	std::string errInfo;
	std::vector<uint64> familyIds;
	if (_get_family_ids_by_user(userId, familyIds, errInfo) != 0)
	{
		return false;
	}

	std::vector<uint64>::iterator it = std::find(familyIds.begin(), familyIds.end(), familyId);
	if (it != familyIds.end())
	{
		return true;
	}

	return false;
}

int Family_Mgt::_get_familyInfo_by_id(const uint64 familyId, StFamilyInfo &info, std::string &errInfo)
{
	// 查询缓存，命中，刷新超时时间，返回
	/*if (_get_FamilyInfo_from_Redis(familyId, info) == 0)
	{
		return ERR_SUCCESS;
	}*/

	// 未命中缓存，查询数据库，如果查询数据库失败，说明id不存在
	int nRet = _get_FamilyInfo_from_sql(familyId, info, errInfo);
	CHECK_RET(nRet);

	// 写入缓冲，忽略失败
	/*if (_set_FamilyInfo_to_Redis(info) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family(%d) info to cach", familyId);
	}*/

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_match_invite_code(const uint64 familyId, const std::string &code, std::string &errInfo)
{
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_CODE_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf, real_code;
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	if (conn_redis->get_string(redis_key, real_code) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot get the family code from cach\n", familyId);
		errInfo = "invite code have expired";
		return ERR_APPLY_CODE_EXPIRE;
	}
	if(real_code == REDIS_FAMILY_CODE_BEEN_USED)
	{
		XCP_LOGGER_ERROR(&g_logger, "invite code have been used\n", familyId);
		errInfo = "invite code have been used";
		return ERR_APPLY_CODE_HAVE_BEEN_USED;
	}
	if (code != real_code)
	{
		XCP_LOGGER_ERROR(&g_logger, "invite code not correct, %s <---> %s\n", code.c_str(), real_code.c_str());
		errInfo = "invite code is not correct";
		return ERR_APPLY_CODE_DONOT_CORRECT;
	}
	// 如果邀请码正确，标记成已使用过
	XCP_LOGGER_INFO(&g_logger, "invite code correct, now it will be marked because it can use only once\n");
	conn_redis->set_string(redis_key, REDIS_FAMILY_CODE_BEEN_USED, FAMILY_CODE_EXPIRE_SECOND - 60);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_get_family_id_by_router(const uint64 routerId, uint64 &familyId, std::string &errInfo)
{
	int nRet = 0;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	MySQL_Row_Set row_set;
	std::ostringstream sql;
	sql << "select F_family_id  from tbl_router_" << routerId % 10
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find router id from sql\n");
		errInfo = "cannot find router id from sql";
		return ERR_ROUTER_GET_BIND_FAMILY_FAILED;
	}

	familyId = (uint64)atoll(row_set[0][0].c_str());

	XCP_LOGGER_INFO(&g_logger, "get family id(%llu) bind router(%llu\n", familyId, routerId);
	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_get_members_by_family(const uint64 familyId, std::vector<uint64> &members, uint64 &ownerId, std::string &errInfo)
{
	// 查询缓存，命中，刷新超时时间，返回
	/*if (_get_family_all_user_list_from_Redis(familyId, members, ownerId) == 0)
	{
		return ERR_SUCCESS;
	}*/

	// 未命中缓存，查询数据库，如果查询数据库失败，说明id不存在
	if (_get_family_all_user_list_from_sql(familyId, members, ownerId, errInfo) != 0)
	{
		return ERR_FAMILY_ID_IS_NOT_EXIST;
	}

	// 写入缓冲，忽略失败
	/*if (_set_family_all_user_list_to_Redis(familyId, members, ownerId))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family(%d) member list to cach", familyId);
	}*/

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_get_family_ids_by_user(const uint64 userId, std::vector<uint64> &familyIds, std::string &errInfo)
{
	// 查询缓存，命中，刷新超时时间，返回
	/*if (_get_user_joined_family_list_from_Redis(userId, familyIds) == 0)
	{
		return ERR_SUCCESS;
	}*/

	// 未命中缓存，查询数据库
	if (_get_user_joined_family_list_from_sql(userId, familyIds, errInfo) != 0)
	{
		//如果查询数据库失败，返回异常
		return ERR_FAMILY_ID_IS_NOT_EXIST;
	}
	// 写入缓冲，忽略失败
	/*if (_set_user_joined_family_list_to_Redis(userId, familyIds))
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot write the family(%d) list to cach", userId);
	}*/

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_muti_get_family_info(const std::vector<uint64> &familyIds, std::vector<StFamilyInfo> &result, std::string &errInfo)
{
	int nRet = 0;
	result.clear();

	std::vector<uint64>::const_iterator end = familyIds.end();
	for (std::vector<uint64>::const_iterator it = familyIds.begin(); it != end; it++)
	{
		StFamilyInfo info;
		//if (_get_FamilyInfo_from_Redis(*it, info) != 0)
		//{
			// 查询缓存失败，查数据库，写缓存
			if (_get_FamilyInfo_from_sql(*it, info, errInfo) != 0)
			{
				// 查数据库失败，一般不会发生，有则跳过这段记录
				XCP_LOGGER_ERROR(&g_logger, "user join family(%llu), but family id is not exist.\n", *it);
				continue;
			}
		//	_set_FamilyInfo_to_Redis(info);
		//}
		result.push_back(info);
	}
	errInfo = "OK";
	return ERR_SUCCESS;
}

#if 0
int Family_Mgt::_get_FamilyInfo_from_Redis(const uint64 &familyId, StFamilyInfo &result)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_INFO_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	result.familyId = familyId;

	std::vector<std::string> keys, values;
	keys.push_back(REDIS_KEY_FAMILY_INFO_NAME);
	keys.push_back(REDIS_KEY_FAMILY_INFO_AVATAR);
	keys.push_back(REDIS_KEY_FAMILY_INFO_OWNER);
	keys.push_back(REDIS_KEY_FAMILY_INFO_ROUTER);
	keys.push_back(REDIS_KEY_FAMILY_INFO_CREATE);
	keys.push_back(REDIS_KEY_FAMILY_INFO_UPDATE);
	if (conn_redis->hmget_array(redis_key, keys, values) == 0)
	{
		int i = 0;
		result.familyName = values[i++];
		result.familyAvatar = values[i++];
		result.ownerId = (uint64)atoll(values[i++].c_str());
		result.routerId = (uint64)atoll(values[i++].c_str());
		result.createdAt = (uint64)atoll(values[i++].c_str());
		result.updateAt = (uint64)atoll(values[i++].c_str());
		return 0;
	}
	
	XCP_LOGGER_INFO(&g_logger, "query redis of family(%s) failed.\n", redis_key.c_str());
	return -1;
}

int Family_Mgt::_set_FamilyInfo_to_Redis(const StFamilyInfo &info)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_INFO_PRE_HEAD.c_str(), info.familyId);
	std::string redis_key = buf;

	std::vector<std::string> keys, values;
	keys.push_back(REDIS_KEY_FAMILY_INFO_NAME);
	keys.push_back(REDIS_KEY_FAMILY_INFO_AVATAR);
	keys.push_back(REDIS_KEY_FAMILY_INFO_OWNER);
	keys.push_back(REDIS_KEY_FAMILY_INFO_ROUTER);
	keys.push_back(REDIS_KEY_FAMILY_INFO_CREATE);
	keys.push_back(REDIS_KEY_FAMILY_INFO_UPDATE);

	values.push_back(info.familyName);
	values.push_back(info.familyAvatar);
	values.push_back(_int_to_string(info.ownerId));
	values.push_back(_int_to_string(info.routerId));
	values.push_back(_int_to_string(info.createdAt));
	values.push_back(_int_to_string(info.updateAt));
	
	if(conn_redis->hmset_array(redis_key, keys, values) == 0)
	{
		/* std::string err_info;
		conn_redis->refresh_security_channel(redis_key, err_info); */
		XCP_LOGGER_INFO(&g_logger, "set redis of redis_key(%s) sucess.\n", redis_key.c_str());
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "set redis of redis_key(%s) failed.\n", redis_key.c_str());
	conn_redis->remove(redis_key);
	return -1;
}

int Family_Mgt::_remove_FamilyInfo_from_Redis(const uint64 &familyId)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_INFO_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	int nRet = conn_redis->remove(redis_key);
	CHECK_RET(nRet);

	return ERR_SUCCESS;
}

int Family_Mgt::_get_user_joined_family_list_from_Redis(const uint64 &userId, std::vector<uint64> &familyIds)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_LIST_PRE_HEAD.c_str(), userId);
	std::string redis_key = buf;

	if (!conn_redis->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "get user(%d) joined family list from redis but it's not exist.\n", userId);
		return -1;
	}

	uint64 specialId = 0;
	if (conn_redis->sget_all(redis_key, familyIds, specialId) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "get user(%d) joined family list from redis failed.\n", userId);
		return -1;
	}

	XCP_LOGGER_INFO(&g_logger, "get user(%d) joined family list(%d record) from redis success.\n", userId, familyIds.size());
	return 0;
}

int Family_Mgt::_set_user_joined_family_list_to_Redis(const uint64 &userId, const std::vector<uint64> &familyIds)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_LIST_PRE_HEAD.c_str(), userId);
	std::string redis_key = buf;

	if (conn_redis->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "redis key %s have exists.\n", redis_key.c_str());
		return 0;
	}

	XCP_LOGGER_INFO(&g_logger, "size:%d\n", familyIds.size());
	std::vector<uint64>::const_iterator end = familyIds.end();
	for (std::vector<uint64>::const_iterator it = familyIds.begin(); it != end; it++)
	{
		if (conn_redis->sadd_int(redis_key, *it) != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "put user(%d) joined family list to redis failed.\n", userId);

			// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
			conn_redis->remove(redis_key);

			return -1;
		}
		XCP_LOGGER_INFO(&g_logger, "set redis pair(%s, %llu) success.\n", redis_key.c_str(), *it);
	}

	XCP_LOGGER_INFO(&g_logger, "set user(%d)'s family list to redis success.\n", userId);
	return 0;
}

int Family_Mgt::_remove_user_joined_family_from_Redis(const uint64 &userId)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_LIST_PRE_HEAD.c_str(), userId);
	std::string redis_key = buf;

	int nRet = conn_redis->remove(redis_key);
	CHECK_RET(nRet);

	return ERR_SUCCESS;
}

int Family_Mgt::_set_family_all_user_list_to_Redis(const uint64 &familyId, const std::vector<uint64> &userIds, const uint64 ownerId)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_MEMBER_LIST_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	if (conn_redis->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "%s have exists in redis.\n", redis_key.c_str());
		return 0;
	}

	std::vector<uint64>::const_iterator end = userIds.end();
	for (std::vector<uint64>::const_iterator it = userIds.begin(); it != end; it++)
	{
		int nRet = 0;
		if (*it == ownerId)
		{
			nRet = conn_redis->sadd_int_with_special_symbol(redis_key, *it);
		}
		else
		{
			nRet = conn_redis->sadd_int(redis_key, *it);
		}

		if (nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "put family(%d) all memeber to redis failed.\n", familyId);

			// 写入缓存如果失败，则清除缓存中的数据，删除失败忽略
			conn_redis->remove(redis_key);

			return -1;
		}
	}

	XCP_LOGGER_INFO(&g_logger, "put family(%d) all memeber to redis success.\n", familyId);
	return ERR_SUCCESS;
}

int Family_Mgt::_get_family_all_user_list_from_Redis(const uint64 &familyId, std::vector<uint64> &userIds, uint64 &ownerId)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_MEMBER_LIST_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	if (!conn_redis->exists(redis_key))
	{
		XCP_LOGGER_INFO(&g_logger, "%s is not exist.\n", redis_key.c_str());
		return -1;
	}
	if (conn_redis->sget_all(redis_key, userIds, ownerId) != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "get member list of family(%s) from redis failed.\n", redis_key.c_str());
		return -1;
	}

	XCP_LOGGER_INFO(&g_logger, "get member list of family(%s) from redis success.\n", redis_key.c_str());
	return ERR_SUCCESS;
}

int Family_Mgt::_remove_family_all_user_list_from_Redis(const uint64 &familyId)
{
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_MEMBER_LIST_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;

	int nRet = conn_redis->remove(redis_key);
	CHECK_RET(nRet);

	return ERR_SUCCESS;
}
#endif

int Family_Mgt::_get_FamilyInfo_from_sql(const uint64 &familyId, StFamilyInfo &result, std::string &errInfo)
{
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	std::ostringstream sql;
	sql << "select F_name,F_image,F_owner_id,F_router_id,F_created_at,F_updated_at from tbl_family_"
		<< (familyId % 10)
		<< " where F_family_id = '"
		<< familyId
		<< "';";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);

	if (row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "family id(%llu) is not exist.\n", familyId);
		errInfo = "family id is not exists";
		return ERR_FAMILY_ID_IS_NOT_EXIST;
	}

	int i = 0;
	result.familyId = familyId;
	result.familyName = row_set[0][i++];
	result.familyAvatar = row_set[0][i++];
	result.ownerId = (uint64)atoll(row_set[0][i++].c_str());
	result.routerId = (uint64)atoll(row_set[0][i++].c_str());
	result.createdAt = (uint64)atoll(row_set[0][i++].c_str());
	result.updateAt = (uint64)atoll(row_set[0][i++].c_str());

	XCP_LOGGER_INFO(&g_logger, "query sql sucess.id:%d, name:%s, avatar:%s, ownerId:%d\n",
					familyId, result.familyName.c_str(), result.familyAvatar.c_str(), result.ownerId);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_set_FamilyInfo_to_sql(const StFamilyInfo &info, std::string &errInfo)
{
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	std::ostringstream sql;
	sql << "insert into tbl_family_"
		<< (info.familyId % 10)
		<< "(F_family_id, F_name, F_owner_id, F_image, F_created_at, F_updated_at) values("
		<< info.familyId << ", "
		<< "'" << info.familyName << "', "
		<< info.ownerId << ", "
		<< "'" << info.familyAvatar << "', "
		<< info.createdAt << ", "
		<< info.updateAt << ")"
		<< ";";

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET(nRet);

	// 添加户主到家庭成员表，和成员家庭表
	if (affected_rows >= 1)
	{
		uint64 timeStamp = getTimestamp();
		// insert into tbl_family_member_0 (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values (11, 11, 1, 11111,11111);
		sql.str("");
		sql << "insert into tbl_family_member_"
			<< (info.familyId % 10)
			<< " (F_family_id, F_uid, F_role_id, F_created_at, F_updated_at) values ("
			<< info.familyId << ", " << info.ownerId << ", " << FAMILY_ROLE_OWNER << ", " << timeStamp << ", " << timeStamp
			<< ");";

		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
		CHECK_RET(nRet);

		// 刷新缓存，忽略失败
		/*nRet = _remove_user_joined_family_from_Redis(info.ownerId);
		CHECK_RET(nRet);*/

		// insert into tbl_user_family_0 (F_uid, F_family_id) values (123, 123);
		sql.str("");
		sql << "insert into tbl_user_family_"
			<< (info.ownerId % 10)
			<< " (F_uid, F_family_id) values ("
			<< info.ownerId << ", " << info.familyId
			<< ");";

		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
		CHECK_RET(nRet);

		// 刷新缓存，忽略失败
		/*nRet = _remove_family_all_user_list_from_Redis(info.familyId);
		CHECK_RET(nRet);*/
	}

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_update_FamilyInfo_to_sql(const StFamilyInfo &info, std::string &errInfo)
{
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	// update tbl_family_1 set F_name = '', F_image = '' where F_family_id = 111111;
	std::ostringstream sql;
	sql << "update tbl_family_"	<< (info.familyId % 10)
		<< " set F_updated_at = " << info.updateAt;
	if(info.familyName != "")
	{
		sql<< ",F_name = '" << info.familyName << "'";
	}
	if(info.familyAvatar != "")
	{
		sql << ",F_image = '" << info.familyAvatar << "'";
	}
	sql	<< " where F_family_id = " << info.familyId	<< ";";

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows, errInfo);
	CHECK_RET(nRet);

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_get_user_joined_family_list_from_sql(const uint64 &userId, std::vector<uint64> &familyIds, std::string &errInfo)
{
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* select F_family_id from tbl_user_family_0 WHERE F_uid = 'aaa'; */
	std::ostringstream sql;
	sql << "select F_family_id from tbl_user_family_"
		<< (userId % 10)
		<< " where F_uid = " << userId
		<< ";";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);

	for (int i = 0; i < row_set.row_count(); i++)
	{
		familyIds.push_back((uint64)atoll(row_set[i][0].c_str()));
	}
	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_get_family_all_user_list_from_sql(const uint64 &familyId, std::vector<uint64> &userIds, uint64 &ownerId, std::string &errInfo)
{
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* select F_uid from tbl_family_member_0 where F_family_id = 1111; */
	std::ostringstream sql;
	sql << "select F_uid from tbl_family_member_"
		<< (familyId % 10)
		<< " where F_family_id = '"
		<< familyId
		<< "';";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set, errInfo);
	CHECK_RET(nRet);

	for (int i = 0; i < row_set.row_count(); i++)
	{
		userIds.push_back((uint64)atoll(row_set[i][0].c_str()));
	}

	/* select F_owner_id from tbl_family_3 where F_family_id = 2013; */
	sql.str("");
	sql << "select F_owner_id from tbl_family_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";

	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1, errInfo);
	CHECK_RET(nRet);
	if(row_set1.row_count() == 0)
	{
		errInfo = "family id is not exist";
		return ERR_FAMILY_ID_IS_NOT_EXIST;
	}
	ownerId = (uint64)atoll(row_set1[0][0].c_str());

	errInfo = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_push_msg(const std::string &method, const std::string &msg_tag, const uint64 userId, const uint64 familyId, const uint64 operatorId, std::string &errInfo)
{
	Conn_Ptr push_conn;
	if (!g_push_conn.get_conn(push_conn))
	{
		XCP_LOGGER_ERROR(&g_logger, "get push conn failed.\n");
		return 0;
	}

	int nRet = push_conn->push_msg(msg_tag, method, userId, familyId, operatorId, errInfo);
	CHECK_RET(nRet);

	errInfo = "OK";
	return ERR_SUCCESS;
}

std::string Family_Mgt::_rand_str(const unsigned int length)
{
	std::string str;
	srand(getTimestamp());

	int size = (length < RANDON_STRING_MAX_LENGTH) ? length : RANDON_STRING_MAX_LENGTH;
	for (int i = 0; i < size; i++)
	{
		//string &append(int n,char c); ----在当前字符串结尾添加n个字符c
		str.append(1, (char)('a' + rand() % 26));
	}

	return str;
}


std::string Family_Mgt::_int_to_string(const uint64 number)
{
    char data[30];
	sprintf(data, "%llu", number);
	
	return data;
}

int Family_Mgt::_generate_token(const std::string &key, TokenInfo &token_info, std::string &token, std::string &err_info)
{
	int nRet = 0;

	if (token_info.token_type <= TokenTypeBegin || token_info.token_type >= TokenTypeEnd)
	{
		err_info = "token_type error!";
		return ERR_SYSTEM;
	}
	unsigned long long timestamp = getTimestamp();
	token_info.create_time = timestamp;
	token_info.expire_time = timestamp + FAMILY_INVITITION_TOKEN_TTL;
	token_info.nonce = _rand_str(TOKEN_NONCE_STRING_LENGTH);

	if (token_info.user_id <= 0 || token_info.phone.empty() || token_info.create_time <= 0 || token_info.nonce.empty())
	{
		err_info = "generate token fail!less must param";
		return ERR_SYSTEM;
	}

	if (token_info.token_type == TokenTypeUpdatePwd)
	{
		if (token_info.pwd.empty() || token_info.salt.empty())
		{
			err_info = "generate token fail!less must param about pwd";
			return ERR_SYSTEM;
		}
	}

	std::string sig;
	if (token_info.token_type != TokenTypeGetAccount)
	{
		_get_sig_token(key, token_info, sig, err_info);
		CHECK_RET(nRet);
	}

	char src[512] = {0};
	if (token_info.token_type == TokenTypeLogin)
	{
		snprintf(src, sizeof(src), "{\"user_id\":%llu,\"phone\":\"%s\",\"token_type\":%d,\"create_time\":%llu,\"expire_time\":%llu,\"from\":\"Server\",\"nonce\":\"%s\",\"sig\":\"%s\"}",
				 token_info.user_id, token_info.phone.c_str(), token_info.token_type, token_info.create_time, token_info.expire_time, token_info.nonce.c_str(), sig.c_str());
	}
	else if (token_info.token_type == TokenTypeGetAccount)
	{
		snprintf(src, sizeof(src), "{\"user_id\":%llu,\"phone\":\"%s\",\"token_type\":%d,\"create_time\":%llu,\"expire_time\":%llu,\"from\":\"Server\",\"nonce\":\"%s\",\"salt\":\"%s\",\"password\":\"%s\"}",
				 token_info.user_id, token_info.phone.c_str(), token_info.token_type, token_info.create_time, token_info.expire_time, token_info.nonce.c_str(), token_info.salt.c_str(), token_info.pwd.c_str());
	}
	else
	{
		snprintf(src, sizeof(src), "{\"user_id\":%llu,\"phone\":\"%s\",\"token_type\":%d,\"create_time\":%llu,\"expire_time\":%llu,\"from\":\"Server\",\"nonce\":\"%s\",\"sig\":\"%s\",\"salt\":\"%s\",\"password\":\"%s\"}",
				 token_info.user_id, token_info.phone.c_str(), token_info.token_type, token_info.create_time, token_info.expire_time, token_info.nonce.c_str(), sig.c_str(), token_info.salt.c_str(), token_info.pwd.c_str());
	}
	//base64
	Base64 base64;
	std::string s_src = src;
	char *tmp = NULL;
	if (base64.encrypt(s_src.c_str(), s_src.size(), tmp) != 0)
	{
		return ERR_BASE64_ENCODE_FAILED;
	}
	token = tmp;
	XCP_LOGGER_INFO(&g_logger, "src token:%s\n", s_src.c_str());
	err_info = "OK";
	return ERR_SUCCESS;
}

int Family_Mgt::_get_sig_token(const std::string &key, const TokenInfo &token_info, std::string &sig, std::string &errInfo)
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
	if (token_info.token_type == TokenTypeUpdatePwd || token_info.token_type == TokenTypeInviteMember)
	{
		m_src.insert(std::make_pair<std::string, std::string>("salt", (std::string)token_info.salt));
		m_src.insert(std::make_pair<std::string, std::string>("password", (std::string)token_info.pwd));
	}

	ss.str("");
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
		XCP_LOGGER_ERROR(&g_logger, "src:%s,key:%s,sig:%s\n", sig_src.c_str(), key.c_str(), sig.c_str());
		errInfo = "hamc encode failed";
		return ERR_HMAC_ENCODE_FAILED;
	}
	XCP_LOGGER_INFO(&g_logger, "src:%s,sig:%s\n", sig_src.c_str(), sig.c_str());
	errInfo = "OK";
	return ERR_SUCCESS;
}
