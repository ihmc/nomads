/*
 * ZLibConnectorReader.cpp
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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

#include <cstring>
#include <cstdlib>

#include "Logger.h"

#include "LzmaConnectorReader.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    LzmaConnectorReader::LzmaConnectorReader (const CompressionSetting * const pCompressionSetting, unsigned int ulInBufSize, unsigned int ulOutBufSize) : ConnectorReader (pCompressionSetting)
    {
         if ((ulOutBufSize == 0) || (ulInBufSize == 0) || 
             (NULL == (_pOutputBuffer = new unsigned char[ulOutBufSize])) ||
             (NULL == (_pInputBuffer = new unsigned char[ulInBufSize]))) {
            // throw a c++ exception here
        }
        _ulInBufSize = ulInBufSize;
        _ulOutBufSize = ulOutBufSize;

        resetDecompStream();

        if (LZMA_OK != lzma_stream_decoder (&_lzmaDecompStream, lzma_easy_decoder_memusage (getCompressionLevel()), DECODER_FLAGS)) {
            // throw a c++ exception here
        }
    }

    LzmaConnectorReader::~LzmaConnectorReader (void)
    {
        lzma_end (&_lzmaDecompStream);
        if (_pOutputBuffer) {
            delete[] _pOutputBuffer;
        }
        _pOutputBuffer = NULL;
        if (_pInputBuffer) {
            delete[] _pInputBuffer;
        }
        _pInputBuffer = NULL;
    }

    int LzmaConnectorReader::receiveTCPDataProxyMessage (const uint8 *const ui8SrcData, uint16 ui16SrcLen, uint8 **pDest, uint32 &ui32DestLen)
    {
        int rc = 0;
        ui32DestLen = 0;
        // Check if the buffer is large enough; if not, reallocate it
        if (_ulInBufSize < (_lzmaDecompStream.avail_in + ui16SrcLen)) {
            unsigned int uiIncreaseRate = 2;
            while ((uiIncreaseRate * _ulInBufSize) < (_lzmaDecompStream.avail_in + ui16SrcLen)) {
                uiIncreaseRate *= 2;
            }
            if (NULL == (_pInputBuffer = (unsigned char*) realloc (_pInputBuffer, uiIncreaseRate * _ulInBufSize))) {
                checkAndLogMsg ("LzmaConnectorReader::receiveTCPDataProxyMessage", Logger::L_MildError,
                                "error reallocating memory for _pInputBuffer; impossible to increase size from %u to %u bytes\n",
                                _ulInBufSize, uiIncreaseRate*_ulInBufSize);
                return -1;
            }
            _lzmaDecompStream.next_in = _pInputBuffer;
            _ulInBufSize = uiIncreaseRate * _ulInBufSize;
        }
        memcpy (_pInputBuffer + _lzmaDecompStream.avail_in, ui8SrcData, ui16SrcLen);
        _lzmaDecompStream.avail_in += ui16SrcLen;

        bool bIterate;
        do {
            bIterate = false;
            unsigned int uiOldAvailableBytes = _lzmaDecompStream.avail_out;
            rc = lzma_code (&_lzmaDecompStream, LZMA_RUN);
            if (rc > LZMA_STREAM_END) {
                if (rc == LZMA_MEMLIMIT_ERROR) {
                    // doubling memory limit
                    if (LZMA_OK != (rc = lzma_memlimit_set (&_lzmaDecompStream, 2*lzma_memlimit_get (&_lzmaDecompStream)))) {
                        checkAndLogMsg ("LzmaConnectorReader::receiveTCPDataProxyMessage", Logger::L_MildError,
                                        "error calling lzma_memlimit_set() with new memory limit equal to %llu bytes; returned error code is %d\n",
                                        2*lzma_memlimit_get (&_lzmaDecompStream), rc);
                        return -2;
                    }
                    bIterate = true;

                    ui32DestLen += uiOldAvailableBytes - _lzmaDecompStream.avail_out;
                    if ((_lzmaDecompStream.avail_out == 0) && (rc == LZMA_OK)) {
                        _ulOutBufSize *= 2;
                        _pOutputBuffer = (unsigned char *) realloc (_pOutputBuffer, _ulOutBufSize);
                    }
                    _lzmaDecompStream.avail_out = _ulOutBufSize - ui32DestLen;
                    _lzmaDecompStream.next_out = _pOutputBuffer + ui32DestLen;

                    checkAndLogMsg ("LzmaConnectorReader::receiveTCPDataProxyMessage", Logger::L_Warning,
                                    "set a new memory limit of %llu bytes in the LZMA decompressor", lzma_memlimit_get (&_lzmaDecompStream));
                    continue;
                }
                else if ((rc >= LZMA_NO_CHECK) && (rc <= LZMA_GET_CHECK)) {
                    checkAndLogMsg ("LzmaConnectorReader::receiveTCPDataProxyMessage", Logger::L_HighDetailDebug,
                                    "calling to lzma_code() with flag LZMA_RUN returned check code %d; ignoring\n", rc);
                    bIterate = true;
                    continue;
                }
                checkAndLogMsg ("LzmaConnectorReader::receiveTCPDataProxyMessage", Logger::L_MildError,
                                "error calling lzma_code() with flag LZMA_RUN; returned error code is %d\n", rc);
                return -3;
            }
            ui32DestLen += uiOldAvailableBytes - _lzmaDecompStream.avail_out;

            if ((_lzmaDecompStream.avail_out == 0) && (rc == LZMA_OK)) {
                bIterate = true;
                _ulOutBufSize *= 2;
                _pOutputBuffer = (unsigned char *) realloc (_pOutputBuffer, _ulOutBufSize);
            }
            _lzmaDecompStream.avail_out = _ulOutBufSize - ui32DestLen;
            _lzmaDecompStream.next_out = _pOutputBuffer + ui32DestLen;
        } while (bIterate);

        if (_lzmaDecompStream.avail_out < _ulOutBufSize) {
            *pDest = _pOutputBuffer;
        }
        else {
            *pDest = NULL;
            ui32DestLen = 0;
        }

        _lzmaDecompStream.avail_out = _ulOutBufSize;
        _lzmaDecompStream.next_out = _pOutputBuffer;
        if (_lzmaDecompStream.avail_in > 0) {
            // Any useful byte is always at the beginning of the buffer
            memmove (_pInputBuffer, _lzmaDecompStream.next_in, _lzmaDecompStream.avail_in);
        }
        _lzmaDecompStream.next_in = _pInputBuffer;

        return 0;
    }

    void LzmaConnectorReader::resetDecompStream (void)
    {
        _lzmaDecompStream.allocator = NULL;
        _lzmaDecompStream.next_in = _pInputBuffer;
        _lzmaDecompStream.avail_in = 0;
        _lzmaDecompStream.total_in = 0;
        _lzmaDecompStream.next_out = _pOutputBuffer;
        _lzmaDecompStream.avail_out = _ulOutBufSize;
        _lzmaDecompStream.total_out = 0;
        _lzmaDecompStream.allocator = NULL;
        _lzmaDecompStream.internal = NULL;
        _lzmaDecompStream.reserved_ptr1 = NULL;
        _lzmaDecompStream.reserved_ptr2 = NULL;
        _lzmaDecompStream.reserved_ptr3 = NULL;
        _lzmaDecompStream.reserved_ptr4 = NULL;
        _lzmaDecompStream.reserved_int1 = 0;
        _lzmaDecompStream.reserved_int2 = 0;
        _lzmaDecompStream.reserved_int3 = 0;
        _lzmaDecompStream.reserved_int4 = 0;
        _lzmaDecompStream.reserved_enum1 = LZMA_RESERVED_ENUM;
        _lzmaDecompStream.reserved_enum2 = LZMA_RESERVED_ENUM;
    }

    int LzmaConnectorReader::resetConnectorReader (void)
    {
        lzma_end (&_lzmaDecompStream);

        resetDecompStream();

        if (LZMA_OK != lzma_stream_decoder (&_lzmaDecompStream, lzma_easy_decoder_memusage (getCompressionLevel()), DECODER_FLAGS)) {
            return -1;
        }

        return 0;
    }

    void * LzmaConnectorReader::alloc_mem (void *userdata, uint32_t items, uint32_t size)
    {
        return malloc (items * size);
    }

    void LzmaConnectorReader::free_mem (void *userdata, void *data)
    {
        free (data);
    }

}
