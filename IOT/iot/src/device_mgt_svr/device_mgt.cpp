
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "device_mgt.h"
#include "redis_mgt.h"
#include "mongo_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;
extern redis_mgt g_redis_mgt;

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
		return ERR_GET_MYSQL_CONN_FAILED;                        \
	}                                                            \
	MySQL_Guard guard_mysql(conn);                               \

#define REDIS_GET_CONN_THEN_LOCK(conn)                           \
	if (!g_redis_mgt.get_conn(conn))                             \
	{                                                            \
		XCP_LOGGER_ERROR(&g_logger, "get redis conn failed.\n"); \
		return ERR_REDIS_OPER_EXCEPTION;                       	 \
	}                                                            \
	Redis_Guard guard_redis(conn);                               \

const std::string REDIS_FAMILY_INFO_PRE_HEAD = "Family:family_";
const std::string REDIS_KEY_FAMILY_INFO_ROUTER = "router";

const unsigned int REDIS_KEY_MAX_LENGTH			= 40;
const unsigned int TOKEN_NONCE_STRING_LENGTH	= 8;
const unsigned int USER_ACCOUNT_TOKEN_TTL		= 3 * 24 * 3600;
const unsigned int RANDON_STRING_MAX_LENGTH		= 25;

const unsigned int MAX_QUERY_DEVICE_STATUS_NUMBER	= 10;

const unsigned int MULTI_PROCESS_MAX_NUMBER		= 10;

Device_Mgt::Device_Mgt()
{

}


Device_Mgt::~Device_Mgt()
{

}

int Device_Mgt::add_room(const uint64 familyId, const std::vector<StRoomInfo> &infos, std::vector<uint64> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add room, nothing to be add\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "add too many room, total number:%d\n", infos.size());
		return ERR_ADD_TOO_MANY_ROOMS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* insert into tbl_room_0
		(F_room_id,F_family_id,F_room_name,F_default,F_disp_order,F_creator,F_created_at,F_updated_at)
	   values
		(10010,2010,'客厅',0,1,123,123456,123456),
		(10011,2010,'餐厅',0,1,123,123456,123456);*/
	std::ostringstream sql;
	sql << "insert into tbl_room_" << (familyId % 10)
		<< " (F_room_id, F_family_id, F_room_name, F_default, F_disp_order, F_creator, F_created_at, F_updated_at) values";
	
	std::vector<StRoomInfo>::const_iterator end = infos.end();
	for (std::vector<StRoomInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		if(it != infos.begin())
		{
			sql << ",";
		}
		sql <<"(" << it->id << ", "
			<< familyId << ", "
			<< "'" << it->name << "', "
			<< it->isDefault << ", "
			<< it->orderNum<< ", "
			<< it->creatorId<< ", "
			<< it->createAt << ", "
			<< it->updateAt << ")";
	}
	sql << ";";
	uint64 last_insert_id = 0, affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add room success, request %d record, affect %d record\n", infos.size(), affected_rows);
		return ERR_SUCCESS;
	}
	XCP_LOGGER_ERROR(&g_logger, "add room failed, request %d record, ret %d\n", infos.size(), nRet);

	// 批量插入失败，尝试逐条插入
	for (std::vector<StRoomInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql.str("");
		sql << "insert into tbl_room_" << (familyId % 10)
			<< " (F_room_id, F_family_id, F_room_name, F_default, F_disp_order, F_creator, F_created_at, F_updated_at) values"
			<<"(" << it->id << ", "
			<< familyId << ", "
			<< "'" << it->name << "', "
			<< it->isDefault << ", "
			<< it->orderNum<< ", "
			<< it->creatorId<< ", "
			<< it->createAt << ", "
			<< it->updateAt << ");";

		int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "add room failed, roomId %d, ret %d\n", it->id, nRet);
			failed.push_back(it->id);
		}
	}
	if(failed.size() != 0)
	{
		return ERR_ADD_ROOMS_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::delete_room(const uint64 familyId, const std::vector<uint64> &roomIds, std::vector<uint64> &failed)
{
	if(roomIds.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add room, nothing to be add\n");
		return ERR_SUCCESS;
	}
	if(roomIds.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "delete too many room, total number:%d\n", roomIds.size());
		return ERR_DELETE_TOO_MANY_ROOMS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* delete from tbl_room_0 where F_room_id in(1, 2); */
	std::ostringstream sql;
	sql << "delete from tbl_room_" << (familyId % 10)
		<< "  where F_room_id in(";
	
	std::vector<uint64>::const_iterator end = roomIds.end();
	for (std::vector<uint64>::const_iterator it = roomIds.begin(); it != end; it++)
	{
		if(it != roomIds.begin())
		{
			sql << ",";
		}
		sql << *it;
	}
	sql << ");";

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "delte room success, request %d record, affect %d record\n", roomIds.size(), affected_rows);
		return ERR_SUCCESS;
	}
	XCP_LOGGER_ERROR(&g_logger, "delte room failed, request %d record, ret %d\n", roomIds.size(), nRet);

	// 批量删除失败，尝试逐条删除
	/* for (std::vector<uint64>::const_iterator it = roomIds.begin(); it != end; it++)
	{
		sql.str("");
		sql << "delete from tbl_room_" << (familyId % 10)
			<< "  where F_room_id =" << *it <<";";

		int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "delete room failed, roomId %d, ret %d\n", *it, nRet);
			failed.push_back(*it);
		}
	} */
	return ERR_DELETE_ROOMS_FAILED;
}

