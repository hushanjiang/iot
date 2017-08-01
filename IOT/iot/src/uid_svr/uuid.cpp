
#include "base/base_os.h"
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "uuid.h"
#include "mysql_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;

extern Logger g_logger;

UUID::UUID(const std::string name, const unsigned long long index, const unsigned int nr_peralloc):
	_name(name), _index(index), _nr_peralloc(nr_peralloc) 
{
	_next = index + nr_peralloc;
}



UUID::~UUID()	
{
	
}


int UUID::get_uuid(unsigned long long &uuid, std::string &err_info)
{
	Thread_Mutex_Guard guard(_mutex);
	
	int nRet = 0;
	
	if(_index >= _next)
	{
		nRet = update_index(err_info);
		if(nRet != 0)
		{
			return nRet;
		}
	}

	uuid = _index++;
	
	return nRet;
	
}



int UUID::update_index(std::string &err_info)
{
	int nRet = 0;

	XCP_LOGGER_INFO(&g_logger, "prepare to update_index: name:%s, index:%llu, nr_peralloc:%u, next:%llu\n", 
			_name.c_str(), _index, _nr_peralloc, _next);
	
	mysql_conn_Ptr conn;
	if(!g_mysql_mgt.get_conn(conn))
	{
		XCP_LOGGER_INFO(&g_logger, "get mysql conn failed.\n");
		err_info = "get mysql conn failed.";
		return ERR_GET_MYSQL_CONN_FAILED;
	}

	MySQL_Guard guard(conn);

	std::ostringstream sql;
	sql << "update tbl_uuid set F_index="
		<< _next
		<< ", F_last_update_at=unix_timestamp() where F_name='"
		<< _name.c_str()
		<< "'";
	
	unsigned long long last_insert_id = 0;
	unsigned long long affected_rows = 0;
	nRet = conn->execute_sql(sql.str(), last_insert_id, affected_rows);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "execute_sql update index failed, name:%s, ret:%d\n", _name.c_str(), nRet);
		err_info = "execute_sql update index failed.";
	}
	else
	{
		_index = _next;
		_next = _index + _nr_peralloc;
	}

	XCP_LOGGER_INFO(&g_logger, "complete to update_index: name:%s, index:%llu, nr_peralloc:%u, next:%llu\n", 
			_name.c_str(), _index, _nr_peralloc, _next);
	
	return nRet;

}


