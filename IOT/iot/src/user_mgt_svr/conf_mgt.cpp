
#include "base/base_os.h"
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_xml_parser.h"
#include "base/base_logger.h"
#include "conf_timer_handler.h"
#include "conf_mgt.h"
#include "conn_mgt_lb.h"
#include "mysql_mgt.h"
#include "redis_mgt.h"
#include "rsp_tcp_event_handler.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern mysql_mgt g_mysql_mgt;

extern Conn_Mgt_LB g_conf_mgt_conn;
extern Conn_Mgt_LB g_uid_conn;

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
		XCP_LOGGER_INFO(&g_logger, "refresh conf mgt falied, ret:%d\n", nRet);
		return nRet;
	}

	//初始化TCP 连接管理
	nRet = init_conn_mgt();
	if(nRet != 0)
	{	
		XCP_LOGGER_INFO(&g_logger, "init_conn_mgt failed, ret:%d\n", nRet);
		return nRet;
	}

	//启动conf mgt  timer
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start conf mgt timer ---\n");
	Select_Timer *timer_req = new Select_Timer;
	Conf_Timer_handler *conf_thandler = new Conf_Timer_handler;
	nRet = timer_req->register_timer_handler(conf_thandler, 10000000);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "register conf mgt timer handler falied, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = timer_req->init();
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "int conf mgt timer failed, ret:%d\n", nRet);
		return nRet;
	}

	nRet = timer_req->run();
	if(nRet != 0)
	{
		XCP_LOGGER_ERROR(&g_logger, "conf mgt timer run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start conf mgt timer ===\n");

	//mysql wirte
	nRet = g_mysql_mgt.init(_mysqls["write"]._ip, _mysqls["write"]._port, _mysqls["write"]._user, _mysqls["write"]._pwd, _mysqls["write"]._db, 
		_mysqls["write"]._num, _mysqls["write"]._chars);
	if(nRet != 0)
	{
		printf("init mysql write failed, ret:%d\n", nRet);
		return nRet;
	}

	//redis
	nRet = PSGT_Redis_Mgt->init(_redises[0]._ip, _redises[0]._port, _redises[0]._auth);
	if(nRet != 0)
	{
		printf("init redis failed, ret:%d\n", nRet);
		return nRet;
	}
	
	return nRet;
	
}