int Device_Mgt::update_room(const uint64 familyId, const std::vector<StRoomInfo> &infos, std::vector<uint64> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "update room, nothing to be update\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "add too many room, total number:%d\n", infos.size());
		return ERR_UPDATE_TOO_MANY_ROOMS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	std::vector<StRoomInfo>::const_iterator end = infos.end();
	uint64 last_insert_id = 0, affected_rows = 0;

	/* update tbl_room_0
		set F_room_name = case F_room_id
		when 10010 then 'test1'
		when 10011 then 'test2'
		end,
		F_updated_at = 1234567890
	   where F_room_id in (10010, 10011);*/
	/*
	sql << "update tbl_room_" << (familyId % 10)
		<< " set F_room_name = case F_room_id";	
	for (std::vector<StRoomInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql <<" when " << it->id << " then '" << it->name << "'";
	}
	sql << " end, F_updated_at = " << getTimestamp() << " where F_room_id in (";
	for (std::vector<StRoomInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		if(it != infos.begin())
		{
			sql << ",";
		}
		sql << it->id;
	}
	sql << ");";
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "update room success, request %d record, affect %d record\n", infos.size(), affected_rows);
		return ERR_SUCCESS;
	}
	XCP_LOGGER_ERROR(&g_logger, "add room failed, request %d record, ret %d\n", infos.size(), nRet); */

	// 更新插入失败，尝试逐条更新
	for (std::vector<StRoomInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql.str("");
		sql << "update tbl_room_" << (familyId % 10)
			<< "  set F_room_name = '" << it->name << "'"
			<< ",F_updated_at = " << it->updateAt
			<< " where F_room_id = " << it->id
			<< ";";

		int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "add room failed, roomId %d, ret %d\n", it->id, nRet);
			failed.push_back(it->id);
		}
	}
	if(failed.size() != 0)
	{
		return ERR_UPDATE_ROOMS_FAILED;
	}	
	return ERR_SUCCESS;
}

// 更新，如果不存在，插入
int Device_Mgt::update_room_new(const uint64 familyId, const std::vector<StRoomInfo> &infos, std::vector<uint64> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "update room, nothing to be update\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "add too many room, total number:%d\n", infos.size());
		return ERR_UPDATE_TOO_MANY_ROOMS;
	}
	failed.clear();

	std::ostringstream sql;
	uint64 last_insert_id = 0, affected_rows = 0;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	std::vector<StRoomInfo>::const_iterator end = infos.end();
	for (std::vector<StRoomInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql.str("");
		sql << "insert into tbl_room_" << (familyId % 10)
			<< " (F_room_id, F_family_id, F_room_name, F_default, F_disp_order, F_creator, F_created_at, F_updated_at) values"
			<<"(" << it->id << ", "
			<< familyId << ", "
			<< "'" << it->name << "', "
			<< it->isDefault << ", "
			<< it->orderNum<< ", "
			<< it->creatorId<< ", "
			<< it->createAt << ", "
			<< it->updateAt << ") on duplicate key update "
			<< "F_room_name = '" << it->name << "',"
			<< "F_updated_at = " << it->updateAt << ";";

		int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "add room failed, roomId %d, ret %d\n", it->id, nRet);
			failed.push_back(it->id);
		}
	}
	if(failed.size() != 0)
	{
		return ERR_UPDATE_ROOMS_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::get_room_list(const uint64 &familyId, std::vector<StRoomInfo> &infos)
{
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* select F_room_id,F_room_name,F_disp_order,F_created_at,F_updated_at from tbl_room_0 where F_family_id = 1017; */
	std::ostringstream sql;
	sql << "select F_room_id,F_room_name,F_disp_order,F_created_at,F_updated_at from tbl_room_"
		<< (familyId % 10)
		<< " where F_family_id = " << familyId
		<< ";";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		StRoomInfo info;
		info.id 		= (uint64)atoll(row_set[i][0].c_str());
		info.familyId 	= familyId;
		info.name 		= row_set[i][1];
		info.orderNum 	= (unsigned int)atoi(row_set[i][2].c_str());
		info.createAt 	= (uint64)atoll(row_set[i][3].c_str());
		info.updateAt	= (uint64)atoll(row_set[i][4].c_str());
		
		infos.push_back(info);
	}
	
	return ERR_SUCCESS;
}


int Device_Mgt::update_room_order(const uint64 &userId, const uint64 &familyId, const std::vector<StRoomInfo> &infos)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	std::ostringstream sql;

	std::vector<StRoomInfo>::const_iterator end = infos.end();
	for(std::vector<StRoomInfo>::const_iterator it=infos.begin(); it != end; it++)
	{
		sql.str("");
		/* update tbl_room_0 set F_disp_order = 1 where F_room_id = 1; */
		sql << "update tbl_room_" << (familyId % 10)
			<< "  set F_disp_order = '" << it->orderNum
			<< "' where F_room_id = " << it->id
			<< ";";

		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
	}
	return ERR_SUCCESS;
}


int Device_Mgt::add_device(const uint64 routerId, const uint64 familyId, const std::vector<StDeviceInfo> &infos, std::vector<std::string> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add device, nothing to be add\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "add too many device, total number:%d\n", infos.size());
		return ERR_ADD_TOO_MANY_DEVICES;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	int nRet = 0;

	/* insert into tbl_device_info_0(F_family_id, F_room_id, F_did, F_device_id, F_device_name,
		F_product_id, F_business_id, F_device_params, F_creator, F_created_at, F_updated_at)
		values
		(10011,2010,'餐厅',0,1,123,123456,123456);*/
	sql << "insert into tbl_room_device_" << (familyId % 10)
		<< " (F_family_id, F_room_id, F_did, F_device_id, F_device_name, F_product_id, F_business_id, F_device_params, F_creator, F_created_at, F_updated_at) values";
	
	std::vector<StDeviceInfo>::const_iterator end = infos.end();
	for (std::vector<StDeviceInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		if(it != infos.begin())
		{
			sql << ",";
		}
		sql <<"(" << familyId << ", "
			<< it->roomId << ", "
			<< it->id << ", "
			<< "'" << it->deviceId << "', "
			<< "'" << it->name << "', "
			<< it->productId << ", "
			<< it->businessId << ", "
			<< "'" << it->attr << "', "
			<< it->creatorId<< ", "
			<< it->createAt << ", "
			<< it->updateAt << ")";
	}
	sql << ";";
	uint64 last_insert_id = 0, affected_rows = 0;
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add room success, request %d record, affect %d record\n", infos.size(), affected_rows);
		return ERR_SUCCESS;
	}
	XCP_LOGGER_ERROR(&g_logger, "add room failed, request %d record, ret %d\n", infos.size(), nRet);

	for (std::vector<StDeviceInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		/* insert into tbl_device_info_0(F_did, F_router_id, F_product_id, F_business_id, 
		F_device_name, F_device_id, F_device_params, F_created_at, F_updated_at)
		values (1000, 123, 123, 123, '1号灯', 1, 'test', 123456, 123456); */
		sql.str("");
		sql << "insert into tbl_room_device_" << (familyId % 10)
			<< " (F_family_id, F_room_id, F_did, F_device_id, F_device_name, F_product_id, F_business_id, F_device_params, F_creator, F_created_at, F_updated_at) values"
			<<" (" << familyId << ", "
			<< it->roomId << ", "
			<< it->id << ", "
			<< "'" << it->deviceId << "', "
			<< "'" << it->name << "', "
			<< it->productId << ", "
			<< it->businessId << ", "
			<< "'" << it->attr << "', "
			<< it->creatorId<< ", "
			<< it->createAt << ", "
			<< it->updateAt << ");";

		uint64 last_insert_id = 0, affected_rows = 0;
		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows );
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "update device failed, device id %s, ret %d\n", it->deviceId.c_str(), nRet);
			failed.push_back(it->deviceId);
			continue;
		}
	}
	if(failed.size() != 0)
	{
		return ERR_ADD_DEVICES_FAILED;
	}	
	return ERR_SUCCESS;
}


