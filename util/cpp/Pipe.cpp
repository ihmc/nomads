/*
 * Pipe.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "Pipe.h"

#include <stdio.h>

#if defined (WIN32)
    #define close _close
    #define write _write
    #define read _read
#endif

using namespace NOMADSUtil;

Pipe::Pipe (void)
{
    _fdRead = -1;
    _fdWrite = -1;
}

Pipe::Pipe (int fdRead, int fdWrite)
{
    _fdRead = fdRead;
    _fdWrite = fdWrite;
}

Pipe::~Pipe (void)
{
    // Close the write handle first so that any threads blocked trying
    //     to read
    if (_fdWrite >= 0) {
        close (_fdWrite);
    }
    if (_fdRead >= 0) {
        close (_fdRead);
    }
}

int Pipe::init (void)
{
    int fd[2];
    #if defined (WIN32)
        if (_pipe(fd, 0, _O_BINARY)) {
            return -1;
        }
    #elif defined (UNIX)
        if (pipe (fd)) {
            return -1;
        }
    #endif
    _fdRead = fd[0];
    _fdWrite = fd[1];
    return 0;
}

int Pipe::sendBytes (void *pData, unsigned long ulSize)
{
    if (ulSize != write (_fdWrite, pData, ulSize)) {
        return -1;
    }
    return 0;
}

int Pipe::receive (void *pBuf, int iSize)
{
    return read (_fdRead, pBuf, iSize);
}

int Pipe::receiveBytes (void *pBuf, unsigned long ulSize)
{
    unsigned long ulTotalBytesRead = 0;
    while (ulTotalBytesRead < ulSize) {
        long lBytesRead;
        if ((lBytesRead = read (_fdRead, &((char*)pBuf)[ulTotalBytesRead], ulSize-ulTotalBytesRead)) < 0) {
            return -1;
        }
        ulTotalBytesRead += lBytesRead;
    }
    return 0;
}

unsigned long Pipe::bytesAvail (void)
{
    if (fstat (_fdRead, &statbuf)) {
        return 0;
    }
    return statbuf.st_size;
}
