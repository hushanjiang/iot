
#include "base/base_os.h"
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_logger.h"
#include "base/base_xml_parser.h"
#include "conf_mgt.h"
#include "mysql_mgt.h"

extern Logger g_logger;
extern mysql_mgt g_mysql_mgt;

Conf_Mgt::Conf_Mgt(): _cfg("")
{

}


Conf_Mgt::~Conf_Mgt()
{

}

int Conf_Mgt::init(const std::string &cfg)
{
	int nRet = 0;
	
	_cfg = cfg;
	nRet = refresh();
	if(nRet != 0)
	{
		printf("refresh conf failed, ret:%d\n", nRet);
		return nRet;
	}

	//mysql read
	nRet = g_mysql_mgt.init(_mysqls["read"]._ip, _mysqls["read"]._port, _mysqls["read"]._user, _mysqls["read"]._pwd, _mysqls["read"]._db, 
		_mysqls["read"]._num, _mysqls["read"]._chars);
	if(nRet != 0)
	{
		printf("init mysql read failed, ret:%d\n", nRet);
		return nRet;
	}

	return nRet;
	
}




int Conf_Mgt::refresh()
{
	int nRet = 0;
		
	XML_Parser _parser;
	nRet = _parser.parse_file(_cfg);
	if(nRet != 0)
	{
		printf("init conf mgt failed, ret:%d, cfg:%s\n", nRet, _cfg.c_str());
		XCP_LOGGER_INFO(&g_logger, "init conf mgt failed, ret:%d, cfg:%s\n", nRet, _cfg.c_str());
		return nRet;
	}

	//---------------------- sysinfo ---------------------
	
	StSysInfo stSysInfo;
	
	//id
	XML_Node node;
	nRet = _parser.get_node("conf_svr/system/id", node);
	if(nRet != 0)
	{
		printf("get id failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get id failed, ret:%d\n", nRet);
		return -1;
	}
	stSysInfo._id = node.get_text();
	trim(stSysInfo._id);
	if(stSysInfo._id == "")
	{
		printf("id is empty\n");
		XCP_LOGGER_INFO(&g_logger, "id is empty\n");
		return -1;
	}


	//ip
	nRet = _parser.get_node("conf_svr/system/ip", node);
	if(nRet != 0)
	{
		printf("get ip failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get ip failed, ret:%d\n", nRet);
		return -1;
	}
	stSysInfo._ip = node.get_text();
	trim(stSysInfo._ip);
	if(stSysInfo._ip == "")
	{
		printf("ip is empty\n");
		XCP_LOGGER_INFO(&g_logger, "ip is empty\n");
		return -1;
	}
	
	//port
	nRet = _parser.get_node("conf_svr/system/port", node);
	if(nRet != 0)
	{
		printf("get port failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get port failed, ret:%d\n", nRet);
		return -1;
	}
	else
	{
		stSysInfo._port = (unsigned short)atoll(node.get_text().c_str());
		if(stSysInfo._port == 0)
		{
			printf("port is 0\n");
			XCP_LOGGER_INFO(&g_logger, "port is 0\n");
			return -1;
		}
	}

	//new id:[id]_[ip_port]
	stSysInfo._new_id = format("%s_%s_%u", stSysInfo._id.c_str(), stSysInfo._ip.c_str(), stSysInfo._port);
	
	//thr_num
	nRet = _parser.get_node("conf_svr/system/thr_num", node);
	if(nRet != 0)
	{
		printf("get thr_num failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get thr_num failed, ret:%d\n", nRet);
		get_cpu_number_proc(stSysInfo._thr_num);
	}
	else
	{
		stSysInfo._thr_num = (unsigned short)atoll(node.get_text().c_str());
		if(stSysInfo._thr_num == 0)
		{
			printf("thr_num is 0\n");
			XCP_LOGGER_INFO(&g_logger, "thr_num is 0\n");
			return -1;
		}		
	}

	//max_queue_size
	nRet = _parser.get_node("conf_svr/system/max_queue_size", node);
	if(nRet != 0)
	{
		printf("get max_queue_size failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get max_queue_size failed, ret:%d\n", nRet);
	}
	stSysInfo._max_queue_size = (unsigned int)atoll(node.get_text().c_str());
	if(stSysInfo._max_queue_size < 100000)
	{
		printf("max_queue_size < 100000\n");
		XCP_LOGGER_INFO(&g_logger, "max_queue_size < 100000\n");
		stSysInfo._max_queue_size = 100000;
	}

	//TZ
	nRet = _parser.get_node("conf_svr/system/TZ", node);
	if(nRet != 0)
	{
		printf("get TZ failed, ret:%d\n", nRet);
	}
	else
	{
		stSysInfo._TZ = node.get_text();
	}

	if(true)
	{
		Thread_Mutex_Guard guard(_mutex);
		_sysInfo = stSysInfo;
	}


	//------------------  mysql  ------------------
	std::map<std::string, StMysql_Access> mysqls;

	std::vector<XML_Node> vecNode;
	nRet = _parser.get_nodes("conf_svr/mysql/svr", vecNode);
	if(nRet != 0)
	{
		printf("get mysql svr failed, ret:%d\n", nRet);
		return -1;
	}

	if(vecNode.empty())
	{
		printf("mysql/svr is empty\n");
		return -1;
	}

	for(unsigned int i=0; i<vecNode.size(); ++i)
	{
		StMysql_Access stMysql_Access;

		std::string mode = "";
		mode = vecNode[i].get_attr_str("mode");
		trim(mode);
		if((mode == "") || (mode != "read"))
		{
			printf("mysql svr mode is invalid, mode:%s\n", mode.c_str());
			return -1;
		}
		
		stMysql_Access._ip = vecNode[i].get_attr_str("ip");
		trim(stMysql_Access._ip);
		if(stMysql_Access._ip == "")
		{
			printf("mysql svr ip is empty\n");
			return -1;
		}
		
		stMysql_Access._port = (unsigned short)atoll(vecNode[i].get_attr_str("port").c_str());
		if(stMysql_Access._port == 0)
		{
			printf("mysql svr port is 0\n");
			return -1;
		}

		stMysql_Access._db = vecNode[i].get_attr_str("db");
		if(stMysql_Access._db == "")
		{
			printf("mysql svr db is empty\n");
			return -1;
		}

		stMysql_Access._user = vecNode[i].get_attr_str("user");
		if(stMysql_Access._user == "")
		{
			printf("mysql svr user is empty\n");
			return -1;
		}

		stMysql_Access._pwd = vecNode[i].get_attr_str("pwd");
		if(stMysql_Access._pwd == "")
		{
			printf("mysql svr pwd is empty\n");
			return -1;
		}

		stMysql_Access._chars = vecNode[i].get_attr_str("chars");
		if(stMysql_Access._chars == "")
		{
			printf("mysql svr chars is empty\n");
			return -1;
		}

		stMysql_Access._num = (unsigned int)atoll(vecNode[i].get_attr_str("num").c_str());
		if(stMysql_Access._num == 0)
		{
			printf("mysql svr num is 0\n");
			return -1;
		}
		mysqls[mode] = stMysql_Access;
		
	}
	
	if(mysqls.size() == 0)
	{
		printf("the size of mysql svr is 0\n");
		return -1;
	}
	else
	{
		Thread_Mutex_Guard guard(_mutex);
		_mysqls = mysqls;
	}
	
	return 0;

}



