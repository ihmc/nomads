include Makefile.common.inc
include Makefile.inc

MAKEFILE_FOLDER=linux


%.o : ../%.cpp
	$(CPP) $(C11FLAG) -c $(CPPFLAGS) $<


all: libnetproxy.a netProxy

libz.a :
	make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER)/ libz.a

liblzma.a :
	make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER)/ liblzma.a

libmsgpack.a:
	make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER)/ libmsgpack.a

libsecurity.a :
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

libutil.a :
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libmockets.a :
	make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER)/ libmockets.a

libnetsensor.a :
	make -C $(NETSENSOR_HOME)/$(MAKEFILE_FOLDER)/ libnetsensor.a

libnetproxy.a : $(objects)
	ar -rcv libnetproxy.a $(objects)

netProxy: libnetproxy.a libsecurity.a libutil.a libmockets.a libnetsensor.a libmsgpack.a libz.a liblzma.a ../main.cpp
	$(CPP) $(C11FLAG) $(CPPFLAGS) \
	../main.cpp \
	libnetproxy.a \
	$(LIB_LIST) $(LD_FLAGS) \
	-o netProxy
	cp netProxy $(ACI_HOME)/bin/.


clean :
	rm -rf *.o *.a netProxy

cleanall :
	make clean
	(make -C $(NETSENSOR_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER) clean)
