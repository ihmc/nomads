#
# Makefile.in
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

NOMADS_HOME = ../../../../..
UTIL_HOME = $(NOMADS_HOME)/util
SQLITE_HOME = $(NOMADS_HOME)/externals/SQLite

cpp = g++
archflags=-DUNIX -DLINUX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -lpthread

LIB_FOLDER=linux
MAKEFILE_FOLDER=linux

options=-Wall -Wextra

INCLUDES=-I.. \
         -I../.. \
         -I$(NOMADS_HOME)/util/cpp \
	     -I$(SQLITE_HOME)/

cppflags= -g $(archflags) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)
LIBS=$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
     $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
     $(JPEG_HOME)/linux/libjpeg.a \
     $(EXTERNALS_LIB)/libcrypto.a \
     $(EXTERNALS_LIB)/libz.a \
     $(SQLITE_HOME)/linux/libsqlite3.a
