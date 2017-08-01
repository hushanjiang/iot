
#include "base_curl.h"
#include "base_time.h"
#include "curl/curl.h"

NS_BASE_BEGIN

/*
This function gets called by libcurl as soon as there is data received that needs to be saved.
The size of the data pointed to by ptr is size multiplied with nmemb, it will not be zero terminated.
ptr:  接收到的数据所在缓冲区
size: 数据元素的字节数(一般都是1字节)
nmemb: 数据元素个数
userdata: 用户自定义指针

总接收数据长度 = size * nmemb
*/
size_t writer_file( char *ptr, size_t size, size_t nmemb, void *userdata)
{
	FILE *fp = (FILE *)userdata;
	size_t ret_size = fwrite(ptr, size, nmemb, fp);
	
	printf("receive msg(size:%d):\n%s\n", (int)ret_size, ptr);
	
	return ret_size; //same to size * nmemb;
	
}


size_t writer_data( char *ptr, size_t size, size_t nmemb, std::string *userdata)
{
	//printf("receive msg(size:%d):\n%s\n", (int)size, ptr);
	
	if (userdata == NULL)
	{
		return 0;
	}

	userdata->append(ptr, size * nmemb);

	return size * nmemb;
	
}



size_t send_mail( char *ptr, size_t size, size_t nmemb, void *userdata)
{
	if (userdata == NULL)
	{
		return 0;
	}

	StMail *stMail = (StMail*)userdata;
	if(!(stMail->_complete))
	{
		snprintf(ptr, nmemb, 
			"Date: %s\r\n"
			"To: %s\r\n"
			"From: %s\r\n"
			"CC: %s\r\n"
			"Subject: %s\r\n"
			"MIME-Version: 1.0\r\n"
			"Content-Type: text/html; Charset=UTF-8\r\n"
			"\r\n"  //empty line to divide headers from body, see RFC5322
			"%s\r\n",
			FormatDateTimeStr().c_str(),
			stMail->_recipients.c_str(),
			stMail->_addresser.c_str(),
			stMail->_CC.c_str(),
			stMail->_subject.c_str(),
			stMail->_content.c_str());

		stMail->_complete = true;
		return strlen(ptr);
		
	}
	
	return 0;
	
}


int Curl_Tool::http_get(const std::string &url, std::string &rsp, unsigned int conn_timeout, unsigned int rw_timeout)
{
	int nRet = 0;

	printf("Curl_Tool::http_get, url:%s, conn_timeout:%u, rw_timeout:%u\n", url.c_str(), conn_timeout, rw_timeout);
	
	CURL *curl = NULL;
	curl = curl_easy_init();
	if(!curl)
	{
		printf("curl_easy_init failed, url(%u):%s\n", url.size(), url.c_str());
		return -1;
	}

	//CURLOPT_HEADER:	输出完整的 http response 报，无http request 报
	//CURLOPT_VERBOSE	打印调试信息， 包括 http request 和http response 信息
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, conn_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, rw_timeout);

	// NOTE: It must be set, otherwise it will crash when timeout 
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	nRet = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writer_data);
	if (nRet != CURLE_OK) 
	{
		curl_easy_cleanup(curl);
		printf("curl_easy_setopt CURLOPT_WRITEFUNCTION failed, ret:%d, url(%u):%s\n", 
			nRet, url.size(), url.c_str());
		return -2;
	}

	nRet = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rsp);
	if (nRet != CURLE_OK) 
	{
		curl_easy_cleanup(curl);
		printf("curl_easy_setopt CURLOPT_WRITEDATA failed, ret:%d, url(%u):%s\n", 
		nRet, url.size(), url.c_str());		
		return -3;
	}

	nRet = curl_easy_perform(curl);
	if(nRet != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		printf("curl_easy_perform get failed, ret:%d, url(%u):%s\n", nRet, url.size(), url.c_str());
		return -4;
	}

	//获取http rsp status 信息
	int http_status_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
	if (http_status_code != 200) 
	{	
		curl_easy_cleanup(curl);
		printf("http rsp isn't correct, http_status_code:%d, url(%u):%s\n", 
			http_status_code, url.size(), url.c_str());
		return -5;
	}

	if (rsp.empty()) 
	{
		printf("rsp is empty, url(%u):%s\n", url.size(), url.c_str());
	}

	curl_easy_cleanup(curl);

	return nRet;

}



