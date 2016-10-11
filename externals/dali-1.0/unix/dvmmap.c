/*
 * dvmmap.c
 * platform specific file for doing mmaps on unix
 * Basically a wrapper around the mmap calls so as to present the
 * same interface to the rest of the dvm.
 *
 * 1/1/98 - sugata
 */

#ifdef HAVE_MMAP
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dvmbasic.h"


/************************************************************************
 * Function: Dvm_Mmap
 *
 * Parameters: look at mmap(2) on solaris. The exceptions are:
 *             the file name is used instead of the file descriptor.
 *             the PROT flag can only be DVM_MMAP_READ or DVM_MMAP_WRITE
 *             len is a value result param, *len = 0 means map the entire 
 *             file. Returns the length actually mapped in *len.
 *
 * Return: ditto
 *
 * Action: wraps around the unix mmap
 *
 ************************************************************************/

void *
Dvm_Mmap(addr, len, prot, flags, fname, off)
    void *addr;
    int *len;
    int prot;
    int flags;
    char *fname;
    int off;
{
    int fd, oflag, mode = 0, unixprot, unixflags;
    void *retaddr;


    if (prot == DVM_MMAP_READ) {
        oflag = O_RDONLY;
        unixprot = PROT_READ;
    } else if (prot == PROT_WRITE) {
        oflag = O_RDWR;
        unixprot = PROT_WRITE;
        mode = O_CREAT;
    } else {
        return NULL;
    }

    if (flags == DVM_MMAP_MAP_SHARED) {
        unixflags = MAP_SHARED;
    } else if (flags == DVM_MMAP_MAP_PRIVATE) {
        unixflags = MAP_PRIVATE;
    } else {
        return NULL;
    }

    fd = open(fname, oflag, mode);
    if (fd == -1)
        return NULL;

    if (*len == 0) {
        /* Map all of the file starting at offset */
        int size;

        size = lseek(fd, 0, SEEK_END);
        *len = size - off;
    }
    retaddr = mmap(addr, *len, unixprot, unixflags, fd, off);
    close(fd);
    if ((int) retaddr == -1)
        retaddr = NULL;
    return retaddr;
}

/************************************************************************
 * Function: Dvm_Munmap
 *
 * Parameters: look at munmap(2) on solaris. 
 *
 * Return: ditto
 *
 * Action: wraps around the unix munmap call
 *
 ************************************************************************/


int
Dvm_Munmap(addr, len)
    void *addr;
    int len;
{
    return (munmap(addr, len));
}
#endif
