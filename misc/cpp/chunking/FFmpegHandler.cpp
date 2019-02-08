/*
 * FFmpegHandler.cpp
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
 * Created on July 28, 2015, 2:13 AM
 */

#include "FFmpegHandler.h"

#include "ChunkingUtils.h"

#include "FFMPEGReader.h"

#include "BufferReader.h"
#include "FileUtils.h"
#include "Logger.h"

#include <math.h>
#include  <limits.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_MISC;
using namespace NOMADSUtil;

static const int QUALITY = 2;  // ranges from 0 to 2.  ) being the lowest quality, 2 being the highest

namespace IHMC_MISC
{
    Reader * chunk (FFMPEGReader *pReader, unsigned int uiStartIndex, unsigned int uiEndIndex, uint8 ui8Increment)
    {
        if (pReader == NULL) {
            return NULL;
        }
        const FFMPEGUtil::VideoEncodingProfile *pProfile = pReader->getVideoEncProfile();
        const FFMPEGUtil::VideoFormat *pVideoformat = pReader->getVideoFormat();
        if ((pProfile == NULL) || (pVideoformat == NULL)) {
            return NULL;
        }
        FFMPEGWriter writer;
        FFMPEGUtil::Audio *pAudio = NULL;  // TODO: fix FFMPEGReader and FFMPEGWriter to support audio codiing/encoding
        const bool bConvertAudio = false;
        const String filename ("TMP-chunk.dat");
        FileUtils::deleteFile (filename);
        if (writer.createFile (filename, pProfile, pVideoformat, QUALITY, bConvertAudio, pAudio) < 0) {
            return NULL;
        }
        RGBImage *pRGB;
        RGBImage *pBaseRGB;
        for (unsigned int i = uiStartIndex; (i < uiEndIndex) && ((pRGB = pReader->getFrame (i)) != NULL); i += ui8Increment) {
            for (unsigned int j = 0; j < ui8Increment; j++) {
                if ((writer.encodeImage (*pRGB, pReader->getFrameTimeInMillis (i + j))) < 0) {
                    return NULL;
                }
            }
        }
        writer.close();
        BufferReader *pBufReader = ChunkingUtils::toReader (filename);
        FileUtils::deleteFile (filename);
        return pBufReader;
    }
}

//-----------------------------------------------------------------------------
// FFmpegChunker
//-----------------------------------------------------------------------------


Reader * FFmpegChunker::fragmentFFmpeg (FFMPEGReader *pReader, uint8 ui8DesiredChunkId, uint8 ui8TotalNoOfChunks)
{
    const char *pszMethodName = "FFmpegChunker::fragmentFFmpeg";
    if ((pReader == NULL) || (ui8DesiredChunkId < 1)) {
        return NULL;
    }
    const FFMPEGUtil::VideoFormat *pVidefoFormat = pReader->getVideoFormat();
    if (pVidefoFormat == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not read video format\n");
    }
    int64 i64NFrames = pVidefoFormat->getNumberOfFrames();
    if (i64NFrames <= 0) {
        i64NFrames = 515;  // TODO: fix this!
    }
    if (i64NFrames > UINT_MAX) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "the video has more than %lld frames.  It will be truncated.\n", i64NFrames);
        i64NFrames = UINT_MAX;
    }
    return chunk (pReader, ui8DesiredChunkId, static_cast<unsigned int>(i64NFrames), ui8TotalNoOfChunks);
}

Reader * FFmpegChunker::extractFromFFmpeg (FFMPEGReader *pReader, int64 i64StartT, int64 i64EndT)
{
    if ((pReader == NULL) || (i64StartT < 0) || (i64EndT <= 0) || (i64StartT >= i64EndT)) {
        return NULL;
    }
    const int iStartT = pReader->getFrameIndexByTime (i64StartT);
    const int iEndT = pReader->getFrameIndexByTime (i64EndT);
    if ((iStartT < 0) || (iEndT < 0)) {
        return NULL;
    }
    return chunk (pReader, iStartT, iEndT, 1);
}

//-----------------------------------------------------------------------------
// FFmpegReassembler
//-----------------------------------------------------------------------------

FFmpegReassembler::FFmpegReassembler (void)
    : _ui8TotalNoOfChunks (0), _chunks (false), _annotations (false)
{
}

FFmpegReassembler::~FFmpegReassembler (void)
{
}

