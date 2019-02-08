#************************************************************************#
#									 #
#		Makefile for the C4.5 induction system			 #
#		author: Silvia Rota (srota@ihmc.us)                      #
#									 #
#************************************************************************#

PROJECT_ROOT = ../../../../..
NOMADS_HOME = ../../../..
CPPSOURCES = $(wildcard ../*.cpp)
CPPOBJECTS = $(CPPSOURCES:../%.cpp=%.o)
CSOURCES = $(wildcard ../*.c)
COBJECTS = $(CSOURCES:../%.c=%.o)

#Environment
ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
include ../../../../util/scripts/Makefile.$(ARCH)

CFLAGS = -g -Wall -DUNIX $(BUILD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM
CPPFLAGS = -g -Wall -DUNIX $(ARCH_FLAGS) $(BUILD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -I.. \
            -I$(NOMADS_HOME)/util/cpp
LD_FLAGS = -lpthread -lnsl -ldl

#  Make all
all:
	make libc4.5.a
#	cc -o xval-prep ../xval-prep.c		# other algorithms
#	cc -o average ../average.c -lm

%.o : ../%.c
	cc $(CFLAGS) -c $<

%.o : ../%.cpp
	@echo "Building for $(ARCH)"
	g++ $(CPPFLAGS) -c $<

#	Definitions of file sets

core = \
	../getnames.o ../getdata.o ../trees.o

trees = \
	../besttree.o ../build.o ../info.o ../discr.o ../contin.o ../subset.o ../prune.o \
	../stats.o ../st-thresh.o ../classify.o ../confmat.o ../sort.o

rules = \
	../rules.o ../genlogs.o ../genrules.o ../makerules.o ../prunerule.o \
	../siftrules.o ../testrules.o ../stats.o ../confmat.o ../sort.o

cinterface = \
	../c4.5.o ../c4.5rules.o ../consult.o ../consultr.o ../userint.o

libutil.a :
	make -C $(NOMADS_HOME)/util/cpp/linux/ libutil.a

libc4.5.a:	$(COBJECTS) $(CPPOBJECTS) libutil.a
	@echo "Building for $(ARCH)"
	ar -rcv libc4.5.a $(COBJECTS) $(CPPOBJECTS) \
	$(NOMADS_HOME)/util/cpp/linux/libutil.a

#libc4.5.a:	$(cinterface) $(trees) $(rules) $(core) $(CPPOBJECTS) libutil.a
#	ar -rcv libc4.5.a $(cinterface) $(trees) $(rules) $(core) $(CPPOBJECTS) \
#	$(NOMADS_HOME)/util/cpp/linux/libutil.a

$(trees): ../defns.h ../types.h ../extern.h
$(rules): ../defns.h ../types.h ../extern.h
$(cppinterface): ../TestInfo.h ../C45TreeInfo.h

# Make clean
clean :
	rm -rf *.o *.a *.gch ../*.gch
