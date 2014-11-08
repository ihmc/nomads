include Makefile.inc

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
	rm -rf *.o *.a

ThreadTest : ThreadTest.o libutil.a
	$(cpp) -lpthread -o a.out ThreadTest.o libutil.a
