# MAKEFILE FOR DISSERVICEPRO PROJECT

PROJECT_ROOT = ../../../../..
NOMADS_HOME = ../../../..
EXTERNALS = $(NOMADS_HOME)/externals
UTIL_HOME = $(NOMADS_HOME)/util
MOCKETS_HOME = $(NOMADS_HOME)/mockets
SQLITE_HOME = $(EXTERNALS)/SQLite
TINYXPATH_HOME = $(NOMADS_HOME)/externals/TinyXPath
DISSERVICE_HOME = $(NOMADS_HOME)/aci/cpp/DisService
NMS_HOME= $(NOMADS_HOME)/nms
C45_HOME = $(NOMADS_HOME)/misc/cpp/c45
LCPPDC_HOME = $(NOMADS_HOME)/misc/cpp/lcppdc
MIL_STD_2525= $(NOMADS_HOME)/misc/cpp/milstd2525
GRP_MANAGER_HOME = $(NOMADS_HOME)/aci/cpp/grpMgr
LCPPDC_HOME= $(NOMADS_HOME)/misc/cpp/lcppdc
CHUNKING_HOME = $(NOMADS_HOME)/misc/cpp/chunking
MEDIA_HOME = $(NOMADS_HOME)/misc/cpp/media
DTLS_HOME = $(NOMADS_HOME)/misc/cpp/dtlsInfrastructure
JPEG_HOME=$(NOMADS_HOME)/externals/libjpeg
PNG_HOME=$(NOMADS_HOME)/externals/lpng1513
JASPER_HOME=$(NOMADS_HOME)/externals/jasper
NOCKET_HOME = $(NOMADS_HOME)/misc/cpp/nockets
NORM_HOME = $(EXTERNALS)/norm-1.5r6
DALI_HOME=$(EXTERNALS)/dali-1.0

EXECUTABLE = DSPro

#Environment
ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
include ../../../../util/scripts/Makefile.$(ARCH)

CPP=g++
CPPFLAGS = -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas -DUNIX $(ARCH_FLAGS) -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
                -I../ \
		-I../ranking/ \
		-I../awareness/ \
		-I../storage/ \
		-I../scheduler/ \
		-I../proxy/ \
		-I../comm/ \
		-I../comm/disservice/ \
		-I../comm/mockets/ \
		-I../comm/tcp/ \
		-I../controllers/query/ \
		-I../controllers/forwarding/ \
		-I$(DISSERVICE_HOME)/ \
		-I$(UTIL_HOME)/cpp/ \
		-I$(UTIL_HOME)/cpp/net/ \
		-I$(UTIL_HOME)/cpp/graph/ \
		-I$(MOCKETS_HOME)/cpp/ \
		-I$(SQLITE_HOME)/ \
		-isystem $(TINYXPATH_HOME)/ \
		-I$(C45_HOME)/ \
		-I$(LCPPDC_HOME)/ \
		-I$(MIL_STD_2525)/ \
		-I$(GRP_MANAGER_HOME)/ \
		-I$(LCPPDC_HOME)/ \
		-I$(CHUNKING_HOME)/ \
		-I../jni/ \
		-I$(NOMADS_HOME)/externals/JDK/ \
		-I$(NOMADS_HOME)/externals/JDK/$(LIB_FOLDER) 

#HAS_SSL_LIB = $(shell g++ -lssl 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
#EXTRA_LIB += $(shell if [ ${HAS_SSL_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libssl.a"; else  echo "-lssl"; fi)

#HAS_CRYPTO_LIB = $(shell g++ -lcrypto 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
#EXTRA_LIB += $(shell if [ ${HAS_CRYPTO_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libcrypto.a"; else  echo "-lcrypto"; fi)

#HAS_Z_LIB = $(shell g++ -lz 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
#EXTRA_LIB += $(shell if [ ${HAS_Z_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libz.a"; else  echo "-lz"; fi)

EXTRA_LIB += $(EXTERNALS_LIB)/libssl.a $(EXTERNALS_LIB)/libcrypto.a $(EXTERNALS_LIB)/libz.a 

LD_FLAGS = -lpthread -ldl

LIBS=libdspro.a \
	$(MOCKETS_HOME)/cpp/linux/libmockets.a \
	$(DISSERVICE_HOME)/linux/libdisservice.a \
	$(DISSERVICE_HOME)/linux/libdisserviceproxy.a \
	$(NMS_HOME)/cpp/linux/libnms.a \
	$(C45_HOME)/linux/libc4.5.a \
	$(LCPPDC_HOME)/linux/liblcppdc.a \
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
	$(NORM_HOME)/lib/libnorm.a \
        $(NORM_HOME)/build/protolib/libprotolib.a \
        $(NORM_HOME)/protolib/lib/libprotokit.a \
	$(DALI_HOME)/$(LIB_FOLDER)/libdvmmpeg.a \
	$(DALI_HOME)/$(LIB_FOLDER)/libdvmbasic.a \
	$(EXTRA_LIB)
