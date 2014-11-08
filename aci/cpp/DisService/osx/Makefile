all: onNMS

onNMS:
	make -C ../make/
	cp ../make/*.a ./
	cp ../make/DisServiceLauncher ./

onXLayer:
	make -f ../make/Makefile-xlayer -C ../make/
	cp ../make/*.a ./

chunk:
	make -f ../make/Makefile-chunk -C ../make/
	cp ../make/*.a ./

clean:
	if test -e DisServiceLauncher; \
		then rm DisServiceLauncher; \
	fi
	if test -e libdisservice.a; \
		then rm libdisservice.a; \
	fi
	make -C ../make clean

cleanall: clean
	make -C ../make/ cleanall

cleanall-xlayer: clean
	make -f ../make/Makefile-xlayer -C ../make/ cleanall

cleanall-noChunk: clean
	make -f ../make/Makefile-chunk -C ../make/ cleanall
