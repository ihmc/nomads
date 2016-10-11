SQLITE_HOME = ../..

cpp = g++
archflags=-DUNIX -DLINUX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -lpthread -lnsl -ldl

options=-Wall -Wextra
INCLUDES=-I.. -I$(SQLITE_HOME)/

cppflags= -g $(archflags) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
#sources = $(wildcard ../*.cpp)
#objects = $(sources:../%.cpp=%.o)
LIBS=$(SQLITE_HOME)/linux/libsqlite3.a

all : test

libsqlite.a :
	make -C $(SQLITE_HOME)/linux libsqlite3.a

test:   $(objects) libsqlite.a
	$(cpp) $(cppflags) $(LD_FLAGS) \
	../test.cpp \
	$(SQLITE_HOME)/linux/libsqlite3.a \
	-o Test

clean :
	rm -rf *.o *.a *.gch ../*.gch Test
