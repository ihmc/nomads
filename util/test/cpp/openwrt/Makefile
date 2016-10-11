SDK=$(HOME)/OpenWrt-SDK-Linux-i686-1/staging_dir_mipsel/
CPP = $(SDK)/bin/mipsel-linux-g++

NOMADS_HOME=../../../../
ACI_HOME=$(NOMADS_HOME)/aci
LD_LIBRARY_PATH=$(SDK)/lib

CPPFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM  \
		   -I$(NOMADS_HOME)/util/cpp 

LD_FLAGS = -lnsl -lpthread -ldl

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

all: multicast_echo wildNetIFs netIFs multicast_receiver multicast_sender

multicast_echo: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_echo.cpp \
		$(NOMADS_HOME)/util/cpp/openwrt/libutil.a \
		-o multicast_echo

netIFs: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../netIFs.cpp \
		$(NOMADS_HOME)/util/cpp/openwrt/libutil.a \
		-o netIFs

wildNetIFs: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../wildNetIFs.cpp \
		$(NOMADS_HOME)/util/cpp/openwrt/libutil.a \
		-o wildNetIFs

multicast_receiver: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_receiver.cpp \
		$(NOMADS_HOME)/util/cpp/openwrt/libutil.a \
		-o multicast_receiver

multicast_sender: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_sender.cpp \
		$(NOMADS_HOME)/util/cpp/openwrt/libutil.a \
		-o multicast_sender

libutil.a :
	(cd $(NOMADS_HOME)/util/cpp/openwrt; make)

clean :
	(cd $(NOMADS_HOME)/util/cpp/openwrt; make clean)
	rm -rf *.o *.a multicast_echo wildNetIFs netIFs multicast_receiver multicast_sender

