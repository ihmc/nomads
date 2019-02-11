

libjasper.a:
	make -f ACIMakefile -C ../ && cp ../src/libjasper/.libs/libjasper.a .

clean:
	make -f ACIMakefile -C ../ clean
	if test -e libjasper.a; then \
		rm libjasper.a; \
	fi
	
