#include "MTNProtocol.h"
#include "MTNMsgApi.h"
#include "socktool.h"	
#include "time.h"
#include "Md5.h"
#include "base64.h"
	
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <limits.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>  
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/select.h> 
#include <sys/types.h>
#include <unistd.h>
#include <iostream> 
#include <pthread.h>
#include <sys/ioctl.h>	
#include <fcntl.h>	
#include <net/if.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

//#define EUCPC_DEBUG

#ifdef EUCPC_DEBUG  
	#include "log.h"
#endif

	
using namespace std;





namespace MTNMsgApi_NP
{
	
extern char* vertemp;  

#ifdef EUCPC_DEBUG    		
	extern CLog g_Log;
#endif		
	
	
	
#define TIMEDELTA_MS(time1,time0) \
  ((time1.tv_sec-time0.tv_sec)*1000+(time1.tv_usec-time0.tv_usec)/1000)
  	
  	
TMTNMsgApi::TMTNMsgApi() 
{
	m_bSockError = true;
	memset(m_ip,0,sizeof(m_ip));
	m_port = 80;
#ifdef EUCPC_DEBUG    		
	m_port = 9000;
#endif		
	
	
	m_bOnWork = true;
	
	m_bThreadCreated = false;
	
	memset(m_KeyVal,0,sizeof(m_KeyVal));
	//get netmac.
	
	char Mac[100];
	//int ret = GetNetMac(Mac);
	GetNetMac(Mac);
    char* temp = Md5(Mac);
	memcpy(m_KeyVal, temp, strlen(temp));
    delete[] temp;
	temp = NULL;
	
	memset(m_ServiceID,0,sizeof(m_ServiceID));
	strcpy(m_ServiceID,"01");
	m_ServiceID[2] = '\0';
	
	m_bIsRecSms = false;
	m_bIsRecReport = false;
	pthread_mutex_init(&m_sc_mutex,NULL);
	pthread_cond_init(&m_sc_cond,NULL);
	//
	m_bStop = true;
	m_nActiveTimes = 0;
	
	//
	m_nSockFunThreadNum = 0;
	m_nSCFunThreadNum = 0;
	//
	
	memset(m_strTemp,0,sizeof(m_strTemp));
	m_nPos = 0;
	m_nSmsPos = 0;
	m_nSmsCount = 0;
	m_nReportPos = 0;
	m_nReportCount = 0;
	
	//
	memset(m_arrSN,0,sizeof(m_arrSN));
	memset(m_arrBalance,0,sizeof(m_arrBalance));
	m_nCurSnPos = 0;


}
TMTNMsgApi::~TMTNMsgApi() 
{

	pthread_mutex_destroy(&m_sc_mutex);
	pthread_cond_destroy(&m_sc_cond);

}

int TMTNMsgApi::Close()
{
	m_sock = INVALID_SOCKET;
	m_nActiveTimes = 0;					
	m_bSockError = true;			
	m_bStop = true;

	sleep(4);//wait.
	
	int iOut = 0;
	while(m_socketThread.m_dwThreadId != 0 || m_SCThread.m_dwThreadId != 0)
	{
		iOut++;
		sleep(1);
		
		if(iOut> 10)
			break;
	}
		
	if(iOut>10)
	{
		return 0;
	}
	
	return 1;
}
int TMTNMsgApi::GetNetMac(char* netMac)
{
	struct ifreq ifreqTemp;
	
	int sock;
	
	if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
	{
#ifdef EUCPC_DEBUG    		
		g_Log.writeLog(D1,"GetNetMac(): socket erro");		
#endif		
		return -1;
	}
	strcpy(ifreqTemp.ifr_name,"eth0");
	if(ioctl(sock,SIOCGIFHWADDR,&ifreqTemp)<0)
	{
#ifdef EUCPC_DEBUG    		
		g_Log.writeLog(D1,"GetNetMac(): ioctl error");		
#endif		
		::close(sock);
		return -2;
	}
	sprintf(netMac,"%02x:%02x:%02x:%02x:%02x:%02x",
					(unsigned char)ifreqTemp.ifr_hwaddr.sa_data[0],
					(unsigned char)ifreqTemp.ifr_hwaddr.sa_data[1],
					(unsigned char)ifreqTemp.ifr_hwaddr.sa_data[2],
					(unsigned char)ifreqTemp.ifr_hwaddr.sa_data[3],
					(unsigned char)ifreqTemp.ifr_hwaddr.sa_data[4],
					(unsigned char)ifreqTemp.ifr_hwaddr.sa_data[5]);
	::close(sock);
	return 0;	
}
//获得IP地址
int TMTNMsgApi::domainToIP(char* domain)    //byz modify method declared 20050523
{

	struct hostent *host = NULL; 
	struct in_addr ia;

	char strTemp[50];  //byz be released 20050523
	memset(strTemp,0,sizeof(strTemp));
	
	int len = 0;
	host = gethostbyname(domain);	//not free hst
	if(NULL == host)
	{
		return -1;
	}
	else if(host != NULL) 
	{
		memcpy(&ia.s_addr,host->h_addr_list[0],sizeof(ia.s_addr));
		sscanf(inet_ntoa(ia),"%s/n",strTemp);
		len = strlen(strTemp);
		memcpy(m_ip,strTemp,len);
		
		return 0;
	}
	return -1;
}

//验证是否为数字
int TMTNMsgApi::ValidateDigit(const char* mobile) 
{
	if (mobile == NULL) return 0;
	for(unsigned int i = 0; i < strlen(mobile); i++) 
	{		
		if(isdigit(mobile[i]) == 0)
		{	   
			return 0;
		}
	}
	return 1;
}
int TMTNMsgApi::ValidateDigitPriority(const char* priority) 
{
	if (priority == NULL || strlen(priority) == 0) return 0;
	int len = strlen(priority);
	if (len > 1) return 0;
	if (!ValidateDigit(priority)) return 0;
	return 1;
}

void TMTNMsgApi::SetSocket(int sk) 
{
	this->m_sock = sk;
}

void TMTNMsgApi::ActiveThread()
{
	m_socketThread.create(static_thread_func, (void*)this);
	if(m_bThreadCreated)  
	{
		return;
	}
	else
	{
		m_SCThread.create(static_sc_thread_func, (void*)this);
		m_bThreadCreated = true;	
	}
}
int TMTNMsgApi::static_sc_thread_func(void* p)
{
	int ret = reinterpret_cast<TMTNMsgApi*>(p)->ActiveSocketSCFun();
	return ret;
}
int TMTNMsgApi::static_thread_func(void* p)
{
	int ret = reinterpret_cast<TMTNMsgApi*>(p)->ActiveSocketFun();
	return ret;
}




int TMTNMsgApi::on_read()
{
	
	try
	{
		int ret = splitStr();
		if(ret <= 0) //socket error
		{
#ifdef EUCPC_DEBUG    		
		g_Log.writeLog(D1,"splitStr()  recv socket error!");	
#endif				
			return ret;
		}
		else// if>0  the socket not error .return 1;
			return 1;
	}
	catch(...)
	{
#ifdef EUCPC_DEBUG    		
		g_Log.writeLog(D1,"on_read() error");	
#endif	
	}
	return 0;
}

int TMTNMsgApi::splitStr()
{

	int nLen = sizeof(m_strTemp);
	int nRead = -1;
	int recvCount = 0;
	if(true)
	{
		recvCount++;
		nRead = recv(this->m_sock,&m_strTemp[m_nPos],nLen,0);
#ifdef EUCPC_DEBUG    	
		g_Log.writeLog(D1,"splitStr() recv() m_strTemp:%s",m_strTemp);
#endif		
		
		if(nRead == SOCKET_ERROR)
		{  
#ifdef EUCPC_DEBUG    	
		g_Log.writeLog(D1,"splitStr() recv() nRead == SOCKET_ERROR");
#endif
			return -1;
		}
		else if(0 == nRead)
		{
#ifdef EUCPC_DEBUG    	
		g_Log.writeLog(D1,"splitStr() recv() nRead = [%d]",nRead);
#endif
			return 0;
		}
		else 
		{
	    	//now recv data.
		    m_nPos += nRead;
		    while(true)
		    {
		        char* atPos = strstr(m_strTemp,"@");
		        if(NULL == atPos)
		        {
		            return 2;
		        }
		
		        char tmpStr[16*1024];
		        memset(tmpStr,0,sizeof(tmpStr));
		        int readCount = atPos - m_strTemp + 1;
		        memcpy(tmpStr,m_strTemp,readCount);
		        //
						#ifdef EUCPC_DEBUG    	
							g_Log.writeLog(D1,"splitStr() recv() tmpStr:%s",tmpStr);
						#endif	        
		        parse(tmpStr);
		        //
		        int leftLen = m_nPos - readCount;
		        if(leftLen <= 0)//no other char
		        {
		            m_nPos = 0;
		            memset(m_strTemp,0,sizeof(m_strTemp));
		            return 3;
		        }
		        else
		        {
		            memmove(m_strTemp,(atPos+1),leftLen);
		        }
		        
		        //
		        m_nPos = leftLen;
		        int needSet = sizeof(m_strTemp)-m_nPos;
		        if(needSet <=0)//no @
		        {
		            m_nPos = 0;
		            memset(m_strTemp,0,sizeof(m_strTemp));
		            return 4;
		        }
		        else
		        {
		            memset(&m_strTemp[m_nPos],0,needSet);
		        }
		    }
		}
	}
	return 1;
}

//--<Added by hai>
/*
* Check if the message is heartbeat message.
* @msg, the message string with terminating null.
* @seq_number, the returned sequence number from heartbeat message.
*
* Notice,there it will not check if the string is empty.
*/
bool TMTNMsgApi::is_heartbeat_msg(const char* msg,char* seq_number_str)
{
	const char* pos = strstr(msg,",");
	if (NULL == pos)
		return false;

	pos = strstr(pos + 1,"-");
	if (NULL == pos)
		return false;

	if (strlen(pos + 1) < 4 )
		return false;

	if (0 == strncasecmp(pos + 1,"C015",4))
	{
		if (NULL == seq_number_str)
			return true;

		pos = strstr(pos + 4,",");
		if (NULL == pos)
			return false;

		const char* pos1 = strstr(pos+1,",");
		if (NULL == pos1)
			return false;

		strncpy(seq_number_str,pos + 1, (pos1 - pos -1));

		return true;
	}

	return false;
}

/*
* Send heartbeat response message to server, and use the specific sequence number in request message.
*
* @seq_number_str, sequence number string.
*/
int TMTNMsgApi::send_heartbeat_resp(const char* seq_number_str)
{

	if (NULL == seq_number_str)
		return -1;

	//99,[A-Z]{4}-C\d{3}-[A-Z]{10},ACTI-C015-ACTIVETEST,1652,ACTI@
	char heartbeat_resp[MAX_MSG_BODY_LEN];
	strcpy(heartbeat_resp,"99,");
	strcat(heartbeat_resp,"[A-Z]{4}\\-C\\d{3}-[A-Z]{10},ACTI-C015-ACTIVETEST,");
	strcat(heartbeat_resp,seq_number_str);
	strcat(heartbeat_resp,",ACTI@");

	//printf("Sent heartbeat response to server,content \'%s\'\n",heartbeat_resp);

	return (SendMsgBody(heartbeat_resp,strlen(heartbeat_resp)));
}
//--<Added by hai>

int TMTNMsgApi::parse(char* src_in)
{		
	char tmpStr[MAX_MSG_BODY_LEN];
	memset(tmpStr,0,sizeof(tmpStr));
	int len = -1;
	
	RecvMsgBody tmpMsgBody;
	try
	{
		
		
	    char *atPos = strstr(src_in,"@");
	    char *atPos2 = NULL;
	    if(NULL == atPos)
	    {
	    	return -1;
	    }
	    

		atPos = strstr(src_in,",MOANDRPT-");
		

		if(NULL == atPos)
		{
			atPos = strstr(src_in,",");
			len = atPos - src_in;
			memcpy(tmpMsgBody.m_strResult,src_in,len);
			//cout << tmpMsgBody.m_strResult << endl;
		}
		else
		{
			atPos = strstr(src_in,".R001-");
			if(NULL != atPos)
			{
				atPos2 = strstr(src_in,",");
				len = atPos2 - atPos - 6;
				memcpy(tmpMsgBody.m_strCnt,(atPos+6),len);
				atPos = strstr(src_in,",");
			}
			else
			{
				atPos = strstr(src_in,".R002-");
				atPos2 = strstr(src_in,",");
				len = atPos2 - atPos - 6;
				memcpy(tmpMsgBody.m_strCnt,(atPos+6),len);
				#ifdef EUCPC_DEBUG   
					g_Log.writeLog(D1,"splitStr() recv() len=%d,tmpMsgBody.m_strCnt:%s",len,tmpMsgBody.m_strCnt);
				#endif	
				atPos = strstr(src_in,",");
			}  
		}
		
	
		atPos = strstr((atPos+1),",");
		atPos2 = strstr((atPos+1),",");
		

		
		char* atPos3 = strstr(atPos+1,"-");
		//--<Added by hai >
		if (NULL == atPos3)
		{

			/*Now, this message is not common message, then we need check the message type,
			if it is one heartbeat message, we need force to send response message.*/
			//First, check this message if it is one heartbeat from server.
			char seq_number_str[32];
			seq_number_str[0] = '\0';
			if (is_heartbeat_msg(src_in,seq_number_str))
			{
				return (send_heartbeat_resp(seq_number_str));
			}

			return -1;
		}
		//--<Added by hai >

		len = 4;
		memcpy(tmpMsgBody.m_strCommand,(atPos3+1),4);
		#ifdef EUCPC_DEBUG   
			g_Log.writeLog(D1,"splitStr() recv() atPos3:%s,tmpMsgBody.m_strCommand:%s",atPos3,tmpMsgBody.m_strCommand);
		#endif	
		

	
		atPos = strstr(atPos3+1,"-");
		len = atPos2 - atPos -1;
		memcpy(tmpMsgBody.m_strExpress,(atPos+1),len);
		

		
		if(strcmp(tmpMsgBody.m_strResult,"00")==0) 
		{

			
			atPos = strstr((atPos2+1),",");
			len = atPos - atPos2 -1;
			memcpy(tmpMsgBody.m_strKeyid,(atPos2+1),len);
			

		}
		else{
			memcpy(tmpMsgBody.m_strKeyid,"123456",6);
		}
		
		//now judge tmpMsgBody.
		if(strcmp(tmpMsgBody.m_strCommand, "C001")==0)  //注册
		{ 
			 tmpMsgBody.m_nCommandId = 1; 
		}
		else if(strcmp(tmpMsgBody.m_strCommand, "C002")==0)  //发送短信
		{ 
			 tmpMsgBody.m_nCommandId = 2; 
			 if(strlen(tmpMsgBody.m_strCnt) > 1) 
			 {
			 	setBalanceFromSms(tmpMsgBody.m_strCnt);
			 }
			 else if(strlen(tmpMsgBody.m_strExpress) > 1)
			 {
			 	setBalanceFromSms(tmpMsgBody.m_strExpress);
			 }
		}	
		else if(strcmp(tmpMsgBody.m_strCommand, "C003")==0)  //发送定时短信
		{ 
			 tmpMsgBody.m_nCommandId = 3; 
			 
			 if(strlen(tmpMsgBody.m_strCnt) > 1) 
			 {
			 	setBalanceFromSms(tmpMsgBody.m_strCnt);
			 }
			 else if(strlen(tmpMsgBody.m_strExpress) > 1)
			 {
			 	setBalanceFromSms(tmpMsgBody.m_strExpress);
			 }
		}			
		else if(strcmp(tmpMsgBody.m_strCommand, "C004")==0)  //充值
		{ 
			 tmpMsgBody.m_nCommandId = 4; 
		}	
		else if(strcmp(tmpMsgBody.m_strCommand, "C005")==0)  //注销序列号
		{ 
			 tmpMsgBody.m_nCommandId = 5; 
		}			
		else if(strcmp(tmpMsgBody.m_strCommand, "C006")==0)  //企业信息注册
		{ 
			 tmpMsgBody.m_nCommandId = 6; 
		}
		else if(strcmp(tmpMsgBody.m_strCommand, "C007")==0)  //密码修改
		{ 
			 tmpMsgBody.m_nCommandId = 7; 
		}	
		else if(strcmp(tmpMsgBody.m_strCommand, "C008")==0)  //取消转接
		{ 
			 tmpMsgBody.m_nCommandId = 8; 
		}			
		else if(strcmp(tmpMsgBody.m_strCommand, "C010")==0)  //注册转接
		{ 
			 tmpMsgBody.m_nCommandId = 10; 
		}			
		else if(strcmp(tmpMsgBody.m_strCommand, "C011")==0)  //查询余额
		{ 
			 tmpMsgBody.m_nCommandId = 11; 
		}		
		else if(strcmp(tmpMsgBody.m_strCommand, "C012")==0)  //查询单价
		{ 
			 tmpMsgBody.m_nCommandId = 12;	
		}  		
		else if(strcmp(tmpMsgBody.m_strCommand, "C013")==0)  //接收短信
		{ 
			#ifdef EUCPC_DEBUG    		
				g_Log.writeLog(D1,"C013 recv sms bag");
			#endif	
			 tmpMsgBody.m_nCommandId = 13;	
			 addRecvSmsBody(tmpMsgBody.m_strCnt);
		}	
		else if(strcmp(tmpMsgBody.m_strCommand, "C014")==0)  //接收状态报告
		{ 
			#ifdef EUCPC_DEBUG    		
				g_Log.writeLog(D1,"C014 recv report bag");
			#endif		
			#ifdef EUCPC_DEBUG    		
				g_Log.writeLog(D1,"C014 tmpMsgBody.m_strCnt:%s",tmpMsgBody.m_strCnt);
			#endif	
			 tmpMsgBody.m_nCommandId = 14;	
			 addRecvReportBody(tmpMsgBody.m_strCnt);	
		}
		else if(strcmp(tmpMsgBody.m_strCommand, "C015")==0)  //接收心跳包
		{ 
			if(strcmp(tmpMsgBody.m_strResult,"99") == 0) 
			{
#ifdef EUCPC_DEBUG    		
				g_Log.writeLog(D1,"C015 99 recv heart bag");
#endif					
			}
			else if(strcmp(tmpMsgBody.m_strResult,"98") == 0) 	
			{
				SendMsgHead2(1234,1234,1234);
#ifdef EUCPC_DEBUG    		
				g_Log.writeLog(D1,"C015 98 send heart bag");
#endif					
			}
		}
		
		//set value: 1,2,3,4,5,6,7,8,10,11,12
		if(tmpMsgBody.m_nCommandId >= 1 && tmpMsgBody.m_nCommandId <= 12)
		{
			tmpMsgBody.m_nSequenceNum = atoi(tmpMsgBody.m_strKeyid);
			tmpMsgBody.m_nResult = atoi(tmpMsgBody.m_strResult);//0 success.
			
			
			tmpMsgBody.m_bSet = true;
			m_revMsgBody[tmpMsgBody.m_nCommandId] = tmpMsgBody;
		}
	}catch(...)
	{
		
#ifdef EUCPC_DEBUG    	
		g_Log.writeLog(D1,"parse() in cathc() error");
#endif
		return -2;
	}
	return 0;
}




//发送消息包括消息头
int TMTNMsgApi::SendMsgBody(char* msg,int nLen)
{
	auto_lock lock(m_MutexObject);	
	int	status = send(this->m_sock, (char*)msg,nLen,0);
	if(status == SOCKET_ERROR)
	{
#ifdef EUCPC_DEBUG    		
		g_Log.writeLog(D1,"send() fail errno = %d, msg = [%s]",errno,strerror(errno));
#endif
		m_sock = INVALID_SOCKET;
		m_nActiveTimes = 0;					
		m_bSockError = true;			
		m_bStop = true;
		m_bSockError = true;

		return 0;	
	}
	else
	{
#ifdef EUCPC_DEBUG    		
		//g_Log.writeLog(D1,"SendMsgBody() status = %d, nLen = %d",status,nLen);
#endif		
		m_bOnWork = true;
		return 1;
	}	
	

	if(strcmp(msg,"[A-Z]{4}\\-C\\d{3}-[A-Z]{10},ACTI-C015-ACTIVETEST,99,ACTI@") != 0)
	  m_bOnWork = true;
	return 1;
}

int TMTNMsgApi::ConnectTest()
{
   return SendMsgHead1(1234,1234,1234);
}

int TMTNMsgApi::ValidateFlag() 
{
	int nLen = strlen(m_KeyVal);
	if (nLen != 32) 
	{
        return 0; 
	}
	return 1;
}
int TMTNMsgApi::sendAndGetResult(long commID,char*message,int& retResult,char* retValue,char* reconnStr)
{
	int ret = SendMsgBody((char*)message,strlen(message));
	if (ret == 1) 
	{	
		int ret = getResult(commID,retResult,retValue,reconnStr);
		if (ret == 1) 
		{
			if (retResult == 0) 
			{  //ok
				return SUCCESS;
			}
			else
			{
				return retResult;
			}
		}
		else
		{
			return ret;
		}
	}
	else 
	{
		return ret;
	}
	return FAILURE;		
}
//*****声明，所以的返回值为int型的，1为成功，其他为错误,这些是对外发布的方法*****//
//注册消息
int TMTNMsgApi::SendRegister_1(char* sn, char* pwd) 
{
	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,pwd,NULL,m_ServiceID,METONE_FIRST_REGISTRY,NULL,NULL,NULL);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_REG_1,message,retResult,retValue);
	return ret;
}

