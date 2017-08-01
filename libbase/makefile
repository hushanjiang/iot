
CC = g++ -DLINUX -D_REENTRANT
FLAGS = -W -g -fPIC

TINY_XML_ROOT=../../3rd_party/sdk/tinyxml/
CURL_ROOT=../../3rd_party/sdk/curl/
CRYPTOPP_ROOT=../../3rd_party/sdk/cryptopp/
OPENSSL_ROOT=../../3rd_party/sdk/openssl/
ZLIB_ROOT=../../3rd_party/sdk/zlib/

INCLUDES=-I./ -I$(TINY_XML_ROOT)/include/ -I$(CURL_ROOT)/include/ \
-I$(CRYPTOPP_ROOT)/include/ -I$(OPENSSL_ROOT)/include/ -I$(ZLIB_ROOT)/include/

LIB_PATH=

SHARE_LIBS=$(TINY_XML_ROOT)/lib/libtinyxml.a $(CURL_ROOT)/lib/libcurl.a \
$(CRYPTOPP_ROOT)/lib/libcryptopp.a $(OPENSSL_ROOT)/lib/libssl.a $(OPENSSL_ROOT)/lib/libcrypto.a \
$(ZLIB_ROOT)/lib/libz.a \
-lpthread -ldl -lrt -lz

SONAME=libbase.so
TARGET=libbase.so.1.0
S_TARGET=libbase.a

ALL: $(TARGET) $(S_TARGET)

OBJS = $(patsubst %.cpp,obj/%.o,$(wildcard *.cpp))

obj/%.o: %.h
obj/%.o: %.cpp
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@ 

$(TARGET): $(OBJS)
	$(CC) -shared -fPIC -Wl,-soname,$(SONAME) -o $(TARGET) $(OBJS) $(SHARE_LIBS)
	cp $(TARGET) ../../sdk/base/lib/
	cp $(TARGET) ../../sdk/base/lib/$(SONAME)
	cp *.h *.inl ../../sdk/base/include/base/

$(S_TARGET): $(OBJS) 
	ar -r $(S_TARGET) obj/*.o 
	cp $(S_TARGET)  ../../sdk/base/lib

init:
	mkdir -p obj
	mkdir -p ../../sdk/base/include/base
	mkdir -p ../../sdk/base/lib
	
clean:
	-rm obj/*.o
	-rm ./$(S_TARGET)
	-rm ./$(TARGET)	
	-rm ../../sdk/base/include/base/*.h
	-rm ../../sdk/base/include/base/*.inl
	-rm ../../sdk/base/lib/$(S_TARGET)
	-rm ../../sdk/base/lib/$(TARGET)
	-rm ../../sdk/base/lib/$(SONAME)
	
