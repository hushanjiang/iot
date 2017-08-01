
#include "EUCPCInterface.h"

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
	
using namespace std;  

using namespace MTNMsgApi_NP; 

void parseMsg(int result)
{
	if (result == 1)
		printf("成功 Return=0\n");
	else if (result == -1)
		printf("系统异常 Return=-1\n");
	else if (result == -111)
		printf("服务器返回:企业信息注册失败\n");
	else if (result == -124)
		printf("服务器返回:查询余额失败\n");
	else if (result == -113)
		printf("服务器返回:充值失败\n");
	else if (result == -117)
		printf("服务器返回:发送信息失败\n");
	else if (result == -122)
		printf("服务器返回:注销失败\n");
	else
	{
		printf("其它错误值:");
		printf("%d\n",result);
	}
	return;
}
/////////////////////////////////////////////////////////////////////////
//接收短信需要的用户自定义的回调函数
// 示例代码为直接输出到控制台
// mobile:手机号
// senderaddi:发送者附加号
// recvaddi:接收者附加号
// ct:短信内容
// sd:日期
// *flag : 标志，为 1 表示有短信
// 上行短信务如果要保存,需要加入业务逻辑代码,如：保存数据库，写文件等等
////////////////////////////////////////////////////////////////////////
void MTNRecvContentA_test(char * mobile,char* senderaddi, char* recvaddi,char* ct,char* sd,int* flag)
{
	if(*flag)
	{
		printf("mobile:%s\n",mobile);
		printf("senderaddi:%s\n",senderaddi);
		printf("recvaddi:%s\n",recvaddi);
		printf("ct:%s\n",ct);
		printf("sd:%s\n",sd);
		printf("\n\n\n");
	}

	return;
}

/////////////////////////////////////////////////////////////////////////
//接收短信状态报告需要的用户自定义的回调函数
// 示例代码为直接输出到控制台
// seqnum:序列号
// mobile:手机号
// errorcode:错误码
// senderaddi:发送者附加号
// reptype:报告类型
// *flag : 标志，为 1 表示有短信
// 上行短信务如果要保存,需要加入业务逻辑代码,如：保存数据库，写文件等等
////////////////////////////////////////////////////////////////////////
void MTNGetStatusReportEx_test(char *seqnum, char *mobile, char *errorcode, 
										   char *senderaddi,
										   char *reptype, int *flag)
{
	if(*flag)
	{
		printf("seqnum:%s\n",seqnum);	
		printf("mobile:%s\n",mobile);
		printf("errorcode:%s\n",errorcode);
		printf("senderaddi:%s\n",senderaddi);
		printf("reptype:%s\n",reptype);
		printf("\n\n\n");
	}

	return;	
}



//API 参数结构体							   
struct API_PARA
{
	//通用信息
	char sn[100];          //序列号
	char pwd[50];          //密码
	char pwd2[50];         //新密码
	char cardcode[100];    //充值卡号
	char cardpwd[50];      //充值卡密码
	char sechdueTime[100]; //定时时间(必须大于当前时间半小时，小于一年以内)
	
	//企业注册
	char EntName[100];   //企业名称
	char LinkMan[100];   //联系人
	char Phone[10*1024]; //手机号码(以半角逗号 "," 分隔，不能超过1000个号码)
	char Mobile[100];    //联系人手机号(必须为合法的手机号码)
	char Email[100];     //联系人Email(邮箱地址格式必须合法)
	char Fax[100];       //联系人传真号码(必须为合法的电话号码)
	char sAddress[100];  //联系人地址
	char Postcode[100];  //联系人邮政编码
	
	char Balance[50];    //余额
	char Price[50];      //短信单价(0.1元/条)
	char Verison[50];	 //SDK版本号
	
	//
	char TransferPhone[200]; //注册转接的手机号码
	char key[100];           //生成Key值的字符串
	
	//手机发送短信
	char mn[20*1024]; //手机号码
	char ct[200];     //短信内容
	char addi[20];    //通道扩展号码
	int  seqid;       //短信ID
	char priority[10];//短信优先级(1到5之间，5为最高优先级)
}; 



