TARGETS = RTTNetem RTTnetfilter RTTnistnet
CFLAGS  = -g -O2 -Wall
CPPFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
		-I.. -I$(NOMADS_HOME)/misc/cpp/queuemanager/singlethreaded -I$(NOMADS_HOME)/util/cpp -I$(NOMADS_HOME)/mockets/cpp
LDFLAGS = /usr/local/lib/libnetlink.a
LDCPPFLAGS = -lpthread -lipq
NOMADS_HOME ?= ../../../../../
CC = gcc
CPP = g++

all: $(TARGETS) 

.PHONY: clean

netem_libnetlink.o: ../netem_libnetlink.c ../wnetemu.h

RTTNetem: RTTNetem.o netem_libnetlink.o
	$(CC) -o $@ RTTNetem.o netem_libnetlink.o $(LDFLAGS)

RTTnetfilter: RTTnetfilter.o libqueuemanager.a libutil.a
	$(CPP) -o $@ RTTnetfilter.o $(NOMADS_HOME)/misc/cpp/queuemanager/singlethreaded/linux//libqueuemanager.a $(NOMADS_HOME)/util/cpp/linux/libutil.a $(LDCPPFLAGS)

RTTnistnet: RTTnistnet.o libutil.a
	$(CPP) -o $@ RTTnistnet.o $(NOMADS_HOME)/util/cpp/linux/libutil.a

libqueuemanager.a:
	(cd $(NOMADS_HOME)/misc/cpp/queuemanager/singlethreaded/linux/; make)

libutil.a : 
	(cd $(NOMADS_HOME)/util/cpp/linux; make)

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

%.o : ../%.c
	$(CC) -c $(CFLAGS) $<
	
clean: 
	rm -f $(TARGETS) *~ *.o
	(cd $(NOMADS_HOME)/util/cpp/linux; make clean)	
	(cd $(NOMADS_HOME)/misc/cpp/queuemanager/singlethreaded/linux/; make clean)
	
