.PHONY: clean

GOPATH := ${PWD}/
export GOPATH

CXXFLAGS = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DRTT_TIMESTAMP \
           -DLITTLE_ENDIAN_SYSTEM -fPIC -I../../../externals/openssl/1.0.2h/linux/include -I../ \
           -I../../../externals/include \
           -I../../../util/cpp \
           -I../../../util/cpp/security 

#openssl static libraries compiled with -fPIC flag
EXTRA_LIBS = ../jni/openssl_linux/libssl.a
EXTRA_LIBS += ../jni/openssl_linux/libcrypto.a
LDFLAGS=-ldflags "-L src/ihmc.us/mocket-go/wrapper"


wrappers = $(wildcard src/ihmc.us/mocket-go/wrapper/*Wrapper.cpp)
wrappers_objs = $(wrappers:src/ihmc.us/mocket-go/wrapper/%.cpp=src/ihmc.us/mocket-go/wrapper/%.o)

all: libmockets.a libutil.a libsecurity.a libmocketgowrapper.so mocket-go

libmockets.a : $(objects) libutil.a libsecurity.a 
	make -C ../linux

libsecurity.a :
	make -C ../../../util/cpp/linux libsecurity.a


libutil.a : 
	make -C ../../../util/cpp/linux

libmocketgowrapper.so: libutil.a libsecurity.a libmockets.a $(wrappers_objs)
	@echo $(wrappers_objs)
	$(CXX) -Wall -Wextra -Werror -shared -o src/ihmc.us/mocket-go/wrapper/libmocketgowrapper.so \
	$(wrappers_objs) \
	../linux/libmockets.a \
	../../../util/cpp/linux/libsecurity.a \
	../../../util/cpp/linux/libutil.a \
	$(EXTRA_LIBS)

mocket-go: libmocketgowrapper.so
	@echo $(GOPATH)
	cd src/ihmc.us/mocket-go/mocket; go get ihmc.us/mocket-go/mocket

clean :
	(make -C ../../../util/cpp/linux clean)
	(make -C ../linux clean)
	rm -f src/ihmc.us/mocket-go/wrapper/*.o src/ihmc.us/mocket-go/wrapper/*.so

%.o : src/ihmc.us/mocket-go/wrapper/%.cpp
	$(CXX) -c $(CXXFLAGS) $<

