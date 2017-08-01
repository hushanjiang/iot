############################################################################################
# install configure
############################################################################################

DESTDIR=
PREFIX=/usr/local
prefix=$(DESTDIR)${PREFIX}
libdir=$(DESTDIR)${PREFIX}/lib/search
shlibdir=$(DESTDIR)${PREFIX}/lib/search
incdir=$(DESTDIR)${PREFIX}/include/search
bindir=$(DESTDIR)${PREFIX}/bin
mandir=$(DESTDIR)${PREFIX}/man

############################################################################################
# compile tools
############################################################################################

MAKE=make
CC=gcc
CCC=g++
AR=ar
RANLIB=ranlib

############################################################################################
# common options
############################################################################################

BUILD_CPP=yes

STRIP=echo ignoring strip
OPTCPPFLAGS= -fomit-frame-pointer -Wall -Wno-switch -Wdisabled-optimization -Wpointer-arith -Wredundant-decls
OPTFLAGS= -fomit-frame-pointer -Wdeclaration-after-statement -Wall -Wno-switch -Wdisabled-optimization -Wpointer-arith -Wredundant-decls
LIBOBJFLAGS=
EXTRALIBS=

ifeq ($(BUILD_RELEASE),yes)
OPTCPPFLAGS+= -O3
OPTFLAGS+= -O3
else
#OPTCPPFLAGS+= -g -DDEBUG
#OPTFLAGS+= -g -DDEBUG
OPTCPPFLAGS+= -g 
OPTFLAGS+= -g 
endif

############################################################################################
# static libraries
############################################################################################

BUILD_STATIC=yes

BUILDSUF=
LIBPREF=lib
LIBSUF=${BUILDSUF}.a

ifeq ($(BUILD_STATIC),yes)
LIB=$(LIBPREF)$(NAME)$(LIBSUF)
else
LIB=
endif

############################################################################################
# share libraries
############################################################################################

BUILD_SHARED=no

SLIBPREF=
SLIBSUF=${BUILDSUF}.so
SLIBNAME_WITH_VERSION=$(SLIBPREF)$(NAME)-$(LIBVERSION)$(SLIBSUF)
SLIBNAME_WITH_MAJOR=$(SLIBPREF)$(NAME)-$(LIBMAJOR)$(SLIBSUF)
SLIB_EXTRA_CMD=
SLIB_INSTALL_EXTRA_CMD=-install -m 644 $(SLIBNAME_WITH_MAJOR:$(SLIBSUF)=.lib) "$(shlibdir)/$(SLIBNAME_WITH_MAJOR:$(SLIBSUF)=.lib)"
LIB_INSTALL_EXTRA_CMD=$(RANLIB) "$(libdir)/$(LIB)"
SHFLAGS=-shared

ifeq ($(BUILD_SHARED),yes)
SLIBNAME=$(SLIBPREF)$(NAME)$(SLIBSUF)
else
SLIBNAME=
endif

############################################################################################
# custom configure
############################################################################################

SRC_PATH_BARE=..
SRC_PATH="$(SRC_PATH_BARE)"
BUILD_ROOT="$(SRC_PATH_BARE)"

# OS_64            64bits system
# OS_BIG_ENDIAN    bigendian system
# DEBUG            print debug information
# OS_LINUX         3GWS linux version
#OPTCPPFLAGS += -DOS_64 -DOS_LINUX
OPTCPPFLAGS += -DOS_LINUX  -D_REENTRANT
