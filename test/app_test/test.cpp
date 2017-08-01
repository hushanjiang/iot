
#include "base/base_cryptopp.h"
#include "base/base_convert.h"
#include "base/base_string.h"
#include "base/base_base64.h"
#include <Des.h>
#include <Base64.h>

USING_NS_BASE;

int main(int argc, char * argv[])
{
	int nRet = 0;
	
	//aes
	//std::string src = "{ \"method\": \"login_pwd\", \"timestamp\": 123456789, \"req_id\": 123, \"params\": { \"login_id\": \"13480669985\", \"pwd\": \"abc123\" } }";
	std::string src = "{ \"method\": \"register_user\", \"timestamp\": 123456789, \"req_id\": 123, \"params\": { \"login_id\": \"13480669985\", \"code\": \"9527\" } }";
	std::string encrypt = "";
	
	printf("--------- aes-cbc -----------\n\n");
	std::string key = "111";
	X_AES_CBC aes;
	if(!aes.encrypt(key, src, encrypt))
	{
		printf("aes encrypt failed.\n");
		return -1;
	}
	//printf("aes encrypt:%s\n", encrypt.c_str());
	print_hex(encrypt.c_str(), encrypt.size(), "aes encrypt:");
	
	
	//base64
	X_BASE64 base64;
	char *tmp = NULL;
	base64.encrypt(encrypt.c_str(), encrypt.size(), tmp);
	std::string base64_value = tmp;
	DELETE_POINTER_ARR(tmp);	
	printf("base64(%u):%s\n", base64_value.size(), base64_value.c_str());

	std::string buf = "";
	nRet = base64.decrypt(base64_value.c_str(), tmp);
	buf = tmp;
	DELETE_POINTER_ARR(tmp);

	//printf("buf:%s\n", buf.c_str());
	print_hex(buf.c_str(), buf.size(), "base64 decrypt:");
	
	std::string new_req = "";
	if(!aes.decrypt(key, buf, new_req))
	{
		printf("aes decrypt failed.\n");
		return 0;
	}
	
	printf("new_req:%s\n", new_req.c_str());
	
	return 0;
	
}