int Curl_Tool::http_post_msg(const std::string &url, const std::string &msg, std::string &rsp, unsigned int conn_timeout, unsigned int rw_timeout)
{
	int nRet = 0;
	
	CURL *curl = NULL;	
	curl = curl_easy_init();
	if(!curl) 
	{
		printf("curl_easy_init failed.\n");
		return -1;
	}
	
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
	//如果CURLOPT_NOBODY 设置1，http resonse 只打印head， 不打印body信息
	//curl_easy_setopt(curl, CURLOPT_NOBODY, 0);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, conn_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, rw_timeout);
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

	//CURLOPT_POSTFIELDS设置 post 请求包的body信息
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	//设置这个选项libcurl不再使用任何可能产生singal的函数。主要应用在多线程unix app 中
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	nRet = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writer_data);
	if (nRet != CURLE_OK) 
	{
		curl_easy_cleanup(curl);
		printf("curl_easy_setopt CURLOPT_WRITEFUNCTION failed, ret:%d, url(%u):%s\n", 
			nRet, url.size(), url.c_str());
		return -2;
	}

	nRet = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rsp);
	if (nRet != CURLE_OK) 
	{
		curl_easy_cleanup(curl);
		printf("curl_easy_setopt CURLOPT_WRITEDATA failed, ret:%d, url(%u):%s\n", 
		nRet, url.size(), url.c_str());		
		return -3;
	}
	
	//curl_slist_append 负责首部字段添加删除
	struct curl_slist *chunk = NULL;
	//chunk = curl_slist_append(chunk, "Content-Type: multipart/form-data");
	chunk = curl_slist_append(chunk, "Content-Type: application/x-www-form-urlencoded");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg.c_str());	
	
	nRet = curl_easy_perform(curl);
	if(nRet != CURLE_OK)
	{
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		printf("curl_easy_perform post failed, ret:%d, url(%u):%s\n", nRet, url.size(), url.c_str());
		return -4;
	}

	//获取http rsp status 信息
	int http_status_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
	if (http_status_code != 200) 
	{	
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		printf("http rsp isn't correct, http_status_code:%d, url(%u):%s\n", 
			http_status_code, url.size(), url.c_str());
		return -5;
	}

	if (rsp.empty()) 
	{
		printf("rsp is empty, url(%u):%s\n", url.size(), url.c_str());
	}
	
	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);
	
	return nRet;

}



int Curl_Tool::smtp(const std::string &mailserver, unsigned short port,  
		const std::string &user, const std::string &pwd, 
		const std::string &addresser, std::vector<std::string> &recipients, std::vector<std::string> &CC, 
		const std::string &subject, const std::string &content)
{
	int nRet = 0;

	CURL *curl = NULL;	
	curl = curl_easy_init();
	if(!curl) 
	{
		printf("curl_easy_init failed.\n");
		return -1;
	}

	std::vector<std::string> recipients_new(recipients.size()+CC.size()); 
	std::merge(recipients.begin(), recipients.end(), CC.begin(), CC.end(), recipients_new.begin());
	
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	/* NOTE: It must be set, otherwise it will crash when timeout */
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_URL, mailserver.c_str());  //smtp server url

	if(port != 0)
	{
		curl_easy_setopt(curl, CURLOPT_PORT, port);  //smtp server port
	}
	
	curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str()); //发送人名称
	curl_easy_setopt(curl, CURLOPT_PASSWORD, pwd.c_str());    //发送邮箱密码
	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, addresser.c_str());  //发送人邮箱

	//接收者
	struct curl_slist *slist = NULL;
	for(unsigned int i=0; i<recipients_new.size(); i++)
	{
		slist = curl_slist_append(slist, recipients_new[i].c_str());
	}
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, slist);  //收件人邮箱	

	//构造StMail 对象
	StMail stMail;
	stMail._addresser = addresser;

	bool first = true;
	std::string recipients_list = "";
	for(unsigned int i=0; i<recipients.size(); i++)
	{
		if(first)
		{
			recipients_list = recipients[i];
			first = false;
			continue;
		}
		
		recipients_list += (std::string(",") + recipients[i]);
	}	
	stMail._recipients = recipients_list;

	first = true;
	std::string CC_list = "";
	for(unsigned int i=0; i<CC.size(); i++)
	{
		if(first)
		{
			CC_list = CC[i];
			first = false;
			continue;
		}
			
		CC_list += (std::string(",") + CC[i]);
	}
	stMail._CC = CC_list;
	
	stMail._subject = subject;
	stMail._content = content;
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, send_mail);
	curl_easy_setopt(curl, CURLOPT_READDATA, &stMail);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	
	nRet = curl_easy_perform(curl);
	if(nRet != CURLE_OK)
	{
		printf("curl_easy_perform failed, ret:%d\n", nRet);
		nRet = -1;
	}
	else
	{
		printf("smtp success.\n");
	}
	
	curl_slist_free_all(slist);
	curl_easy_cleanup(curl);
		
	
	return nRet;

}



