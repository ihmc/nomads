#
# Makefile
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

include Makefile.inc

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../%.c
	$(cpp) -c $(cppflags) $<

%.o : ../graph/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../net/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../security/%.cpp
	$(cpp) -c  $(cppflags) $<

%.o : ../comm/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../proxy/%.cpp
	$(cpp) -c $(cppflags) $<

all: libutil.a libsecurity.a 

libutil.a : $(objects) $(graphobjects) $(netobjects) $(commobjects) $(proxyobjects)
	ar -rcv libutil.a $(objects) $(graphobjects) $(netobjects) $(commobjects) $(proxyobjects)

libsecurity.a :  $(securityobjects)
	ar -rcv libsecurity.a $(securityobjects) 

libutil.so: $(objects) $(graphobjects) $(netobjects) $(commobjects) $(proxyobjects)
	$(cpp) -shared -o libutil.so $(objects) $(graphobjects) $(netobjects) $(commobjects) $(proxyobjects)

libsecurity.so: $(securityobjects)
	$(cpp) -shared -o libutil.so $(securityobjects)

clean :
	rm -rf *.o *.a

ThreadTest : ThreadTest.o libutil.a
	$(cpp) -lpthread -o a.out ThreadTest.o libutil.a
