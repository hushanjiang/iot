
#include "conn.h"
#include "base/base_logger.h"
#include "base/base_string.h"
#include "base/base_time.h"
#include "base/base_uid.h"
#include "protocol.h"
#include "client_mgt.h"
#include "request.h"
#include "rsp_mgt.h"

extern Logger g_logger;
extern StSysInfo g_sysInfo;
extern Rsp_Mgt *g_syn_mgt;

Conn::Conn(StSvr svr, Event_Handler *handler, bool asyn): _svr(svr), _stmp_create(0),
	_stmp_hb(0), _client(NULL), _handler(handler), _asyn(asyn), _registered(false)
{
	Event_Handler *tmp_handler = NULL;
	if(handler && _asyn)
	{
		tmp_handler = handler->renew();
	}
	
	_client = new TCP_Client_Epoll(tmp_handler, _asyn);
	
	if(asyn)
	{
		/*
		为了防止对端关闭连接后，_client 线程被自动释放了，
		别的线程再对_client 操作导致crash
		*/
		_client->inc_ref();
	}
	
}



Conn::~Conn()
{
	printf("-------- prepare to release conn --------\n");

	close();

	if(_asyn)
	{
		//异步的TCP Client 一定要保证线程退出后才能释放对象。
		while(Thread::ST_EXIT != _client->thr_status())
		{
			::sleep(1); //线程执行回调函数1秒后释放tcp client 对象。
		}
	}

	//释放TCP Client 对象
	DELETE_POINTER(_client);

	printf("-------- complete to release conn --------\n");
	
}




int Conn::connect()
{
	int nRet = 0;

	if(!is_close())
	{
		return 0;
	}
	
	_registered = false;

	nRet = _client->open(_svr._ip, _svr._port);
	if(nRet == 0)
	{
		_stmp_create = getTimestamp();
		XCP_LOGGER_INFO(&g_logger, "connect to svr(%s:%u) success\n", _svr._ip.c_str(), _svr._port);
	}
	else
	{
		_client->close();
		XCP_LOGGER_INFO(&g_logger, "connect to svr(%s:%u) failed, ret:%d\n", _svr._ip.c_str(), _svr._port, nRet);
	}
	
	return nRet;
	
}




int Conn::check()
{
	int nRet = 0;

	return nRet;
}



void Conn::close()
{
	printf("-------- close conn --------\n");
	_client->close();
	_registered = false;
}



bool Conn::is_close()
{
	return _client->is_close();
}



int Conn::send(const char *buf, unsigned int &len, int flags, unsigned int timeout)
{
	int nRet = 0;

	if(_client->is_close())
	{
		XCP_LOGGER_INFO(&g_logger, "conn to svr(%s:%u) err.\n", _svr._ip.c_str(), _svr._port);
		return -1;
	}

	nRet = _client->send_msg(buf, len, flags, timeout);
	if(nRet == 0)
	{
		//XCP_LOGGER_INFO(&g_logger, "send to svr(%s:%u) success.\n", _ip.c_str(), _port);
	}
	else if(nRet == 2)
	{
		XCP_LOGGER_INFO(&g_logger, "send to svr(%s:%u) timeout, msg(%u):%s\n", 
			_svr._ip.c_str(), _svr._port, (unsigned int)strlen(buf), buf);
	}
	else
	{
		//发送失败后会关闭该socket
		/*
		同步TCP 客户端， 如果对端关闭。由于是同步所以无法知道对端关闭，后面的流程如下:
		(1) 还可以从 conn mgt 中获取到该conn， send 成功， 对端返回RST报文
		(2) 使用该conn 接收数据失败，recv 返回0, 表示对端已经关闭
		*/
		_client->close();
		XCP_LOGGER_INFO(&g_logger, "send to svr(%s:%u) failed, ret:%d, msg(%u):%s\n",  
			_svr._ip.c_str(), _svr._port, nRet, (unsigned int)strlen(buf), buf);
	}
	
	return nRet;
	
}



