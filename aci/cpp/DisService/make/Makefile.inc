#
# Disservice Include Makefile
#
# author Giacomo Benincasa (gbenincasa@ihmc.us)
#

#Paths
PROJECT_ROOT = ../../../../..
NOMADS_HOME = ../../../..
EXTERNALS = $(NOMADS_HOME)/externals

SQLITE_HOME = $(EXTERNALS)/SQLite
SQLITE_LIB = $(SQLITE_HOME)/linux/.libs
TINYXPATH_HOME = $(NOMADS_HOME)/externals/TinyXPath
NMS_HOME= $(NOMADS_HOME)/nms/cpp
LCPPDC_HOME= $(NOMADS_HOME)/misc/cpp/lcppdc
CHUNKING_HOME= $(NOMADS_HOME)/misc/cpp/chunking
MEDIA_HOME= $(NOMADS_HOME)/misc/cpp/media
JPEG_HOME=$(NOMADS_HOME)/externals/libjpeg
JASPER_HOME=$(NOMADS_HOME)/externals/jasper
PNG_HOME=$(NOMADS_HOME)/externals/lpng1513
MSGPACK_HOME = $(EXTERNALS)/msgpack
NOCKET_HOME = $(NOMADS_HOME)/misc/cpp/nockets
NORM_HOME = $(EXTERNALS)/norm-1.5r6
DALI_HOME=$(EXTERNALS)/dali-1.0
OPEN_SSL_HOME=$(EXTERNALS)/openssl/1.0.2h
LIBZ_HOME = $(EXTERNALS)/zlib

#Compiler
CPP = g++

LD_FLAGS = -lpthread

#Environment
ARCH = $(shell sh $(NOMADS_HOME)/util/scripts/guessArch.sh)
include Makefile.$(ARCH)

CPPFLAGS = -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas -Wno-implicit-fallthrough \
	-DUNIX $(BUILD_FLAGS) -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -std=c++11 \
	-I.. \
	-isystem $(EXTERNALS)/include \
	-I$(NOMADS_HOME)/util/cpp \
	-I$(NOMADS_HOME)/util/cpp/security \
	-I$(NOMADS_HOME)/util/cpp/net \
	-I$(NOMADS_HOME)/util/cpp/proxy \
	-I$(NOMADS_HOME)/util/cpp/graph \
	-I$(NMS_HOME)/ \
	-I$(NMS_HOME)/proxy \
	-I$(NMS_HOME)/proxy/client \
	-I$(NMS_HOME)/proxy/server \
	-I$(SQLITE_HOME)/ \
	-I$(LCPPDC_HOME)/ \
	-I$(CHUNKING_HOME)/ \
	-isystem $(TINYXPATH_HOME)/ \
	-I$(MOCKETS_HOME)/cpp \
	-I$(NOMADS_HOME)/externals/JDK \
        -I$(NOMADS_HOME)/externals/JDK/$(LIB_FOLDER)

SOURCESPROXY=../DisseminationServiceProxyAdaptor.cpp ../DisseminationServiceProxyCallbackHandler.cpp ../DisseminationServiceProxy.cpp ../DisseminationServiceProxyServer.cpp
JNISOURCES =../DisServiceJNIWrapper.cpp ../JNIUtils.cpp
APPLICATIONS=../DisServiceLauncher.cpp ../DisServiceMessageInjector.cpp ../DisServicePacketTool.cpp 
NOWANTS = $(JNI) $(APPLICATIONS) $(SOURCESPROXY) 

ifdef USE_SYSTEM_LIBS
  EXTRA_LIB = -lcrypto -lz
else
  HAS_Z_LIB = $(shell g++ -lz 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
  EXTRA_LIB += $(shell if [ ${HAS_Z_LIB} -gt 0 ]; then echo "$(LIBZ_HOME)/$(LIB_FOLDER)/libz.a"; else  echo "-lz"; fi)
endif

LIB_LIST = $(LCPPDC_HOME)/$(LIB_FOLDER)/liblcppdc.a \
	$(NMS_HOME)/$(LIB_FOLDER)/libnms.a \
        $(CHUNKING_HOME)/$(LIB_FOLDER)/libchunking.a \
        $(MEDIA_HOME)/$(LIB_FOLDER)/libmedia.a \
	$(NOCKET_HOME)/linux/libnockets.a \
        $(SQLITE_HOME)/$(LIB_FOLDER)/libsqlite3.a \
        $(TINYXPATH_HOME)/$(LIB_FOLDER)/libtinyxpath.a \
        $(NOMADS_HOME)/util/cpp/$(LIB_FOLDER)/libsecurity.a \
        $(NOMADS_HOME)/util/cpp/$(LIB_FOLDER)/libutil.a \
        $(JASPER_HOME)/$(LIB_FOLDER)/libjasper.a \
        $(JPEG_HOME)/$(LIB_FOLDER)/libjpeg.a \
        $(PNG_HOME)/$(LIB_FOLDER)/libpng.a \
        $(MSGPACK_HOME)/$(LIB_FOLDER)/libmsgpack.a \
	$(DALI_HOME)/$(LIB_FOLDER)/libdvmmpeg.a \
	$(DALI_HOME)/$(LIB_FOLDER)/libdvmbasic.a \
	$(OPEN_SSL_HOME)/$(ARCH)/lib/libcrypto.a \
	$(EXTRA_LIB)

#	$(NORM_HOME)/lib/libnorm.a \
#	$(NORM_HOME)/build/protolib/libprotolib.a \
#	$(NORM_HOME)/protolib/lib/libprotokit.a \

