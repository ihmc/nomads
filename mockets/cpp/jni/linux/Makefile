CC  = gcc
CXX = g++

CXXFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DRTT_TIMESTAMP \
           -DLITTLE_ENDIAN_SYSTEM -fPIC -I../../../../externals/openssl/1.1.0f/linux64/include -I../.. \
           -I../../../../externals/include \
           -I../../../../util/cpp \
           -I../../../../util/cpp/security \
           -I../../../../externals/JDK -I../../../../externals/JDK/linux 

#openssl static libraries compiled with -fPIC flag
EXTRA_LIBS = ../../../../externals/openssl/1.0.2h/linux64/lib/libssl.a
EXTRA_LIBS += ../../../../externals/openssl/1.0.2h/linux64/lib/libcrypto.a


wrappers = $(wildcard ../*Wrapper.cpp)
wrappers_objs = $(wrappers:../%.cpp=%.o)

all: libmockets.a libutil.a libsecurity.a libmocketsjavawrapper.so 

libmockets.a : $(objects) libutil.a libsecurity.a 
	make -C ../../linux

libsecurity.a :
	make -C ../../../../util/cpp/linux libsecurity.a


libutil.a : 
	make -C ../../../../util/cpp/linux

libmocketsjavawrapper.so: $(wrappers_objs) libutil.a libsecurity.a libmockets.a 
	$(CXX) -Wall -Wextra -Werror -shared -o ../../../lib/libmocketsjavawrapper.so \
	$(wrappers_objs) \
	../../linux/libmockets.a \
	../../../../util/cpp/linux/libsecurity.a \
	../../../../util/cpp/linux/libutil.a \
	$(EXTRA_LIBS)


clean :
	(make -C ../../../../util/cpp/linux clean)
	(make -C ../../linux clean)
	rm -rf *.o *.a ../../../bin/libmocketsjavawrapper.so

%.o : ../%.cpp
	$(CXX) -c $(CXXFLAGS) $<

