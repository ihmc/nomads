/*
 * BufferedWriter.cpp
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

#include "BufferedWriter.h"

#include <string.h>

using namespace NOMADSUtil;

BufferedWriter::BufferedWriter (Writer *pw, unsigned long ulBufSize)
{
    if ((pw == NULL) || (ulBufSize == 0)) {
        // Throw C++ exception here     /*!!*/
    }
    _pWriter = pw;
    if (NULL == (_pBuf = new char [ulBufSize])) {
        // Throw C++ exception here     /*!!*/
    }
    _ulBufSize = ulBufSize;
    _ulBufCount = 0;
}

BufferedWriter::~BufferedWriter (void)
{
    flush();
    delete[] _pBuf;
}

int BufferedWriter::flush (void)
{
    if (_ulBufCount > 0) {
        if (_pWriter->writeBytes (_pBuf, _ulBufCount)) {
            return -1;
        }
        _ulBufCount = 0;
    }
    return 0;
}

int BufferedWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    if ((_ulBufCount+ulCount) <= _ulBufSize) {
        // There is room in the buffer, just place it there
        memcpy (&_pBuf[_ulBufCount], pBuf, ulCount);
        _ulBufCount += ulCount;
    }
    else {
        // There is no room in the buffer, so flush the buffer first
        if (flush()) {
            return -1;
        }
        if (ulCount > _ulBufSize) {
            // Too large to fit in buffer - write directly
            if (_pWriter->writeBytes (pBuf, ulCount)) {
                return -2;
            }
        }
        else {
            // Now there is room (after flushing) so put the data in the buffer
            memcpy (&_pBuf[_ulBufCount], pBuf, ulCount);
            _ulBufCount += ulCount;
        }
    }
    return 0;
}
