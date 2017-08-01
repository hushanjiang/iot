//---------------------------------------------------------------------------
//    名称：log.cpp
//    功能：CLog类的源文件
//    版权：
//    版本：1.0
//    工具：Visual Studio 2005
//    引入：无
//    引出：无
//    历史：版本1.0。创建
//---------------------------------------------------------------------------

#include "log.h"

//#define NO_PTHREAD

namespace MTNMsgApi_NP
{

CLog::CLog()
{	
	// 初始化类的成员变量		
	m_nMaxLen      = MAXBUFLEN;
	m_nMask        = 0x03FF;
	m_arrDir[0]    = 0;
	m_arrPrefix[0] = 0;	

	m_pLogFp_MS       = NULL;
	m_pLogFp_WR       = NULL;
	m_nLogLen_MS      = 0; 
	m_nLogLen_WR      = 0; 
	m_nNeedChange_MS  = 0;
	m_nNeedChange_WR  = 0;
	m_arrFname_MS[0]  = 0;
	m_arrFname_WR[0]  = 0;
	
	m_nOpenTime    = 0;
	m_nLastMod     = 0;
#ifndef NO_PTHREAD
	pthread_mutex_init(&ms_mutex,NULL);	
#endif
	// 结束
}

CLog::~CLog()
{
	if(m_pLogFp_MS)
		fclose(m_pLogFp_MS);
	m_pLogFp_MS = NULL;

	if(m_pLogFp_WR)
		fclose(m_pLogFp_WR);
	m_pLogFp_WR = NULL;

#ifndef NO_PTHREAD
	pthread_mutex_destroy(&ms_mutex);
#endif
}

//-------------------------------------------------------------------------------------------------
//    名称：CLog::openLog()
//    功能：打开日志，若不存在则创建日志
//    输入：1. char* pLogDir        ( 传入的日志文件的保存路径 )
//          2. char* pLogPrefix     ( 传入的日志文件的前缀 )
//          3. int nMaxFileLen     ( 传入的日志文件的最大长度)
//          4. int nLevelMask       ( 传入的日志等级 )//          
//    输出：int
//          1. 1(操作成功)
//          2. 0(操作失败)
//    版本：1.0
// ---------------------------------------------------------------------------------------------
int CLog::openLog(char* pLogDir, char* pLogPrefix, int nMaxFileLen, int nLevelMask)
{	
	
	/*接收参数*/
	if(pLogDir != NULL)
		strcpy(m_arrDir,pLogDir);

	if(pLogPrefix != NULL)
		strcpy(m_arrPrefix,pLogPrefix);
	else
	{
		printf("filename can not be null!\n");
		return 0;
	}

	m_nMaxLen  = nMaxFileLen;
	m_nMask    = nLevelMask;    
    /*END 接收参数*/

	/*设置两个文件的名称*/
	int nDirLen;                        /// 目录长度
	char arrFileName_MS[MAXBUFLEN];     /// 用于存放文件名的数组	
	char arrFileName_WR[MAXARRLEN];      /// 用于存放当前时间的数组	
	arrFileName_MS[0] = 0;
	arrFileName_WR[0] = 0;		

	
	if((nLevelMask & 0x03FF) == 0)
	{
		printf("this level is not exist , please check the level_mask you set\n");
		return 0;
	}
	
	if(pLogDir && strlen(pLogDir)>0)
	{
		nDirLen = strlen(pLogDir);		
		if(pLogDir[nDirLen-1] == '/')
		{
			snprintf(arrFileName_MS,MAXBUFLEN-1,"%sMS_%s.txt",m_arrDir,m_arrPrefix);
			snprintf(arrFileName_WR,MAXBUFLEN-1,"%sWR_%s.txt",m_arrDir,m_arrPrefix);
		}
		else
		{
			snprintf(arrFileName_MS,MAXBUFLEN-1,"%s/MS_%s.txt",m_arrDir,m_arrPrefix);
			snprintf(arrFileName_WR,MAXBUFLEN-1,"%s/WR_%s.txt",m_arrDir,m_arrPrefix);
		}
	}
	else if(!pLogDir || strlen(pLogDir) == 0)
	{
		snprintf(arrFileName_MS,MAXBUFLEN-1,"MS_%s.txt",m_arrPrefix);
		snprintf(arrFileName_WR,MAXBUFLEN-1,"WR_%s.txt",m_arrPrefix);
	}
	
	strcpy(m_arrFname_MS, arrFileName_MS);
	strcpy(m_arrFname_WR, arrFileName_WR);
    /*END 设置两个文件的名称*/

	/*打开两个文件*/
	if(m_pLogFp_MS == NULL)
	{
		m_pLogFp_MS = fopen(arrFileName_MS,"r+");		
		if(!m_pLogFp_MS)
		{
			m_pLogFp_MS = fopen(arrFileName_MS,"w+");	
			if(!m_pLogFp_MS)
			{
				printf("open file : %s failed!!\n",arrFileName_MS);
				m_pLogFp_MS = NULL;
				return 0;
			}
			
			fseek(m_pLogFp_MS,0,SEEK_END);
			m_nLogLen_MS = ftell(m_pLogFp_MS);
			//return 1; 
		}
		else
		{
			fseek(m_pLogFp_MS,0,SEEK_END);
			m_nLogLen_MS = ftell(m_pLogFp_MS);
			//return 1; 
		}
	}
	

	if(m_pLogFp_WR == NULL)
	{
		m_pLogFp_WR = fopen(arrFileName_WR,"r+");		
		if(!m_pLogFp_WR)
		{
			m_pLogFp_WR = fopen(arrFileName_WR,"w+");	
			if(!m_pLogFp_WR)
			{
				printf("open file : %s failed!!\n",arrFileName_WR);
				m_pLogFp_WR = NULL;
				return 0;
			}
			
			fseek(m_pLogFp_WR,0,SEEK_END);
			m_nLogLen_WR = ftell(m_pLogFp_WR);
			//return 1; 
		}
		else
		{
			fseek(m_pLogFp_WR,0,SEEK_END);
			m_nLogLen_WR = ftell(m_pLogFp_WR);
			//return 1; 
		}	
	}

	/*END 打开两个文件*/	
	return 1; 
}

//-------------------------------------------------------------------------------------------------
//    名称：CLog::writeLog()
//    功能：写日志，若需要切换（大小超过最大长度时），重新创建文件
//    输入：1. int nLevel            ( 传入的日志等级)
//          2. char* pFmt            ( 传入的格式化信息)
//    输出：int
//          1. 1(操作成功)
//          2. 0(操作失败)
//    版本：1.0
// ---------------------------------------------------------------------------------------------
//    以下为用于自动生成文档的XML格式说明 ：
// ---------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// <summary> 
/// 写日志到日志文件中，若需要切换，重开一个文件再写
/// </summary> 
/// <param name=" nLevel ">
/// 传入的日志等级
/// </param>
/// <param name=" pFmt">
/// 传入的自定义格式的信息
/// </param>
/// <returns>
/// 成功时返回1，失败时返回0。
/// </returns>
//-------------------------------------------------------------------------------------------------
int CLog::writeLog( enum enumMessage nLevel, char* pFmt, ...)
{
	if(!(nLevel & m_nMask))
		return 0;

	bool bM1 = false;
	//FILE *pTry = NULL;
	int nDirLen = strlen(m_arrDir);	
	char arrTimeRec[MAXARRLEN];      /// 用于记录时间的数组	
	char arrTimeRec1[MAXARRLEN];
	char arrMessage[MAXARRLEN];      /// 记录日志等级信息的数组	
	char arrBuffer[MAXLOGLEN];       /// 记录输出日志信息的数组
	char arrFmat[MAXLOGLEN];         /// 接收格式化参数的数组
	char arrTimeNewFile[MAXARRLEN];  /// 需要修改名称时对应的时间 
	char arrNewFileName[MAXARRLEN];  /// 修改后的文件名
	//char arrNewFileName_WR[MAXARRLEN];  /// 修改后的文件名
	arrFmat[0]         = 0;
	arrTimeRec[0]      = 0;
	arrTimeRec1[0]     = 0;
	arrMessage[0]      = 0;
	arrBuffer[0]       = 0;
	arrTimeNewFile[0]  = 0;
	arrNewFileName[0]  = 0;	
	//arrNewFileName_WR[0]  = 0;	

	switch(nLevel)
	{
		case M1:
			strcpy(arrMessage,"M1");
			break;
		case NT:
			strcpy(arrMessage,"NT");
			break;
		case WA:
			strcpy(arrMessage,"WA");
			break;
		case ER:
			strcpy(arrMessage,"ER");
			break;
		case FA:
			strcpy(arrMessage,"FA");
			break;
		case D1:
			strcpy(arrMessage,"D1");
			break;
		case D2:
			strcpy(arrMessage,"D2");
			break;
		case D3:
			strcpy(arrMessage,"D3");
			break;
		case D4:
			strcpy(arrMessage,"D4");
			break;
		case D5:
			strcpy(arrMessage,"D5");
			break;		
		default:
			return 0;
	}  	

	if(nLevel == M1)
		bM1 = true;

	//实现可变参数		
	va_list argptr;
	va_start(argptr, pFmt);
	vsnprintf(arrFmat,MAXLOGLEN-1,pFmt,argptr);
	va_end(argptr);
	//结束	

	getTime(arrTimeRec,0); 
	getTime(arrTimeRec1,2);

	if(!bM1)
		snprintf(arrBuffer,MAXLOGLEN-1,"%s %s %d.%u %s\n",
		     arrTimeRec, arrMessage,getpid(),(unsigned int)pthread_self(),arrFmat);
	else
		snprintf(arrBuffer,MAXLOGLEN-1,"%s %s %s\n",
		     arrTimeRec1, arrMessage,arrFmat);

	
	int nBufferLength = strlen(arrBuffer);
	

	// 如果文件已经到达最大长度，则重新开一个文件写，否则直接写入到文件中

	int nLogLenNow = 0;
	
#ifndef  NO_PTHREAD
		pthread_mutex_lock (&ms_mutex);
#endif

		if(bM1)
			nLogLenNow = m_nLogLen_MS;
		else
			nLogLenNow = m_nLogLen_WR;

		if(nLogLenNow+nBufferLength > m_nMaxLen)
		{
			if(bM1)
			{
				if(m_pLogFp_MS)
					fclose(m_pLogFp_MS);
				m_pLogFp_MS = NULL;
			}
			else
			{
				if(m_pLogFp_WR)
					fclose(m_pLogFp_WR);
				m_pLogFp_WR = NULL;
			}

			if(bM1)
				m_nNeedChange_MS ++;
			else
				m_nNeedChange_WR ++;

			getTime(arrTimeNewFile,1);
			
			if(nDirLen > 0)
			{
				if(m_arrDir[nDirLen-1] == '/')
				{
					if(bM1)
						snprintf(arrNewFileName,MAXARRLEN-1,"%sMS_%s_%s_%d.txt",
						    m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_MS);
					else
						snprintf(arrNewFileName,MAXARRLEN-1,"%sWR_%s_%s_%d.txt",
						    m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_WR);
				}
				else
				{
					if(bM1)
						snprintf(arrNewFileName,MAXARRLEN-1,"%s/MS_%s_%s_%d.txt",
						   m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_MS);
					else
						snprintf(arrNewFileName,MAXARRLEN-1,"%s/WR_%s_%s_%d.txt",
						   m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_WR);
				}
			}
			else
			{
				if(bM1)
					snprintf(arrNewFileName,MAXARRLEN-1,"MS_%s_%s_%d.txt",
						  m_arrPrefix,arrTimeNewFile,m_nNeedChange_MS);
				else
					snprintf(arrNewFileName,MAXARRLEN-1,"WR_%s_%s_%d.txt",
						  m_arrPrefix,arrTimeNewFile,m_nNeedChange_WR);
			}


			//pTry = NULL;
			while(1)
			{
				// 重命名后文件若已经存在，再改名
				//if( (pTry = fopen(arrNewFileName,"r") ) != NULL)  
				if( access(arrNewFileName,F_OK)  != -1)  
				{
					if(bM1)
						m_nNeedChange_MS ++;
					else 
						m_nNeedChange_WR ++;
					
					if(nDirLen > 0)
					{
						if(m_arrDir[nDirLen-1] == '/')
						{
							if(bM1)
								snprintf(arrNewFileName,MAXARRLEN-1,"%sMS_%s_%s_%d.txt",
								m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_MS);
							else
								snprintf(arrNewFileName,MAXARRLEN-1,"%sWR_%s_%s_%d.txt",
								m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_WR);
						}
						else
						{
							if(bM1)
								snprintf(arrNewFileName,MAXARRLEN-1,"%s/MS_%s_%s_%d.txt",
								m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_MS);
							else
								snprintf(arrNewFileName,MAXARRLEN-1,"%s/WR_%s_%s_%d.txt",
								m_arrDir,m_arrPrefix,arrTimeNewFile,m_nNeedChange_WR);
						}
					}
					else
					{
						if(bM1)
							snprintf(arrNewFileName,MAXARRLEN-1,"MS_%s_%s_%d.txt",
							m_arrPrefix,arrTimeNewFile,m_nNeedChange_MS);
						else
							snprintf(arrNewFileName,MAXARRLEN-1,"WR_%s_%s_%d.txt",
							m_arrPrefix,arrTimeNewFile,m_nNeedChange_WR);

					}	
					
                    //fclose(pTry);
					//pTry = NULL;

				}
				else
				{						
					break;
				}
				// END 重命名后文件若已经存在，再改名
			}
			


			//  重命名文件
			char  arrFnameNow[MAXARRLEN];
			arrFnameNow[0] = 0;

			if(bM1)
				strcpy(arrFnameNow,m_arrFname_MS);
			else
				strcpy(arrFnameNow,m_arrFname_WR);

			if(0!=rename(arrFnameNow,arrNewFileName))
			{
#ifndef  NO_PTHREAD	
				pthread_mutex_unlock (&ms_mutex);
#endif
				return 0;	
			}
			// end 重命名文件
			
			if(openLog(m_arrDir,m_arrPrefix,m_nMaxLen,m_nMask)==0)
			{		  
#ifndef  NO_PTHREAD	
				pthread_mutex_unlock (&ms_mutex);
#endif
				return 0;
			}
		}

		
	//输出到屏幕
	if( (bM1 && !m_pLogFp_MS) || (!bM1 && !m_pLogFp_WR) )
	{
		if(nLevel==WA || nLevel==FA || nLevel==ER)
			cerr<<arrBuffer;
		else
			cout<<arrBuffer;

#ifndef  NO_PTHREAD	
		pthread_mutex_unlock (&ms_mutex);
#endif
		return 1;
	}
	//end 输出到屏幕

	if(bM1)
	{
		fseek(m_pLogFp_MS,0,SEEK_END);
		if(0==fwrite(arrBuffer,strlen(arrBuffer),1,m_pLogFp_MS))
		{

#ifndef  NO_PTHREAD
			pthread_mutex_unlock (&ms_mutex);
#endif
			printf("write information to file failed!\n");
			return 0;			
		}

		fflush(m_pLogFp_MS);
		m_nLogLen_MS += nBufferLength;

	}
	else
	{
		fseek(m_pLogFp_WR,0,SEEK_END);
		if(0==fwrite(arrBuffer,strlen(arrBuffer),1,m_pLogFp_WR))
		{

#ifndef  NO_PTHREAD
			pthread_mutex_unlock (&ms_mutex);
#endif
			printf("write information to file failed!\n");
			return 0;			
		}

		fflush(m_pLogFp_WR);
		m_nLogLen_WR += nBufferLength;

	}

#ifndef  NO_PTHREAD	
	pthread_mutex_unlock (&ms_mutex);
#endif
	
	return 1;	
}

//-------------------------------------------------------------------------------------------------
//    名称：CLog::setMask()
//    功能：设置掩码
//    输入：1. int nMaskLevel （传入的掩码）          
//    输出：int ：  1（成功） 0（失败）
//    版本：1.0
// ---------------------------------------------------------------------------------------------
//    以下为用于自动生成文档的XML格式说明 ：
// ---------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// <summary> 
/// 设置掩码
/// </summary> 
/// <param name=" nMaskLevel ">
/// 传入的掩码
/// </param>	
/// <returns>
/// 成功时返回1，失败时返回0。
/// </returns>
//-------------------------------------------------------------------------------------------------
int CLog::setMask(int nMaskLevel)
{
	m_nMask = nMaskLevel;
	return 1;
}

//-------------------------------------------------------------------------------------------------
//    名称：CLog::setMaxLen()
//    功能：设置日志文件最大长度
//    输入：1. int nMaxLen （日志文件最大长度）          
//    输出：int ：  1（成功） 0（失败）
//    版本：1.0
// ---------------------------------------------------------------------------------------------
//    以下为用于自动生成文档的XML格式说明 ：
// ---------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// <summary> 
/// 设置日志文件最大长度
/// </summary> 
/// <param name=" nMaxLen ">
/// 传入的日志文件最大的长度值
/// </param>	
/// <returns>
/// 成功时返回1，失败时返回0。
/// </returns>
//-------------------------------------------------------------------------------------------------
int CLog::setMaxLen(int nMaxLen)
{
	m_nMaxLen = nMaxLen;
	return 1;
}

//-------------------------------------------------------------------------------------------------
//    名称：CLog::closeLog()
//    功能：关闭日志
//    输入：无          
//    输出：无
//    版本：1.0
// ---------------------------------------------------------------------------------------------
//    以下为用于自动生成文档的XML格式说明 ：
// ---------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// <summary> 
/// 关闭日志文件
/// </summary> 
//-------------------------------------------------------------------------------------------------
void CLog::closeLog()
{
	if(m_pLogFp_MS)
		fclose(m_pLogFp_MS);
	m_pLogFp_MS = NULL;

	if(m_pLogFp_WR)
		fclose(m_pLogFp_WR);
	m_pLogFp_WR = NULL;
}


//-------------------------------------------------------------------------------------------------
//    名称：CLog::getTime()
//    功能：根据参数返回不同的格式化时间表达
//    输入：1. char * pOutBuffer ( 传出的存放时间表达式的数组 )
//          2. int nType         ( 传入的时间表达式类型 )
//    输出：无 
//    版本：1.0
// ---------------------------------------------------------------------------------------------
//    以下为用于自动生成文档的XML格式说明 ：
// ---------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// <summary> 
/// 得到格式化的时间表达
/// </summary> 
/// <param name=" pOutBuffer ">
/// 传出的用于存放时间的数组
/// </param>
/// <param name=" nType">
/// 传入的类型，不同的类型输出不同的格式：0（MM-DD HH24:MI:SS.MS） 1（MMDDMI）
/// </param>
/// <returns>
/// 无
/// </returns>
//-------------------------------------------------------------------------------------------------
void CLog::getTime(char * pOutBuffer,int nType)
{
	time_t   nTimeP;   
	struct   tm   *pTm;   
	time(&nTimeP);   
	pTm=localtime(&nTimeP);
    
	// 根据type得到不同格式化的时间表达
	if(nType == 0) 
		snprintf(pOutBuffer,MAXARRLEN-1,"%02d-%02d %02d:%02d:%02d",1+pTm->tm_mon,pTm->tm_mday,pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
	else if(nType == 1) 
		snprintf(pOutBuffer,MAXARRLEN-1,"%02d%02d%02d",1+pTm->tm_mon,pTm->tm_mday,pTm->tm_hour);
	else if(nType == 2) 
		snprintf(pOutBuffer,MAXARRLEN-1,"%04d-%02d-%02d %02d:%02d:%02d", pTm->tm_year+1900,1+pTm->tm_mon,pTm->tm_mday,pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
	// 结束
}

}//namespace end




