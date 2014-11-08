/*
 * BufferWriter.cpp
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "BufferWriter.h"

#include <memory.h>
#include <stdlib.h>

using namespace NOMADSUtil;

BufferWriter::BufferWriter (void)
{
    _pBuf = (char*) malloc (DEFAULT_INITIAL_SIZE);
    _ulBufSize = DEFAULT_INITIAL_SIZE;
    _ulPos = 0;
    _ulIncrement = DEFAULT_INCREMENT;
}

BufferWriter::BufferWriter (unsigned long ulInitialSize, unsigned long ulIncrement)
{
    _pBuf = (char*) malloc (ulInitialSize);
    _ulBufSize = ulInitialSize;
    _ulPos = 0;
    _ulIncrement = ulIncrement;
}

BufferWriter::~BufferWriter (void)
{
    if (_pBuf) {
        free (_pBuf);
        _pBuf = NULL;
    }
    _ulBufSize = 0;
    _ulPos = 0;
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