int FFmpegReassembler::init (uint8 ui8TotalNoOfChunks)
{
    _ui8TotalNoOfChunks = ui8TotalNoOfChunks;
    return 0;
}

int FFmpegReassembler::incorporateChunk (NOMADSUtil::FFMPEGReader *pReader, uint8 ui8ChunkId)
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

int FFmpegReassembler::incorporateChunk (NOMADSUtil::FFMPEGReader *pReader, int64 i64StartT, int64 i64EndT)
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

NOMADSUtil::BufferReader * FFmpegReassembler::getAnnotatedVideo (void)
{
    ChunkWrapper *pCurr = _chunks.getFirst ();
    if (pCurr == NULL) {
        return NULL;
    }
    const FFMPEGUtil::VideoFormat *pVideoformat = pCurr->_pReader->getVideoFormat ();
    const FFMPEGUtil::VideoEncodingProfile *pProfile = pCurr->_pReader->getVideoEncProfile ();
    if ((pVideoformat == NULL) || (pProfile == NULL)) {
        return NULL;
    }
    const String outputFileName ("TMP-reassembled-and-annotated.dat");
    FFMPEGWriter writer;
    if (writer.createFile (outputFileName, pProfile, pVideoformat, QUALITY, false, NULL) < 0) {
        return NULL;
    }
    int64 i64NumberOfFrames = pVideoformat->getNumberOfFrames();
    AnnotationWrapper *pCurrAnn = _annotations.getFirst();
    ChunkWrapper *pNext = NULL;
    int64 iAnnotationFrameIdx = 1;
    for (int64 i = 1; i <= i64NumberOfFrames; i++) {
        const int64 i64FrameTimeInMillis = pCurr->_pReader->getFrameTimeInMillis (i);
        const int iIncluded = pCurrAnn == NULL ? -1 : pCurrAnn->includes (i64FrameTimeInMillis);
        RGBImage *pRGB;
        if (iIncluded == 0) {
            const int64 i64AnnNumberOfFrames = pCurrAnn->_pReader->getVideoFormat()->getNumberOfFrames();
            for (int64 j = 1; j <= i64AnnNumberOfFrames; j++) {
                pRGB = pCurrAnn->_pReader->getFrame (j);
                pCurr->_pReader->getFrame (i+j);
                if (writer.encodeImage (*pRGB, pCurr->_pReader->getFrameTimeInMillis (i+j)) < 0) {
                    FileUtils::deleteFile (outputFileName);
                    return NULL;
                }
            }
            i += i64AnnNumberOfFrames;
            pCurrAnn = _annotations.getNext();
            //iAnnotationFrameIdx++;
        }
        else {
            pRGB = pCurr->_pReader->getFrame (i);
            if (writer.encodeImage (*pRGB, i64FrameTimeInMillis) < 0) {
                FileUtils::deleteFile (outputFileName);
                return NULL;
            }
            if (pNext == NULL) {
                pNext = _chunks.getNext();
                if (pNext == NULL) {
                    pNext = _chunks.getFirst();
                }
            }
            if (0 == ((i + 1) % pNext->_ui8ChunkId)) {
                pCurr = pNext;
                pNext = NULL;
            }
        }

        // if a chunk is missing take the next frame from the current chunk
    }
    writer.close();
    BufferReader *pReader = ChunkingUtils::toReader (outputFileName);
    FileUtils::deleteFile (outputFileName);
    return pReader;
}

