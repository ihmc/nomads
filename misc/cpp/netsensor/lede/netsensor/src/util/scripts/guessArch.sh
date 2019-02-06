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
arch=`uname -m`
case "$kernel" in
	*Linux)
		ver=`ldd --version | head -1`
		case "$ver" in
			*2.2*)
			case "$arch" in
				aarch64) arch=linux64-arm-glibc22;;
				*arm*) arch=linux-arm-glibc22;;
				*86_64)	arch=linux64-glibc22;;
				*) arch=linux-glibc22;;
		        esac;;
			*)
			case "$arch" in
                aarch64) arch=linux64-arm;;
				*arm*) arch=linux-arm;;
				*86_64)	arch=linux64;;
				*) arch=linux;;
			esac;;
		esac;;
	*Darwin)
		case "$arch" in
			*86_64) arch=osx64;;
			*i386) arch=osx;;
			*) arch=osxppc;;
		esac;;
esac

echo $arch