//获取副本
StSysInfo Conf_Mgt::get_sysinfo()
{
	Thread_Mutex_Guard guard(_mutex);
	return _sysInfo;
}



int Conf_Mgt::get_server_access(std::string &svr_name, std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info)
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
	if(svr_name.empty())
	{
		sql << "select distinct F_svr, F_ip, F_port from tbl_svr_access;";
	}
	else
	{
		sql << "select distinct F_svr, F_ip, F_port from tbl_svr_access where F_svr='"
			<< svr_name.c_str()
			<< "';";
	}

	MySQL_Row_Set row_set;
	nRet = conn->query_sql(sql.str(), row_set);
	if(nRet != 0)
	{
		printf("query_sql server access failed, ret:%d\n", nRet);
		return -1;
	}
	
	if(row_set.row_count() == 0)
	{
		printf("server access isn't found.\n");
		return -1;
	}
	
	for(int i=0; i<row_set.row_count(); i++)
	{
		StSvr svr;
		std::string name = row_set[i][0];
		svr._ip = row_set[i][1];
		svr._port = (unsigned short)atoi(row_set[i][2].c_str());

		std::map<std::string, std::vector<StSvr> >::iterator itr = svrs.find(name);
		if(itr == svrs.end())
		{
			std::vector<StSvr> svr_list;
			svr_list.push_back(svr);
			svrs.insert(std::make_pair(name, svr_list));
		}
		else
		{
			itr->second.push_back(svr);
		}
			
	}
	
	return nRet;
	
}


