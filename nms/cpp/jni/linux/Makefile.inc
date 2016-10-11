#
# Makefile.inc
# author: Rita Lenzi	(rlenzi@ihmc.us)
#

ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
MakefileArch=../../../../util/scripts/Makefile.$(ARCH)
include $(MakefileArch)

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util

cpp = g++
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -lpthread -ldl

options=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable
#options= 

INCLUDES=-I.. -I../.. \
        -I$(NOMADS_HOME)/externals/JDK \
        -I$(NOMADS_HOME)/externals/JDK/$(LIB_FOLDER) \
	-isystem $(NOMADS_HOME)/util/cpp

cppflags= -g -fPIC $(ARCH_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)

sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

wrappers = $(wildcard ../*.cpp)
wrappers_objs = $(wrappers:../%.cpp=%.o)

LIBS=$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
     -L/usr/local/lib/
