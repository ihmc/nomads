/*
 * CompressedReader.cpp
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

#include "CompressedReader.h"

using namespace NOMADSUtil;

CompressedReader::CompressedReader (Reader *pr, bool bDeleteWhenDone, bool bNoWrapMode, unsigned long ulInBufSize)
{
    if ((pr == NULL) || (ulInBufSize == 0)) {
        // throw a C++ exception here
    }
    _pReader = pr;
    _bDeleteWhenDone = bDeleteWhenDone;
    _ulInBufSize = ulInBufSize;

    if (NULL == (_pInputBuffer = new char [_ulInBufSize])) {
        // Throw a C++ exception here
    }
    // init decompression stream
    _zsDecompStream.zalloc = alloc_mem;
    _zsDecompStream.zfree = free_mem;
    _zsDecompStream.opaque = (voidpf) 0;
    _zsDecompStream.next_in = Z_NULL;
    _zsDecompStream.avail_in = 0;
    _zsDecompStream.total_in = 0;
    _zsDecompStream.next_out = Z_NULL;
    _zsDecompStream.avail_out = 0;
    _zsDecompStream.total_out = 0;
    int iWindowBits = 15;              // Default window bits (from zlib.h)
    if (bNoWrapMode) {
        iWindowBits = -iWindowBits;    // inflate.c uses this backdoor method to set the "nowrap" option
    }
    if (Z_OK != inflateInit2 (&_zsDecompStream, iWindowBits)) {
        // throw a c++ exception here
    }
}

CompressedReader::~CompressedReader (void)
{
    inflateEnd (&_zsDecompStream);
    if (_bDeleteWhenDone) {
        delete _pReader;
    }
    _pReader = NULL;
    if (_pInputBuffer) {
        delete[] _pInputBuffer;
    }
    _pInputBuffer = NULL;
}

int CompressedReader::read (void *pBuf, int iCount)
{
    int rc;
    _zsDecompStream.avail_out = iCount;
    _zsDecompStream.next_out = (unsigned char*) pBuf;
    if (_zsDecompStream.avail_in == 0) {
        // Need to read more data
        if ((rc = _pReader->read (_pInputBuffer, _ulInBufSize)) < 0) {
            return -1;
        }
        _zsDecompStream.avail_in = rc;
        _zsDecompStream.next_in = (unsigned char*) _pInputBuffer;
    }
    rc = inflate (&_zsDecompStream, Z_NO_FLUSH);
    if ((rc != Z_OK) && (rc != Z_STREAM_END)) {
        return -2;
    }
    return (iCount - _zsDecompStream.avail_out);
}

int CompressedReader::readBytes (void *pBuf, uint32 ui32Count)
{
    int rc;
    _zsDecompStream.avail_out = ui32Count;
    _zsDecompStream.next_out = (unsigned char*) pBuf;
    while (_zsDecompStream.avail_out > 0) {
        if (_zsDecompStream.avail_in == 0) {
            // Need to read more data
            if ((rc = _pReader->read (_pInputBuffer, _ulInBufSize)) <= 0) {
                printf ("read failed with rc = %d\n", rc);
                return -1;
            }
            _zsDecompStream.avail_in = rc;
            _zsDecompStream.next_in = (unsigned char*) _pInputBuffer;
        }
        if (0 != (rc = inflate (&_zsDecompStream, Z_NO_FLUSH))) {
            if ((rc == Z_STREAM_END) && (_zsDecompStream.avail_out == 0)) {
                return 0;
            }
            printf ("inflate failed; rc = %d; avail_out = %d\n", rc, _zsDecompStream.avail_out);
            return -2;
        }
    }
    return 0;
}

void * CompressedReader::alloc_mem (void *userdata, uInt items, uInt size)
{
    return malloc (items * size);
}

void CompressedReader::free_mem (void *userdata, void *data)
{
    free (data);
}
