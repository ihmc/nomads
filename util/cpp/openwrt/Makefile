SDK=$(HOME)/OpenWrt-SDK-Linux-i686-1/staging_dir_mipsel/
cpp = $(SDK)/bin/mipsel-linux-g++

cppflags = -g -DUNIX -DLINUX -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -I.. -I../../../externals/include
LD_LIBRARY_PATH=$(SDK)/lib

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

netsources = $(wildcard ../net/*.cpp)
netobjects = $(netsources:../net/%.cpp=%.o)

securitysources = $(wildcard ../security/*.cpp)
securityobjects = $(securitysources:../security/%.cpp=%.o)

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../net/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../security/%.cpp
	$(cpp) -c -I../../../externals/include $(cppflags) $<

libutil.a : $(objects) $(netobjects)
	ar -rcv libutil.a $(objects) $(netobjects)

libsecurity.a :  $(securityobjects)
	ar -rcv libsecurity.a $(securityobjects) 

libutil.so: $(objects) $(netobjects)
	$(cpp) -shared -o libutil.so $(objects) $(netobjects)

libsecurity.so: $(securityobjects)
	$(cpp) -shared -o libutil.so $(securityobjects)

clean :
	rm -rf *.o *.a

