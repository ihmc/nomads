/*
 * dvmmap.c
 * platform specific file for doing mmaps on windows
 *
 * 1/1/98 - sugata
 */

#ifdef HAVE_MMAP
#include <windows.h>
#include "dvmbasic.h"

/************************************************************************
 * Function: Dvm_Mmap
 *
 * Parameters: look at mmap(2) on solaris. The only exception is that
 *             the file name is used instead of the file descriptor.
 *             The addr parameter is not used, set it to zero.
 *             the PROT flag can only be DVM_MMAP_READ or DVM_MMAP_WRITE
 *
 * Return: ditto
 *
 * Action: emulates the unix mmap call on NT
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
    HANDLE hfile, hmap;
    DWORD access, protect, desaccess, exflags;
    void *retaddr;
    int mappedsize;

    if (prot == DVM_MMAP_READ) {
        access = GENERIC_READ;
        desaccess = FILE_MAP_READ;
        protect = PAGE_READONLY | SEC_NOCACHE | SEC_RESERVE;
        exflags = OPEN_EXISTING;
    } else if (prot == DVM_MMAP_WRITE) {
        access = GENERIC_READ | GENERIC_WRITE;
        desaccess = FILE_MAP_WRITE;
        protect = PAGE_READWRITE;
    } else {
        return NULL;
    }

    hfile = CreateFile(fname, access, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    if (*len == 0) {
        mappedsize = SetFilePointer(hfile, 0, 0, FILE_END);
        mappedsize -= off;
    }
    hmap = CreateFileMapping(hfile, NULL, protect, 0, 0, NULL);
    if (hmap == NULL) {
        CloseHandle(hfile);
        return NULL;
    }
    /* Mapping creation successful  */
    CloseHandle(hfile);

    retaddr = MapViewOfFile(hmap, desaccess, 0, off, *len);
    if (retaddr == NULL) {
        CloseHandle(hmap);
        return NULL;
    }
    /* Map View successful */
    CloseHandle(hmap);
    *len = mappedsize;
    return retaddr;
}

/************************************************************************
 * Function: Dvm_Munmap
 *
 * Parameters: look at munmap(2) on solaris. 
 *             the len parameter is not used.
 *
 * Return: ditto
 *
 * Action: emulates the unix munmap call on NT
 *
 ************************************************************************/


int
Dvm_Munmap(addr, len)
    void *addr;
    int len;
{
    BOOL retval;

    retval = UnmapViewOfFile(addr);
    if (retval == TRUE)
        return 0;
    else
        return -1;
}
#endif
