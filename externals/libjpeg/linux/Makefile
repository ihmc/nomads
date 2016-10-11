all: libjpeg.a

LIB=libjpeg.a
LIBS=../.libs

libjpeg.a:
	make -f ACIMakefile -C ../; cp $(LIBS)/$(LIB) ./;

clean:
	make -f ACIMakefile -C ../ clean
	if test -e $(LIB); \
		then rm $(LIB); \
	fi
