#
# Makefile.in
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

NOMADS_HOME = ../../..
UTIL_HOME = $(NOMADS_HOME)/util
NOCKET_HOME = $(NOMADS_HOME)/misc/cpp/nockets
EXTERNALS=$(NOMADS_HOME)/externals
NORM_HOME = $(EXTERNALS)/norm-1.5r6

ARCH = $(shell sh $(NOMADS_HOME)/util/scripts/guessArch.sh)
MakefileArch=$(NOMADS_HOME)/util/scripts/Makefile.$(ARCH)
include $(MakefileArch)

cpp = g++
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -lpthread -ldl 
EXECUTABLE=NMSLauncher

options=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable

INCLUDES=-I..\
	-I../ifaces \
	-I../proxy -I../proxy/client -I../proxy/server \
	-I$(NOCKET_HOME) \
	-isystem $(UTIL_HOME)/cpp \
        -isystem $(UTIL_HOME)/cpp/net \
        -isystem $(UTIL_HOME)/cpp/proxy \
	-isystem $(SQLITE_HOME)/	\
	-isystem $(UTIL_HOME)/cpp/security \
	-I$(EXTERNALS)/include

#sanitizer=-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope -Wshadow
cppflags= -g -std=c++11 -fPIC $(ARCH_FLAGS) ${sanitizer} -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)

sources = $(filter-out ../NMSLauncher.cpp, $(wildcard ../*.cpp))
objects = $(sources:../%.cpp=%.o)

ifacessources += $(wildcard ../ifaces/*.cpp)
ifacesobjects += $(ifacessources:../ifaces/%.cpp=%.o)

proxysources += $(wildcard ../proxy/*.cpp)
proxyobjects += $(proxysources:../proxy/%.cpp=%.o)

proxyssources += $(wildcard ../proxy/server/*.cpp)
proxysobjects += $(proxyssources:../proxy/server/%.cpp=%.o)

proxycsources += $(wildcard ../proxy/client/*.cpp)
proxycobjects += $(proxycsources:../proxy/client/%.cpp=%.o)

LIBS=$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
     $(JPEG_HOME)/linux/libjpeg.a \
     $(EXTERNALS_LIB)/libz.a \
     $(EXTERNALS)/openssl/1.0.2h/linux/lib/libcrypto.a \
     -L/usr/local/lib/
