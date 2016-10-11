/*
 * ZLibConnectorReader.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include <cstring>
#include <cstdlib>
#if defined (LINUX)
    #include <algorithm>
#endif

#include "Logger.h"

#include "ZLibConnectorReader.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    ZLibConnectorReader::ZLibConnectorReader (const CompressionSetting * const pCompressionSetting, unsigned int ulInBufSize, unsigned int ulOutBufSize, bool bNoWrapMode)
        : ConnectorReader (pCompressionSetting), _bNoWrapMode (bNoWrapMode)
    {
         if ((ulOutBufSize == 0) || (ulInBufSize == 0) ||
             (NULL == (_pOutputBuffer = new unsigned char[ulOutBufSize])) ||
             (NULL == (_pInputBuffer = new unsigned char[ulInBufSize]))) {
            // throw a c++ exception here
        }
        _ulInBufSize = ulInBufSize;
        _ulOutBufSize = ulOutBufSize;

        resetDecompStream();

        int iWindowBits = 15;              // Default window bits (from zlib.h)
        if (bNoWrapMode) {
            iWindowBits = -iWindowBits;    // inflate.c uses this backdoor method to set the "nowrap" option
        }
        if (Z_OK != inflateInit2 (&_zsDecompStream, iWindowBits)) {
            // throw a c++ exception here
        }
    }

    ZLibConnectorReader::~ZLibConnectorReader (void)
    {
        inflateEnd (&_zsDecompStream);
        if (_pOutputBuffer) {
            delete[] _pOutputBuffer;
        }
        _pOutputBuffer = NULL;
        if (_pInputBuffer) {
            delete[] _pInputBuffer;
        }
        _pInputBuffer = NULL;
    }

    int ZLibConnectorReader::receiveTCPDataProxyMessage (const uint8 *const ui8SrcData, uint16 ui16SrcLen, uint8 **pDest, uint32 &ui32DestLen)
    {
        int rc = 0;
        ui32DestLen = 0;
        // Check if the buffer is large enough; if not, reallocate it
        if (_ulInBufSize < (_zsDecompStream.avail_in + ui16SrcLen)) {
            unsigned int uiIncreaseRate = 2;
            while ((uiIncreaseRate * _ulInBufSize) < (_zsDecompStream.avail_in + ui16SrcLen)) {
                uiIncreaseRate *= 2;
            }
            if (NULL == (_pInputBuffer = (unsigned char*) realloc (_pInputBuffer, uiIncreaseRate*_ulInBufSize))) {
                checkAndLogMsg ("ZLibConnectorReader::receiveTCPDataProxyMessage", Logger::L_MildError,
                                "error reallocating memory for _pInputBuffer; impossible to increase size from %u to %u bytes\n",
                                _ulInBufSize, uiIncreaseRate * _ulInBufSize);
                return -1;
            }
            _zsDecompStream.next_in = _pInputBuffer;
            _ulInBufSize = uiIncreaseRate * _ulInBufSize;
        }
        memcpy (_pInputBuffer + _zsDecompStream.avail_in, ui8SrcData, ui16SrcLen);
        _zsDecompStream.avail_in += ui16SrcLen;

        bool bIterate;
        do {
            bIterate = false;
            unsigned int uiOldAvailableBytes = _zsDecompStream.avail_out;
            rc = inflate (&_zsDecompStream, Z_SYNC_FLUSH);
            if ((rc != Z_OK) && (rc != Z_STREAM_END)) {
                checkAndLogMsg ("ZLibConnectorReader::receiveTCPDataProxyMessage", Logger::L_MildError,
                                "error calling inflate() with flag Z_SYNC_FLUSH; returned error code is %d\n", rc);
                return -2;
            }
            ui32DestLen += uiOldAvailableBytes - _zsDecompStream.avail_out;

            if (_zsDecompStream.avail_out == 0) {
                bIterate = true;
                _ulOutBufSize *= 2;
                _pOutputBuffer = (unsigned char *) realloc (_pOutputBuffer, _ulOutBufSize);
            }
            _zsDecompStream.avail_out = _ulOutBufSize - ui32DestLen;
            _zsDecompStream.next_out = _pOutputBuffer + ui32DestLen;
        } while (bIterate);

        if (_zsDecompStream.avail_out < _ulOutBufSize) {
            *pDest = _pOutputBuffer;
        }
        else {
            *pDest = NULL;
            ui32DestLen = 0;
        }

        _zsDecompStream.avail_out = _ulOutBufSize;
        _zsDecompStream.next_out = _pOutputBuffer;
        if (_zsDecompStream.avail_in > 0) {
            // Any useful byte is always at the beginning of the buffer
            memmove (_pInputBuffer, _zsDecompStream.next_in, _zsDecompStream.avail_in);
        }
        _zsDecompStream.next_in = _pInputBuffer;

        return 0;
    }

    void ZLibConnectorReader::resetDecompStream (void)
    {
        _zsDecompStream.zalloc = NULL;
        _zsDecompStream.zfree = NULL;
        _zsDecompStream.opaque = (voidpf) 0;

        _zsDecompStream.next_in = _pInputBuffer;
        _zsDecompStream.avail_in = 0;
        _zsDecompStream.total_in = 0;
        _zsDecompStream.next_out = _pOutputBuffer;
        _zsDecompStream.avail_out = _ulOutBufSize;
        _zsDecompStream.total_out = 0;
    }

    int ZLibConnectorReader::resetConnectorReader (void)
    {
        if (Z_STREAM_ERROR == inflateEnd (&_zsDecompStream)) {
            return -1;
        }

        resetDecompStream();

        int iWindowBits = 15;              // Default window bits (from zlib.h)
        if (_bNoWrapMode) {
            iWindowBits = -iWindowBits;    // inflate.c uses this backdoor method to set the "nowrap" option
        }
        if (Z_OK != inflateInit2 (&_zsDecompStream, iWindowBits)) {
            return -2;
        }

        return 0;
    }

}
