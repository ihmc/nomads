/*
 * BufferedReader.cpp
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

#include "BufferedReader.h"

#include <string.h>

using namespace NOMADSUtil;

BufferedReader::BufferedReader (Reader *pr, bool bDeleteWhenDone, uint32 ui32BufSize)
{
    if ((pr == NULL) || (ui32BufSize == 0)) {
        // Throw C++ exception here     /*!!*/
    }
    _pReader = pr;
    _bDeleteReaderWhenDone = bDeleteWhenDone;
    if (NULL == (_pBuf = new char [ui32BufSize])) {
        // Throw C++ exception here     /*!!*/
    }
    _ui32BufSize = ui32BufSize;
    _ui32BufStart = _ui32BufEnd = 0;
}

BufferedReader::~BufferedReader (void)
{
    if (_bDeleteReaderWhenDone) {
        delete _pReader;
    }
    _pReader = NULL;
    delete[] _pBuf;
    _pBuf = NULL;
}

int BufferedReader::read (void *pBuf, int iCount)
{
    if ((_ui32BufEnd-_ui32BufStart) > 0) {
        // There is some data in the buffer - return that
        int iBytesAvail = _ui32BufEnd - _ui32BufStart;
        if (iBytesAvail > iCount) {
            memcpy (pBuf, &_pBuf[_ui32BufStart], iCount);
            _ui32BufStart += iCount;
            return iCount;
        }
        else {
            memcpy (pBuf, &_pBuf[_ui32BufStart], iBytesAvail);
            _ui32BufStart += iBytesAvail;
            return iBytesAvail;
        }
    }
    else {
        // There is no data in the buffer - try to read as much as possible
        _ui32BufStart = 0;
        _ui32BufEnd = 0;
        int rc;
        if ((rc = _pReader->read (_pBuf, _ui32BufSize)) < 0) {
            return -1;
        }
        else if (rc == 0) {
            return 0;
        }
        _ui32BufEnd = rc;
        return read (pBuf, iCount);
    }
}

int BufferedReader::readBytes (void *pBuf, uint32 ui32Count)
{
    if ((_ui32BufEnd - _ui32BufStart) >= ui32Count) {
        // There is enough data in the buffer, just copy it
        memcpy (pBuf, &_pBuf[_ui32BufStart], ui32Count);
        _ui32BufStart += ui32Count;
    }
    else {
        // There is not enough data in the buffer to service the request
        // First empty the buffer and copy as much data as possible
        uint32 ui32BytesCopied = 0;
        if ((_ui32BufEnd - _ui32BufStart) > 0) {
            ui32BytesCopied = _ui32BufEnd - _ui32BufStart;
            memcpy (pBuf, &_pBuf[_ui32BufStart], ui32BytesCopied);
        }
        if (ui32BytesCopied < ui32Count) {
            // At this point, the buffer is empty, so fill the buffer
            _ui32BufStart = 0;
            int rc;
            if ((rc = _pReader->read (_pBuf, _ui32BufSize)) <= 0) {
                return -1;
            }
            _ui32BufEnd = rc;
            return readBytes (&((char*)pBuf)[ui32BytesCopied], ui32Count - ui32BytesCopied);
        }
    }
    return 0;
}

uint32 BufferedReader::getBytesAvailable (void)
{
    return (_ui32BufEnd - _ui32BufStart) + _pReader->getBytesAvailable();
}
