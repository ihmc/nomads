# Makefile for netsensor
#-Wl, -R/path/to/libdir
CPP = g++
NOMADS_HOME = ../../../..
CPPFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM  \
			-I.. \
			-I$(NOMADS_HOME)/externals/include/pcap \
			-I$(NOMADS_HOME)/externals/include \
			-I$(NOMADS_HOME)/util/cpp \
			-I$(NOMADS_HOME)/util/cpp/net \
			-I$(NOMADS_HOME)/util/cpp/linux \
			-I$(NOMADS_HOME)/externals/protobuf/linux/install3.00-beta-2/include
LIB_LIST =  $(NOMADS_HOME)/util/cpp/linux/libutil.a \
		-L/usr/local/lib/libprotobuf.so
LD_FLAGS = -lpcap -lprotobuf -lpthread -pthread
        
sources = ../PacketStructure.cpp ../NetworkInterface.cpp ../PCapInterface.cpp ../container.pb.cc ../grump.pb.cc ../info.pb.cc ../node.pb.cc ../networkhealth.pb.cc ../topology.pb.cc ../traffic.pb.cc ../NetSensor.cpp ../NetSensorLauncher.cpp
objects = ../PacketStructure.o ../NetworkInterface.o ../PCapInterface.o ../container.pb.o ../grump.pb.o ../info.pb.o ../node.pb.o ../networkhealth.pb.o ../topology.pb.o ../traffic.pb.o ../NetSensor.o ../NetSensorLauncher.o
all: NetSensorLauncher
NetSensorLauncher: libnetsensor.a libutil.a ../NetSensorLauncher.cpp
	$(CPP) $(CPPFLAGS) -L/usr/local/lib ../NetSensorLauncher.cpp libnetsensor.a \
	$(LIB_LIST) $(LD_FLAGS) \
	-o NetSensorLauncher
libnetsensor.a : $(objects)
	ar -rcv libnetsensor.a $(objects)
libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux/ libutil.a
%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<
clean :
	rm -rf ../*.o *../*.a NetSensorLauncher
cleanall :
	make clean
	(make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
