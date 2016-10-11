include Makefile.inc

MAKEFILE_FOLDER=linux

%.o : ../%.cpp
	$(CPP) -c $(CPPFLAGS) $<

all: libacinetproxy.a netProxy

netsensorlib.a :
	make -C $(NETSENSOR_HOME)/$(MAKEFILE_FOLDER)/ libnetsensor.a

libsecurity.a :
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

libutil.a :
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libmockets.a :
	make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER)/ libmockets.a

libmsgpack.a:
	make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER)/ libmsgpack.a

libz.a :
	make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER)/ libz.a

liblzma.a :
	make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER)/ liblzma.a

libacinetproxy.a : $(objects)
	ar -rcv libacinetproxy.a $(objects)

netProxy: libacinetproxy.a libmockets.a libutil.a libsecurity.a libmsgpack.a libz.a liblzma.a netsensorlib.a ../main.cpp
	$(CPP) $(CPPFLAGS) \
	../main.cpp \
	libacinetproxy.a \
	$(LIB_LIST) $(LD_FLAGS) \
	-o netProxy
	cp netProxy $(ACI_HOME)/bin/.

clean :
	rm -rf *.o *.a netProxy

cleanall :
	make clean
	(make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(NETSENSOR_HOME)/$(MAKEFILE_FOLDER) clean)
