
#include "base/base_common.h"
#include "base/base_args_parser.h"
#include "base/base_reactor.h"
#include "base/base_signal.h"
#include "base/base_logger.h"
#include "base/base_time.h"
#include "base/base_os.h"
#include "base/base_utility.h"

#include "comm/common.h"
#include "admin.h"
#include "conf_mgt.h"
#include "req_mgt.h"
#include "rsp_mgt.h"

#include "req_processor.h"
#include "rsp_processor.h"
#include "msg_processor.h"
#include "req_tcp_event_handler.h"
#include "session_timer_handler.h"
#include "route_release.h"
#include "broadcast_processor.h"


USING_NS_BASE;

Logger g_logger; //内部日志打印器
StSysInfo g_sysInfo;

Reactor *g_reactor_req = NULL;
Req_Mgt *g_req_mgt = NULL;   //客户端请求处理队列
Rsp_Mgt *g_syn_mgt = NULL;   //业务服务同步响应处理队列
Req_Mgt *g_asyn_mgt = NULL;  //业务服务异步响应处理队里
Req_Mgt *g_msg_mgt = NULL;   //MDP通道消息处理队里
Req_Mgt *g_broadcast_mgt = NULL;

void usage()
{
    fprintf(stderr, 
        "Usage: ./access_svr.bin -s cfg\n\n"
        "  -s  Access_svr_cfg\n\n"
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
	add_signal_in_set(sigset, 2, SIGPIPE, SIGUSR2);
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

	//(5)生成pid file
	nRet = pid_file(g_sysInfo._log_id + std::string(".pid"));
	if(nRet != 0)
	{
		printf("pid file failed, ret:%d\n", nRet);
		return nRet;
	}

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

	//(6) 初始化logger
	nRet = g_logger.init("./../log/", g_sysInfo._log_id, MAX_LOG_SIZE, 3600);
	if(nRet != 0)
	{
		printf("init debug logger failed, ret:%d\n", nRet);
		return nRet;
	}

	XCP_LOGGER_INFO(&g_logger, "=============================================\n");
	XCP_LOGGER_INFO(&g_logger, "        date:%s\n", FormatDateTimeStr().c_str());
	XCP_LOGGER_INFO(&g_logger, "        process:%d, thread:%llu \n", get_pid(), get_thread_id());
	XCP_LOGGER_INFO(&g_logger, "        prepare to start Access Svr! ...\n");
	XCP_LOGGER_INFO(&g_logger, "=============================================\n");

	//(7) 启动Req 工作线程池
	g_req_mgt = new Req_Mgt;
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start req processor ---\n");
	Req_Processor req_processor;
	nRet = req_processor.init(NULL, g_sysInfo._thr_num);
	if(nRet != 0)
	{
		printf("init req processor failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = req_processor.run();
	if(nRet != 0)
	{
		printf("run req processor failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start req processor ===\n");


	//(8) 启动Rsp工作线程池
	g_syn_mgt = new Rsp_Mgt;
	g_asyn_mgt = new Req_Mgt;
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start rsp processor ---\n");
	Rsp_Processor rsp_processor;
	nRet = rsp_processor.init(NULL, g_sysInfo._thr_num);
	if(nRet != 0)
	{
		printf("init rsp processor failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = rsp_processor.run();
	if(nRet != 0)
	{
		printf("run rsp processor failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start rsp processor ===\n");


	//(9) 启动req tcp reactor
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start req reactor(tcp) ---\n");
	Req_TCP_Event_Handler *req_handler = new Req_TCP_Event_Handler;
	StReactorAgrs args_req;
	args_req.port = g_sysInfo._port;
	g_reactor_req = new Reactor(req_handler);
	nRet = g_reactor_req->init(&args_req);
	if(nRet != 0)
	{
		printf("init req reactor(tcp) failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = g_reactor_req->run();
	if(nRet != 0)
	{
		printf("req reactor(tcp) run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start req reactor(tcp) ===\n");


	//(10) 启动Msg 工作线程池
	g_msg_mgt = new Req_Mgt;
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start msg processor ---\n");
	Msg_Processor msg_processor;
	nRet = msg_processor.init(NULL, g_sysInfo._thr_num);
	if(nRet != 0)
	{
		printf("init msg processor failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = msg_processor.run();
	if(nRet != 0)
	{
		printf("run msg processor failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start msg processor ===\n");


	//(11) 启动Broadcast 工作线程池
	g_broadcast_mgt = new Req_Mgt;
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start broacast processor ---\n");
	Broadcast_Processor broacast_processor;
	nRet = broacast_processor.init(NULL, g_sysInfo._thr_num);
	if(nRet != 0)
	{
		printf("init broacast processor failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = broacast_processor.run();
	if(nRet != 0)
	{
		printf("run broacast processor failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start broacast processor ===\n");


	//(12) 启动Route_Release
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start Route_Release ---\n");
	Route_Release *route_release = new Route_Release;
	nRet = route_release->init(NULL);
	if(nRet != 0)
	{
		printf("Route_Release init failed, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = route_release->run();
	if(nRet != 0)
	{
		printf("Route_Release run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start Route_Release ===\n");


	//(13) 启动Admin  --- 主动刷新配置
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


	//(13)启动session timer  --- 10分钟检查一次，1分钟该链接没有数据处理就关闭该链接
	XCP_LOGGER_INFO(&g_logger, "--- prepare to start session timer ---\n");
	Select_Timer *timer_session = new Select_Timer;
	Session_Timer_handler *session_thandler = new Session_Timer_handler;
	nRet = timer_session->register_timer_handler(session_thandler, 600000000);
	if(nRet != 0)
	{
		printf("register session timer handler falied, ret:%d\n", nRet);
		return nRet;
	}
		
	nRet = timer_session->init();
	if(nRet != 0)
	{
		printf("session timer init failed, ret:%d\n", nRet);
		return nRet;
	}

	//nRet = timer_session->run();
	if(nRet != 0)
	{
		printf("session timer run failed, ret:%d\n", nRet);
		return nRet;
	}
	XCP_LOGGER_INFO(&g_logger, "=== complete to start session timer ===\n");

	
	//(11) 完成access svr  的启动
	XCP_LOGGER_INFO(&g_logger, "===== complete to start Access Svr! =====\n");
	
	while(true)
	{
		::sleep(5);
	}
	
	return 0;
	
}


