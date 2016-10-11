include Makefile.inc

CC  = gcc
CXX = g++

#LD_FLAGS = -lnsl -lpthread -ldl        

all: libnmsjavawrapper.so

%.o : ../%.cpp
	$(CXX) -c $(cppflags) $<

libsecurity.a :
	make -C $(NOMADS_HOME)/util/cpp/linux libsecurity.a
	
libutil.a : 
	make -C $(NOMADS_HOME)/util/cpp/linux

libnms.a : libutil.a libsecurity.a 
	make -C ../../linux

libnmsjavawrapper.so: $(wrappers_objs) libutil.a libsecurity.a libnms.a 
	$(CXX) -shared -o ../../../bin/libgrpmgrjavawrapper.so \
	$(wrappers_objs) \
	$(LIBS)
#	../../linux/libgrpmgr.a \
#	../../../../../util/cpp/linux/libsecurity.a \
#	../../../../../util/cpp/linux/libutil.a
#	     $(OPENSSL_LIBS) 


clean:
	(make -C $(NOMADS_HOME)/util/cpp/linux clean)
	(make -C ../../linux clean)
	rm -rf *.o *.a ../../../bin/libnmsjavawrapper.so