/*
注意: 通过异步回调下载文件场景，
如果文件不存在，需要进行处理。因为writer是可以将buf 填充404 not found等网页内容的，
不能将这个内容当成文件内容，所以需要判断http web返回来的code，进行判断。
*/
int Curl_Tool::http_download(const std::string &url, const std::string &file)
{
	int nRet = 0;
	
	CURL *curl = NULL;
	curl = curl_easy_init();
	if(!curl) 
	{
		printf("curl_easy_init failed.\n");
		return -1;
	}
	
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1L);	
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	
	/* NOTE: It must be set, otherwise it will crash when timeout */
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writer_file);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  //支持重定向

	/*
	允许将TIME-WAIT sockets重新用于新的TCP连接
	*/
	curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
	
	FILE *fp = fopen(file.c_str(), "w+");
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	nRet = curl_easy_perform(curl);
	if(nRet != CURLE_OK)
	{
		printf("curl_easy_perform failed, ret:%d\n", nRet);
		nRet = -1;
	}
	else
	{
		printf("curl_easy_perform(%s) success, ret:%d\n", url.c_str(), nRet);
	}
	
	fclose(fp);
	curl_easy_cleanup(curl);

	return nRet;
	
}




int Curl_Tool::http_get_tmp(const std::string &url, std::string &rsp, const std::string &real_ip, unsigned int conn_timeout, unsigned int rw_timeout)
{
	int nRet = 0;

	printf("Curl_Tool::http_get, url:%s, conn_timeout:%u, rw_timeout:%u\n", url.c_str(), conn_timeout, rw_timeout);
	
	CURL *curl = NULL;
	curl = curl_easy_init();
	if(!curl)
	{
		printf("curl_easy_init failed, url(%u):%s\n", url.size(), url.c_str());
		return -1;
	}

	//CURLOPT_HEADER:	输出完整的 http response 报，无http request 报
	//CURLOPT_VERBOSE	打印调试信息， 包括 http request 和http response 信息
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, conn_timeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, rw_timeout);

	// NOTE: It must be set, otherwise it will crash when timeout 
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

	nRet = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writer_data);
	if (nRet != CURLE_OK) 
	{
		curl_easy_cleanup(curl);
		printf("curl_easy_setopt CURLOPT_WRITEFUNCTION failed, ret:%d, url(%u):%s\n", 
			nRet, url.size(), url.c_str());
		return -2;
	}

	nRet = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rsp);
	if (nRet != CURLE_OK) 
	{
		curl_easy_cleanup(curl);
		printf("curl_easy_setopt CURLOPT_WRITEDATA failed, ret:%d, url(%u):%s\n", 
		nRet, url.size(), url.c_str());		
		return -3;
	}

	//curl_slist_append 负责首部字段添加删除
	struct curl_slist *chunk = NULL;
	char out_ip[100] = {0};
	snprintf(out_ip, 100, "X-Forwarded-For: %s", real_ip.c_str());
	//std::string tmp = std::string("X-Forwarded-For: ") + real_ip;
	//chunk = curl_slist_append(chunk, tmp.c_str());
	chunk = curl_slist_append(chunk, out_ip);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	nRet = curl_easy_perform(curl);
	if(nRet != CURLE_OK)
	{
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		printf("curl_easy_perform get failed, ret:%d, url(%u):%s\n", nRet, url.size(), url.c_str());
		return -4;
	}

	//获取http rsp status 信息
	int http_status_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
	if (http_status_code != 200) 
	{	
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		printf("http rsp isn't correct, http_status_code:%d, url(%u):%s\n", 
			http_status_code, url.size(), url.c_str());
		return -5;
	}

	if (rsp.empty()) 
	{
		printf("rsp is empty, url(%u):%s\n", url.size(), url.c_str());
	}

	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);

	return nRet;

}


NS_BASE_END


