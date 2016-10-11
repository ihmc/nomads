all: libsqlite3.a

libsqlite3.a:
	make -f ACIMakefile -C ../; cp ../.libs/libsqlite3.a ./;

clean:
	make -f ACIMakefile -C ../ clean
	if test -e libsqlite3.a; \
		then rm libsqlite3.a; \
	fi
