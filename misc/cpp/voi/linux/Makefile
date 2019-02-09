# MAKEFILE FOR DISSERVICEPRO PROJECT
include Makefile.inc

voisources = $(wildcard ../*.cpp)
voiobjects = $(voisources:../%.cpp=%.o)

coresources = $(wildcard ../core/*.cpp)
coreobjects = $(coresources:../core/%.cpp=%.o)

ctxtsources = $(wildcard ../ctxt/*.cpp)
ctxtobjects = $(ctxtsources:../ctxt/%.cpp=%.o)

utilsources = $(wildcard ../util/*.cpp)
utilobjects = $(utilsources:../util/%.cpp=%.o)

allobjs = $(coreobjects) $(ctxtobjects) $(utilobjects) $(voiobjects) 

%.o : ../util/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../core/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../ctxt/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../%.cpp
	$(CPP) $(CPPFLAGS) -c $<

all: $(EXECUTABLE)

libvoi.a:  $(allobjs) libutil.a
	ar -rcv libvoi.a $(allobjs)

libsqlite3.a :
	make -C $(SQLITE_HOME)/linux/ libsqlite3.a

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux/ libutil.a

libc4.5.a :
	make -C $(C45_HOME)/linux/ libc4.5.a

liblcppdc.a :
	make -C	$(LCPPDC_HOME)/$(MAKEFILE_FOLDER)/ liblcppdc.a

libmil2525.a :
	make -C	$(MIL_STD_2525)/$(MAKEFILE_FOLDER)/ libmil2525.a

libz.a :
	make -C $(ZLIB_HOME)/$(MAKEFILE_FOLDER)/ libz.a

libtinyxpath.a :
	make -C $(TINYXPATH_HOME)/$(MAKEFILE_FOLDER)/ libtinyxpath.a

$(EXECUTABLE): libvoi.a libutil.a libsqlite3.a libc4.5.a libmil2525.a liblcppdc.a ../VoiLauncher.cpp
	$(CPP) $(CPPFLAGS) ../VoiLauncher.cpp \
	libvoi.a \
	$(LIBS) \
	$(LD_FLAGS) \
	-o $(EXECUTABLE)

#  Make all

clean :
	rm -rf *.o *.a *.gch ../*.gch *.dSYM $(EXECUTABLE) libvoi.a

cleanall: clean
	make -C $(DISSERVICE_HOME)/linux/ cleanall
	make -C $(C45_HOME)/linux/ clean
