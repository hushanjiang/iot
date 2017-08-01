
#include "base/base_os.h"
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_xml_parser.h"
#include "base/base_logger.h"
#include "conf_mgt.h"

extern Logger g_logger;

Conf_Mgt::Conf_Mgt(): _cfg("")
{

}


Conf_Mgt::~Conf_Mgt()
{

}



int Conf_Mgt::init(const std::string &cfg)
{
	_cfg = cfg;
	return refresh();
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
	nRet = _parser.get_node("lb_svr/system/id", node);
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
	nRet = _parser.get_node("lb_svr/system/ip", node);
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
	nRet = _parser.get_node("lb_svr/system/port", node);
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

	//worker_port
	nRet = _parser.get_node("lb_svr/system/worker_port", node);
	if(nRet != 0)
	{
		printf("get worker_port failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get worker_port failed, ret:%d\n", nRet);
		return -1;
	}
	else
	{
		sysInfo._worker_port = (unsigned short)atoll(node.get_text().c_str());
		if(sysInfo._worker_port == 0)
		{
			printf("worker_port is 0\n");
			XCP_LOGGER_INFO(&g_logger, "worker_port is 0\n");
			return -1;
		}
	}

	
	//thr_num
	nRet = _parser.get_node("lb_svr/system/thr_num", node);
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
	nRet = _parser.get_node("lb_svr/system/max_queue_size", node);
	if(nRet != 0)
	{
		printf("get max_queue_size failed, ret:%d\n", nRet);
		XCP_LOGGER_INFO(&g_logger, "get max_queue_size failed, ret:%d\n", nRet);
	}
	sysInfo._max_queue_size = (unsigned int)atoll(node.get_text().c_str());
	if(sysInfo._max_queue_size < 100000)
	{
		printf("max_queue_size < 100000\n");
		sysInfo._max_queue_size = 100000;
	}

	//TZ
	nRet = _parser.get_node("lb_svr/system/TZ", node);
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
	
	return 0;

}



//获取副本
StSysInfo Conf_Mgt::get_sysinfo()
{
	Thread_Mutex_Guard guard(_mutex);
	return _sysInfo;
}