//修改软件序列号对应的密码
int TMTNMsgApi::RegistryPwdUpd(char* sn, char* oldpwd, char* newpwd) 
{
	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,oldpwd,newpwd,m_ServiceID,METONE_REGISTRY_PWD_UPDATE,NULL,NULL,NULL);
		
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_PWD_UP,message,retResult,retValue);
	return ret;

}
//短信充值
int TMTNMsgApi::ChargeUp(char* sn,char* acco,char* pass) 
{

	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_CHARGE_UP,acco,pass,NULL);

	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_CHARGE_UP,message,retResult,retValue);
	return ret;
	
}

//注销注册
//取消转移功能
int TMTNMsgApi::CancelTransfer (char* sn) 
{
	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_CANCEL_MO_FORWARD,NULL,NULL,NULL);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_CANCAL_TRANS,message,retResult,retValue);
	return ret;
}
int TMTNMsgApi::RegistryTransfer(char* sn,char* phone[]) 
{
	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_MO_FORWARD_EX,NULL,NULL,phone);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_REG_TRANS,message,retResult,retValue);
	return ret;
		
}

//注销注册
int TMTNMsgApi::UnRegister (char* sn) 
{
	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_LOGOUT_SN,NULL,NULL,NULL);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_UN_REG,message,retResult,retValue);
	return ret;
		
}
int TMTNMsgApi::GetReceiveSMS(int extFlag,MTNRecvContent* pFrcvEx,MTNRecvContentCHL* pCHLEx) 
{
	int j=1;//must be 1.
	struct timeval timeStart, timeStop;
	gettimeofday(&timeStart,0);
	int timeuse = 0;
	
	while(!m_bStop && (m_sock != INVALID_SOCKET))
	{
		gettimeofday(&timeStop,0);
		timeuse = TIMEDELTA_MS(timeStop,timeStart);
		
			
		if(timeuse > 20000)
		{
			m_nLateTimes++;
			break;
		}
		CSockTool::wait(100);
			
		if(m_bIsRecSms)
		{ 
			m_nLateTimes = 0;  
			m_bIsRecSms = false;
			
			auto_lock lock(m_MuxSmsObj);
			if(m_nSmsCount>0)
			{

				int i=0;
				for(i=0;i<m_nSmsCount;i++)
				{
					if(i>=MAX_SMS_BODY)
						break;
					
					if(0 == extFlag){//no ext
					pFrcvEx(m_revSmsBody[i].m_strSMSMobile,NULL,NULL,
							m_revSmsBody[i].m_strSMSContent,
							m_revSmsBody[i].m_strSMSTime,&j);
					}	
					else if(1 == extFlag) { //扩展函数
				    	pFrcvEx(m_revSmsBody[i].m_strSMSMobile,
				    		m_revSmsBody[i].m_strAddiSerial,
				    		m_revSmsBody[i].m_strAddSerialRecv,
							m_revSmsBody[i].m_strSMSContent,
							m_revSmsBody[i].m_strSMSTime,&j);
					}
					else if(2 == extFlag){//channel
						pCHLEx(m_revSmsBody[i].m_strSMSMobile,
							m_revSmsBody[i].m_strAddiSerial,
							m_revSmsBody[i].m_strAddSerialRecv,
							m_revSmsBody[i].m_strSMSContent,
							m_revSmsBody[i].m_strSMSTime,
							m_revSmsBody[i].m_strCHANNELNUMBER,&j);
					}
				}
				m_nSmsPos   = 0;
				m_nSmsCount = 0;
			}
			return 1;
		}//now continue read from vec until timeout.
	}
 	
	if(m_nLateTimes >= 2) 
	{
		m_bSockError = true;
		m_nLateTimes = 0;
	}
	return 1;
}
//获取客户端软件接收短信状态报告响应消息,1函数调用成功，其他失败
int TMTNMsgApi::GetReceiveReportEx(MTNGetStatusReportEx* pFstaEx)
{

	struct timeval timeStart, timeStop;
	
	int j = 1;

	//DWORD dwStart = time(NULL); //second
	gettimeofday(&timeStart, 0);  //ms
	
	int timeuse = 0;
	while(!m_bStop && (m_sock != INVALID_SOCKET))
	{
		gettimeofday(&timeStop, 0);
		timeuse = TIMEDELTA_MS(timeStop,timeStart);
		
		if(timeuse > 20000)
		{
			m_nLateTimes++;
			break;
		}
		CSockTool::wait(100);
		
		if(m_bIsRecReport)
		{ 
			m_nLateTimes = 0;  
			m_bIsRecReport = false;
			
			
			if(m_nReportCount>0)
			{		
				int i=0;
				for(i=0;i<m_nReportCount;i++)
				{
					if(i>=MAX_REPORT_BODY)
						break;

				    pFstaEx(m_revReportBody[i].m_strSequenceId,
				    		m_revReportBody[i].m_strMobile,
				        	m_revReportBody[i].m_strErrorCode,
				        	m_revReportBody[i].m_strServiceCodeAdd,
				        	m_revReportBody[i].m_strReportType,&j);
					
				}
				m_nReportPos   = 0;
				m_nReportCount = 0;
			}
			return 1;
		}//now continue read from vec until timeout.
	}
 	
	if(m_nLateTimes >= 2) 
	{
	  m_bSockError = true;
	  m_nLateTimes = 0;
	}
	return 1;

}

