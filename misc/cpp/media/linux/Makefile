#
# Makefile
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

include Makefile.inc

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

all: libmedia.a 

libutil.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libdali.a:
	make -C $(DALI_HOME)/$(MAKEFILE_FOLDER)/ libdali

libsecurity.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libsecurity.a

libmedia.a: libutil.a libsecurity.a libdali.a $(objects)
	@echo "LIBS: $INCLUDES"
	ar -rcv libmedia.a $(objects)

FFMPEGReaderTest: libmedia.a ../FFMPEGReaderTest.cpp
	$(cpp) $(cppflags) \
	../FFMPEGReaderTest.cpp \
	$(LIBS) $(LD_FLAGS) \
	-o FFMPEGReaderTest

MPEG1ParserTest: libmedia.a ../../../test/cpp/media/MPEG1Test.cpp
	$(cpp) $(cppflags) \ 
	../../../test/cpp/media/MPEG1Test.cpp \
	$(LIBS) $(LD_FLAGS) \
	-o MPEG1ParserTest

clean:
	rm -rf *.o *.a FFMPEGReaderTest MPEG1ParserTest

cleanall: clean
	make -C $(JPEG_HOME)/$(MAKEFILE_FOLDER)/ clean
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ clean
	
