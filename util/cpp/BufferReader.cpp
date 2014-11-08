/*
 * BufferReader.cpp
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
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

