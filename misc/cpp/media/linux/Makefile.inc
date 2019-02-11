#
# Makefile.in
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util
EXTERNALS_HOME=$(NOMADS_HOME)/externals
EXTERNALS_LIB=$(EXTERNALS_HOME)/lib/linux-x86
JPEG_HOME=$(NOMADS_HOME)/externals/libjpeg
PNG_HOME=$(NOMADS_HOME)/externals/lpng1513
JASPER_HOME=$(NOMADS_HOME)/externals/jasper/src/libjasper
DALI_HOME=$(EXTERNALS_HOME)/dali-1.0

cpp = g++
ARCH_FLAGS+=-DUNIX -DLITTLE_ENDIAN_SYSTEM
LD_FLAGS = -pthread
LIB_FOLDER=linux
MAKEFILE_FOLDER=linux
options=-Wall -Wextra -Wno-unused-parameter

INCLUDES=-I..\
    -I$(UTIL_HOME)/cpp \
    -I$(DALI_HOME)/include \
    -I$(EXTERNALS_HOME)/include/ffmpeg 

HAS_CRYPTO_LIB = $(shell g++ -lcrypto 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
EXTRA_LIB += $(shell if [ ${HAS_CRYPTO_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libcrypto.a"; else  echo "-lcrypto"; fi)
HAS_Z_LIB = $(shell g++ -lz 2>&1 | grep "undefined reference to" 1>/dev/null; echo $$?)
EXTRA_LIB += $(shell if [ ${HAS_Z_LIB} -gt 0 ]; then echo "$(EXTERNALS_LIB)/libz.a"; else  echo "-lz"; fi)

cppflags= -g -std=c++11 -fPIC $(ARCH_FLAGS) $(LD_FLAGS) -DENABLE_DEBUG -DERROR_CHECKING $(options) $(INCLUDES)
NOWANTS=../FFMPEGReader.cpp ../FFMPEGReader.h ../FFMPEGUtil.cpp ../FFMPEGUtil.h  ../FFMPEGWriter.cpp ../FFMPEGWriter.h FFMPEGReaderTest.cpp
sources = $(filter-out $(NOWANTS),$(wildcard ../*.cpp))
#sources = $(wildcard ../*.cpp)
objects = $(sources:../%.cpp=%.o)

LIBS=libmedia.a \
     $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libutil.a \
     $(UTIL_HOME)/cpp/$(LIB_FOLDER)/libsecurity.a \
     $(DALI_HOME)/$(LIB_FOLDER)/libdvmmpeg.a \
     $(DALI_HOME)/$(LIB_FOLDER)/libdvmbasic.a \
     $(EXTRA_LIB)