int Conn::rcv(char *buf, unsigned int &len, unsigned int timeout)
{
	int nRet = 0;

	if(_client->is_close())
	{
		XCP_LOGGER_INFO(&g_logger, "conn to svr(%s:%u) err.\n", _svr._ip.c_str(), _svr._port);
		return -1;
	}

	nRet = _client->rcv_msg(buf, len, timeout);
	if(nRet == 1)
	{
		//XCP_LOGGER_INFO(&g_logger, "rcv from svr(%s:%u)  success.\n", _stServer._ip.c_str(), _stServer._port);
	}	
	else if(nRet == 2)
	{
		XCP_LOGGER_INFO(&g_logger, "rcv from svr(%s:%u) timeout, msg(%u):%s\n", 
			_svr._ip.c_str(), _svr._port, (unsigned int)strlen(buf), buf);
	}
	else
	{
		_client->close();
		XCP_LOGGER_INFO(&g_logger, "the svr(%s:%u) is close.\n", _svr._ip.c_str(), _svr._port);
	}
	
	return nRet;

}




int Conn::register_lb()
{
	int nRet = 0;

	XCP_LOGGER_INFO(&g_logger, "-------- register lb -------\n");

	std::string msg = "";
	msg = XProtocol::register_lb_req(UID::uid_inc(g_sysInfo._log_id), g_sysInfo._new_id, g_sysInfo._ip_out, g_sysInfo._port);

	unsigned int len = msg.size();
	nRet = this->send(msg.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send register lb failed, ret:%d\n", nRet);
		nRet = ERR_SEND_FAIL;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "send register lb success.\n");
	}

	return nRet;

}




int Conn::hb_lb()
{
	int nRet = 0;
	
	std::string msg = XProtocol::hb_lb_req(UID::uid_inc(g_sysInfo._log_id), PSGT_Client_Mgt->client_num());
	unsigned int len = msg.size();
	nRet = this->send(msg.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send hb_lb failed, ret:%d\n", nRet);
		nRet = ERR_SEND_FAIL;
	}
	
	return nRet;
}




int Conn::register_mdp()
{
	int nRet = 0;

	XCP_LOGGER_INFO(&g_logger, "-------- register mdp -------\n");

	std::string msg = "";
	msg = XProtocol::register_mdp_req(UID::uid_inc(g_sysInfo._log_id), g_sysInfo._new_id);

	unsigned int len = msg.size();
	nRet = this->send(msg.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send register mdp failed, ret:%d\n", nRet);
		nRet = ERR_SEND_FAIL;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "send register mdp success.\n");
	}

	return nRet;

}




int Conn::get_server_access(std::map<std::string, std::vector<StSvr> > &svrs, std::string &err_info)
{	
	int nRet = 0;
	err_info = "";
	
	std::string msg = XProtocol::get_server_access_req(UID::uid_inc(g_sysInfo._log_id));
	unsigned int len = msg.size();
	nRet = this->send(msg.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send get_server_access req to conf svr failed, ret:%d\n", nRet);
		err_info = "send get_server_access req to conf svr failed.";
		return ERR_SEND_FAIL;
	}

	char buf[4096] = {0};
	len = 4095;
	nRet = this->rcv(buf, len);
	if(nRet != 1)
	{
		XCP_LOGGER_INFO(&g_logger, "rcv server access from conf svr failed, ret:%d\n", nRet);
		err_info = "rcv server access from conf svr failed.";
		return ERR_RCV_FAIL;
	}
	
	nRet = XProtocol::get_server_access_rsp(buf, svrs, err_info);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "get server access rsp failed, ret:%d, err_info:%s\n", nRet, err_info.c_str());
		return nRet;
	}
	
	return nRet;
	
}



