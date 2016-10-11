#
# Makefile
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

include Makefile.inc

%.o : ../%.cpp
	$(cpp) -c $(cppflags) $<

all: libmil2525.a validator

libutil.a:
	make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER)/ libutil.a

libmil2525.a: libutil.a $(objects)
	@echo "Compiling with $(MakefileArch) for $(ARCH) architecture"
	@echo "MAKEFILE_FOLDER: $(MAKEFILE_FOLDER) ARCH_FLAGS: $(ARCH_FLAGS) EXTERNALS_LIB: $(EXTERNALS_LIB)"
	ar -rcv libmil2525.a $(objects)

validator: $(objects) libmil2525.a
	$(cpp) $(cppflags) \
	../main.cpp \
	libmil2525.a \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a \
        $(LD_FLAGS) \
	-o milSTD2525Validator

clean :
	rm -rf *.o *.a milSTD2525Validator
