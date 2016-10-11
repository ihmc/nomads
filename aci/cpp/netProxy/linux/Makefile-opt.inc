# Guess the architecture
ARCH = $(shell sh ./guessArch.sh)

CPP = g++

NOMADS_HOME = ../../../..
ACI_HOME = $(NOMADS_HOME)/aci
EXTERNALS = $(NOMADS_HOME)/externals
EXTERNALS_LIB = $(EXTERNALS)/lib/$(ARCH)
MOCKETS_HOME = $(NOMADS_HOME)/mockets
UTIL_HOME = $(NOMADS_HOME)/util
NETSENSOR_HOME = $(NOMADS_HOME)/misc/cpp/netsensor
MSGPACK_HOME = $(EXTERNALS)/msgpack
LIBZ_HOME = $(EXTERNALS)/zlib
LZMA_HOME = $(EXTERNALS)/xz
LZMA_LIB = $(LZMA_HOME)/src/liblzma/api

LIB_FOLDER = linux
LD_FLAGS = -ldl -lpcap -lpthread -lprotobuf -lssl -lcrypto

CPPFLAGS = -O2 -DUNIX -DLINUX -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
			-I.. -I../.. \
			-I$(EXTERNALS)/include/pcap \
			-I$(EXTERNALS)/include \
			-I$(UTIL_HOME)/cpp \
			-I$(UTIL_HOME)/cpp/net \
			-I$(MOCKETS_HOME)/cpp \
			-I$(NOMADS_HOME)/externals/protobuf/linux/install3.00-beta-2/include \
			-I$(NETSENSOR_HOME) \

LIB_LIST = $(MOCKETS_HOME)/cpp/$(LIB_FOLDER)/libmockets.a \
			-L/usr/local/lib/libprotobuf.so \
			$(NETSENSOR_HOME)/linux/libnetsensor.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
			$(EXTERNALS_LIB)/libcrypto.a \
			$(MSGPACK_HOME)/$(LIB_FOLDER)/libmsgpack.a \
			$(LIBZ_HOME)/$(LIB_FOLDER)/libz.a \
			$(LZMA_HOME)/$(LIB_FOLDER)/liblzma.a \
			-L$(NOMADS_HOME)/externals/lib/$(ARCH)/

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)
