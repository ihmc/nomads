UTILDIR = ../../../util/cpp

CC  = gcc
CXX = g++

MACHINE = $(shell uname -m)
ifeq ($(MACHINE), x86_64)
CXXFLAGS = -m64
else
CXXFLAGS = -m32
endif

CXXFLAGS += -g -Wall -Wno-unused-parameter -Wno-unknown-pragmas -fPIC -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING \
	    -DLITTLE_ENDIAN_SYSTEM \
	    -I.. -I../../../util/cpp \
	    -I../../../externals/openssl/1.0.2h/linux/include \
	    -I../../../externals/include \
	    -I../../../externals/JDK -I../../../externals/JDK/linux

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

all: libmockets.a

libmockets.a : $(objects) libutil.a libsecurity.a
	ar -rcv libmockets.a $(objects) 

libsecurity.a :
	make -C $(UTILDIR)/linux libsecurity.a

libutil.a :
	make -C $(UTILDIR)/linux libutil.a

clean :
	rm -rf *.o *.a 

%.o : ../%.cpp
	$(CXX) -c $(CXXFLAGS) $<

