
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "device_mgt.h"
#include "redis_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;

#define CHECK_RET(nRet)	\
{ \
	if(nRet != 0) \
	{ \
		XCP_LOGGER_ERROR(&g_logger, "check return failed: %d.\n", nRet); \
		return nRet; \
	} \
}

#define GET_CONN_THEN_LOCK(conn) \
{ \
	if(!g_mysql_mgt.get_conn(conn)) \
	{ \
		XCP_LOGGER_ERROR(&g_logger, "get mysql conn failed.\n"); \
		return ERR_GET_MYSQL_CONN_FAILED; \
	} \
	MySQL_Guard guard(conn); \
}

const unsigned int REDIS_KEY_MAX_LENGTH			= 40;

const std::string REDIS_FAMILY_ROUTER_PRE_HEAD	= "FamilyBind:family_";


Device_Mgt::Device_Mgt()
{

}


Device_Mgt::~Device_Mgt()
{

}

int Device_Mgt::add_room(const StRoomInfo &info, uint64 &roomId)
{	
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

	/* insert into tbl_room_0(F_family_id, F_room_name, F_default, F_disp_order, F_creator, F_created_at, F_updated_at)
		values (1017, '客厅', 0, 1, 123, 123456, 123456);*/
	std::ostringstream sql;
	sql << "insert into tbl_room_" << (info.familyId % 10)
		<< " (F_family_id, F_room_name, F_default, F_disp_order, F_creator, F_created_at, F_updated_at) values("
		<< info.familyId << ", "
		<< "'" << info.name << "', "
		<< info.isDefault << ", "
		<< info.orderNum<< ", "
		<< info.creatorId<< ", "
		<< info.createAt << ", "
		<< info.updateAt << ");";

	uint64 last_insert_id = 0;
	roomId = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, roomId);
	CHECK_RET(nRet);
	return 0;
}


int Device_Mgt::delete_room(const uint64 &userId, const uint64 &roomId, const uint64 &familyId)
{
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

	/* delete from tbl_room_0 where F_id = 1; */
	std::ostringstream sql;
	sql << "delete from tbl_room_" << (familyId % 10)
		<< "  where F_id = " << roomId
		<< ";";

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	return 0;
}


int Device_Mgt::update_room(const uint64 &userId, const StRoomInfo &info)
{
	XCP_LOGGER_INFO(&g_logger, "%llu start to update room:%llu \n", userId, info.id);
	
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

	/* update tbl_room_1  set F_room_name = 'room',F_updated_at = 1222334 where F_id = 1; */
	std::ostringstream sql;
	sql << "update tbl_room_" << (info.familyId % 10)
		<< "  set F_room_name = '" << info.name << "'"
		<< ",F_updated_at = " << info.updateAt
		<< " where F_id = " << info.id
		<< ";";

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	int nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	return 0;
}


int Device_Mgt::get_room_list(const uint64 &familyId, std::vector<StRoomInfo> &infos)
{
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

	/* select F_id,F_room_name,F_disp_order,F_created_at,F_updated_at from tbl_room_0 where F_family_id = 1017; */
	std::ostringstream sql;
	sql << "select F_id,F_room_name,F_disp_order,F_created_at,F_updated_at from tbl_room_"
		<< (familyId % 10)
		<< " where F_family_id = " << familyId
		<< ";";

	MySQL_Row_Set row_set;
	int nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		StRoomInfo info;
		info.id 		= (uint64)atoi(row_set[i][0].c_str());
		info.familyId 	= familyId;
		info.name 		= row_set[i][1];
		info.orderNum 	= (uint64)atoi(row_set[i][2].c_str());
		info.createAt 	= (uint64)atoi(row_set[i][3].c_str());
		info.updateAt	= (uint64)atoi(row_set[i][4].c_str());
		
		infos.push_back(info);
	}
	
	return 0;
}


int Device_Mgt::update_room_order(const uint64 &userId, const uint64 &familyId, const std::vector<StRoomInfo> &infos)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	std::ostringstream sql;

	std::vector<StRoomInfo>::const_iterator end = infos.end();
	for(std::vector<StRoomInfo>::const_iterator it=infos.begin(); it != end; it++)
	{
		sql.str("");
		/* update tbl_room_0 set F_disp_order = 1 where F_id = 1; */
		sql << "update tbl_room_" << (familyId % 10)
			<< "  set F_disp_order = '" << it->orderNum
			<< "' where F_id = " << it->id
			<< ";";

		nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
		CHECK_RET(nRet);
	}
	return 0;
}


