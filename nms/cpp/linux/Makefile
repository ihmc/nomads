#
# Makefile
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

include Makefile.inc

#allobjs =  $(objects)
allobjs = $(ifacesobjects) $(proxycobjects) $(proxysobjects) $(proxyobjects) $(objects)

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../ifaces/%.cpp 
	$(cpp) -c $(cppflags) $<

%.o : ../proxy/%.cpp 
	$(cpp) -c $(cppflags) $<

%.o : ../proxy/server/%.cpp
	$(cpp) -c $(cppflags) $<

%.o : ../proxy/client/%.cpp
	$(cpp) -c $(cppflags) $<

all: NMSLauncher NMSProxyShell

libnockets.a: libutil.a libsecurity.a
	make -C $(NOCKET_HOME)/$(MAKEFILE_FOLDER)/ libnockets.a

libutil.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libsecurity.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

libnms.a: $(allobjs) libnockets.a
	@echo "Proxy objects: $(ifacessources)"
	@echo "Proxy objects: $(proxysources)"
	@echo "Proxy s objects: $(proxyssources)"
	@echo "Proxy c objects: $(proxycsources)"
	@echo "All objects: $(allobjs)"
	@echo "Proxy objects: $(proxyobjs)"
	@echo "Proxy s objects: $(proxysobjs)"
	@echo "Proxy c objects: $(proxycobjs)"
	@echo "Compiling with $(MakefileArch) for $(ARCH) architecture"
	@echo "MAKEFILE_FOLDER: $(MAKEFILE_FOLDER) ARCH_FLAGS: $(ARCH_FLAGS) EXTERNALS_LIB: $(EXTERNALS_LIB)"
	ar -rcv libnms.a $(allobjs)

NMSLauncher: libnms.a ../$(EXECUTABLE).cpp
	$(cpp) $(cppflags) ../$(EXECUTABLE).cpp \
	libnms.a \
	$(NOCKET_HOME)/linux/libnockets.a \
	$(NOMADS_HOME)/util/cpp/linux/libsecurity.a \
        $(NOMADS_HOME)/util/cpp/linux/libutil.a \
	$(NORM_HOME)/lib/libnorm.a \
        $(NORM_HOME)/build/protolib/libprotolib.a \
        $(NORM_HOME)/protolib/lib/libprotokit.a \
        $(LD_FLAGS) \
	-o $(EXECUTABLE) 

NMSProxyShell: libnms.a ../$(EXECUTABLE).cpp
	$(cpp) $(cppflags) ../proxy/client/NMSProxyShell.cpp \
	libnms.a \
	$(NOCKET_HOME)/linux/libnockets.a \
	$(NOMADS_HOME)/util/cpp/linux/libsecurity.a \
        $(NOMADS_HOME)/util/cpp/linux/libutil.a \
	$(NORM_HOME)/lib/libnorm.a \
        $(NORM_HOME)/build/protolib/libprotolib.a \
        $(NORM_HOME)/protolib/lib/libprotokit.a \
	$(LD_FLAGS) \
	-o NMSProxyShell 

clean:
	rm -rf *.o *.a $(EXECUTABLE) NMSProxyShell

cleanall: clean
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ clean
