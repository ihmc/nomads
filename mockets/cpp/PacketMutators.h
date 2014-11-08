#ifndef INCL_PACKET_MUTATORS_H
#define INCL_PACKET_MUTATORS_H

/*
 * PacketMutators.h
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

#include <memory.h>
#include <stddef.h>
#include <string.h>

#include "EndianHelper.h"
#include "FTypes.h"


class DataChunkMutator
{
    public:
        DataChunkMutator (void);
        uint16 getSpaceAvail (void);
        int addDataFragment (const void *pBuf, uint16 ui16BufSize);
    private:
        friend class Packet;
        DataChunkMutator (char *pBuf, uint16 ui16SpaceAvail, uint16 *pui16Offset);
    private:
        char *_pBuf;                        // Pointer to the beginning of the chunk header in the packet
                                            // Therefore, _pBuf + 6 points to the start of the data
        uint16 _ui16Offset;                 // Offset from _pBuf where data needs to be copied
        uint16 _ui16SpaceAvail;             // Number of bytes remaining in buffer to add data
        uint16 *_pui16OffsetInPacket;       // Pointer to the variable in Packet that contains the current offset into the packet
                                            // Incremented as data is added
};

class TSNChunkMutator
{
    public:
        virtual bool initializedWithoutErrors (void);

        virtual int startAddingRanges (void) = 0;
        virtual int addRange (uint32 ui32Start, uint32 ui32End);
        virtual int doneAddingRanges (void);

        virtual int startAddingTSNs (void) = 0;
        virtual int addTSN (uint32 ui32TSN);
        virtual int doneAddingTSNs (void);

    protected:
        // If there is an error in adding the SAckChunk or the CancelledPacketChunk, the Packet
        // class initializes the mutator with a NULL value for pBuf and a 0 for ui16BufSize
        TSNChunkMutator (char *pBuf, uint16 ui16BufSize, uint16 ui16OffsetToBlocks, uint16 *pui16Offset);

        int startNewBlock (uint8 ui8Flags);
        int finishBlock (void);

    private:
        char *_pBuf;                        // pBuf points to the beginning of the chunk header in the packet
                                            // Therefore, pBuf + 2 points to the chunk size, pBuf + ui16OffsetToBlocks points to the first block,

        uint16 _ui16BufSize;                // Number of bytes remaining in the buffer to add blocks

        uint16 _ui16OffsetToBlocks;         // Offset from the start of the chunk to the location of the information blocks

        uint16 _ui16ChunkOffset;            // Current offset into the chunk (incremented as blocks are added)

        uint16 *_pui16OffsetInPacket;       // Pointer to the variable in Packet that contains the current offset into the packet
                                            // Incremented as blocks are added

        uint16 _ui16ChunkSize;              // Local variable used to keep track of the chunk size
                                            // The original value in the chunk is updated by using this value

        uint16 _ui16OffsetToCurrBlock;      // Current offset from the start of the chunk to the starting point of a the current
                                            // block of ranges or individual values being added
                                            // Therefore, pBuf + ui16OffsetToCurrBlock points to the flags for the block and
                                            // pBuf + ui16OffsetToCurrBlock + 1 points to the size of the current block
};

class SAckChunkMutator : public TSNChunkMutator
{
    public:
        void selectControlFlow (void);
        void selectReliableSequencedFlow (void);
        void selectReliableUnsequencedFlow (void);

        virtual int startAddingRanges (void);
        virtual int startAddingTSNs (void);

    private:
        friend class Packet;
        friend class SAckChunkAccessor;
        SAckChunkMutator (char *pBuf, uint16 ui16BufSize, uint8 ui8BlocksOffset, uint16 *pui16Offset);

    private:
        static const uint8 SACK_CHUNK_BLOCK_FOR_CONTROL_FLOW = 0x01;
        static const uint8 SACK_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW = 0x02;
        static const uint8 SACK_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW = 0x04;
        static const uint8 SACK_CHUNK_BLOCK_TYPE_RANGE = 0x10;
        static const uint8 SACK_CHUNK_BLOCK_TYPE_SINGLE = 0x20;

    private:
        uint8 _ui8SelectedFlow;
};

class CancelledChunkMutator : public TSNChunkMutator
{
    public:
        void selectReliableSequencedFlow (void);
        void selectReliableUnsequencedFlow (void);
        void selectUnreliableSequencedFlow (void);

        virtual int startAddingRanges (void);
        virtual int startAddingTSNs (void);

    private:
        friend class Packet;
        friend class CancelledChunkAccessor;
        CancelledChunkMutator (char *pBuf, uint16 ui16BufSize, uint16 *pui16Offset);

    private:
        static const uint8 CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW = 0x01;
        static const uint8 CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW = 0x02;
        static const uint8 CANCELLED_CHUNK_BLOCK_FOR_UNRELIABLE_SEQUENCED_FLOW = 0x04;
        static const uint8 CANCELLED_CHUNK_BLOCK_TYPE_RANGE = 0x10;
        static const uint8 CANCELLED_CHUNK_BLOCK_TYPE_SINGLE = 0x20;

    private:
        uint8 _ui8SelectedFlow;
};

// Inline functions for DataChunkMutator

inline DataChunkMutator::DataChunkMutator (void)
{
    _pBuf = NULL;
    _ui16Offset = 0;
    _ui16SpaceAvail = 0;
    _pui16OffsetInPacket = NULL;
}

inline DataChunkMutator::DataChunkMutator (char *pBuf, uint16 ui16SpaceAvail, uint16 *pui16Offset)
{
    _pBuf = pBuf;
    _ui16Offset = 6;
    _ui16SpaceAvail = ui16SpaceAvail;
    _pui16OffsetInPacket = pui16Offset;
}

inline uint16 DataChunkMutator::getSpaceAvail (void)
{
    return _ui16SpaceAvail;
}

inline int DataChunkMutator::addDataFragment (const void *pBuf, uint16 ui16BufSize)
{
    if (_pBuf == NULL) {
        return -1;
    }
    else if (ui16BufSize > _ui16SpaceAvail) {
        return -2;
    }
    memcpy (_pBuf+_ui16Offset, pBuf, ui16BufSize);
    _ui16Offset += ui16BufSize;
    _ui16SpaceAvail -= ui16BufSize;
    *_pui16OffsetInPacket += ui16BufSize;        // Increment offset variable in Packet
    *((uint16*)(_pBuf + 2)) = NOMADSUtil::EndianHelper::htons (NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf + 2))) + ui16BufSize);    // Update chunk size in DataChunk in Packet
    return 0;
}

// Inline functions for TSNChunkMutator

inline TSNChunkMutator::TSNChunkMutator (char *pBuf, uint16 ui16BufSize, uint16 ui16OffsetToBlocks, uint16 *pui16Offset)
{
    _pBuf = pBuf;
    _ui16BufSize = ui16BufSize;
    _ui16OffsetToBlocks = ui16OffsetToBlocks;
    _ui16ChunkOffset = _ui16OffsetToBlocks;
    _pui16OffsetInPacket = pui16Offset;
    if (pBuf != NULL) {
        _ui16ChunkSize = NOMADSUtil::EndianHelper::ntohs (*((uint16*)(_pBuf + 2)));
    }
    else {
        _ui16ChunkSize = 0;
    }
}

inline bool TSNChunkMutator::initializedWithoutErrors (void)
{
    return (_pBuf != NULL);
}

inline int TSNChunkMutator::addRange (uint32 ui32Start, uint32 ui32End)
{
    if (_ui16BufSize < 8) {
        // Insufficient room
        return -1;
    }

    if (_ui16OffsetToCurrBlock == 0) {
        // startNewBlock() has not been called yet
        return -2;
    }

    *((uint32*)(_pBuf + _ui16ChunkOffset)) = NOMADSUtil::EndianHelper::htonl (ui32Start);
    _ui16ChunkOffset += 4;
    *((uint32*)(_pBuf + _ui16ChunkOffset)) = NOMADSUtil::EndianHelper::htonl (ui32End);
    _ui16ChunkOffset += 4;

    _ui16BufSize -= 8;
    _ui16ChunkSize += 8;
    return 0;
}

inline int TSNChunkMutator::doneAddingRanges (void)
{
    return finishBlock();
}

inline int TSNChunkMutator::addTSN (uint32 ui32TSN)
{
    if (_ui16BufSize < 4) {
        // Insufficient room
        return -1;
    }

    if (_ui16OffsetToCurrBlock == 0) {
        // startNewBlock() has not been called yet
        return -2;
    }

    *((uint32*)(_pBuf + _ui16ChunkOffset)) = NOMADSUtil::EndianHelper::htonl (ui32TSN);
    _ui16ChunkOffset += 4;

    _ui16BufSize -= 4;
    _ui16ChunkSize += 4;

    return 0;
}

inline int TSNChunkMutator::doneAddingTSNs (void)
{
    return finishBlock();
}

inline int TSNChunkMutator::startNewBlock (uint8 ui8Flags)
{
    if (_ui16BufSize < 3) {
        // Insufficient room
        return -1;
    }

    _ui16OffsetToCurrBlock = _ui16ChunkOffset;
    *((uint8*)(_pBuf + _ui16ChunkOffset)) = ui8Flags;
    _ui16ChunkOffset++;
    *((uint16*)(_pBuf + _ui16ChunkOffset)) = 0;        // Initialize the size of the current block to 0
    _ui16ChunkOffset += 2;

    _ui16BufSize -= 3;
    _ui16ChunkSize += 3;
    return 0;
}

inline int TSNChunkMutator::finishBlock (void)
{
    uint16 ui16BlockSize = _ui16ChunkOffset - _ui16OffsetToCurrBlock;
    *((uint16*)(_pBuf + _ui16OffsetToCurrBlock + 1)) = NOMADSUtil::EndianHelper::htons (ui16BlockSize);
    *((uint16*)(_pBuf + 2)) = NOMADSUtil::EndianHelper::htons (_ui16ChunkSize);  // Store new chunk size in Packet
    _ui16OffsetToCurrBlock = 0;
    *_pui16OffsetInPacket += (ui16BlockSize);         // Increment offset variable in Packet
    return 0;
}

// Inline functions for SAckChunkMutator

/*
 * Assumptions about parameters
 *     pBuf points to beginning of chunk header; therefore pBuf+2 points to chunk size and
 *         pBuf+16 points to the count of the number of SAck blocks and pBuf+18 points to
           the first SAck block that will be added
 *     ui16BufSize contains the number of bytes remaining in the packet to add SAck blocks
 *     puil16Offset points to the variable in Packet that contains the current offset
 *         into the packet. The value of the variable in Packet has already been
 *         updated to include the SAck Chunk Header and the cumulative acknowledgements
 */
