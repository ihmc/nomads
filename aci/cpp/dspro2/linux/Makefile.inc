# MAKEFILE FOR DISSERVICEPRO PROJECT

PROJECT_ROOT = ../../../../..
NOMADS_HOME = ../../../..
EXTERNALS = $(NOMADS_HOME)/externals
UTIL_HOME = $(NOMADS_HOME)/util
MOCKETS_HOME = $(NOMADS_HOME)/mockets
SQLITE_HOME = $(EXTERNALS)/SQLite
TINYXPATH_HOME = $(NOMADS_HOME)/externals/TinyXPath
CNATS_HOME = $(NOMADS_HOME)/externals/cnats/cnats-master
DISSERVICE_HOME = $(NOMADS_HOME)/aci/cpp/DisService
NMS_HOME= $(NOMADS_HOME)/nms
C45_HOME = $(NOMADS_HOME)/misc/cpp/c45
LCPPDC_HOME = $(NOMADS_HOME)/misc/cpp/lcppdc
MIL_STD_2525= $(NOMADS_HOME)/misc/cpp/milstd2525
GRP_MANAGER_HOME = $(NOMADS_HOME)/aci/cpp/grpMgr
LCPPDC_HOME= $(NOMADS_HOME)/misc/cpp/lcppdc
VOI_HOME= $(NOMADS_HOME)/misc/cpp/voi
CHUNKING_HOME = $(NOMADS_HOME)/misc/cpp/chunking
MEDIA_HOME = $(NOMADS_HOME)/misc/cpp/media
DTLS_HOME = $(NOMADS_HOME)/misc/cpp/dtlsInfrastructure
JPEG_HOME=$(NOMADS_HOME)/externals/libjpeg
PNG_HOME=$(NOMADS_HOME)/externals/lpng1513
JASPER_HOME=$(NOMADS_HOME)/externals/jasper
MSGPACK_HOME = $(EXTERNALS)/msgpack
NOCKET_HOME = $(NOMADS_HOME)/misc/cpp/nockets
NORM_HOME = $(EXTERNALS)/norm-1.5r6
DALI_HOME=$(EXTERNALS)/dali-1.0
OPEN_SSL_HOME=$(EXTERNALS)/openssl/1.0.2h
ZLIB_HOME=$(EXTERNALS)/zlib
NATS_HOME=$(EXTERNALS)/cnats/cnats-master
NATSWR_HOME=$(NOMADS_HOME)/misc/cpp/natswr
PROTOBUF_HOME=$(EXTERNALS)/protobuf/3.6.1

EXECUTABLE = DSPro
DSPROSHELL = DSProShell

#Environment
ARCH = $(shell sh $(UTIL_HOME)/scripts/guessArch.sh)
include $(UTIL_HOME)/scripts/Makefile.$(ARCH)

CPP=g++
CPPFLAGS = -std=c++11 -no-pie -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas -Wno-implicit-fallthrough -DUNIX $(ARCH_FLAGS) -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
                -I../ \
                -I../apps/ \
		-I../ranking/ \
		-I../awareness/ \
		-I../awareness/parts \
		-I../storage/ \
		-I../scheduler/ \
		-I../proxy/ \
		-I../comm/ \
		-I../comm/disservice/ \
		-I../comm/mockets/ \
		-I../comm/netlogger/ \
		-I../comm/tcp/ \
		-I../comm/udp/ \
		-I../controllers/query/ \
		-I../controllers/forwarding/ \
		-I$(DISSERVICE_HOME)/ \
		-I$(UTIL_HOME)/cpp/ \
		-I$(UTIL_HOME)/cpp/proxy \
		-I$(UTIL_HOME)/cpp/net/ \
		-I$(UTIL_HOME)/cpp/graph/ \
		-I$(UTIL_HOME)/cpp/proxy/ \
		-I$(MOCKETS_HOME)/cpp/ \
		-I$(SQLITE_HOME)/ \
		-isystem $(TINYXPATH_HOME)/ \
		-I$(C45_HOME)/ \
		-I$(LCPPDC_HOME)/ \
		-I$(MIL_STD_2525)/ \
		-I$(GRP_MANAGER_HOME)/ \
		-I$(LCPPDC_HOME)/ \
		-I$(VOI_HOME)/ \
		-I$(VOI_HOME)/core/ \
		-I$(VOI_HOME)/ctxt/ \
		-I$(VOI_HOME)/util/ \
		-I$(CHUNKING_HOME)/ \
		-I$(CHUNKING_HOME)/proxy/ \
		-isystem$(PROTOBUF_HOME)/include/ \
		-I$(NATSWR_HOME)/ \
		-I$(ZLIB_HOME)/ \
		-I../jni/ \
		-I$(NOMADS_HOME)/externals/JDK/ \
		-I$(NOMADS_HOME)/externals/JDK/$(LIB_FOLDER)/

LD_FLAGS = -lpthread -ldl

LIBS=libdspro.a \
	$(MOCKETS_HOME)/cpp/linux/libmockets.a \
	$(DISSERVICE_HOME)/linux/libdisservice.a \
	$(DISSERVICE_HOME)/linux/libdisserviceproxy.a \
	$(NMS_HOME)/cpp/linux/libnms.a \
	$(C45_HOME)/linux/libc4.5.a \
	$(LCPPDC_HOME)/linux/liblcppdc.a \
	$(VOI_HOME)/linux/libvoi.a \
	$(MIL_STD_2525)/linux/libmil2525.a \
	$(CHUNKING_HOME)/linux/libchunking.a \
	$(MEDIA_HOME)/linux/libmedia.a \
	$(JASPER_HOME)/linux/libjasper.a \
	$(JPEG_HOME)/linux/libjpeg.a \
	$(PNG_HOME)/linux/libpng.a \
	$(NOCKET_HOME)/linux/libnockets.a \
	$(NOMADS_HOME)/util/cpp/linux/libsecurity.a \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	$(SQLITE_HOME)/linux/libsqlite3.a \
	$(TINYXPATH_HOME)/libtinyxpath.a \
	$(DALI_HOME)/$(LIB_FOLDER)/libdvmmpeg.a \
	$(DALI_HOME)/$(LIB_FOLDER)/libdvmbasic.a \
	$(OPEN_SSL_HOME)/lib/$(ARCH)/libssl.a \
	$(OPEN_SSL_HOME)/lib/$(ARCH)/libcrypto.a \
	$(ZLIB_HOME)/$(LIB_FOLDER)/libz.a \
	$(NATSWR_HOME)/$(LIB_FOLDER)/libnatswr.a \
	$(NATS_HOME)/$(LIB_FOLDER)/libnats_static.a \
	$(PROTOBUF_HOME)/lib/$(PRECOMPILED_LIB_FOLDER)/libprotobuf.a \
	$(EXTRA_LIB)

#	$(NORM_HOME)/lib/libnorm.a \
#        $(NORM_HOME)/build/protolib/libprotolib.a \
#        $(NORM_HOME)/protolib/lib/libprotokit.a \

