cpp = gcc

cppflags = -g -DUNIX -DOSX -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -I..

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

netsources = $(wildcard ../net/*.cpp)
netobjects = $(netsources:../net/%.cpp=%.o)

securitysources = $(wildcard ../security/*.cpp)
securityobjects = $(securitysources:../security/%.cpp=%.o)

graphsources = $(wildcard ../graph/*.cpp)
graphobjects = $(graphsources:../graph/%.cpp=%.o)

commsources = $(wildcard ../comm/*.cpp)
commobjects = $(commsources:../comm/%.cpp=%.o)

mediasources = $(wildcard ../media/*.cpp)
mediaobjects = $(mediasources:../media/%.cpp=%.o)

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../graph/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../net/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../security/%.cpp
	$(cpp) -c -I../../../externals/include $(cppflags) $<

%.o : ../comm/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../media/%.cpp
	$(cpp) -c $(cppflags) $<

all: libutil.a libsecurity.a

libutil.a : $(objects) $(graphobjects) $(netobjects) $(commobjects) $(mediaobjects)
	ar -rcv libutil.a $(objects) $(graphobjects) $(netobjects) $(commobjects) $(mediaobjects)

libsecurity.a :  $(securityobjects)
	ar -rcv libsecurity.a $(securityobjects) 

libutil.so: $(objects) $(graphobjects) $(netobjects) $(commobjects) $(mediaobjects)
	$(cpp) -shared -o libutil.so $(objects) $(graphobjects) $(netobjects) $(commobjects) $(mediaobjects)

libsecurity.so: $(securityobjects)
	$(cpp) -shared -o libutil.so $(securityobjects)

clean :
	rm -rf *.o *.a *.dylib

