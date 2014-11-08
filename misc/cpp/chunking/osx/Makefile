#
# Makefile
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

include Makefile.inc

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

all: libchunking.a 

libutil.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libsecurity.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

libjpeg.a:
	make -C $(JPEG_HOME)/$(MAKEFILE_FOLDER)/ libjpeg.a

libjasper.a:
	make -C $(JASPER_HOME)/../../$(MAKEFILE_FOLDER)/ libjasper.a

libchunking.a: libjpeg.a libjpeg.a libjasper.a libutil.a libsecurity.a $(objects)
	@echo "Compiling with $(MakefileArch) for $(ARCH) architecture"
	@echo "MAKEFILE_FOLDER: $(MAKEFILE_FOLDER) ARCH_FLAGS: $(ARCH_FLAGS) EXTERNALS_LIB: $(EXTERNALS_LIB)"
	ar -rcv libchunking.a $(objects)

testAndDump: libchunking.a libjasper.a ../go_test.cpp
	$(cpp) $(LIBS) $(cppflags) -DDUMP_CHUNK_ON_FILE \
	../go_test.cpp \
	libchunking.a \
	$(LIBS) \
	-o $(EXECUTABLE)AndDup

test: libchunking.a ../go_test.cpp
	$(cpp) $(LIBS) $(cppflags) \
	../go_test.cpp \
	libchunking.a \
	$(LIBS) \
	-o $(EXECUTABLE)

clean:
	rm -rf *.o *.a $(EXECUTABLE) $(EXECUTABLE)AndDup

cleanall: clean
	make -C $(JPEG_HOME)/$(MAKEFILE_FOLDER)/ clean
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ clean
	
