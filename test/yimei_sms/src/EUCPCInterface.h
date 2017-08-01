/**
	不做穿透代理处理
*/
#ifndef EUCPCINTERFACE_H
#define EUCPCINTERFACE_H 

namespace MTNMsgApi_NP
{
	
	//接收短信时使用的回调函数
	typedef void  MTNRecvContent(char* mobile,char* senderaddi, char* recvaddi,	
									char* ct,char* sd,int* flag); 
	//接收短信时使用的回调函数(带通道号)
	typedef void  MTNRecvContentCHL(char* mobile,char* senderaddi, char* recvaddi,
								   char* ct,char* sd,char* chl,int* flag);

	//接收短信状态报告时使用的回调函数
	typedef void MTNGetStatusReport(char *mobile, char *errorcode,char *senderaddi,char *reptype, int *flag);
	//注册消息
	
	//接收‘发送短信结果’时使用的回调函数
	typedef void  MTNSendResultCallBack(void* sequencenum,int* sendresult);
	
	
	//zft add 2005-11-04
	//以下为接收状态报告扩展的回调函数
	
	//接收状态报告使用的回调函数
	typedef void MTNGetStatusReportEx(char *seqnum, char *mobile, char *errorcode, 
										   char *senderaddi,
										   char *reptype, int *flag);
	
	
	//1.Register 10 para.
	int Register(char* sn,char* pwd,char* EntName,char* LinkMan,char* Phone,char* Mobile,char* Email,char* Fax,char* sAddress,char* Postcode);
	//获得帐户余额(length 10)
	//2.GetBalance  2 para.
	int GetBalance(char* sn,char* balance);
	//短信充值
	//3.ChargeUp  3 para.
	int ChargeUp(char* sn,char* acco,char* pass);
	
	int RegistryTransfer(char* sn,char* phone);
	//取消转移功能
	int CancelTransfer(char* sn);
	
	int GetPrice(char* sn,char* balance); 
	
	
	//发送短消息
	//发送短消息到EUCP平台
	int SendSMS(char* sn,char* mn,char* ct,char* priority);
	int SendSMS2(char* sn,char* mn,char* ct, char* seqnum,char* priority);
	//发送短消息到EUCP平台,可以带附加号码
	int SendSMSEx(char* sn,char* mn,char* ct,char* addi,char* priority);
	//发送短消息到EUCP平台,可以带附加号码
	int SendSMSEx2(char* sn,char* mn,char* ct,char* addi, char* seqnum,char* priority); 
	//发送短消息到EUCP平台 通用函数
	int SendCommSMS(char* sn,char* mn,char* ct,char* addi, char* seqnum,char* priority);
	
	
	//发送定时短信,时间格式yyyymmddhhnnss
	int SendScheSMS(char* sn,char* mn,char* ct,char* sendtime,char* priority);
	int SendScheSMS2(char* sn,char* mn,char* ct, char* sendtime,char* seqnum,char* priority);
	int SendScheSMSEx(char* sn,char* mn,char* ct,char* sendtime,char* addi,char* priority);
	int SendScheSMSEx2(char* sn,char* mn,char* ct,char* sendtime,char* addi, char* seqnum,char* priority);
	//发送短消息到EUCP平台 通用函数
	int SendCommScheSMS(char* sn,char* mn,char* ct,char* sendtime,char* addi, char* seqnum,char* priority);
	
	//接收短消息
	int ReceiveSMS(char* sn, MTNRecvContent* rcex);
	int ReceiveSMSEx(char* sn, MTNRecvContent* rcex);
	int ReceiveSMSCHL(char* sn, MTNRecvContentCHL* rcex);//带通道号
	
	//接收状态报告
	int ReceiveStatusReportEx(char* sn, MTNGetStatusReportEx* Stax); 
	
	//注销注册
	int UnRegister(char* sn);
	
	//修改软件序列号对应的密码
	int RegistryPwdUpd(char* sn, char* oldpwd, char* newpwd);
	
	//设置关键字
	int  SetKey(char* key);
	
	void  GetSDKVersion(char* ver);

	//
	int SetLog(char*path,char*name,int size,int mask);
	int Close();
	
	
}//namespace end.

#endif



