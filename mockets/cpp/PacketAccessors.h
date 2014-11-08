#ifndef INCL_PACKET_ACCESSORS_H
#define INCL_PACKET_ACCESSORS_H

/*
 * PacketAccessors.h
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "PacketMutators.h"
#include "StateCookie.h"

#include "EndianHelper.h"
#include "FTypes.h"

#include <stdio.h>


class DeliveryPrerequisitesAccessor
{
    public:
        uint32 getControlTSN (void);
        uint32 getReliableSequencedTSN (void);
        uint32 getUnreliableSequencedTSN (void);
    private:
        friend class Packet;
        DeliveryPrerequisitesAccessor (uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN);
    private:
        uint32 _ui32ControlTSN;
        uint32 _ui32ReliableSequencedTSN;
        uint32 _ui32UnreliableSequencedTSN;
};

class InitChunkAccessor
{
    public:
        uint32 getValidation (void);
        uint32 getControlTSN (void);
        uint32 getReliableSequencedTSN (void);
        uint32 getUnreliableSequencedTSN (void);
        uint32 getReliableUnsequencedId (void);
        uint32 getUnreliableUnsequencedId (void);
    private:
        friend class Packet;
        InitChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class InitAckChunkAccessor
{
    public:
        uint32 getValidation (void);
        uint32 getControlTSN (void);
        uint32 getReliableSequencedTSN (void);
        uint32 getUnreliableSequencedTSN (void);
        uint32 getReliableUnsequencedId (void);
        uint32 getUnreliableUnsequencedId (void);
        StateCookie getStateCookie (void);
    private:
        friend class Packet;
        InitAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class CookieEchoChunkAccessor
{
    public:
        StateCookie getStateCookie (void);
        uint32 getKeyLength (void);
        const char * getKeyData (void);
    private:
        friend class Packet;
        CookieEchoChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class CookieAckChunkAccessor
{
    public:
        uint16 getPort (void);
        uint32 getEncryptedDataLength (void);
        const char * getEncryptedData (void);
    private:
        friend class Packet;
        CookieAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class ShutdownChunkAccessor
{
    public:
    private:
        friend class Packet;
        ShutdownChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class ShutdownAckChunkAccessor
{
    public:
    private:
        friend class Packet;
        ShutdownAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class ShutdownCompleteChunkAccessor
{
    public:
    private:
        friend class Packet;
        ShutdownCompleteChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class AbortChunkAccessor
{
    public:
    private:
        friend class Packet;
        AbortChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class TSNChunkAccessor
{
    public:
        bool haveMoreBlocks (void);
        uint8 getBlockType (void);
        bool advanceToNextBlock (void);

        bool haveMoreElements (void);
        virtual bool advanceToNextElement (void) = 0;

        uint32 getStartTSN (void);
        uint32 getEndTSN (void);

        uint32 getTSN (void);

    protected:
        TSNChunkAccessor (const char *pBuf, uint16 ui16OffsetToBlocks);

    protected:
        const char *_pBuf;
        uint16 _ui16OffsetToBlocks;
        uint16 _ui16ChunkSize;
        uint16 _ui16Offset;
        uint8 _ui8CurrBlockType;
        uint16 _ui16BytesLeftInCurrBlock;
};

class SAckChunkAccessor : public TSNChunkAccessor
{
    public:
        enum BlockType {
            BT_RANGE_CONTROL = SAckChunkMutator::SACK_CHUNK_BLOCK_FOR_CONTROL_FLOW | SAckChunkMutator::SACK_CHUNK_BLOCK_TYPE_RANGE,
            BT_SINGLE_CONTROL = SAckChunkMutator::SACK_CHUNK_BLOCK_FOR_CONTROL_FLOW | SAckChunkMutator::SACK_CHUNK_BLOCK_TYPE_SINGLE,
            BT_RANGE_RELIABLE_SEQUENCED = SAckChunkMutator::SACK_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW | SAckChunkMutator::SACK_CHUNK_BLOCK_TYPE_RANGE,
            BT_SINGLE_RELIABLE_SEQUENCED = SAckChunkMutator::SACK_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW | SAckChunkMutator::SACK_CHUNK_BLOCK_TYPE_SINGLE,
            BT_RANGE_RELIABLE_UNSEQUENCED = SAckChunkMutator::SACK_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW | SAckChunkMutator::SACK_CHUNK_BLOCK_TYPE_RANGE,
            BT_SINGLE_RELIABLE_UNSEQUENCED = SAckChunkMutator::SACK_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW | SAckChunkMutator::SACK_CHUNK_BLOCK_TYPE_SINGLE
        };
        uint32 getControlCumulateAck (void);
        uint32 getReliableSequencedCumulateAck (void);
        uint32 getReliableUnsequencedCumulativeAck (void);
        int64 getTimestamp (void);
        uint32 getBytesReceived (void);
        virtual bool advanceToNextElement (void);
    private:
        friend class Packet;
        SAckChunkAccessor (const char *pBuf, uint8 ui8BlocksOffset);
        bool bIsRecBandEst;
};

class CancelledChunkAccessor : public TSNChunkAccessor
{
    public:
        enum BlockType {
            BT_RANGE_RELIABLE_SEQUENCED = CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW | CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_TYPE_RANGE,
            BT_SINGLE_RELIABLE_SEQUENCED = CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW | CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_TYPE_SINGLE,
            BT_RANGE_RELIABLE_UNSEQUENCED = CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW | CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_TYPE_RANGE,
            BT_SINGLE_RELIABLE_UNSEQUENCED = CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW | CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_TYPE_SINGLE,
            BT_RANGE_UNRELIABLE_SEQUENCED = CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_FOR_UNRELIABLE_SEQUENCED_FLOW | CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_TYPE_RANGE,
            BT_SINGLE_UNRELIABLE_SEQUENCED = CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_FOR_UNRELIABLE_SEQUENCED_FLOW | CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_TYPE_SINGLE
        };
        virtual bool advanceToNextElement (void);
    private:
        friend class Packet;
        CancelledChunkAccessor (const char *pBuf);
};

class HeartbeatChunkAccessor
{
    public:
        int64 getTimestamp (void);
    private:
        friend class Packet;
        HeartbeatChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class TimestampChunkAccessor
{
    public:
        int64 getTimestamp (void);
    private:
        friend class Packet;
        TimestampChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class TimestampAckChunkAccessor
{
    public:
        int64 getTimestamp (void);
    private:
        friend class Packet;
        TimestampAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class DataChunkAccessor
{
    public:
        DataChunkAccessor (const DataChunkAccessor &src);
        uint16 getTagId (void);
        uint32 getDataLength (void);
        const char * getData (uint32 ui32Offset = 0);
    private:
        friend class Packet;
        DataChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class SimpleSuspendChunkAccessor
{
    public:
    private:
        friend class Packet;
        SimpleSuspendChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
    
};

class SimpleSuspendAckChunkAccessor
{
    public:
    private:
        friend class Packet;
        SimpleSuspendAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
    
};

class SuspendChunkAccessor
{
    public:
        uint32 getKeyLength (void);
        const char * getKeyData (uint32 ui32Offset = 0);
    private:
        friend class Packet;
        SuspendChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
    
};

class SuspendAckChunkAccessor
{
    public:
        uint32 getEncryptedDataLength (void);
        const char * getEncryptedData (uint32 ui32Offset = 0);
    private:
        friend class Packet;
        SuspendAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class ResumeChunkAccessor
{
    public:
        uint32 getEncryptedNonceLength (void);
        const char * getEncryptedNonce (uint32 ui32Offset = 0);
    private:
        friend class Packet;
        ResumeChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;    
};

class ResumeAckChunkAccessor
{
    public:
    private:
        friend class Packet;
        ResumeAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class ReEstablishChunkAccessor
{
    public:
        uint32 getEncryptedNonceLength (void);
        const char * getEncryptedNonce (uint32 ui32Offset = 0);
    private:
        friend class Packet;
        ReEstablishChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;    
};

class ReEstablishAckChunkAccessor
{
    public:
    private:
        friend class Packet;
        ReEstablishAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class SimpleConnectChunkAccessor
{
    public:
        uint32 getValidation (void);
        uint32 getControlTSN (void);
        uint32 getReliableSequencedTSN (void);
        uint32 getUnreliableSequencedTSN (void);
        uint32 getReliableUnsequencedId (void);
        uint32 getUnreliableUnsequencedId (void);
    private:
        friend class Packet;
        SimpleConnectChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

class SimpleConnectAckChunkAccessor
{
    public:
        uint32 getValidation (void);
        uint32 getControlTSN (void);
        uint32 getReliableSequencedTSN (void);
        uint32 getUnreliableSequencedTSN (void);
        uint32 getReliableUnsequencedId (void);
        uint32 getUnreliableUnsequencedId (void);
        uint16 getRemotePort (void);
        StateCookie getStateCookie (void);
    private:
        friend class Packet;
        SimpleConnectAckChunkAccessor (const char *pBuf);
    private:
        const char *_pBuf;
};

// Inline Methods for DeliveryPrerequisitesAccessor

inline DeliveryPrerequisitesAccessor::DeliveryPrerequisitesAccessor (uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN)
{
    _ui32ControlTSN = ui32ControlTSN;
    _ui32ReliableSequencedTSN = ui32ReliableSequencedTSN;
    _ui32UnreliableSequencedTSN = ui32UnreliableSequencedTSN;
}

inline uint32 DeliveryPrerequisitesAccessor::getControlTSN (void)
{
    return _ui32ControlTSN;
}

inline uint32 DeliveryPrerequisitesAccessor::getReliableSequencedTSN (void)
{
    return _ui32ReliableSequencedTSN;
}

inline uint32 DeliveryPrerequisitesAccessor::getUnreliableSequencedTSN (void)
{
    return _ui32UnreliableSequencedTSN;
}

// Inline Methods for InitChunkAccessor

inline InitChunkAccessor::InitChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 InitChunkAccessor::getValidation (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 4)));
}

inline uint32 InitChunkAccessor::getControlTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 8)));
}

inline uint32 InitChunkAccessor::getReliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 12)));
}

inline uint32 InitChunkAccessor::getUnreliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 16)));
}

inline uint32 InitChunkAccessor::getReliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 20)));
}

inline uint32 InitChunkAccessor::getUnreliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 24)));
}

// Inline Methods for InitAckChunkAccessor

inline InitAckChunkAccessor::InitAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 InitAckChunkAccessor::getValidation (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 4)));
}

inline uint32 InitAckChunkAccessor::getControlTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 8)));
}

inline uint32 InitAckChunkAccessor::getReliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 12)));
}

inline uint32 InitAckChunkAccessor::getUnreliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 16)));
}

inline uint32 InitAckChunkAccessor::getReliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 20)));
}

inline uint32 InitAckChunkAccessor::getUnreliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 24)));
}

inline StateCookie InitAckChunkAccessor::getStateCookie (void)
{
    return StateCookie (_pBuf+28);
}

// Inline Methods for CookieEchoChunkAccessor

inline CookieEchoChunkAccessor::CookieEchoChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline StateCookie CookieEchoChunkAccessor::getStateCookie (void)
{
    return StateCookie (_pBuf+4);
}

inline uint32 CookieEchoChunkAccessor::getKeyLength (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint32*)(_pBuf + 68 + 4))); // STATE_COOKIE_SIZE is 68 CHUNK_HEADER_SIZE is 4
}

inline const char * CookieEchoChunkAccessor::getKeyData (void)
{
    return (_pBuf + 4 + 68 + 4); // CHUNK_HEADER_SIZE is 4 STATE_COOKIE_SIZE is 68 keyLength is 4
}

// Inline Methods for CookieAckChunkAccessor

inline CookieAckChunkAccessor::CookieAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint16 CookieAckChunkAccessor::getPort (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf + 4)));
}

inline uint32 CookieAckChunkAccessor::getEncryptedDataLength (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint32*)(_pBuf + 6))); // CHUNK_HEADER_SIZE+port is 6
}

inline const char * CookieAckChunkAccessor::getEncryptedData (void)
{
    return (_pBuf + 6 + 4); // CHUNK_HEADER_SIZE+port is 6 dataLen is 4
}

// Inline Methods for ShutdownChunkAccessor

inline ShutdownChunkAccessor::ShutdownChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline Methods for ShutdownAckChunkAccessor

inline ShutdownAckChunkAccessor::ShutdownAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline Methods for ShutdownCompleteChunkAccessor

inline ShutdownCompleteChunkAccessor::ShutdownCompleteChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline Methods for AbortChunkAccessor

inline AbortChunkAccessor::AbortChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline Methods for TSNChunkAccessor

inline TSNChunkAccessor::TSNChunkAccessor (const char *pBuf, uint16 ui16OffsetToBlocks)
{
    _pBuf = pBuf;
    _ui16OffsetToBlocks = ui16OffsetToBlocks;
    _ui16ChunkSize = NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf + 2)));
    _ui16Offset = _ui16OffsetToBlocks;
    _ui8CurrBlockType = 0;
    _ui16BytesLeftInCurrBlock = 0;
    // Get information about the first block
    advanceToNextBlock();
}

inline bool TSNChunkAccessor::haveMoreBlocks (void)
{
    return (_ui16Offset < _ui16ChunkSize);
}

inline uint8 TSNChunkAccessor::getBlockType (void)
{
    return _ui8CurrBlockType;
}

inline bool TSNChunkAccessor::advanceToNextBlock (void)
{
    if (haveMoreBlocks()) {
        _ui8CurrBlockType = *((uint8*)(_pBuf + _ui16Offset));
        _ui16Offset++;
        _ui16BytesLeftInCurrBlock = NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf+_ui16Offset))) - 3;   // The length field includes the flags (1 byte) and the length (2 bytes)
        _ui16Offset += 2;
        return true;
    }
    else {
        _ui8CurrBlockType = 0;
        _ui16BytesLeftInCurrBlock = 0;
        return false;
    }
}

inline bool TSNChunkAccessor::haveMoreElements (void)
{
    return (_ui16BytesLeftInCurrBlock > 0);
}

inline uint32 TSNChunkAccessor::getStartTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf+_ui16Offset+0)));
}

inline uint32 TSNChunkAccessor::getEndTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf+_ui16Offset+4)));
}

inline uint32 TSNChunkAccessor::getTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf+_ui16Offset)));
}

// Inline Methods for SAckChunkAccessor

inline SAckChunkAccessor::SAckChunkAccessor (const char *pBuf, uint8 ui8BlocksOffset)
    : TSNChunkAccessor (pBuf, ui8BlocksOffset) // pBuf+ui8BlocksOffset points to the start of the blocks in this chunk (either 16 or 28 if SAckRecBandEst)
{
    if (ui8BlocksOffset == 16) {
        // This SAck is a regular one
        bIsRecBandEst = false;
        return;
    }
    if (ui8BlocksOffset == 28) {
        // This SAck contains additional info to calculate the bandwidth receiver side
        bIsRecBandEst = true;
        return;
    }
    // ERROR
    // TODO: log error!!
}

inline uint32 SAckChunkAccessor::getControlCumulateAck (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 4)));
}

inline uint32 SAckChunkAccessor::getReliableSequencedCumulateAck (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 8)));
}

inline uint32 SAckChunkAccessor::getReliableUnsequencedCumulativeAck (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 12)));
}

inline uint32 SAckChunkAccessor::getBytesReceived (void)
{
    if (bIsRecBandEst) {
        return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 16)));
    }
    // TODO log error
    return 0;
}

inline int64 SAckChunkAccessor::getTimestamp (void)
{
    if (bIsRecBandEst) {
        return NOMADSUtil::EndianHelper::ntoh64 (*((int64*)(_pBuf + 20)));
    }
    // TODO log error
    return 0;
}

inline bool SAckChunkAccessor::advanceToNextElement (void)
{
    if (getBlockType() & SAckChunkMutator::SACK_CHUNK_BLOCK_TYPE_RANGE) {
        _ui16Offset += 8;
        _ui16BytesLeftInCurrBlock -= 8;
    }
    else {
        _ui16Offset += 4;
        _ui16BytesLeftInCurrBlock -= 4;
    }
    return (_ui16BytesLeftInCurrBlock > 0);
}

// Inline Methods for CancelledChunkAccessor

inline CancelledChunkAccessor::CancelledChunkAccessor (const char *pBuf)
    : TSNChunkAccessor (pBuf, 4) // pBuf+4 points to the start of the blocks in this chunk
{
}

inline bool CancelledChunkAccessor::advanceToNextElement (void)
{
    if (getBlockType() & CancelledChunkMutator::CANCELLED_CHUNK_BLOCK_TYPE_RANGE) {
        _ui16Offset += 8;
        _ui16BytesLeftInCurrBlock -= 8;
    }
    else {
        _ui16Offset += 4;
        _ui16BytesLeftInCurrBlock -= 4;
    }
    return (_ui16BytesLeftInCurrBlock > 0);
}

// Inline Methods for HeartbeatChunkAccessor

inline HeartbeatChunkAccessor::HeartbeatChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline int64 HeartbeatChunkAccessor::getTimestamp (void)
{
    return NOMADSUtil::EndianHelper::ntoh64 (*((int64*)(_pBuf + 4)));
}

// Inline Methods for TimestampChunkAccessor

inline TimestampChunkAccessor::TimestampChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline int64 TimestampChunkAccessor::getTimestamp (void)
{
    return NOMADSUtil::EndianHelper::ntoh64 (*((int64*)(_pBuf + 4)));
}

// Inline Methods for TimestampAckChunkAccessor

inline TimestampAckChunkAccessor::TimestampAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline int64 TimestampAckChunkAccessor::getTimestamp (void)
{
    return NOMADSUtil::EndianHelper::ntoh64 (*((int64*)(_pBuf + 4)));
}

// Inline Methods for DataChunkAccessor

inline DataChunkAccessor::DataChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline DataChunkAccessor::DataChunkAccessor (const DataChunkAccessor &src)
{
    _pBuf = src._pBuf;
}

inline uint16 DataChunkAccessor::getTagId (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf + 4)));
}

inline uint32 DataChunkAccessor::getDataLength (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf + 2))) - 6; // DATA_CHUNK_HEADER_SIZE is 6
}

inline const char * DataChunkAccessor::getData (uint32 ui32Offset)
{
    return (_pBuf + 6 + ui32Offset);
}

// Inline methods for SimpleSuspendChunkAccessor

inline SimpleSuspendChunkAccessor::SimpleSuspendChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline methods for SimpleSuspendAckChunkAccessor

inline SimpleSuspendAckChunkAccessor::SimpleSuspendAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline methods for SuspendChunkAccessor

inline SuspendChunkAccessor::SuspendChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 SuspendChunkAccessor::getKeyLength (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint32*)(_pBuf + 2))) - 4; // CHUNK_HEADER_SIZE is 4
}

inline const char * SuspendChunkAccessor::getKeyData (uint32 ui32Offset)
{
    return (_pBuf + 4 + ui32Offset); // CHUNK_HEADER_SIZE is 4
}

// Inline Methods for SuspendAckChunkAccessor

inline SuspendAckChunkAccessor::SuspendAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 SuspendAckChunkAccessor::getEncryptedDataLength (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint32*)(_pBuf + 2))) - 4; // CHUNK_HEADER_SIZE is 4
}

inline const char * SuspendAckChunkAccessor::getEncryptedData (uint32 ui32Offset)
{
    return (_pBuf + 4 + ui32Offset); // CHUNK_HEADER_SIZE is 4
}

// Inline methods for ResumeChunkAccessor

inline ResumeChunkAccessor::ResumeChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 ResumeChunkAccessor::getEncryptedNonceLength (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint32*)(_pBuf + 2))) - 4; // CHUNK_HEADER_SIZE is 4
}

inline const char * ResumeChunkAccessor::getEncryptedNonce (uint32 ui32Offset)
{
    return (_pBuf + 4 + ui32Offset); // CHUNK_HEADER_SIZE is 4
}

// Inline Methods for ResumeAckChunkAccessor

inline ResumeAckChunkAccessor::ResumeAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline methods for ReEstablishChunkAccessor

inline ReEstablishChunkAccessor::ReEstablishChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 ReEstablishChunkAccessor::getEncryptedNonceLength (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint32*)(_pBuf + 2))) - 4; // CHUNK_HEADER_SIZE is 4
}

inline const char * ReEstablishChunkAccessor::getEncryptedNonce (uint32 ui32Offset)
{
    return (_pBuf + 4 + ui32Offset); // CHUNK_HEADER_SIZE is 4
}

// Inline Methods for ReEstablishAckChunkAccessor

inline ReEstablishAckChunkAccessor::ReEstablishAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

// Inline Methods for SimpleConnectChunkAccessor

inline SimpleConnectChunkAccessor::SimpleConnectChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 SimpleConnectChunkAccessor::getValidation (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 4)));
}

inline uint32 SimpleConnectChunkAccessor::getControlTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 8)));
}

inline uint32 SimpleConnectChunkAccessor::getReliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 12)));
}

inline uint32 SimpleConnectChunkAccessor::getUnreliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 16)));
}

inline uint32 SimpleConnectChunkAccessor::getReliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 20)));
}

inline uint32 SimpleConnectChunkAccessor::getUnreliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 24)));
}

// Inline Methods for SimpleConnectAckChunkAccessor

inline SimpleConnectAckChunkAccessor::SimpleConnectAckChunkAccessor (const char *pBuf)
{
    _pBuf = pBuf;
}

inline uint32 SimpleConnectAckChunkAccessor::getValidation (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 4)));
}

inline uint32 SimpleConnectAckChunkAccessor::getControlTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 8)));
}

inline uint32 SimpleConnectAckChunkAccessor::getReliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 12)));
}

inline uint32 SimpleConnectAckChunkAccessor::getUnreliableSequencedTSN (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 16)));
}

inline uint32 SimpleConnectAckChunkAccessor::getReliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 20)));
}

inline uint32 SimpleConnectAckChunkAccessor::getUnreliableUnsequencedId (void)
{
    return NOMADSUtil::EndianHelper::ntohl (*((uint32*)(_pBuf + 24)));
}

inline uint16 SimpleConnectAckChunkAccessor::getRemotePort (void)
{
    return NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf + 28)));
}

inline StateCookie SimpleConnectAckChunkAccessor::getStateCookie (void)
{
    return StateCookie (_pBuf+30);
}

#endif   // #ifndef INCL_PACKET_ACCESSORS_H