int Device_Mgt::add_device(const StDeviceInfo &info)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

	// TODO: 检查家庭和路由器的绑定关系�
	/* insert into tbl_device_info_0(F_did, F_router_id, F_product_id, F_business_id, 
		F_device_name, F_device_id, F_device_params, F_created_at, F_updated_at)
		values (1000, 123, 123, 123, '1号灯', 1, 'test', 123456, 123456); */
	std::ostringstream sql;
	sql << "insert into tbl_device_info_" << (info.routerId % 10)
		<< "(F_did, F_router_id, F_product_id, F_business_id,  F_device_name, F_device_id, F_device_params, F_created_at, F_updated_at) values("
		<< info.id << ", "
		<< info.routerId << ", "
		<< info.productId << ", "
		<< info.businessId << ", "
		<< "'" << info.name << "', "
		<< "'" << info.deviceId << "', "
		<< "'" << info.attr << "', "
		<< info.createAt << ", "
		<< info.updateAt << ");";

	uint64 last_insert_id = 0, affected_rows = 0;
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows );
	CHECK_RET(nRet);

	/* insert into tbl_room_device_0(F_family_id, F_room_id, F_did, F_product_id,
		F_device_name, F_creator, F_created_at, F_updated_at) 
		values (1017, 1, 1000, 123, '1号灯', 123, 123456, 123456); */
	sql.str("");
	sql << "insert into tbl_room_device_" << (info.familyId % 10)
		<< "(F_family_id, F_room_id, F_did, F_product_id, F_device_name, F_creator, F_created_at, F_updated_at) values("
		<< info.familyId << ", "
		<< info.roomId << ", "
		<< info.id << ", "
		<< info.productId << ", "
		<< "'" << info.name << "', "
		<< info.creatorId << ", "
		<< info.createAt << ", "
		<< info.updateAt << ");";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows );
	CHECK_RET(nRet);
	
	return 0;
}


int Device_Mgt::delete_device(const uint64 &userId, const StDeviceInfo &info)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;

	/* delete from tbl_room_device_0 where F_did = 1; */
	std::ostringstream sql;
	sql << "delete from tbl_room_device_" << (info.routerId % 10)
		<< "  where F_did = " << info.id
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);

	/* delete from tbl_device_info_0 where F_did = 1; */
	sql.str("");
	sql << "delete from tbl_device_info_" << (info.familyId % 10)
		<< "  where F_did = " << info.id
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	
	return 0;
}


int Device_Mgt::update_device(const uint64 &userId, const StDeviceInfo &info)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;

	/* update tbl_room_device_0 set F_room_id = 22, F_device_name = 'a', F_updated_at = 123455
		where F_did = 1000; */
	std::ostringstream sql;
	sql << "update tbl_room_device_" << (info.routerId % 10)
		<< " set F_updated_at = " << info.updateAt;
	if(info.roomId != 0)
	{
		sql << ", F_room_id = " << (info.roomId);
	}
	if(!info.name.empty())
	{
		sql << ", F_device_name = '" << (info.name) << "'";
	}
	sql << " where F_did = " << info.id << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	
	/* update tbl_device_info_0 set F_updated_at = 123456, F_device_id = '123', F_business_id = 123,
		F_device_name = '111', F_product_id = 2, F_device_params = '1234' where F_did = 1000; */
	sql.str("");
	sql << "update tbl_device_info_" << (info.familyId % 10)
		<< "  set F_updated_at = " << info.updateAt;
	if(!info.deviceId.empty())
	{
		sql << ", F_device_id = '" << (info.deviceId) << "'";
	}
	if(info.businessId != 0)
	{
		sql << ", F_business_id = " << (info.businessId);
	}
	if(!info.name.empty())
	{
		sql << ", F_device_name = '" << (info.name) << "'";
	}
	if(info.productId != DM_DEVICE_CATEGORY_END)
	{
		sql << ", F_product_id = " << (info.productId);
	}
	if(!info.attr.empty())
	{
		sql << ", F_device_params = '" << (info.attr) << "'";
	}
	sql << " where F_did = " << info.id << ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	CHECK_RET(nRet);
	
	return 0;
}