//接收短消息
int TMTNMsgApi::ReceiveSMS(char* sn, MTNRecvContent* rcex) 
{
	try
	{
		char message[MAX_MSG_BODY_LEN];
		memset(message,0,sizeof(message));
		setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_RECEIVE_SMS,NULL,NULL,NULL);

		int ret = SendMsgBody((char*)message,strlen(message));
		if (ret == 1) 
		{
			return GetReceiveSMS(0,rcex,NULL);
		}
		return 0;
	}	
	catch(...)
	{
		return 0;
	}
}
int TMTNMsgApi::ReceiveSMSEx(char* sn, MTNRecvContent* rcex) 
{
	try
	{
		char message[MAX_MSG_BODY_LEN];
		memset(message,0,sizeof(message));
		setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_RECEIVE_SMS,NULL,NULL,NULL);
		
		#ifdef EUCPC_DEBUG    		
			g_Log.writeLog(D1,"send recv sms : %s",message);		
		#endif	
		int ret = SendMsgBody((char*)message,strlen(message));
		if (ret == 1) 
		{
			return GetReceiveSMS(1,rcex,NULL);
		}
		return 0;
	}	
	catch(...)
	{
		return 0;
	}
}
//channel
int TMTNMsgApi::ReceiveSMSCHL(char* sn, MTNRecvContentCHL* channel) 
{
	try
	{
		char message[MAX_MSG_BODY_LEN];
		memset(message,0,sizeof(message));
		setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_RECEIVE_SMS,NULL,NULL,NULL);
		
		int ret = SendMsgBody((char*)message,strlen(message));
		if (ret == 1) 
		{
			return GetReceiveSMS(2,NULL,channel);
		}
		return 0;
	}	
	catch(...)
	{
		return 0;
	}
}
//接收短信状态报告
int TMTNMsgApi::ReceiveStatusReportEx(char* sn, MTNGetStatusReportEx* Stax)
{	
	try
	{
		char message[MAX_MSG_BODY_LEN];
		memset(message,0,sizeof(message));
		setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,ERCP_METONE_GET_STATUS_REPORT_EX,NULL,NULL,NULL);
		
		int ret = SendMsgBody((char*)message,strlen(message));

		#ifdef EUCPC_DEBUG    		
			g_Log.writeLog(D1,"send report string : %s",message);		
		#endif	

		if (ret == 1) 
		{
			ret = GetReceiveReportEx(Stax);
			return ret;
		}
		return 0;
	}	
	catch(...)
	{
		return 0;
	}	
}

