# MAKEFILE FOR DISSERVICEPRO PROJECT
include Makefile.inc

dsprosources = $(wildcard ../*.cpp)
dsproobjects = $(dsprosources:../%.cpp=%.o)

awarenesssources = $(wildcard ../awareness/*.cpp)
awarenessobjects = $(awarenesssources:../awareness/%.cpp=%.o)

awarenesspartssources = $(wildcard ../awareness/parts/*.cpp)
awarenesspartsobjects = $(awarenesspartssources:../awareness/parts/%.cpp=%.o)

rankingsources = $(wildcard ../ranking/*.cpp)
rankingobjects = $(rankingsources:../ranking/%.cpp=%.o)

storagesources = $(wildcard ../storage/*.cpp)
storageobjects = $(storagesources:../storage/%.cpp=%.o)

schedulersources = $(wildcard ../scheduler/*.cpp)
schedulerobjects = $(schedulersources:../scheduler/%.cpp=%.o)

proxysources = $(wildcard ../proxy/*.cpp)
proxyobjects = $(proxysources:../proxy/%.cpp=%.o)

commsources = $(wildcard ../comm/*.cpp) $(wildcard ../comm/*.cc)
commobjects = $(commsources:../comm/%.cpp=%.o) $(commsources:../comm/%.cc=%.o)

disservicecommsources = $(wildcard ../comm/disservice/*.cpp) $(wildcard ../comm/subjects/disservice/*.cc)
disservicecommobjects = $(disservicecommsources:../comm/disservice/%.cpp=%.o) $(disservicecommsources:../comm/subjects/disservice/%.cc=%.o)

mocketscommsources = $(wildcard ../comm/mockets/*.cpp)
mocketscommobjects = $(mocketscommsources:../comm/mockets/%.cpp=%.o)

netloggercommsources = $(wildcard ../comm/netlogger/*.cpp)
netloggercommobjects = $(netloggercommsources:../comm/netlogger/%.cpp=%.o)

tcpcommsources = $(wildcard ../comm/tcp/*.cpp)
tcpcommobjects = $(tcpcommsources:../comm/tcp/%.cpp=%.o)

udpcommsources = $(wildcard ../comm/udp/*.cpp)
udpcommobjects = $(udpcommsources:../comm/udp/%.cpp=%.o)

querycontrollerssources = $(wildcard ../controllers/query/*.cpp)
querycontrollersobjects = $(querycontrollerssources:../controllers/query/%.cpp=%.o)

forwardingcontrollerssources = $(wildcard ../controllers/forwarding/*.cpp)
forwardingcontrollersobjects = $(forwardingcontrollerssources:../controllers/forwarding/%.cpp=%.o)

wrapperssources = $(wildcard ../jni/*.cpp)
wrappersobjects = $(wrapperssources:../jni/%.cpp=%.o)

amtsources = ../apps/AMTDictator.cpp
amtobjects = $(amtsources:../apps/%.cpp=%.o)

statssources = ../apps/Stats.cpp
statsobjects = $(statssources:../apps/%.cpp=%.o)

missionpkgssources = ../apps/MissionPkg.cpp
missionpkgsobjects = $(missionpkgssources:../apps/%.cpp=%.o)

resetsources = ../apps/Reset.cpp
resetobjects = $(resetsources:../apps/%.cpp=%.o)

allobjs = $(rankingobjects) $(awarenesspartsobjects) $(awarenessobjects) $(schedulerobjects) $(storageobjects) $(disservicecommobjects) $(mocketscommobjects) $(netloggercommobjects) $(dsproobjects) $(tcpcommobjects) $(udpcommobjects) $(commobjects) $(proxyobjects) $(querycontrollersobjects) $(forwardingcontrollersobjects) $(jniobjects) $(amtobjects) $(statsobjects) $(missionpkgsobjects) $(resetobjects)

%.o : ../apps/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../ranking/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../awareness/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../awareness/parts/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../storage/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../scheduler/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../proxy/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/%.cc
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/disservice/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/subjects/disservice/%.cc
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/mockets/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/netlogger/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/tcp/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/udp/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../controllers/query/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../controllers/forwarding/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../jni/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

all: $(EXECUTABLE)

jni: libdsprojniwrapper.so

.PHONY : touchDSProMain
	touch ../DSProMain.cpp

libtinyxpath.a :
	make -C $(TINYXPATH_HOME)/$(MAKEFILE_FOLDER)/ libtinyxpath.a

libnms.a :
	make -C $(NMS_HOME)/cpp/$(MAKEFILE_FOLDER)/ libnms.a

libchunking.a :
	make -C $(CHUNKING_HOME)/$(MAKEFILE_FOLDER)/ libchunking.a

libmsgpack.a:
	make -C $(MSGPACK_HOME)/$(MAKEFILE_FOLDER)/ libmsgpack.a

libsqlite3.a :
	make -C $(SQLITE_HOME)/linux/ libsqlite3.a

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux/ libutil.a

libdisservice.a :
	make -C $(DISSERVICE_HOME)/linux/ libdisservice.a

libmockets.a:
	make -C $(MOCKETS_HOME)/cpp/linux/ libmockets.a

libc4.5.a :
	make -C $(C45_HOME)/linux/ libc4.5.a

liblcppdc.a :
	make -C	$(LCPPDC_HOME)/$(MAKEFILE_FOLDER)/ liblcppdc.a

libvoi.a:
	make -C	$(VOI_HOME)/$(MAKEFILE_FOLDER)/ libvoi.a

libz.a :
	make -C $(ZLIB_HOME)/$(MAKEFILE_FOLDER)/ libz.a

libmil2525.a:
	make -C	$(MIL_STD_2525)/$(MAKEFILE_FOLDER)/ libmil2525.a

libnatswr.a:
	make -C $(NATSWR_HOME)/$(MAKEFILE_FOLDER)/ natswr 

libnats.a:
	make -C $(CNATS_HOME)/

builddeps: libmil2525.a libvoi.a liblcppdc.a libc4.5.a libmockets.a libdisservice.a libutil.a libsqlite3.a libmsgpack.a libchunking.a libnms.a libtinyxpath.a libz.a libnatswr.a libnats.a 

libdspro.a:  $(allobjs) touchDSProMain builddeps
	ar -rcv libdspro.a $(allobjs)

$(EXECUTABLE): touchDSProMain libdspro.a libutil.a ../DSProMain.cpp
	$(CPP) $(CPPFLAGS) ../DSProMain.cpp \
	libdspro.a \
	$(LIBS) \
	$(LD_FLAGS) \
	-o $(EXECUTABLE)

$(DSPROSHELL): touchDSProMain libdspro.a libutil.a ../apps/DSProShell.cpp
	$(CPP) $(CPPFLAGS) ../apps/DSProShell.cpp \
	libdspro.a \
	$(LIBS) \
	$(LD_FLAGS) \
	-o $(DSPROSHELL)

libdsprojniwrapper.so: $(wrappersobjects) libdspro.a libutil.a 
	$(CPP) $(CPPFLAGS) -shared -o ../../../bin/libdsprojniwrapper.so \
	libdspro.a \
	$(LIBS) \
	$(LD_FLAGS) 

#  Make all

clean :
	rm -rf *.o *.a *.gch ../*.gch *.dSYM $(EXECUTABLE) $(DSPROSHELL) libdspro.a ../../../bin/libdsprojniwrapper.so

cleanall: clean
	make -C $(SQLITE_HOME)/linux/ clean
	make -C $(NOMADS_HOME)/util/cpp/linux/ clean 
	make -C $(MOCKETS_HOME)/cpp/linux/ clean
	make -C $(C45_HOME)/linux/ clean
	make -C	$(LCPPDC_HOME)/$(MAKEFILE_FOLDER)/ clean
	make -C	$(VOI_HOME)/$(MAKEFILE_FOLDER)/ clean
	make -C	$(MIL_STD_2525)/$(MAKEFILE_FOLDER)/ clean
	make -C $(DISSERVICE_HOME)/linux/ cleanall
	make -C $(C45_HOME)/linux/ clean
	make -C $(NATSWR_HOME)/$(MAKEFILE_FOLDER)/ clean