inline SAckChunkMutator::SAckChunkMutator (char *pBuf, uint16 ui16BufSize, uint8 ui8BlocksOffset, uint16 *pui16Offset)
    : TSNChunkMutator (pBuf, ui16BufSize, ui8BlocksOffset, pui16Offset)
{
    _ui8SelectedFlow = 0;
}

inline void SAckChunkMutator::selectControlFlow (void)
{
    _ui8SelectedFlow = SACK_CHUNK_BLOCK_FOR_CONTROL_FLOW;
}

inline void SAckChunkMutator::selectReliableSequencedFlow (void)
{
    _ui8SelectedFlow = SACK_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW;
}

inline void SAckChunkMutator::selectReliableUnsequencedFlow (void)
{
    _ui8SelectedFlow = SACK_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW;
}

inline int SAckChunkMutator::startAddingRanges (void)
{
    return startNewBlock (_ui8SelectedFlow | SACK_CHUNK_BLOCK_TYPE_RANGE);
}

inline int SAckChunkMutator::startAddingTSNs (void)
{
    return startNewBlock (_ui8SelectedFlow | SACK_CHUNK_BLOCK_TYPE_SINGLE);
}

// Inline functions for CancelledChunkMutator

/*
 * Assumptions about parameters
 *     pBuf points to beginning of chunk header; therefore pBuf+2 points to chunk size and
 *         pBuf+4 points to the count of the number of cancelled blocks and pBuf+6 points to
           the first cancelled block that will be added
 *     ui16BufSize contains the number of bytes remaining in the packet to add cancelled blocks
 *     puil16Offset points to the variable in Packet that contains the current offset
 *         into the packet. The value of the variable in Packet has already been
 *         updated to include the Cancelled Chunk Header and the cumulative acknowledgements
 */

