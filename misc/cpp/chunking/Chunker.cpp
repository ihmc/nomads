/*
 * Chunker.cpp
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
 */

#include "Chunker.h"

#include "AudioCodec.h"
#include "ChunkingUtils.h"
#include "BMPHandler.h"
#include "Defs.h"
#ifdef WIN32
    #include "FFmpegHandler.h"
#endif
#include "ImageCodec.h"
#include "MPEG1Handler.h"
#include "VideoCodec.h"

#include "BufferReader.h"
#include "BMPImage.h"
#include "FFMPEGReader.h"

#include "FileUtils.h"
#include "Logger.h"

#include <stdlib.h>
#include <limits.h>

#define encodingErrMsg Logger::L_MildError, "failed to encode BMP chunk into %s.\n"
#define decodingErrMsg Logger::L_MildError, "failed to decode %s chunk into BMP.\n"

using namespace NOMADSUtil;
using namespace IHMC_MISC;

namespace IHMC_MISC
{
    typedef PtrLList<Chunker::Fragment> Fragments;

    Chunker::Fragment * toFragment (Reader *pReader, Chunker::Type inputObjectType, uint8 ui8CurrentChunk, uint8 ui8NoOfChunks, Chunker::Type outputChunkType)
    {
        Chunker::Fragment *pFragment = new Chunker::Fragment();
        pFragment->src_type = inputObjectType;
        pFragment->pReader = pReader;
        pFragment->ui64FragLen = pReader->getBytesAvailable ();
        pFragment->out_type = outputChunkType;
        pFragment->ui8Part = ui8CurrentChunk;
        pFragment->ui8TotParts = ui8NoOfChunks;
        return pFragment;
    }

    int fragmentMpegV1 (MPEG1Parser *pParser, PtrLList<Chunker::Fragment> &fragments, uint8 ui8NoOfChunks,
                       Chunker::Type inputObjectType, Chunker::Type outputChunkType)
    {
        if (pParser == NULL) {
            return -1;
        }
        for (uint8 ui8CurrentChunk = 1; ui8CurrentChunk <= ui8NoOfChunks; ui8CurrentChunk++) {
            Reader *pReader = MPEG1Chunker::fragmentMpeg1 (pParser, ui8CurrentChunk, ui8NoOfChunks);
            if (pReader == NULL) {
                delete pParser;
                return -2;
            }
            fragments.append (toFragment (pReader, inputObjectType, ui8CurrentChunk, ui8NoOfChunks, outputChunkType));
        }
        return 0;
    }
}

PtrLList<Chunker::Fragment> * Chunker::fragmentFile (const char *pszFileName, Type inputObjectType,
                                                     uint8 ui8NoOfChunks, Type outputChunkType,
                                                     uint8 ui8ChunkCompressionQuality)
{
    if (pszFileName == NULL) {
        return NULL;
    }
    if (ImageCodec::supports (inputObjectType)) {
        // Convert to buffer (images are assumed not to be too big...)
        BufferReader *pReader = ChunkingUtils::toReader (pszFileName);
        if (pReader == NULL) {
            return NULL;
        }
        PtrLList<Fragment> *pFragments = fragmentBuffer (pReader->getBuffer(), pReader->getBufferLength(),
                                                         inputObjectType, ui8NoOfChunks, outputChunkType,
                                                         ui8ChunkCompressionQuality);
        delete pReader;
        return pFragments;
    }

    PtrLList<Fragment> *pFragments = new PtrLList<Fragment>();
    if (pFragments == NULL) {
        return NULL;
    }
    if ((VideoCodec::supports (inputObjectType) && VideoCodec::supports (outputChunkType)) ||
        (AudioCodec::supports (inputObjectType) && AudioCodec::supports (outputChunkType))) {
        if (V_MPEG) {
            pFragments = new PtrLList<Fragment>();
            MPEG1Parser *pParser = MPEG1ParserFactory::newParser (pszFileName);
            if (fragmentMpegV1 (pParser, *pFragments, ui8NoOfChunks, inputObjectType, outputChunkType) < 0) {
                delete pFragments;
            }
            delete pParser;
        }
        else {
#ifdef WIN32
            FFMPEGReader reader;
            if (reader.openFile (pszFileName) < 0) {
                return NULL;
            }
            for (uint8 ui8CurrentChunk = 1; ui8CurrentChunk <= ui8NoOfChunks; ui8CurrentChunk++) {
                Reader *pReader = FFmpegChunker::fragmentFFmpeg (&reader, ui8CurrentChunk, ui8NoOfChunks);
                if (pReader == NULL) {
                    delete pFragments;
                    return NULL;
                }
                pFragments->append (toFragment (pReader, inputObjectType, ui8CurrentChunk, ui8NoOfChunks, outputChunkType));
            }
#else
            return NULL;
#endif
        }
    }
    if (pFragments->getFirst() == NULL) {
        delete pFragments;
        pFragments = NULL;
    }
    return pFragments;
}

