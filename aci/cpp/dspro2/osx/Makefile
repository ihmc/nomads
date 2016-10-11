# MAKEFILE FOR DISSERVICEPRO PROJECT
include Makefile.inc

dsprosources = $(wildcard ../*.cpp)
dsproobjects = $(dsprosources:../%.cpp=%.o)

awarenesssources = $(wildcard ../awareness/*.cpp)
awarenessobjects = $(awarenesssources:../awareness/%.cpp=%.o)

rankingsources = $(wildcard ../ranking/*.cpp)
rankingobjects = $(rankingsources:../ranking/%.cpp=%.o)

storagesources = $(wildcard ../storage/*.cpp)
storageobjects = $(storagesources:../storage/%.cpp=%.o)

schedulersources = $(wildcard ../scheduler/*.cpp)
schedulerobjects = $(schedulersources:../scheduler/%.cpp=%.o)

proxysources = $(wildcard ../proxy/*.cpp)
proxyobjects = $(proxysources:../proxy/%.cpp=%.o)

commsources = $(wildcard ../comm/*.cpp)
commobjects = $(commsources:../comm/%.cpp=%.o)

disservicecommsources = $(wildcard ../comm/disservice/*.cpp)
disservicecommobjects = $(disservicecommsources:../comm/disservice/%.cpp=%.o)

mocketscommsources = $(wildcard ../comm/mockets/*.cpp)
mocketscommobjects = $(mocketscommsources:../comm/mockets/%.cpp=%.o)

querycontrollerssources = $(wildcard ../controllers/query/*.cpp)
querycontrollersobjects = $(querycontrollerssources:../controllers/query/%.cpp=%.o)

%.o : ../ranking/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../awareness/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../storage/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../scheduler/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../proxy/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../comm/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../comm/disservice/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../comm/mockets/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../controllers/query/%.cpp
	g++ $(CPPFLAGS) -c $<

%.o : ../%.cpp
	g++ $(CPPFLAGS) -c $<

all: $(EXECUTABLE)

libsqlite3.a :
	make -C $(SQLITE_HOME)/osx/ libsqlite3.a

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/osx/ libutil.a

libdisservice.a :
	make -C $(DISSERVICE_HOME)/osx/ libdisservice.a

libmockets.a :
	make -C $(MOCKETS_HOME)/cpp/osx/ libmockets.a

libc4.5.a :
	make -C $(C45_HOME)/osx/ libc4.5.a

liblcppdc.a :
	make -C	$(LCPPDC_HOME)/$(MAKEFILE_FOLDER)/ liblcppdc.a

libdspro.a:  $(rankingobjects) $(awarenessobjects) $(schedulerobjects) $(storageobjects) $(disservicecommobjects) $(mocketscommobjects) $(dsproobjects) $(commobjects) $(proxyobjects) $(querycontrollersobjects) libutil.a libsqlite3.a libdisservice.a libmockets.a libc4.5.a
	ar -rcv libdspro.a $(rankingobjects) $(awarenessobjects) $(schedulerobjects) $(storageobjects) $(disservicecommobjects) $(mocketscommobjects) $(dsproobjects) $(commobjects) $(proxyobjects) $(querycontrollersobjects)

$(EXECUTABLE): libdspro.a libutil.a ../DSProMain.cpp
	g++ $(CPPFLAGS) ../DSProMain.cpp \
	libdspro.a \
	$(MOCKETS_HOME)/cpp/osx/libmockets.a \
	$(DISSERVICE_HOME)/osx/libdisservice.a \
	$(C45_HOME)/osx/libc4.5.a \
	$(LCPPDC_HOME)/osx/liblcppdc.a \
	$(CHUNKING_HOME)/osx/libchunking.a \
	$(JASPER_HOME)/osx/libjasper.a \
	$(JPEG_HOME)/osx/libjpeg.a \
	$(PNG_HOME)/osx/libpng.a \
	$(NOMADS_HOME)/util/cpp/osx/libsecurity.a \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a \
	$(SQLITE_HOME)/osx/libsqlite3.a \
	$(TINYXPATH_HOME)/libtinyxpath.a \
	$(EXTERNALS_LIB)/libz.a \
	$(EXTERNALS_LIB)/libcrypto.a \
	$(LD_FLAGS) \
	-o $(EXECUTABLE)

#  Make all

clean :
	rm -rf *.o *.a *.gch ../*.gch *.dSYM $(EXECUTABLE) libdspro.a

cleanall: clean
	make -C $(DISSERVICE_HOME)/osx/ cleanall
	make -C $(C45_HOME)/osx/ clean
