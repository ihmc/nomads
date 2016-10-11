CPP = g++

NOMADS_HOME=../../../..
ACI_HOME=$(NOMADS_HOME)/aci

CPPFLAGS = -g -DUNIX -DOSX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM  \
		   -I$(NOMADS_HOME)/util/cpp 

LD_FLAGS = -lpthread -ldl

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

all: multicast_echo multicast_receiver multicast_sender netIFs netmsgsvc

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

multicast_echo: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_echo.cpp \
        $(NOMADS_HOME)/util/cpp/osx/libutil.a \
	    -o multicast_echo

three_string_hashtable: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../three_string_hashtable.cpp \
		$(NOMADS_HOME)/util/cpp/osx/libutil.a \
		-o three_string_hashtable

multicast_receiver: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_receiver.cpp \
		$(NOMADS_HOME)/util/cpp/osx/libutil.a \
		-o multicast_receiver

multicast_sender: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../multicast_sender.cpp \
		$(NOMADS_HOME)/util/cpp/osx/libutil.a \
		-o multicast_sender

netIFs: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../netIFs.cpp \
		$(NOMADS_HOME)/util/cpp/osx/libutil.a \
		-o netIFs

event_notifier_driver: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../event_notifier_driver.cpp \
		$(NOMADS_HOME)/util/cpp/osx/libutil.a \
		-o event_notifier_driver

event_monitor_driver: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../event_monitor_driver.cpp \
		$(NOMADS_HOME)/util/cpp/osx/libutil.a \
		-o event_monitor_driver

netmsgsvc: libutil.a
	$(CPP) $(CPPFLAGS) $(LD_FLAGS) \
		../NetworkMessageServiceTest.cpp \
		$(NOMADS_HOME)/util/cpp/osx/libutil.a \
		-o netmsgsvc

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/osx libutil.a

clean :
	make -C $(NOMADS_HOME)/util/cpp/osx clean
	rm -rf *.o *.a *.dSYM multicast_echo multicast_receiver multicast_sender netIFs netmsgsvc
