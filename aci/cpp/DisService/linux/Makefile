all: onNMS

libdisservice.a: onNMS

onNMS:
	make -C ../make/
	cp ../make/*.a ./
	cp ../make/DisServiceLauncher ./
	cp ../make/libDisServiceJNIWrapper.so ./

onNMSValgrind:
	make -C ../make/ -f Makefile-valgrind
	cp ../make/*.a ./
	cp ../make/DisServiceLauncher ./

onXLayer:
	make -f ../make/Makefile-xlayer -C ../make/
	cp ../make/*.a ./
	cp ../make/DisServiceLauncher ./

clean:
	if test -e DisServiceLauncher; \
		then rm DisServiceLauncher; \
	fi
	if test -e libdisservice.a; \
		then rm libdisservice.a; \
	fi
	if test -e libdisserviceproxy.a; \
		then rm libdisserviceproxy.a; \
	fi
	if test -e libDisServiceJNIWrapper.so; \
		then rm libDisServiceJNIWrapper.so; \
	fi 
	make -C ../make clean

cleanall: clean
	make -C ../make/ cleanall

cleanall-xlayer: clean
	make -f ../make/Makefile-xlayer -C ../make/ cleanall