//帮助提示
int showHelp()
{
		cout << "**********Select number for test call**********" << endl;
		cout << "0  for exit"<<endl;
		cout << "1  for Help"<<endl;
		cout << "2  for Get SDK Version" << endl;
		cout << "3  for Set Key" << endl;
		cout << "4  for Register" << endl;
		cout << "5  for Change Pwd" << endl;
		cout << "6  for Charge Up" << endl;
		cout << "7  for Get Balance" << endl;
		cout << "8  for Get Price" << endl;
		cout << "9  for UnRegister" << endl;
		cout << "10 for Registry Transfer" << endl;
		cout << "11 for Cancel Transfer" << endl;
		cout << "********** test send sms**********" << endl;
		cout << "12 for SendSMS()" << endl;
		cout << "13 for SendSMSEx()" << endl;
		cout << "14 for SendScheSMS()" << endl;
		cout << "15 for SendScheSMSEx()" << endl;
		cout << "********** test recv sms**********" << endl;
		cout << "16 for ReceiveSMS()" << endl;
		cout << "17 for ReceiveSMSEx()" << endl;
		cout << "18 for ReceiveStatusReportEx()" << endl;
		cout << "********** test send sms with seqid**********" << endl;
		cout << "19 for SendSMSEx2()" << endl;
		cout << "********** close**********" << endl;
		cout << "20 for Close()" << endl;		
		return 0;
}


//设置API参数
int setParam(API_PARA &param)
{
	sprintf(param.sn,"%s","8SDK-EMY-6699-RJYNR");
	sprintf(param.pwd,"%s","834693");

	sprintf(param.pwd2,"%s","834693");
	
	sprintf(param.cardcode,"%s","AAA-BBB-CCC");
	sprintf(param.cardpwd,"%s","20080808");	

	sprintf(param.EntName,"%s","1");
	sprintf(param.LinkMan,"%s","1");		
	sprintf(param.Phone,"%s","1");		
	sprintf(param.Mobile,"%s","1");		
	sprintf(param.Email,"%s","1");		
	sprintf(param.Fax,"%s","1");		
	sprintf(param.sAddress,"%s","1");		
	sprintf(param.Postcode,"%s","1");	
	
	sprintf(param.TransferPhone,"%s","13322224444,13566667777,1234");	
	
	
	//手机发送短信		
	sprintf(param.mn,"%s","13480669285");				
	sprintf(param.ct,"%s","Hello,测试啊！");
	
	sprintf(param.sechdueTime,"%s","20171023105501");	

	sprintf(param.addi,"%s","123");	
	sprintf(param.priority,"%s","3");		
		
	return 0;
}




