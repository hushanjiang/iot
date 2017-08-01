
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
 * Author: xucuiping(89617663@qq.com)
 */
 

#ifndef _BASE_CRYPTOPP_H
#define _BASE_CRYPTOPP_H

#include "base_common.h"


/*
由于加密解密算法，密钥长度和数据块长度都是固定的，但是在实际
应用中明文的长度是不确定的，而且会远大于加密解密算法中数据块
的固定长度， 所以这个时候就需要使用一定的模式来处理这些情况。
就出现了现在的几种加密解密模式， 比如： ECB，CBC，CFB，OFB。这些
模式提供分块，缓存，补充字节，加密解密等功能， 提供开发效率。


(1)ECB (Electronic Code Book:电码本) 
----------------------------------------
ECB是最简单的模式，同样的明文分组总是加密成相同的密文分组。
这对于发送使用频率不高的明文分组是非常好的；
但对于发送频率较高明文分组不是很好，因为相同的明文会加密成
同样的密文被多次发送， 存在一定的安全隐患。
ECB最大的弱点是对每一个块用相同的方式进行加密。如果我们的密钥
或者数据不断发生变化，ECB是完全安全的。
但是如果经过同样的密钥加密发出以后，攻击者可能获得一些我们
并不想让别人知道的信息。


(2)CBC (Cipher Block Chaining:密码分组链接)
----------------------------------------
CBC模式改变了加密方式，同样的明文分组不一定加密或解密同样的
密文块，因此解决了ECB存在的主要问题。
CBC使用前一分组的信息加密当前分组。因此和ECB模式大不相同。
这个方法依然存在问题，那就是相同的信息
仍然加密成相同的密文，因为所有的分组是同时变成密文分组的。
为了解决这个问题，引入一个IV(Initialization Vector，初始化向量)，也就是前不久有人问到的IV问题。IV仅仅是
一个初始化加密程序的随机数。
它无需秘密保存，但对每一个信息来说它都是不同的，通过这个方式，
即使有两条相同的信息，只要他们有不同的IV
，那么他们加密后的密文也是不同的。从这个意义上来说，初始化
向量无疑就和口令加密过程中使用的加盐值是一样的。
CBC很适合文本传输，但它每一次都需要传送一个完整的数据块，
一般选8个字符。
但是该加密模式的缺点是不能实时解密，也就是说，必须等到每8个
字节都接受到之后才能开始加密，否则就不能得到正确的结果。
这在要求实时性比较高的时候就显得不合适了。

【分析】
加密过程:  明文块 XOR  IV  ---> 加密---> 密文
解密过程:  密文---> 解密---> XOR 前一个输入块 --->  明文
由于解密后又异或操作了一次， 相同数字异或XOR操作后为0x0，
0x0再和源数据块再异或结果还是源数据块



(3)CFB (Cipher FeedBack:密码反馈)
----------------------------------------
CFB的工作方式与CBC类似，但它可以执行更小的数据块，典型的有8位，
这非常适合加密像聊天对话这样的信息，
因为每次可以发送单一的字节数据块。和CBC一样，CFB也需要一个IV，
且相同及钥发送的每条信息的IV都必须
是唯一的。


(4)OFB (Output FeedBack:输出反馈)
-----------------------------------------
OFB除了在传输中能给数据提供更好的保护，
防止数据丢失外，其他和CFB类似。
密文中一位出错，也只造成明文中的一位出错，其他的方式会造成
整个块丢失。和CBC以及CFB一样，OFB也需要一个IV。

*/



NS_BASE_BEGIN


/*
对MD5算法简要的叙述可以为：MD5以512位分组来处理输入的信息，
且每一分组又被划分为16个32位子分组，首先先和初始的32位分组经过了
一系列的处理后输出四个32位分组，再将这四个32位分组与其他的512位
分组继续计算，最后将生成一个128(16字节)位散列值。
*/
class X_MD5
{
public:
	X_MD5();
	
	~X_MD5();

	static void info();
	
	bool calculate_digest(const std::string src, std::string &digest);	
};


//------------------------------------------


/*
与MD5 算法类似，也是对512 bit数据块进过计算
产生256bit (32字节)的摘要， 再和下一个512 bit 数
据块进行计算最终产生sha摘要。
sha目前有sha-1 sha-2 sha-3 标准
sha-2系列包括: sha-224, sha-256, sha-384, sha-512
后面的数字表示生成的摘要长度
*/
class X_SHA256
{
public:
	X_SHA256();
	