inline CancelledChunkMutator::CancelledChunkMutator (char *pBuf, uint16 ui16BufSize, uint16 *pui16Offset)
: TSNChunkMutator (pBuf, ui16BufSize, 4, pui16Offset)
{
    _ui8SelectedFlow = 0;
}

inline void CancelledChunkMutator::selectReliableSequencedFlow (void)
{
    _ui8SelectedFlow = CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_SEQUENCED_FLOW;
}

inline void CancelledChunkMutator::selectReliableUnsequencedFlow (void)
{
    _ui8SelectedFlow = CANCELLED_CHUNK_BLOCK_FOR_RELIABLE_UNSEQUENCED_FLOW;
}

inline void CancelledChunkMutator::selectUnreliableSequencedFlow (void)
{
    _ui8SelectedFlow = CANCELLED_CHUNK_BLOCK_FOR_UNRELIABLE_SEQUENCED_FLOW;
}

inline int CancelledChunkMutator::startAddingRanges (void)
{
    return startNewBlock (_ui8SelectedFlow | CANCELLED_CHUNK_BLOCK_TYPE_RANGE);
}

inline int CancelledChunkMutator::startAddingTSNs (void)
{
    return startNewBlock (_ui8SelectedFlow | CANCELLED_CHUNK_BLOCK_TYPE_SINGLE);
}

#endif   // #ifndef INCL_PACKET_MUTATORS_H