int Device_Mgt::delete_device(const uint64 routerId, const uint64 familyId, const std::vector<uint64> &deviceIds, std::vector<uint64> &failed)
{
	if(deviceIds.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "delete device, nothing to be delete\n");
		return ERR_SUCCESS;
	}
	if(deviceIds.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "delete too many device, total number:%d\n", deviceIds.size());
		return ERR_DELETE_TOO_MANY_DEVICES;
	}
	failed.clear();
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	int nRet = 0;
	uint64 last_insert_id = 0, affected_rows = 0;
	std::ostringstream sql;

	sql << "delete from tbl_room_device_" << (familyId % 10)
		<< "  where F_did in(";
	std::vector<uint64>::const_iterator end = deviceIds.end();
	for (std::vector<uint64>::const_iterator it = deviceIds.begin(); it != end; it++)
	{
		if(it != deviceIds.begin())
		{
			sql << ",";
		}
		sql << *it;
	}
	sql << ") and F_family_id = " << familyId << ";";

	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "delete device failed, ret %d\n", nRet);
		return nRet;
	}
	return ERR_SUCCESS;
}

int Device_Mgt::update_device(const uint64 routerId, const uint64 familyId, const std::vector<StDeviceInfo> &infos, std::vector<std::string> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "update device, nothing to be update\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "update too many device, total number:%d\n", infos.size());
		return ERR_UPDATE_TOO_MANY_SHORTCUTS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	int nRet = 0;
	uint64 last_insert_id = 0, affected_rows = 0;

	std::vector<StDeviceInfo>::const_iterator end = infos.end();
	for (std::vector<StDeviceInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		/* update tbl_room_device_0 set F_room_id = 22, F_device_name = 'a', F_updated_at = 123455
			where F_did = 1000 and F_family_id = 1001; */
		sql.str("");
		sql << "update tbl_room_device_" << (familyId % 10)
			<< " set F_updated_at = " << it->updateAt;
		if(it->roomId != 0)
		{
			sql << ", F_room_id = " << (it->roomId);
		}
		if(!it->name.empty())
		{
			sql << ", F_device_name = '" << (it->name) << "'";
		}
		if(it->businessId != 0)
		{
			sql << ", F_business_id = " << (it->businessId);
		}
		if(it->productId != 0)
		{
			sql << ", F_product_id = " << (it->productId);
		}
		if(!it->attr.empty())
		{
			sql << ", F_device_params = '" << (it->attr) << "'";
		}
		sql << " where F_did = " << it->id << " and F_family_id = " << familyId << ";";
		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "update device failed, device id %s, ret %d\n", it->deviceId.c_str(), nRet);
			failed.push_back(it->deviceId);
			continue;
		}
	}
	
	if(failed.size() != 0)
	{
		return ERR_UPDATE_DEVICES_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::update_device_new(const uint64 routerId, const uint64 familyId, const std::vector<StDeviceInfo> &infos, std::vector<std::string> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add device, nothing to be add\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "add too many device, total number:%d\n", infos.size());
		return ERR_ADD_TOO_MANY_DEVICES;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	int nRet = 0;
	std::vector<StDeviceInfo>::const_iterator end = infos.end();
	for (std::vector<StDeviceInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql.str("");
		sql << "insert into tbl_room_device_" << (familyId % 10)
			<< " (F_family_id, F_room_id, F_did, F_device_id, F_device_name, F_product_id, F_business_id, F_device_params, F_creator, F_created_at, F_updated_at) values"
			<<"(" << familyId << ", "
			<< it->roomId << ", "
			<< it->id << ", "
			<< "'" << it->deviceId << "', "
			<< "'" << it->name << "', "
			<< it->productId << ", "
			<< it->businessId << ", "
			<< "'" << it->attr << "', "
			<< it->creatorId<< ", "
			<< it->createAt << ", "
			<< it->updateAt << ")"
			<< " on duplicate key update F_updated_at = " << it->updateAt
			<< ", F_room_id = " << (it->roomId)
			<< ", F_business_id = " << (it->businessId)
			<< ", F_device_name = '" << (it->name) << "'"
			<< ", F_product_id = " << (it->productId)
			<< ", F_device_params = '" << (it->attr) << "';";

		uint64 last_insert_id = 0, affected_rows = 0;
		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows );
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "add device failed, uuid:%s, ret:%d\n", it->deviceId.c_str(), nRet);
			failed.push_back(it->deviceId);
			continue;
		}
	}
	if(failed.size() != 0)
	{
		return ERR_UPDATE_DEVICES_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::get_device_info(const uint64 &userId, StDeviceInfo &info)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;

	/* select F_product_id, F_business_id, F_device_name, F_device_id, F_device_params,
		F_created_at, F_updated_at from tbl_device_info_0 where F_did = 1000; */
	sql << "select F_room_id, F_product_id, F_business_id, F_device_name, F_device_id, F_device_params, F_creator, F_created_at, F_updated_at "
		<< "from tbl_room_device_" << (info.familyId % 10)
		<< " where F_did = " << info.id << "and F_family_id = " << info.familyId << ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "device id is not exists: %d.\n", info.id);
		return ERR_DEVICE_ID_NOT_EXIST;
	}

	int i = 0;
	info.roomId 	= (uint64)atoll(row_set[0][i++].c_str());
	info.productId	= (enum DM_DEVICE_CATEGORY)(atoi(row_set[0][i++].c_str()));
	info.businessId = (uint64)atoll(row_set[0][i++].c_str());
	info.name		= row_set[0][i++];
	info.deviceId	= row_set[0][i++];
	info.attr		= row_set[0][i++];
	info.creatorId 	= (uint64)atoll(row_set[0][i++].c_str());
	info.createAt	= (uint64)atoll(row_set[0][i++].c_str());
	info.updateAt	= (uint64)atoll(row_set[0][i++].c_str());

	return ERR_SUCCESS;
}