PtrLList<Chunker::Fragment> * Chunker::fragmentBuffer (const void *pBuf, uint32 ui32Len, Type inputObjectType, uint8 ui8NoOfChunks,
                                                       Type outputChunkType, uint8 ui8ChunkCompressionQuality)
{
    PtrLList<Fragment> *pFragments = NULL;
    if (ImageCodec::supports (inputObjectType)) {
        BMPImage *pBMPImage = ImageCodec::decode (pBuf, ui32Len, inputObjectType);
        if (pBMPImage == NULL) {
            return NULL;
        }
        pFragments = new PtrLList<Fragment>();
        for (uint8 ui8CurrentChunk = 1; ui8CurrentChunk <= ui8NoOfChunks; ui8CurrentChunk++) {
            BMPImage *pBMPChunk = BMPChunker::fragmentBMP (pBMPImage, ui8CurrentChunk, ui8NoOfChunks);
            if (pBMPChunk == NULL) {
                checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                "failed to fragment BMP to create chunk %d of %d\n",
                                static_cast<int>(ui8CurrentChunk), static_cast<int>(ui8NoOfChunks));
                delete pFragments;
                return NULL;
            }

            BufferReader *pReader = ImageCodec::encode (pBMPChunk, outputChunkType, ui8ChunkCompressionQuality);
            if (pReader == NULL) {
                delete pFragments;
                return NULL;
            }
            pFragments->append (toFragment (pReader, inputObjectType, ui8CurrentChunk, ui8NoOfChunks, outputChunkType));
        }
    }
    else if (VideoCodec::supports (inputObjectType)) {
        if (V_MPEG) {
            pFragments = new PtrLList<Fragment>();
            MPEG1Parser *pParser = MPEG1ParserFactory::newParser (pBuf, ui32Len);
            if (fragmentMpegV1 (pParser, *pFragments, ui8NoOfChunks, inputObjectType, outputChunkType) < 0) {
                delete pFragments;
            }
            delete pParser;
        }
        else {
            String tmpfilename ("TMP-srcvideo.data");
            if (FileUtils::dumpBufferToFile (pBuf, ui32Len, tmpfilename) < 0) {
                return NULL;
            }
            pFragments = fragmentFile (tmpfilename, inputObjectType, ui8NoOfChunks,
                                       outputChunkType, ui8ChunkCompressionQuality);
            FileUtils::deleteFile (tmpfilename);
            return pFragments;
        }
    }

    return pFragments;
}

