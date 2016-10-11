all: libmsgpack.a

libmsgpack.a:
	make -f ACIMakefile -C ../make; cp ../src/.libs/libmsgpack.a ./;

clean:
	make -f ACIMakefile -C ../make clean
	if test -e libmsgpack.a; \
		then rm libmsgpack.a; \
	fi
