#ifndef TCP_SEGMENT_H
#define TCP_SEGMENT_H

/*
 * TCPSegment.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
 * File that defines the TCPSegment and the ReceivedData classes.
 * They are both devised to implement the concept of a TCP segment.
 * The TCPSegment class is different from the ReceivedData
 * class as it does not allocate new memory to store
 * the data contained in the TCP packet.
 */

#include "FTypes.h"
#include "SequentialArithmetic.h"

#include "OrderableBufferWriter.h"

#define TCP_DATA_FLAGS_MASK 0xF8U


namespace ACMNetProxy
{
    class TCPSegment : public OrderableBufferWrapper<unsigned char>
    {
    public:
        TCPSegment (uint32 ui32DataSeqNum, uint32 ui32DataLen, const uint8 * pui8Data, uint8 ui8Flags = 0);
        explicit TCPSegment (const TCPSegment & rTCPSegment) = delete;
        TCPSegment (TCPSegment && rTCPSegment);
        virtual ~TCPSegment (void) {};

        uint8 getTCPFlags (void) const;

        void setTCPFlags (uint8 ui8TCPFlags);
        void addTCPFlags (uint8 ui8TCPFlags);
        virtual int overwriteData (const unsigned char * pData, unsigned int uiSize);


    private:
        uint8 _ui8TCPFlags;
    };


    class ReceivedData : public TCPSegment
    {
    public:
        ReceivedData (uint32 ui32DataSeqNum, uint32 ui32DataLen, const uint8 *pui8Data, uint8 ui8Flags = 0);
        explicit ReceivedData (const ReceivedData & rReceivedData) = delete;
        ReceivedData (ReceivedData && rReceivedData);
        virtual ~ReceivedData (void);

        const int64 getLastTransmitTime (void) const;
        virtual const unsigned char * const getData (void) const;
        bool canAppendData (void) const;

        void setLastTransmitTime (int64 i64LastTransmitTime);
        virtual int incrementValues (unsigned int uiValue);

        // Reallocates buffer copies new data into reallocated buffer
        int appendDataToBuffer (const uint8 * const pui8Data, uint32 ui32DataLen);
        // Returns the byte in the buffer at the position specified by shift without dequeuing it
        int peekOctet (uint32 uiSequenceNumber) const;
        // Returns and dequeues the first uiLen bytes in the buffer
        int dequeueBytes (uint8 *pBuf, unsigned int uiLen);


    private:
        unsigned int _uiBeginningSequenceNumber;
        unsigned int _uiBufSize;
        int64 _i64LastTransmitTime;
    };


    inline TCPSegment::TCPSegment (uint32 ui32DataSeqNum, uint32 ui32DataLen, const uint8 *pui8Data, uint8 ui8Flags) :
        OrderableBufferWrapper<unsigned char>{ui32DataSeqNum, ui32DataLen, pui8Data}, _ui8TCPFlags{ui8Flags}
    { }

    inline TCPSegment::TCPSegment (TCPSegment && rTCPSegment) :
        OrderableBufferWrapper<unsigned char>{std::move (rTCPSegment)}, _ui8TCPFlags{rTCPSegment._ui8TCPFlags}
    {
        rTCPSegment._ui8TCPFlags = 0;
    }

    inline uint8 TCPSegment::getTCPFlags (void) const
    {
        return _ui8TCPFlags;
    }

    inline void TCPSegment::setTCPFlags (uint8 ui8TCPFlags)
    {
        _ui8TCPFlags = ui8TCPFlags;
    }

    inline void TCPSegment::addTCPFlags (uint8 ui8TCPFlags)
    {
        _ui8TCPFlags |= ui8TCPFlags;
    }

    inline int TCPSegment::overwriteData (const unsigned char *pData, unsigned int uiSize)
    {
        if (uiSize > _uiItemLength) {
            return -1;
        }
        memcpy (const_cast<unsigned char*> (getData()), pData, uiSize);

        return uiSize;
    }

    inline ReceivedData::ReceivedData (uint32 ui32DataSeqNum, uint32 ui32DataLen, const uint8 * pui8Data, uint8 ui8Flags) :
        TCPSegment{ui32DataSeqNum, (pui8Data ? ui32DataLen : 0), (pui8Data && (ui32DataLen > 0)) ? new uint8[ui32DataLen] : nullptr, ui8Flags},
        _uiBeginningSequenceNumber{ui32DataSeqNum}, _uiBufSize{ui32DataLen}, _i64LastTransmitTime{0}
    {
        if (pui8Data && (ui32DataLen > 0)) {
            memcpy (_pData, pui8Data, ui32DataLen);
        }
    }

    inline ReceivedData::ReceivedData (ReceivedData && rReceivedData) :
        TCPSegment{std::move (rReceivedData)}, _uiBeginningSequenceNumber{rReceivedData._uiBeginningSequenceNumber},
        _uiBufSize{rReceivedData._uiBufSize}, _i64LastTransmitTime{rReceivedData._i64LastTransmitTime}
    {
        rReceivedData._uiBeginningSequenceNumber = 0;
        rReceivedData._uiBufSize = 0;
        rReceivedData._i64LastTransmitTime = 0;
    }

    inline ReceivedData::~ReceivedData (void)
    {
        delete[] _pData;
    }

    inline const int64 ReceivedData::getLastTransmitTime (void) const
    {
        return _i64LastTransmitTime;
    }

    inline const unsigned char * const ReceivedData::getData (void) const
    {
        unsigned int uiOffset = NOMADSUtil::SequentialArithmetic::delta (_uiSequenceNumber, _uiBeginningSequenceNumber);
        if (uiOffset >= _uiBufSize) {
            return nullptr;
        }

        return &(_pData[uiOffset]);
    }

    inline bool ReceivedData::canAppendData (void) const
    {
        // Data can actually be appended only if packet had never been sent before
        return (_i64LastTransmitTime == 0) && (_uiSequenceNumber == _uiBeginningSequenceNumber);
    }

    inline void ReceivedData::setLastTransmitTime (int64 i64LastTransmitTime)
    {
        _i64LastTransmitTime = i64LastTransmitTime;
    }

    inline int ReceivedData::incrementValues (unsigned int uiValue)
    {
        if (uiValue > _uiItemLength) {
            uiValue = _uiItemLength;
        }

        _uiSequenceNumber += uiValue;
        _uiItemLength -= uiValue;

        return uiValue;
    }

}

#endif   // #ifndef TCP_SEGMENT_H
