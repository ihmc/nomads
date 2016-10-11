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
DALI_HOME=$(NOMADS_HOME)/externals/dali-1.0

cpp = g++
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -pthread
LIB_FOLDER=linux
MAKEFILE_FOLDER=linux
options=-Wall -Wextra -Wno-unused-parameter

INCLUDES=-I..\
         -I$(NOMADS_HOME)/util/cpp \
         -I../../media \
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

HAS_CRYPTO_LIB = $(shell g++ -lcrypto 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
EXTRA_LIB += $(shell if [ ${HAS_CRYPTO_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libcrypto.a"; else  echo "-lcrypto"; fi)
HAS_Z_LIB = $(shell g++ -lz 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
EXTRA_LIB += $(shell if [ ${HAS_Z_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libz.a"; else  echo "-lz"; fi)

cppflags= -g -fPIC $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
sources = $(wildcard ../*.cpp)

NOWANTS=../FFmpegHandler.cpp ../TestDriver.cpp
sources = $(filter-out $(NOWANTS),$(wildcard ../*.cpp))
objects = $(sources:../%.cpp=%.o)

LIBS=libchunking.a \
     $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
     $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
     $(JASPER_HOME)/../../linux/libjasper.a \
     $(JPEG_HOME)/linux/libjpeg.a \
     $(PNG_HOME)/linux/libpng.a \
     ../../media/$(LIB_FOLDER)/libmedia.a \
     $(DALI_HOME)/$(LIB_FOLDER)/libdvmmpeg.a \
     $(DALI_HOME)/$(LIB_FOLDER)/libdvmbasic.a \
     $(EXTRA_LIB)

EXECUTABLE=
