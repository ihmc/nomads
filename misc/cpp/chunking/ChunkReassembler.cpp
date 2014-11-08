/*
 * ChunkReassembler.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2014 IHMC.
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
#include "JasperWrapper.h"
#include "JPEGLibWrapper.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "Logger.h"
#include "media/BMPImage.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

ChunkReassembler::ChunkReassembler (void)
{
    _type = UNSUPPORTED;
    _ui8NoOfChunks = 0;
    _pBMPReassembler = NULL;
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
    else {
        checkAndLogMsg ("ChunkReassembler::init", Logger::L_MildError,
                        "type <%d> is not supported by the reassembler\n", (int) reassemblerType);
        return -3;
    }
    return 0;
}

int ChunkReassembler::incorporateChunk (const void *pBuf, uint32 ui32BufLen, Chunker::Type chunkType, uint8 ui8ChunkId)
{
    if ((pBuf == NULL) || (ui32BufLen == 0)) {
        checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                        "pBuf is NULL or ui32BufLen is 0\n");
        return -1;
    }
    if ((chunkType != Chunker::BMP) && (chunkType != Chunker::JPEG) && (chunkType != Chunker::JPEG2000)) {
        checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                        "chunk type of %d is currently not supported\n", (int) chunkType);
        return -2;
    }
    if (_pBMPReassembler == NULL) {
        checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                        "not initialized for reassembling images\n");
        return -3;
    }
    int rc;
    BMPImage *pBMPChunk = NULL;
    if (chunkType == Chunker::BMP) {
        pBMPChunk = new BMPImage();
        BufferReader br (pBuf, ui32BufLen);
        if (0 != (rc = pBMPChunk->readHeaderAndImage (&br))) {
            checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                            "failed to read BMP chunk; rc = %d\n", rc);
            return -4;
        }
    }
    else if (chunkType == Chunker::JPEG) {
        pBMPChunk = JPEGLibWrapper::convertJPEGToBMP (pBuf, ui32BufLen);
        if (pBMPChunk == NULL) {
            checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                            "failed to convert JPEG chunk to BMP\n");
            return -5;
        }
    }
    else if (chunkType == Chunker::JPEG2000) {
        #if defined (ANDROID)
            checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                            "JPEG2000 not supported on the Android platform\n");
            return -6;
        #else
            BufferReader *pReader = JasperWrapper::convertToBMP (pBuf, ui32BufLen);
            if (pReader == NULL) {
                checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                                "failed to convert JPEG2000 chunk to BMP\n");
                return -6;
            }
            int rc;
            pBMPChunk = new BMPImage();
            if (0 != (rc = pBMPChunk->readHeaderAndImage (pReader))) {
                checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                                "failed to read BMP chunk after converting JPEG2000 chunk to BMP; rc = %d\n", rc);
                delete pBMPChunk;
                delete pReader;
                return -7;
            }
            delete pReader;
        #endif
    }
    if (0 != (rc = _pBMPReassembler->incorporateChunk (pBMPChunk, ui8ChunkId))) {
        checkAndLogMsg ("ChunkReassembler::incorporateChunk", Logger::L_MildError,
                        "failed to incorporate chunk id %d; rc = %d\n", (int) ui8ChunkId, rc);
        delete pBMPChunk;
        return -8;
    }
    delete pBMPChunk;
    return 0;
}

BufferReader * ChunkReassembler::getReassembedObject (Chunker::Type outputType, uint8 ui8CompressionQuality)
{
    if ((outputType != Chunker::BMP) && (outputType != Chunker::JPEG) && (outputType != Chunker::JPEG2000)) {
        checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                        "output type of %d is currently not supported\n", (int) outputType);
        return NULL;
    }
    if (_pBMPReassembler == NULL) {
        checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                        "not initialized for reassembling images\n");
        return NULL;
    }
    const BMPImage *pImage = _pBMPReassembler->getReassembledImage();
    if (pImage == NULL) {
        checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                        "failed to get reassembled image\n");
        return NULL;
    }
    if (outputType == Chunker::BMP) {
        int rc;
        BufferWriter bw (pImage->getTotalSize(), 1024);
        if (0 != (rc = pImage->writeHeaderAndImage (&bw))) {
            checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                            "failed to write reassembled image as a BMP; rc = %d\n", rc);
            return NULL;
        }
        uint32 ui32BufLen = bw.getBufferLength();
        BufferReader *pReader = new BufferReader (bw.relinquishBuffer(), ui32BufLen, true);
        return pReader;
    }
    else if (outputType == Chunker::JPEG) {
        BufferReader *pReader = JPEGLibWrapper::convertBMPToJPEG (pImage, ui8CompressionQuality);
        if (pReader == NULL) {
            checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                            "failed to convert reassembled image into JPEG\n");
            return NULL;
        }
        return pReader;
    }
    else if (outputType == Chunker::JPEG2000) {
        #if defined (ANDROID)
            checkAndLogMsg ("ChunkReassembler::getReassembledObject", Logger::L_MildError,
                            "JPEG2000 not supported on the Android Platform\n");
            return NULL;
        #else
            int rc;
            BufferWriter bw (pImage->getTotalSize(), 1024);
            if (0 != (rc = pImage->writeHeaderAndImage (&bw))) {
                checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                                "failed to write reassembled image as a BMP before converting to JPEG2000; rc = %d\n", rc);
                return NULL;
            }
            BufferReader *pReader = JasperWrapper::convertToJPEG2000 (bw.getBuffer(), bw.getBufferLength());
            if (pReader == NULL) {
                checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                                "failed to convert reassembled image into JPEG2000\n");
                return NULL;
            }
            return pReader;
        #endif
    }
    else {
        // Should not come here...
        checkAndLogMsg ("ChunkReassembler::getReassembedObject", Logger::L_MildError,
                        "output type of %d is currently not supported\n", (int) outputType);
        return NULL;
    }
}
