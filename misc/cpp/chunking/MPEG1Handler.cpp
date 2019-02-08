/*
 * MPEG1Handler.cpp
 *
 * This file is part of the IHMC Misc Library
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#include "MPEG1Handler.h"

#include "MPEG1Parser.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "CircularPtrLList.h"
#include "DArray.h"
#include "Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

namespace IHMC_MISC
{
    bool chunksHaveMoreFrames (DArray<bool> &haveMoreFrames)
    {
        for (unsigned int i = 1; i < haveMoreFrames.size(); i++) {
            if (haveMoreFrames[i]) {
                return true;
            }
        }
        return false;
    }
}

using namespace IHMC_MISC;

static const int QUALITY = 2;  // ranges from 0 to 2.  ) being the lowest quality, 2 being the highest

//-----------------------------------------------------------------------------
// MPEG1Chunker
//-----------------------------------------------------------------------------

Reader * MPEG1Chunker::fragmentMpeg1 (MPEG1Parser *pParser, uint8 ui8DesiredChunkId, uint8 ui8TotalNoOfChunks)
{
    const char *pszMethodName = "MPEG1Chunker::fragmentMpeg1";
    if ((pParser == NULL) || (ui8DesiredChunkId < 1)) {
        return NULL;
    }
    pParser->reset();
    unsigned int uiFrameCounter = 0;
    unsigned int uiGOPCounter = 0;
    int iOffset;
    BufferWriter fw (10240, 10240);
    static const unsigned int BUF_LEN = 1024 * 1024;
    void *pBuf = malloc (BUF_LEN);
    if (pBuf == NULL) {
        return NULL;
    }
    while ((iOffset = pParser->readToNextSequence (pBuf, BUF_LEN)) >= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "New sequence\n");
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes\n", iOffset);
        if (fw.writeBytes (pBuf, iOffset) < 0) {
            free (pBuf);
            return NULL;
        }
        if ((iOffset = pParser->readToNextSequenceGOP (pBuf, BUF_LEN)) >= 0) {
            if (fw.writeBytes (pBuf, iOffset) < 0) {
                free (pBuf);
                return NULL;
            }
        }
        while ((iOffset = pParser->readToNextSequenceGOP (pBuf, BUF_LEN)) >= 0) {
            checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "New group of pictures\n");
            checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes\n", iOffset);
            if (uiGOPCounter % ui8TotalNoOfChunks == (ui8DesiredChunkId -1)) {
                if (fw.writeBytes (pBuf, iOffset) < 0) {
                    free (pBuf);
                    return NULL;
                }
            }
            while ((iOffset = pParser->readToNextGOPFrame (pBuf, BUF_LEN)) >= 0) {
                checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes for frame %u.\n",
                                iOffset, uiFrameCounter);
                if (uiGOPCounter % ui8TotalNoOfChunks == (ui8DesiredChunkId -1)) {
                    if (fw.writeBytes (pBuf, iOffset) < 0) {
                        free (pBuf);
                        return NULL;
                    }
                }
                ++uiFrameCounter;
            }
            ++uiGOPCounter;
        }
    }

    if ((iOffset = pParser->readToEOF (pBuf, BUF_LEN)) >= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes to EOF\n", iOffset);
        if (fw.writeBytes (pBuf, iOffset) < 0) {
            free (pBuf);
            return NULL;
        }
    }

    free (pBuf);
    unsigned long uiBufLen = fw.getBufferLength();
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "created chunk %d of size %u.\n", ui8DesiredChunkId, uiBufLen);
    return new BufferReader (fw.relinquishBuffer(), uiBufLen, true);
}

Reader * MPEG1Chunker::extractFromMpeg1 (MPEG1Parser *pReader, int64 i64StartT, int64 i64EndT)
{
    const char *pszMethodName = "";
    if ((pReader == NULL) || (i64StartT < 0) || (i64EndT <= 0) || (i64StartT >= i64EndT)) {
        return NULL;
    }
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "\n");
    return NULL;
}

//-----------------------------------------------------------------------------
//FFmpegReassembler
//-----------------------------------------------------------------------------

MPEG1Reassembler::MPEG1Reassembler (void)
    : _ui8TotalNoOfChunks (0), _chunks (false), _annotations (false)
{
}

MPEG1Reassembler::~MPEG1Reassembler (void)
{
}

int MPEG1Reassembler::init (uint8 ui8TotalNoOfChunks)
{
    _ui8TotalNoOfChunks = ui8TotalNoOfChunks;
    return 0;
}

int MPEG1Reassembler::incorporateChunk (MPEG1Parser *pReader, uint8 ui8ChunkId)
{
    if ((pReader == NULL) || (ui8ChunkId < 1)) {
        return -1;
    }
    ChunkWrapper *pChunkWr = new ChunkWrapper (ui8ChunkId, pReader);
    if (pChunkWr == NULL) {
        return -2;
    }
    if (_chunks.search (pChunkWr) != NULL) {
        return -3;
    }
    _chunks.insert (pChunkWr);
    return 0;
}

int MPEG1Reassembler::incorporateChunk (MPEG1Parser *pReader, int64 i64StartT, int64 i64EndT)
{
    if ((pReader == NULL) || (i64StartT < 0) || (i64EndT <= 0) || (i64StartT >= i64EndT)) {
        return -1;
    }
    AnnotationWrapper *pAnnWr = new AnnotationWrapper (i64StartT, i64EndT, pReader);
    if (pAnnWr == NULL) {
        return -2;
    }
    _annotations.insert (pAnnWr);
    return 0;
}

NOMADSUtil::BufferReader * MPEG1Reassembler::getAnnotatedVideo (void)
{
    return NULL;
}

BufferReader * MPEG1Reassembler::getReassembledVideo (void)
{
    const char *pszMethodName = "MPEG1Reassembler::getReassembledVideo";
    const int iCount =  _chunks.getCount();
    if (iCount <= 0) {
        return NULL;
    }
    ChunkWrapper *pChunkWr = _chunks.getFirst();
    MPEG1Parser *pParser = pChunkWr->_pReader;
    const int64 i64ChunkSize = pParser->getSize();
    if (iCount == 1) {
        void *pBuf = malloc (i64ChunkSize);
        if (pBuf == NULL) {
            return NULL;
        }
        if (pParser->readAll (pBuf, i64ChunkSize) < 0) {
            return NULL;
        }
        return new BufferReader (pBuf, static_cast<uint32>(i64ChunkSize), true);
    }
    unsigned int uiFrameCounter = 0;
    unsigned int uiGOPCounter = 0;
    static const unsigned int BUF_LEN = 1024 * 1024;
    BufferWriter bw (i64ChunkSize * iCount, 1024 * 15);
    void *pBuf = malloc (BUF_LEN);
    if (pBuf == NULL) {
        return NULL;
    }

    // Write sequence header
    int iOffset = pParser->readToNextSequence (pBuf, BUF_LEN);
    if (iOffset >= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "New sequence\n");
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes\n", iOffset);
        if ((iOffset > 0) && (bw.writeBytes (pBuf, iOffset) < 0)) {
            free (pBuf);
            return NULL;
        }
        if ((iOffset = pParser->readToNextSequenceGOP (pBuf, BUF_LEN)) >= 0) {
            checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "New group of pictures\n");
            checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes\n", iOffset);
            if ((iOffset > 0) && (bw.writeBytes (pBuf, iOffset) < 0)) {
                free (pBuf);
		        return NULL;
	        }
            ++uiGOPCounter;
        }
    }

    // Skip the sequence header for all the other chunks
    DArray<bool> hasMoreFrames (static_cast<unsigned int>(iCount + 1));
    hasMoreFrames[pChunkWr->_ui8ChunkId] = true;
    while ((pChunkWr = _chunks.getNext()) != NULL) {
        if ((pChunkWr->_pReader->goToNextSequence () < 0) ||
            ((iOffset = pChunkWr->_pReader->goToNextSequenceGOP()) < 0)) {
            free (pBuf);
            return NULL;
        }
        hasMoreFrames[pChunkWr->_ui8ChunkId] = true;
    }

    CircularPtrLList<ChunkWrapper> chunks (&_chunks);
    while ((pChunkWr = chunks.getNext()) != NULL) {
        uint8 ui8ChunkId = pChunkWr->_ui8ChunkId;
        pParser = pChunkWr->_pReader;
	    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "New group of pictures\n");
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes\n", iOffset);
        iOffset = pParser->readToNextSequenceGOP (pBuf, BUF_LEN);
        if ((iOffset > 0) && (bw.writeBytes (pBuf, iOffset) < 0)) {
            free (pBuf);
	        return NULL;
	    }
        unsigned int uiCurrBufLen = bw.getBufferLength();
        bool bAtLeastOne = false;
        while ((iOffset = pParser->readToNextGOPFrame (pBuf, BUF_LEN)) >= 0) {
	        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes for frame %u.\n",
                iOffset, uiFrameCounter);
            if ((iOffset > 0) && (bw.writeBytes (pBuf, iOffset) < 0)) {
                free (pBuf);
		        return NULL;
	        }
            bAtLeastOne = true;
            uiCurrBufLen = bw.getBufferLength();
	        ++uiFrameCounter;
	    }
        if (!bAtLeastOne) {
            hasMoreFrames[pChunkWr->_ui8ChunkId] = false;
        }
	    ++uiGOPCounter;
        /*
        if (offsets[ui8ChunkId] == 0) {
            offsets[ui8ChunkId] = pParser->readToNextGOPFrame (pBuf, BUF_LEN);
        }
        uiCurrBufLen = bw.getBufferLength();*/
        if (!chunksHaveMoreFrames (hasMoreFrames)) {
            break;
        }
    }

    if ((iOffset = pParser->readToEOF (pBuf, BUF_LEN)) >= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Writing %d bytes to EOF\n", iOffset);
        if ((iOffset > 0) && (bw.writeBytes (pBuf, iOffset) < 0)) {
            free (pBuf);
            return NULL;
        }
    }

    const unsigned long uiBufLen = bw.getBufferLength();
    return new BufferReader (bw.relinquishBuffer(), uiBufLen, true);
}

