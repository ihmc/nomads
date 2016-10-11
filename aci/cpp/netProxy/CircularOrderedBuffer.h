#ifndef INCL_CIRCULAR_ORDERED_BUFFER_H
#define INCL_CIRCULAR_ORDERED_BUFFER_H

/*
 * CircularOrderedBuffer.h
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
 * The CircularOrderedBuffer class handles received TCP segments.
 * The class is designed in such a way that it accepts out-of-order
 * segments without the need to perform memory (de)allocations,
 * since a (growing) circular buffer is used to store the data.
 * The buffer size can grow up to a predefined threshold,
 * which is passed to the constructor as a parameter.
 * Data can be peeked or extracted only from the beginning of the buffer.
 * The CircularOrderedBuffer class also maintains an ordered
 * list of the contiguous chucks that have been received and
 * that have not yet been extracted from the buffer.
 */

#include "FTypes.h"
#include "PtrLList.h"

#include "OrderableBufferWriter.h"
#include "TCPSegment.h"

#define DEFAULT_MIN_SIZE 8192
#define DEFAULT_MAX_SIZE 1048576


namespace ACMNetProxy
{
    class CircularOrderedBuffer
    {
    public:
        CircularOrderedBuffer (unsigned int &uiStartingSequenceNumber, unsigned int uiMaxSize = DEFAULT_MAX_SIZE, unsigned int uiMinSize = DEFAULT_MIN_SIZE);
        ~CircularOrderedBuffer (void);

        bool isEmpty (void) const;
        bool isDataReady (void);
        bool isDataOutOfOrder (void) const;
        unsigned int getReadingSequenceNumber (void) const;
        unsigned int getAvailableBytesCount (void) const;
        unsigned int getTotalBytesCount (void) const;
        unsigned int getRemainingSpace (void) const;
        double getFreeSpacePercentage (void) const;
        const TCPSegment * const getLastSegment (void);

        void setBeginningSequenceNumber (unsigned int uiBeginningSequenceNumber);
        int insertData (TCPSegment *pTCPSegment, bool bOverwriteData = false);
        int peekData (TCPSegment *pTCPSegment);
        int peekData (unsigned int *uiSequenceNumber, unsigned char *pBuf, unsigned int uiBytesToRead, uint8 *ui8Flag);
        int extractData (TCPSegment *pTCPSegment);
        int extractData (unsigned int *uiSequenceNumber, unsigned char *pBuf, unsigned int uiBytesToRead, uint8 *ui8Flag);
        int removeData (unsigned int uiBytesToRemove);

        void resetBuffer (void);


    private:
        bool isDataUseful (const OrderableBufferWrapper<unsigned char> * const pOrderableItem, bool bOverwriteData);
        bool fitsInBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem) const;
        bool isDataWrappingAround (unsigned int uiStartingSequenceNumber, unsigned int uiBytesToRead) const;
        int growBufferIfNecessary (const OrderableBufferWrapper<unsigned char> * const pOrderableItem);
        unsigned char *getPositionInBuffer (unsigned int uiSequenceNumber) const;
        int availableBytesBeforeWrappingAround (unsigned int uiStartingSequenceNumber) const;
        int copyBytesToCircularBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem);
        int copyBytesToCircularBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem, unsigned int uiStartingSequenceNumber);
        int copyBytesToCircularBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem, unsigned int uiStartingSequenceNumber, unsigned int uiFinalSequenceNumber);
        int copyBytesFromCircularBuffer (unsigned char *pDest, unsigned int uiBytesToRead);
        int rawCopyFromCircularBuffer (unsigned char *pDest, unsigned int uiBytesToRead);

        unsigned char *_pBuf;
        unsigned int _uiDataReaderPointer;
        const unsigned int _uiBufMinSize;
        const unsigned int _uiBufMaxSize;
        unsigned int _uiBufCurrentSize;

        unsigned int _uiReadingSequenceNumber;
        unsigned int &_uiNextExpectedSequenceNumber;
        int _iReadyBytesInBuffer;
        int _iTotalBytesInBuffer;

        NOMADSUtil::PtrLList<TCPSegment> _SeparateNodesList;
    };


    inline bool CircularOrderedBuffer::isEmpty (void) const
    {
        return _iTotalBytesInBuffer == 0;
    }

    inline bool CircularOrderedBuffer::isDataReady (void)
    {
        return (getAvailableBytesCount() > 0) || (!_SeparateNodesList.isEmpty() &&
                (_SeparateNodesList.getFirst()->getSequenceNumber() == _uiReadingSequenceNumber));
    }

    inline bool CircularOrderedBuffer::isDataOutOfOrder (void) const
    {
        return _iReadyBytesInBuffer < _iTotalBytesInBuffer;
    }

    inline unsigned int CircularOrderedBuffer::getReadingSequenceNumber (void) const
    {
        return _uiReadingSequenceNumber;
    }

    inline unsigned int CircularOrderedBuffer::getAvailableBytesCount (void) const
    {
        return _iReadyBytesInBuffer;
    }

    inline unsigned int CircularOrderedBuffer::getTotalBytesCount (void) const
    {
        return (_iTotalBytesInBuffer > 0) ? _iTotalBytesInBuffer : 0;
    }

    inline unsigned int CircularOrderedBuffer::getRemainingSpace (void) const
    {
        return (_uiBufMaxSize - (unsigned int) _iTotalBytesInBuffer);
    }

    inline double CircularOrderedBuffer::getFreeSpacePercentage (void) const
    {
        return (((double) getRemainingSpace()) / _uiBufMaxSize) * 100.0L;
    }

    inline const TCPSegment * const CircularOrderedBuffer::getLastSegment (void)
    {
        return _SeparateNodesList.getTail();
    }

    inline void CircularOrderedBuffer::setBeginningSequenceNumber (unsigned int uiBeginningSequenceNumber)
    {
        _uiNextExpectedSequenceNumber = _uiReadingSequenceNumber = uiBeginningSequenceNumber;
    }

    inline int CircularOrderedBuffer::copyBytesToCircularBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem)
    {
        if (!pOrderableItem) {
            return -1;
        }
        return copyBytesToCircularBuffer (pOrderableItem, pOrderableItem->getSequenceNumber(), pOrderableItem->getFollowingSequenceNumber());
    }

    inline int CircularOrderedBuffer::copyBytesToCircularBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem, unsigned int uiStartingSequenceNumber)
    {
        if (!pOrderableItem) {
            return -1;
        }
        return copyBytesToCircularBuffer (pOrderableItem, uiStartingSequenceNumber, pOrderableItem->getFollowingSequenceNumber());
    }

}

#endif  // INCL_CIRCULAR_ORDERED_BUFFER_H
