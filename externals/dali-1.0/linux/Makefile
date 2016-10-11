
all: libdali

libdali:
	if [ ! -e libdvmbasic.a ]; then \
                make -C ../unix -f IHMC-Makefile libdali; \
		cp ../unix/lib/libdvmbasic.a ../unix/lib/libdvmmpeg.a ./; \
        fi

clean:
	if [ -e libtinyxpath.a ]; then \
                rm -rf libtinyxpath.a; \
        fi
	make -C ../unix -f IHMC_Makefile clean