int Device_Mgt::get_devices_by_room(const uint64 &userId, const uint64 &familyId, const uint64 &roomId, const uint64 &routerId, enum DM_DEVICE_CATEGORY category,
			std::vector<StDeviceInfo> &infos, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;
	
	/* select F_did, F_creator, F_room_id from tbl_room_device_0
		where F_family_id = 1017 and F_room_id = 22 and F_product_id = 1 limit 0,10; */
	sql << "select F_did, F_product_id, F_business_id, F_device_name, F_device_id, F_device_params, F_creator, F_created_at, F_updated_at "
		<< "from tbl_room_device_" << (familyId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_room_id = " << roomId;	
	if(category != DM_DEVICE_CATEGORY_END)
	{
		sql << " and F_product_id = " << category;
	}
	if(size != 0)
	{
		sql << " limit " << beginAt << "," << size;
	}
	sql << ";";
		
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		int j = 0;
		StDeviceInfo info;
		info.familyId	= familyId;
		info.roomId		= roomId;
		info.routerId	= routerId;
		info.id 		= (uint64)atoll(row_set[i][j++].c_str());
		info.productId	= (enum DM_DEVICE_CATEGORY)(atoi(row_set[i][j++].c_str()));
		info.businessId = (uint64)atoll(row_set[i][j++].c_str());
		info.name		= row_set[i][j++];
		info.deviceId	= row_set[i][j++];
		info.attr		= row_set[i][j++];
		info.creatorId 	= (uint64)atoll(row_set[i][j++].c_str());
		info.createAt	= (uint64)atoll(row_set[i][j++].c_str());
		info.updateAt	= (uint64)atoll(row_set[i][j++].c_str());
		
		infos.push_back(info);
	}

	left = 0;
	if(size != 0)
	{
		unsigned int count = row_set.row_count();
		XCP_LOGGER_INFO(&g_logger, "device total num:%d\n", count);
		left = (count > (size + beginAt))? (count - size - beginAt) : 0;
		CHECK_RET(nRet);
	}

	return ERR_SUCCESS;
}


int Device_Mgt::get_devices_by_family(const uint64 &userId, const uint64 &familyId, const uint64 &routerId, enum DM_DEVICE_CATEGORY category,
			std::vector<StDeviceInfo> &infos, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	int nRet = 0;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;
	
	/* select F_did, F_creator, F_room_id from tbl_room_device_0
		where F_family_id = 1017 and F_room_id = 22 and F_product_id = 1 limit 0,10; */
	sql << "select F_room_id, F_did, F_product_id, F_business_id, F_device_name, F_device_id, F_device_params, F_creator, F_created_at, F_updated_at "
		<< "from tbl_room_device_" << (familyId % 10)
		<< " where F_family_id = " << familyId;
	if(category != DM_DEVICE_CATEGORY_END)
	{
		sql << " and F_product_id = " << category;
	}
	if(size != 0)
	{
		sql << " limit " << beginAt << "," << size;
	}
	sql << ";";
		
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		int j = 0;
		StDeviceInfo info;
		info.familyId	= familyId;
		info.routerId	= routerId;
		info.roomId		= (uint64)atoll(row_set[i][j++].c_str());;
		info.id 		= (uint64)atoll(row_set[i][j++].c_str());
		info.productId	= (enum DM_DEVICE_CATEGORY)(atoi(row_set[i][j++].c_str()));
		info.businessId = (uint64)atoll(row_set[i][j++].c_str());
		info.name		= row_set[i][j++];
		info.deviceId	= row_set[i][j++];
		info.attr		= row_set[i][j++];
		info.creatorId 	= (uint64)atoll(row_set[i][j++].c_str());
		info.createAt	= (uint64)atoll(row_set[i][j++].c_str());
		info.updateAt	= (uint64)atoll(row_set[i][j++].c_str());
		
		infos.push_back(info);
	}

	left = 0;
	if(size != 0)
	{
		unsigned int count = row_set.row_count();
		XCP_LOGGER_INFO(&g_logger, "device total num:%d\n", count);
		left = (count > (size + beginAt))? (count - size - beginAt) : 0;
		CHECK_RET(nRet);
	}

	return ERR_SUCCESS;
}


int Device_Mgt::report_status(const uint64 familyId, const std::vector<StDeviceStatus> &statues)
{
	std::vector<StDeviceStatus>::const_iterator end = statues.end();
	for(std::vector<StDeviceStatus>::const_iterator it = statues.begin(); it != end; it++)
	{
		_StDeviceStatusMsg msg;
		msg._date = FormatDateTimeStr(it->createAt, "%Y%m%d");
		msg._id = it->deviceId;

		char buf[512] = {0};
		memset(buf, 0x0, 512); 
		snprintf(buf, 512, "{\"device_category_id\":%u, \"device_id\": %llu, \"device_uuid\": \"%s\", \"family_id\": %llu, \"report_msg\":%s, \"report_created_at\":%llu}",
			it->type, it->deviceId, it->device_uuid.c_str(), familyId, it->msg.c_str(), it->createAt);
		msg._msg = buf;

		PSGT_Mongo_Mgt->storage(msg);
	}
	return ERR_SUCCESS;
}


int Device_Mgt::report_alert(const StDeviceAlert &alert, uint64 &alertId)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* insert into t_device_alert_0 (F_did, F_alert_id, F_family_id, F_alert_msg,
		F_handler, F_alert_staus, F_created_at, F_handle_at)
		values (1000, 1, 1017, 'test', 12, 0, 12345, 23456); */
	std::ostringstream sql;
	sql << "insert into t_device_alert_" << (alert.deviceId % 10)
		<< " (F_did, F_alert_id, F_family_id, F_alert_msg, F_handler, F_alert_staus, F_created_at, F_handle_at) values("
		<< alert.deviceId << ", "
		<< alert.type << ", "
		<< alert.familyId << ", "
		<< "'" << alert.msg << "', "
		<< alert.handler << ", "
		<< alert.status<< ", "
		<< alert.createAt << ", "
		<< alert.handleAt << ");";

	uint64 last_insert_id = 0;
	nRet = conn->execute_sql(sql.str(), last_insert_id, alertId);
	CHECK_RET(nRet);
	
	return ERR_SUCCESS;
}