//-----------------------------------------------------------------------------
// ChunkWrapper
//-----------------------------------------------------------------------------

MPEG1Reassembler::ChunkWrapper::ChunkWrapper (uint8 ui8ChunkId, MPEG1Parser *pReader)
    : _ui8ChunkId (ui8ChunkId),
      _uiCurrChunk (0U),
      _pReader (pReader)
{
}

MPEG1Reassembler::ChunkWrapper::~ChunkWrapper (void)
{
}

bool MPEG1Reassembler::ChunkWrapper::operator > (const MPEG1Reassembler::ChunkWrapper &rhsChunkWr)
{
    return (_ui8ChunkId > rhsChunkWr._ui8ChunkId);
}

bool MPEG1Reassembler::ChunkWrapper::operator < (const MPEG1Reassembler::ChunkWrapper &rhsChunkWr)
{
    return (_ui8ChunkId < rhsChunkWr._ui8ChunkId);
}

bool MPEG1Reassembler::ChunkWrapper::operator == (const MPEG1Reassembler::ChunkWrapper &rhsChunkWr)
{
    return (_ui8ChunkId == rhsChunkWr._ui8ChunkId);
}

//-----------------------------------------------------------------------------
// AnnotationWrapper
//-----------------------------------------------------------------------------

MPEG1Reassembler::AnnotationWrapper::AnnotationWrapper (int64 i64StartT, int64 i64EndT, MPEG1Parser *pReader)
    : _i64StartT (i64StartT),
      _i64EndT (i64EndT),
      _pReader (pReader)
{
}

MPEG1Reassembler::AnnotationWrapper::~AnnotationWrapper (void)
{
}

int MPEG1Reassembler::AnnotationWrapper::includes (int64 i64FrameTimeInMillis) const
{
    if (i64FrameTimeInMillis < _i64StartT) {
        return -1;
    }
    if (i64FrameTimeInMillis > _i64EndT) {
        return 1;
    }
    return 0;
}

bool MPEG1Reassembler::AnnotationWrapper::operator > (const AnnotationWrapper &rhsAnnWr) const
{
    return (_i64StartT > rhsAnnWr._i64StartT);
}

bool MPEG1Reassembler::AnnotationWrapper::operator < (const AnnotationWrapper &rhsAnnWr) const
{
    return (_i64StartT < rhsAnnWr._i64StartT);
}