int Device_Mgt::get_device_info(const uint64 &userId, StDeviceInfo &info)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;

	/* select F_product_id, F_business_id, F_device_name, F_device_id, F_device_params,
		F_created_at, F_updated_at from tbl_device_info_0 where F_did = 1000; */
	sql << "select F_product_id, F_business_id, F_device_name, F_device_id, F_device_params, F_created_at, F_updated_at "
		<< "from tbl_device_info_" << (info.routerId % 10)
		<< " where F_did = " << info.id
		<< ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);

	info.productId	= (enum DM_DEVICE_CATEGORY)(atoi(row_set[0][0].c_str()) % DM_DEVICE_CATEGORY_END);
	info.businessId = (uint64)atoi(row_set[0][1].c_str());
	info.name		= row_set[0][2];
	info.deviceId	= row_set[0][3];
	info.attr		= row_set[0][4];
	info.createAt	= (uint64)atoi(row_set[0][5].c_str());
	info.updateAt	= (uint64)atoi(row_set[0][6].c_str());

	/* select F_room_id, F_creator from tbl_room_device_0 where F_did = 1000;*/
	sql.str("");
	sql << "select F_room_id, F_creator from tbl_room_device_" << (info.familyId % 10)
		<< " where F_did = " << info.id	<< ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);

	info.roomId 	= (uint64)atoi(row_set[0][0].c_str());
	info.creatorId 	= (uint64)atoi(row_set[0][1].c_str());
			
	return 0;
}


int Device_Mgt::get_devices_by_room(const uint64 &userId, const uint64 &familyId, const uint64 &roomId, const uint64 &routerId, enum DM_DEVICE_CATEGORY category,
			std::vector<StDeviceInfo> &infos, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;
	
	/* select F_did, F_creator, F_room_id from tbl_room_device_0
		where F_family_id = 1017 and F_room_id = 22 and F_product_id = 1 limit 0,10; */
	sql << "select F_did, F_creator from tbl_room_device_" << (familyId % 10)
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
		StDeviceInfo info;
		info.id 		= (uint64)atoi(row_set[i][0].c_str());
		info.creatorId 	= (uint64)atoi(row_set[i][1].c_str());
		info.familyId	= familyId;
		info.roomId		= roomId;
		info.routerId	= routerId;
		
		MySQL_Row_Set row_set_device;
		/* select F_product_id, F_business_id, F_device_name, F_device_id, F_device_params,
			F_created_at, F_updated_at from tbl_device_info_0 where F_did = 1000; */
		sql << "select F_product_id, F_business_id, F_device_name, F_device_id, F_device_params, F_created_at, F_updated_at "
			<< "from tbl_device_info_" << (routerId % 10)
			<< " where F_did = " << info.id
			<< ";";
		nRet = conn->query_sql(sql.str(), row_set_device);
		CHECK_RET(nRet);

		info.productId	= (enum DM_DEVICE_CATEGORY)(atoi(row_set_device[0][0].c_str()) % DM_DEVICE_CATEGORY_END);
		info.businessId = (uint64)atoi(row_set_device[0][1].c_str());
		info.name		= row_set_device[0][2];
		info.deviceId	= row_set_device[0][3];
		info.attr		= row_set_device[0][4];
		info.createAt	= (uint64)atoi(row_set_device[0][5].c_str());
		info.updateAt	= (uint64)atoi(row_set_device[0][6].c_str());
		
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

	return 0;
}


