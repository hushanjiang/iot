
CC = g++ -DLINUX -D_REENTRANT
FLAGS = -W -g

BASE_ROOT=../../../../../sdk/base/
JSON_ROOT=../../../../../3rd_party/sdk/json-c/
TINY_XML_ROOT=../../../../../3rd_party/sdk/tinyxml/
GEOIP_ROOT=../../../../../3rd_party/sdk/geoip/
HIREDIS_ROOT=../../../../../3rd_party/sdk/hiredis/
CRYPTOPP_ROOT=../../../../../3rd_party/sdk/cryptopp/
OPENSSL_ROOT=../../../../../3rd_party/sdk/openssl/

INCLUDES=-I./ -I../ -I$(BASE_ROOT)/include/ -I$(JSON_ROOT)/include/ \
-I$(TINY_XML_ROOT)/include/  -I$(GEOIP_ROOT)/include/ -I$(HIREDIS_ROOT)/include/ \
-I$(CRYPTOPP_ROOT)/include/ -I$(OPENSSL_ROOT)/include/

LIB_PATH=

SHARE_LIBS=$(BASE_ROOT)/lib/libbase.a \
$(JSON_ROOT)/lib/libjson-c.a $(TINY_XML_ROOT)/lib/libtinyxml.a \
$(GEOIP_ROOT)/lib/libGeoIP.a $(HIREDIS_ROOT)/lib/libhiredis.a \
$(CRYPTOPP_ROOT)/lib/libcryptopp.a $(OPENSSL_ROOT)/lib/libssl.a $(OPENSSL_ROOT)/lib/libcrypto.a \
-lpthread -ldl -lrt -lz

TARGET=access_svr.bin

OBJS = $(patsubst %.cpp,obj/%.o,$(wildcard *.cpp))

obj/%.o: %.h
obj/%.o: %.cpp
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(LIB_PATH) $(OBJS) $(SHARE_LIBS);
	cp $(TARGET)  ../../../../../release/bin/$(TARGET)

clean:
	rm obj/*.o
	rm $(TARGET)

init:
	mkdir -p obj
	
