# MAKEFILE FOR DISSERVICEPRO PROJECT

NOMADS_HOME = ../../../..
EXTERNALS = $(NOMADS_HOME)/externals
UTIL_HOME = $(NOMADS_HOME)/util
SQLITE_HOME = $(EXTERNALS)/SQLite
TINYXPATH_HOME = $(NOMADS_HOME)/externals/TinyXPath
C45_HOME = $(NOMADS_HOME)/misc/cpp/c45
LCPPDC_HOME = $(NOMADS_HOME)/misc/cpp/lcppdc
MIL_STD_2525= $(NOMADS_HOME)/misc/cpp/milstd2525
DTLS_HOME = $(NOMADS_HOME)/misc/cpp/dtlsInfrastructure
OPEN_SSL_HOME=$(EXTERNALS)/openssl/1.0.2h
ZLIB_HOME=$(EXTERNALS)/zlib

EXECUTABLE = VoiLauncher
DSPROSHELL = DSProShell

#Environment
ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
include ../../../../util/scripts/Makefile.$(ARCH)

CPP=g++ 
CPPFLAGS = -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas -Wno-implicit-fallthrough -DUNIX $(ARCH_FLAGS) -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -std=c++11 \
                -I../ \
		-I../core/ \
		-I../ctxt/ \
		-I../util/ \
		-I$(UTIL_HOME)/cpp/ \
		-I$(UTIL_HOME)/cpp/proxy \
		-I$(SQLITE_HOME)/ \
		-isystem $(TINYXPATH_HOME)/ \
		-I$(C45_HOME)/ \
		-I$(LCPPDC_HOME)/ \
		-I$(MIL_STD_2525)/ \
		-I$(CHUNKING_HOME)/ \
		-I../jni/ \
		-I$(NOMADS_HOME)/externals/JDK/ \
		-I$(NOMADS_HOME)/externals/JDK/$(LIB_FOLDER) 


LD_FLAGS = -lpthread -ldl

LIBS=libvoi.a \
	$(C45_HOME)/linux/libc4.5.a \
	$(LCPPDC_HOME)/linux/liblcppdc.a \
	$(MIL_STD_2525)/linux/libmil2525.a \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	$(SQLITE_HOME)/linux/libsqlite3.a \
	$(OPEN_SSL_HOME)/$(PRECOMPILED_LIB_FOLDER)/lib/libssl.a \
	$(OPEN_SSL_HOME)/$(PRECOMPILED_LIB_FOLDER)/lib/libcrypto.a \
	$(TINYXPATH_HOME)/libtinyxpath.a \
	$(ZLIB_HOME)/$(LIB_FOLDER)/libz.a \
	$(EXTRA_LIB)


#	$(NOMADS_HOME)/util/cpp/linux/libsecurity.a \
