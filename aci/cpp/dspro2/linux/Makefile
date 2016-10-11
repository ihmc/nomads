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

tcpcommsources = $(wildcard ../comm/tcp/*.cpp)
tcpcommobjects = $(tcpcommsources:../comm/tcp/%.cpp=%.o)

querycontrollerssources = $(wildcard ../controllers/query/*.cpp)
querycontrollersobjects = $(querycontrollerssources:../controllers/query/%.cpp=%.o)

forwardingcontrollerssources = $(wildcard ../controllers/forwarding/*.cpp)
forwardingcontrollersobjects = $(forwardingcontrollerssources:../controllers/forwarding/%.cpp=%.o)

wrapperssources = $(wildcard ../jni/*.cpp)
wrappersobjects = $(wrapperssources:../jni/%.cpp=%.o)

allobjs = $(rankingobjects) $(awarenessobjects) $(schedulerobjects) $(storageobjects) $(disservicecommobjects) $(mocketscommobjects) $(dsproobjects) $(tcpcommobjects) $(commobjects) $(proxyobjects) $(querycontrollersobjects) $(forwardingcontrollersobjects) $(jniobjects)

%.o : ../ranking/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../awareness/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../storage/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../scheduler/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../proxy/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/disservice/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/mockets/%.cpp
	$(CPP) $(CPPFLAGS) -c $<

%.o : ../comm/tcp/%.cpp
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

libmil2525.a :
	make -C	$(MIL_STD_2525)/$(MAKEFILE_FOLDER)/ libmil2525.a

libdspro.a:  $(allobjs) touchDSProMain libutil.a libsqlite3.a libdisservice.a libmockets.a libc4.5.a libmil2525.a
	ar -rcv libdspro.a $(allobjs)

$(EXECUTABLE): touchDSProMain libdspro.a libutil.a ../DSProMain.cpp
	$(CPP) $(CPPFLAGS) ../DSProMain.cpp \
	libdspro.a \
	$(LIBS) \
	$(LD_FLAGS) \
	-o $(EXECUTABLE)

libdsprojniwrapper.so: $(wrappersobjects) libdspro.a libutil.a 
	$(CPP) $(CPPFLAGS) -shared -o ../../../bin/libdsprojniwrapper.so \
	libdspro.a \
	$(LIBS) \
	$(LD_FLAGS) 

#  Make all

clean :
	rm -rf *.o *.a *.gch ../*.gch *.dSYM $(EXECUTABLE) libdspro.a ../../../bin/libdsprojniwrapper.so

cleanall: clean
	make -C $(DISSERVICE_HOME)/linux/ cleanall
	make -C $(C45_HOME)/linux/ clean
