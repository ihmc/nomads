CC  = gcc
CXX = g++

CXXFLAGS = -g -fPIC -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING \
           -DLITTLE_ENDIAN_SYSTEM -I.. -I../.. -I../../../../util/cpp \
           -I../../../../externals/JDK -I../../../../externals/JDK/linux

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

all: libMocketsCWrapper.so 

libMocketsCWrapper.so : $(objects) libutil.a libmockets.a
	$(CC) -shared -Wl,-soname,$@  \
	-lstdc++ \
	$(objects) \
	../../linux/libmockets.a \
	 ../../../../util/cpp/linux/libutil.a \
	-o $@

libutil.a : 
	(cd ../../../../util/cpp/linux; make)

libmockets.a:
	(cd ../../linux; make)
	

clean :
	rm -rf *.o *.so

%.o : ../%.cpp
	$(CXX) -c $(CXXFLAGS) $<

