#
# Makefile.in
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
MakefileArch=../../../../util/scripts/Makefile.$(ARCH)
include $(MakefileArch)

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util
EXTERNALS=$(NOMADS_HOME)/externals/
SQLITE_HOME = $(EXTERNALS)/SQLite

cpp = gcc
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS=

MAKEFILE_FOLDER=osx

options=-Wall -Wextra

INCLUDES=-I..-I../test \
         -I$(NOMADS_HOME)/util/cpp \
	 -I$(SQLITE_HOME)/

cppflags= -g $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)
LIBS=$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
     $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
     $(JPEG_HOME)/linux/libjpeg.a \
     $(EXTERNALS_LIB)/libcrypto.a \
     $(EXTERNALS_LIB)/libz.a \
     $(SQLITE_HOME)/$(LIB_FOLDER)/libsqlite3.a \
     -L/usr/local/lib/
EXECUTABLE=go_test
