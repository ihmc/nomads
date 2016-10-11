#
# Makefile
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

include Makefile.inc

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

all: libnockets.a nocketrcvr

libutil.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libnorm.a:
	make -f NOMADSMakefile -C $(NORM_HOME) libnorm.a

libnockets.a: libnorm.a libutil.a $(objects)
	@echo "Compiling with $(MakefileArch) for $(ARCH) architecture"
	@echo "MAKEFILE_FOLDER: $(MAKEFILE_FOLDER) ARCH_FLAGS: $(ARCH_FLAGS) EXTERNALS_LIB: $(EXTERNALS_LIB)"
	ar -rcv libnockets.a $(objects)

nocketrcvr: $(objects) libnockets.a
	$(cpp) $(cppflags) \
	../main.cpp \
	libnockets.a \
	$(NORM_HOME)/lib/libnorm.a \
	$(NORM_HOME)/build/protolib/libprotolib.a \
	$(NORM_HOME)/protolib/lib/libprotokit.a \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
        $(LD_FLAGS) \
	-o nockettest

clean :
	rm -rf *.o *.a nockettest 
