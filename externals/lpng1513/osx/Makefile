#Environment

all: checkMakefile

checkMakefile:
	if test -e ../Makefile; \
                then rm ../Makefile; \
        fi; \
        cp ../scripts/makefile.linux ../Makefile
	make -C ../ libpng.a
	if test -e ../libpng.a; \
		then cp ../libpng.a .; \
	fi

clean:
	if test -e libpng.a; \
		then rm libpng.a; \
	fi
	if test -e ../Makefile; \
		then make -C ../ clean; rm ../Makefile; \
	fi
