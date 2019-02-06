# Makefile for netsensor
#-Wl, -R/path/to/libdirl

CXX = g++
C11FLAG = -std=c++11

PKG_CONFIG = pkg-config

ifeq ($(NO_PKG_CONFIG),)
NL3xFOUND := $(shell $(PKG_CONFIG) --atleast-version=3.2 libnl-3.0 && echo Y)
ifneq ($(NL3xFOUND),Y)
NL31FOUND := $(shell $(PKG_CONFIG) --exact-version=3.1 libnl-3.1 && echo Y)
ifneq ($(NL31FOUND),Y)
NL3FOUND := $(shell $(PKG_CONFIG) --atleast-version=3 libnl-3.0 && echo Y)
ifneq ($(NL3FOUND),Y)
NL2FOUND := $(shell $(PKG_CONFIG) --atleast-version=2 libnl-2.0 && echo Y)
ifneq ($(NL2FOUND),Y)
NL1FOUND := $(shell $(PKG_CONFIG) --atleast-version=1 libnl-1 && echo Y)
endif
endif
endif
endif
endif

ifeq ($(NL1FOUND),Y)
NLLIBNAME = libnl-1
endif

ifeq ($(NL2FOUND),Y)
CXXFLAGS += -DCONFIG_LIBNL20
LIBS += -lnl-genl
NLLIBNAME = libnl-2.0
endif

ifeq ($(NL3xFOUND),Y)
# libnl 3.2 might be found as 3.2 and 3.0
NL3FOUND = N
CXXFLAGS += -DCONFIG_LIBNL30
LIBS += -lnl-genl-3
NLLIBNAME = libnl-3.0
endif

ifeq ($(NL3FOUND),Y)
CXXFLAGS += -DCONFIG_LIBNL30
LIBS += -lnl-genl
NLLIBNAME = libnl-3.0
endif

# nl-3.1 has a broken libnl-gnl-3.1.pc file
# as show by pkg-config --debug --libs --cflags --exact-version=3.1 libnl-genl-3.1;echo $?
ifeq ($(NL31FOUND),Y)
CXXFLAGS += -DCONFIG_LIBNL30
LIBS += -lnl-genl
NLLIBNAME = libnl-3.1
endif

ifeq ($(NLLIBNAME),)
$(error Cannot find development files for any supported version of libnl)
endif



NOMADS_HOME = .
UTIL_HOME = $(NOMADS_HOME)/util

ARCH = $(shell sh $(UTIL_HOME)/scripts/guessArch.sh)
MACHINE = $(shell uname -m)
CXXFLAGS += -g
CXXFLAGS += -DUNIX -DCONFIG_LIBNL30 -DLINUX -Dnullptr=0 -D_GLIBCXX_USE_C99 -DERROR_CHECKING -DPTHREAD_MUTEX_RECURSIVE_NP=PTHREAD_MUTEX_RECURSIVE -DLITTLE_ENDIAN_SYSTEM -DIW -fpermissive \
            -I$(NOMADS_HOME)/externals/include/pcap \
            -I$(NOMADS_HOME)/externals/include \
            -I$(NOMADS_HOME)/util/cpp/net \
            -I$(NOMADS_HOME)/util/cpp \
            -I/usr/include/libnl3
            

LIB_LIST = $(NOMADS_HOME)/util/cpp/linux/libutil.a 
            #$(LIBZ_HOME)/linux/libz.a

LD_FLAGS = -lprotobuf -lstdc++ -lpcap -lpthread $(LIBS) -lz $(shell $(PKG_CONFIG) --libs $(NLLIBNAME)) 

sources = $(wildcard ./*.cpp) $(wildcard ./*.cc)
objects = $(sources:./%.cpp=%.o) $(sources:./%.cc=%.o)



MAKEFILE_FOLDER = linux


%.o : ./%.cpp
	$(CXX) $(C11FLAG) -c $(CXXFLAGS) $<

%.o : ./%.cc
	$(CXX) $(C11FLAG) -c $(CXXFLAGS) $<


all: libnetsensor.a Launcher

#libz.a :
#	make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER)/ libz.a

libutil.a :
	make -C $(UTIL_HOME)/cpp/linux libutil.a

libnetsensor.a : $(objects) libutil.a
	ar -rcv libnetsensor.a $(objects)

Launcher: libnetsensor.a libutil.a Launcher.cpp
	$(CXX) $(C11FLAG) $(CXXFLAGS) \
	Launcher.cpp \
	libnetsensor.a \
	$(LIB_LIST) $(LD_FLAGS) \
	-o netsensor


clean :
	rm -rf *.o ./*.a netsensor 
	
cleanall :
	make clean
	(make -C $(UTIL_HOME)/cpp/$(MAKEFILE_FOLDER) clean)
	(make -C $(LIBZ_HOME)/$(MAKEFILE_FOLDER) clean)     