//主服务入口
int main()
{
	
#ifdef EUCPC_DEBUG    		
	//SetLog(NULL,"EUCPCLog",1024*1024*10,0x0020);//10M的日志，日志名称WR_EUCPCLog.txt
#endif
	
	API_PARA param;
	memset(&param,0,sizeof(API_PARA));
	
	//设置参数
	setParam(param);

	int ret = 0;//false;
	
	//显示帮助
	showHelp();
	
	
	while(1)
	{
		cout << "input choice:" ;
		string strTemp;
		cin >> strTemp;
		
		int input_num;
		input_num = atoi(strTemp.c_str());
		//cout << "input_num=" << input_num << endl;
		switch (input_num)
		{
			case 0:
				cout << "realy want quit?(y/n)" << endl;
				cin >> strTemp;
				if(strTemp[0] == 'y' || strTemp[0] == 'Y')
					return 0;
				else
					break;
			case 1:
				showHelp();
				cout << "input choice:" ;
				break;				
			case 2:
				/**
				* 查询余额
				* GetSDKVersion(软件版本号)
				* 【软件版本号字符串】:最少20个字节
				*/
				//查询版本号     
				GetSDKVersion(param.Verison);
				printf("GetSDKVersion() = %s\n",param.Verison);
				break;
			case 3:
				cout << "input Key:";
				cin >> param.key;
				
				/**
				* 设置Key
				* SetKey（自定义的字符串）
				* 如果软件序列号需要在多台机器上使用，需要先调用该函数一次，再发送短信
				* 
				*/				
				//设置Key(用于多台机器共用同一个序列号)这里添加 统一自定义的字符串   
				ret = SetKey(param.key);
				parseMsg(ret);
				break;
			case 4:
				/*
				cout << "input register sn:";
				cin >> param.sn;
				cout << "input register pwd:";
				cin >> param.pwd;
				
				
				cout << "input Ent Name:";
				cin >> param.EntName;
				cout << "input Link Man:";
				cin >> param.LinkMan;
				cout << "input Mobile:";
				cin >> param.Mobile;
				cout << "input Email:";
				cin >> param.Email;
				cout << "input Fax:";
				cin >> param.Fax;
				cout << "input Address:";
				cin >> param.sAddress;
				cout << "input Post code:";
				cin >> param.Postcode;
				*/
				
				/**
				 * 序列号注册
				 * 在计算机上发短信必须先注册该软件序列号
				 * 注意：此函数只需执行一次即可，直到将该序列号换到别的计算机上时才需要重新注册。
				 * Register(序列号,密码,企业名称,联系人姓名,联系电话,联系手机,电子邮件, 联系传真,公司地址, 邮政编码)
				 * 参数中： 
				 *【序列号】  通过亿美销售人员获取
				 *【密码】    通过亿美销售人员获取，长度为6个字节 
				 *【企业名称】最多60字节，必须输入
				 *【联系人姓名】(最多20字节)，必须输入
				 *【联系电话】(最多20字节)，必须输入
				 *【联系手机】(最多15字节)，必须输入
				 *【电子邮件】(最多60字节)，必须输入
				 *【联系传真】(最多20字节)，必须输入
				 *【公司地址】(最多60字节)，必须输入
				 *【邮政编码】(最多6字节)，必须输入			
				*/				
				ret = Register(param.sn,param.pwd,
								param.EntName,
								param.LinkMan,param.Phone,
								param.Mobile,param.Email,param.Fax,
								param.sAddress, param.Postcode);
				parseMsg(ret);
				break;	
			case 5:
				cout << "input old pwd:";
				cin >> param.pwd;
				cout << "input newpwd:";
				cin >> param.pwd2;
					
				/**
				* 修改序列号密码
				* RegistryPwdUpd(软件序列号,旧密码, 新密码);
				* 【密码】长度为6个字节
				*/									
				//修改密码   这里添加软件序列号 旧密码   新密码
				ret = RegistryPwdUpd(param.sn,param.pwd,param.pwd2);
				parseMsg(ret);
				break;	
				
			case 6:			
				cout << "input card code:";
				cin >> param.cardcode;
				cout << "input card pwd:";
				cin >> param.cardpwd;
							
				/**
				* 充值
				* chargeUp(软件序列号，充值卡卡号, 密码) 
				* 请通过亿美销售人员获取卡号和密码 
				* 【充值卡卡号】长度为20个字节以内
				* 【密码】长度为6个字节
				* 如果余额不足，发送短信会失败
				*/	
				//充值  这里添加软件序列号   　　卡号　　   密码								
				ret = ChargeUp(param.sn,param.cardcode,param.cardpwd);
				parseMsg(ret);
				break;	
				
			case 7:
				memset(param.Balance,0,sizeof(param.Balance));
				
				/**
				* 查询余额
				* 余额不足，发送短信回失败
				* GetBalance(软件序列号,余额);
				* 【余额字符串】最少10个字节
				*/	
				//查询余额   这里添加软件序列号  余额			
				ret = GetBalance(param.sn,param.Balance);
				parseMsg(ret);
				if(0 != strlen(param.Balance))
					printf("balance:%s\n",param.Balance);
				else
					printf("balance: null\n");
				break;	
				
			case 8:
				/**
				* 查询短信单条费用
				* GetPrice(软件序列号,价格);
				* 【价格字符串】至少20个字节
				*/			
				//查询单价 这里添加软件序列号,价格
				ret = GetPrice(param.sn,param.Price);
				parseMsg(ret);
				if(0 != strlen(param.Price))
					printf("price per sms:%s\n",param.Price);
				else
					printf("price per sms: null\n");
				break;	
			case 9:
				/**
				* 注销软件序列号
				* UnRegister(软件序列号)
				* 当需要在另外一台计算上发短信时，需要先将本机的软件序列号注销，再在别的机器上注册后，才能继续使用
				*/			
				//注销    这里添加软件序列号
				ret = UnRegister(param.sn);
				parseMsg(ret);
				break;	
				
			case 10:
				cout << "input RegistryTransfer phone(split with ','):";
				cin >> param.TransferPhone;
				
				/**
				* 注册转接
				* 如果需要将上行短信转到该手机上接收，需要调用该函数。
				* RegistryTransfer（软件序列号，手机号）
				* 【手机号】最多10个，手机号码以半角逗号分隔
				*/
				//注册转接的手机号  这里添加软件序列号    手机号
				ret = RegistryTransfer(param.sn,param.TransferPhone);
			    parseMsg(ret);
				break;	
			case 11:
				/**
				* 取消注册转接
				* 如果需要取消上行短信转发到手机上，需要调用该函数。
				* CancelTransfer（软件序列号）
				* 
				*/			
				//取消注册转接的手机号  这里添加软件序列号
				ret = CancelTransfer(param.sn);
				parseMsg(ret);
				break;	
			case 12:
				/**
				* 发送即时短信 
				* sendSMS(软件序列号, 手机号码,短信内容,优先级)
				* 
				* [手机号码]为手机号码字符串,以半角逗号分隔,最大小于200个手机号码
				* [短信内容]为短信内容,最大长度为500字
				* (不区分汉字与英文),亿美短信平台会根据实际通道自动拆分,计费以实际拆分条数为准.亿美推荐短信长度70字以内
				* [优先级]代表优先级，范围1~5，数值越高优先级越高
				*/		
				//即时发送 这里添加软件序列号,手机号,短信内容 ,优先级
				ret = SendSMS(param.sn,param.mn,param.ct,param.priority);
		    	parseMsg(ret);
		    	break;
			case 13:
				/**
				 * 发送即时短信(带附加号码，即扩展号)
				 * SendSMSEx(软件序列号,手机号码,短信内容, 附加号码,优先级)
				 * 
				 * [手机号码]为手机号码字符串,以半角逗号分隔,最大小于200个手机号码
				 * [短信内容]为短信内容,最大长度为500字
				 * (不区分汉字与英文),亿美短信平台会根据实际通道自动拆分,计费以实际拆分条数为准.亿美推荐短信长度70字以内 
				 * [附加号码]用户自定义的附加号码，一般小于6个字节
				 * [优先级]代表优先级，范围1~5，数值越高优先级越高
				 */
				//即时发送   这里添加软件序列号,手机号,短信内容,附加号码  ,优先级
				ret = SendSMSEx(param.sn,param.mn,param.ct,param.addi ,param.priority);
		    	parseMsg(ret);
		    	break;
			case 14:
				/**
				* 发送定时短信   
				* SendScheSMS(软件序列号, 手机号码,短信内容, 定时时间,优先级)
				* 
				* [定时时间]格式为:年年年年月月日日时时分分秒秒,例如:20090504111010 代表2009年5月4日 11时10分10秒,时间大于当前时间，小于一年以内
				* [手机号码]为手机号码字符串,以半角逗号分隔,最大小于200个手机号码
				* [短信内容]为短信内容,最大长度为500字(不区分汉字与英文),亿美短信平台会根据实际通道自动拆分
				* ,计费以实际拆分条数为准.亿美推荐短信长度70字以内 
				* [优先级]代表优先级，范围1~5，数值越高优先级越高
				*/
				//定时发送   这里添加软件序列号,手机号,短信内容   ,定时时间      ,优先级
				ret = SendScheSMS(param.sn,param.mn,param.ct,param.sechdueTime ,param.priority);
		    	parseMsg(ret);
		    	break;
			case 15:
				/**
				* 发送定时短信(带附加号码，即扩展号) 
				* SendScheSMSEx(软件序列号, 手机号码,短信内容, 定时时间,附加号码,优先级)
				* 
				* [定时时间]格式为:年年年年月月日日时时分分秒秒,例如:20090504111010 代表2009年5月4日 11时10分10秒,时间大于当前时间，小于一年以内
				* [手机号码]为手机号码字符串,以半角逗号分隔,最大小于200个手机号码
				* [短信内容]为短信内容,最大长度为500字(不区分汉字与英文),亿美短信平台会根据实际通道自动拆分
				* ,计费以实际拆分条数为准.亿美推荐短信长度70字以内 
				* [附加号码]用户自定义的附加号码，一般小于6位
				* [优先级]代表优先级，范围1~5，数值越高优先级越高
				*/			
				//定时发送    这里添加软件序列号,手机号,短信内容   ,定时时间     ,附加号码       ,优先级
				ret = SendScheSMSEx(param.sn,param.mn,param.ct,param.sechdueTime ,param.addi ,param.priority);
		    	parseMsg(ret);
		    	break;
			case 16:
				/**
				* 得到上行短信
				* ReceiveSMS(软件序列号，自定义的接收回调函数)
				* 【自定义的接收回调函数】：接收到的短信，会通过回调函数的参数传入，在函数内部可以进行处理，例如存入数据库，写文件等。
				*/
				//接收短信  这里添加软件序列号 ,自定义的接收回调函数
				ret = ReceiveSMS(param.sn,&MTNRecvContentA_test);
		    	parseMsg(ret);
		    	break;	
			case 17:
				/**
				* 得到上行短信(带附加号码)
				* ReceiveSMSEx(软件序列号，自定义的接收回调函数)
				* 【自定义的接收回调函数】：接收到的短信和附加号码，会通过回调函数的参数传入，在函数内部可以进行处理，例如存入数据库，写文件等。
				*  
				*/
				//接收短信(带附加号码)  这里添加软件序列号,自定义的回调函数
				ret = ReceiveSMSEx(param.sn,&MTNRecvContentA_test);
		    	parseMsg(ret);
		    	break;	
		   case 18:
				/**
				* 得到上行状态报告(带附加号码)
				* ReceiveSMSEx(软件序列号，自定义的接收回调函数)
				* 【自定义的接收回调函数】：接收到的短信和附加号码，会通过回调函数的参数传入，在函数内部可以进行处理，例如存入数据库，写文件等。
				*  
				*/
				//接收短信(带附加号码)  这里添加软件序列号,自定义的回调函数
				ret = ReceiveStatusReportEx(param.sn,&MTNGetStatusReportEx_test);
		    	parseMsg(ret);
		    	break;	
			case 19:
				/**
				 * 发送即时短信(带附加号码，即扩展号,短信ID)
				 * SendSMSEx(软件序列号,手机号码,短信内容, 附加号码,优先级)
				 * 
				 * [手机号码]为手机号码字符串,以半角逗号分隔,最大小于200个手机号码
				 * [短信内容]为短信内容,最大长度为500字
				 * (不区分汉字与英文),亿美短信平台会根据实际通道自动拆分,计费以实际拆分条数为准.亿美推荐短信长度70字以内 
				 * [附加号码]用户自定义的附加号码，一般小于6个字节
				 * [优先级]代表优先级，范围1~5，数值越高优先级越高
				 */
				//即时发送   这里添加软件序列号,手机号,短信内容,附加号码  ,优先级
				static int seqid=1;
				seqid++;
				char seqid_buf[20];
				memset(seqid_buf,0,sizeof(seqid_buf));
				sprintf(seqid_buf,"2222888800%d",seqid);
				cout << "seqid_buf:"<<seqid_buf<<endl;
				ret = SendSMSEx2(param.sn,param.mn,param.ct,param.addi ,seqid_buf ,param.priority);
		    	parseMsg(ret);
		    	break;
			    	
			case 20:
				ret = Close();
		    	parseMsg(ret);
		    	break;	
			default:
				cout << "input error!" << endl;
				break;
		}												
	}
	return 0;
}  




