
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "security_mgt.h"
#include "mysql_mgt.h"
#include "redis_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;


Security_Mgt::Security_Mgt()
{

}


Security_Mgt::~Security_Mgt()
{

}


int Security_Mgt::create_security_channel(const std::string &id, std::string &key, std::string &err_info)
{
	return PSGT_Redis_Mgt->create_security_channel(id, key, err_info);
}




int Security_Mgt::refresh_security_channel(const std::string &id, std::string &err_info)
{
	return PSGT_Redis_Mgt->refresh_security_channel(id, err_info);
}




/*
int Security_Mgt::create_security_channel(std::string &id, std::string &key, std::string &err_info)
{
	int nRet = 0;

	XCP_LOGGER_INFO(&g_logger, "prepare to create security channel: id:%s, key:%s\n", id.c_str(), key.c_str());
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_INFO(&g_logger, "get mysql conn failed.\n");
		err_info = "get mysql conn failed.";
		return ERR_GET_MYSQL_CONN_FAILED;
	}

	MySQL_Guard guard(conn);

	unsigned long long cur_time = getTimestamp();
	std::ostringstream sql;
	sql << "insert into tbl_security_channel (F_guid, F_key, F_created_at, F_expires_at) values ('"
		<< id.c_str()
		<< "', '"
		<< key.c_str()
		<< "', "
		<< cur_time
		<< ", "
		<< (cur_time + SECURITY_CHANNEL_TTL)
		<< ") on duplicate key update F_guid='"
		<< id.c_str()
		<< "';";
	
	unsigned long long last_insert_id = 0;
	unsigned long long affected_rows = 0;
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "execute_sql create security channel failed, id:%s, key:%s, ret:%d\n",
			id.c_str(), key.c_str(), nRet);
		err_info = "execute_sql create security channel failed.";
	}

	XCP_LOGGER_INFO(&g_logger, "complete to create security channel: id:%s, key:%s\n", id.c_str(), key.c_str());
	return nRet;

}




int Security_Mgt::refresh_security_channel(std::string &id, std::string &err_info)
{
	int nRet = 0;

	XCP_LOGGER_INFO(&g_logger, "prepare to refresh security channel: id:%s\n", id.c_str());
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_INFO(&g_logger, "get mysql conn failed.\n");
		err_info = "get mysql conn failed.";
		return ERR_GET_MYSQL_CONN_FAILED;
	}

	MySQL_Guard guard(conn);
	
	unsigned long long cur_time = getTimestamp();
	std::ostringstream sql;
	sql << "update tbl_security_channel set F_updated_at="
		<< cur_time
		<< ", F_expires_at="
		<< (cur_time + SECURITY_CHANNEL_TTL)		
		<< " where F_guid='"
		<< id.c_str()
		<< "';";

	unsigned long long last_insert_id = 0;
	unsigned long long affected_rows = 0;
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "execute_sql refresh security channel failed, id:%s, ret:%d\n",
			id.c_str(), nRet);
		err_info = "execute_sql refresh security channel failed.";
	}

	XCP_LOGGER_INFO(&g_logger, "complete to refresh security channel: id:%s\n", id.c_str());
	return nRet;

}
*/