int TMTNMsgApi::SendRegister_2(char* sn, 
				char* EntName,char* LinkMan,char* Phone,
				char* Mobile,char* Email,char* Fax,char* sAddress,char* Postcode) 
{		
		MetoneEntityRegistryBody entity;
		memset(&entity,0,sizeof(MetoneEntityRegistryBody));
	
		int bodyLen = entity.GetLen();
	
		entity.messageLength = htonl(bodyLen);
		entity.commandId = htonl(METONE_SECOND_REGISTRY);
		
		clock_t start; 
		start = clock(); 
		entity.sequenceNum = htonl(start);		
			
		memcpy(entity.SerialID,sn,strlen(sn));    
		memcpy(entity.ServiceTypeID,m_ServiceID,strlen(m_ServiceID));
	
		memcpy(entity.HDSerial,GetKeyVal(),strlen(GetKeyVal()));
		
		if (EntName != NULL) 
		{
			int len = strlen(EntName);
			len = len > 60 ? 60 : len;
			memcpy(entity.EName,EntName,len);
		}
		if (LinkMan != NULL)
		{
			int len = strlen(LinkMan);
			len = len > 20 ? 20 : len;
			memcpy(entity.ELinkMan,LinkMan,len);
		}
		if (Phone != NULL) 
		{
			int len = strlen(Phone);
			len = len > 20 ? 20 : len;
			memcpy(entity.EPhone,Phone,len);
		}
		if (Mobile != NULL)	
		{
			int len = strlen(Mobile);
			len = len > 15 ? 15 : len;
			memcpy(entity.EMobile,Mobile,len);
		}
		if (Email != NULL) 
		{
			int len = strlen(Email);
			len = len > 60 ? 60 : len;
			memcpy(entity.EEmail,Email,len);
		}
		if (Fax != NULL) 
		{
			int len = strlen(Fax);
			len = len > 20 ? 20 : len;
			memcpy(entity.EFax,Fax,len);
		}
		if (sAddress != NULL) 
		{
			int len = strlen(sAddress);
			len = len > 60 ? 60 : len;
			memcpy(entity.EAddress,sAddress,len);
		}
		if (Postcode != NULL) 
		{
			int len = strlen(Postcode);
			len = len > 6 ? 6 : len;
			memcpy(entity.EPostcode,Postcode,len);
		}
		
		char message[MAX_MSG_BODY_LEN];
		memset(message,0,sizeof(message));
		
		strcat(message,sn);
		strcat(message,",");
		strcat(message,"01");
		strcat(message,",");
		strcat(message,entity.HDSerial);
		strcat(message,",");
		strcat(message,entity.EName);
		strcat(message,",");
		strcat(message,entity.ELinkMan);
		strcat(message,",");
		strcat(message,entity.EPhone);
		strcat(message,",");
		strcat(message,entity.EMobile);
		strcat(message,",");
		strcat(message,entity.EEmail);
		strcat(message,",");
		strcat(message,entity.EFax);
		strcat(message,",");
		strcat(message,entity.EAddress);
		strcat(message,",");
		strcat(message,entity.EPostcode);
		
		
		char *baseenstr = base64_encode((unsigned char*)message, strlen(message));
		int baseenstr_len = strlen(baseenstr);
		char bufLen[10];
		sprintf(bufLen,"%d",baseenstr_len);

		memset(message,0,sizeof(message));
		
		strcat(message,"[A-Z]{4}\\-C\\d{3}-[A-Z|0-9|a-z|+|/|=]{");
		strcat(message,bufLen);
		strcat(message,"},");

		strcat(message,"SYNC-C006-");
		strcat(message,baseenstr);
		strcat(message,",");
		memset(bufLen,0,sizeof(bufLen));
		sprintf(bufLen,"%ld",start);
        strcat(message,bufLen);
		strcat(message,",SYNC@");
		
		free(baseenstr);
		
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_REG_2,message,retResult,retValue);
	return ret;

}

int TMTNMsgApi::GetBalance(char* sn,char* balance) 
{
	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_QUERY_BALANCE,NULL,NULL,NULL);
	
	int retResult = -1;
	int ret = sendAndGetResult(CMDID_BALANCE,message,retResult,balance);
	return ret;
}

//获得发送一条短信的价格
int TMTNMsgApi::GetPrice(char* sn,char* balance) 
{	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	setSendMsgChar(message,sn,NULL,NULL,m_ServiceID,METONE_QUERY_EACH_FEE,NULL,NULL,NULL);
	
	int retResult = -1;
	int ret = sendAndGetResult(CMDID_PRICE,message,retResult,balance);
	return ret;
}

int TMTNMsgApi::setSendMsgChar(char* message,char* sn, char*pwd,char* newpwd,
			   char*ServiceTypeId,long commID,
			   char* acco,char* pass,char* phone[])
{
	//sn msg.
	strcat(message,sn);
	strcat(message,",");
	if(METONE_REGISTRY_PWD_UPDATE == commID)
	{
		strcat(message,pwd);
	}
	else if(METONE_CHARGE_UP == commID)
	{
		strcat(message,acco);
	}
	else
	{
		strcat(message,ServiceTypeId);
	}
	
	//
	strcat(message,",");
	strcat(message,GetKeyVal());

	//
	if(METONE_FIRST_REGISTRY == commID)
	{
		strcat(message,",");
		strcat(message,pwd);
		strcat(message,",");
		strcat(message,vertemp);
	}
	else if(METONE_CHARGE_UP == commID)
	{
		strcat(message,",");
		strcat(message,pass);
	}
	else if(METONE_REGISTRY_PWD_UPDATE == commID)
	{
		strcat(message,",");
		strcat(message,newpwd);
	}
	else if(METONE_MO_FORWARD_EX == commID)
	{
		int i=0;
		for(i=0; i<10; i++)
		{
			if(phone[i] != NULL && strlen(phone[i]) != 0)
			{
				strcat(message,",");
				strcat(message,phone[i]);
			}
		}
	}

	//encode sn...
	//base64_encode() : return malloc memeory.
	char *baseenstr = base64_encode((unsigned char*)message, strlen(message));
	int baseenstr_len = strlen(baseenstr);
	char bufLen[10];
	sprintf(bufLen,"%d",baseenstr_len);
	//
	memset(message,0,strlen(message));

	if(METONE_RECEIVE_SMS == commID)
	{
		strcat(message,"[A-Z]{8}\\-C\\d{3}-[A-Z|0-9|a-z|+|/|=]{");	
	}
	else if(ERCP_METONE_GET_STATUS_REPORT_EX == commID)
	{
		strcat(message,"[A-Z]{8}\\-C\\d{3}-[A-Z|0-9|a-z|+|/|=]{");	
	}
	else
	{
		strcat(message,"[A-Z]{4}\\-C\\d{3}-[A-Z|0-9|a-z|+|/|=]{");
	}
	strcat(message,bufLen);
	strcat(message,"},");

	//judge commID
	if(METONE_FIRST_REGISTRY == commID)
	{
		strcat(message,"SYNC-C001-");
	}
	else if(METONE_CHARGE_UP == commID)
	{
		strcat(message,"SYNC-C004-");
	}
	else if(METONE_REGISTRY_PWD_UPDATE == commID)
	{
		strcat(message,"SYNC-C007-");
	}
	else if(METONE_QUERY_BALANCE == commID)
	{
		strcat(message,"SYNC-C011-");
	}
	else if(METONE_QUERY_EACH_FEE == commID)
	{
		strcat(message,"SYNC-C012-");
	}
	else if(METONE_LOGOUT_SN == commID)
	{
		strcat(message,"SYNC-C005-");
	}
	else if(METONE_MO_FORWARD_EX == commID)
	{
		strcat(message,"SYNC-C010-");
	}
	else if(METONE_CANCEL_MO_FORWARD == commID)
	{
		strcat(message,"SYNC-C008-");
	}
	else if(METONE_RECEIVE_SMS == commID)
	{
		strcat(message,"MOANDRPT-C013-");
	}
	else if(ERCP_METONE_GET_STATUS_REPORT_EX == commID)
	{
		strcat(message,"MOANDRPT-C014-");
	}
	else
	{
		strcat(message,"SYNC-QCG?-");
	}
	//
	strcat(message,baseenstr);
	strcat(message,",");

	clock_t start; 
	start = clock(); 
	memset(bufLen,0,sizeof(bufLen));
	sprintf(bufLen,"%ld",start);

	strcat(message,bufLen);
	strcat(message,",SYNC@");

	free(baseenstr);

	return 0;
}
char* TMTNMsgApi::GetKeyVal()
{
	int nLen = strlen(m_KeyVal);
	if(nLen != 32)
		return NULL;
	else
		return m_KeyVal;
}

