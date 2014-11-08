#!/bin/sh

kernel=`uname -s`
case "$kernel" in
	*Linux)
		arch=`uname -m`
		ver=`ldd --version | head -1`
		case "$ver" in
			*2.2*) arch=linux-x86-glibc22;;
			*)
			case "$arch" in
				*86_64)	arch=linux-x86_64;;
				*arm*) arch=linux-arm;;
				*) arch=linux-x86;;
			esac;;
		esac;;
	*Darwin)
        	arch=`uname -m`
		case "$arch" in
			*i386) arch=osx-intel;;
			*86_64) arch=osx-intel_64;;
			*) arch=osx-ppc;;
		esac;;
esac

echo $arch
