
/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: cuipingxu918@qq.com
 */
 

#ifndef _BASE_CURL_H
#define _BASE_CURL_H

#include "base_common.h"

NS_BASE_BEGIN

struct StMail
{
	std::string _addresser; //寄件人邮箱
	std::string _recipients;  //收件人邮箱列表
	std::string _CC;  //抄送
	std::string _BCC; //密送
	std::string _subject; //主题
	std::string _content; //内容
	bool _complete;

	StMail()
	{
		_addresser = "";
		_recipients = "";
		_CC = "";
		_BCC = "";
		_subject = "";
		_content = "";
		_complete = false;
	}
	
};


class Curl_Tool
{
public:
	
	static int http_get(const std::string &url, std::string &rsp, unsigned int conn_timeout=3, unsigned int rw_timeout=3);

	static int http_post_msg(const std::string &url, const std::string &msg, std::string &rsp, unsigned int conn_timeout=3, unsigned int rw_timeout=3);

	static int smtp(const std::string &mailserver, unsigned short port,  
		const std::string &user, const std::string &pwd, 
		const std::string &addresser, std::vector<std::string> &recipients, std::vector<std::string> &CC, 
		const std::string &subject, const std::string &content);

	static int http_download(const std::string &url, const std::string &file);

	//临时使用
	static int http_get_tmp(const std::string &url, std::string &rsp, const std::string &real_ip, unsigned int conn_timeout=3, unsigned int rw_timeout=3);

};

NS_BASE_END

#endif