//验证手机号码,号码都必须是数字
//byz modify 20050505 添加小灵通号码解析
int TMTNMsgApi::ValidateMobile(const char* mobile) {
	if (mobile == NULL || strlen(mobile) <= 0) return 0;
	
 	int len = strlen(mobile);
	
	if (len>MOBILE_LONG_LEN) return 1;
	if (ValidateDigit(mobile)) return 1;
	return 0;
}
//过滤手机号码
int TMTNMsgApi::FilterMobile(char* dest,const char* srcmobile,int* count) 
{
	if (dest == NULL || srcmobile == NULL) return 0;
	int ret = 0;
	bool flag = true;
	int i = 0;
	char* pos = (char*)strchr(srcmobile,',');
	if (pos == NULL) 
	{
		//效验手机号码
		if (!ValidateMobile(srcmobile)) return 0;
		int len = strlen(srcmobile);
		memcpy(dest,srcmobile,len);	
		dest[len] = '\0';
		i += 1;
		*count = i;
		return 1;
	}else 
	{
		char* tempPhone ;		
		tempPhone = (char*)malloc(21);
		while (pos != NULL)	
		{			
			memset(tempPhone,0,21);
			memcpy(tempPhone,srcmobile,pos-srcmobile);
			//效验手机号码
			if(ValidateMobile(tempPhone))
			{
				if (i == 0) 
				{
					int len = strlen(tempPhone);
					memcpy(dest,tempPhone,len);
					dest[len] = '\0';
					strcat(dest,".");
				}
				else 
				{						
					strcat(dest,tempPhone);
					strcat(dest,".");
				}
				ret = 1;
				i += 1;
			}
			else 
			{
				flag = false;				
			}
			srcmobile = pos + 1;
			pos = (char*)strchr(srcmobile,',');		
		}
		//最后一个号码
		if (pos == NULL && srcmobile != NULL && strlen(srcmobile) > 0) 
		{
			memset(tempPhone,0,21);
			memcpy(tempPhone, srcmobile, strlen(srcmobile));
			//效验手机号码
			if (!ValidateMobile(tempPhone)) 
			{
				flag = false;	
			}
			else 
			{
				strcat(dest,tempPhone);
				i += 1;
			}
		}
		if (dest[strlen(dest) - 1] == '.') 
		{
			dest[strlen(dest) - 1] = '\0';
		}
		*count = i;
		free(tempPhone);
		tempPhone = NULL;  //byz modify 20050524
	}	
	if (!flag) return MOBILEPART_ERROR;
	else return ret;
}

//发送短消息
int TMTNMsgApi::SendSMS(char* sn,char* mn,char* ct,int count,int mobilecount,char* priority) 
{	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	
	setSendSMSMsgChar(message,sn,METONE_SEND_SMS,mn,ct,NULL,NULL,count,mobilecount,0,priority,reconnBuf);

	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SMS,message,retResult,retValue,reconnBuf);
	
#ifdef EUCPC_DEBUG    		
	if(ret != 1 )
	{
		g_Log.writeLog(D1,"sendAndGetResult ret == [%d], ct = [%s],reconnStr = [%s]",ret,ct,reconnBuf);
	}
#endif	
	
	
	return ret;
}
//发送短消息
int TMTNMsgApi::SendSMS2(char* sn,char* mn,char* ct,int count,int mobilecount, char* seqnum,char* priority)
{
	
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	setSendSMSMsgChar(message,sn,METONE_SEND_SMS_2,mn,ct,NULL,NULL,count,mobilecount,seqnum,priority,reconnBuf);

	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SMS,message,retResult,retValue,reconnBuf);
	return ret;
}


//发送短消息到EUCP平台,可以带附加号码
int TMTNMsgApi::SendSMSEx(char* sn,char* mn,char* ct,char* addi,int count,int mobilecount,char* priority)
{
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	
	setSendSMSMsgChar(message,sn,METONE_SEND_SMS_EX,mn,ct,NULL,addi,count,mobilecount,NULL,priority,reconnBuf);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SMS,message,retResult,retValue,reconnBuf);
	return ret;
}
int TMTNMsgApi::SendSMSEx2(char* sn,char* mn,char* ct,char* addi,int count,int mobilecount, char* seqnum,char* priority)
{
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	
	setSendSMSMsgChar(message,sn,METONE_SEND_SMS_EX_2,mn,ct,NULL,addi,count,mobilecount,seqnum,priority,reconnBuf);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SMS,message,retResult,retValue,reconnBuf);
	return ret;
}


//发送定时短信
int TMTNMsgApi::SendScheSMS(char* sn,char* mn,char* ct,char* sendtime,int count,int mobilecount,char* priority) 
{
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	
	setSendSMSMsgChar(message,sn,METONE_SEND_SCHEDULED_SMS,mn,ct,sendtime,NULL,count,mobilecount,NULL,priority,reconnBuf);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SCHE_SMS,message,retResult,retValue,reconnBuf);
	return ret;
}
//发送短消息
int TMTNMsgApi::SendScheSMS2(char* sn,char* mn,char* ct,char* sendtime,int count,int mobilecount, char* seqnum,char* priority)
{
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	
	setSendSMSMsgChar(message,sn,METONE_SEND_SCHEDULED_SMS_2,mn,ct,sendtime,NULL,count,mobilecount,seqnum,priority,reconnBuf);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SCHE_SMS,message,retResult,retValue,reconnBuf);
	return ret;
}


//发送短消息到EUCP平台,可以带附加号码
int TMTNMsgApi::SendScheSMSEx(char* sn,char* mn,char* ct,char* sendtime,char* addi,int count,int mobilecount,char* priority)
{
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	
	setSendSMSMsgChar(message,sn,METONE_SEND_SCHEDULED_SMS_EX,mn,ct,sendtime,addi,count,mobilecount,0,priority,reconnBuf);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SCHE_SMS,message,retResult,retValue,reconnBuf);
	return ret;
}
int TMTNMsgApi::SendScheSMSEx2(char* sn,char* mn,char* ct,char* sendtime,char* addi,int count,int mobilecount, char* seqnum,char* priority)
{
	char message[MAX_MSG_BODY_LEN];
	memset(message,0,sizeof(message));
	char reconnBuf[1024];
	memset(reconnBuf,0,sizeof(reconnBuf));
	
	setSendSMSMsgChar(message,sn,METONE_SEND_SCHEDULED_SMS_EX_2,mn,ct,sendtime,addi,count,mobilecount,seqnum,priority,reconnBuf);
	
	int retResult = -1;
	char retValue[100];
	int ret = sendAndGetResult(CMDID_SEND_SCHE_SMS,message,retResult,retValue,reconnBuf);
	return ret;
}

int TMTNMsgApi::GetCurTime(char* strtime)
{
    struct timeb  tp;
    struct tm     ltime;
    ftime(&tp);
    localtime_r(&tp.time,&ltime);
    sprintf(strtime,"%04d%02d%02d%02d%02d%02d%03d",
    ltime.tm_year+1900,
    ltime.tm_mon+1,
    ltime.tm_mday,
    ltime.tm_hour,
    ltime.tm_min,
    ltime.tm_sec,
    tp.millitm);
    	
	return 0;
}

int TMTNMsgApi::GetAndSetCurSendSmsReconnStr(char* sn,char*timestr,char*deststr)
{

	char tmp_sn_and_id[1024];
	memset(tmp_sn_and_id,0,sizeof(tmp_sn_and_id));
	strcat(tmp_sn_and_id,sn);
	strcat(tmp_sn_and_id,",");
	strcat(tmp_sn_and_id,timestr);

	char *sdk_num_64 = base64_encode((unsigned char*)tmp_sn_and_id, strlen(tmp_sn_and_id));
	
	int sdk_num_64_len = strlen(sdk_num_64);
	char num_len[20];
	sprintf(num_len,"%d",sdk_num_64_len);


	strcat(deststr,"[A-Z]{9}\\-C\\d{3}-[A-Z|0-9|a-z|+|/|=]{");
	strcat(deststr,num_len);
	strcat(deststr,"},AUTOMATON-C016-");
	strcat(deststr,sdk_num_64);
	strcat(deststr,",");
	strcat(deststr,timestr);
	strcat(deststr,",AUTOMATON@");
	
	free(sdk_num_64);

	return 0;
}
int TMTNMsgApi::setSendSMSMsgChar(char* message,char* sn, long commID,
				char*mn,char* ct,char* sendtime,char* addi,
				int count,int mobilecount,char* seqnum,
				char* priority,char* destStr)
{
    MetoneSendSMSBody sms;
	memset(&sms,0,sms.GetLen(count));	

	if(ValidateDigitPriority(priority)) 
	  memcpy(sms.SMSPriority,priority,strlen(priority)); 
	else
	{
	  const char* pri = "5";
	  memcpy(sms.SMSPriority,pri,1); 
	}
	memcpy(sms.HDSerial,GetKeyVal(),strlen(GetKeyVal()));
	memcpy(sms.SMSContent,ct,strlen(ct)); 
	
	clock_t start; 
	start = clock(); 
	
	char seqBuf[1024];
	memset(seqBuf,0,sizeof(seqBuf));
	
	if(METONE_SEND_SMS == commID)//1
		sprintf(seqBuf,"%ld",start);
	else if(METONE_SEND_SMS_2 == commID)//2
		sprintf(seqBuf,"%s",seqnum);
	else if(METONE_SEND_SMS_EX == commID)//3
		sprintf(seqBuf,"%ld",start);
	else if(METONE_SEND_SMS_EX_2 == commID)//4
		sprintf(seqBuf,"%s",seqnum);
	else if(METONE_SEND_SCHEDULED_SMS == commID)//5
		sprintf(seqBuf,"%ld",start);
	else if(METONE_SEND_SCHEDULED_SMS_2 == commID)//6
		sprintf(seqBuf,"%s",seqnum);
	else if(METONE_SEND_SCHEDULED_SMS_EX == commID)//7
		sprintf(seqBuf,"%ld",start);
	else if(METONE_SEND_SCHEDULED_SMS_EX_2 == commID)//8
		sprintf(seqBuf,"%s",seqnum);
	
	//目前不用GetCurTime()函数生成
	//GetCurTime(sms.sequenceNum1);
	strcat(sms.sequenceNum1,seqBuf);

	if(METONE_SEND_SMS == commID)//1
	{
		;//do nothing
	}
	else if(METONE_SEND_SMS_2 == commID)//2
	{
		;//do nothing
	}
	else if(METONE_SEND_SMS_EX == commID)//3
	{
		if (addi != NULL)
			memcpy(sms.AddiSerial,addi,strlen(addi)); 
	}
	else if(METONE_SEND_SMS_EX_2 == commID)//4
	{
		if (addi != NULL)
			memcpy(sms.AddiSerial,addi,strlen(addi)); 
	}
	else if(METONE_SEND_SCHEDULED_SMS == commID)//5
	{
		;//do nothing
	}
	else if(METONE_SEND_SCHEDULED_SMS_2 == commID)//6
	{
		;//do nothing
	}
	else if(METONE_SEND_SCHEDULED_SMS_EX == commID)//7
	{
		if (addi != NULL)
			memcpy(sms.AddiSerial,addi,strlen(addi)); 
	}
	else if(METONE_SEND_SCHEDULED_SMS_EX_2 == commID)//8
	{
		if (addi != NULL)
			memcpy(sms.AddiSerial,addi,strlen(addi)); 
	}
		
	
	strcat(message,sn);
	strcat(message,",");


	if(METONE_SEND_SMS == commID
		|| METONE_SEND_SMS_2 == commID
		|| METONE_SEND_SMS_EX == commID
		|| METONE_SEND_SMS_EX_2 == commID) //SendSMS
	{
		strcat(message,sms.SMSPriority);	
	}
	else if(METONE_SEND_SCHEDULED_SMS == commID
		|| METONE_SEND_SCHEDULED_SMS_2 == commID
		|| METONE_SEND_SCHEDULED_SMS_EX == commID
		|| METONE_SEND_SCHEDULED_SMS_EX_2 == commID) //SendScheSMS
	{
		strcat(message,sendtime);
	}
	
	strcat(message,",");
	strcat(message,sms.HDSerial);
	strcat(message,",");
	strcat(message,sms.sequenceNum1);
	strcat(message,",");
	strcat(message,sms.AddiSerial);
	strcat(message,",");
	strcat(message,"GBK");
	strcat(message,",");
	strcat(message,mn);
	strcat(message,",");
	strcat(message,sms.SMSContent);
		
	char *baseenstr = base64_encode((unsigned char*)message, strlen(message));
	int baseenstr_len = strlen(baseenstr);
	char bufLen[10];
	sprintf(bufLen,"%d",baseenstr_len);

	memset(message,0,strlen(message));
	strcat(message,"[A-Z]{4}\\-C\\d{3}-[A-Z|0-9|a-z|+|/|=]{");

	strcat(message,bufLen);
	strcat(message,"},");

	if(METONE_SEND_SMS == commID
		|| METONE_SEND_SMS_2 == commID
		|| METONE_SEND_SMS_EX == commID
		|| METONE_SEND_SMS_EX_2 == commID) //SendSMS
	{
		strcat(message,"SYNC-C002-");
		strcat(message,baseenstr);
		strcat(message,",");	
	  strcat(message,sms.sequenceNum1);
		strcat(message,",SYNC@");
	}
	else if(METONE_SEND_SCHEDULED_SMS == commID
		|| METONE_SEND_SCHEDULED_SMS_2 == commID
		|| METONE_SEND_SCHEDULED_SMS_EX == commID
		|| METONE_SEND_SCHEDULED_SMS_EX_2 == commID) //SendScheSMS
	{
		strcat(message,"SYNC-C003-");
		strcat(message,baseenstr);
		strcat(message,",");
        strcat(message,sms.sequenceNum1);
		strcat(message,",SYNC@");
	}
	
	free(baseenstr);
	
	//
	GetAndSetCurSendSmsReconnStr(sn,sms.sequenceNum1,destStr);
	//

	return 0;
}