int Device_Mgt::get_devices_by_family(const uint64 &userId, const uint64 &familyId, const uint64 &routerId, enum DM_DEVICE_CATEGORY category,
			std::vector<StDeviceInfo> &infos, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;
	
	/* select F_did, F_creator, F_room_id from tbl_room_device_0 where F_family_id = 1017 and F_room_id = 22 limit 0,10; */
	sql << "select F_did, F_creator, F_room_id from tbl_room_device_" << (familyId % 10)
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
	
	for(int i=0; i < row_set.row_count(); i++)
	{
		StDeviceInfo info;
		info.id 		= (uint64)atoi(row_set[i][0].c_str());
		info.creatorId 	= (uint64)atoi(row_set[i][1].c_str());
		info.familyId	= familyId;
		info.roomId		= (uint64)atoi(row_set[i][2].c_str());
		info.routerId	= routerId;
		
		MySQL_Row_Set row_set_device;
		/* select F_product_id, F_business_id, F_device_name, F_device_id, F_device_params,
			F_created_at, F_updated_at from tbl_device_info_0 where F_did = 1000; */
		sql.str("");
		sql << "select F_product_id, F_business_id, F_device_name, F_device_id, F_device_params, F_created_at, F_updated_at "
			<< "from tbl_device_info_" << (routerId % 10)
			<< " where F_did = " << info.id
			<< ";";
		nRet = conn->query_sql(sql.str(), row_set_device);
		CHECK_RET(nRet);

		info.productId	= (enum DM_DEVICE_CATEGORY)(atoi(row_set_device[0][0].c_str()) % DM_DEVICE_CATEGORY_END);
		info.businessId = (uint64)atoi(row_set_device[0][1].c_str());
		info.name		= row_set_device[0][2];
		info.deviceId	= row_set_device[0][3];
		info.attr		= row_set_device[0][4];
		info.createAt	= (uint64)atoi(row_set_device[0][5].c_str());
		info.updateAt	= (uint64)atoi(row_set_device[0][6].c_str());
		
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

	return 0;
}


int Device_Mgt::report_status(const StDeviceStatus &status, uint64 &statusId)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

	/* insert into t_device_report_0 (F_did, F_report_id, F_family_id, F_report_msg, F_created_at)
		values (1000, 1, 1017, 'test', 12345); */
	std::ostringstream sql;
	sql << "insert into t_device_report_" << (status.deviceId % 10)
		<< " (F_did, F_report_id, F_family_id, F_report_msg, F_created_at) values("
		<< status.deviceId << ", "
		<< status.type << ", "
		<< status.familyId << ", "
		<< "'" << status.msg << "', "
		<< status.createAt << ");";

	uint64 last_insert_id = 0;
	nRet = conn->execute_sql(sql.str(), last_insert_id, statusId);
	CHECK_RET(nRet);
	
	return 0;
}


int Device_Mgt::report_alert(const StDeviceAlert &alert, uint64 &alertId)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);

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
	
	return 0;
}


int Device_Mgt::get_dev_status_list(const uint64 &userId, const uint64 &deviceId,
	std::vector<StDeviceStatus> &status, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;
	
	/* select F_id, F_report_id, F_family_id, F_report_msg, F_created_at from t_device_report_0
		where F_did = 1000 limit 0,10; */
	sql << "select F_id, F_report_id, F_family_id, F_report_msg, F_created_at from t_device_report_" << (deviceId % 10)
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
		StDeviceStatus statu;
		statu.id 		= (uint64)atoi(row_set[i][0].c_str());
		statu.deviceId	= deviceId;
		statu.type		= (enum DM_DEVICE_STATUS_TYPE)(atoi(row_set[i][1].c_str()) % DM_DEVICE_STATUS_TYPE_END);
		statu.familyId	= (uint64)atoi(row_set[i][2].c_str());
		statu.msg		= row_set[i][3];
		statu.createAt	= (uint64)atoi(row_set[i][4].c_str());
		
		status.push_back(statu);
	}

	left = 0;
	if(size != 0)
	{
		unsigned int count = row_set.row_count();
		XCP_LOGGER_INFO(&g_logger, "status total num:%d\n", count);
		left = (count > (size + beginAt))? (count - size - beginAt) : 0;
		CHECK_RET(nRet);
	}

	return 0;
}

int Device_Mgt::get_dev_alert_list(const uint64 &userId, const uint64 &deviceId,
	std::vector<StDeviceAlert> &alerts, const unsigned int &beginAt, const unsigned int &size, unsigned int &left)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
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
		alert.id 		= (uint64)atoi(row_set[i][0].c_str());
		alert.deviceId	= deviceId;
		alert.type		= (enum DM_DEVICE_ALERT_TYPE)(atoi(row_set[i][1].c_str()) % DM_DEVICE_ALERT_TYPE_END);
		alert.familyId	= (uint64)atoi(row_set[i][2].c_str());
		alert.msg		= row_set[i][3];
		alert.handler	= (uint64)atoi(row_set[i][4].c_str());
		alert.status	= (enum DM_DEVICE_ALERT_STATUS)(atoi(row_set[i][5].c_str()) % DM_DEVICE_ALERT_STATUS_END);
		alert.createAt	= (uint64)atoi(row_set[i][6].c_str());
		alert.handleAt	= (uint64)atoi(row_set[i][7].c_str());
		
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
	
	return 0;
}


