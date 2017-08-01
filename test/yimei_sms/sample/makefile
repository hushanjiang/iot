
CC = g++ -DLINUX
FLAGS = -W -g

SMS_ROOT=../src

INCLUDES=-I./ -I../ -I$(SMS_ROOT)

LIB_PATH=

SHARE_LIBS=$(SMS_ROOT)/libEUCPCommC.a -lpthread -ldl -lrt -lz

TARGET=test.bin

OBJS = $(patsubst %.cpp,obj/%.o,$(wildcard *.cpp))

obj/%.o: %.h
obj/%.o: %.cpp
	$(CC) $(FLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(LIB_PATH) $(OBJS) $(SHARE_LIBS);

clean:
	rm obj/*.o
	rm $(TARGET)

init:
	mkdir -p obj
	