int TMTNMsgApi::addRecvSmsBody(char* src_cnt)
{
	char tmpStr[MAX_MSG_BODY_LEN];
	memset(tmpStr,0,sizeof(tmpStr));
	int len = strlen(src_cnt);
	int i=0;
	int j=0;
	for(i=0;i<len;i++)   
	{   
		if((src_cnt[i]==0x0D)||(src_cnt[i]==0x0A))   
			continue;   
		tmpStr[j++]=src_cnt[i];   
	}   
	unsigned char* pCnt = NULL;

	j=0;
	len = strlen(tmpStr);
	pCnt = base64_decode(tmpStr,&len);	
	len = strlen((const char*)pCnt);
	
	memset(tmpStr,0,sizeof(tmpStr));
	for(i=0;i<len;i++)   
	{   
		if((pCnt[i]==0x0D)||(pCnt[i]==0x0A))   
			continue;   
		tmpStr[j++]=pCnt[i];   
	}  
	len = strlen(tmpStr);

	char* atPos = strstr(tmpStr,";");
	char* atPos2 = NULL;
	char* atPos3 = NULL;
	
	//while(pos != -1) 
	while(NULL != atPos) 
	{
		
		RecvSMSBody smsbody;
		char tmpSubcnt[MAX_MSG_BODY_LEN];
		memset(tmpSubcnt,0,sizeof(tmpSubcnt));
		len = atPos - tmpStr;
		memcpy(tmpSubcnt,tmpStr,len);
		//cout <<tmpSubcnt << endl;
		
		char*tmpPos = strstr(tmpSubcnt,".");
		atPos2 = strstr((tmpPos+1),".");
		atPos3 = strstr((atPos2+1),".");
		
		free(pCnt);
		pCnt = NULL;
	
		
		char smsStr[MAX_MSG_BODY_LEN];
    	memset(smsStr,0,sizeof(smsStr));
    	len = atPos3-atPos2-1;
    	memcpy(smsStr,(atPos2+1),len);
    	//cout << smsStr<<endl;
    	
    	len = strlen(smsStr);
    	pCnt = base64_decode(smsStr,&len);
    	
    	
    	
    	len = strlen((char*)pCnt);
    	memcpy(smsbody.m_strSMSContent,pCnt,len);
    	
		
		atPos2 = strstr((atPos3+1),".");
		memcpy(smsbody.m_strSMSTime,(atPos3+1),atPos2-atPos3-1);
			
		
		atPos2 = strstr(atPos2+1,".");
		atPos3 = strstr(atPos2+1,".");
		memcpy(smsbody.m_strAddSerialRecv,(atPos2+1),(atPos3-atPos2-1));
		
		
		atPos2 = strstr(atPos3+1,".");
		atPos3 = strstr(atPos2+1,".");
		memcpy(&smsbody.m_strAddiSerial,(atPos2+1),(atPos3-atPos2-1));
		
		//mobile
		
		atPos2 = strstr(atPos2+1,".");
		atPos3 = strstr(atPos2+1,".");
		memcpy(smsbody.m_strSMSMobile,(atPos2+1),(atPos3-atPos2-1));
		
		//channel.	
		
		atPos2 = strstr((atPos3+1),".");
		memcpy(&smsbody.m_strCHANNELNUMBER,(atPos3+1),(atPos2-atPos3-1));


		int needSet = (atPos-tmpStr+1);
		int leftLen = strlen(tmpStr) - (atPos-tmpStr+1);
		
		
		memmove(tmpStr,(atPos+1),leftLen);
		memset(tmpStr+leftLen,0,needSet); 

		atPos = strstr(tmpStr,";");
		if(1)
		{
			auto_lock lock(m_MuxSmsObj);
			m_revSmsBody[m_nSmsPos] = smsbody;
			m_nSmsPos++;
			m_nSmsCount++;
			if(m_nSmsPos > MAX_SMS_BODY) 
			{
				m_nSmsPos = 0;
				m_nSmsCount = 0;
			}
		}
	}
    
    m_bIsRecSms = true;
	free(pCnt);

	return 1;
}	

int TMTNMsgApi::setBalanceFromSms(char* src_cnt)
{
	if(NULL == src_cnt)
		return -1;
	
	char tmpStr[1024];
	memset(tmpStr,0,sizeof(tmpStr));
	int len = strlen(src_cnt);
	int i=0;
	int j=0;
	for(i=0;i<len;i++)   
	{   
		if((src_cnt[i]==0x0D)||(src_cnt[i]==0x0A))   
			continue;   
		tmpStr[j++]=src_cnt[i];   
	}   
	unsigned char* pCnt = NULL;

	j=0;
	len = strlen(tmpStr);
	pCnt = base64_decode(tmpStr,&len);	
	len = strlen((const char*)pCnt);
	
	memset(tmpStr,0,sizeof(tmpStr));
	for(i=0;i<len;i++)   
	{   
		if((pCnt[i]==0x0D)||(pCnt[i]==0x0A))   
			continue;   
		tmpStr[j++]=pCnt[i];   
	}  
	len = strlen(tmpStr);

	char SN[256] = {0};
	char Balance[256] = {0};
	char* atPos = strstr(tmpStr,",");
	if(NULL != atPos)
	{
		len = atPos - tmpStr;
		strncpy(SN,tmpStr,len);
		atPos++;
		len = strlen((char*)atPos);
		strncpy(Balance,atPos,len);
		if(1)
		{
			auto_lock lock(m_MuxSmsObj);
			setSnAndBalanceToArray(SN,Balance);
		}
	}

	free(pCnt);
	return 1;
}
int TMTNMsgApi::setSnAndBalanceToArray(char*SN,char* Balance)
{
	if(NULL == SN || NULL == Balance)
		return 0;
	
	int len = 0;	
	int i=0;
	for(i=0;i<MAX_SN_COUNT;i++)
	{
		len = strlen(SN);
		if(0 == strncmp((char*)m_arrSN[i],SN,len))
		{
			//
			 float cur = atof(Balance);
			 float old = atof((char*)m_arrBalance[i]);
			 if((cur - old ) <= 0.1) // cur < old
			 {
				len = strlen(Balance);
				strcpy((char*)m_arrBalance[i],Balance);
				gettimeofday(&m_arrLastBalanceTime[i],0);
				return 1;
			 }
			 else if((cur - old ) > 0.1) // cur > old
			 {
			 	if((cur - old) >= 5.0)
			 	{
					len = strlen(Balance);
					strcpy((char*)m_arrBalance[i],Balance);
					gettimeofday(&m_arrLastBalanceTime[i],0);
					return 1;
			 	}
			 	else
			 	{
			 		return 1;
			 	}
			 }
		}
	}
	
	m_nCurSnPos++;
	if(m_nCurSnPos >= MAX_SN_COUNT)
		m_nCurSnPos = 0;
	if(1)
	{
		len = strlen(SN);
		strcpy((char*)m_arrSN[m_nCurSnPos],SN);
		strcpy((char*)m_arrBalance[m_nCurSnPos],Balance);

		gettimeofday(&m_arrLastBalanceTime[m_nCurSnPos],0);
		return 1;
	}
	
	return 0;
}
int TMTNMsgApi::getSnAndBalanceFromArray(char*SN,char* Balance,struct timeval* LastTimeVal )
{
	if(NULL == SN)
		return 0;

	int len = 0;	
	int i=0;
	for(i=0;i<MAX_SN_COUNT;i++)
	{
		len = strlen(SN);
		if(0 == strncmp((char*)m_arrSN[i],SN,len))
		{
			len = strlen((char*)m_arrBalance[i]);
			strcpy(Balance,(char*)m_arrBalance[i]);

			LastTimeVal->tv_sec = m_arrLastBalanceTime[i].tv_sec;
			LastTimeVal->tv_usec = m_arrLastBalanceTime[i].tv_usec;
			return 1;
		}
	}
	return 0;
}

