
#include "base/base_cryptopp.h"
#include "base/base_convert.h"
#include "base/base_string.h"

USING_NS_BASE;

int main(int argc, char * argv[])
{
	int nRet = 0;

	//std::string src = "{\"id\":\"11111\", \"token\":\"123abc#\"}";
	std::string src = "{\"id\":\"11111\", \"token\":\"fffffffffffffffffffffffffffffffffffffffffffffdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd123\"}";
	std::string digest = "";

	printf("src:%s\n", src.c_str());

	printf("--------- md5 -----------\n\n");
	X_MD5 md5;
	if(!md5.calculate_digest(src, digest))
	{
		printf("md5 failed.\n");
		return -1;
	}
	printf("md5:%s\n", digest.c_str());
	


	printf("--------- sha256 -----------\n\n");
	X_SHA256 sha256;
	if(!sha256.calculate_digest(src, digest))
	{
		printf("sha256 failed.\n");
		return -1;
	}
	printf("sha256:%s\n", digest.c_str());


	printf("--------- sha1 -----------\n\n");
	X_SHA1 sha1;
	if(!sha1.calculate_digest(src, digest))
	{
		printf("sha1 failed.\n");
		return -1;
	}
	printf("sha1:%s\n", digest.c_str());
	

	std::string encrypt = "";
	std::string decrypt = "";
	printf("--------- aes-cbc -----------\n\n");
	std::string key = "xcp";
	X_AES_CBC aes;
	if(!aes.encrypt(key, src, encrypt))
	{
		printf("aes encrypt failed.\n");
		return -1;
	}
	printf("aes encrypt:%s\n", encrypt.c_str());

	//key = "aaa";
	if(!aes.decrypt(key, encrypt, decrypt))
	{
		printf("aes decrypt failed.\n");
		return -1;
	}
	printf("aes decrypt:%s\n", decrypt.c_str());


	encrypt = "";
	decrypt = "";
	printf("--------- des-cbc -----------\n\n");
	X_DES_CBC des;
	des.info();
	if(!des.encrypt(key, src, encrypt))
	{
		printf("des encrypt failed.\n");
		return -1;
	}
	printf("des encrypt:%s\n", encrypt.c_str());
	
	if(!des.decrypt(key, encrypt, decrypt))
	{
		printf("des decrypt failed.\n");
		return -1;
	}
	printf("des decrypt:%s\n", decrypt.c_str());
	


	encrypt = "";
	decrypt = "";
	printf("--------- rsa-oaep pub->pri -----------\n\n");
	X_RSA_OAEP rsa_oaep;
	key = "xcp";
	/*
	if(!rsa_oaep.generate_rsa_key("./", key))
	{
		printf("generate rsa key failed.\n");
		return -1;
	}
	*/

	if(!rsa_oaep.encrypt(key+std::string("_public.key"), src, encrypt))
	{
		printf("rsa_oaep encrypt failed.\n");
		return -1;
	}
	printf("rsa_oaep encrypt:%s\n", encrypt.c_str());
	
	if(!rsa_oaep.decrypt(key+std::string("_private.key"), encrypt, decrypt))
	{
		printf("rsa_oaep decryp failed.\n");
		return -1;
	}
	printf("rsa_oaep decrypt:%s\n", decrypt.c_str());
	
	return 0;
	
}

