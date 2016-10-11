# Makefile for netutils

CPP = g++
NOMADS_HOME = ../../../..
CPPFLAGS = -g -DUNIX -DLITTLE_ENDIAN_SYSTEM  \
            -I.. -I/usr/include/vmware-vix \
            -I$(NOMADS_HOME)/util/cpp \
            -I$(NOMADS_HOME)/util/cpp/net \
            -I$(NOMADS_HOME)/util/cpp/osx

ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
MakefileArch=../../../../util/scripts/Makefile.$(ARCH)
include $(MakefileArch)

CPPFLAGS += $(ARCH_FLAGS)

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

all: UDPSender UDPReceiver MCastSender MCastReceiver SleepTest WaitTest IPConverter

UDPSender: UDPSender.o libutil.a
	$(CPP) -lpthread  UDPSender.o \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o UDPSender
     
UDPReceiver : UDPReceiver.o libutil.a
	$(CPP) -lpthread UDPReceiver.o \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o UDPReceiver

MCastSender: MCastSender.o libutil.a
	$(CPP) -lpthread  MCastSender.o \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o MCastSender

MCastReceiver: MCastReceiver.o libutil.a
	$(CPP) -lpthread  MCastReceiver.o \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o MCastReceiver

SleepTest : SleepTest.o libutil.a
	$(CPP) -lpthread SleepTest.o \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o SleepTest

WaitTest : WaitTest.o libutil.a
	$(CPP) -lpthread WaitTest.o \
 	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o WaitTest

IPConverter : IPConverter.o libutil.a
	$(CPP) -lpthread IPConverter.o \
 	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o IPConverter

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/osx libutil.a

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

clean :
	rm -rf *.o *.a UDPSender UDPReceiver MCastSender MCastReceiver SleepTest WaitTest IPConverter

cleanall : clean
	make -C $(NOMADS_HOME)/util/cpp/osx clean

cleanall : clean
	make -C $(NOMADS_HOME)/util/cpp/osx clean
