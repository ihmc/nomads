all: libtinyxpath.a

libtinyxpath.a:
	if [ ! -e libtinyxpath.a ]; then \
		make -C ../make -f ACIMakefile libtinyxpath.a; \
		cp ../libtinyxpath.a ./; \
	fi

clean:
	if [ -e libtinyxpath.a ]; then \
		rm -rf libtinyxpath.a; \
	fi
	make -C ../make -f ACIMakefile clean
