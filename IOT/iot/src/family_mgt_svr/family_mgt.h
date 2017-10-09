
/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: 89617663@qq.com
 */

 
#ifndef _FAMILY_MGT_H
#define _FAMILY_MGT_H

#include "base/base_common.h"
#include "base/base_singleton_t.h"
#include "base/base_thread_mutex.h"
#include "base/base_base64.h"
#include "base/base_cryptopp.h"
#include "comm/common.h"

#include "conn_mgt_lb.h"
#include "conn.h"
#include "mysql_mgt.h"
#include "redis_mgt.h"

USING_NS_BASE;

#define uint64 unsigned long long

enum talk_msg_type
{
	TALK_MSG_TYPE_P2P = 1,
	TALK_MSG_TYPE_P2R = 2,
	TALK_MSG_TYPE_P2F = 3,
	TALK_MSG_TYPE_R2P = 4,
	TALK_MSG_TYPE_R2F = 5,

	TALK_MSG_TYPE_END
};

enum member_join_apply_result
{
	MEMBER_JOIN_WAITING_APPROVE = 0,
	MEMBER_JOIN_ACCEPT = 1,
	MEMBER_JOIN_REFUSE = 2,

	MEMBER_JOIN_END
};


typedef struct _StFamilyInfo
{
	uint64 familyId;
	uint64 ownerId;
	uint64 createdAt;
	uint64 updateAt;
	uint64 routerId;
	std::string familyName;
	std::string familyAvatar;
	_StFamilyInfo():familyId(0),ownerId(0),createdAt(0),updateAt(0),routerId(0),familyName(""),familyAvatar("")
	{
	}
}StFamilyInfo;

typedef struct _StMemberApplyInfo
{
	uint64 applyId;
	uint64 userId;
	uint64 createdAt;
	uint64 updateAt;
	enum member_join_apply_result state;
}StMemberApplyInfo;

typedef struct _StFamilyMemberInfo
{
	uint64 memberId;
	uint64 createdAt;
	uint64 updateAt;
	enum member_role_in_family role;
	enum member_gender gender;
	std::string name;
	std::string nickName;
}StFamilyMemberInfo;

struct TokenInfo {
	TokenInfo() :from("Server") {};
	unsigned long long user_id;
	std::string phone;
	unsigned long long create_time;
	unsigned long long expire_time;
	std::string from;
	std::string salt;
	std::string pwd;//ԭ���뾭���ͻ���md5����ַ���
	std::string nonce;//�����
	TokenType token_type;//token����
	std::string sig;//ǩ��hmac_sha1�㷨
};


class Family_Mgt
{
public:
	Family_Mgt();

	~Family_Mgt();

public:
	int create_family(const uint64 &userId, const std::string &msg_tag, const StFamilyInfo &info, std::string &token, std::string &errInfo);

	/* "params":{"user_id":123, "family_id":123, "family_name":"", "family_avatar":"" } */
	int update_family(const uint64 &userId, const std::string &msg_tag, const StFamilyInfo &info, std::string &errInfo);

	int member_switch_family(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &errInfo);

	int get_family_info(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, StFamilyInfo &result, std::string &errInfo);

	int get_family_list(const uint64 &userId, const std::string &msg_tag, std::vector<StFamilyInfo> &result, std::string &errInfo);

	int bind_router(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, uint64 &routerId, const std::string &uuid, const std::string &pwd, std::string &errInfo);
	
	int unbind_router(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &errInfo);

	int get_apply_code(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &code, uint64 &timestamp, std::string &errInfo);

	int create_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const std::string &code, std::string &token, std::string &errInfo);

	int get_invitation(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::string &token, std::string &errInfo);
	
	int remove_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const uint64 &memberId, std::string &errInfo);
	
	int update_member(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const uint64 &memberId, const std::string &memberName, std::string &errInfo);

	int get_member_info(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, const uint64 memberId, StFamilyMemberInfo &result, std::string &errInfo);

	int get_member_info_list(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::vector<StFamilyMemberInfo> &result, std::string &errInfo);

	int get_member_id_list(const uint64 &userId, const std::string &msg_tag, const uint64 &familyId, std::vector<uint64> &result, uint64 &routerId, std::string &errInfo);
	
	int check_talk_condition(const uint64 srcId, const uint64 targetId, const std::string &msg_tag, enum talk_msg_type type, std::string &errInfo);

private:
	bool _is_user_joined_family(const uint64 userId, const uint64 familyId);
	int _get_familyInfo_by_id(const uint64 familyId, StFamilyInfo &info, std::string &errInfo);
	int _match_invite_code(const uint64 familyId, const std::string &code, std::string &errInfo);
	int _get_family_id_by_router(const uint64 routerId, uint64 &familyId, std::string &errInfo);
	int _get_members_by_family(const uint64 familyId, std::vector<uint64> &members, uint64 &ownerId, std::string &errInfo);
	int _get_family_ids_by_user(const uint64 userId, std::vector<uint64> &familyIds, std::string &errInfo);
	int _muti_get_family_info(const std::vector<uint64> &familyIds, std::vector<StFamilyInfo> &result, std::string &errInfo);
	
	/*int _get_FamilyInfo_from_Redis(const uint64 &familyId, StFamilyInfo &result);
	int _set_FamilyInfo_to_Redis(const StFamilyInfo &info);
	int _remove_FamilyInfo_from_Redis(const uint64 &familyId);
	int _get_user_joined_family_list_from_Redis(const uint64 &userId, std::vector<uint64> &familyIds);
	int _set_user_joined_family_list_to_Redis(const uint64 &userId, const std::vector<uint64> &familyInfos);
	int _remove_user_joined_family_from_Redis(const uint64 &userId);
 	int _get_family_all_user_list_from_Redis(const uint64 &familyId, std::vector<uint64> &userIds, uint64 &ownerId);
	int _set_family_all_user_list_to_Redis(const uint64 &familyId, const std::vector<uint64> &userIds, const uint64 ownerId);
	int _remove_family_all_user_list_from_Redis(const uint64 &familyId);*/
	
	int _get_FamilyInfo_from_sql(const uint64 &familyId, StFamilyInfo &result, std::string &errInfo);
	int _set_FamilyInfo_to_sql(const StFamilyInfo &info, std::string &errInfo);
	int _update_FamilyInfo_to_sql(const StFamilyInfo &info, std::string &errInfo);
	int _get_user_joined_family_list_from_sql(const uint64 &userId, std::vector<uint64> &familyIds, std::string &errInfo);
	int _get_family_all_user_list_from_sql(const uint64 &familyId, std::vector<uint64> &userIds, uint64 &ownerId, std::string &errInfo);

	int _push_msg(const std::string &method, const std::string &msg_tag, const uint64 userId, const uint64 familyId, const uint64 operatorId, std::string &errInfo);
	std::string _rand_str(const unsigned int length);
	std::string _int_to_string(const uint64 number);
	int _generate_token(const std::string &key, TokenInfo &token_info, std::string &token, std::string &err_info);
	int _get_sig_token(const std::string &key, const TokenInfo &token_info, std::string &sig, std::string &errInfo);
};

#define PSGT_Family_Mgt Singleton_T<Family_Mgt>::getInstance()

#endif


