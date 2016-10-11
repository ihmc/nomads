#*************************************************************************#
#*									 *#
#*		Makefile for the C4.5 induction system			 *#
#*		author: Silvia Rota (srota@ihmc.us)
#*									 *#
#*************************************************************************#

ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
MakefileArch=../../../../util/scripts/Makefile.$(ARCH)
include $(MakefileArch)

PROJECT_ROOT = ../../../../..
NOMADS_HOME = ../../../..
CPPSOURCES = $(wildcard ../*.cpp)
CPPOBJECTS = $(CPPSOURCES:../%.cpp=%.o)
CSOURCES = $(wildcard ../*.c)
COBJECTS = $(CSOURCES:../%.c=%.o)

CFLAGS = -g -Wall -DUNIX $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM
CPPFLAGS = -g -Wall -DUNIX $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM -I.. \
            -I$(NOMADS_HOME)/util/cpp
LD_FLAGS = -lpthread -lnsl -ldl

%.o : ../%.c
	cc $(CFLAGS) -c $<

%.o : ../%.cpp
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
	make -C $(NOMADS_HOME)/util/cpp/osx/ libutil.a

libc4.5.a:	$(COBJECTS) $(CPPOBJECTS) libutil.a
	ar -rcv libc4.5.a $(COBJECTS) $(CPPOBJECTS) \
	$(NOMADS_HOME)/util/cpp/osx/libutil.a

#libc4.5.a:	$(cinterface) $(trees) $(rules) $(core) $(CPPOBJECTS) libutil.a
#	ar -rcv libc4.5.a $(cinterface) $(trees) $(rules) $(core) $(CPPOBJECTS) \
#	$(NOMADS_HOME)/util/cpp/osx/libutil.a

#  Make all

all:
	make libc4.5.a
#	cc -o xval-prep ../xval-prep.c		# other algorithms
#	cc -o average ../average.c -lm

$(trees): ../defns.h ../types.h ../extern.h
$(rules): ../defns.h ../types.h ../extern.h
$(cppinterface): ../TestInfo.h ../C45TreeInfo.h

# Make clean

clean :
	rm -rf *.o *.a *.gch ../*.gch
