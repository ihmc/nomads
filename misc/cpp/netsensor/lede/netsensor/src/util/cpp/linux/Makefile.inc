#
# Makefile.in OpenWrt (MIPS)
# author: Alessandro Morelli	(amorelli@ihmc.us)
#

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util

#options=-Wall -Wextra -Werror 
#options=

INCLUDES=-I.. -I../ -I../../../externals/include

CXXFLAGS+= -fPIC $(options) $(INCLUDES)

sources = $(wildcard ../*.cpp)
sources += $(wildcard ../*.c)
objects = $(sources:../%.cpp=%.o)
objects += $(sources:../%.c=%.o)
netsources = $(wildcard ../net/*.cpp)
netobjects = $(netsources:../net/%.cpp=%.o)
