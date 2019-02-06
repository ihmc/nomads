/*
 * FIFOBuffer.h
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
 *
 * Provides a fixed-size FIFO buffer class
 *
 * NOTE: This class does not perform any synchronization. If used in a
 * multi-threaded environment, caller must perform all mutual exclusion.
 *
 * NOTE: The Buffer returned by getBuffer() and dequeue() return a
 * pointer to a static buffer in the class that is reused for subsequent
 * calls.
 */

#ifndef INCL_FIFO_BUFFER_H
#define INCL_FIFO_BUFFER_H

#include "FTypes.h"

#include <stddef.h>

namespace NOMADSUtil
{
    template<class T> class FIFOBuffer
    {
        public:
            FIFOBuffer (uint32 ui32Size);
            virtual ~FIFOBuffer (void);

            struct Buffer {
                const T *pFirstBuf;
                uint32 ui32FirstBufLen;
                const T *pSecondBuf;
                uint32 ui32SecondBufLen;
            };

            // Enqueues elements into the FIFO queue, space permitting
            //     pBuf is a pointer to an array of elements of type T
            //     ui32BufSize is the number of elements to enqueue
            // Returns the number of elements that were successfully enqueued
            // (which could range from 0 to ui32BufSize, depending on space available)
            uint32 enqueue (const T *pBuf, uint32 ui32BufSize);

            // Returns the number of elements in the queue
            uint32 getQueueSize (void);

            // Returns the space available in the queue
            uint32 getFreeSpace (void);

            // Returns a Buffer structure pointing to the data in the queue, possibly offset from the beginning
            //     ui32OffsetFromStart - specifies the number of elements to skip over from the beginning of the queue
            // Returns a Buffer object where the total data returned is a concatenation of the two pointers,
            //     pFirstBuf with ui32FirstBufLen and pSecondBuf with ui32SecondBufLen
            const Buffer * peek (uint32 ui32OffsetFromStart = 0);

            // Dequeues the specified number of elements and returns a Buffer structure pointing to the dequeued data
            // Note that if the buffer has fewer elements, the number of elements dequeued could be less than requested
            // The total number of elements returned is determined by adding ui32FirstBufLen and ui32SecondBufLen from
            //     the returned Buffer
            const Buffer * dequeue (uint32 ui32NumElements);

            void clear (void);

        private:
             T *_pBuf;
             bool _bEmpty;
             uint32 _ui32QueueSize;
             uint32 _ui32HeadPos;
             uint32 _ui32TailPos;
             Buffer _buffer;

    };

    template <class T> FIFOBuffer<T>::FIFOBuffer (uint32 ui32Size)
    {
        _ui32QueueSize = ui32Size;
        _pBuf = NULL;
        clear();
        /*_pBuf = new T [ui32Size];
        _bEmpty = true;
        _ui32HeadPos = _ui32TailPos = 0;
        _buffer.pFirstBuf = NULL;
        _buffer.ui32FirstBufLen = 0;
        _buffer.pSecondBuf = NULL;
        _buffer.ui32SecondBufLen = 0;*/
    }

    template <class T> FIFOBuffer<T>::~FIFOBuffer (void)
    {
        delete[] _pBuf;
        _pBuf = NULL;
        _bEmpty = true;
        _ui32QueueSize = 0;
        _ui32HeadPos = _ui32TailPos = 0;
        _buffer.pFirstBuf = NULL;
        _buffer.ui32FirstBufLen = 0;
        _buffer.pSecondBuf = NULL;
        _buffer.ui32SecondBufLen = 0;
    }

    template <class T> uint32 FIFOBuffer<T>::enqueue (const T *pBuf, uint32 ui32BufSize)
    {
        uint32 ui32FreeSpace = getFreeSpace();
        if (ui32BufSize > ui32FreeSpace) {
            ui32BufSize = ui32FreeSpace;
        }
        if (ui32BufSize == 0) {
            // Buffer is already full
            return 0;
        }
        for (uint32 i = 0; i < ui32BufSize; i++) {
            _pBuf[_ui32HeadPos++] = pBuf[i];
            // Wrap around if _ui32QueueSize is reached
            _ui32HeadPos %= _ui32QueueSize;
        }
        _bEmpty = false;
        return ui32BufSize;
    }

