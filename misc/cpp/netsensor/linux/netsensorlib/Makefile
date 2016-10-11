#
# Makefile
# author: Roberto Fronteddu	(rfronteddu@ihmc.us)
#
CPP = g++
NOMADS_HOME_NETSENSOR = ../../../../..
CPPFLAGS = -g -DUNIX -DLINUX -DLITTLE_ENDIAN_SYSTEM  \
	-I$(NOMADS_HOME_NETSENSOR)/externals/include/pcap \
	-I$(NOMADS_HOME_NETSENSOR)/externals/include \
	-I$(NOMADS_HOME_NETSENSOR)/util/cpp \
	-I$(NOMADS_HOME_NETSENSOR)/util/cpp/net \
	-I$(NOMADS_HOME_NETSENSOR)/util/cpp/linux

SOURCES = NetSensor.cpp NetworkInterface.cpp PCapInterface.cpp

OBJECTS = NetSensor.o NetworkInterface.o PCapInterface.o 			

NetSensor.o: ../../NetSensor.cpp
	$(CPP) -c $(CPPFLAGS) $<

NetworkInterface.o: ../../NetworkInterface.cpp
	$(CPP) -c $(CPPFLAGS) $<

PCapInterface.o: ../../PCapInterface.cpp
	$(CPP) -c $(CPPFLAGS) $<

all: libnetsensor.a   	

libnetsensor.a : $(OBJECTS)
		ar -rcv libnetsensor.a $(OBJECTS)

libutil.a :
	make -C $(NOMADS_HOME_NETSENSOR)/util/cpp/linux libutil.a

clean :
	rm -rf *.o *.a