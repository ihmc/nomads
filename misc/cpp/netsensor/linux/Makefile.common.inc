CPP = g++
C11FLAG = -std=c++11

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util
EXTERNALS_HOME = $(NOMADS_HOME)/externals
LIBZ_HOME = $(EXTERNALS_HOME)/zlib

ARCH = $(shell sh $(UTIL_HOME)/scripts/guessArch.sh)
MACHINE = $(shell uname -m)

CPPFLAGS = -DUNIX -DLINUX -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
            -I.. \
            -I$(NOMADS_HOME)/externals/protobuf/3.6.1/include \
            -I$(NOMADS_HOME)/externals/include/pcap \
            -I$(NOMADS_HOME)/externals/include \
            -I$(NOMADS_HOME)/util/cpp/net \
            -I$(NOMADS_HOME)/util/cpp

LIB_LIST = $(NOMADS_HOME)/util/cpp/linux/libutil.a \
            $(EXTERNALS_HOME)/protobuf/3.6.1/lib/$(ARCH)/libprotobuf.a \
            $(LIBZ_HOME)/linux/libz.a

LD_FLAGS = -lpcap -lpthread -pthread

sources = $(wildcard ../*.cpp) $(wildcard ../*.cc)
objects = $(sources:../%.cpp=%.o) $(sources:../%.cc=%.o)
