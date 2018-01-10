CPP = g++
C11FLAG = -std=c++11

NOMADS_HOME = ../../../..
ACI_HOME = $(NOMADS_HOME)/aci
UTIL_HOME = $(NOMADS_HOME)/util
MOCKETS_HOME = $(NOMADS_HOME)/mockets
NETSENSOR_HOME = $(NOMADS_HOME)/misc/cpp/netsensor
EXTERNALS = $(NOMADS_HOME)/externals
PROTOBUF_HOME = $(EXTERNALS)/protobuf/protobuf-3.1.0/$(ARCH)
OPENSSL_HOME = $(EXTERNALS)/openssl/1.0.2h/$(ARCH)
LIBZ_HOME = $(EXTERNALS)/zlib
LZMA_HOME = $(EXTERNALS)/xz
LZMA_LIB = $(LZMA_HOME)/src/liblzma/api
MSGPACK_HOME = $(EXTERNALS)/msgpack

LIB_FOLDER = linux

ARCH = $(shell sh $(UTIL_HOME)/scripts/guessArch.sh)
MACHINE = $(shell uname -m)

ifeq ($(MACHINE), x86_64)
CPPFLAGS = -m64
else
CPPFLAGS = -m32
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

LIB_LIST =  $(NETSENSOR_HOME)/linux/libnetsensor.a \
            $(MOCKETS_HOME)/cpp/$(LIB_FOLDER)/libmockets.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
			$(MSGPACK_HOME)/$(LIB_FOLDER)/libmsgpack.a \
			$(LIBZ_HOME)/$(LIB_FOLDER)/libz.a \
			$(LZMA_HOME)/$(LIB_FOLDER)/liblzma.a \
            -L$(PROTOBUF_HOME)/lib \
            -L$(OPENSSL_HOME)/lib

LD_FLAGS = -lpcap -lprotobuf -lssl -lcrypto -lpthread -ldl

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)
