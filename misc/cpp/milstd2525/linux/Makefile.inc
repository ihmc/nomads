#
# Makefile.in
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
MakefileArch=../../../../util/scripts/Makefile.$(ARCH)
include $(MakefileArch)

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util

cpp = g++
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -lpthread

options=-Wall -Wextra -Werror 

INCLUDES=-I..-I../test \
	-isystem $(NOMADS_HOME)/util/cpp

cppflags=-g -fPIC $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)
LIBS=$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
