
#include "base/base_os.h"
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_xml_parser.h"
#include "base/base_logger.h"
#include "conf_timer_handler.h"
#include "conf_mgt.h"
#include "conn_mgt_lb.h"
#include "redis_mgt.h"
#include "rsp_tcp_event_handler.h"
#include "register_event_handler.h"
#include "msg_tcp_event_handler.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;

extern Conn_Mgt_LB g_conf_mgt_conn;
extern Conn_Mgt_LB g_lb_conn;
extern Conn_Mgt_LB g_user_mgt_conn;
extern Conn_Mgt_LB g_device_mgt_conn;
extern Conn_Mgt_LB g_family_mgt_conn;
extern Conn_Mgt_LB g_push_conn;
extern Conn_Mgt_LB g_mdp_conn;


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
		printf("init conf mgt conn, ret:%d\n", nRet);
		return nRet;
	}

	//lb svr
	std::vector<StSvr> svrs;
	Register_Event_Handler *handler1 = new Register_Event_Handler(&g_lb_conn, ST_LB);
	nRet = g_lb_conn.init(&svrs, handler1, true, ST_LB);
	if(nRet != 0)
	{
		printf("init lb conn.\n");
		return nRet;
	}

	//user mgt
	RSP_TCP_Event_Handler *handler2 = new RSP_TCP_Event_Handler(ST_USER_MGT);
	nRet = g_user_mgt_conn.init(&svrs, handler2, true);
	if(nRet != 0)
	{
		printf("init user mgt conn.\n");
		return nRet;
	}
	
	//device mgt
	RSP_TCP_Event_Handler *handler3 = new RSP_TCP_Event_Handler(ST_DEVICE_MGT);
	nRet = g_device_mgt_conn.init(&svrs, handler3, true);
	if(nRet != 0)
	{
		printf("init device mgt conn.\n");
		return nRet;
	}
	
	//family mgt
	RSP_TCP_Event_Handler *handler4 = new RSP_TCP_Event_Handler(ST_FAMILY_MGT);
	nRet = g_family_mgt_conn.init(&svrs, handler4, true);
	if(nRet != 0)
	{
		printf("init family mgt conn.\n");
		return nRet;
	}

	//push svr
	//RSP_TCP_Event_Handler *handler5 = new RSP_TCP_Event_Handler(ST_PUSH);
	//nRet = g_push_conn.init(&svrs, handler5, true);
	nRet = g_push_conn.init(&svrs);
	if(nRet != 0)
	{
		printf("init push conn.\n");
		return nRet;
	}

	//mdp
	Msg_TCP_Event_Handler *handler6 = new Msg_TCP_Event_Handler(&g_mdp_conn);
	nRet = g_mdp_conn.init(&svrs, handler6, true, ST_MDP);
	if(nRet != 0)
	{
		printf("init mdp conn.\n");
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
	nRet = _parser.get_node("access_svr/system/id", node);
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
	nRet = _parser.get_node("access_svr/system/ip", node);
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


	//ip_out
	nRet = _parser.get_node("access_svr/system/ip_out", node);
	if(nRet != 0)
	{
		printf("get ip_out failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get ip_out failed, ret:%d\n", nRet);
		sysInfo._ip_out = sysInfo._ip;
	}
	else
	{
		sysInfo._ip_out = node.get_text();
		trim(sysInfo._ip_out);
		if(sysInfo._ip_out == "")
		{
			printf("ip_out is empty\n");
			XCP_LOGGER_INFO(&g_logger, "ip_out is empty\n");
			return -1;
		}
	}

	
	//port
	nRet = _parser.get_node("access_svr/system/port", node);
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
	
	//Message ID命名规则： [svr id]_[ip_out]_[port]
	sysInfo._log_id = format("%s_%s_%u", sysInfo._id.c_str(), sysInfo._ip_out.c_str(), sysInfo._port);
	sysInfo._new_id = format("%s_%llu_%d", sysInfo._log_id.c_str(), getTimestamp(), get_pid());
	
	//thr_num
	nRet = _parser.get_node("access_svr/system/thr_num", node);
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
	nRet = _parser.get_node("access_svr/system/max_queue_size", node);
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
	nRet = _parser.get_node("access_svr/system/TZ", node);
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
	nRet = _parser.get_nodes("access_svr/conf/svr", vecNode);
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


	//------------------ redis ------------------
	std::vector<StRedis_Access> redises;

	vecNode.clear();
	nRet = _parser.get_nodes("access_svr/redis/svr", vecNode);
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
			//lb
			if(!svrs[ST_LB].empty())
			{
				std::vector<StSvr> servers;
				for(unsigned int i=0; i<svrs[ST_LB].size(); i++)
				{
					StSvr svr;
					svr._ip = svrs[ST_LB][i]._ip;
					svr._port = svrs[ST_LB][i]._port;
					servers.push_back(svr);
				}
				g_lb_conn.refresh(&servers);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "lb svr is empty.\n");
			}
			
			//user mgt
			if(!svrs[ST_USER_MGT].empty())
			{
				std::vector<StSvr> servers;
				for(unsigned int i=0; i<svrs[ST_USER_MGT].size(); i++)
				{
					StSvr svr;
					svr._ip = svrs[ST_USER_MGT][i]._ip;
					svr._port = svrs[ST_USER_MGT][i]._port;
					servers.push_back(svr);
				}
				g_user_mgt_conn.refresh(&servers);

			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "user mgt svr is empty.\n");
			}

			//device mgt
			if(!svrs[ST_DEVICE_MGT].empty())
			{
				std::vector<StSvr> servers;
				for(unsigned int i=0; i<svrs[ST_DEVICE_MGT].size(); i++)
				{
					StSvr svr;
					svr._ip = svrs[ST_DEVICE_MGT][i]._ip;
					svr._port = svrs[ST_DEVICE_MGT][i]._port;
					servers.push_back(svr);
				}
				g_device_mgt_conn.refresh(&servers);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "device mgt svr is empty.\n");
			}

			//family mgt
			if(!svrs[ST_FAMILY_MGT].empty())
			{
				std::vector<StSvr> servers;
				for(unsigned int i=0; i<svrs[ST_FAMILY_MGT].size(); i++)
				{
					StSvr svr;
					svr._ip = svrs[ST_FAMILY_MGT][i]._ip;
					svr._port = svrs[ST_FAMILY_MGT][i]._port;
					servers.push_back(svr);
				}
				g_family_mgt_conn.refresh(&servers);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "family mgt svr is empty.\n");
			}

			//push
			if(!svrs[ST_PUSH].empty())
			{
				std::vector<StSvr> servers;
				for(unsigned int i=0; i<svrs[ST_PUSH].size(); i++)
				{
					StSvr svr;
					svr._ip = svrs[ST_PUSH][i]._ip;
					svr._port = svrs[ST_PUSH][i]._port;
					servers.push_back(svr);
				}
				g_push_conn.refresh(&servers);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "push svr is empty.\n");
			}

			//mdp
			if(!svrs[ST_MDP].empty())
			{
				std::vector<StSvr> servers;
				for(unsigned int i=0; i<svrs[ST_MDP].size(); i++)
				{
					StSvr svr;
					svr._ip = svrs[ST_MDP][i]._ip;
					svr._port = svrs[ST_MDP][i]._port;
					servers.push_back(svr);
				}
				g_mdp_conn.refresh(&servers);
			}
			else
			{
				XCP_LOGGER_INFO(&g_logger, "mdp svr is empty.\n");
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


