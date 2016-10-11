# MAKEFILE FOR DISSERVICEPRO PROJECT

PROJECT_ROOT = ../../../../..
NOMADS_HOME = ../../../..
EXTERNALS = $(NOMADS_HOME)/externals
UTIL_HOME = $(NOMADS_HOME)/util
MOCKETS_HOME = $(NOMADS_HOME)/mockets
SQLITE_HOME = $(EXTERNALS)/SQLite
TINYXPATH_HOME = $(NOMADS_HOME)/externals/TinyXPath
DISSERVICE_HOME = $(NOMADS_HOME)/aci/cpp/DisService
C45_HOME = $(NOMADS_HOME)/misc/cpp/c45
GRP_MANAGER_HOME = $(NOMADS_HOME)/aci/cpp/grpMgr
LCPPDC_HOME= $(NOMADS_HOME)/misc/cpp/lcppdc
CHUNKING_HOME = $(NOMADS_HOME)/misc/cpp/chunking
JPEG_HOME=$(NOMADS_HOME)/externals/libjpeg
PNG_HOME=$(NOMADS_HOME)/externals/lpng1513
JASPER_HOME=$(NOMADS_HOME)/externals/jasper

EXECUTABLE = DSPro

#Environment
ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
include ../../../../util/scripts/Makefile.$(ARCH)

CPPFLAGS = -g -Wall -DUNIX $(ARCH_FLAGS) -DUSE_SQLITE -DENABLE_DEBUG -DERROR_CHECKING -DLITTLE_ENDIAN_SYSTEM \
                -I../ \
		-I../ranking/ \
		-I../awareness/ \
		-I../storage/ \
		-I../scheduler/ \
		-I../proxy/ \
		-I../comm/ \
		-I../comm/disservice/ \
		-I../comm/mockets/ \
		-I../controllers/query/ \
		-I$(DISSERVICE_HOME)/ \
		-I$(UTIL_HOME)/cpp/ \
		-I$(UTIL_HOME)/cpp/net/ \
		-I$(UTIL_HOME)/cpp/graph/ \
		-I$(MOCKETS_HOME)/cpp/ \
		-I$(SQLITE_HOME)/ \
		-I$(TINYXPATH_HOME)/ \
		-I$(C45_HOME)/ \
		-I$(GRP_MANAGER_HOME)/ \
		-I$(LCPPDC_HOME)/ \
		-I$(CHUNKING_HOME)/

LD_FLAGS = -lpthread -ldl