int TMTNMsgApi::addRecvReportBody(char* src_cnt)
{
	
	int i = 0;
	int j = 0;
	
	string cntexpress;
	string subcnt;

	int len = strlen(src_cnt);
    char* PSrcCnt = new char[len+1];
	memset(PSrcCnt,0,len+1);
	
	unsigned char* pCnt = NULL;
	
	 
	for(int i=0;i<len;i++)   
	{   
		if   ((src_cnt[i]==0x0D)||(src_cnt[i]==0x0A))   
			continue;   
		PSrcCnt[j++]=src_cnt[i];   
	}   
    cntexpress.append(PSrcCnt);
    j=0;   

    memset(PSrcCnt,0,len+1);
    
    len = cntexpress.length();
	pCnt = base64_decode(cntexpress.c_str(),&len);	
	len = strlen((const char*)pCnt);
	for(i=0;i<len;i++)   
	{   
		if((pCnt[i]==0x0D)||(pCnt[i]==0x0A))   
			continue;   
		PSrcCnt[j++]=pCnt[i];   
	}  

	cntexpress.clear();
	cntexpress = PSrcCnt;

	//unsigned int reporttype = 0;
  cntexpress = cntexpress.substr(cntexpress.find_last_of(",")+1,cntexpress.length());
	int pos = cntexpress.find(";");
	
	//int pos = cntexpress.find(";");
	//cntexpress = cntexpress.substr(cntexpress.find_last_of(",")+1,pos-1);
	int subpos,subposL;
	
	while(pos != -1) 
	{
		RecvReportBody reportBody;
		
		subcnt.clear();
		subcnt.append(cntexpress.substr(0,pos));
		subpos = subcnt.find(".",0);
		memcpy(reportBody.m_strErrorCode,subcnt.substr(0,subpos).c_str(),subpos);        
		subpos = subcnt.find(".",subpos+1); 
		subposL = subcnt.find(".",subpos+1);		
		memcpy(reportBody.m_strMemo,subcnt.substr(subpos+1,subposL-subpos-1).c_str(),subposL-subpos-1);

        subpos = subcnt.find(".",subposL+1);
		memcpy(reportBody.m_strRecieverdate,subcnt.substr(subposL+1,subpos-subposL-1).c_str(),subpos-subposL-1);

		subposL = subcnt.find(".",subpos+1);
		memcpy(reportBody.m_strReportType,subcnt.substr(subpos+1,subposL-subpos-1).c_str(),subposL-subpos-1);
		
		subpos = subcnt.find(".",subpos+1);
		subposL = subcnt.find(".",subpos+1);
	    memcpy(reportBody.m_strSequenceId,subcnt.substr(subpos+1,subposL-subpos-1).c_str(),subposL-subpos-1);
        
		subpos = subcnt.find(".",subpos+1);
		subposL = subcnt.find(".",subpos+1);
		memcpy(reportBody.m_strServiceCodeAdd,subcnt.substr(subpos+1,subposL-subpos-1).c_str(),subposL-subpos-1);

		subpos = subcnt.find(".",subpos+1);
		subposL = subcnt.find(".",subpos+1);
		memcpy(reportBody.m_strSubmitdate,subcnt.substr(subpos+1,subposL-subpos-1).c_str(),subposL-subpos-1);

		subpos = subcnt.find(".",subposL+1);
		memcpy(reportBody.m_strMobile,subcnt.substr(subposL+1,subpos-subposL-1).c_str(),subpos-subposL-1);

		cntexpress = cntexpress.substr(pos+1, cntexpress.length());
		pos = cntexpress.find(";");	
		//
		m_revReportBody[m_nReportPos] = reportBody;
		m_nReportPos++;
		m_nReportCount++;
		if(m_nReportPos > MAX_REPORT_BODY) 
		{
			m_nReportPos = 0;
			m_nReportCount = 0;
		}
	}
  m_bIsRecReport = true;
	free(pCnt);
  delete []PSrcCnt;
	return 1;
}

//成功1
int TMTNMsgApi::SendMsgHead1(const int commID,const int SMSCount, int seq) 
{
	const char* message = "[A-Z]{4}\\-C\\d{3}-[A-Z]{10},ACTI-C015-ACTIVETEST,99,ACTI@";
	if (SendMsgBody((char*)message,strlen(message))) {		
		return 1;
	}
	return 0;	 
}
//成功1
int TMTNMsgApi::SendMsgHead2(const int commID,const int SMSCount, int seq) 
{
	const char* message = "[A-Z]{9}\\-C\\d{3}-[A-Z]{20},CLIENTACT-C015-RESPONSESERVERACTIVE,98,SACTI@";
	if (SendMsgBody((char*)message,strlen(message))) 
	{		
		return 1;
	}
	return 0;	 
}
//成功1
int TMTNMsgApi::SendCloseHead1(const int commID,const int SMSCount, int seq) 
{
	const char *message = "[A-Z]{4}\\-C\\d{3}-[A-Z]{16},KILL-C016-KILLREMOTESOCKET,89,KILL@";
	if (SendMsgBody((char*)message,strlen(message))) {		
		return 1;
	}
	return 0;	 
}

int TMTNMsgApi::ActiveSocketSCFun()
{
	m_nSCFunThreadNum++;
	
	pthread_mutex_lock(&m_sc_mutex);
	while(!m_bStop && (m_sock != INVALID_SOCKET))
	{
		//
		struct timeval now;
		struct timespec timeout;
		gettimeofday(&now,0);
		timeout.tv_sec = now.tv_sec +  1;//every 1s,check
		timeout.tv_nsec = now.tv_usec * 1000; //
		
		int ret = pthread_cond_timedwait(&m_sc_cond,&m_sc_mutex,&timeout);
		switch(ret)
		{
		case 0: //recv cond signal. now it is not use.
#ifdef EUCPC_DEBUG    		
			g_Log.writeLog(D1,"ActiveSocketSCFun() recv pthread_cond_timedwait() signal! now stop.");
#endif			
			m_bStop = true; 
			break;
		case ETIMEDOUT: //timeout break switch while continue;
			m_bStop = false;
			break;
		default:   //like unix signal, break switch while continue;    
			break;
		}
		
		if(m_bOnWork)
		{
		  m_bOnWork = false;
		  m_nActiveTimes = 0;
		  continue;
		}
		m_nActiveTimes++;
		if(m_nActiveTimes == 30 || 
			m_nActiveTimes == 240 )
		//if(m_nActiveTimes == 12 || 
		//	m_nActiveTimes == 24 )			
		{
#ifdef EUCPC_DEBUG    					
			g_Log.writeLog(D1,"ActiveSocketSCFun() m_nActiveTimes = [%d],SendMsgHead1()",m_nActiveTimes);			
#endif
			if(m_sock != INVALID_SOCKET)
				SendMsgHead1(1234,1234,1234);
			else
				m_bStop = true;
		}
		
		if(m_nActiveTimes >= 3*120)
		//if(m_nActiveTimes >= 3*12)
		{
			if(m_sock != INVALID_SOCKET)
			{
				SendCloseHead1(1234,1234,1234);
			}
			if(m_sock != INVALID_SOCKET)
			{
				::shutdown(m_sock,SHUT_RDWR);
				::close(m_sock);
				m_sock = INVALID_SOCKET;
			}	 
			m_nActiveTimes = 0;
			
			m_bSockError = true;
			
			m_bStop = true;
#ifdef EUCPC_DEBUG    		
			g_Log.writeLog(D1,"ActiveSocketSCFun() m_nActiveTimes >= 360, now stop.");
#endif			
			
		}
	}
	
#ifdef EUCPC_DEBUG    		
			g_Log.writeLog(D1,"ActiveSocketSCFun() m_bStop = true, now stop.");
#endif	
	
	m_sock = INVALID_SOCKET;
	m_nActiveTimes = 0;
	m_bSockError = true;
	m_bStop = true;

	//
	pthread_mutex_unlock(&m_sc_mutex);
	
	m_SCThread.m_dwThreadId = 0;
	return 1;
}