NOMADSUtil::BufferReader * FFmpegReassembler::getReassembledVideo (void)
{
    ChunkWrapper *pCurr = _chunks.getFirst();
    if (pCurr == NULL) {
        return NULL;
    }
    const FFMPEGUtil::VideoFormat *pVideoformat = pCurr->_pReader->getVideoFormat();
    const FFMPEGUtil::VideoEncodingProfile *pProfile = pCurr->_pReader->getVideoEncProfile();
    if ((pVideoformat == NULL) || (pProfile == NULL)) {
        return NULL;
    }
    const String outputFileName ("TMP-reassembled.dat");
    FFMPEGWriter writer;
    if (writer.createFile (outputFileName, pProfile, pVideoformat, QUALITY, false, NULL) < 0) {
        FileUtils::deleteFile (outputFileName);
        return NULL;
    }
    int64 i64Gap = pCurr->_pReader->getFrameTimeInMillis (2) - pCurr->_pReader->getFrameTimeInMillis (1);
    i64Gap = static_cast<int64>(ceil (static_cast<float>(i64Gap) / static_cast<float>(_ui8TotalNoOfChunks)));
    int64 i64NumberOfFrames = pVideoformat->getNumberOfFrames();
    int64 i64Idx = 1;
    /*for (int64 i = 1; i <= i64NumberOfFrames; i++) {
        while (pCurr != NULL) {
            RGBImage *pRGB = pCurr->_pReader->getFrame (i);
            //int64 i64FrameIndexForTimestamp = i + (i64Gap * (pCurr->_ui8ChunkId - 1));
            int64 i64FrameIndexForTimestamp = i * pCurr->_ui8ChunkId;
            // int64 i64Timestamp = pCurr->_pReader->getFrameTimeInMillis (i64FrameIndexForTimestamp);
            int64 i64Timestamp = pCurr->_pReader->getFrameTimeInMillis (i64Idx);
            if (writer.encodeImage (*pRGB, i64Timestamp) < 0) {
                return NULL;
            }
            pCurr = _chunks.getNext();
            i64Idx++;
        }
        pCurr = _chunks.getFirst();
    }*/
    ChunkWrapper *pNext = NULL;
    for (int64 i = 1; i <= i64NumberOfFrames; i++) {
        RGBImage *pRGB = pCurr->_pReader->getFrame (i);
        int rc = writer.encodeImage (*pRGB, pCurr->_pReader->getFrameTimeInMillis (i));
        if (rc < 0) {
            return NULL;
        }
        if (pNext == NULL) {
            pNext = _chunks.getNext();
            if (pNext == NULL) {
                pNext = _chunks.getFirst();
            }
        }
        if (0 == ((i + 1) % pNext->_ui8ChunkId)) {
            pCurr = pNext;
            pNext = NULL;
        }
        // if a chunk is missing take the next frame from the current chunk
    }
    writer.close();
    BufferReader *pReader = ChunkingUtils::toReader (outputFileName);
    FileUtils::deleteFile (outputFileName);
    return pReader;
}

//-----------------------------------------------------------------------------
// ChunkWrapper
//-----------------------------------------------------------------------------

FFmpegReassembler::ChunkWrapper::ChunkWrapper (uint8 ui8ChunkId, FFMPEGReader *pReader)
    : _ui8ChunkId (ui8ChunkId),
      _uiCurrChunk (0U),
      _pReader (pReader)
{
}

FFmpegReassembler::ChunkWrapper::~ChunkWrapper (void)
{
}

bool FFmpegReassembler::ChunkWrapper::operator > (const FFmpegReassembler::ChunkWrapper &rhsChunkWr)
{
    return (_ui8ChunkId > rhsChunkWr._ui8ChunkId);
}

bool FFmpegReassembler::ChunkWrapper::operator < (const FFmpegReassembler::ChunkWrapper &rhsChunkWr)
{
    return (_ui8ChunkId < rhsChunkWr._ui8ChunkId);
}

bool FFmpegReassembler::ChunkWrapper::operator == (const FFmpegReassembler::ChunkWrapper &rhsChunkWr)
{
    return (_ui8ChunkId == rhsChunkWr._ui8ChunkId);
}

//-----------------------------------------------------------------------------
// AnnotationWrapper
//-----------------------------------------------------------------------------

FFmpegReassembler::AnnotationWrapper::AnnotationWrapper (int64 i64StartT, int64 i64EndT, NOMADSUtil::FFMPEGReader *pReader)
    : _i64StartT (i64StartT),
      _i64EndT (i64EndT),
      _pReader (pReader)
{
}

FFmpegReassembler::AnnotationWrapper::~AnnotationWrapper (void)
{
}

int FFmpegReassembler::AnnotationWrapper::includes (int64 i64FrameTimeInMillis) const
{
    if (i64FrameTimeInMillis < _i64StartT) {
        return -1;
    }
    if (i64FrameTimeInMillis > _i64EndT) {
        return 1;
    }
    return 0;
}

bool FFmpegReassembler::AnnotationWrapper::operator > (const AnnotationWrapper &rhsAnnWr) const
{
    return (_i64StartT > rhsAnnWr._i64StartT);
}

bool FFmpegReassembler::AnnotationWrapper::operator < (const AnnotationWrapper &rhsAnnWr) const
{
    return (_i64StartT < rhsAnnWr._i64StartT);
}

