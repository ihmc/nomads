UTILDIR = ../../../util/cpp

CC  = gcc
CXX = g++

CXXFLAGS = -g -fPIC -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING \
           -DLITTLE_ENDIAN_SYSTEM \
	   -I.. -I../../../util/cpp \
           -I../../../externals/JDK -I../../../externals/JDK/linux

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

all: libmockets.a libutil.a libsecurity.a  

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

