#Environment
ARCH = $(shell sh ../../../util/scripts/guessArch.sh)

all: libpng.a

libpng.a:
	if test -e ../Makefile; \
                then rm ../Makefile; \
        fi; \
	cp ../scripts/makefile.${ARCH} ../Makefile
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
