include Makefile.inc
include Makefile.common.inc
include Makefile.version.inc

MAKEFILE_FOLDER=linux


%.o : ../%.cpp
	$(CPP) $(C11FLAG) -c $(CPPFLAGS) $<

%.o : ../%.cc
	$(CPP) $(C11FLAG) -c $(CPPFLAGS) $<


all: libnetproxy.a netProxy

proto_files :
	$(PROTO_SCRIPT)

libz.a :
	make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER)/ libz.a

liblzma.a :
	make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER)/ liblzma.a

libsecurity.a :
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

libutil.a :
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libmockets.a :
	make -C $(MOCKETS_HOME)/cpp/$(MAKEFILE_FOLDER)/ libmockets.a

libnetsensor.a :
	make -C $(NETSENSOR_HOME)/$(MAKEFILE_FOLDER)/ libnetsensor.a

libnetproxy.a : proto_files $(objects)
	ar -rcv libnetproxy.a $(objects)

makeBinDir :
	mkdir -p $(ACI_HOME)/bin

netProxy: libnetproxy.a libsecurity.a libutil.a libmockets.a libnetsensor.a libz.a liblzma.a makeBinDir ../main.cpp
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
	(make -C $(LZMA_HOME)/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER) clean)
