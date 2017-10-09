#include "base/base_common.h"
#include "base/base_args_parser.h"
#include "base/base_reactor.h"
#include "base/base_signal.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_os.h"
#include "base/base_utility.h"

#include "admin.h"
#include "mongo_mgt.h"
#include "conf_mgt.h"
#include "processor.h"
#include "req_tcp_event_handler.h"

USING_NS_BASE;

Logger g_logger;         //内部日志打印器
StSysInfo g_sysInfo;

void usage()
{
    fprintf(stderr, 
        "Usage: ./family_mgt_svr.bin -s cfg\n\n"
        "  -s  Family_Mgt_Svr_cfg\n\n"
        "  build time: %s %s\n"
        "  Bug reports, feedback, etc, to: 89617663@qq.com\n"
        "\n",
        __DATE__, __TIME__
    );
};




int main(int argc, char * argv[])
{

	int nRet = 0;

	//(1) usage
    if (argc != 3) 
	{
        usage();
        return 0;
    }


	//设置随机数种子
	set_random_seed();
	
	
	//(2) 信号屏蔽
	sigset_t sigset;
	add_signal_in_set(sigset, 3, SIGPIPE, SIGUSR2, SIGTERM);
	nRet = block_process_signal(sigset);
	if(nRet != 0)
	{
		printf("block_process_signal failed, ret:%d\n", nRet);
		return 0;
	}

	//(3) 解析程序运行参数
	printf("--- prepare to start parse arg ---\n");
	Args_Parser args_parser;
	args_parser.parse_args(argc, argv);
	args_parser.show();

	std::string cfg = "";
	if(!args_parser.get_opt("s", &cfg))
	{
		printf("get_opt cfg failed!\n");
		return 0;
	}	
	printf("=== complete to start parse arg ===\n");


	//(4) 初始化配置模块
	printf("--- prepare to init conf mgt ---\n");
	nRet = PSGT_Conf_Mgt->init(std::string("../conf/")+cfg);
	if(nRet != 0)
	{
		printf("init conf mgt failed, ret:%d\n", nRet);
		return 0;
	}
	g_sysInfo = PSGT_Conf_Mgt->get_sysinfo();
	printf("===== complete to init conf mgt =====\n");


	//设置TZ
	if(g_sysInfo._TZ != "")
	{
		char tz[100] = {0};
		snprintf(tz, 100, "TZ=%s", (g_sysInfo._TZ).c_str());
		nRet = putenv(tz);
		if(nRet != 0)
		{
			printf("putenv(%s) failed, ret:%d\n", tz, nRet);
			return nRet;
		}
		else
		{
			printf("putenv(%s) success\n", tz);
		}
	}
	tzset();
	printf("date:%s\n", FormatDateTimeStr().c_str());
	

	//(5) 初始化logger
	nRet = g_logger.init("./../log/", g_sysInfo._new_id, MAX_LOG_SIZE, 3600);
	if(nRet != 0)
	{
		printf("init debug logger failed, ret:%d\n", nRet);
		return nRet;
	}


	XCP_LOGGER_INFO(&g_logger, "=============================================\n");
	XCP_LOGGER_INFO(&g_logger, "        date:%s\n", FormatDateTimeStr().c_str());
	XCP_LOGGER_INFO(&g_logger, "        process:%d, thread:%llu \n", get_pid(), get_thread_id());
	XCP_LOGGER_INFO(&g_logger, "        prepare to start Family Mgt Svr! ...\n");
	XCP_LOGGER_INFO(&g_logger, "=============================================\n");


	//(7) 启动工作线程池
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start processor ---\n");
	Processor processor;
	nRet = processor.init(NULL, g_sysInfo._thr_num);
	if(nRet != 0)
	{
		printf("processor init failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = processor.run();
	if(nRet != 0)
	{
		printf("processor run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start processor ===\n");
	

	//(8) 启动req tcp reactor
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start req reactor(tcp) ---\n");
	Req_TCP_Event_Handler *req_handler = new Req_TCP_Event_Handler;
	StReactorAgrs args_req;
	args_req.port = g_sysInfo._port;
	Reactor *reactor_req = new Reactor(req_handler);
	nRet = reactor_req->init(&args_req);
	if(nRet != 0)
	{
		printf("init req reactor(tcp) failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = reactor_req->run();
	if(nRet != 0)
	{
		printf("req reactor(tcp) run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start req reactor(tcp) ===\n");


	//(9) 启动Admin  --- 主动刷新配置
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start Admin ---\n");
	Admin admin;
	nRet = admin.init(NULL);
	if(nRet != 0)
	{
		printf("Admin init failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = admin.run();
	if(nRet != 0)
	{
		printf("Admin run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start Admin ===\n");

	//(10) 完成Family Mgt Svr  的启动
	XCP_LOGGER_INFO(&g_logger, "===== complete to start Family Mgt Svr! =====\n");
	
	while(true)
	{
		::sleep(5);
	}
	
	return 0;
	
}


