#
# Makefile
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

include Makefile.inc

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

all: liblcppdc.a

libutil.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libsecurity.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

libsqlite.a :
	make -C $(SQLITE_HOME)/$(MAKEFILE_FOLDER)/ libsqlite3.a

liblcppdc.a: libutil.a libsecurity.a libsqlite.a $(objects)
	@echo "Compiling with $(MakefileArch) for $(ARCH) architecture"
	@echo "MAKEFILE_FOLDER: $(MAKEFILE_FOLDER) ARCH_FLAGS: $(ARCH_FLAGS) EXTERNALS_LIB: $(EXTERNALS_LIB)"
	ar -rcv liblcppdc.a $(objects)

clean :
	rm -rf *.o *.a $(EXECUTABLE)