int Device_Mgt::get_dev_status_list(const uint64 &familyId, const uint64 &deviceId, const std::string &date,
	std::deque<std::string> &status, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	long long total;
	std::string err_info;
	if(MAX_QUERY_DEVICE_STATUS_NUMBER < size)
	/* std::string date_str = FormatDateTimeStr(date, "%Y%m%d"); */
	PSGT_Mongo_Mgt->get_device_status_message(familyId, deviceId, date, 0, 0, status, total, err_info);

	return ERR_SUCCESS;
}

int Device_Mgt::get_dev_alert_list(const uint64 &userId, const uint64 &deviceId,
	std::vector<StDeviceAlert> &alerts, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;
	
	/* select F_id, F_alert_id, F_family_id, F_alert_msg, F_handler, F_alert_staus, F_created_at, F_handle_at
		from t_device_alert_0 where F_did = 1000 limit 0,10; */
	sql << "select F_id, F_alert_id, F_family_id, F_alert_msg, F_handler, F_alert_staus, F_created_at, F_handle_at from t_device_alert_" << (deviceId % 10)
		<< " where F_did = " << deviceId;
	if(size != 0)
	{
		sql << " limit " << beginAt << "," << size;
	}
	sql << ";";
		
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		StDeviceAlert alert;
		alert.id 		= (uint64)atoll(row_set[i][0].c_str());
		alert.deviceId	= deviceId;
		alert.type		= (enum DM_DEVICE_ALERT_TYPE)(atoi(row_set[i][1].c_str()) % DM_DEVICE_ALERT_TYPE_END);
		alert.familyId	= (uint64)atoll(row_set[i][2].c_str());
		alert.msg		= row_set[i][3];
		alert.handler	= (uint64)atoll(row_set[i][4].c_str());
		alert.status	= (enum DM_DEVICE_ALERT_STATUS)(atoi(row_set[i][5].c_str()) % DM_DEVICE_ALERT_STATUS_END);
		alert.createAt	= (uint64)atoll(row_set[i][6].c_str());
		alert.handleAt	= (uint64)atoll(row_set[i][7].c_str());
		
		alerts.push_back(alert);
	}

	left = 0;
	if(size != 0)
	{
		unsigned int count = row_set.row_count();
		XCP_LOGGER_INFO(&g_logger, "alert total num:%d\n", count);
		left = (count > (size + beginAt))? (count - size - beginAt) : 0;
		CHECK_RET(nRet);
	}
	
	return ERR_SUCCESS;
}


int Device_Mgt::router_auth(const std::string &uuid, const std::string &pwd, uint64 &routerId)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;

	/* select F_rid from tbl_router_map where F_device_id = '1'; */
	sql << "select F_rid from tbl_router_map where F_device_id = '" << uuid << "';";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find uuid:%s in database\n", uuid.c_str());
		return ERR_ROUTER_UUID_NOT_EXIST;
	}
	routerId = (uint64)atoll(row_set[0][0].c_str());
	
	/* select F_device_auth_key from tbl_router_0 where F_router_id = 1; */
	sql.str("");
	sql << "select F_device_auth_key from tbl_router_" << routerId % 10
		<< " where F_router_id = " << routerId
		<< ";";
	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1);
	CHECK_RET(nRet);
	if(pwd != row_set1[0][0])
	{
		XCP_LOGGER_ERROR(&g_logger, "pwd is not correct, %s <---> %s\n", pwd.c_str(), row_set1[0][0].c_str());
		//XCP_LOGGER_ERROR(&g_logger, "pwd is not correct\n");
		return ERR_ROUTER_PWD_NOT_CORRECT;
	}
	
	return ERR_SUCCESS;
}

int Device_Mgt::router_bind(const uint64 userId, const uint64 familyId, const uint64 routerId)
{
	XCP_LOGGER_INFO(&g_logger, "start bind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);

	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	// TODO: 这个地方要不要检查，如果检查，那么要考虑路由器已经绑定成功云端又不让成功，有不一致问题，如果不检查，重复绑定怎么办？
	uint64 bindFamilyId = 0;
	nRet = _get_family_id_by_router_id(routerId, bindFamilyId);
	CHECK_RET(nRet);
	if(bindFamilyId == familyId)
	{
		XCP_LOGGER_INFO(&g_logger, "the family have bind this router before\n", userId, familyId, routerId);
		return ERR_SUCCESS;
	}
	if(bindFamilyId != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "router have been bind for family:%llu\n", bindFamilyId);
		return ERR_DEVICE_FAMILY_HAVA_BIND_ROUTER;
	}
	
	MySQL_Row_Set row_set;
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	std::ostringstream sql;

	sql << "select F_owner_id, F_router_id from tbl_family_" << (familyId % 10)
		<<" where F_family_id = " << familyId << ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find family id:%llu\n", familyId);
		return ERR_DEVICE_FAMILY_ID_IS_NOT_EXIST;
	}
	uint64 ownerId  = (uint64)atoll(row_set[0][0].c_str());
	uint64 bind_routerId = (uint64)atoll(row_set[0][1].c_str());
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only family owner(id:%llu) can bind router\n", ownerId);
		// TODO: 要不要检查是不是户主操作
		return ERR_DEVICE_USER_IS_NOT_OWNER;
	}
	if(bind_routerId != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "this family have bind router(%llu) before\n", bind_routerId);
		return ERR_DEVICE_FAMILY_HAVA_BIND_ROUTER;
	}

	// 关闭自动提交，绑定过程出错需要回滚数据
	conn->autocommit(false);

	/* update tbl_family_0 set F_router_id = 1 where F_family_id = 10; */
	sql.str("");
	sql << "update tbl_family_" << (familyId % 10)
		<< " set F_router_id = " << routerId
		<< " where F_family_id = " << familyId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET_ROLL_BACK(nRet, conn);
	
	// 更新路由器信息表
	/* update tbl_router_0 set F_family_id = 2013 where F_router_id = 1001; */
	sql.str("");
	sql << "update tbl_router_" << (routerId % 10)
		<< " set F_family_id = " << familyId
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET_ROLL_BACK(nRet, conn);
	
	conn->commit();
	conn->autocommit(true);

	// 删除缓存
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_INFO_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;
	nRet = conn_redis->remove(redis_key);
	XCP_LOGGER_INFO(&g_logger, "remove redis_key(%s), nRet = %d.\n", redis_key.c_str(), nRet);
	
	return ERR_SUCCESS;
}

