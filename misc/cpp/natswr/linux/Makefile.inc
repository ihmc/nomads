
ARCH = $(shell sh ../../../../util/scripts/guessArch.sh)
MakefileArch=../../../../util/scripts/Makefile.$(ARCH)
include $(MakefileArch)

NOMADS_HOME = ../../../..
UTIL_HOME = $(NOMADS_HOME)/util
