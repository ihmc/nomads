NOMADS_HOME = ../../../../..
ACI_HOME = $(NOMADS_HOME)/aci
UTIL_HOME = $(NOMADS_HOME)/util

#Compiler
CPP = g++

LD_FLAGS = -lpthread
BUILD_FLAGS = -DLINUX

CPPFLAGS = -g -DUNIX $(BUILD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -I../ -I$(UTIL_HOME)/cpp

SOURCES = $(wildcard ../*.cpp)
OBJECTS = $(SOURCES:../%.cpp=%.o)

LD_FLAGS += -lnsl -ldl

all: libaciclient.a

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

libutil.a :
	make -C $(UTIL_HOME)/cpp/linux/ libutil.a

libaciclient.a : $(OBJECTS) libutil.a
	ar -rcv libaciclient.a $(OBJECTS) $(UTIL_HOME)/cpp/linux/libutil.a

clean :
	rm -rf *.o *.a
