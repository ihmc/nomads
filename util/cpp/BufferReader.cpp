/*
 * BufferReader.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#if defined (ANDROID)
	#include <string.h>
#endif

#include "BufferReader.h"

using namespace NOMADSUtil;

BufferReader::BufferReader (const void *pBuf, uint32 ui32BufLen, bool bDeleteWhenDone)
{
    _pui8Buf = (const uint8 *) pBuf;
    _ui32BufLen = ui32BufLen;
    _ui32NextPtr = 0;
    _bDeleteWhenDone = bDeleteWhenDone;
}

BufferReader::~BufferReader (void)
{
    if ((_bDeleteWhenDone) && (_pui8Buf != NULL)) {
        free ((void*) _pui8Buf);
    }
    _pui8Buf = NULL;
    _ui32BufLen = 0;
    _ui32NextPtr = 0;
}

int BufferReader::init (const void *pBuf, uint32 ui32BufLen, bool bDeleteWhenDone)
{
    if ((pBuf == NULL) || (ui32BufLen == 0U)) {
        return 1;
    }
    _pui8Buf = (const uint8 *) pBuf;
    _ui32BufLen = ui32BufLen;
    return 0;
}

int BufferReader::read (void *pBuf, int iCount)
{
    int iBytesAvail = (_ui32BufLen - _ui32NextPtr);
    if (iCount > iBytesAvail) {
        iCount = iBytesAvail;
    }
    if (iCount > 0) {
        memcpy (pBuf, &_pui8Buf[_ui32NextPtr], iCount);
        _ui32NextPtr += iCount;
    }

    _ui32TotalBytesRead += iCount;
    return iCount;
}

int BufferReader::readBytes (void *pBuf, uint32 ui32Count)
{
    if (_ui32NextPtr+(ui32Count-1) >= _ui32BufLen) {
        return -1;
    }

    memcpy (pBuf, &_pui8Buf[_ui32NextPtr], ui32Count);
    _ui32NextPtr += ui32Count;

    _ui32TotalBytesRead += ui32Count;
    return 0;
}

int BufferReader::skipBytes (uint32 ui32Count)
{
    if (_ui32NextPtr+(ui32Count-1) >= _ui32BufLen) {
        return -1;
    }

    _ui32NextPtr += ui32Count;
    _ui32TotalBytesRead += ui32Count;
    return 0;
}

uint32 BufferReader::getBytesAvailable (void)
{
    return (_ui32BufLen - _ui32NextPtr);
}

int BufferReader::setPosition (uint32 ui32Index)
{
    if (ui32Index > _ui32BufLen) {
        return -1;
    }

    _ui32NextPtr = ui32Index;
    return 0;
}

