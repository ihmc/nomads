# MAKEFILE FOR TESTING

PROJECT_ROOT = ../../../../../..
NOMADS_HOME = ../../../../..
C45_HOME = $(NOMADS_HOME)/misc/cpp/c45

CPPFLAGS = -g -Wall -DUNIX -DLINUX -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
		-I.. \
		-I../../ \
		-I$(NOMADS_HOME)/util/cpp 

LD_FLAGS = -lpthread -lnsl -ldl

SOURCES = $(wildcard ../../*.cpp)
OBJECTS = $(SOURCES:../../%.cpp=%.o)

EXECUTABLE1 = GenerateXmlDataset

all: $(EXECUTABLE1)

%.o : %.cpp
	g++ $(CPPFLAGS) -c $<

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux/ libutil.a

libc4.5.a :
	make -C $(C45_HOME)/linux/ libc4.5.a

$(EXECUTABLE1): libutil.a libc4.5.a ../GenerateXmlDataset.cpp
	g++ $(CPPFLAGS) ../GenerateXmlDataset.cpp \
	$(C45_HOME)/linux/libc4.5.a \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
	$(LD_FLAGS) \
	-o $(EXECUTABLE1)

clean :
	rm -rf *.o *.a $(EXECUTABLE1) *.gch ../*.gch *.dSYM
