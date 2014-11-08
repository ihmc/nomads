/*
 * Chunker.cpp
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

#include "Chunker.h"

#include "BMPHandler.h"
#include "JasperWrapper.h"
#include "JPEGLibWrapper.h"
#include "PNGLibWrapper.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "media/BMPImage.h"

#include "Defs.h"

#include "Logger.h"

#include <stdlib.h>

using namespace NOMADSUtil;
using namespace IHMC_MISC;

PtrLList<Chunker::Fragment> * Chunker::fragmentBuffer (const void *pBuf, uint32 ui32Len, Type inputObjectType, uint8 ui8NoOfChunks, Type outputChunkType, uint8 ui8ChunkCompressionQuality)
{
    BMPImage *pBMPImage = NULL;
    if (inputObjectType == Chunker::BMP) {
        BufferReader br (pBuf, ui32Len, false);
        int rc;
        pBMPImage = new BMPImage();
        if (0 != (rc = pBMPImage->readHeaderAndImage (&br))) {
            checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                            "failed to read BMP image; rc = %d\n", rc);
            delete pBMPImage;
            return NULL;
        }
    }
    else if (inputObjectType == Chunker::JPEG) {
        pBMPImage = JPEGLibWrapper::convertJPEGToBMP (pBuf, ui32Len);
        if (pBMPImage == NULL) {
            checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                            "failed to convert JPEG image to BMP\n");
            return NULL;
        }
    }
    else if (inputObjectType == Chunker::JPEG2000) {
        #if defined (ANDROID)
            checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                            "JPEG2000 not supported on the Android platform\n");
            return NULL;
        #else
            BufferReader *pReader = JasperWrapper::convertToBMP (pBuf, ui32Len);
            if (pReader == NULL) {
                checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                "failed to convert JPEG2000 image to BMP\n");
                return NULL;
            }
            int rc;
            pBMPImage = new BMPImage();
            if (0 != (rc = pBMPImage->readHeaderAndImage (pReader))) {
                checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                "failed to read BMP image after converting JPEG2000 to BMP; rc = %d\n", rc);
                delete pBMPImage;
                delete pReader;
                return NULL;
            }
            delete pReader;
        #endif
    }
    else if (inputObjectType == Chunker::PNG) {
        #if defined (ANDROID)
            checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                            "JPEG2000 not supported on the Android platform\n");
            return NULL;
        #else
            pBMPImage = PNGLibWrapper::convertPNGToBMP (pBuf, ui32Len);
            if (pBMPImage == NULL) {
                checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                "failed to convert PNG image to BMP\n");
                return NULL;
            }
        #endif
    }
    else {
        checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                        "unsupported input type %d\n", (int) inputObjectType);
        return NULL;
    }
    PtrLList<Fragment> *pFragments = new PtrLList<Fragment>;
    for (uint8 ui8CurrentChunk = 1; ui8CurrentChunk <= ui8NoOfChunks; ui8CurrentChunk++) {
        BMPImage *pBMPChunk = BMPChunker::fragmentBMP (pBMPImage, ui8CurrentChunk, ui8NoOfChunks);
        if (pBMPChunk == NULL) {
            checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                            "failed to fragment BMP to create chunk %d of %d\n", (int) ui8CurrentChunk, (int) ui8NoOfChunks);
            delete pFragments;
            return NULL;
        }
        int rc;
        Fragment *pFragment = new Fragment();
        pFragment->src_type = inputObjectType;
        if (outputChunkType == Chunker::BMP) {
            BufferWriter bw (pBMPChunk->getTotalSize(), 1024);
            if (0 != (rc = pBMPChunk->writeHeaderAndImage (&bw))) {
                checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                "failed to write chunk BMP into buffer; rc = %d\n", rc);
                delete pFragments;
                return NULL;
            }
            uint32 ui32ChunkLen = bw.getBufferLength();
            pFragment->pReader = new BufferReader (bw.relinquishBuffer(), ui32ChunkLen, true);
            pFragment->ui64FragLen = ui32ChunkLen;
        }
        else if (outputChunkType == Chunker::JPEG) {
            BufferReader *pReader = JPEGLibWrapper::convertBMPToJPEG (pBMPChunk, ui8ChunkCompressionQuality);
            if (pReader == NULL) {
                checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                "failed to encode BMP chunk into JPEG\n");
                delete pFragments;
                return NULL;
            }
            pFragment->pReader = pReader;
            pFragment->ui64FragLen = pReader->getBytesAvailable();
        }
        else if (outputChunkType == Chunker::JPEG2000) {
            #if defined (ANDROID)
                checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                "JPEG2000 not supported on the Android platform\n");
                delete pFragments;
                return NULL;
            #else
                BufferWriter bw (pBMPChunk->getTotalSize(), 1024);
                if (0 != (rc = pBMPChunk->writeHeaderAndImage (&bw))) {
                    checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                    "failed to write chunk BMP into buffer for creating JPEG2000; rc = %d\n", rc);
                    delete pFragments;
                    return NULL;
                }
                BufferReader *pReader = JasperWrapper::convertToJPEG2000 (bw.getBuffer(), bw.getBufferLength());
                if (pReader == NULL) {
                    checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                                    "failed to encode BMP chunk into JPEG2000\n");
                    delete pFragments;
                    return NULL;
                }
                pFragment->pReader = pReader;
                pFragment->ui64FragLen = pReader->getBytesAvailable();
            #endif
        }
        else {
            checkAndLogMsg ("Chunker::fragmentBuffer", Logger::L_MildError,
                            "unsupported output type %d\n", (int) outputChunkType);
            return NULL;
        }
        pFragment->out_type = outputChunkType;
        pFragment->ui8Part = ui8CurrentChunk;
        pFragment->ui8TotParts = ui8NoOfChunks;
        pFragments->append (pFragment);
    }
    return pFragments;
}
