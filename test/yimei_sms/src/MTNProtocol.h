#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#ifndef MTNPROTOCOL_H
#define MTNPROTOCOL_H 

namespace MTNMsgApi_NP
{
	
	typedef unsigned int  DWORD;
	typedef unsigned char BYTE;   
	
	//#define NULL  (void*)0;
	
	/*
	满意通传输协议
	*/
	//=====================================================================
	//                         命令ID   
	//=====================================================================
	#define MESSAGE_HEAD_LEN			         12								 //消息头	
	
	#define METONE_FIRST_REGISTRY             0xC001      //客户端软件注册（方法1）
	#define METONE_SECOND_REGISTRY            0xC002      //客户端软件企业信息注册
	#define METONE_QUERY_BALANCE              0xC003      //客户端软件查询余额
	#define METONE_CHARGE_UP                  0xC004      //客户端软件充值
	#define METONE_SEND_SMS                   0xC015      //客户端软件发送短信
	#define METONE_SEND_SMS_2                 0xC025      //客户端软件发送短信2	
	#define METONE_SEND_SMS_EX                0xC035      //客户端软件发送短信EX	
	#define METONE_SEND_SMS_EX_2              0xC045      //客户端软件发送短信EX2	
	#define METONE_SEND_SCHEDULED_SMS         0xC016      //客户端软件发送定时短信
	#define METONE_SEND_SCHEDULED_SMS_2       0xC026      //客户端软件发送定时短信2
	#define METONE_SEND_SCHEDULED_SMS_EX      0xC036      //客户端软件发送定时短信Ex
	#define METONE_SEND_SCHEDULED_SMS_EX_2    0xC046      //客户端软件发送定时短信Ex2
	#define METONE_RECEIVE_SMS                0xC007      //客户端软件接收短信
	#define METONE_MO_FORWARD_EX              0xC009      //客户端软件开通MO转发服务扩展
	#define METONE_CANCEL_MO_FORWARD          0xC00A      //客户端软件取消MO转发服务
	#define METONE_QUERY_EACH_FEE             0xC00B      //客户端软件查询单条发送费用
	#define METONE_LOGOUT_SN                  0xC00C      //客户端软件注销注册号码
	#define METONE_REGISTRY_PWD_UPDATE        0xC00E      //客户端软件注册号码密码修改
	#define ERCP_METONE_GET_STATUS_REPORT_EX  0xC012      //客户端软件获得状态报告(扩展)
	
	//=====================================================================
	//                        协议消息头 
	//=====================================================================	
	
	const static int CMDID_REG_1          = 1;//注册
	const static int CMDID_SEND_SMS       = 2;//发送短信
	const static int CMDID_SEND_SCHE_SMS  = 3;//发送定时短信
	const static int CMDID_CHARGE_UP      = 4;//充值
	const static int CMDID_UN_REG         = 5;//注销序列号
	const static int CMDID_REG_2          = 6;//企业信息注册
	const static int CMDID_PWD_UP         = 7;//密码修改
	const static int CMDID_CANCAL_TRANS   = 8;//取消转接
	
	const static int CMDID_REG_TRANS      = 10;//注册转接
	const static int CMDID_BALANCE        = 11;//查询余额
	const static int CMDID_PRICE          = 12;//查询单价
	const static int CMDID_RECV_SMS       = 13;//接收短信
	const static int CMDID_RECV_REPORT    = 14;//接收状态报告
	const static int CMDID_HEART_BAG      = 15;//心跳处理
            
	const static int MAX_SMS_BODY         = 10;//短信最大数量
	const static int MAX_REPORT_BODY      = 40;//报告最大数量
	
	const static int MAX_MSG_BODY_LEN     = 16*1024;//发送消息最大长度
	const static int MAX_SN_COUNT         = 128;//在内存中存储的最大SN的数量
	const static int SLEEP_SECOND_20      = 20;
	const static int SLEEP_SECOND_10      = 10;


	
	class RecvMsgBody
	{
	public:
		char m_strResult[10];
		char m_strExpress[2048];
		char m_strCnt[16*2048];
		char m_strCommand[8];
		char m_strKeyid[15];
		int  m_nResult;
		DWORD m_nCommandId;
		DWORD m_nSequenceNum;
	  bool m_bSet;