int Device_Mgt::router_unbind(const uint64 userId, const uint64 familyId, const uint64 routerId)
{
	XCP_LOGGER_INFO(&g_logger, "start unbind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);
	
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	std::ostringstream sql;
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;

	// 检查是不是户主操作；检查家庭是不是绑定的这个路由器
	MySQL_Row_Set row_set;
	sql << "select F_owner_id, F_router_id from tbl_family_" << (familyId % 10)
		<<" where F_family_id = " << familyId << ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find family id:%llu\n", familyId);
		return ERR_DEVICE_FAMILY_ID_IS_NOT_EXIST;
	}
	uint64 ownerId  = (uint64)atoll(row_set[0][0].c_str());
	uint64 bind_routerId = (uint64)atoll(row_set[0][1].c_str());
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only family owner(id:%llu) can unbind router\n", ownerId);
		// TODO: 要不要检查是不是户主操作
		//return ERR_DEVICE_USER_IS_NOT_OWNER;
	}
	if(routerId != bind_routerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "family donot bind on this router, but %llu\n", bind_routerId);
		return ERR_ROUTER_UNBIND_FAMILY_ID_NOT_CORRECT;
	}
	
	// 关闭自动提交，绑定过程出错需要回滚数据
	conn->autocommit(false);

	/* update tbl_family_0 set F_router_id = 1 where F_family_id = 10; */
	sql.str("");
	sql << "update tbl_family_" << (familyId % 10)
		<< " set F_router_id = 0 where F_family_id = " << familyId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET_ROLL_BACK(nRet, conn);

	// 更新路由器信息表
	/* update tbl_router_0 set F_family_id = 0 where F_router_id = 1001; */
	sql.str("");
	sql << "update tbl_router_" << (routerId % 10)
		<< " set F_family_id = null where F_router_id = " << routerId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET_ROLL_BACK(nRet, conn);

	conn->commit();
	conn->autocommit(true);
	
	sql.str("");
	sql << "delete from tbl_room_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	XCP_LOGGER_INFO(&g_logger, "unbind router, delete %d room record\n", affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "unbind router, delete room data failed, nRet:%d\n", nRet);
	}
	/* delete from tbl_room_device_3 where F_family_id = 2013; */
	sql.str("");
	sql << "delete from tbl_room_device_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	XCP_LOGGER_INFO(&g_logger, "unbind router, delete %d device record\n", affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "unbind router, delete device data failed, nRet:%d\n", nRet);
	}
	/* delete from tbl_shortcut_3 where F_family_id = 2013; */
	sql.str("");
	sql << "delete from tbl_shortcut_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	XCP_LOGGER_INFO(&g_logger, "unbind router, delete %d shortcut record\n", affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "unbind router, delete shortcut data failed, nRet:%d\n", nRet);
	}

	// 删除缓存
	redis_conn_Ptr conn_redis;
	REDIS_GET_CONN_THEN_LOCK(conn_redis);
	char buf[REDIS_KEY_MAX_LENGTH] = {0};
	memset(buf, 0x0, REDIS_KEY_MAX_LENGTH);
	snprintf(buf, REDIS_KEY_MAX_LENGTH, "%s%d", REDIS_FAMILY_INFO_PRE_HEAD.c_str(), familyId);
	std::string redis_key = buf;
	nRet = conn_redis->remove(redis_key);
	XCP_LOGGER_INFO(&g_logger, "remove redis_key(%s), nRet = %d.\n", redis_key.c_str(), nRet);
	
	return ERR_SUCCESS;
}

int Device_Mgt::router_get_info(const uint64 &routerId, StRouterInfo &info)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;

	/* select F_family_id, F_device_name, F_device_id, F_device_sn, F_device_mac, F_device_state,
		F_device_params, F_created_at from tbl_router_1 where F_router_id = 1001; */
	sql << "select F_family_id, F_device_name, F_device_id, F_device_sn, F_device_mac, F_device_state, F_device_params, F_created_at "
		<< "from tbl_router_" << (routerId % 10)
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "router id is not exists: %d.\n", routerId);
		return ERR_ROUTER_ID_NOT_EXIST;
	}

	info.routerId	= routerId;
	info.familyId	= (uint64)atoll(row_set[0][0].c_str());
	info.name		= row_set[0][1];
	info.uuid		= row_set[0][2];
	info.snNo		= row_set[0][3];
	info.mac		= row_set[0][4];
	info.state		= (unsigned int)atoi(row_set[0][5].c_str());
	info.attr		= row_set[0][6];
	info.creatAt	= (uint64)atoll(row_set[0][7].c_str());

	return ERR_SUCCESS;
}

int Device_Mgt::router_get_user_account(const uint64 &routerId, const std::vector<uint64> &userIds, std::vector<std::string> &tokens, std::vector<uint64> &failed)
{
	int nRet = 0;
	uint64 familyId = 0, ownerId = 0;
	std::vector<uint64> members;
	nRet = _get_family_id_by_router_id(routerId, familyId);
	CHECK_RET(nRet);
	nRet = _get_user_list_by_family_id(familyId, members, ownerId);
	CHECK_RET(nRet);

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	failed.clear();

	std::vector<uint64>::const_iterator end = userIds.end();
	for (std::vector<uint64>::const_iterator it_user = userIds.begin(); it_user != end; it_user++)
	{
		uint64 userId = *it_user;
		// 判断用户是否属于路由器绑定的家庭
		std::vector<uint64>::iterator it = std::find(members.begin(), members.end(), userId);
		if(it == members.end() && userId != ownerId)
		{
			XCP_LOGGER_ERROR(&g_logger, "user(%llu) donnot included in family(%llu).\n", userId, familyId);
			failed.push_back(userId);
			continue;
		}

		// 查询用户的账户信息
		sql.str("");
		sql << "select F_phone_num, F_password, F_salt from tbl_user_info_" << (userId % 10)
			<< " where F_uid = " << userId << ";";

		MySQL_Row_Set row_set;
		nRet = conn->query_sql(sql.str(), row_set);
		if(nRet != 0 || row_set.row_count() == 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "can not get user(%llu) info from sql, ret = %d.\n", userId, nRet);
			failed.push_back(userId);
			continue;
		}

		std::string errInfo;
		TokenInfo _token;
		std::string token;
		_token.user_id	= userId;  
		_token.phone	= row_set[0][0];
		_token.pwd		= row_set[0][1];
		_token.salt		= row_set[0][2];
		_token.token_type	= TokenTypeGetAccount;
		nRet = _generate_token("", _token, token, errInfo);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "can not get user(%llu) info from sql, ret = %d.\n", userId, nRet);
			failed.push_back(userId);
			continue;
		}
		tokens.push_back(token);
	} 

	if(!failed.empty())
	{
		return ERR_ROUTER_GET_USERS_ACCOUNT_FAILED;
	}
	return ERR_SUCCESS;
}