int Conn::inner_message(const std::string &msg_tag, std::string &req, std::string &err_info)
{
	XCP_LOGGER_INFO(&g_logger, "-------- inner_message -------\n");
	
	int nRet = 0;
	err_info = "";

	req += std::string("\n");
	unsigned int len = req.size();
	nRet = this->send(req.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send msg to inner svr failed, ret:%d\n", nRet);
		err_info = "send msg to inner svr failed.";
		return ERR_SEND_FAIL;
	}
	
	return nRet;
}



int Conn::check_talk_condition(const std::string &msg_tag, const std::string &from_svr, const std::string &uuid, const std::string &encry, const std::string &session_id, 
	const unsigned long long src_id, const unsigned long long target_id, const std::string &msg_type, std::string &err_info)
{
	XCP_LOGGER_INFO(&g_logger, "-------- check user talk condition -------\n");
	
	int nRet = 0;
	err_info = "";

	std::string new_req = XProtocol::check_talk_condition_req(msg_tag, from_svr, uuid, encry, session_id, src_id, target_id, msg_type);
	unsigned int len = new_req.size();
	nRet = this->send(new_req.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send check_user_talk_condition_req failed, ret:%d\n", nRet);
		err_info = "send check_user_talk_condition_req failed";
		return ERR_SEND_FAIL;
	}
	
	Request_Ptr msg;
	nRet = g_syn_mgt->get_req(msg_tag, msg);
	if(nRet == 0)
	{
		nRet = XProtocol::get_rsp_result(msg->_req, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "get_rsp_result is failed, ret:%d, err_info:%s\n", nRet, err_info.c_str());
			return nRet;
		}

	}
	else
	{
		err_info = "can't get rsp of check talk condition.";
		XCP_LOGGER_INFO(&g_logger, "can't get rsp of check talk condition, ret:%d, msg_tag:%s\n", nRet, msg_tag.c_str());
	}
	
	return nRet;

}



int Conn::get_member_id_list(const std::string &msg_tag, const std::string &from_svr, const std::string &uuid, const std::string &encry, const std::string &session_id,
	const unsigned long long user_id, const unsigned long long family_id, std::set<unsigned long long> &members, std::string &err_info)
{
	XCP_LOGGER_INFO(&g_logger, "-------- get member id list -------\n");
	
	int nRet = 0;
	err_info = "";

	std::string new_req = XProtocol::get_member_id_list_req(msg_tag, from_svr, uuid, encry, session_id, user_id, family_id);
	unsigned int len = new_req.size();
	nRet = this->send(new_req.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send get_member_id_list_req failed, ret:%d\n", nRet);
		return nRet;
	}
	
	Request_Ptr msg;
	nRet = g_syn_mgt->get_req(msg_tag, msg);
	if(nRet == 0)
	{
		nRet = XProtocol::get_member_id_list_rsp(msg->_req, members, err_info);
		if(nRet != 0)
		{
			XCP_LOGGER_INFO(&g_logger, "get_member_id_list_rsp failed, ret:%d\n", nRet);
			return nRet;
		}
	}
	else
	{
		err_info = "can't get rsp of get member id list.";
		XCP_LOGGER_INFO(&g_logger, "can't get rsp of get member id list, ret:%d, msg_tag:%s\n", nRet, msg_tag.c_str());
	}
		
	return nRet;

}



int Conn::notify_client_status(const std::string &msg_tag, unsigned long long id, const std::string &type, const std::string &status, std::string &err_info)
{
	XCP_LOGGER_INFO(&g_logger, "-------- notify client status -------\n");
	
	int nRet = 0;
	err_info = "";
	
	std::string msg = XProtocol::notify_client_status_req(msg_tag, id, type, status);
	unsigned int len = msg.size();
	nRet = this->send(msg.c_str(), len);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "send notify client status req failed, ret:%d\n", nRet);
		err_info = "send notify client status req failed.";
		return ERR_SEND_FAIL;
	}
	
	return nRet;
}

