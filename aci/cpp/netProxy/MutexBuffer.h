#ifndef INCL_MUTEX_BUFFER_H
#define INCL_MUTEX_BUFFER_H

/*
 * MutexBuffer.h
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
 *
 * The MutexBuffer class provides a resizable buffer with
 * mutually exclusive access, if required.
 * Buffers grows by doubling their size until either the
 * buffer size limit is reached or the buffer has grown
 * enough to satisfy the required size.
 */

#include <cstdlib>

#include "Mutex.h"

#define UI_DEFAULT_BUFFER_INITIAL_SIZE 1024U
#define UI_BUFFER_SIZE_LIMIT 262144U


namespace ACMNetProxy
{
    class MutexBuffer
    {
    public:
        MutexBuffer (bool bMutex, unsigned int uiMaxBufSize = UI_BUFFER_SIZE_LIMIT, unsigned int uiInitialBufSize = UI_DEFAULT_BUFFER_INITIAL_SIZE);
        ~MutexBuffer (void);

        unsigned int resetBuffer (void);
        unsigned int resizeBuffer (unsigned int uiNewMinSize, bool bKeepData);

        int lock (void);
        int tryLock (void);
        int unlock (void);

        unsigned char * getBuffer (void) const;
        unsigned int getBufferSize (void) const;
        const unsigned int getInitialBufferSize (void) const;
        const unsigned int getMaxBufferSize (void) const;
        const bool _isBufferMutex (void) const;
        const bool _isBufferResizable (void) const;

        operator unsigned char * const (void) const;


    private:
        unsigned char *_pBuf;
        unsigned int _uiCurrBufSize;
        const unsigned int _uiMinBufSize;
        const unsigned int _uiMaxBufSize;
        const bool _bResizable;

        NOMADSUtil::Mutex *_pMutex;
    };


    inline int MutexBuffer::lock (void)
    {
        return _pMutex ? _pMutex->lock() : NOMADSUtil::Mutex::RC_Ok;
    }

    inline int MutexBuffer::tryLock (void)
    {
        return _pMutex ? _pMutex->tryLock() : NOMADSUtil::Mutex::RC_Ok;
    }

    inline int MutexBuffer::unlock (void)
    {
        return _pMutex ? _pMutex->unlock() : NOMADSUtil::Mutex::RC_Ok;
    }

    inline unsigned char * MutexBuffer::getBuffer (void) const
    {
        return _pBuf;
    }

    inline unsigned int MutexBuffer::getBufferSize (void) const
    {
        return _uiCurrBufSize;
    }

    inline const unsigned int MutexBuffer::getInitialBufferSize (void) const
    {
        return _uiMinBufSize;
    }

    inline const unsigned int MutexBuffer::getMaxBufferSize (void) const
    {
        return _uiMaxBufSize;
    }

    inline MutexBuffer::operator unsigned char * const (void) const
    {
        return _pBuf;
    }
}

#endif
