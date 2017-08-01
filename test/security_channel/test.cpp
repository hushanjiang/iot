
#include "base/base_cryptopp.h"
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_base64.h"

USING_NS_BASE;

int main(int argc, char * argv[])
{
	int nRet = 0;

	//std::string src = "{\"method\":\"create_security_channel\", \"timestamp\":123456789, \"req_id\":123, \"params\":{\"id\":\"111\", \"iot_key\":\"111\"} }";
	std::string src = "{\"method\":\"refresh_security_channel\", \"timestamp\":123456789, \"req_id\":123, \"params\":{\"id\":\"111\"} }";
	printf("src:%s\n", src.c_str());

	std::string encrypt = "";
	std::string decrypt = "";
	printf("--------- rsa-oaep pub->pri -----------\n\n");
	X_RSA_OAEP rsa_oaep;
	if(!rsa_oaep.encrypt(std::string("iot_public.key"), src, encrypt))
	{
		printf("rsa_oaep encrypt failed.\n");
		return -1;
	}
	printf("rsa_oaep encrypt:%s\n", encrypt.c_str());

	if(!rsa_oaep.decrypt(std::string("iot_private.key"), encrypt, decrypt))
	{
		printf("rsa_oaep decryp failed.\n");
		return -1;
	}
	printf("rsa_oaep decrypt:%s\n", decrypt.c_str());
	
	return 0;
	
}

