# Makefile for geoutils

CPP = g++
NOMADS_HOME = ../../../..
CPPFLAGS = -g -DUNIX -DLINUX -DLITTLE_ENDIAN_SYSTEM  \
            -I.. -I/usr/include/vmware-vix \
            -I$(NOMADS_HOME)/util/cpp \
            -I$(NOMADS_HOME)/util/cpp/net \
            -I$(NOMADS_HOME)/util/cpp/linux

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

all: GreatCircleDistance

GreatCircleDistance: GreatCircleDistance.o libutil.a
			$(CPP) GreatCircleDistance.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o GreatCircleDistance

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux libutil.a

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

clean :
	rm -rf *.o *.a GreatCircleDistance

cleanall : clean
	make -C $(NOMADS_HOME)/util/cpp/linux clean
