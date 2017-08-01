#ifndef __SOCKTOOL_H__
#define __SOCKTOOL_H__

#include <sys/socket.h>
#include <errno.h>

//-----------------------------------------------------------------------------
/// <summary>
/// socket工具类
/// </summary>
//-----------------------------------------------------------------------------
// 管道不能使用select判断是否可读和可写

namespace MTNMsgApi_NP{
	
	class CSockTool
	{
	  public:
	  	    
	    // 等待nTimeout毫秒
	    static void wait( int nTimeout );
	
	    // 等待nTimeout微秒
	    static void wait2( int nTimeout );
	    
	    // 设置fd为非阻塞模式
	    static bool setNonBlocking( int fd, bool bNoBlock = true );
	    
	    // 返回socket，返回为非阻塞的
	    // -1 失败，其他socket
	    static int listen( const char* pAddress, int nPort, int nMaxBuf = 200 );
	    
	    // 等待连接，nListenSock是非阻塞的，返回的也为非阻塞的
	    // nTimeout为-1表示永久等待 毫秒
	    // 返回-1 失败，0 超时，>0 成功
	    static int waitConnect( int nListenSock, int nTimeout = -1 );
	    
	    // nListenSock是非阻塞的，返回的也为非阻塞的
	    static int accept( int nListenSock, struct sockaddr* pClientAddr, socklen_t *pClientLen );
	
	    // 建立到服务端的连接，返回的为非阻塞的 毫秒
	    // 当指定了pLocalAddress和nLocalPort时，绑定本地socket到指定的地址和端口
	    static int connect( const char* pServerAddress, int nServerPort, int nTimeout = 10000, const char* pLocalAddress = 0, int nLocalPort = 0 );
	    
	    // 是否可以读入，nSock为非阻塞的 毫秒
	    // 0 超时，-1 错误，>0 可以读
	    static int waitRead( int nSock, int nTimeout = 10000 );
	    
	    // 读非阻塞的socket 毫秒
	    static int read( int nSock, char* pBuf, int nLen, int nTimeout = 10000 );
	    
	    // 是否可以写出，nSock为非阻塞的 毫秒
	    // 0 超时，-1 错误，>0 可以写
	    static int waitWrite( int nSock, int nTimeout = 10000 );
	    
	    // 写非阻塞的socket 毫秒
	    static int write( int nSock, const char* pBuf, int nLen, int nTimeout = 10000 );
	    
	    // shutdown并关闭socket
	    static void close( int nSock );
	};

}//namespace end.

#endif // __SOCKTOOL_H__