Chunker::Fragment * Chunker::extractFromFile (const char *pszFileName, Type inputObjectType, Type outputChunkType,
                                              uint8 ui8ChunkCompressionQuality, Interval **ppPortionIntervals)
{
    
    if (ImageCodec::supports (inputObjectType)) {
        // Convert to buffer (images are assumed not to be too big...)
        BufferReader *pReader = ChunkingUtils::toReader (pszFileName);
        if (pReader == NULL) {
            return NULL;
        }
        Chunker::Fragment *pFragment = extractFromBuffer (pReader->getBuffer(), pReader->getBufferLength(),
                                                          inputObjectType, outputChunkType,
                                                          ui8ChunkCompressionQuality, ppPortionIntervals);
        delete pReader;
        return pFragment;
    }

    if ((VideoCodec::supports (inputObjectType) && VideoCodec::supports (outputChunkType)) ||
        (AudioCodec::supports (inputObjectType) && AudioCodec::supports (outputChunkType))) {

        int64 i64StartT = 0U;
        int64 i64EndT = 0U;
        if (ppPortionIntervals != NULL) {
            for (unsigned int i = 0; ppPortionIntervals[i] != NULL; i++) {
                if ((ppPortionIntervals[i]->uiStart > 0xFFFFFFFFFFFFFFFF) || (ppPortionIntervals[i]->uiEnd > 0xFFFFFFFFFFFFFFFF)) {
                    return NULL;
                }
                if (ppPortionIntervals[i]->dimension == T) {
                    i64StartT = ppPortionIntervals[i]->uiStart;
                    i64EndT = ppPortionIntervals[i]->uiEnd;
                }
                else {
                    return NULL;
                }
            }
        }

        #ifdef WIN32
        FFMPEGReader reader;
        if (reader.openFile (pszFileName) < 0) {
            return NULL;
        }
        Reader *pReader = FFmpegChunker::extractFromFFmpeg (&reader, i64StartT, i64EndT);
        if (pReader == NULL) {
            return NULL;
        }  
        return toFragment (pReader, inputObjectType, 1, 1, outputChunkType);
        #else
        return NULL;
        #endif
    }

    return NULL;
}

Chunker::Fragment * Chunker::extractFromBuffer (const void *pBuf, uint32 ui32Len, Type inputObjectType, Type outputChunkType,
                                                uint8 ui8ChunkCompressionQuality, Interval **ppPortionIntervals)
{
    
    if (ImageCodec::supports (inputObjectType)) {
        BMPImage *pBMPImage = ImageCodec::decode (pBuf, ui32Len, inputObjectType);
        if (pBMPImage == NULL) {
            return NULL;
        }

        uint32 ui32StartX = 0U;
        uint32 ui32EndX = pBMPImage->getWidth();
        uint32 ui32StartY = 0U;
        uint32 ui32EndY = pBMPImage->getHeight();

        if (ppPortionIntervals != NULL) {
            for (unsigned int i = 0; ppPortionIntervals[i] != NULL; i++) {
                if ((ppPortionIntervals[i]->uiStart > UINT_MAX) || (ppPortionIntervals[i]->uiEnd > UINT_MAX)) {
                    return NULL;
                }
                if (ppPortionIntervals[i]->dimension == X) {
                    ui32StartX = ppPortionIntervals[i]->uiStart;
                    ui32EndX = ppPortionIntervals[i]->uiEnd;
                }
                else if (ppPortionIntervals[i]->dimension == Y) {
                    ui32StartY = ppPortionIntervals[i]->uiStart;
                    ui32EndY = ppPortionIntervals[i]->uiEnd;
                }
                else {
                    return NULL;
                }
            }
        }

        BMPImage *pBMPChunk = BMPChunker::extractFromBMP (pBMPImage, ui32StartX, ui32EndX, ui32StartY, ui32EndY);
        if (pBMPChunk == NULL) {
            return NULL;
        }
        BufferReader *pReader = ImageCodec::encode (pBMPChunk, outputChunkType, ui8ChunkCompressionQuality);
        if (pReader == NULL) {
            return NULL;
        }
        return toFragment (pReader, inputObjectType, 1, 1, outputChunkType);   
    }
    if ((VideoCodec::supports (inputObjectType) && VideoCodec::supports (outputChunkType)) ||
             (AudioCodec::supports (inputObjectType) && AudioCodec::supports (outputChunkType))) {

        String tmpfilename ("TMP-srcvideo.data");
        if (FileUtils::dumpBufferToFile (pBuf, ui32Len, tmpfilename) < 0) {
            return NULL;
        }
        Chunker::Fragment *pFragment = extractFromFile (tmpfilename, inputObjectType, outputChunkType,
                                                        ui8ChunkCompressionQuality, ppPortionIntervals);
        FileUtils::deleteFile (tmpfilename);
        return pFragment;
    }
    return NULL;
}

Chunker::Interval::Interval ()
    : dimension (Chunker::X),
      uiStart (0U),
      uiEnd (0U)
{
}

Chunker::Interval::Interval (const Interval& interval)
    : dimension (interval.dimension),
      uiStart (interval.uiStart),
      uiEnd (interval.uiEnd)
{
}

Chunker::Interval::~Interval (void)
{
}

