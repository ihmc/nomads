CPP = g++

NOMADS_HOME = ../../../../..
ACI_HOME = $(NOMADS_HOME)/aci
MOCKETS_HOME = $(NOMADS_HOME)/mockets
UTIL_HOME = $(NOMADS_HOME)/util
EXTERNALS = $(NOMADS_HOME)/externals
EXTERNALS_LIB = $(EXTERNALS)/lib/$(ARCH)
OPENSSL_HOME = $(EXTERNALS)/openssl/1.0.2h/$(ARCH)
MSGPACK_HOME = $(EXTERNALS)/msgpack
LIBUDT_HOME = $(EXTERNALS)/udt4/src
LIBZ_HOME = $(EXTERNALS)/zlib
LZMA_HOME = $(EXTERNALS)/xz
LZMA_LIB = $(LZMA_HOME)/src/liblzma/api

LIB_FOLDER = linux
MAKEFILE_FOLDER = linux
ARCH = $(shell sh $(UTIL_HOME)/scripts/guessArch.sh)

CPPFLAGS = -g -std=c++11 -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
			-I$(MOCKETS_HOME)/cpp \
			-I$(UTIL_HOME)/cpp \
			-I$(EXTERNALS)/include \
			-I$(EXTERNALS)/openssl/1.0.2h/linux/include

LIB_LIST = $(MOCKETS_HOME)/cpp/$(LIB_FOLDER)/libmockets.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
			$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
			$(MSGPACK_HOME)/$(LIB_FOLDER)/libmsgpack.a \
			$(LIBZ_HOME)/$(LIB_FOLDER)/libz.a \
			$(LZMA_HOME)/$(LIB_FOLDER)/liblzma.a \
			-L$(OPENSSL_HOME)/lib

LD_FLAGS = -lpcap -lssl -lcrypto -lpthread -ldl

sources = $(wildcard /*.cpp)
objects = $(sources: /%./.cpp=%.o)


%.o : /%.cpp
	$(CPP) -c $(CPPFLAGS) $<


all: test

libmockets.a :
	make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER)/ libmockets.a

libutil.a :
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libmsgpack.a :
	make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER)/ libmsgpack.a

libz.a :
	make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER)/ libz.a

liblzma.a :
	make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER)/ liblzma.a

test: libmockets.a libutil.a libz.a liblzma.a libmsgpack.a ../launcher.cpp
	$(CPP) $(CPPFLAGS) \
	../launcher.cpp \
	$(LIB_LIST) $(LD_FLAGS) \
	-o test

clean :
	rm -rf *.o *.a test

cleanall: clean
	(make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBUDT_HOME)/ clean)
	(make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER) clean)
