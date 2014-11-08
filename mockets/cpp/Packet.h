#ifndef INCL_PACKET_H
#define INCL_PACKET_H

/*
 * Packet.h
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

#include "PacketAccessors.h"
#include "PacketMutators.h"

#include "FTypes.h"
#include "EndianHelper.h"

#include <stdio.h>


#ifndef MOCKETS_NO_CRYPTO
#include "security/CryptoUtils.h"
using namespace CryptoUtils;
#endif

class Mocket;
class StateCookie;

class Packet
{
    public:
        // Constructor used when creating a new packet
        Packet (Mocket *pMocket);

        // Constructor used when no Mocket exists yet
        Packet (unsigned short usBufSize);

        // Constructor used when parsing an existing packet
        // NOTE: Initially, the packet does not make a copy of the memory buffer
        //       Caller must invoke makePrivateCopyOfMemoryBuffer() if necessary
        Packet (char *pBuf, unsigned short usBufSize);
        
        // Constructor used when defrosting
        Packet (NOMADSUtil::ObjectDefroster &objectDefroster);

        virtual ~Packet (void);

        // Removes any piggyback chunks that were in the packet and then duplicates the memory buffer
        // so that the buffer passed in the constructor may be reused
        int prepareForProcessing (void);

        // Write the contents of the packet for debugging purposes
        void dump (FILE *file);

        static const uint16 CHUNK_HEADER_SIZE = 4;         // 2 bytes for chunk type, 2 bytes for chunk length
        static const uint16 DATA_CHUNK_HEADER_SIZE  = CHUNK_HEADER_SIZE + 2;   // 2 additional bytes for the tag value
        static const uint16 CHUNK_CLASS_METADATA    = 0x1000;
        static const uint16 CHUNK_CLASS_DATA        = 0x2000;
        static const uint16 CHUNK_CLASS_STATECHANGE = 0x4000;

        enum ChunkType {
            CT_None = 0,
            CT_SAck = CHUNK_CLASS_METADATA | 0x0001,
            CT_Heartbeat = CHUNK_CLASS_METADATA | 0x0002,
            CT_Cancelled = CHUNK_CLASS_METADATA | 0x0003,
            CT_Timestamp = CHUNK_CLASS_METADATA | 0x0004,
            CT_TimestampAck = CHUNK_CLASS_METADATA | 0x0005,
            CT_SAckRecBandEst = CHUNK_CLASS_METADATA | 0x0006,
            CT_Data = CHUNK_CLASS_DATA | 0x0001,
            CT_Init = CHUNK_CLASS_STATECHANGE | 0x0001,
            CT_InitAck = CHUNK_CLASS_STATECHANGE | 0x0002,
            CT_CookieEcho = CHUNK_CLASS_STATECHANGE | 0x0003,
            CT_CookieAck = CHUNK_CLASS_STATECHANGE | 0x0004,
            CT_Shutdown = CHUNK_CLASS_STATECHANGE | 0x0005,
            CT_ShutdownAck = CHUNK_CLASS_STATECHANGE | 0x0006,
            CT_ShutdownComplete = CHUNK_CLASS_STATECHANGE | 0x0007,
            CT_Abort = CHUNK_CLASS_STATECHANGE | 0x0008,
            CT_Suspend = CHUNK_CLASS_STATECHANGE | 0x0009,
            CT_SuspendAck = CHUNK_CLASS_STATECHANGE | 0x000A,
            CT_Resume = CHUNK_CLASS_STATECHANGE | 0x000B,
            CT_ResumeAck = CHUNK_CLASS_STATECHANGE | 0x000C,
            CT_ReEstablish = CHUNK_CLASS_STATECHANGE | 0x000D,
            CT_ReEstablishAck = CHUNK_CLASS_STATECHANGE | 0x000E,
            CT_SimpleSuspend = CHUNK_CLASS_STATECHANGE | 0x000F,
            CT_SimpleSuspendAck = CHUNK_CLASS_STATECHANGE | 0x0010,
            CT_SimpleConnect = CHUNK_CLASS_STATECHANGE | 0x0011,
            CT_SimpleConnectAck = CHUNK_CLASS_STATECHANGE | 0x0012
        };

        int setControlPacket (bool bControl);
        int setReliablePacket (bool bReliable);
        int setSequencedPacket (bool bSequenced);

        int setAsFirstFragment (void);
        int setAsIntermediateFragment (void);
        int setAsLastFragment (void);
        
        int setRetransmittedPacket (void);

        int setValidation (uint32 ui32Validation);
        int setSequenceNum (uint32 ui32SequenceNum);
        int setWindowSize (uint32 ui32WindowSize);

        // Methods related to setting delivery prerequisites
        // NOTE: allocateSpaceForDeliveryPrerequsites() must be called before any chunks are added to the packet
        int allocateSpaceForDeliveryPrerequisites (void);
        int setControlPacketDeliveryPrerequisite (uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN);
        int setReliableSequencedPacketDeliveryPrerequisite (uint32 ui32ControlTSN, uint32 ui32UnreliableSequencedTSN);
        int setUnreliableSequencedPacketDeliveryPrerequisite (uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN);

        int addInitChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId);
        int addInitAckChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId, StateCookie *pStateCookie);
        int addCookieEchoChunk (StateCookie *pStateCookie);
        int addPublicKey (const char *pKeyData, uint32 ui32KeyLen);
        int addCookieAckChunk (uint16 ui16Port);
        int addConnectionId (void *pEncryptedParam, uint32 ui32EncryptedParamLen);
        int addShutdownChunk (void);
        int addShutdownAckChunk (void);
        int addShutdownCompleteChunk (void);
        int addAbortChunk (void);
        SAckChunkMutator addSAckChunk (uint32 ui32ControlCumulativeAck, uint32 ui32ReliableSequencedCumulativeAck, uint32 ui32ReliableUnsequencedCumulativeAck);
        // This SAck is also used to pass the info to estimate the bandwidth receiver side
        SAckChunkMutator addSAckChunk (uint32 ui32ControlCumulativeAck, uint32 ui32ReliableSequencedCumulativeAck, uint32 ui32ReliableUnsequencedCumulativeAck, int64 i64Timestamp, uint32 ui32BytesReceived);
        int addHeartbeatChunk (int64 i64Timestamp);
        CancelledChunkMutator addCancelledChunk (void);
        int addTimestampChunk (int64 i64Timestamp);
        int addTimestampAckChunk (int64 i64Timestamp);
        int addDataChunk (uint16 ui16TagId, const char *pData, uint32 ui32DataLen);
        int addReEstablishChunk (void *pEnchriptedUUID, uint32 ui32EncryptedDataLen);
        int addReEstablishAckChunk (void);
        int addSimpleSuspendChunk (void);
        int addSimpleSuspendAckChunk (void);
        int addSuspendChunk (const char *pKeyData, uint32 ui32KeyLen);
        int addSuspendAckChunk (void *pEncryptedParam, uint32 ui32EncryptedParamLen);
        int addResumeChunk (void *pEnchriptedUUID, uint32 ui32EncryptedDataLen);
        int addResumeAckChunk (void);
        int addSimpleConnectChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId);
        int addSimpleConnectAckChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId, uint16 ui16LocalPort, StateCookie *pStateCookie);

        // Adds a data chunk to the packet
        // NOTE: There can be only one
        DataChunkMutator addDataChunk (uint16 ui16TagId);

        // Removes any piggyback chunks that were added to the packet
        int removePiggybackChunks (void);

        // Returns the size of the packet
        uint16 getPacketSize (void);

        // Returns the size of the packet without the piggyback chunks, which include SAckChunk and CancelledChunk
        // If there are no piggyback chunks, then the size of the whole packet is returned
        // This is used to estimate the size of a received packet that will be stored after the packet has been prepared for processing
        // See prepareForProcessing() above
        // NOTE: This method cannot be called until all the chunks have been iterated through once.
        //       The reason is that when a packet is received, the constructor does not examine the packet to find
        //       the piggyback chunks. The receiver iterates through the packet anyway, so the code takes advantage
        //       of that to initialize the _ui16PiggybackChunksOffset variable.
        //       This is done for efficiency, since the Receiver::run() method anyway iterates through all the chunks
        uint16 getPacketSizeWithoutPiggybackChunks (void);

        const char * getPacket (void);

        bool isControlPacket (void);
        bool isReliablePacket (void);
        bool isSequencedPacket (void);

        // Returns true if this packet contains a fragment of a message (first, intermediate, or last)
        bool isFragment (void);

        bool isFirstFragment (void);
        bool isIntermediateFragment (void);
        bool isLastFragment (void);
        
        bool isRetransmittedPacket (void);

        uint32 getValidation (void);
        uint32 getSequenceNum (void);
        uint32 getWindowSize (void);
        int64 getSentTime (void);

        // Returns the tag value of the data chunk (if any)
        // NOTE: This method cannot be called until all the chunks have been iterated through once or the data chunk has been accessed
        //       The reason is that the tag id is stored as part of the data chunk (if there is one present) and the constructor
        //       to create a Packet with a buffer that holds a received packet does not parse the whole packet to find
        //       out whether there is a data chunk and find the tag.
        //       This is done for efficiency, since the Receiver::run() method anyway iterates through all the chunks
        uint16 getTagId (void);

        bool areDeliveryPrerequisitesSet (void);
        DeliveryPrerequisitesAccessor getDeliveryPrerequisites (void);

        void resetChunkIterator (void);
        bool advanceToNextChunk (void);
        ChunkType getChunkType (void);
        InitChunkAccessor getInitChunk (void);
        InitAckChunkAccessor getInitAckChunk (void);
        CookieEchoChunkAccessor getCookieEchoChunk (void);
        CookieAckChunkAccessor getCookieAckChunk (void);
        ShutdownChunkAccessor getShutdownChunk (void);
        ShutdownAckChunkAccessor getShutdownAckChunk (void);
        ShutdownCompleteChunkAccessor getShutdownCompleteChunk (void);
        AbortChunkAccessor getAbortChunk (void);
        SAckChunkAccessor getSAckChunk (void);
        SAckChunkAccessor getSAckRecBandEstChunk (void);
        HeartbeatChunkAccessor getHeartbeatChunk (void);
        CancelledChunkAccessor getCancelledChunk (void);
        TimestampChunkAccessor getTimestampChunk (void);
        TimestampAckChunkAccessor getTimestampAckChunk (void);
        DataChunkAccessor getDataChunk (void);
        SimpleSuspendChunkAccessor getSimpleSuspendChunk (void);
        SimpleSuspendAckChunkAccessor getSimpleSuspendAckChunk (void);
        SuspendChunkAccessor getSuspendChunk (void);
        SuspendAckChunkAccessor getSuspendAckChunk (void);
        ResumeChunkAccessor getResumeChunk (void);
        ReEstablishChunkAccessor getReEstablishChunk (void);
        SimpleConnectChunkAccessor getSimpleConnectChunk (void);
        SimpleConnectAckChunkAccessor getSimpleConnectAckChunk (void);
        
        void printPacket (void);
        void freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        // Note that there is no defrost method! There is a constructor for packet that accepts an instance of ObjectDefroster
        
    public:
        static const uint16 HEADER_SIZE                        = 14;
        static const uint16 DELIVERY_PREREQUISITES_SIZE        = 8;
        static const uint16 HEADER_FLAG_RELIABLE               = 0x0001;
        static const uint16 HEADER_FLAG_SEQUENCED              = 0x0002;
        static const uint16 HEADER_FLAG_MSGPKT                 = 0x0004;
        static const uint16 HEADER_FLAG_CONTROL                = 0x0008;
        static const uint16 HEADER_FLAG_DELIVERY_PREREQUISITES = 0x0010;
        static const uint16 HEADER_FLAG_FIRST_FRAGMENT         = 0x0020;
        static const uint16 HEADER_FLAG_INTERMEDIATE_FRAGMENT  = 0x0040;
        static const uint16 HEADER_FLAG_LAST_FRAGMENT          = 0x0080;
        static const uint16 HEADER_FLAG_RETRANSMITTED          = 0x0100;        // Used for information only, can be repurposed if needed
        static const uint16 HEADER_PROTOCOL_VERSION            = 0x1000;    // (0x01 << 12)
        static const uint16 HEADER_FLAGS_MASK                  = HEADER_FLAG_RELIABLE | HEADER_FLAG_SEQUENCED |
                                                                 HEADER_FLAG_MSGPKT | HEADER_FLAG_CONTROL |
                                                                 HEADER_FLAG_DELIVERY_PREREQUISITES |
                                                                 HEADER_FLAG_FIRST_FRAGMENT |
                                                                 HEADER_FLAG_INTERMEDIATE_FRAGMENT |
                                                                 HEADER_FLAG_LAST_FRAGMENT |
                                                                 HEADER_FLAG_RETRANSMITTED;

    protected:
        int writeHeader (void);
        int parseHeader (void);

    private:
        char *_pBuf;
        bool _bDeleteBuf;
        unsigned short _usBufSize;
        unsigned short _usOffset;
        unsigned short _usFirstChunkOffset; // Used to keep track of the location of the first chunk (for resetting the read iterator)
        uint16 _ui16PiggybackChunksOffset;  // Used to keep track of the starting location of any piggyback chunks, which include the
                                            // SAckChunk and the CancelledChunk
                                            // Initialized either when a piggyback chunk is added during packet creation mode or when
                                            //     a piggyback chunk is accessed during packet read mode
        bool _bReadMode;
        bool _bDataChunkAdded;              // Used to keep track of the addition of a data chunk
                                            // Only one data chunk can be added to a packet because the entire packet is tagged with the same tag as the data chunk
        uint16 _ui16TagId;                  // The tag value that was specified when a data chunk was added to this packet

        uint16 _ui16Flags;
        uint32 _ui32Validation;
        uint32 _ui32SequenceNum;
        uint32 _ui32WindowSize;
        int64  _i64SentTime;

        enum DeliveryPrerequisiteFlags {
            DPF_ReliableSequenceNum = 0x01,
            DPF_UnreliableSequenceNum = 0x02,
            DPF_ControlSequenceNum = 0x04
        };
};

inline uint16 Packet::getPacketSize (void)
{
    return (_bReadMode ? _usBufSize : _usOffset);
}

inline const char * Packet::getPacket (void)
{
    return _pBuf;
}

inline int Packet::setControlPacket (bool bControl)
{
    if (bControl) {
        _ui16Flags |= HEADER_FLAG_CONTROL;
    }
    else {
        _ui16Flags &= ~HEADER_FLAG_CONTROL;
    }
    *((uint16*)(_pBuf + 0)) = NOMADSUtil::EndianHelper::htons (_ui16Flags);
    return 0;
}

inline bool Packet::isControlPacket (void)
{
    return (_ui16Flags & HEADER_FLAG_CONTROL) != 0;
}

inline int Packet::setReliablePacket (bool bReliable)
{
    if (bReliable) {
        _ui16Flags |= HEADER_FLAG_RELIABLE;
    }
    else {
        _ui16Flags &= ~HEADER_FLAG_RELIABLE;
    }
    *((uint16*)(_pBuf + 0)) = NOMADSUtil::EndianHelper::htons (_ui16Flags);
    return 0;
}

inline bool Packet::isReliablePacket (void)
{
    return (_ui16Flags & HEADER_FLAG_RELIABLE) != 0;
}

inline int Packet::setSequencedPacket (bool bSequenced)
{
    if (bSequenced) {
        _ui16Flags |= HEADER_FLAG_SEQUENCED;
    }
    else {
        _ui16Flags &= ~HEADER_FLAG_SEQUENCED;
    }
    *((uint16*)(_pBuf + 0)) = NOMADSUtil::EndianHelper::htons (_ui16Flags);
    return 0;
}

inline bool Packet::isSequencedPacket (void)
{
    return (_ui16Flags & HEADER_FLAG_SEQUENCED) != 0;
}

inline int Packet::setAsFirstFragment (void)
{
    _ui16Flags |= HEADER_FLAG_FIRST_FRAGMENT;
    *((uint16*)(_pBuf + 0)) = NOMADSUtil::EndianHelper::htons (_ui16Flags);
    return 0;
}

inline bool Packet::isFirstFragment (void)
{
    return (_ui16Flags & HEADER_FLAG_FIRST_FRAGMENT) != 0;
}

inline int Packet::setAsIntermediateFragment (void)
{
    _ui16Flags |= HEADER_FLAG_INTERMEDIATE_FRAGMENT;
    *((uint16*)(_pBuf + 0)) = NOMADSUtil::EndianHelper::htons (_ui16Flags);
    return 0;
}

inline bool Packet::isIntermediateFragment (void)
{
    return (_ui16Flags & HEADER_FLAG_INTERMEDIATE_FRAGMENT) != 0;
}

inline int Packet::setAsLastFragment (void)
{
    _ui16Flags |= HEADER_FLAG_LAST_FRAGMENT;
    *((uint16*)(_pBuf + 0)) = NOMADSUtil::EndianHelper::htons (_ui16Flags);
    return 0;
}

inline bool Packet::isRetransmittedPacket (void)
{
    return (_ui16Flags & HEADER_FLAG_RETRANSMITTED) != 0;
}

inline int Packet::setRetransmittedPacket (void)
{
    _ui16Flags |= HEADER_FLAG_RETRANSMITTED;
    *((uint16*)(_pBuf + 0)) = NOMADSUtil::EndianHelper::htons (_ui16Flags);
    return 0;
}

inline bool Packet::isLastFragment (void)
{
    return (_ui16Flags & HEADER_FLAG_LAST_FRAGMENT) != 0;
}

inline bool Packet::isFragment (void)
{
    return (_ui16Flags & (HEADER_FLAG_FIRST_FRAGMENT | HEADER_FLAG_INTERMEDIATE_FRAGMENT | HEADER_FLAG_LAST_FRAGMENT)) != 0;
}

inline int Packet::setValidation (uint32 ui32Validation)
{
    if ((ui32Validation == 0) || (_bReadMode)) {
        return -1;
    }
    _ui32Validation = ui32Validation;
    *((uint32*)(_pBuf + 6)) = NOMADSUtil::EndianHelper::htonl (_ui32Validation);
    return 0;
}

inline uint32 Packet::getValidation (void)
{
    return _ui32Validation;
}

inline int Packet::setSequenceNum (uint32 ui32SequenceNum)
{
    if (_bReadMode) {
        return -1;
    }
    _ui32SequenceNum = ui32SequenceNum;
    *((uint32*)(_pBuf + 10)) = NOMADSUtil::EndianHelper::htonl (_ui32SequenceNum);
    return 0;
}

inline uint32 Packet::getSequenceNum (void)
{
    return _ui32SequenceNum;
}

inline int Packet::setWindowSize (uint32 ui32WindowSize)
{
    if (_bReadMode) {
        return -1;
    }
    _ui32WindowSize = ui32WindowSize;
    *((uint32*)(_pBuf + 2)) = NOMADSUtil::EndianHelper::htonl (_ui32WindowSize);
    return 0;
}

inline uint32 Packet::getWindowSize (void)
{
    return _ui32WindowSize;
}

inline uint16 Packet::getTagId (void)
{
    return _ui16TagId;
}

#endif   // #ifndef INCL_PACKET_H
