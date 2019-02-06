/*
 * BufferWriter.cpp
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

#include "BufferWriter.h"

#include <memory.h>
#include <stdlib.h>

using namespace NOMADSUtil;

BufferWriter::BufferWriter (void)
{
    _pBuf = (char*) malloc (DEFAULT_INITIAL_SIZE);
    _ulBufSize = DEFAULT_INITIAL_SIZE;
    _ulPos = 0U;
    _ulIncrement = DEFAULT_INCREMENT;
}

BufferWriter::BufferWriter (unsigned long ulInitialSize, unsigned long ulIncrement)
{
    _pBuf = (char*) malloc (ulInitialSize);
    _ulBufSize = ulInitialSize;
    _ulPos = 0U;
    _ulIncrement = ulIncrement;
}

BufferWriter::~BufferWriter (void)
{
    if (_pBuf) {
        free (_pBuf);
        _pBuf = NULL;
    }
    _ulBufSize = 0U;
    _ulPos = 0U;
}

int BufferWriter::init (char *pBuf, unsigned long uiLen, unsigned long ulIncrement)
{
    if (_pBuf != NULL) {
        free (_pBuf);
    }
    _pBuf = pBuf;
    _ulBufSize = uiLen;
    _ulPos = 0;
    _ulIncrement = ulIncrement;
    return 0;
}

char * BufferWriter::reliquishAndSetBuffer (char *pNewBuf, unsigned long ulCount)
{
    char *pRet = relinquishBuffer();
    _pBuf = pNewBuf;
    _ulBufSize = _ulPos = ulCount;
    return pRet;
}

int BufferWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    if (_pBuf == NULL) {
        return -1;
    }
    if ((_ulPos + ulCount) >= _ulBufSize) {
        unsigned long ulNewSize = _ulBufSize + _ulIncrement;
        if ((_ulPos + ulCount) >= ulNewSize) {
            ulNewSize = _ulPos + ulCount + _ulIncrement;
        }
        char *pNewBuf;
        if (NULL == (pNewBuf = (char*) realloc (_pBuf, ulNewSize))) {
            return -1;
        }
        _pBuf = pNewBuf;
        _ulBufSize = ulNewSize;
    }
    memcpy (_pBuf + _ulPos, pBuf, ulCount);
    _ulPos += ulCount;
    return 0;
}
