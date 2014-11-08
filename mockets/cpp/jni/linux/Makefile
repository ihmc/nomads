CC  = gcc
CXX = g++

CXXFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DRTT_TIMESTAMP \
           -DLITTLE_ENDIAN_SYSTEM -fPIC -I../.. -I../../../../util/cpp \
           -I../../../../externals/JDK -I../../../../externals/JDK/linux \
           -I../../../../util/cpp/security

ifdef USE_X64_LIBS
  EXTERNALS_LIB = ../../../../externals/lib/linux-x86_64
else
  EXTERNALS_LIB = ../../../../externals/lib/linux-x86
endif

ifdef USE_SYSTEM_LIBS
  OPENSSL_LIBS = -lcrypto -lssl
else
  OPENSSL_LIBS = $(EXTERNALS_LIB)/libcrypto.a $(EXTERNALS_LIB)/libssl.a 
endif


wrappers = $(wildcard ../*Wrapper.cpp)
wrappers_objs = $(wrappers:../%.cpp=%.o)

all: libmocketsjavawrapper.so

libmockets.a : $(objects) libutil.a libsecurity.a 
	make -C ../../linux

libsecurity.a :
	make -C ../../../../util/cpp/linux libsecurity.a


libutil.a : 
	make -C ../../../../util/cpp/linux

libmocketsjavawrapper.so: $(wrappers_objs) libutil.a libsecurity.a libmockets.a 
	$(CXX) -shared -o ../../../bin/libmocketsjavawrapper.so \
	$(wrappers_objs) \
	../../linux/libmockets.a \
	../../../../util/cpp/linux/libsecurity.a \
	../../../../util/cpp/linux/libutil.a \
	$(OPENSSL_LIBS) 

clean :
	(make -C ../../../../util/cpp/linux clean)
	(make -C ../../linux clean)
	rm -rf *.o *.a ../../../bin/libmocketsjavawrapper.so

%.o : ../%.cpp
	$(CXX) -c $(CXXFLAGS) $<

