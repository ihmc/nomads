# Makefile for netsensor
#-Wl, -R/path/to/libdirl

include Makefile.common.inc
include Makefile.inc

MAKEFILE_FOLDER = linux


%.o : ../%.cpp
	$(CPP) $(C11FLAG) -c $(CPPFLAGS) $<

%.o : ../%.cc
	$(CPP) $(C11FLAG) -c $(CPPFLAGS) $<


all: libnetsensor.a Launcher

libz.a :
	make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER)/ libz.a

libutil.a :
	make -C $(UTIL_HOME)/cpp/linux libutil.a

libnetsensor.a : $(objects)
	ar -rcv libnetsensor.a $(objects)

Launcher: libnetsensor.a libutil.a libz.a ../Launcher.cpp
	$(CPP) $(C11FLAG) $(CPPFLAGS) \
	../Launcher.cpp \
	libnetsensor.a \
	$(LIB_LIST) $(LD_FLAGS) \
	-o Launcher


clean :
	rm -rf *.o ../*.o *../*.a ./*.a Launcher 
	
cleanall :
	make clean
	(make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER) clean)