		RecvMsgBody()
		{
			memset(m_strResult,0,sizeof(m_strResult));
			memset(m_strExpress,0,sizeof(m_strExpress));
			memset(m_strCnt,0,sizeof(m_strCnt));
			memset(m_strCommand,0,sizeof(m_strCommand));
			memset(m_strKeyid,0,sizeof(m_strKeyid));
			m_nResult = 0;	
			m_nCommandId = 0;
			m_nSequenceNum = 0;
			m_bSet = false;
		};
		// 拷贝构造函数 
		RecvMsgBody(const RecvMsgBody &other) 
		{ 
			memcpy(m_strResult,other.m_strResult,sizeof(m_strResult));
			memcpy(m_strExpress,other.m_strExpress,sizeof(m_strExpress));
			memcpy(m_strCnt,other.m_strCnt,sizeof(m_strCnt));
			memcpy(m_strCommand,other.m_strCommand,sizeof(m_strCommand));
			memcpy(m_strKeyid,other.m_strKeyid,sizeof(m_strKeyid));
			m_nResult = other.m_nResult;
			m_nCommandId = other.m_nCommandId;
			m_nSequenceNum = other.m_nSequenceNum;
			m_bSet = other.m_bSet;
		};
		// 赋值函数 
		RecvMsgBody& operator=(const RecvMsgBody &other) 
		{ 
			// (1) 检查自赋值 
			if(this == &other) 
				return *this; 
			// (2) 释放原有的内存资源 
			//delete [] m_data; 
			// (3) 分配新的内存资源，并复制内容 
			//int length = strlen(other.m_data); 
			//m_data = new char[length+1]; 
			//strcpy(m_data, other.m_data); 
			memcpy(m_strResult,other.m_strResult,sizeof(m_strResult));
			memcpy(m_strExpress,other.m_strExpress,sizeof(m_strExpress));
			memcpy(m_strCnt,other.m_strCnt,sizeof(m_strCnt));
			memcpy(m_strCommand,other.m_strCommand,sizeof(m_strCommand));
			memcpy(m_strKeyid,other.m_strKeyid,sizeof(m_strKeyid));
			m_nResult = other.m_nResult;
			m_nCommandId = other.m_nCommandId;
			m_nSequenceNum = other.m_nSequenceNum;
			m_bSet = other.m_bSet;
			// (4)返回本对象的引用 
			return *this; 
		};
	};	
	//======================================================================
	//                     协议命令响应消息体
	//======================================================================
	//接收状态报告的消息体扩展  带sequenceid
	class RecvReportBody
	{
	public:
		char m_strSequenceId[15];
		char m_strMobile[21];
		char m_strSubmitdate[15];
		char m_strRecieverdate[15];
		char m_strErrorCode[25];
		char m_strMemo[50];
		char m_strServiceCodeAdd[15];
		char m_strReportType[2];

		RecvReportBody()
		{
			memset(m_strSequenceId,0,sizeof(m_strSequenceId));
			memset(m_strMobile,0,sizeof(m_strMobile));
			memset(m_strSubmitdate,0,sizeof(m_strSubmitdate));
			memset(m_strRecieverdate,0,sizeof(m_strRecieverdate));
			memset(m_strErrorCode,0,sizeof(m_strErrorCode));
			memset(m_strMemo,0,sizeof(m_strMemo));
			memset(m_strServiceCodeAdd,0,sizeof(m_strServiceCodeAdd));
			memset(m_strReportType,0,sizeof(m_strReportType));
		};
		// 拷贝构造函数 
		RecvReportBody(const RecvReportBody &other) 
		{ 
			memcpy(m_strSequenceId,other.m_strSequenceId,sizeof(m_strSequenceId));
			memcpy(m_strMobile,other.m_strMobile,sizeof(m_strMobile));
			memcpy(m_strSubmitdate,other.m_strSubmitdate,sizeof(m_strSubmitdate));
			memcpy(m_strRecieverdate,other.m_strRecieverdate,sizeof(m_strRecieverdate));
			memcpy(m_strErrorCode,other.m_strErrorCode,sizeof(m_strErrorCode));
			memcpy(m_strMemo,other.m_strMemo,sizeof(m_strMemo));
			memcpy(m_strServiceCodeAdd,other.m_strServiceCodeAdd,sizeof(m_strServiceCodeAdd));
			memcpy(m_strReportType,other.m_strReportType,sizeof(m_strReportType));
		};
		// 赋值函数 
		RecvReportBody& operator=(const RecvReportBody &other) 
		{ 
			// (1) 检查自赋值 
			if(this == &other) 
				return *this; 
			// (2) 释放原有的内存资源 
			//delete [] m_data; 
			// (3) 分配新的内存资源，并复制内容 
			//int length = strlen(other.m_data); 
			//m_data = new char[length+1]; 
			//strcpy(m_data, other.m_data); 
			memcpy(m_strSequenceId,other.m_strSequenceId,sizeof(m_strSequenceId));
			memcpy(m_strMobile,other.m_strMobile,sizeof(m_strMobile));
			memcpy(m_strSubmitdate,other.m_strSubmitdate,sizeof(m_strSubmitdate));
			memcpy(m_strRecieverdate,other.m_strRecieverdate,sizeof(m_strRecieverdate));
			memcpy(m_strErrorCode,other.m_strErrorCode,sizeof(m_strErrorCode));
			memcpy(m_strMemo,other.m_strMemo,sizeof(m_strMemo));
			memcpy(m_strServiceCodeAdd,other.m_strServiceCodeAdd,sizeof(m_strServiceCodeAdd));
			memcpy(m_strReportType,other.m_strReportType,sizeof(m_strReportType));
			// (4)返回本对象的引用 
			return *this; 
		};		
		
	};	
	//======================================================================
	//                       协议命令消息体
	//======================================================================
	//接收短信的响应消息体
	class RecvSMSBody
	{
	public:
		char m_strSMSMobile[21];    //发送者（可能是手机用户也可能是企业服务代码）
		char m_strAddiSerial[15];   //上行服务代码的附加号码
		char m_strAddSerialRecv[15]; //上行服务代码的附加号码(接收者)
		char m_strSMSContent[255];  //发送内容
		char m_strSMSTime[15];      //发送时间（格式：yyyymmddhhnnss）
		char m_strCHANNELNUMBER[31];