int Device_Mgt::router_auth(const std::string &uuid, const std::string &pwd, uint64 &routerId)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;

	/* select F_rid from tbl_router_map where F_device_id = '1'; */
	sql << "select F_rid from tbl_router_map where F_device_id = '" << uuid << "';";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);

	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find uuid:%s in database\n", uuid.c_str());
		return -1;
	}
	routerId = (uint64)atoi(row_set[0][0].c_str());
	
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
		return -1;
	}
	
	return 0;
}

int Device_Mgt::router_bind(const uint64 userId, const uint64 familyId, const uint64 routerId)
{
	XCP_LOGGER_INFO(&g_logger, "start bind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);

	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	uint64 last_insert_id = 0;
	uint64 affected_rows = 0;
	std::ostringstream sql;

	// 检查是不是户主操作
	sql << "select F_owner_id, F_router_id from tbl_family_" << (familyId % 10)
		<<" where F_family_id = " << familyId << ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);

	if(row_set.row_count() == 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "cannot find family id:%llu\n", familyId);
		return -1;
	}
	uint64 ownerId  = (uint64)atoi(row_set[0][0].c_str());
	uint64 bind_routerId = (uint64)atoi(row_set[0][1].c_str());
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only family owner(id:%llu) can bind router\n", ownerId);
		return -1;
	}
	
	// 直接写数据库，不必理会家庭是否已经绑定过路由器，直接覆盖
	if(bind_routerId != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "this family have bind router(%llu) before\n", bind_routerId);
	}
	
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
	
	PSGT_Redis_Mgt->set(redis_key, routerId);
	
	return 0;
}

int Device_Mgt::router_unbind(const uint64 userId, const uint64 familyId, const uint64 routerId)
{
	XCP_LOGGER_INFO(&g_logger, "start unbind family, userId:%llu, familyId:%llu, routerId:%llu\n", userId, familyId, routerId);
	
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
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
		return -1;
	}
	uint64 ownerId  = (uint64)atoi(row_set[0][0].c_str());
	uint64 bind_routerId = (uint64)atoi(row_set[0][1].c_str());
	if(userId != ownerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "only family owner(id:%llu) can unbind router\n", ownerId);
		return -1;
	}
	if(routerId != bind_routerId)
	{
		XCP_LOGGER_ERROR(&g_logger, "family donot bind on this router, but %llu\n", bind_routerId);
		return -1;
	}
	
	/* update tbl_family_0 set F_router_id = 1 where F_family_id = 10; */
	sql.str("");
	sql << "update tbl_family_" << (familyId % 10)
		<< " set F_router_id = 0 where F_family_id = " << familyId
		<< ";";
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
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
	PSGT_Redis_Mgt->set(redis_key, 0);
	
	return 0;
}

int Device_Mgt::router_get_info(const uint64 &routerId, StRouterInfo &info)
{
	int nRet;
	mysql_conn_Ptr conn;
	GET_CONN_THEN_LOCK(conn);
	
	MySQL_Row_Set row_set;
	std::ostringstream sql;

	/* select F_family_id, F_device_name, F_device_id, F_device_sn, F_device_mac, F_device_state,
		F_device_params, F_created_at from tbl_router_1 where F_router_id = 1001; */
	sql << "select F_device_name, F_device_id, F_device_sn, F_device_mac, F_device_state, F_device_params, F_created_at "
		<< "from tbl_router_" << (routerId % 10)
		<< " where F_router_id = " << routerId
		<< ";";
	nRet = conn->query_sql(sql.str(), row_set);
	CHECK_RET(nRet);

	info.familyId	= (uint64)atoi(row_set[0][0].c_str());
	info.name		= row_set[0][1];
	info.uuid		= (uint64)atoi(row_set[0][2].c_str());
	info.snNo		= row_set[0][3];
	info.mac		= row_set[0][4];
	info.state		= (uint64)atoi(row_set[0][5].c_str());
	info.attr		= row_set[0][6];
	info.creatAt	= (uint64)atoi(row_set[0][7].c_str());

	return 0;
}

