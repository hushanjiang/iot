
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "uuid_mgt.h"
#include "mysql_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;

UUID_Mgt::UUID_Mgt()
{

}


UUID_Mgt::~UUID_Mgt()
{

}


int UUID_Mgt::init()
{
	int nRet = 0;

	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		printf("get mysql conn failed.\n");
		return ERR_GET_MYSQL_CONN_FAILED;
	}

	MySQL_Guard guard(conn);

	std::ostringstream sql;
	sql << "select F_name, F_index, F_nr_peralloc from tbl_uuid;";

	MySQL_Row_Set row_set;
	nRet = conn->query_sql(sql.str(), row_set);
	if(nRet != 0)
	{
		printf("query_sql uuid conf failed, ret:%d\n", nRet);
		return -1;
	}
	
	if(row_set.row_count() == 0)
	{
		printf("uuid conf isn't found.\n");
		return -1;
	}

	for(int i=0; i<row_set.row_count(); i++)
	{
		std::string name = row_set[i][0];
		unsigned long long index = (unsigned long long)atoll(row_set[i][1].c_str());
		unsigned int nr_peralloc = (unsigned int)atoi(row_set[i][2].c_str());
		unsigned long long next = index + nr_peralloc;

		printf("name:%s, index:%llu, nr_peralloc:%u, next:%llu\n", 
			name.c_str(), index, nr_peralloc, next);

		//¸üÐÂindex
		std::ostringstream sql_update;
		sql_update << "update tbl_uuid set F_index="
			<< next
			<< ", F_last_update_at=unix_timestamp() where F_name='"
			<< name.c_str()
			<< "';";
		
		unsigned long long last_insert_id = 0;
		unsigned long long affected_rows = 0;
		nRet = conn->execute_sql(sql_update.str(), last_insert_id, affected_rows);
		if(nRet != 0)
		{
			printf("execute_sql update index failed, ret:%d, sql:%s\n", 
				name.c_str(), nRet, sql_update.str().c_str());
			return -1;
		}
		index = next;

		UUID *uuid = new UUID(name, index, nr_peralloc);
		_uuids[name] = uuid;
		
	}
	
	return nRet;
	
}




int UUID_Mgt::get_uuid(const std::string &name, unsigned long long &uuid, std::string &err_info)
{
	int nRet = 0;

	std::map<std::string, UUID*>::iterator itr = _uuids.find(name);
	if(itr == _uuids.end())
	{
		XCP_LOGGER_INFO(&g_logger, "get uuid failed, invalid uuid name(%s).\n", name.c_str());
		err_info = "invalid uuid name.";
		return -1;
	}

	return (itr->second)->get_uuid(uuid, err_info);
}


