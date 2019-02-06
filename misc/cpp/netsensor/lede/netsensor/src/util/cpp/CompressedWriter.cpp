/*
 * CompressedWriter.cpp
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

#include "CompressedWriter.h"

using namespace NOMADSUtil;

CompressedWriter::CompressedWriter (Writer *pw, bool bDeleteWhenDone, unsigned long ulOutBufSize)
{
    if ((pw == NULL) || (ulOutBufSize == 0)) {
        // Throw C++ exception here 
    }
    _pWriter = pw;
    _ulOutBufSize = ulOutBufSize;
    _bDeleteWhenDone = bDeleteWhenDone;
    if ((_pOutputBuffer = new char [_ulOutBufSize]) == NULL) {
        // throw C++ exception here
    }
    // init dynamic memory management functions
    _zsCompStream.zalloc = alloc_mem;
    _zsCompStream.zfree = free_mem;
    _zsCompStream.opaque = (voidpf) 0;
    // init necessary compression state tracking vars
    _zsCompStream.avail_in = 0;
    _zsCompStream.next_in = Z_NULL;
    _zsCompStream.total_in = 0;
    _zsCompStream.avail_out = _ulOutBufSize;
    _zsCompStream.next_out = (unsigned char *) _pOutputBuffer;
    _zsCompStream.total_out = 0;
    if (Z_OK != deflateInit (&_zsCompStream, Z_BEST_SPEED)) {
        // Throw C++ exception here
    }
    _bFlushed = true;
}

CompressedWriter::~CompressedWriter ()
{
    flush();
    deflateEnd (&_zsCompStream);
    if (_bDeleteWhenDone) {
        delete _pWriter;
    }
    _pWriter = NULL;
    if (_pOutputBuffer) {
        delete[] _pOutputBuffer;
    }
    _pOutputBuffer = NULL;
}

int CompressedWriter::flush()
{
    if (_bFlushed) {
        return 0;
    }
    bool bDone = false;
    while (!bDone) {
        int rc;
        if (0 != (rc = deflate (&_zsCompStream, Z_FINISH))) {
            if (rc == Z_STREAM_END) {
                bDone = true;
            }
            else {
                return -1;
            }
        }
        if (_zsCompStream.avail_out < _ulOutBufSize) {
            if (_pWriter->writeBytes (_pOutputBuffer, (_ulOutBufSize - _zsCompStream.avail_out))) {
                return -2;
            }
            _zsCompStream.avail_out = _ulOutBufSize;
            _zsCompStream.next_out = (unsigned char*) _pOutputBuffer;
        }
        else {
            if (!bDone) {
                // deflate was not done but did not put anything into the output buffer
                return -3;
            }
        }
    }
    deflateReset (&_zsCompStream);
    return 0;
}

int CompressedWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    _zsCompStream.avail_in = ulCount;
    _zsCompStream.next_in = (unsigned char*) pBuf;

    while (_zsCompStream.avail_in > 0) {
        int rc;
        if (0 != (rc = deflate (&_zsCompStream, Z_NO_FLUSH))) {
            return -1;
        }
        if (_zsCompStream.avail_out == 0) {
            // Output buffer is full, so write that out
            if (_pWriter->writeBytes (_pOutputBuffer, _ulOutBufSize)) {
                return -2;
            }
            _zsCompStream.avail_out = _ulOutBufSize;
            _zsCompStream.next_out = (unsigned char*) _pOutputBuffer;
        }
    }
    _bFlushed = false;
    return 0;
}

void * CompressedWriter::alloc_mem (void *userdata, uInt items, uInt size)
{
    return malloc (items * size);
}

void CompressedWriter::free_mem (void *userdata, void *data)
{
    free (data);
}

