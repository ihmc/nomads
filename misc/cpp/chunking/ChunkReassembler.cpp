/*
 * ChunkReassembler.cpp
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

#include "ChunkReassembler.h"

#include "BMPHandler.h"
#include "ImageCodec.h"
#include "MimeUtils.h"
#include "MPEG1Handler.h"

#include "BufferReader.h"
#include "Logger.h"
#include "BMPImage.h"
#ifdef WIN32
    #include "FFmpegHandler.h"
    #include "FFMPEGReader.h"
#endif
#include "VideoCodec.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define unsupportedType(type) Logger::L_MildError, "chunk type of %s is currently not supported\n", MimeUtils::toExtesion (type).c_str()

namespace IHMC_MISC
{
    typedef void Reassembler;

    const char * toString (ChunkReassembler::Type type)
    {
        switch (type) {
            case ChunkReassembler::Image: return "Image";
            case ChunkReassembler::Audio: return "Audio";
            case ChunkReassembler::Video: return "Video";
            default: return "Unknown";
        }
    }

    bool checkAndLogReassembler (Reassembler *pReassembler, ChunkReassembler::Type type)
    {
        if (pReassembler == NULL) {     
            checkAndLogMsg ("ChunkReassembler::checkAndLogReassembler", Logger::L_MildError,
                            "not initialized for reassembling type %s.\n", toString (type));
            return false;
        }
        return true;
    }

    int incorporateImage (BMPReassembler *pReassembler, const void *pBuf, uint32 ui32BufLen, Chunker::Type chunkType, uint8 ui8ChunkId)
    {
        const char *pszMethodName = "ChunkReassembler::incorporateImage (1)";
        if (!checkAndLogReassembler (pReassembler, ChunkReassembler::Image)) {
            return -1;
        }
        BMPImage *pBMPChunk = ImageCodec::decode (pBuf, ui32BufLen, chunkType);
        if (pBMPChunk == NULL) {
            return -2;
        }

        int rc = pReassembler->incorporateChunk (pBMPChunk, ui8ChunkId);
        if (0 != rc) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to incorporate "
                            "chunk id %d; rc = %d\n", (int) ui8ChunkId, rc);
            delete pBMPChunk;
            return -5;
        }
        delete pBMPChunk;
        return 0;
    }

    int incorporateImage (BMPReassembler *pReassembler, Chunker::Interval **ppIntervals,
                          const void *pBuf, uint32 ui32BufLen, Chunker::Type chunkType)
    {
        const char *pszMethodName = "ChunkReassembler::incorporateImage (2)";
        if (!checkAndLogReassembler (pReassembler, ChunkReassembler::Image)) {
            return -1;
        }
        BMPImage *pBMPChunk = ImageCodec::decode (pBuf, ui32BufLen, chunkType);
        if (pBMPChunk == NULL) {
            return -2;
        }

        uint32 ui32StartX = 0U;
        uint32 ui32EndX = 0U;
        uint32 ui32StartY = 0U;
        uint32 ui32EndY = 0U;
        for (uint8 i = 0; ppIntervals[i] != NULL; i++) {
            switch (ppIntervals[i]->dimension) {
                case Chunker::X:
                    ui32StartX = ppIntervals[i]->uiStart;
                    ui32EndX = ppIntervals[i]->uiEnd;
                    break;

                case Chunker::Y:
                    ui32StartY = ppIntervals[i]->uiStart;
                    ui32EndY = ppIntervals[i]->uiEnd;
                    break;

                default:
                    checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                    "unsupported dimension %d\n", (int) ppIntervals[i]->dimension);
                    return -3;
            }
        }

        int rc = pReassembler->incorporateChunk (pBMPChunk, ui32StartX, ui32EndX, ui32StartY, ui32EndY);
        if (0 != rc) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to incorporate annotation; rc = %d\n", rc);
            delete pBMPChunk;
            return -4;
        }
        delete pBMPChunk;
        return 0;
    }

    int incorporateVideo (MPEG1Reassembler *pMpeg1Reassembler, const void *pBuf, uint32 ui32BufLen, uint8 ui8ChunkId)
    {
        MPEG1Parser *pParser = MPEG1ParserFactory::newParser (pBuf, ui32BufLen);
        if (pParser == NULL) {
            return -2;
        }
        if (pMpeg1Reassembler->incorporateChunk (pParser, ui8ChunkId) < 0) {
            return -3;
        }
        return 0;
    }

    #ifdef WIN32
    int incorporateVideo (FFmpegReassembler *pFFMPEGReassembler, const void *pBuf, uint32 ui32BufLen, uint8 ui8ChunkId)
    {
        FFMPEGReader *pReader = new FFMPEGReader();
        if (pReader == NULL) {
            return -2;
        }
        if (pReader->read (pBuf, ui32BufLen) < 0) {
            delete pReader;
            return -3;
        }
        if (pFFMPEGReassembler->incorporateChunk (pReader, ui8ChunkId) < 0) {
            return -4;
        }
        return 0;
    }

    int incorporateVideo (FFmpegReassembler *pFFMPEGReassembler, Chunker::Interval **ppIntervals, const void *pBuf, uint32 ui32BufLen)
    {
        const char *pszMethodName = "ChunkReassembler::incorporateVideo (2)";
        if (!checkAndLogReassembler (pFFMPEGReassembler, ChunkReassembler::Image)) {
            return -1;
        }

        int64 i642StartX = 0U;
        int64 i64EndX = 0U;
        for (uint8 i = 0; ppIntervals[i] != NULL; i++) {
            switch (ppIntervals[i]->dimension) {
                case Chunker::T:
                    i642StartX = ppIntervals[i]->uiStart;
                    i64EndX = ppIntervals[i]->uiEnd;
                    break;

                default:
                    checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                    "unsupported dimension %d\n", (int) ppIntervals[i]->dimension);
                    return -3;
            }
        }

        FFMPEGReader *pReader = new FFMPEGReader();
        if (pReader == NULL) {
            return -5;
        }
        if (pReader->read (pBuf, ui32BufLen) < 0) {
            delete pReader;
            return -6;
        }

        int rc = pFFMPEGReassembler->incorporateChunk (pReader, i642StartX, i64EndX);
        if (0 != rc) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to incorporate annotation; rc = %d\n", rc);
            return -4;
        }
        return 0;
    }
    #endif

    BufferReader * getReassembledImage (BMPReassembler *pBMPReassembler, Chunker::Type outputType, uint8 ui8CompressionQuality, bool bAnnotated)
    {
        const char *pszMethodName = "ChunkReassembler::getReassembledImage";
        if (pBMPReassembler == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "not initialized for reassembling images\n");
            return NULL;
        }
        const BMPImage *pImage = bAnnotated ?
                                 pBMPReassembler->getAnnotatedImage() :
                                 pBMPReassembler->getReassembledImage();
        if (pImage == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to get %s image\n",
                            bAnnotated ? "annotated" : "reassembled");
            return NULL;
        }

        return ImageCodec::encode (pImage, outputType, ui8CompressionQuality);
    }
}


ChunkReassembler::ChunkReassembler (void)
    : _type (UNSUPPORTED),
      _ui8NoOfChunks (0),
      _pBMPReassembler (NULL),
      _pFFMPEGReassembler (NULL)
{
}

ChunkReassembler::~ChunkReassembler (void)
{    
}

int ChunkReassembler::init (Type reassemblerType, uint8 ui8NoOfChunks)
{
    if (ui8NoOfChunks < 2) {
        checkAndLogMsg ("ChunkReassembler::init", Logger::L_MildError,
                        "invalid number of chunks <%d>\n", (int) ui8NoOfChunks);
        return -1;
    }
    if (reassemblerType == Image) {
        int rc;
        _pBMPReassembler = new BMPReassembler();
        if (0 != (rc = _pBMPReassembler->init (ui8NoOfChunks))) {
            checkAndLogMsg ("ChunkReassembler::init", Logger::L_MildError,
                            "failed to initialize BMPReassembler; rc = %d\n", rc);
            return -2;
        }
        _type = reassemblerType;
        _ui8NoOfChunks = ui8NoOfChunks;
    }
    else if (reassemblerType == Video) {
        bool bUseMpeg1 = true;
        if (bUseMpeg1) {
            int rc;
            _pMpeg1Reassembler = new MPEG1Reassembler();
            if (0 != (rc = _pMpeg1Reassembler->init (ui8NoOfChunks))) {
                checkAndLogMsg ("ChunkReassembler::init", Logger::L_MildError,
                                "failed to initialize FFmpegReassembler; rc = %d\n", rc);
                return -3;
            }
            _type = reassemblerType;
            _ui8NoOfChunks = ui8NoOfChunks;
        }
        else {
#ifdef WIN32
            int rc;
            _pFFMPEGReassembler = new FFmpegReassembler();
            if (0 != (rc = _pFFMPEGReassembler->init (ui8NoOfChunks))) {
                checkAndLogMsg ("ChunkReassembler::init", Logger::L_MildError,
                    "failed to initialize FFmpegReassembler; rc = %d\n", rc);
                return -3;
            }
            _type = reassemblerType;
            _ui8NoOfChunks = ui8NoOfChunks;
#else
            return -3;
#endif
        }
    }
    else {
        return -4;
    }
    return 0;
}

int ChunkReassembler::incorporateChunk (const void *pBuf, uint32 ui32BufLen, Chunker::Type chunkType, uint8 ui8ChunkId)
{
    const char *pszMethodName = "ChunkReassembler::incorporateChunk";
    if ((pBuf == NULL) || (ui32BufLen == 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "pBuf is NULL, or ui32BufLen is 0\n");
        return -1;
    }

    if (ImageCodec::supports (chunkType)) {
        return ((incorporateImage (_pBMPReassembler, pBuf, ui32BufLen, chunkType, ui8ChunkId) < 0) ? -2 : 0);
    }
    if (VideoCodec::supports (chunkType)) {
        if (chunkType == Chunker::V_MPEG) {
            return (incorporateVideo (_pMpeg1Reassembler, pBuf, ui32BufLen, ui8ChunkId) < 0 ? -3 : 0);
        }
        #ifdef WIN32
        return (incorporateVideo (_pFFMPEGReassembler, pBuf, ui32BufLen, ui8ChunkId) < 0 ? -3 : 0);
        #endif
    }
    checkAndLogMsg (pszMethodName, unsupportedType (chunkType));
    return -4;
}

int ChunkReassembler::incorporateAnnotation (Chunker::Interval **ppIntervals, const void *pBuf, uint32 ui32BufLen,
                                             Chunker::Type chunkType)
{
    const char *pszMethodName = "ChunkReassembler::incorporateAnnotation";
    if ((ppIntervals == NULL) || (pBuf == NULL) || (ui32BufLen == 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "ppIntervals is NULL, or pBuf is NULL, or ui32BufLen is 0\n");
        return -1;
    }

    if (ImageCodec::supports (chunkType)) {
        return ((incorporateImage (_pBMPReassembler, ppIntervals, pBuf, ui32BufLen, chunkType) < 0) ? -2 : 0);
    }
    if (VideoCodec::supports (chunkType)) {
        #ifdef WIN32
        return (incorporateVideo (_pFFMPEGReassembler, ppIntervals, pBuf, ui32BufLen) < 0 ? -3 : 0);
        #endif
    }

    checkAndLogMsg (pszMethodName, unsupportedType (chunkType));
    return -3;
}

BufferReader * ChunkReassembler::getReassembledObject (Chunker::Type outputType, uint8 ui8CompressionQuality)
{
    if (ImageCodec::supports (outputType)) {
        return getReassembledImage (_pBMPReassembler, outputType, ui8CompressionQuality, false);
    }
    if (VideoCodec::supports (outputType)) {
        if (outputType == Chunker::V_MPEG) {
            if (_pMpeg1Reassembler != NULL) {
                return _pMpeg1Reassembler->getReassembledVideo();
            }
        }
#ifdef WIN32
        else if (_pFFMPEGReassembler != NULL) {
            return _pFFMPEGReassembler->getReassembledVideo();
        }
#endif
    }
    checkAndLogMsg ("ChunkReassembler::getAnnotatedObject", unsupportedType (outputType));
    return NULL;
}

NOMADSUtil::BufferReader * ChunkReassembler::getAnnotatedObject (Chunker::Type outputType, uint8 ui8CompressionQuality)
{
    if (ImageCodec::supports (outputType)) {
        return getReassembledImage (_pBMPReassembler, outputType, ui8CompressionQuality, true);
    }
    if (VideoCodec::supports (outputType)) {
        #ifdef WIN32
        if (_pFFMPEGReassembler != NULL) {
            return _pFFMPEGReassembler->getAnnotatedVideo();
        }
        #endif
    }
    checkAndLogMsg ("ChunkReassembler::getAnnotatedObject", unsupportedType (outputType));
    return NULL;
}

