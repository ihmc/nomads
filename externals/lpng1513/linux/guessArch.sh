#!/bin/sh

#
# guessArch.sh
#
# Discover and prints the local computer architecture
# It can be embedded in Makefile in order to choose the
# proper compiler configuration flags and libraries to
# link to.
#
# author: Giacomo Benincasa	(gbenincasa@ihmc.us)
#

kernel=`uname -s`
case "$kernel" in
	*Linux)
		arch=`uname -m`
		case "$arch" in
			*86_64)	arch=linux64;;
			*arm*) arch=linux-arm;;
			*) arch=linux;;
		esac;;
	*Darwin)
        	arch=`uname -m`
		case "$arch" in
			*i386) arch=osx;;
			*86_64) arch=osx64;;
			*) arch=osxppc;;
		esac;;
esac

echo $arch
