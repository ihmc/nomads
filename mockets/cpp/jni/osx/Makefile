CC  = gcc

CXXFLAGS = -g -DUNIX -DOSX -DENABLE_DEBUG -DERROR_CHECKING \
           -DLITTLE_ENDIAN_SYSTEM -I../.. -I../../../../util/cpp \
           -I../../../../util/cpp/security \
	   -I../../../../externals/JDK -I../../../../externals/JDK/osx \
	   -I/System/Library/Frameworks/JavaVM.framework/Headers

wrappers = $(wildcard ../*Wrapper.cpp)
wrappers_objs = $(wrappers:../%.cpp=%.o)

all: libmocketsjavawrapper.jnilib

libmockets.a : 
	(make -C ../../osx)

libutil.a : 
	(make -C ../../../../util/cpp/osx)

libsecurity.a:
	(make -C ../../../../util/cpp/osx)

libmocketsjavawrapper.jnilib : $(wrappers_objs) libutil.a libmockets.a libsecurity.a
	$(CXX) -dynamiclib -o ../../../bin/libmocketsjavawrapper.jnilib \
	$(wrappers_objs) \
	../../osx/libmockets.a  \
	../../../../util/cpp/osx/libutil.a \
	../../../../util/cpp/osx/libsecurity.a \
	../../../../externals/lib/osx-intel/libcrypto.a \
	../../../../externals/lib/osx-intel/libssl.a

clean :
	(make -C ../../../../util/cpp/osx clean)
	(make -C ../../osx clean)
	rm -rf *.o *.a ../../../bin/libmocketsjavawrapper.so ../../../bin/libmocketsjavawrapper.jnilib

%.o : ../%.cpp
	$(CXX) -c $(CXXFLAGS) $<

