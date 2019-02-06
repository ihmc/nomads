/*
 * CircularBuffer.h
 *
 * Provides a fixed-length circular buffer. If buffer is full and
 * additional elements are appended, the oldest elements are overwritten.
 *
 * NOTE: When elements are overwritten, they are not deleted. This could
 * cause memory leaks with elements allocated on the heap.
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

#ifndef INCL_CIRCULARBUFFER_H
#define INCL_CIRCULARBUFFER_H

#include <stddef.h>
#include <stdlib.h>

namespace NOMADSUtil
{

    template<class T> class CircularBuffer
    {
        public:
             CircularBuffer (void);
             ~CircularBuffer (void);
             struct Buffer {
                 T *pFirstBuf;
                 unsigned long ulFirstBufLen;
                 T *pSecondBuf;
                 unsigned long ulSecondBufLen;
             };
             int init (unsigned long ulQueueSize);
             void append (T newElement);
             const Buffer * getBuffer (void);
        protected:
             T *_pBuf;
             unsigned long _ulQueueSize;
             unsigned long _ulPos;
             bool _bWrappedAround;
             Buffer _buffer;
    };

    template <class T> CircularBuffer<T>::CircularBuffer (void)
    {
        _pBuf = NULL;
        _ulQueueSize = 0;
        _ulPos = 0;
        _bWrappedAround = false;
    }

    template<class T> CircularBuffer<T>::~CircularBuffer (void)
    {
        delete _pBuf;
        _pBuf = NULL;
    }

    template<class T> int CircularBuffer<T>::init (unsigned long ulQueueSize)
    {
        if (ulQueueSize == 0) {
            return -1;
        }
        if (NULL == (_pBuf = new T[ulQueueSize])) {
            return -2;
        }
        _ulQueueSize = ulQueueSize;
        return 0;
    }

    template<class T> void CircularBuffer<T>::append (T newElement)
    {
        if (_ulPos == _ulQueueSize) {
            _ulPos = 0;
            _bWrappedAround = true;
        }
        _pBuf[_ulPos++] = newElement;
    }

    template<class T> const CircularBuffer<T>::Buffer * CircularBuffer<T>::getBuffer (void)
    {
        if (_bWrappedAround) {
            _buffer.pFirstBuf = &_pBuf[_ulPos];
            _buffer.ulFirstBufLen = _ulQueueSize - _ulPos;
            if (_ulPos > 0) {
                _buffer.pSecondBuf = _pBuf;
                _buffer.ulSecondBufLen = _ulPos;
            }
            else {
                _buffer.pSecondBuf = NULL;
                _buffer.ulSecondBufLen = 0;
            }
        }
        else {
            _buffer.pFirstBuf = _pBuf;
            _buffer.pFirstBufLen = _ulPos;
            _buffer.ulSecondBuf = NULL;
            _buffer.ulSecondBufLen = 0;
        }
        return &_buffer;
    }

}

#endif  // #ifndef INCL_CIRCULAR_BUFFER_H