//初始化所有的长连接池
int Conf_Mgt::init_conn_mgt()
{
	int nRet = 0;

	//conf vr
	nRet = g_conf_mgt_conn.init(&_conf_svr);
	if(nRet != 0)
	{
		printf("init conf mgt req conn, ret:%d\n", nRet);
		return nRet;
	}

	//uid svr
	std::vector<StSvr> svrs;
	RSP_TCP_Event_Handler *handler = new RSP_TCP_Event_Handler(ST_UID);
	nRet = g_uid_conn.init(&svrs, handler, true);
	if(nRet != 0)
	{
		printf("init uid svr conn.\n");
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
	nRet = _parser.get_node("user_mgt_svr/system/id", node);
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
	nRet = _parser.get_node("user_mgt_svr/system/ip", node);
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
	nRet = _parser.get_node("user_mgt_svr/system/port", node);
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
	
	//Message ID命名规则： [svr id]_[ip]_[port]
	sysInfo._new_id = format("%s_%s_%u", sysInfo._id.c_str(), sysInfo._ip.c_str(), sysInfo._port);
	
	//thr_num
	nRet = _parser.get_node("user_mgt_svr/system/thr_num", node);
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
	nRet = _parser.get_node("user_mgt_svr/system/max_queue_size", node);
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
	nRet = _parser.get_node("user_mgt_svr/system/TZ", node);
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

	
	//----------------------- conf svr --------------------
	std::vector<StSvr> conf_svr;
	std::vector<XML_Node> vecNode;
	nRet = _parser.get_nodes("user_mgt_svr/conf/svr", vecNode);
	if(nRet != 0)
	{
		printf("get conf svr failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get conf svr failed, ret:%d\n", nRet);
		return -1;
	}

	if(vecNode.empty())
	{
		printf("conf svr is empty\n");
		XCP_LOGGER_INFO(&g_logger, "conf svr is empty\n");
		return -1;
	}

	StSvr svr;
	for(unsigned int i=0; i<vecNode.size(); ++i)
	{
		svr._ip = vecNode[i].get_attr_str("ip");
		trim(svr._ip);
		if(svr._ip == "")
		{
			printf("conf svr ip is empty\n");
			XCP_LOGGER_INFO(&g_logger, "conf svr ip is empty\n");
			return -1;
		}
		
		svr._port = (unsigned short)atoll(vecNode[i].get_attr_str("port").c_str());
		if(svr._port == 0)
		{
			printf("conf svr port is empty\n");
			XCP_LOGGER_INFO(&g_logger, "conf svr port is empty\n");
			return -1;
		}
	
		conf_svr.push_back(svr);

	}
	
	if(conf_svr.empty())
	{
		printf("conf svr is empty\n");
		XCP_LOGGER_INFO(&g_logger, "conf svr is empty\n");
	}
	else
	{
		Thread_Mutex_Guard guard(_mutex);
		_conf_svr = conf_svr;
	}

	//------------------  mysql  ------------------
	std::map<std::string, StMysql_Access> mysqls;

	vecNode.clear();
	nRet = _parser.get_nodes("user_mgt_svr/mysql/svr", vecNode);
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
	

	//------------------ redis ------------------
	std::vector<StRedis_Access> redises;

	vecNode.clear();
	nRet = _parser.get_nodes("user_mgt_svr/redis/svr", vecNode);
	if(nRet != 0)
	{
		printf("get redis svr failed, ret:%d\n", nRet);
		return -1;
	}

	if(vecNode.empty())
	{
		printf("redis/svr is empty\n");
		return -1;
	}

	for(unsigned int i=0; i<vecNode.size(); ++i)
	{
		StRedis_Access stRedis_Access;
			
		stRedis_Access._ip = vecNode[i].get_attr_str("ip");
		trim(stRedis_Access._ip);
		if(stRedis_Access._ip == "")
		{
			printf("redis svr ip is empty\n");
			return -1;
		}
		
		stRedis_Access._port = (unsigned short)atoll(vecNode[i].get_attr_str("port").c_str());
		if(stRedis_Access._port == 0)
		{
			printf("redis svr port is 0\n");
			return -1;
		}

		stRedis_Access._auth = vecNode[i].get_attr_str("auth");
		trim(stRedis_Access._auth);
		if(stRedis_Access._auth == "")
		{
			printf("redis svr auth is empty\n");
			return -1;
		}
		redises.push_back(stRedis_Access);
		
	}
	
	if(redises.empty())
	{
		printf("redis svr is empty\n");
		return -1;
	}
	else
	{
		Thread_Mutex_Guard guard(_mutex);
		_redises = redises;
	}

	return 0;

}




int Conf_Mgt::update_svr()
{
	int nRet = 0;

	std::string err_info = "";
	Conn_Ptr conn;
	if(g_conf_mgt_conn.get_conn(conn))
	{
		std::map<std::string, std::vector<StSvr> > svrs;
		nRet = conn->get_server_access(svrs, err_info);
		if(nRet == 0)
		{
			//uid
			if(!svrs[ST_UID].empty())
			{
				std::vector<StSvr> servers;
				for(unsigned int i=0; i<svrs[ST_UID].size(); i++)
				{
					StSvr svr;
					svr._ip = svrs[ST_UID][i]._ip;
					svr._port = svrs[ST_UID][i]._port;
					servers.push_back(svr);
				}
				g_uid_conn.refresh(&servers);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "uid svr is empty.\n");
			}
			
			Thread_Mutex_Guard guard(_mutex_svr);
			_svrs = svrs;

		}
		else
		{
			XCP_LOGGER_INFO(&g_logger, "get_server_access failed, err_info:%s\n", err_info.c_str());
		}	
		
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "get conf svr conn failed.\n");
	}

	return nRet;
	
}




//获取副本
StSysInfo Conf_Mgt::get_sysinfo()
{
	Thread_Mutex_Guard guard(_mutex);
	return _sysInfo;
}



std::vector<StSvr> Conf_Mgt::get_conf_svr()
{
	Thread_Mutex_Guard guard(_mutex);
	return _conf_svr;

}


void Conf_Mgt::get_svrs(std::map<std::string, std::vector<StSvr> > &svrs)
{
	Thread_Mutex_Guard guard(_mutex_svr);
	svrs = _svrs;
}