    template <class T> uint32 FIFOBuffer<T>::getQueueSize (void)
    {
        if (_ui32HeadPos == _ui32TailPos) {
            if (_bEmpty) {
                return 0;
            }
            else {
                return _ui32QueueSize;
            }
        }
        else if (_ui32HeadPos >= _ui32TailPos) {
            return (_ui32HeadPos - _ui32TailPos);
        }
        else {
            return (_ui32QueueSize - (_ui32TailPos - _ui32HeadPos));
        }

    }

    template <class T> uint32 FIFOBuffer<T>::getFreeSpace (void)
    {
        return _ui32QueueSize - getQueueSize();
    }

    template <class T> const typename FIFOBuffer<T>::Buffer * FIFOBuffer<T>::peek (uint32 ui32OffsetFromStart)
    {
        _buffer.pFirstBuf = NULL;
        _buffer.ui32FirstBufLen = 0;
        _buffer.pSecondBuf = NULL;
        _buffer.ui32SecondBufLen = 0;
        uint32 ui32NumElements = getQueueSize();
        if ((ui32NumElements == 0) || (ui32OffsetFromStart >= ui32NumElements)) {
            // There are 0 elements to return
            return &_buffer;
        }
        uint32 ui32Start = _ui32TailPos + ui32OffsetFromStart;
        if (ui32Start >= _ui32QueueSize) {
            ui32Start -= _ui32QueueSize;          // Wrapped around
        }
        if (ui32Start >= _ui32HeadPos) {
            _buffer.pFirstBuf = _pBuf+ui32Start;
            _buffer.ui32FirstBufLen = _ui32QueueSize - ui32Start;
            _buffer.pSecondBuf = _pBuf;
            _buffer.ui32SecondBufLen = _ui32HeadPos;
        }
        else {
            _buffer.pFirstBuf = _pBuf + ui32Start;
            _buffer.ui32FirstBufLen = ui32NumElements - ui32OffsetFromStart;
        }
        return &_buffer;
    }

    template <class T> const typename FIFOBuffer<T>::Buffer * FIFOBuffer<T>::dequeue (uint32 ui32Count)
    {
        _buffer.pFirstBuf = NULL;
        _buffer.ui32FirstBufLen = 0;
        _buffer.pSecondBuf = NULL;
        _buffer.ui32SecondBufLen = 0;
        uint32 ui32NumElements = getQueueSize();
        if (ui32Count > ui32NumElements) {
            ui32Count = ui32NumElements;
        }
        if ((_ui32TailPos + ui32Count) > _ui32QueueSize) {
            // Buffer has wrapped around
            _buffer.pFirstBuf = _pBuf + _ui32TailPos;
            _buffer.ui32FirstBufLen = _ui32QueueSize - _ui32TailPos;
            _buffer.pSecondBuf = _pBuf;
            _buffer.ui32SecondBufLen = ui32Count - _buffer.ui32FirstBufLen;
        }
        else {
            // No wrap-around
            _buffer.pFirstBuf = _pBuf + _ui32TailPos;
            _buffer.ui32FirstBufLen = ui32Count;
        }
        _ui32TailPos += ui32Count;
        if (_ui32TailPos >= _ui32QueueSize) {
            _ui32TailPos -= _ui32QueueSize;
        }
        if (_ui32TailPos == _ui32HeadPos) {
            // Must be empty
            _bEmpty = true;
        }
        return &_buffer;
    }

    template <class T> void FIFOBuffer<T>::clear (void)
    {
        if (_pBuf) {
            delete[] _pBuf;
        }
        _pBuf = new T [_ui32QueueSize];
        _bEmpty = true;
        _ui32HeadPos = _ui32TailPos = 0;
        _buffer.pFirstBuf = NULL;
        _buffer.ui32FirstBufLen = 0;
        _buffer.pSecondBuf = NULL;
        _buffer.ui32SecondBufLen = 0;
    }

}

#endif   // #ifndef INCL_FIFO_BUFFER_H
