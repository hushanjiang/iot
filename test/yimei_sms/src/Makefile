#
# EUCPCommC Makefile
#
include ./config.mak

CPPOBJS=  EUCPCInterface.o     \
					Md5.o                \
					MTNMsgApi.o          \
					socktool.o           \
					log.o                \
					thread.o             

HEADERS = 

NAME=EUCPCommC


############################################################################################
# compile path
############################################################################################
INCFLAGS= 
# share libraries or executable file
LDFLAGS= 

EXTRALIBS += -lpthread

#CPPFLAGS+= -DEUCPC_DEBUG
CPPFLAGS+= 

############################################################################################
# application
############################################################################################

BUILD_APPLICATION=no

APPPREF=
APPSUF=${BUILDSUF}

ifeq ($(BUILD_APPLICATION),yes)
APPLICATION=$(APPPREF)$(NAME)$(APPSUF)
else
APPLICATION=
endif

ifeq ($(BUILD_SHARED),yes)
LIBVERSION=1.0
LIBMAJOR=1
endif

include ./common.mak









