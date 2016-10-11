# MAKEFILE FOR TESTING

PROJECT_ROOT = ../../../../../..
NOMADS_HOME = ../../../../..
C45_HOME = $(NOMADS_HOME)/misc/cpp/c45

CPPFLAGS = -g -Wall -DUNIX -DOSX -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
		-I.. \
		-I../../ \
		-I$(NOMADS_HOME)/util/cpp 

LD_FLAGS = -lpthread

SOURCES = $(wildcard ../../*.cpp)
OBJECTS = $(SOURCES:../../%.cpp=%.o)

EXECUTABLE1 = GenerateXmlDataset

all:
	make $(EXECUTABLE1)

%.o : %.cpp
	g++ $(CPPFLAGS) -c $<

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/osx/ libutil.a

libc4.5.a :
	make -C $(C45_HOME)/osx/ libc4.5.a

$(EXECUTABLE1): libutil.a libc4.5.a ../GenerateXmlDataset.cpp
	g++ $(CPPFLAGS) $(LD_FLAGS) ../GenerateXmlDataset.cpp \
	$(C45_HOME)/osx/libc4.5.a \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	-o $(EXECUTABLE1)

clean :
	rm -rf *.o *.a $(EXECUTABLE1) *.gch ../*.gch *.dSYM
