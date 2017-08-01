/**
	消息具体实现类
*/


#include "MTNProtocol.h"
#include "EUCPCInterface.h"
#include "thread.h"
	

#include <string>
#include <set>	
#include <queue>
#include <map>
#include <vector>
	
using namespace std;

namespace MTNMsgApi_NP
{
	/**
		声明，所以的返回值为int型的，1为成功，其他为错误
	*/
	class TMTNMsgApi 
	{
	public:
		TMTNMsgApi();
	    ~TMTNMsgApi();
	    
		int  m_sock; 

		bool m_bSockError;
		bool m_bThreadCreated;
		char m_ip[100];
		unsigned int m_port;//缺省端口号
	
		char m_ServiceID[3];
		bool m_bOnWork;
		char m_KeyVal[33];
		//
	    int domainToIP(char* domain);
		int ValidateDigit(const char* mobile);
		int ValidateDigitPriority(const char* priority); 
		void SetSocket(int socket);
		void ActiveThread();
		
		static int static_thread_func(void* p);
		static int static_sc_thread_func(void* p);
		
		
		int ActiveSocketFun();
		int ActiveSocketSCFun();
		
		int m_nSockFunThreadNum;
		int m_nSCFunThreadNum;

		int on_read();

		int SendMsgHead1(const int commID,const int SMSCount, int seq);
		int SendMsgHead2(const int commID,const int SMSCount, int seq = 0); 
		int SendCloseHead1(const int commID,const int SMSCount, int seq); 
		
	    int SendMsgBody(char* msg,int len);
		int ConnectTest();
	    int ValidateFlag(); 
	    
		bool m_bIsRecSms;
		bool m_bIsRecReport;
		bool m_bStop;
		int m_nLateTimes;
		int m_nActiveTimes;
		
		
		mutex_object m_MutexObject;
		CThread m_socketThread;
		CThread m_SCThread;	
		//
		pthread_mutex_t m_sc_mutex;
		pthread_cond_t  m_sc_cond;
		//

		//获得硬盘标识
		char* GetHDSerial();
		int GetNetMac(char*);
		char* GetKeyVal();
		
		//业务方法
		int SendRegister_1(char* sn, char* pwd);//对外接口 第一次通信
		int SendRegister_2(char* sn,
			char* EntName,char* LinkMan,char* Phone,
			char* Mobile,char* Email,char* Fax,char* sAddress,char* Postcode);//对外接口 第二次通信
		int GetBalance(char* sn,char* balance);
		int GetPrice(char* sn,char* balance); 
	
		int RegistryPwdUpd(char* sn, char* oldpwd, char* newpwd);
		int ChargeUp(char* sn,char* acco,char* pass);
		int UnRegister(char* sn);
		int SendSMS(char* sn,char* mn,char* ct,int count,int mobilecount,char* priority);
		int SendSMS2(char* sn,char* mn,char* ct,int count,int mobilecount, char* seqnum,char* priority);
		//发送短消息到EUCP平台,可以带附加号码
		int SendSMSEx(char* sn,char* mn,char* ct,char* addi,int count,int mobilecount,char* priority);
		int SendSMSEx2(char* sn,char* mn,char* ct,char* addi,int count,int mobilecount, char* seqnum,char* priority);
		//发送定时短信
		int SendScheSMS(char* sn,char* mn,char* ct,char* sendtime,int count,int mobilecount,char* priority); 
		int SendScheSMS2(char* sn,char* mn,char* ct,char* sendtime,int count,int mobilecount, char* seqnum,char* priority);
		//发送短消息到EUCP平台,可以带附加号码
		int SendScheSMSEx(char* sn,char* mn,char* ct,char* sendtime,char* addi,int count,int mobilecount,char* priority);
		int SendScheSMSEx2(char* sn,char* mn,char* ct,char* sendtime,char* addi,int count,int mobilecount, char* seqnum,char* priority);
		//设置MO转发服务
		int RegistryTransfer(char* sn,char*phone[]);
		//取消转移功能
		int CancelTransfer (char* sn);
		int ReceiveSMS(char* sn, MTNRecvContent* rcex);
		int ReceiveSMSEx(char* sn, MTNRecvContent* rcex);
		int ReceiveSMSCHL(char* sn, MTNRecvContentCHL* channel);
		//
		int GetReceiveSMS(int flag,MTNRecvContent* rcex,MTNRecvContentCHL* channel); 
		//
		int ReceiveStatusReportEx(char* sn, MTNGetStatusReportEx* Stax);
		//
		int GetReceiveReportEx(MTNGetStatusReportEx* Stax);
		//
		
		//
		int ValidateMobile(const char* mobile);
		int FilterMobile(char* dest,const char* srcmobile,int* count);	
		//for test
		int Close();

		//--<Added by hai>
		bool is_heartbeat_msg(const char* msg,char* seq_number_str);
		int send_heartbeat_resp(const char* seq_number_str);
		
		//--<Added by hai>

		
		/////////////////////////////////////////////////////////////
		RecvMsgBody m_revMsgBody[20];//use 14
		//
		RecvSMSBody m_revSmsBody[MAX_SMS_BODY];//10, cur only use 5
		int m_nSmsPos;
		int m_nSmsCount;
		RecvReportBody m_revReportBody[MAX_REPORT_BODY];
		int m_nReportPos;
		int m_nReportCount;
		
		int m_nPos;
		char m_strTemp[MAX_MSG_BODY_LEN];
		int splitStr();
		int parse(char* tmpStr);
		int getResult(long commID,int& srcRet,char* value,char*reconnStr=NULL); 
		int addRecvSmsBody(char* src_cnt);
		int addRecvSmsBody_bak(char* src_cnt);
		int addRecvReportBody(char* src_cnt);
		int sendAndGetResult(long commID,char*message,int& retResult,char* retValue,char* reconnStr=NULL);
		int setSendMsgChar(char* message,char* sn, char*pwd,char* newpwd,
			   char*ServiceTypeId,long commID,
			   char* acco,char* pass,char* phone[]);
		int setSendSMSMsgChar(char* message,char* sn, long commID,
				char*mn,char* ct,char* sendtime,char* addi,
				int count,int mobilecount,char* seqnum,
				char* priority,char* destStr);

		mutex_object m_MuxConnObj;
		mutex_object m_MuxSmsObj;
		/////////////////////////////////////////////////////////////
		int setBalanceFromSms(char* src_cnt);
		int setSnAndBalanceToArray(char*SN,char* Balance);
		int getSnAndBalanceFromArray(char*SN,char* Balance,struct timeval* LastTimeVal);
		char m_arrSN[MAX_SN_COUNT][128];
		char m_arrBalance[MAX_SN_COUNT][128];
		struct timeval m_arrLastBalanceTime[MAX_SN_COUNT];
		int m_nCurSnPos;
		
		////////////////////////////////////////////////////////////
		//4.0.4
		int GetCurTime(char* strtime);
		int GetAndSetCurSendSmsReconnStr(char* sn,char*timestr,char*deststr);
		static const int m_reconnect_num = 6;     //
		static const int m_reconnect_interval = 10; //10s	
		int sendsms_reconn(int& srcRet,char* reconnStr);
	};

}//namespace end