int Device_Mgt::add_shortcut(const uint64 familyId, const std::vector<StShortCutInfo> &infos, std::vector<uint64> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add shortcut, nothing to be add\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "add too many shortcut, total number:%d\n", infos.size());
		return ERR_ADD_TOO_MANY_SHORTCUTS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* insert into tbl_room_0
		(F_room_id,F_family_id,F_room_name,F_default,F_disp_order,F_creator,F_created_at,F_updated_at)
	   values
		(10010,2010,'客厅',0,1,123,123456,123456),
		(10011,2010,'餐厅',0,1,123,123456,123456);*/
	std::ostringstream sql;
	sql << "insert into tbl_shortcut_" << (familyId % 10)
		<< "(F_shortcut_id, F_family_id, F_room_id, F_name, F_flag, F_disp_order, F_data) values";
	
	std::vector<StShortCutInfo>::const_iterator end = infos.end();
	for (std::vector<StShortCutInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		if(it != infos.begin())
		{
			sql << ",";
		}
		sql << "(" << it->id << ", "
			<< familyId << ", "
			<< it->roomId << ", "
			<< "'" << it->name << "', "
			<< it->flag << ", "
			<< it->order << ", "
			<< "'" << it->data << "')";
	}
	sql << ";";
	uint64 last_insert_id = 0, affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add shortcut success, request %d record, affect %d record\n", infos.size(), affected_rows);
		return ERR_SUCCESS;
	}
	XCP_LOGGER_ERROR(&g_logger, "add shortcut failed, request %d record, ret %d\n", infos.size(), nRet);

	// 批量插入失败，尝试逐条插入
	for (std::vector<StShortCutInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql.str("");
		sql << "insert into tbl_shortcut_" << (familyId % 10)
			<< "(F_shortcut_id, F_family_id, F_room_id, F_name, F_flag, F_disp_order, F_data) values("
			<< it->id << ", "
			<< familyId << ", "
			<< it->roomId << ", "
			<< "'" << it->name << "', "
			<< it->flag << ", "
			<< it->order << ", "
			<< "'" << it->data << "')";

		int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "add shortcut failed, shortcut id %d, ret %d\n", it->id, nRet);
			failed.push_back(it->id);
		}
	}
	if(failed.size() != 0)
	{
		return ERR_ADD_SHORTCUTS_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::update_shortcut(const uint64 familyId, const std::vector<StShortCutInfo> &infos, std::vector<uint64> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "update shortcut, nothing to be add\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "update too many shortcut, total number:%d\n", infos.size());
		return ERR_UPDATE_TOO_MANY_SHORTCUTS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	std::ostringstream sql;
	int nRet = 0;

	std::vector<StShortCutInfo>::const_iterator end = infos.end();
	for (std::vector<StShortCutInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql.str("");
		if(it->name.empty() && it->data.empty())
		{
			XCP_LOGGER_INFO(&g_logger, "nothing will be update\n");
			continue;
		}
		
		/* update tbl_user_shortcut_0 set F_disp_order = 2, F_data = 'aaaaaa' where F_id = 1 and F_family_id = 1001; */
		sql << "update tbl_shortcut_" << (familyId % 10) << " set ";
		
		bool needComma = false;	//开头是否需要逗号
		if(!it->data.empty())
		{
			sql << "F_data = '" << (it->data) << "'";
			needComma = true;
		}
		if(!it->name.empty())
		{
			if(needComma)
			{
				sql << ", ";
			}
			sql << "F_name = '" << (it->name) << "'";
			needComma = true;
		}
		sql	<< " where F_shortcut_id = " << it->id << " and F_family_id = " << familyId << ";";

		uint64 last_insert_id = 0;
		uint64 affected_rows = 0;
		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "delete shortcut failed, shortcut id %d, ret %d\n", it->id, nRet);
			failed.push_back(it->id);
		}
	}
	
	if(failed.size() != 0)
	{
		return ERR_UPDATE_SHORTCUTS_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::update_shortcut_new(const uint64 familyId, const std::vector<StShortCutInfo> &infos, std::vector<uint64> &failed)
{
	if(infos.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "add shortcut, nothing to be add\n");
		return ERR_SUCCESS;
	}
	if(infos.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "add too many shortcut, total number:%d\n", infos.size());
		return ERR_ADD_TOO_MANY_SHORTCUTS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	std::ostringstream sql;
	uint64 last_insert_id = 0, affected_rows = 0;
	std::vector<StShortCutInfo>::const_iterator end = infos.end();
	for (std::vector<StShortCutInfo>::const_iterator it = infos.begin(); it != end; it++)
	{
		sql.str("");
		sql << "insert into tbl_shortcut_" << (familyId % 10)
			<< "(F_shortcut_id, F_family_id, F_room_id, F_name, F_flag, F_disp_order, F_data) values("
			<< it->id << ", "
			<< familyId << ", "
			<< it->roomId << ", "
			<< "'" << it->name << "', "
			<< it->flag << ", "
			<< it->order << ", "
			<< "'" << it->data << "') on duplicate key update"
			<< " F_data = '" << (it->data) << "',"
			<< "F_name = '" << (it->name) << "';";

		int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "add shortcut failed, shortcut id %d, ret %d\n", it->id, nRet);
			failed.push_back(it->id);
		}
	}
	if(failed.size() != 0)
	{
		return ERR_UPDATE_SHORTCUTS_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::update_shortcut_order(const uint64 &familyId, const std::vector<StShortCutInfo> &shortcuts)
{
	int nRet;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);
	
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	std::ostringstream sql;

	std::vector<StShortCutInfo>::const_iterator end = shortcuts.end();
	for(std::vector<StShortCutInfo>::const_iterator it=shortcuts.begin(); it != end; it++)
	{
		sql.str("");
		/* update tbl_user_shortcut_0 set F_disp_order = 1 where F_id = 1; */
		sql << "update tbl_shortcut_" << (familyId % 10)
			<< "  set F_disp_order = " << it->order
			<< " where F_shortcut_id = " << it->id
			<< " and F_family_id = " << familyId
			<< ";";

		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
	}
	return ERR_SUCCESS;
}

