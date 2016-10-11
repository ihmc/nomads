all: libz.a

libz.a:
	make -f ACIMakefile -C ../make; cp ../libz.a ./;

clean:
	make -f ACIMakefile -C ../make clean
	if test -e libz.a; \
		then rm libz.a; \
	fi
