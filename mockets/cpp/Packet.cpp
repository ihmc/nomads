/*
 * Packet.cpp
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

#include "Packet.h"

#include "Mocket.h"
#include "StateCookie.h"

#include <assert.h>
#include <memory.h>

#include "EndianHelper.h"


using namespace NOMADSUtil;

Packet::Packet (Mocket *pMocket)
{
    _usBufSize = pMocket->getMTU();
    _pBuf = (char*) malloc (_usBufSize);
    _bDeleteBuf = true;
    _usOffset = HEADER_SIZE;
    _usFirstChunkOffset = 0;
    _ui16PiggybackChunksOffset = 0;
    _bReadMode = false;
    _bDataChunkAdded = false;
    _ui16TagId = 0;
    _ui16Flags = HEADER_PROTOCOL_VERSION | HEADER_FLAG_MSGPKT;
    _ui32Validation = pMocket->getOutgoingValidation();
    _ui32SequenceNum = 0;
    _ui32WindowSize = 0;
    writeHeader();
}

Packet::Packet (unsigned short usBufSize)
{
    _usBufSize = usBufSize;
    _pBuf = (char*) malloc (_usBufSize);
    _bDeleteBuf = true;
    _usOffset = HEADER_SIZE;
    _usFirstChunkOffset = 0;
    _ui16PiggybackChunksOffset = 0;
    _bReadMode = false;
    _bDataChunkAdded = false;
    _ui16TagId = 0;
    _ui16Flags = HEADER_PROTOCOL_VERSION | HEADER_FLAG_MSGPKT;
    _ui32Validation = 0;
    _ui32SequenceNum = 0;
    _ui32WindowSize = 0;
    _i64SentTime = 0;
    writeHeader();
}

Packet::Packet (char *pBuf, unsigned short usBufSize)
{
    _pBuf = pBuf;
    _usBufSize = usBufSize;
    _bDeleteBuf = false;
    _usOffset = HEADER_SIZE;      // Will be set by the call to parseHeader() below
    _usFirstChunkOffset = 0;      // Will be set by the call to parseHeader() below
    _ui16PiggybackChunksOffset = 0;
    _bReadMode = true;
    _bDataChunkAdded = false;     // Probably immaterial - since _bReadMode is set to true
    _ui16TagId = 0;
    _ui16Flags = 0;               // Will be set by the call to parseHeader() below
    _ui32Validation = 0;          // Will be set by the call to parseHeader() below
    _ui32SequenceNum = 0;         // Will be set by the call to parseHeader() below
    _ui32WindowSize = 0;          // Will be set by the call to parseHeader() below
    _i64SentTime = 0;
    parseHeader();
}

Packet::Packet (ObjectDefroster &objectDefroster)
{
    objectDefroster >> _bReadMode;
    objectDefroster >> _usBufSize;
    _pBuf = (char*) malloc (_usBufSize);
    _bDeleteBuf = true;
    unsigned short usOffset = 0;
    objectDefroster >> usOffset;
    
    int size;
    if (_bReadMode) {
        size = _usBufSize;
    }
    else {
        size = usOffset;
    }    
    
    void * buf = malloc (size);
    objectDefroster.getBlob (buf);
    memcpy (_pBuf, buf, size);
    free (buf);
    buf = NULL;
    
    objectDefroster >> _bDataChunkAdded;
    objectDefroster >> _ui16TagId;
    objectDefroster >> _ui16PiggybackChunksOffset;
    
    _bDeleteBuf = false;
    
    // Values set by parseHeader()
    _ui16Flags = 0;
    _ui32WindowSize = 0;
    _ui32Validation = 0;
    _ui32SequenceNum = 0;
    _usOffset = HEADER_SIZE;
    _usFirstChunkOffset = 0;
    parseHeader();
    
    _i64SentTime = 0;
    // Reset the value after the change made by parseHeader()
    _usOffset = usOffset;
    
    //printPacket();
}

Packet::~Packet (void)
{
    if (_bDeleteBuf && _pBuf) {
        free (_pBuf);
    }
    _pBuf = NULL;
}

int Packet::prepareForProcessing (void)
{
    if ((!_bReadMode) || (_bDeleteBuf)) {
        // The packet is not in read mode or the buffer is already private
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        // There are one or more piggyback chunks - do not copy those
        _usBufSize = _ui16PiggybackChunksOffset;
    }
    char *pOrigBuf = _pBuf;
    _pBuf = (char*) malloc (_usBufSize);
    memcpy (_pBuf, pOrigBuf, _usBufSize);
    _bDeleteBuf = true;
    return 0;
}

void Packet::dump (FILE *file)
{
    unsigned short usPacketSize = 0;
    if (_bReadMode) {
        usPacketSize = _usBufSize;
    }
    else {
        usPacketSize = _usOffset;
    }
    fprintf (file, "**** Packet Sequence Number %lu - size %u\n", _ui32SequenceNum, (uint32) usPacketSize);
    fprintf (file, "     [");
    if (_ui16Flags & HEADER_FLAG_RELIABLE) {
        fprintf (file, "reliable ");
    }
    else {
        fprintf (file, "unreliable ");
    }
    if (_ui16Flags & HEADER_FLAG_SEQUENCED) {
        fprintf (file, "sequenced");
    }
    else {
        fprintf (file, "unsequenced");
    }
    fprintf (file, "]");
    unsigned short usChunkStart = HEADER_SIZE;
    if (_ui16Flags & HEADER_FLAG_DELIVERY_PREREQUISITES) {
        fprintf (file, " has delivery prerequisites");
        usChunkStart += DELIVERY_PREREQUISITES_SIZE;
    }
    else {
        fprintf (file, " no delivery prerequisites");
    }
    fprintf (file, "\n");
    while ((usChunkStart + CHUNK_HEADER_SIZE) <= usPacketSize) {
        uint16 ui16ChunkType = EndianHelper::ntohs (*((uint16*)(_pBuf + usChunkStart + 0)));
        uint16 ui16ChunkSize = EndianHelper::ntohs (*((uint16*)(_pBuf + usChunkStart + 2)));
        switch (ui16ChunkType) {
            case CT_None:
                 fprintf (file, "     Chunk Type: NONE\n");
                 break;
            case CT_SAck:
                 fprintf (file, "     Chunk Type: SAck; size = %d\n", (int) ui16ChunkSize);
                 break;
            case CT_Heartbeat:
                 fprintf (file, "     Chunk Type: Heartbeat; size = %d\n", (int) ui16ChunkSize);
                 break;
            case CT_Cancelled:
                 fprintf (file, "     Chunk Type: Cancelled; size = %d\n", (int) ui16ChunkSize);
                 break;
            case CT_Data:
            {
                 fprintf (file, "     Chunk Type: Data; size = %d\n", (int) ui16ChunkSize);
                 fprintf (file, "     Data: [");
                 DataChunkAccessor dca (_pBuf + usChunkStart);
                 const char *pData = dca.getData();
                 for (uint32 ui32 = 0; ui32 < dca.getDataLength(); ui32++) {
                     fprintf (file, "%c", pData[ui32]);
                 }
                 fprintf (file, "]\n");
                 break;
            }
            case CT_Init:
                 fprintf (file, "     Chunk Type: Init\n");
                 break;
            case CT_InitAck:
                 fprintf (file, "     Chunk Type: InitAck\n");
                 break;
            case CT_CookieEcho:
                 fprintf (file, "     Chunk Type: CookieEcho\n");
                 break;
            case CT_CookieAck:
                 fprintf (file, "     Chunk Type: CookieAck\n");
                 break;
            case CT_Shutdown:
                 fprintf (file, "     Chunk Type: Shutdown\n");
                 break;
            case CT_ShutdownAck:
                 fprintf (file, "     Chunk Type: ShutdownAck\n");
                 break;
            case CT_ShutdownComplete:
                 fprintf (file, "     Chunk Type: ShutdownComplete\n");
                 break;
            case CT_Abort:
                 fprintf (file, "     Chunk Type: Abort\n");
                 break;
            case CT_Timestamp:
                 fprintf (file, "     Chunk Type: Timestamp\n");
                 break;
            case CT_TimestampAck:
                 fprintf (file, "     Chunk Type: TimestampAck\n");
                 break;
            case CT_Suspend:
                 fprintf (file, "     Chunk Type: Suspend\n");
                 break;
            case CT_SuspendAck:
                 fprintf (file, "     Chunk Type: SuspendAck\n");
                 break;
            case CT_Resume:
                 fprintf (file, "     Chunk Type: Resume\n");
                 break;
            case CT_ResumeAck:
                 fprintf (file, "     Chunk Type: ResumeAck\n");
                 break;
        };
        usChunkStart += ui16ChunkSize;
    }
}

int Packet::allocateSpaceForDeliveryPrerequisites (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_usOffset > HEADER_SIZE) {
        // Space has already been allocated or a chunk has been added, so it is too late now
        return -2;
    }
    _usOffset += DELIVERY_PREREQUISITES_SIZE;
    _ui16Flags |= HEADER_FLAG_DELIVERY_PREREQUISITES;
    *((uint16*)(_pBuf + 0)) = EndianHelper::htons (_ui16Flags);
    return 0;
}

int Packet::setControlPacketDeliveryPrerequisite (uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN)
{
    if (_bReadMode) {
        // Cannot set in read mode
        return -1;
    }
    if ((_ui16Flags & HEADER_FLAG_DELIVERY_PREREQUISITES) == 0) {
        // No space was allocated for delivery prerequisites
        return -2;
    }
    if (!isControlPacket()) {
        // This method is only for control packets
        return -3;
    }
    *((uint32*) (_pBuf + HEADER_SIZE + 0)) = EndianHelper::htonl (ui32ReliableSequencedTSN);
    *((uint32*) (_pBuf + HEADER_SIZE + 4)) = EndianHelper::htonl (ui32UnreliableSequencedTSN);
    return 0;
}

int Packet::setReliableSequencedPacketDeliveryPrerequisite (uint32 ui32ControlTSN, uint32 ui32UnreliableSequencedTSN)
{
    if (_bReadMode) {
        // Cannot set in read mode
        return -1;
    }
    if ((_ui16Flags & HEADER_FLAG_DELIVERY_PREREQUISITES) == 0) {
        // No space was allocated for delivery prerequisites
        return -2;
    }
    if (isControlPacket()) {
        // This method is only for reliable sequenced packets
        return -3;
    }
    if ((!isReliablePacket()) || (!isSequencedPacket())) {
        // This method is only for reliable sequenced packets
        return -4;
    }
    *((uint32*) (_pBuf + HEADER_SIZE + 0)) = EndianHelper::htonl (ui32ControlTSN);
    *((uint32*) (_pBuf + HEADER_SIZE + 4)) = EndianHelper::htonl (ui32UnreliableSequencedTSN);
    return 0;
}

int Packet::setUnreliableSequencedPacketDeliveryPrerequisite (uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN)
{
    if (_bReadMode) {
        // Cannot set in read mode
        return -1;
    }
    if ((_ui16Flags & HEADER_FLAG_DELIVERY_PREREQUISITES) == 0) {
        // No space was allocated for delivery prerequisites
        return -2;
    }
    if (isControlPacket()) {
        // This method is only for unreliable sequenced packets
        return -3;
    }
    if ((isReliablePacket()) || (!isSequencedPacket())) {
        // This method is only for unreliable sequenced packets
        return -4;
    }
    *((uint32*) (_pBuf + HEADER_SIZE + 0)) = EndianHelper::htonl (ui32ControlTSN);
    *((uint32*) (_pBuf + HEADER_SIZE + 4)) = EndianHelper::htonl (ui32ReliableSequencedTSN);
    return 0;
}

int Packet::addInitChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_Init;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 24;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint32*)(_pBuf + _usOffset + 4)) = EndianHelper::htonl (ui32Validation);
    *((uint32*)(_pBuf + _usOffset + 8)) = EndianHelper::htonl (ui32ControlTSN);
    *((uint32*)(_pBuf + _usOffset + 12)) = EndianHelper::htonl (ui32ReliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 16)) = EndianHelper::htonl (ui32UnreliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 20)) = EndianHelper::htonl (ui32ReliableUnsequencedId);
    *((uint32*)(_pBuf + _usOffset + 24)) = EndianHelper::htonl (ui32UnreliableUnsequencedId);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addInitAckChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId, StateCookie *pStateCookie)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_InitAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 20 + StateCookie::STATE_COOKIE_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint32*)(_pBuf + _usOffset + 4)) = EndianHelper::htonl (ui32Validation);
    *((uint32*)(_pBuf + _usOffset + 8)) = EndianHelper::htonl (ui32ControlTSN);
    *((uint32*)(_pBuf + _usOffset + 12)) = EndianHelper::htonl (ui32ReliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 16)) = EndianHelper::htonl (ui32UnreliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 20)) = EndianHelper::htonl (ui32ReliableUnsequencedId);
    *((uint32*)(_pBuf + _usOffset + 24)) = EndianHelper::htonl (ui32UnreliableUnsequencedId);
    if (pStateCookie->write (_pBuf + _usOffset + 28)) {
        return -3;
    }
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addCookieEchoChunk (StateCookie *pStateCookie)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_CookieEcho;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + StateCookie::STATE_COOKIE_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    if (pStateCookie->write (_pBuf + _usOffset + 4)) {
        return -3;
    }
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addPublicKey (const char *pKeyData, uint32 ui32KeyLen)
{
    // Write the key length
    *((uint32*)(_pBuf + _usOffset)) = EndianHelper::htons (ui32KeyLen);
    _usOffset += 4; // 4 is the dimension of keyLen
    if (ui32KeyLen > 0) {
        // Write the public key
        memcpy (_pBuf + _usOffset, pKeyData, ui32KeyLen);
        _usOffset += ui32KeyLen;
    }
    return 0;
}

int Packet::addCookieAckChunk (uint16 ui16Port)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_CookieAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 2;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint16*)(_pBuf + _usOffset + 4)) = EndianHelper::htons (ui16Port);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addConnectionId (void *pEncryptedParam, uint32 ui32EncryptedParamLen)
{
    // Write the encrypted data length
    *((uint32*)(_pBuf + _usOffset)) = EndianHelper::htons (ui32EncryptedParamLen);
    _usOffset += 4; // 4 is the dimension of ui32EncryptedParamLen
    // Write [UUID, Password] encrypted with Ka
    memcpy (_pBuf + _usOffset, pEncryptedParam, ui32EncryptedParamLen);
    _usOffset += ui32EncryptedParamLen;
    return 0;
}

int Packet::addShutdownChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_Shutdown;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addShutdownAckChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_ShutdownAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addShutdownCompleteChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_ShutdownComplete;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addAbortChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_Abort;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}

SAckChunkMutator Packet::addSAckChunk (uint32 ui32ControlCumulativeAck, uint32 ui32ReliableSequencedCumulativeAck, uint32 ui32ReliableUnsequencedCumulativeAck)
{
    if (_bReadMode) {
        return SAckChunkMutator (NULL, 0, 0, NULL);
    }
    if ((_usBufSize - _usOffset) < CHUNK_HEADER_SIZE + 12) {
        return SAckChunkMutator (NULL, 0, 0, NULL);
    }
    if (_ui16PiggybackChunksOffset == 0) {
        // This is the first piggyback chunk being added - remember the position
        _ui16PiggybackChunksOffset = _usOffset;
    }
    uint16 ui16ChunkOffset = _usOffset;
    uint16 ui16ChunkType = CT_SAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 12;    // This will grow as SAck info is added with the mutator
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint32*)(_pBuf + _usOffset + 4)) = EndianHelper::htonl (ui32ControlCumulativeAck);
    *((uint32*)(_pBuf + _usOffset + 8)) = EndianHelper::htonl (ui32ReliableSequencedCumulativeAck);
    *((uint32*)(_pBuf + _usOffset + 12)) = EndianHelper::htonl (ui32ReliableUnsequencedCumulativeAck);
    _usOffset += ui16ChunkSize;
    uint8 ui8BlocksOffset = 16; // Bytes written after the header, here the blocks can start
    return SAckChunkMutator (_pBuf + ui16ChunkOffset, _usBufSize - _usOffset, ui8BlocksOffset, &_usOffset);
}

// This SAck is also used to pass the info to estimate the bandwidth receiver side
SAckChunkMutator Packet::addSAckChunk (uint32 ui32ControlCumulativeAck, uint32 ui32ReliableSequencedCumulativeAck, uint32 ui32ReliableUnsequencedCumulativeAck, int64 i64Timestamp, uint32 ui32BytesReceived)
{
    if (_bReadMode) {
        return SAckChunkMutator (NULL, 0, 0, NULL);
    }
    if ((_usBufSize - _usOffset) < CHUNK_HEADER_SIZE + 24) {
        return SAckChunkMutator (NULL, 0, 0, NULL);
    }
    if (_ui16PiggybackChunksOffset == 0) {
        // This is the first piggyback chunk being added - remember the position
        _ui16PiggybackChunksOffset = _usOffset;
    }
    uint16 ui16ChunkOffset = _usOffset;
    uint16 ui16ChunkType = CT_SAckRecBandEst;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 24;    // This will grow as SAck info is added with the mutator
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint32*)(_pBuf + _usOffset + 4)) = EndianHelper::htonl (ui32ControlCumulativeAck);
    *((uint32*)(_pBuf + _usOffset + 8)) = EndianHelper::htonl (ui32ReliableSequencedCumulativeAck);
    *((uint32*)(_pBuf + _usOffset + 12)) = EndianHelper::htonl (ui32ReliableUnsequencedCumulativeAck);

    *((uint32*)(_pBuf + _usOffset + 16)) = EndianHelper::htonl (ui32BytesReceived);
    *((int64*)(_pBuf + _usOffset + 20)) = EndianHelper::hton64 (i64Timestamp);
    _usOffset += ui16ChunkSize;
    uint8 ui8BlocksOffset = 28; // Bytes written after the header, here the blocks can start
    return SAckChunkMutator (_pBuf + ui16ChunkOffset, _usBufSize - _usOffset, ui8BlocksOffset, &_usOffset);
}

CancelledChunkMutator Packet::addCancelledChunk (void)
{
    if (_bReadMode) {
        return CancelledChunkMutator (NULL, 0, NULL);
    }
    if ((_usBufSize - _usOffset) < CHUNK_HEADER_SIZE) {
        return CancelledChunkMutator (NULL, 0, NULL);
    }
    if (_ui16PiggybackChunksOffset == 0) {
        // This is the first piggyback chunk being added - remember the position
        _ui16PiggybackChunksOffset = _usOffset;
    }
    uint16 ui16ChunkOffset = _usOffset;
    uint16 ui16ChunkType = CT_Cancelled;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;         // This will grow as blocks are added with the mutator
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return CancelledChunkMutator (_pBuf + ui16ChunkOffset, _usBufSize - _usOffset, &_usOffset);
}

int Packet::addHeartbeatChunk (int64 i64Timestamp)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_Heartbeat;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 8;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((int64*)(_pBuf + _usOffset + 4)) = EndianHelper::hton64 (i64Timestamp);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addTimestampChunk (int64 i64Timestamp)
{
    if (_bReadMode) {
        return -1;
    }
    if ((_usBufSize - _usOffset) < CHUNK_HEADER_SIZE + 8) {
        return -2;
    }
    if (_ui16PiggybackChunksOffset == 0) {
        // This is the first piggyback chunk being added - remember the position
        _ui16PiggybackChunksOffset = _usOffset;
    }
    uint16 ui16ChunkType = CT_Timestamp;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 8;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((int64*)(_pBuf + _usOffset + 4)) = EndianHelper::hton64 (i64Timestamp);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addTimestampAckChunk (int64 i64Timestamp)
{
    if (_bReadMode) {
        return -1;
    }
    if ((_usBufSize - _usOffset) < CHUNK_HEADER_SIZE + 8) {
        return -2;
    }
    if (_ui16PiggybackChunksOffset == 0) {
        // This is the first piggyback chunk being added - remember the position
        _ui16PiggybackChunksOffset = _usOffset;
    }
    uint16 ui16ChunkType = CT_TimestampAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 8;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((int64*)(_pBuf + _usOffset + 4)) = EndianHelper::hton64 (i64Timestamp);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addDataChunk (uint16 ui16TagId, const char *pData, uint32 ui32DataLen)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    if (_bDataChunkAdded) {
        // Only one data chunk can be added to each message packet
        return -3;
    }
    uint16 ui16ChunkType = CT_Data;
    uint16 ui16ChunkSize = DATA_CHUNK_HEADER_SIZE + ui32DataLen;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint16*)(_pBuf + _usOffset + 4)) = EndianHelper::htons (ui16TagId);
    memcpy (_pBuf + _usOffset + DATA_CHUNK_HEADER_SIZE, pData, ui32DataLen);
    _usOffset += ui16ChunkSize;
    _ui16TagId = ui16TagId;
    _bDataChunkAdded = true;
    return 0;
}

int Packet::addReEstablishChunk (void *pEncryptedUUID, uint32 ui32EncryptedDataLen)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_ReEstablish;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + ui32EncryptedDataLen;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    // Write nonce (UUID) encrypted with Ks
    memcpy (_pBuf + _usOffset + CHUNK_HEADER_SIZE, pEncryptedUUID, ui32EncryptedDataLen);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addReEstablishAckChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_ReEstablishAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addSimpleSuspendChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    
    uint16 ui16ChunkType = CT_SimpleSuspend;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addSimpleSuspendAckChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_SimpleSuspendAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addSuspendChunk (const char *pKeyData, uint32 ui32KeyLen)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    
    uint16 ui16ChunkType = CT_Suspend;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + ui32KeyLen;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    // Write the public key
    memcpy (_pBuf + _usOffset + CHUNK_HEADER_SIZE, pKeyData, ui32KeyLen);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addSuspendAckChunk (void *pEncryptedParam, uint32 ui32EncryptedParamLen)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_SuspendAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + ui32EncryptedParamLen;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    // Write [UUID, Password] encrypted with Ka
    memcpy (_pBuf + _usOffset + CHUNK_HEADER_SIZE, pEncryptedParam, ui32EncryptedParamLen);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addResumeChunk (void *pEncryptedUUID, uint32 ui32EncryptedDataLen)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_Resume;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + ui32EncryptedDataLen;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    // Write nonce (UUID) encrypted with Ks
    memcpy (_pBuf + _usOffset + CHUNK_HEADER_SIZE, pEncryptedUUID, ui32EncryptedDataLen);
    _usOffset += ui16ChunkSize;
    return 0;
}

int Packet::addResumeAckChunk (void)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_ResumeAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    _usOffset += ui16ChunkSize;
    return 0;
}


int Packet::addSimpleConnectChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_SimpleConnect;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 24;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint32*)(_pBuf + _usOffset + 4)) = EndianHelper::htonl (ui32Validation);
    *((uint32*)(_pBuf + _usOffset + 8)) = EndianHelper::htonl (ui32ControlTSN);
    *((uint32*)(_pBuf + _usOffset + 12)) = EndianHelper::htonl (ui32ReliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 16)) = EndianHelper::htonl (ui32UnreliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 20)) = EndianHelper::htonl (ui32ReliableUnsequencedId);
    *((uint32*)(_pBuf + _usOffset + 24)) = EndianHelper::htonl (ui32UnreliableUnsequencedId);
    _usOffset += ui16ChunkSize;
    return 0;
}
int Packet::addSimpleConnectAckChunk (uint32 ui32Validation, uint32 ui32ControlTSN, uint32 ui32ReliableSequencedTSN, uint32 ui32UnreliableSequencedTSN, uint32 ui32ReliableUnsequencedId, uint32 ui32UnreliableUnsequencedId, uint16 ui16LocalPort, StateCookie *pStateCookie)
{
    if (_bReadMode) {
        return -1;
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return -2;
    }
    uint16 ui16ChunkType = CT_SimpleConnectAck;
    uint16 ui16ChunkSize = CHUNK_HEADER_SIZE + 20 + StateCookie::STATE_COOKIE_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint32*)(_pBuf + _usOffset + 4)) = EndianHelper::htonl (ui32Validation);
    *((uint32*)(_pBuf + _usOffset + 8)) = EndianHelper::htonl (ui32ControlTSN);
    *((uint32*)(_pBuf + _usOffset + 12)) = EndianHelper::htonl (ui32ReliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 16)) = EndianHelper::htonl (ui32UnreliableSequencedTSN);
    *((uint32*)(_pBuf + _usOffset + 20)) = EndianHelper::htonl (ui32ReliableUnsequencedId);
    *((uint32*)(_pBuf + _usOffset + 24)) = EndianHelper::htonl (ui32UnreliableUnsequencedId);
    *((uint16*)(_pBuf + _usOffset + 28)) = EndianHelper::htons (ui16LocalPort);
    if (pStateCookie->write (_pBuf + _usOffset + 30)) {
        return -3;
    }
    _usOffset += ui16ChunkSize;
    return 0;
}

DataChunkMutator Packet::addDataChunk (uint16 ui16TagId)
{
    if (_bReadMode) {
        return DataChunkMutator (NULL, 0, NULL);
    }
    if (_ui16PiggybackChunksOffset != 0) {
        return DataChunkMutator (NULL, 0, NULL);
    }
    if (_bDataChunkAdded) {
        // Only one data chunk can be added to each message packet
        return DataChunkMutator (NULL, 0, NULL);
    }
    uint16 ui16ChunkType = CT_Data;
    uint16 ui16ChunkSize = DATA_CHUNK_HEADER_SIZE;
    *((uint16*)(_pBuf + _usOffset + 0)) = EndianHelper::htons (ui16ChunkType);
    *((uint16*)(_pBuf + _usOffset + 2)) = EndianHelper::htons (ui16ChunkSize);
    *((uint16*)(_pBuf + _usOffset + 4)) = EndianHelper::htons (ui16TagId);
    _usOffset += ui16ChunkSize;
    _ui16TagId = ui16TagId;
    _bDataChunkAdded = true;
    return DataChunkMutator (_pBuf + (_usOffset - DATA_CHUNK_HEADER_SIZE), _usBufSize - _usOffset, &_usOffset);
}

int Packet::removePiggybackChunks (void)
{
    if ((_bReadMode) || (_ui16PiggybackChunksOffset == 0)) {
        // No Piggyback chunks was added
        return -1;
    }
    _usOffset = _ui16PiggybackChunksOffset;
    _ui16PiggybackChunksOffset = 0;
    return 0;
}

bool Packet::areDeliveryPrerequisitesSet (void)
{
    return (_ui16Flags & HEADER_FLAG_DELIVERY_PREREQUISITES) != 0;
}

DeliveryPrerequisitesAccessor Packet::getDeliveryPrerequisites (void)
{
    if ((_ui16Flags & HEADER_FLAG_DELIVERY_PREREQUISITES) == 0) {
        return DeliveryPrerequisitesAccessor (0, 0, 0);
    }
    uint32 ui32TSNOne = EndianHelper::ntohl (*((uint32*)(_pBuf + HEADER_SIZE + 0)));
    uint32 ui32TSNTwo = EndianHelper::ntohl (*((uint32*)(_pBuf + HEADER_SIZE + 4)));
    if (isControlPacket()) {
        return DeliveryPrerequisitesAccessor (0, ui32TSNOne, ui32TSNTwo);
    }
    else if (isReliablePacket() && isSequencedPacket()) {
        return DeliveryPrerequisitesAccessor (ui32TSNOne, 0, ui32TSNTwo);
    }
    else if (isSequencedPacket()) {
        return DeliveryPrerequisitesAccessor (ui32TSNOne, ui32TSNTwo, 0);
    }
    else {
        return DeliveryPrerequisitesAccessor (0, 0, 0);
    }
}

uint16 Packet::getPacketSizeWithoutPiggybackChunks (void)
{
    if ((!_bReadMode) || (_ui16PiggybackChunksOffset == 0)) {
        // The packet is not in read mode or there are no piggyback chunks
        return getPacketSize();
    }
    else {
        return _ui16PiggybackChunksOffset;
    }
}

void Packet::resetChunkIterator (void)
{
    _usOffset = _usFirstChunkOffset;
}

bool Packet::advanceToNextChunk (void)
{
    if (!_bReadMode) {
        return false;
    }
    if ((_usOffset + CHUNK_HEADER_SIZE) > _usBufSize) {
        return false;
    }
    uint16 ui16ChunkSize = EndianHelper::ntohs (*((uint16*)(_pBuf + _usOffset + 2)));
    _usOffset += ui16ChunkSize;
    if ((_usOffset + CHUNK_HEADER_SIZE) > _usBufSize) {
        return false;
    }
    return true;
}

Packet::ChunkType Packet::getChunkType (void)
{
    if (!_bReadMode) {
        return CT_None;
    }
    if ((_usOffset + CHUNK_HEADER_SIZE) > _usBufSize) {
        return CT_None;
    }
    uint16 ui16ChunkType = EndianHelper::ntohs (*((uint16*)(_pBuf + _usOffset + 0)));
    if (ui16ChunkType == CT_Data) {
        // Initialize the _ui16TagId variable from the data chunk
        _ui16TagId = EndianHelper::ntohs (*((uint16*)(_pBuf + _usOffset + 4)));
    }
    else if ((ui16ChunkType == CT_SAck) || (ui16ChunkType == CT_Cancelled) || (ui16ChunkType == CT_SAckRecBandEst)) {
        if ((_bReadMode) && (_ui16PiggybackChunksOffset == 0)) {
            // Keep track of this position as the start of the piggyback chunks
            _ui16PiggybackChunksOffset = _usOffset;
        }
    }

    #ifdef ERROR_CHECKING
        if ((ui16ChunkType != CT_SAck) && (ui16ChunkType != CT_Heartbeat) && (ui16ChunkType != CT_Cancelled) &&
            (ui16ChunkType != CT_Data) && (ui16ChunkType != CT_Init) && (ui16ChunkType != CT_InitAck) &&
            (ui16ChunkType != CT_CookieEcho) && (ui16ChunkType != CT_CookieAck) && (ui16ChunkType != CT_Shutdown) &&
            (ui16ChunkType != CT_ShutdownAck) && (ui16ChunkType != CT_ShutdownComplete) && (ui16ChunkType != CT_Abort) &&
            (ui16ChunkType != CT_Timestamp) && (ui16ChunkType != CT_TimestampAck) && (ui16ChunkType != CT_Suspend) &&
            (ui16ChunkType != CT_SuspendAck) && (ui16ChunkType != CT_Resume) && (ui16ChunkType != CT_ResumeAck) && 
            (ui16ChunkType != CT_ReEstablish) && (ui16ChunkType != CT_ReEstablishAck) && (ui16ChunkType != CT_SimpleSuspend) &&
            (ui16ChunkType != CT_SimpleSuspendAck) && (ui16ChunkType != CT_SimpleConnect) && (ui16ChunkType != CT_SimpleConnectAck) &&
            (ui16ChunkType != CT_SAckRecBandEst)) {
            return CT_None;
        }
    #endif

    return (ChunkType) ui16ChunkType;
}

InitChunkAccessor Packet::getInitChunk (void)
{
    assert (getChunkType() == CT_Init);
    return InitChunkAccessor (_pBuf + _usOffset);
}

InitAckChunkAccessor Packet::getInitAckChunk (void)
{
    assert (getChunkType() == CT_InitAck);
    return InitAckChunkAccessor (_pBuf + _usOffset);
}

CookieEchoChunkAccessor Packet::getCookieEchoChunk (void)
{
    assert (getChunkType() == CT_CookieEcho);
    return CookieEchoChunkAccessor (_pBuf + _usOffset);
}

CookieAckChunkAccessor Packet::getCookieAckChunk (void)
{
    assert (getChunkType() == CT_CookieAck);
    return CookieAckChunkAccessor (_pBuf + _usOffset);
}

ShutdownChunkAccessor Packet::getShutdownChunk (void)
{
    assert (getChunkType() == CT_Shutdown);
    return ShutdownChunkAccessor (_pBuf + _usOffset);
}

ShutdownAckChunkAccessor Packet::getShutdownAckChunk (void)
{
    assert (getChunkType() == CT_ShutdownAck);
    return ShutdownAckChunkAccessor (_pBuf + _usOffset);
}

ShutdownCompleteChunkAccessor Packet::getShutdownCompleteChunk (void)
{
    assert (getChunkType() == CT_ShutdownComplete);
    return ShutdownCompleteChunkAccessor (_pBuf + _usOffset);
}

AbortChunkAccessor Packet::getAbortChunk (void)
{
    assert (getChunkType() == CT_Abort);
    return AbortChunkAccessor (_pBuf + _usOffset);
}

SAckChunkAccessor Packet::getSAckChunk (void)
{
    assert (getChunkType() == CT_SAck);
    uint8 ui8SAckOffset = 16;
    return SAckChunkAccessor (_pBuf + _usOffset, ui8SAckOffset);
}

SAckChunkAccessor Packet::getSAckRecBandEstChunk (void)
{
    assert (getChunkType() == CT_SAckRecBandEst);
    uint8 ui8SAckOffset = 28;
    return SAckChunkAccessor (_pBuf + _usOffset, ui8SAckOffset);
}

CancelledChunkAccessor Packet::getCancelledChunk (void)
{
    assert (getChunkType() == CT_Cancelled);
    return CancelledChunkAccessor (_pBuf + _usOffset);
}

HeartbeatChunkAccessor Packet::getHeartbeatChunk (void)
{
    assert (getChunkType() == CT_Heartbeat);
    return HeartbeatChunkAccessor (_pBuf + _usOffset);
}

TimestampChunkAccessor Packet::getTimestampChunk (void)
{
    assert (getChunkType() == CT_Timestamp);
    return TimestampChunkAccessor (_pBuf + _usOffset);
}

TimestampAckChunkAccessor Packet::getTimestampAckChunk (void)
{
    assert (getChunkType() == CT_TimestampAck);
    return TimestampAckChunkAccessor (_pBuf + _usOffset);
}

DataChunkAccessor Packet::getDataChunk (void)
{
    assert (getChunkType() == CT_Data);
    return DataChunkAccessor (_pBuf + _usOffset);
}

SimpleSuspendChunkAccessor Packet::getSimpleSuspendChunk (void)
{
    assert (getChunkType() == CT_SimpleSuspend);
    return SimpleSuspendChunkAccessor (_pBuf + _usOffset);
}


SimpleSuspendAckChunkAccessor Packet::getSimpleSuspendAckChunk (void)
{
    assert (getChunkType() == CT_SimpleSuspendAck);
    return SimpleSuspendAckChunkAccessor (_pBuf + _usOffset);
}

SuspendChunkAccessor Packet::getSuspendChunk (void)
{
    assert (getChunkType() == CT_Suspend);
    return SuspendChunkAccessor (_pBuf + _usOffset);
}

SuspendAckChunkAccessor Packet::getSuspendAckChunk (void)
{
    assert (getChunkType() == CT_SuspendAck);
    return SuspendAckChunkAccessor (_pBuf + _usOffset);
}

ResumeChunkAccessor Packet::getResumeChunk (void)
{
    assert (getChunkType() == CT_Resume);
    return ResumeChunkAccessor (_pBuf + _usOffset);
}

ReEstablishChunkAccessor Packet::getReEstablishChunk (void)
{
    assert (getChunkType() == CT_ReEstablish);
    return ReEstablishChunkAccessor (_pBuf + _usOffset);
}

SimpleConnectChunkAccessor Packet::getSimpleConnectChunk (void)
{
    assert (getChunkType() == CT_SimpleConnect);
    return SimpleConnectChunkAccessor (_pBuf + _usOffset);
}

SimpleConnectAckChunkAccessor Packet::getSimpleConnectAckChunk (void)
{
    assert (getChunkType() == CT_SimpleConnectAck);
    return SimpleConnectAckChunkAccessor (_pBuf + _usOffset);
}

void Packet::freeze (ObjectFreezer &objectFreezer)
{
    objectFreezer.putBool (_bReadMode);
    objectFreezer.putUInt16 (_usBufSize);
    objectFreezer.putUInt16 (_usOffset);
    // No need to freeze _usFirstChunkOffset it is equal to _usOffset when we strt to process a packet
    
    int size;
    if (_bReadMode) {
        size = _usBufSize;
    }
    else {
        size = _usOffset;
    }
    
    objectFreezer.putBlob (_pBuf, size);
    objectFreezer.putBool (_bDataChunkAdded);
    objectFreezer.putUInt16 (_ui16TagId);
    objectFreezer.putUInt16 (_ui16PiggybackChunksOffset);
    // No need to freeze _ui16Flags; _ui32Validation; _ui32SequenceNum; _ui32WindowSize; they will be set by parseHeader during defrost
   
    //printPacket();
}

int Packet::writeHeader (void)
{
    if (_bReadMode) {
        return -1;
    }
    *((uint16*)(_pBuf + 0)) = EndianHelper::htons (_ui16Flags);
    *((uint32*)(_pBuf + 2)) = EndianHelper::htonl (_ui32WindowSize);
    *((uint32*)(_pBuf + 6)) = EndianHelper::htonl (_ui32Validation);
    *((uint32*)(_pBuf + 10)) = EndianHelper::htonl (_ui32SequenceNum);
    return 0;
}

int Packet::parseHeader (void)
{
    _ui16Flags = EndianHelper::ntohs (*((uint16*)(_pBuf + 0)));
    _ui32WindowSize = EndianHelper::ntohl (*((uint32*)(_pBuf + 2)));
    _ui32Validation = EndianHelper::ntohl (*((uint32*)(_pBuf + 6)));
    _ui32SequenceNum = EndianHelper::ntohl (*((uint32*)(_pBuf + 10)));
    _usOffset = HEADER_SIZE;
    if (_ui16Flags & HEADER_FLAG_DELIVERY_PREREQUISITES) {
        _usOffset += DELIVERY_PREREQUISITES_SIZE;
    }
    _usFirstChunkOffset = _usOffset;
    return 0;
}

void Packet::printPacket (void) {
    printf ("PACKET:\n    _usBufSize=%d\n    _usOffset=%d\n    _usFirstChunkOffset=%d\n"
            "    _ui16PiggybackChunksOffset=%d\n    _ui16TagId=%d\n    _ui16Flags=%d\n"
            "    _ui32Validation=%lu\n    _ui32SequenceNum=%lu\n    _ui32WindowSize=%lu\n"
            , _usBufSize, _usOffset, _usFirstChunkOffset, _ui16PiggybackChunksOffset,
            _ui16TagId, _ui16Flags, _ui32Validation, _ui32SequenceNum, _ui32WindowSize);
    if (_bReadMode == true) {
        printf ("    _bReadMode = true\n");
    }
    else {
        printf ("    _bReadMode = false\n");
    }
/*    // Comment out this code to print the buffer
    printf ("    _pBuff = [");
    int size;
    if (_bReadMode) {
        size = _usBufSize;
    }
    else {
        size = _usOffset;
    }
    for (int i = 0; i < size; i++) {
        printf ("%c", (char) _pBuf[i]);
    }
    printf ("]\n");
 */
}