		RecvSMSBody()
		{
			memset(m_strSMSMobile,0,sizeof(m_strSMSMobile));
			memset(m_strAddiSerial,0,sizeof(m_strAddiSerial));
			memset(m_strAddSerialRecv,0,sizeof(m_strAddSerialRecv));
			memset(m_strSMSContent,0,sizeof(m_strSMSContent));
			memset(m_strSMSTime,0,sizeof(m_strSMSTime));
			memset(m_strCHANNELNUMBER,0,sizeof(m_strCHANNELNUMBER));
		};
		// 拷贝构造函数 
		RecvSMSBody(const RecvSMSBody &other) 
		{ 
			memcpy(m_strSMSMobile,other.m_strSMSMobile,sizeof(m_strSMSMobile));
			memcpy(m_strAddiSerial,other.m_strAddiSerial,sizeof(m_strAddiSerial));
			memcpy(m_strAddSerialRecv,other.m_strAddSerialRecv,sizeof(m_strAddSerialRecv));
			memcpy(m_strSMSContent,other.m_strSMSContent,sizeof(m_strSMSContent));
			memcpy(m_strSMSTime,other.m_strSMSTime,sizeof(m_strSMSTime));
			memcpy(m_strCHANNELNUMBER,other.m_strCHANNELNUMBER,sizeof(m_strCHANNELNUMBER));
		};
		// 赋值函数 
		RecvSMSBody& operator=(const RecvSMSBody &other) 
		{ 
			// (1) 检查自赋值 
			if(this == &other) 
				return *this; 
			// (2) 释放原有的内存资源 
			//delete [] m_data; 
			// (3) 分配新的内存资源，并复制内容 
			//int length = strlen(other.m_data); 
			//m_data = new char[length+1]; 
			//strcpy(m_data, other.m_data); 
			memcpy(m_strSMSMobile,other.m_strSMSMobile,sizeof(m_strSMSMobile));
			memcpy(m_strAddiSerial,other.m_strAddiSerial,sizeof(m_strAddiSerial));
			memcpy(m_strAddSerialRecv,other.m_strAddSerialRecv,sizeof(m_strAddSerialRecv));
			memcpy(m_strSMSContent,other.m_strSMSContent,sizeof(m_strSMSContent));
			memcpy(m_strSMSTime,other.m_strSMSTime,sizeof(m_strSMSTime));
			memcpy(m_strCHANNELNUMBER,other.m_strCHANNELNUMBER,sizeof(m_strCHANNELNUMBER));
			// (4)返回本对象的引用 
			return *this; 
		};		
	};			
	//企业信息注册消息体
	typedef	struct MetoneEntityRegistryBody{
		DWORD messageLength;  //消息的总长度(字节)
		DWORD commandId;      //命令ID
		DWORD sequenceNum;    //序列号，可以循环计数
		char SerialID[30];      //注册号码（非MD5）
		char ServiceTypeID[2];  //服务类型编码（必填写） 
		char HDSerial[33];      //硬盘序列号（MD5）
		char EName[60];         //企业名称
		char ELinkMan[20];      //联系人
		char EPhone[20];        //电话
		char EMobile[15];       //移动电话
		char EEmail[60];        //邮箱
		char EFax[20];          //传真
		char EAddress[60];      //地址
		char EPostcode[6];      //邮编
		int GetLen() 
		{
			return (12 + 30 + 2 + 33 + 60 + 20 + 20 + 15 + 60 + 20 + 60 + 6);
		};
	}MetoneEntityRegistryBody;
	
	
	//发送即时短信消息体
	typedef	struct MetoneSendSMSBody
	{
		DWORD messageLength;  //消息的总长度(字节)
		DWORD commandId;      //命令ID
		DWORD sequenceNum;    //序列号，可以循环计数
		DWORD SMSCount;         //接收者数量，取值范围1至1000，以下字段可以重复
		char SMSPriority[2];
		char SerialID[30];      //注册号码（非MD5）
		char HDSerial[33];      //硬盘序列号（MD5）
		char AddiSerial[15];    //上行服务代码的附件号码
		char SMSContent[1002];   //发送内容
		
		char sequenceNum1[32];  //yyyymmddhhmmss + 毫秒 + 用户自定义8位
		char SMSMobile[15000];  //接收者（可能是手机用户也可能是企业服务代码，
	                            //号码之间使用逗号隔开),长度　15 * SMSCount
		int GetLen(int count) 
		{
			return (16 + 2 + 30 + 33 + 15 + 1002 +  32 + count);
		};
	}MetoneSendSMSBody;
	
	
	const static int SERVICECODE_LEN = 6;										//上行服务代码长度
	const static int XLT_LEN = 10;													//小灵通最短长度
	const static int XLT_LONG_LEN = 12;											//小灵通最长长度
	const static int MOBILE_LEN = 11;												//手机号码长度
	const static int MOBILE_LONG_LEN = 13;									//手机号码长度
	
