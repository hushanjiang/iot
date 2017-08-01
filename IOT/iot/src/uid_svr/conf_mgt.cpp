
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

	//mysql wirte
	nRet = g_mysql_mgt.init(_mysqls["write"]._ip, _mysqls["write"]._port, _mysqls["write"]._user, _mysqls["write"]._pwd, _mysqls["write"]._db, 
		_mysqls["write"]._num, _mysqls["write"]._chars);
	if(nRet != 0)
	{
		printf("init mysql write failed, ret:%d\n", nRet);
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
	
	StSysInfo sysInfo;
	
	//id
	XML_Node node;
	nRet = _parser.get_node("uid_svr/system/id", node);
	if(nRet != 0)
	{
		printf("get id failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get id failed, ret:%d\n", nRet);
		return -1;
	}
	sysInfo._id = node.get_text();
	trim(sysInfo._id);
	if(sysInfo._id == "")
	{
		printf("id is empty\n");
		XCP_LOGGER_INFO(&g_logger, "id is empty\n");
		return -1;
	}


	//ip
	nRet = _parser.get_node("uid_svr/system/ip", node);
	if(nRet != 0)
	{
		printf("get ip failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get ip failed, ret:%d\n", nRet);
		return -1;
	}
	sysInfo._ip = node.get_text();
	trim(sysInfo._ip);
	if(sysInfo._ip == "")
	{
		printf("ip is empty\n");
		XCP_LOGGER_INFO(&g_logger, "ip is empty\n");
		return -1;
	}
	
	//port
	nRet = _parser.get_node("uid_svr/system/port", node);
	if(nRet != 0)
	{
		printf("get port failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get port failed, ret:%d\n", nRet);
		return -1;
	}
	else
	{
		sysInfo._port = (unsigned short)atoll(node.get_text().c_str());
		if(sysInfo._port == 0)
		{
			printf("port is 0\n");
			XCP_LOGGER_INFO(&g_logger, "port is 0\n");
			return -1;
		}
	}

	//new id:[id]_[ip_port]
	sysInfo._new_id = format("%s_%s_%u", sysInfo._id.c_str(), sysInfo._ip.c_str(), sysInfo._port);
	
	//thr_num
	nRet = _parser.get_node("uid_svr/system/thr_num", node);
	if(nRet != 0)
	{
		printf("get thr_num failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get thr_num failed, ret:%d\n", nRet);
		get_cpu_number_proc(sysInfo._thr_num);
	}
	else
	{
		sysInfo._thr_num = (unsigned short)atoll(node.get_text().c_str());
		if(sysInfo._thr_num == 0)
		{
			printf("thr_num is 0\n");
			XCP_LOGGER_INFO(&g_logger, "thr_num is 0\n");
			return -1;
		}		
	}

	//max_queue_size
	nRet = _parser.get_node("uid_svr/system/max_queue_size", node);
	if(nRet != 0)
	{
		printf("get max_queue_size failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get max_queue_size failed, ret:%d\n", nRet);
	}
	sysInfo._max_queue_size = (unsigned int)atoll(node.get_text().c_str());
	if(sysInfo._max_queue_size < 100000)
	{
		printf("max_queue_size < 100000\n");
		XCP_LOGGER_INFO(&g_logger, "max_queue_size < 100000\n");
		sysInfo._max_queue_size = 100000;
	}

	//TZ
	nRet = _parser.get_node("uid_svr/system/TZ", node);
	if(nRet != 0)
	{
		printf("get TZ failed, ret:%d\n", nRet);
	}
	else
	{
		sysInfo._TZ = node.get_text();
	}

	if(true)
	{
		Thread_Mutex_Guard guard(_mutex);
		_sysInfo = sysInfo;
	}


	//------------------  mysql  ------------------
	std::map<std::string, StMysql_Access> mysqls;

	std::vector<XML_Node> vecNode;
	nRet = _parser.get_nodes("uid_svr/mysql/svr", vecNode);
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
		if((mode == "") || (mode != "write"))
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


