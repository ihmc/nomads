/*
 * ZlibConnectorWriter.cpp
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include <cstdlib>

#include "Logger.h"

#include "ZLibConnectorWriter.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    ZLibConnectorWriter::ZLibConnectorWriter (const CompressionSetting * const pCompressionSetting, unsigned long ulOutBufSize) : ConnectorWriter (pCompressionSetting)
    {
        if (ulOutBufSize == 0) {
            // Throw C++ exception here 
        }
        _ulOutBufSize = ulOutBufSize;
        if ((_pOutputBuffer = new unsigned char [_ulOutBufSize]) == NULL) {
            // throw C++ exception here
        }
        resetCompStream();

        if (Z_OK != deflateInit (&_zsCompStream, getCompressionLevel())) {
            // Throw C++ exception here
        }
    }

    ZLibConnectorWriter::~ZLibConnectorWriter ()
    {
        deflateEnd (&_zsCompStream);
        if (_pOutputBuffer) {
            delete[] _pOutputBuffer;
        }
        _pOutputBuffer = NULL;
    }

    int ZLibConnectorWriter::flush (unsigned char **pDest, unsigned int &uiDestLen)
    {
        if (_bFlushed) {
            *pDest = NULL;
            uiDestLen = 0;
            return 0;
        }

        bool bDone = false;
        _zsCompStream.next_in = NULL;
        _zsCompStream.avail_in = 0;
        uiDestLen = 0;
        while (!bDone) {
            int rc;
            unsigned int uiOldAvailableSpace = _zsCompStream.avail_out;
            if (0 != (rc = deflate (&_zsCompStream, Z_FINISH))) {
                if (rc == Z_STREAM_END) {
                    bDone = true;
                }
                else {
                    checkAndLogMsg ("ZLibConnectorWriter::flush", Logger::L_MildError,
                                    "deflate() with flag Z_FINISH returned with error code %d\n", rc);
                    uiDestLen = 0;
                    *pDest = NULL;
                    return -1;
                }
            }
            if (_zsCompStream.avail_out < _ulOutBufSize) {
                uiDestLen += (uiOldAvailableSpace - _zsCompStream.avail_out);
                if (_zsCompStream.avail_out == 0) {
                    _ulOutBufSize *= 2;
                    _pOutputBuffer = (unsigned char*) realloc (_pOutputBuffer, _ulOutBufSize);
                    if (!_pOutputBuffer) {
                        checkAndLogMsg ("ZLibConnectorWriter::flush", Logger::L_MildError,
                                        "error trying to realloc %u (previously %u) bytes\n",
                                        _ulOutBufSize, _ulOutBufSize/2);
                        uiDestLen = 0;
                        *pDest = NULL;
                        return -2;
                    }
                }
                _zsCompStream.avail_out = _ulOutBufSize - uiDestLen;
                _zsCompStream.next_out = _pOutputBuffer + uiDestLen;
            }
            else if (!bDone) {
                // deflate was not done but did not put anything into the output buffer
                checkAndLogMsg ("ZLibConnectorWriter::flush", Logger::L_MildError,
                                "deflate() with flag Z_FINISH didn't produce new output but returned code is not Z_STREAM_END (code: %d)\n", rc);
                uiDestLen = 0;
                *pDest = NULL;
                return -3;
            }
        }

        _bFlushed = true;
        if (uiDestLen > 0) {
            *pDest = _pOutputBuffer;
        }
        else {
            *pDest = NULL;
        }
        _zsCompStream.avail_out = _ulOutBufSize;
        _zsCompStream.next_out = _pOutputBuffer;

        return 0;
    }

    int ZLibConnectorWriter::writeData (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen, bool bLocalFlush)
    {
        int rc;
        unsigned int uiOldAvailableSpace;
        bool bNeedFlush = false;
        _zsCompStream.next_in = const_cast<unsigned char*> (pSrc);
        _zsCompStream.avail_in = uiSrcLen;
        uiDestLen = 0;
        *pDest = NULL;
        _bFlushed = false;

        while ((_zsCompStream.avail_in > 0) || bNeedFlush) {
            bNeedFlush = false;
            uiOldAvailableSpace = _zsCompStream.avail_out;
            if (bLocalFlush) {
                if (0 != (rc = deflate (&_zsCompStream, Z_SYNC_FLUSH))) {
                    if (rc == Z_BUF_ERROR) {
                        checkAndLogMsg ("ZLibConnectorWriter::writeData", Logger::L_Warning,
                                        "deflate returned with error code Z_BUF_ERROR; returning to caller\n");
                        return 0;
                    }
                    checkAndLogMsg ("ZLibConnectorWriter::writeData", Logger::L_MildError,
                                    "deflate with flag Z_PARTIAL_FLUSH returned with error code %d\n", rc);
                    uiDestLen = 0;
                    *pDest = NULL;
                    return -1;
                }
            }
            else {
                if (0 != (rc = deflate (&_zsCompStream, Z_NO_FLUSH))) {
                    if (rc == Z_BUF_ERROR) {
                        checkAndLogMsg ("ZLibConnectorWriter::writeData", Logger::L_Warning,
                                        "deflate returned with error code Z_BUF_ERROR; returning to caller\n");
                        return 0;
                    }
                    checkAndLogMsg ("ZLibConnectorWriter::writeData", Logger::L_MildError,
                                    "deflate with flag Z_NO_FLUSH returned with error code %d\n", rc);
                    uiDestLen = 0;
                    *pDest = NULL;
                    return -2;
                }
            }

            uiDestLen += uiOldAvailableSpace - _zsCompStream.avail_out;
            if (_zsCompStream.avail_out == 0) {
                bNeedFlush = true;
                _ulOutBufSize *= 2;
                _pOutputBuffer = (unsigned char *) realloc (_pOutputBuffer, _ulOutBufSize);
            }
            _zsCompStream.avail_out = _ulOutBufSize - uiDestLen;
            _zsCompStream.next_out = _pOutputBuffer + uiDestLen;
        }

        if (uiDestLen > 0) {
            *pDest = _pOutputBuffer;
        }
        else {
            *pDest = NULL;
        }
        _zsCompStream.avail_out = _ulOutBufSize;
        _zsCompStream.next_out = _pOutputBuffer;

        return 0;
    }

    int ZLibConnectorWriter::writeDataAndResetWriter (const unsigned char *pSrc, unsigned int uiSrcLen, unsigned char **pDest, unsigned int &uiDestLen)
    {
        int rc;
        unsigned int uiOldAvailableSpace = 0;
        *pDest = NULL;
        uiDestLen = 0;
        if (!pSrc || (uiSrcLen == 0)) {
            deflateReset (&_zsCompStream);
            return 0;
        }

        bool bDone = false;
        _zsCompStream.next_in = const_cast<unsigned char*> (pSrc);
        _zsCompStream.avail_in = uiSrcLen;

        while (!bDone) {
            uiOldAvailableSpace = _zsCompStream.avail_out;
            if (0 != (rc = deflate (&_zsCompStream, Z_FINISH))) {
                if (rc == Z_STREAM_END) {
                    bDone = true;
                }
                else {
                    checkAndLogMsg ("ZLibConnectorWriter::flush", Logger::L_MildError,
                                    "deflate() with flag Z_FINISH returned with error code %d\n", rc);
                    uiDestLen = 0;
                    *pDest = NULL;
                    return -1;
                }
            }
            if (_zsCompStream.avail_out < _ulOutBufSize) {
                uiDestLen += (uiOldAvailableSpace - _zsCompStream.avail_out);
                if (_zsCompStream.avail_out == 0) {
                    _ulOutBufSize *= 2;
                    _pOutputBuffer = (unsigned char*) realloc (_pOutputBuffer, _ulOutBufSize);
                    if (!_pOutputBuffer) {
                        checkAndLogMsg ("ZLibConnectorWriter::flush", Logger::L_MildError,
                                        "error trying to realloc %u (previously %u) bytes\n",
                                        _ulOutBufSize, _ulOutBufSize/2);
                        uiDestLen = 0;
                        *pDest = NULL;
                        return -2;
                    }
                }
                _zsCompStream.avail_out = _ulOutBufSize - uiDestLen;
                _zsCompStream.next_out = _pOutputBuffer + uiDestLen;
            }
            else if (!bDone) {
                // deflate was not done but did not put anything into the output buffer
                checkAndLogMsg ("ZLibConnectorWriter::flush", Logger::L_MildError,
                                "deflate() with flag Z_FINISH didn't produce new output but returned code is not Z_STREAM_END (code: %d)\n", rc);
                uiDestLen = 0;
                *pDest = NULL;
                return -3;
            }
        }

        _bFlushed = true;
        if (uiDestLen > 0) {
            *pDest = _pOutputBuffer;
        }
        else {
            *pDest = NULL;
        }

        resetCompStream();
        deflateReset (&_zsCompStream);

        return 0;
    }

    void ZLibConnectorWriter::resetCompStream (void)
    {
        _zsCompStream.zalloc = alloc_mem;
        _zsCompStream.zfree = free_mem;
        _zsCompStream.opaque = (voidpf) 0;

        _zsCompStream.next_in = Z_NULL;
        _zsCompStream.avail_in = 0;
        _zsCompStream.total_in = 0;
        _zsCompStream.next_out = _pOutputBuffer;
        _zsCompStream.avail_out = _ulOutBufSize;
        _zsCompStream.total_out = 0;
    }

    void *ZLibConnectorWriter::alloc_mem (void *userdata, uInt items, uInt size)
    {
        return calloc (items, size);
    }

    void ZLibConnectorWriter::free_mem (void *userdata, void *data)
    {
        free (data);
    }

}