int TMTNMsgApi::sendsms_reconn(int& srcRet,char* reconnStr) 
{
	try
	{
		srcRet = 998;//超时
		
#ifdef EUCPC_DEBUG
		DWORD tid = pthread_self();
		ostringstream ostr;
		ostr << "timeout_" << tid << ".txt";

		ofstream of(ostr.str().c_str(),ios::app);
		of << "timeout  begin======================="<< endl;
		cout << "timeout  begin======================="<< endl;
		
		of << "reconnStr = [" << reconnStr << "]"<< endl;
		cout << "reconnStr = [" << reconnStr << "]"<< endl;
		
#endif
		int reconn_num = TMTNMsgApi::m_reconnect_num;
		
		while(reconn_num > 0)
		{
#ifdef EUCPC_DEBUG
			of << "reconn_num=" << reconn_num << endl;
			cout << "reconn_num=" << reconn_num << endl;
#endif
			//first wait thread terminal and next conn.
			//sleep(TMTNMsgApi::m_reconnect_interval);
			usleep(TMTNMsgApi::m_reconnect_interval * 1000 * 1000);
			//now reconnect;
			int tmpsock = CSockTool::connect(m_ip, m_port, 1000);

			if( tmpsock < 0 )
			{
#ifdef EUCPC_DEBUG
				of << "connect() ret=" << tmpsock << endl;
				cout << "connect() ret=" << tmpsock << endl;
#endif
				reconn_num--;
				continue;
			} 
			else 
			{	
				//now send and recv;
				unsigned int len = strlen(reconnStr);
				//int status = send(tmpsock,reconnStr,len,0);
				int status = CSockTool::write(tmpsock, reconnStr, len, 1000 );
				if(status == SOCKET_ERROR)
				{
#ifdef EUCPC_DEBUG							
					of << "send() ret=" << status << endl;
					cout << "send() ret=" << status << endl;
#endif

					::close(tmpsock);
					reconn_num--;
					continue;
				}
				else
				{
#ifdef EUCPC_DEBUG
					of << "recv() now send=" << status << endl;
					cout << "recv() now send=" << status << endl;
#endif
					char recvbuf[4096];
					memset(recvbuf,0,4096);
					//status = recv(tmpsock,recvbuf,4096,0);
					status = CSockTool::read(tmpsock,recvbuf, 4096, 1000 );
					if (status == SOCKET_ERROR)
					{
#ifdef EUCPC_DEBUG
						of << "recv() ret=" << status << endl;
						cout << "recv() ret=" << status << endl;
#endif

						::close(tmpsock);
						reconn_num--;
						continue;
					}
					else if(status == 0)
					{
#ifdef EUCPC_DEBUG
						of << "recv() ret= "<< status <<endl;
						cout << "recv() ret= "<< status <<endl;
						
						of << "before send content--------- " << endl;
						of << "send buf=" << reconnStr << endl;
						cout << "send buf=" << reconnStr << endl;
#endif

						::close(tmpsock);
						reconn_num--;
						continue;
					}
					else
					{
#ifdef EUCPC_DEBUG
						of << "recvbuf =[" << recvbuf << "]"<< endl;
						cout << "recvbuf = [" << recvbuf <<  "]"<< endl; 
#endif
						
						char* pos = strstr(recvbuf,",");
						if(pos == NULL)
						{
#ifdef EUCPC_DEBUG
							of << "strstr ret=" << "NULL" << endl;
							of << "recvbuf = " << recvbuf << endl; 
							cout << "strstr ret=" << "NULL" << endl;
#endif
							::close(tmpsock);
							reconn_num--;
							continue;
						}
						int result = atoi(recvbuf);
#ifdef EUCPC_DEBUG
	
						of << "atoi ret=" << result << endl;
						cout << "atoi ret=" << result << endl;
#endif

						::close(tmpsock);

						if(result == 0)
						{
#ifdef EUCPC_DEBUG
							of << "timeout  recv and  return ="<< result <<endl;
							cout << "timeout  recv and  return ="<< result <<endl;
#endif
							srcRet = result;	
							return 1;
						}
						else if(result == 997)
						{
#ifdef EUCPC_DEBUG
							of << "timeout  recv and  return ="<< result <<endl;
							cout << "timeout  recv and  return ="<< result <<endl;
							of << "continue recv" << endl;
							cout << "continue recv" << endl;
#endif
							reconn_num--;
							continue;
						}
						else
						{
#ifdef EUCPC_DEBUG
							of << "timeout  recv and  return ="<< result <<endl;
							cout << "timeout  recv and  return ="<< result <<endl;
#endif
							srcRet = result;	
							return 1;
						}
					}
				}

			}
		}
#ifdef EUCPC_DEBUG
		of << "timeout  end======================="<< endl;
		cout << "timeout  end======================="<< endl;
#endif

	}
	catch (...)
	{
	}
	return 1;
}


int TMTNMsgApi::ActiveSocketFun()
{
	memset(m_strTemp,0,sizeof(m_strTemp));
	m_nPos = 0;
	m_nSmsPos   = 0;
	m_nSmsCount = 0;
	m_nReportPos = 0;
	m_nReportCount = 0;

	m_nSockFunThreadNum++;
	
#ifdef EUCPC_DEBUG    						
		g_Log.writeLog(D1,"API thread num=%d", m_nSockFunThreadNum);
#endif	

	while (!m_bStop && (m_sock != INVALID_SOCKET))
	{
		//定义读事件集合
		fd_set fdRead;
		int ret;
		//定义事件等待时间
		struct timeval	aTime;
		aTime.tv_sec = 1;
		aTime.tv_usec = 0;
		
		//置空fdRead事件为空
		FD_ZERO(&fdRead);
		//给客户端socket设置读事件
		FD_SET(m_sock,&fdRead);
		//调用select函数，判断是否有读事件发生
		ret = select(m_sock+1,&fdRead,NULL,NULL,&aTime);
		//ret = select(m_sock+1,&fdRead,NULL,NULL,NULL);
#ifdef EUCPC_DEBUG    		
		//g_Log.writeLog(D1,"select ret == %d !",ret);			
#endif	

		if (ret == SOCKET_ERROR)
		{
#ifdef EUCPC_DEBUG    		
			//g_Log.writeLog(D1,"select ret == socket error !");			
#endif		
			m_sock = INVALID_SOCKET;
			m_nActiveTimes = 0;					
			m_bSockError = true;			
			m_bStop = true;
			break;
		}
		else if(ret == 0)
		{
			continue;
		}
		else if (ret > 0)
		{
			
			if(m_sock == INVALID_SOCKET)
			{
#ifdef EUCPC_DEBUG    		
			//g_Log.writeLog(D1,"select ret >0  m_sock == INVALID_SOCKET!");			
#endif	
				
				m_nActiveTimes = 0;					
				m_bSockError = true;			
				m_bStop = true;
				break;
			}
			//发生读事件
			if (FD_ISSET(m_sock,&fdRead))
			{
#ifdef EUCPC_DEBUG 
			//g_Log.writeLog(D1,"NOW in FD_ISSET()!");				
#endif				
				ret = on_read();
				if(ret != 1)//error or socket close().
				{
					
#ifdef EUCPC_DEBUG 
			g_Log.writeLog(D1,"on_read() error!");				
#endif				
				
					
					::shutdown(m_sock,SHUT_RDWR);
					::close(m_sock);
					m_sock = INVALID_SOCKET;
					
					m_nActiveTimes = 0;					
					m_bSockError = true;			
					m_bStop = true;
				}
			}
		}
	}
#ifdef EUCPC_DEBUG    		
	g_Log.writeLog(D1,"ActiveSocketFun() m_bStop == true, now stop.");
#endif

	m_sock = INVALID_SOCKET;
	m_nActiveTimes = 0;					
	m_bSockError = true;			
	m_bStop = true;

	m_socketThread.m_dwThreadId = 0;
	
	//
	memset(m_strTemp,0,sizeof(m_strTemp));
	m_nPos = 0;
	m_nSmsPos   = 0;
	m_nSmsCount = 0;
	m_nReportPos = 0;
	m_nReportCount = 0;

	return 0;
}
//return 1:success, <0:error.
int TMTNMsgApi::getResult(long commID,int& srcRet,char* value,char* reconnStr) 
{
	
    struct timeval timeStart, timeStop;
    int timeuse = 0;
    gettimeofday(&timeStart,0);
    
	DWORD cmdid;
	while(!m_bStop && (m_sock != INVALID_SOCKET))
	{
		gettimeofday(&timeStop,0);
		timeuse = TIMEDELTA_MS(timeStop,timeStart);;
		if(timeuse > 20000)
		{
			m_nLateTimes++;
			break;
		}
		CSockTool::wait(100);
		if(m_revMsgBody[commID].m_bSet == true)
		{
			m_nLateTimes = 0;
			
			cmdid = m_revMsgBody[commID].m_nCommandId;
			if(11 == cmdid || 12 == cmdid) //balance, price
			{
					int len = strlen(m_revMsgBody[commID].m_strResult);
					memcpy(value,m_revMsgBody[commID].m_strResult,len);
					
					srcRet = 0;
					m_nLateTimes = 0;
					
					m_revMsgBody[commID].m_bSet = false;
					return 1;
			}
			else if(cmdid >=1 && cmdid <=12)
			{
					srcRet = m_revMsgBody[commID].m_nResult;
					m_nLateTimes = 0;
					
					m_revMsgBody[commID].m_bSet = false;
					return 1;			
			}
			return -2;
		}
	}//end while	
	if(m_nLateTimes >= 2)
	{
	    m_nLateTimes = 0;
		m_bSockError = true;
	}

	//judge if sendsms then retry conn.
	if(m_sock == INVALID_SOCKET)
	{
			//block and conn 
			if(CMDID_SEND_SMS == commID || CMDID_SEND_SCHE_SMS == commID )
			{
				int ret = sendsms_reconn(srcRet,reconnStr);
#ifdef EUCPC_DEBUG    		
	g_Log.writeLog(D1,"sendsms_reconn() ret == [%d], srcRet = [%d].",ret,srcRet);
#endif				
				return ret;
			}
	}
    return -1;
}



}//namespace end



