#
# Makefile.in
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

ARCH = $(shell sh ../../scripts/guessArch.sh)
MakefileArch=../../scripts/Makefile.$(ARCH)
include $(MakefileArch)

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util

cpp = g++
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -lpthread

#options=-Wall -Wextra -Werror 
options=

INCLUDES=-I.. -I../ -I../../../externals/include

cppflags= -g -fPIC $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)
netsources = $(wildcard ../net/*.cpp)
netobjects = $(netsources:../net/%.cpp=%.o)
securitysources = $(wildcard ../security/*.cpp)
securityobjects = $(securitysources:../security/%.cpp=%.o)
graphsources = $(wildcard ../graph/*.cpp)
graphobjects = $(graphsources:../graph/%.cpp=%.o)
commsources = $(wildcard ../comm/*.cpp)
commobjects = $(commsources:../comm/%.cpp=%.o)
proxysources = $(wildcard ../proxy/*.cpp)
proxyobjects = $(proxysources:../proxy/%.cpp=%.o)
LIBS=$(EXTERNALS_LIB)