	~X_SHA256();

	static void info();
	
	bool calculate_digest(const std::string src, std::string &digest);
	
};


//------------------------------------------


class X_SHA1
{
public:
	X_SHA1();
	
	~X_SHA1();

	static void info();
	
	bool calculate_digest(const std::string src, std::string &digest);

	bool calculate_digest(const std::string src, char *digest);
	
};


//------------------------------------------


class X_HMACSHA1
{
public:
	X_HMACSHA1();
	
	~X_HMACSHA1();

	static void info();
	
	bool calculate_digest(const std::string key, const std::string src, std::string &digest);
	
};



//------------------------------------------


class X_BASE64
{
public:
	X_BASE64();
	
	~X_BASE64();

	static void info();

	bool encrypt(const char *pSrc, const unsigned int len, char* &pDst);

	bool decrypt(const char *pSrc, char* &pDst);
	
};



//------------------------------------------



/*
AES 可以使用128 bit (16 byte)、192 bit (24 byte)、256 bit (32 byte) 密钥
只能加密128 bit (16 byte)的数据块， 加密后的数据块长度也是128位
*/
class X_AES_CBC
{
public:
	X_AES_CBC();
	
	~X_AES_CBC();

	static void info();

	bool encrypt(const std::string key, const std::string plaintext, std::string &ciphertext, const std::string iv="");

	bool decrypt(const std::string key, const std::string ciphertext, std::string &plaintext, const std::string iv="");
	
};



//------------------------------------------




/*
DES 使用64 bit (8 byte)密钥加密64bit (8byte)的数据块， 加密后的数据块长度也是64位
*/
class X_DES_CBC
{
public:
	X_DES_CBC();
	
	~X_DES_CBC();

	static void info();

	bool encrypt(const std::string key, const std::string plaintext, std::string &ciphertext, const std::string iv="");

	bool decrypt(const std::string key, const std::string ciphertext, std::string &plaintext, const std::string iv="");
	
};





//------------------------------------------


/*
RSA虽然也可以看成是基于Block的加密，但是，RSA的输入和输出的Block的大小是不一样的，
Block的大小依赖于所使用的RSA Key 的长度和RSA的padding模式。
分别对RSA设置三种长度的Key（768, 1024, 2048）和2种padding模式（PKCS 1.5 和OAEP），结果如下:

RSA                        InBlock大小   OutBlock大小  (单位，字节)
=============      ==========    =============
768bit/PKCS            85                    96
1024bit/PKCS          117                  128
2048bit/PKCS          245                  256

------------------------------------------------

768bit/OAEP           54                    192
1000bit/OAEP         83                     250
1024bit/OAEP         86                     256    <-- RSAES_OAEP_SHA_Encryptor 默认使用1024位RSA_Key长度
2048bit/OAEP         214                   512

相同密钥长度，加密出来的密文长度要比明文要长，且OAEP的InBlock/OutBlock要
比PKCS的InBlock/OutBlock要小，单从熵的角度，意味着OAEP padding模式引入更多的熵，
OAEP要比PKCS更安全

OAEP(最佳非对称加密填充):
是一个通常和RSA一起使用的填充方案。
OAEP由Bellare和Rogaway 提出的。

PKCS(The Public-Key Cryptography Standards):
是由美国RSA数据安全公司及其合作伙伴制定的一组公钥密码学标准，
其中包括证书申请、证书更新、证书作废表发布、扩展证书内容以及
数字签名、数字信封的格式等方面的一系列相关协议。

这种算法非常可靠，密钥越长，它就越难破解。根据已经披露的文献，
目前被破解的最长RSA密钥是768个二进制位。也就是说，长度超过768位的密钥，
还无法破解（至少没人公开宣布）。因此可以认为，1024位的RSA密钥基本安全，
2048位的密钥极其安全。


X_RSA_OAEP提供长度1024 key

*/
class X_RSA_OAEP
{
public:
	X_RSA_OAEP();
	
	~X_RSA_OAEP();

	static void info();

	//key_path 必须已经存在
	bool generate_rsa_key(const std::string &key_path, const std::string &key_name);

	bool encrypt(const std::string public_key_file, const std::string plaintext, std::string &ciphertext);

	bool decrypt(const std::string private_key_file, const std::string ciphertext, std::string &plaintext);
	
};



NS_BASE_END


#endif


