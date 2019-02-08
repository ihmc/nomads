all: onNMS DisServiceMessageInjector libDisServiceJNIWrapper.so DisServiceLauncher

libdisservice.a: onNMS

copyExecutables:
	cp ../make/*.a ./
	cp ../make/DisServiceLauncher ./
	cp ../make/libDisServiceJNIWrapper.so ./
	cp ../make/DisServiceMessageInjector ./

DisServiceLauncher:
	make -C ../make/ DisServiceLauncher
	cp ../make/DisServiceLauncher ./

libDisServiceJNIWrapper.so:
	make -C ../make/ libDisServiceJNIWrapper.so
	cp ../make/libDisServiceJNIWrapper.so ./

DisServiceMessageInjector:
	make -C ../make/ DisServiceMessageInjector
	cp ../make/DisServiceMessageInjector ./

onNMS:
	make -C ../make/ libdisservice.a
	make -C ../make/ libdisserviceproxy.a
	cp ../make/*.a ./
	
onNMSValgrind:
	make -C ../make/ -f Makefile-valgrind
	$(MAKE) copyExecutables

onNMSGprof:
	make -C ../make/ -f Makefile-gprof
	$(MAKE) copyExecutables

clean:
	if test -e DisServiceLauncher; \
		then rm DisServiceLauncher; \
	fi
	if test -e libdisservice.a; \
		then rm libdisservice.a; \
	fi
	if test -e DisServiceMessageInjector; \
		then rm DisServiceMessageInjector; \
	fi
	if test -e libdisserviceproxy.a; \
		then rm libdisserviceproxy.a; \
	fi
	if test -e libDisServiceJNIWrapper.so; \
		then rm libDisServiceJNIWrapper.so; \
	fi 
	make -C ../make clean
	rm -f *.log *.out

cleanall: clean
	make -C ../make/ cleanall
