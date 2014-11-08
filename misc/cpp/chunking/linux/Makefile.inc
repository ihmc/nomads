#
# Makefile.in
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util
EXTERNALS_LIB=$(NOMADS_HOME)/externals/lib/linux-x86
JPEG_HOME=$(NOMADS_HOME)/externals/libjpeg
PNG_HOME=$(NOMADS_HOME)/externals/lpng1513
JASPER_HOME=$(NOMADS_HOME)/externals/jasper/src/libjasper
cpp = g++
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -pthread
LIB_FOLDER=linux
MAKEFILE_FOLDER=linux
options=-Wall -Wextra

INCLUDES=-I..\
         -I$(NOMADS_HOME)/util/cpp \
         -I$(JPEG_HOME)/ \
         -I$(PNG_HOME)/ \
         -I$(JASPER_HOME)/include \
         -I$(JASPER_HOME)/bmp/ \
         -I$(JASPER_HOME)/jp2/ \
         -I$(JASPER_HOME)/jpc/ \
         -I$(JASPER_HOME)/jpg/ \
         -I$(JASPER_HOME)/mif/ \
         -I$(JASPER_HOME)/pgx/ \
         -I$(JASPER_HOME)/pnm/ \
         -I$(JASPER_HOME)/ras/
 
cppflags= -g -fPIC $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

LIBS=$(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
     $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
     $(JASPER_HOME)/../../linux/libjasper.a \
     $(JPEG_HOME)/linux/libjpeg.a \
     $(PNG_HOME)/linux/libpng.a \
     $(EXTERNALS_LIB)/libcrypto.a \
     $(EXTERNALS_LIB)/libz.a

EXECUTABLE=
