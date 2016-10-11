all: liblzma.a

liblzma.a:
	make -f ACIMakefile -C ../make; cp ../src/liblzma/.libs/liblzma.a ./;

clean:
	make -f ACIMakefile -C ../make clean
	if test -e liblzma.a; \
		then rm liblzma.a; \
	fi
