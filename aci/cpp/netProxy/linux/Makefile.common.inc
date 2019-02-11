CPP = g++
C11FLAG = -std=c++11

NOMADS_HOME = ../../../..
ACI_HOME = $(NOMADS_HOME)/aci
UTIL_HOME = $(NOMADS_HOME)/util
MOCKETS_HOME = $(NOMADS_HOME)/mockets
NETSENSOR_HOME = $(NOMADS_HOME)/misc/cpp/netsensor
EXTERNALS = $(NOMADS_HOME)/externals
PROTOBUF_HOME = $(EXTERNALS)/protobuf/3.6.1
OPENSSL_HOME = $(EXTERNALS)/openssl/1.0.2h
LIBZ_HOME = $(EXTERNALS)/zlib
LZMA_HOME = $(EXTERNALS)/xz
LZMA_LIB = $(LZMA_HOME)/src/liblzma/api

LIB_FOLDER = linux
PROTO_SCRIPT = ../proto/compile_proto.sh

ARCH = $(shell sh $(UTIL_HOME)/scripts/guessArch.sh)
MACHINE = $(shell uname -m)

ifeq ($(MACHINE), x86_64)
CPPFLAGS += -m64
else
CPPFLAGS += -m32
endif

CPPFLAGS += -DUNIX -DLINUX -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
			-I.. -I../.. \
			-I$(EXTERNALS)/include/pcap \
			-I$(EXTERNALS)/include \
			-I$(PROTOBUF_HOME)/include \
			-I$(UTIL_HOME)/cpp/net \
			-I$(UTIL_HOME)/cpp \
			-I$(MOCKETS_HOME)/cpp \
			-I$(NETSENSOR_HOME)

LIB_LIST = $(NETSENSOR_HOME)/linux/libnetsensor.a \
	   $(MOCKETS_HOME)/cpp/$(LIB_FOLDER)/libmockets.a \
	   $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
	   $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
	   $(LIBZ_HOME)/$(LIB_FOLDER)/libz.a \
	   $(LZMA_HOME)/$(LIB_FOLDER)/liblzma.a \
	   -L$(PROTOBUF_HOME)/lib/$(ARCH) \
	   -L$(OPENSSL_HOME)/lib/$(ARCH)

LD_FLAGS = -lpcap -lprotobuf -lssl -lcrypto -lpthread -ldl

sources = $(wildcard ../*.cpp ) $(wildcard ../*.cc)
objects = $(sources:../%.cpp=%.o) $(sources:../%.cc=%.o)
