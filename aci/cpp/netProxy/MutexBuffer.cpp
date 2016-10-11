/*
 * MutexBuffer.cpp
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
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include <cstdio>

#include "MutexBuffer.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    MutexBuffer::MutexBuffer (bool bMutex, unsigned int uiMaxBufSize, unsigned int uiInitialBufSize) :
        _uiMinBufSize ((uiInitialBufSize <= uiMaxBufSize) ? uiInitialBufSize : uiMaxBufSize),
        _uiMaxBufSize ((uiMaxBufSize <= UI_BUFFER_SIZE_LIMIT) ? uiMaxBufSize : UI_BUFFER_SIZE_LIMIT),
        _bResizable (uiInitialBufSize != uiMaxBufSize), _pMutex (bMutex ? new Mutex() : NULL)
    {
        _pBuf = new unsigned char[_uiMinBufSize];
        _uiCurrBufSize = _uiMinBufSize;
    }

    MutexBuffer::~MutexBuffer (void)
    {
        lock();
        if (_pBuf) {
            delete[] _pBuf;
            _pBuf = NULL;
        }
        _uiCurrBufSize = 0;

        if (_pMutex) {
            _pMutex->unlock();
            delete _pMutex;
            _pMutex = NULL;
        }
    }

    unsigned int MutexBuffer::resetBuffer (void)
    {
        lock();
        if (_bResizable && (_uiCurrBufSize > _uiMinBufSize)) {
            delete[] _pBuf;
            _pBuf = new unsigned char[_uiMinBufSize];
            _uiCurrBufSize = _uiMinBufSize;
        }
        unlock();

        return _uiCurrBufSize;
    }

    unsigned int MutexBuffer::resizeBuffer (unsigned int uiNewMinSize, bool bKeepData)
    {
        if (_bResizable) {
            lock();
            if (_uiCurrBufSize >= uiNewMinSize) {
                unlock();
                return _uiCurrBufSize;
            }
            if (_uiCurrBufSize == _uiMaxBufSize) {
                unlock();
                return _uiCurrBufSize;
            }
            if (uiNewMinSize > _uiMaxBufSize) {
                uiNewMinSize = _uiMaxBufSize;
            }

            while (_uiCurrBufSize < uiNewMinSize) {
                _uiCurrBufSize *= 2;
            }

            if (bKeepData) {
                _pBuf = (unsigned char *) realloc (_pBuf, _uiCurrBufSize);
            }
            else {
                delete[] _pBuf;
                _pBuf = new unsigned char[_uiCurrBufSize];
            }
            unlock();
        }

        return _uiCurrBufSize;
    }

}