	//错误代码常量
	const static int SERIAL_ERROR = 100;					//序列号码为空或无效
	const static int NET_ERROR = 101;							//网络故障
	const static int OTHER_ERROR = 102;						//其它故障
	const static int ENTITYINFO_ERROR = 103;			//注册企业基本信息失败，当软件注册号码注册成功
	const static int NOTENOUGHINFO_ERROR = 104;		//注册信息填写不完整
	const static int PARAMISNULL_ERROR = 105;			//参数balance指针为空
	const static int CARDISNULL_ERROR = 106;			//卡号或密码为空或无效
	const static int MOBILEISNULL_ERROR = 107;		    //手机号码为空
	const static int MOBILESPEA_ERROR = 108;			//手机号码分割符号不正确
	const static int MOBILEPART_ERROR = 109;			//部分手机号码不正确，已删除，其余手机号码被发送
	const static int SMSCONTENT_ERROR = 110;			//短信内容为空或超长（70个汉字）
	const static int ADDICODE_ERROR = 111;				//附加号码过长（8位）
	const static int SCHTIME_ERROR = 112;					//定时时间为空或格式不正确(YYYYMMDDHHNNSS)
	const static int PWD_ERROR = 113;							//旧密码或新密码为空
	const static int FLAG_ERROR = 114;						//得到标识错误
	const static int FEE_ERROR = 201;							//计费失败
	const static int SUCCESS = 1;									//ok
	const static int SUCCESS_LEFT = 2;						//成功,还有短消息接收
	const static int FAILURE = 0;									//失败
	const static int UNKNOW_ERROR = -1;						//未知故障
	//const static char* SERVICETYPEID = "01";      //服务类型，短信服务
	const static int ADDI_LEN = 10;               //附加号码最大长度
	
	const  static int SOCKET_ERROR = -1;
	const  static int INVALID_SOCKET = -1;
	const  static int FUN_CALL_TOO_FAST = 999;  //函数调用过快
	const  static int NOT_RECV_SEND_SMS_RESULT = 997;  //发送短信包后，没有得到短信发送结果包，发送是否成功未知。
  const  static int SEQNUM_ERROR=120;            //序列ID格式错

	



}//namespace end.


#endif