int Device_Mgt::remove_shortcut(const uint64 familyId, const std::vector<uint64> &shortcutIds, std::vector<uint64> &failed)
{
	if(shortcutIds.size() == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "remove shortcut, nothing to be remove\n");
		return ERR_SUCCESS;
	}
	if(shortcutIds.size() > MULTI_PROCESS_MAX_NUMBER)
	{
		XCP_LOGGER_ERROR(&g_logger, "remove too many shortcut, total number:%d\n", shortcutIds.size());
		return ERR_DELETE_TOO_MANY_ROOMS;
	}
	failed.clear();

	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* delete from tbl_shortcut_0 where F_family_id = 1001 and F_id in(1, 2); */
	std::ostringstream sql;
	sql << "delete from tbl_shortcut_" << (familyId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_shortcut_id in(";
	
	std::vector<uint64>::const_iterator end = shortcutIds.end();
	for (std::vector<uint64>::const_iterator it = shortcutIds.begin(); it != end; it++)
	{
		if(it != shortcutIds.begin())
		{
			sql << ",";
		}
		sql << *it;
	}
	sql << ");";

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet == 0)
	{
		XCP_LOGGER_INFO(&g_logger, "delte shortcut success, request %d record, affect %d record\n", shortcutIds.size(), affected_rows);
		return ERR_SUCCESS;
	}
	XCP_LOGGER_ERROR(&g_logger, "add shortcut failed, request %d record, ret %d\n", shortcutIds.size(), nRet);

	// 批量删除失败，尝试逐条删除
	for (std::vector<uint64>::const_iterator it = shortcutIds.begin(); it != end; it++)
	{
		sql.str("");
		sql << "delete from tbl_shortcut_" << (familyId % 10)
			<< " where F_family_id = " << familyId
			<< " and F_shortcut_id = " << *it << ";";

		int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			XCP_LOGGER_ERROR(&g_logger, "delete shortcut failed, shortcut id %d, ret %d\n", *it, nRet);
			failed.push_back(*it);
		}
	}
	if(failed.size() != 0)
	{
		return ERR_DELETE_SHORTCUTS_FAILED;
	}	
	return ERR_SUCCESS;
}

int Device_Mgt::get_list_shortcut(const uint64 userId, const uint64 familyId, const uint64 roomId, std::vector<StShortCutInfo> &shortcuts)
{
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	/* select F_id, F_name, F_flag, F_disp_order, F_data from tbl_user_shortcut_0 where F_family_id = 2013 and F_room_id = 1 and F_uid = 2010; */
	std::ostringstream sql;
	sql << "select F_shortcut_id, F_name, F_flag, F_disp_order, F_data from tbl_shortcut_" << (familyId % 10)
		<< " where F_family_id = " << familyId
		<< " and F_room_id = " << roomId << ";";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		StShortCutInfo info;
		info.id 		= (uint64)atoll(row_set[i][0].c_str());
		info.familyId 	= familyId;
		info.roomId 	= roomId;
		info.name 		= row_set[i][1];
		info.flag	 	= (unsigned int)atoi(row_set[i][2].c_str());
		info.order	 	= (unsigned int)atoi(row_set[i][3].c_str());
		info.data		= row_set[i][4];
		
		shortcuts.push_back(info);
	}
	
	return ERR_SUCCESS;
}

int Device_Mgt::_get_family_id_by_router_id(const uint64 routerId, uint64 &familyId)
{
	int nRet = 0;
	mysql_conn_Ptr conn;
	MYSQL_GET_CONN_THEN_LOCK(conn);

	MySQL_Row_Set row_set;
	std::ostringstream sql;
	sql << "select F_family_id  from tbl_router_" << routerId % 10
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find router id from sql\n");
		return ERR_ROUTER_GET_BIND_FAMILY_FAILED;
	}

	familyId = (uint64)atoll(row_set[0][0].c_str());

	XCP_LOGGER_INFO(&g_logger, "get family id(%llu) bind router(%llu\n", familyId, routerId);
	return ERR_SUCCESS;
}

int Device_Mgt::_get_user_list_by_family_id(const uint64 &familyId, std::vector<uint64> &userIds, uint64 &ownerId)
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
	int nRet = conn->query_sql(sql.str(), row_set);	
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		userIds.push_back((uint64)atoll(row_set[i][0].c_str()));
	}

	/* select F_owner_id from tbl_family_3 where F_family_id = 2013; */
	sql.str("");
	sql << "select F_owner_id from tbl_family_" << (familyId % 10)
		<< " where F_family_id = " << familyId << ";";

	MySQL_Row_Set row_set1;
	nRet = conn->query_sql(sql.str(), row_set1);	
	CHECK_RET(nRet);
	if(row_set1.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "family id is not exists: %d.\n", familyId);
		return ERR_DEVICE_FAMILY_ID_IS_NOT_EXIST;
	}

	ownerId = (uint64)atoll(row_set1[0][0].c_str());
	
	return ERR_SUCCESS;
}

int Device_Mgt::_generate_token(const std::string &key, TokenInfo &token_info, std::string &token, std::string &err_info)
{
	int nRet = ERR_SUCCESS;
	
	if (token_info.token_type <= TokenTypeBegin || token_info.token_type >= TokenTypeEnd)
	{
		err_info = "token_type error!";
		return ERR_SYSTEM;
	}
	unsigned long long timestamp = getTimestamp();
	token_info.create_time = timestamp;
	token_info.expire_time = timestamp + USER_ACCOUNT_TOKEN_TTL;
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
	if(token_info.token_type != TokenTypeGetAccount)
	{
		_get_sig_token(key, token_info, sig);
		CHECK_RET(nRet);
	}

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
		return ERR_BASE64_ENCODE_FAILED;
	}
	token = tmp;
	XCP_LOGGER_INFO(&g_logger, "src token:%s\n", s_src.c_str());
	return ERR_SUCCESS;
}

int Device_Mgt::_get_sig_token(const std::string &key, const TokenInfo &token_info, std::string &sig)
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
		return ERR_HMAC_ENCODE_FAILED;
	}
	XCP_LOGGER_INFO(&g_logger, "src:%s,sig:%s\n", sig_src.c_str(), sig.c_str());
	return ERR_SUCCESS;
}

std::string Device_Mgt::_rand_str(const unsigned int length)
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
