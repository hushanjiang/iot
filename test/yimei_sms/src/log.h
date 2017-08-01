#ifndef _LOG_H_CREATE_BY_HELPING_
#define _LOG_H_CREATE_BY_HELPING_

#include<sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h> 
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <linux/unistd.h>
#include <unistd.h>


using namespace std;

namespace MTNMsgApi_NP
{
	
	#define MAXARRLEN 200
	#define MAXBUFLEN 1024
	#define MAXLOGLEN 4096
	
	#define LOGPATH "Path"
	#define LOGFILE "File"
	#define LOGMASK "Mask"
	#define LOGSIZE "Size"
	
	enum enumMessage{M1=0x0001,NT=0x0002,WA=0x0004,ER=0x0008,FA=0x0010,
	                 D1=0x0020,D2=0x0040,D3=0x0080,D4=0x0100,D5=0x0200};  /// 日志等级
	
	
	//-------------------------------------------------------------------------------------------------
	/// <summary> 
	/// 类CLog的定义
	/// </summary>
	//-------------------------------------------------------------------------------------------------
	class CLog{
	public:
		//-------------------------------------------------------------------------------------------------
	    /// <summary> 
	    /// class CLog的构造函数
	    /// </summary> 	
	    //-------------------------------------------------------------------------------------------------
		CLog();   
	
		//-------------------------------------------------------------------------------------------------
	    /// <summary> 
	    /// class CLog的析构函数
	    /// </summary> 	
	    //-------------------------------------------------------------------------------------------------
		~CLog();   
		
		//-------------------------------------------------------------------------------------------------
	    /// <summary> 
		/// 打开一个已经存在的日志文件或者创建一个新的
	    /// </summary> 
		/// <param name=" pLogDir ">
	    /// 传入的日志文件的存放目录
	    /// </param>
	    /// <param name=" pLogPrefix">
	    /// 传入的日志文件名称的前缀
	    /// </param>
		/// <param name=" nMaxFileLen">
	    /// 传入的日志文件的最大长度
	    /// </param>
		/// <param name=" nLevelMask ">
	    /// 传入的日志等级
	    /// </param>
	    /// <returns>
	    /// 成功时返回1，失败时返回0。
	    /// </returns>
	    //-------------------------------------------------------------------------------------------------
		int  openLog(char* pLogDir, char* pLogPrefix, int nMaxFileLen, int nLevelMask);
	
	
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
		int  writeLog(enum enumMessage nLevel, char* pFmt, ...);
	
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
	    int setMask(int nMaskLevel);
	
		//-------------------------------------------------------------------------------------------------
	    /// <summary> 
		/// 设置日志文件最大长度
	    /// </summary> 
		/// <param name=" nMaxlen ">
	    /// 传入的日志文件最大的长度值
	    /// </param>	
		/// <returns>
	    /// 成功时返回1，失败时返回0。
	    /// </returns>
	    //-------------------------------------------------------------------------------------------------
		int setMaxLen(int nMaxLen);
	
		//-------------------------------------------------------------------------------------------------
	    /// <summary> 
		/// 关闭日志文件
	    /// </summary> 
		//-------------------------------------------------------------------------------------------------
		void closeLog();	
	
	private:
		//-------------------------------------------------------------------------------------------------
	    /// <summary> 
		/// 得到格式化的时间表达
	    /// </summary> 
		/// <param name=" pOutBuffer ">
	    /// 传出的用于存放时间的数组
	    /// </param>
	    /// <param name=" nType">
	    /// 传入的类型，不同的类型输出不同的格式：0（YYYY-MM-DD HH24:MI:SS.MS） 1（MMDDMI）
	    /// </param>
		/// <returns>
	    /// 无
	    /// </returns>
	    //-------------------------------------------------------------------------------------------------
		void getTime(char * pOutBuffer,int nType);
	
	private:	
		char          m_arrDir[MAXARRLEN];     /// 日志文件的保存目录（用户配置）
		char          m_arrPrefix[MAXARRLEN];  /// 日志文件名称的前缀（用户配置）
		int           m_nMaxLen;               /// 日志文件的最大长度（用户配置）
		int           m_nMask;                 /// 日志等级（用户配置）	
		
		FILE*         m_pLogFp_MS;             /// 日志文件的文件句柄
		FILE*         m_pLogFp_WR;             /// 日志文件的文件句柄
		
		char          m_arrFname_MS[MAXARRLEN];   /// 日志文件的名称
		char          m_arrFname_WR[MAXARRLEN];   /// 日志文件的名称
		
		int           m_nNeedChange_MS;           /// 日志文件大小超过最大长度时需要切换的标记		
		int           m_nNeedChange_WR;           /// 日志文件大小超过最大长度时需要切换的标记		
		
		int           m_nLogLen_MS;               /// 日志文件的当前大小
		int           m_nLogLen_WR;               /// 日志文件的当前大小
	
		time_t        m_nOpenTime;             /// 日志文件的创建时间
		time_t        m_nLastMod;              /// 日志文件的最后修改时间
		
	#ifndef NO_PTHREAD
		pthread_mutex_t   ms_mutex ;
	#endif
	};
}
#endif





