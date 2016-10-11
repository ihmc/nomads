# Makefile for netutils

CPP = g++
NOMADS_HOME = ../../../..
CPPFLAGS = -g -DUNIX -DLINUX -DLITTLE_ENDIAN_SYSTEM  \
            -I.. -I/usr/include/vmware-vix \
            -I$(NOMADS_HOME)/util/cpp \
            -I$(NOMADS_HOME)/util/cpp/net \
            -I$(NOMADS_HOME)/util/cpp/linux

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

all: UDPSender UDPReceiver MCastSender MCastReceiver SleepTest WaitTest IPConverter UDPSender-MT

UDPSender: NetMetrics.o PacketFactory.o UDPSender.o libutil.a
			$(CPP) UDPSender.o PacketFactory.o NetMetrics.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o UDPSender

UDPReceiver : NetMetrics.o LinkStateClassifiers.o LossClassifiers.o ReceiverMetrics.o PacketFactory.o UDPReceiver.o libutil.a
			$(CPP) UDPReceiver.o PacketFactory.o ReceiverMetrics.o LossClassifiers.o LinkStateClassifiers.o NetMetrics.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o UDPReceiver

TCPSender: ArgParse.o NetMetrics.o PacketFactory.o TCPSender.o libutil.a
			$(CPP) TCPSender.o ArgParse.o PacketFactory.o NetMetrics.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o TCPSender

TCPReceiver : ArgParse.o NetMetrics.o LinkStateClassifiers.o LossClassifiers.o ReceiverMetrics.o PacketFactory.o TCPReceiver.o libutil.a
			$(CPP) TCPReceiver.o ArgParse.o PacketFactory.o ReceiverMetrics.o LossClassifiers.o LinkStateClassifiers.o NetMetrics.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o TCPReceiver

MCastSender:  MCastSender.o libutil.a
			$(CPP) MCastSender.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o MCastSender

MCastReceiver: MCastReceiver.o libutil.a
			$(CPP) MCastReceiver.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o MCastReceiver

SleepTest : SleepTest.o libutil.a
			$(CPP) SleepTest.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o SleepTest

WaitTest : WaitTest.o libutil.a
			$(CPP) WaitTest.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o WaitTest

IPConverter : IPConverter.o libutil.a
			$(CPP) IPConverter.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o IPConverter

UDPSender-MT: UDPSender-MT.o libutil.a
			$(CPP) UDPSender-MT.o \
			$(NOMADS_HOME)/util/cpp/linux/libutil.a \
			-lpthread \
			-o UDPSender-MT

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux libutil.a

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

clean :
	rm -rf *.o *.a UDPSender UDPReceiver MCastSender MCastReceiver SleepTest WaitTest IPConverter UDPSender-MT

cleanall : clean
	make -C $(NOMADS_HOME)/util/cpp/linux clean
