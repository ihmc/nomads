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

libpng.a:
	make -C $(PNG_HOME)/$(MAKEFILE_FOLDER)/ libpng.a

libjasper.a:
	make -C $(JASPER_HOME)/../../$(MAKEFILE_FOLDER)/ libjasper.a

libchunking.a: libjpeg.a libpng.a libjasper.a libutil.a libsecurity.a $(objects)
	ar -rcv libchunking.a $(objects)

clean:
	rm -rf *.o *.a $(EXECUTABLE) 

cleanall: clean
	make -C $(JPEG_HOME)/$(MAKEFILE_FOLDER)/ clean
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ clean
	
