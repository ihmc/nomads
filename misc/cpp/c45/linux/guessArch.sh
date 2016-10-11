#!/bin/sh

kernel=`uname -s`
case "$kernel" in
	*Linux)
		arch=`uname -m`
		ver=`ls /lib/libc-*.so`
		case "$ver" in
			*2.2*) arch=linux-glibc22;;
			*)
			case "$arch" in
				*86_64)	arch=linux64;;
				*) arch=linux;;
			esac;;
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
