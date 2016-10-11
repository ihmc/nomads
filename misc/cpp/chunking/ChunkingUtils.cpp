/*
 * Utils.cpp
 *
 * This file is part of the IHMC Chunking Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "ChunkingUtils.h"

#include "BMPImage.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "MD5.h"

#include "File.h"
#include "FileReader.h"
#include "FileUtils.h"
#include "Logger.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_MISC;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

int ChunkingUtils::toDimension (uint8 ui8, Chunker::Dimension &dimension)
{
    switch (ui8) {
        case Chunker::X:
            dimension = Chunker::X;
            return 0;
        case Chunker::Y:
            dimension = Chunker::Y;
            return 0;
        case Chunker::T:
            dimension = Chunker::T;
            return 0;
        default:
            return -1;
    }
}

NOMADSUtil::BMPImage * ChunkingUtils::toBMPImage (BufferReader *pReader)
{
    if (pReader == NULL){
        return NULL;
    }
    int rc = 0;
    BMPImage *pBMPImage = new BMPImage (true);
    if (0 != (rc = pBMPImage->readHeaderAndImage(pReader))) {
        checkAndLogMsg ("ChunkingUtils::toBMPImage", Logger::L_MildError,
                        "failed to read BMP image from buffer; rc = %d\n", rc);
        delete pBMPImage;
        pBMPImage = NULL;
    }
    return pBMPImage;
}

BufferReader * ChunkingUtils::toReader (const BMPImage *pBMPImage)
{
    BufferWriter bw (pBMPImage->getTotalSize (), 1024);
    int rc = pBMPImage->writeHeaderAndImage (&bw);
    if (0 != rc) {
        checkAndLogMsg ("ChunkingUtils::toReader", Logger::L_MildError, "failed to write chunk BMP into buffer; rc = %d\n", rc);
        return NULL;
    }
    const uint32 ui32ChunkLen = bw.getBufferLength ();
    return new BufferReader (bw.relinquishBuffer (), ui32ChunkLen, true);
}

NOMADSUtil::BufferReader * ChunkingUtils::toReader (const char *pszFileName)
{
    if (pszFileName == NULL) {
        return NULL;
    }
    File file (pszFileName);
    if (!file.exists ()) {
        return NULL;
    }
    FileReader fr (pszFileName, "rb");
    const int64 i64FileSize = file.getFileSize();
    if (i64FileSize > 0xFFFFFFFF) {
        return NULL;
    }
    const uint32 ui32Len = static_cast<uint32>(i64FileSize);
    void *pBuf = malloc (ui32Len);
    if (pBuf == NULL) {
        return NULL;
    }
    if (fr.readBytes (pBuf, ui32Len) < 0) {
        free (pBuf);
        return NULL;
    }
    return new BufferReader (pBuf, ui32Len, true);
}

unsigned int ChunkingUtils::getPadding (unsigned int uiLength, uint8 ui8NChunks)
{
    unsigned int uiReminder = uiLength % ui8NChunks;
    unsigned int padding = 0;
    if (uiReminder > 0) {
        padding = ui8NChunks - uiReminder;
    }
    return padding;
}
        
char * ChunkingUtils::getMD5Checksum (Reader *pReader)
{
    uint32 ui32Len = pReader->getBytesAvailable();
    void *pBuf = malloc (ui32Len);
    pReader->readBytes (pBuf, ui32Len);
    return getMD5Checksum ((const void*)pBuf, ui32Len);
}

char * ChunkingUtils::getMD5Checksum (BufferWriter *pWriter)
{
    return getMD5Checksum (pWriter->getBuffer(), pWriter->getBufferLength());
}

char * ChunkingUtils::getMD5Checksum (const void *pBuf, uint32 ui32Len)
{
    void *pBufCpy = malloc (ui32Len);
    memcpy (pBufCpy, pBuf, ui32Len);
    char *pszChecksum = getMD5Checksum (pBufCpy, ui32Len);
    free (pBufCpy);
    return pszChecksum;
}

char * ChunkingUtils::getMD5Checksum (void *pBuf, uint32 ui32Len)
{
    MD5 md5;
    md5.init();
    md5.update (pBuf, ui32Len);
    return md5.getChecksumAsString();
}

int ChunkingUtils::dump (const char *pszOutFileName, const NOMADSUtil::BufferReader *pReader)
{
    return FileUtils::dumpBufferToFile (pReader->getBuffer(), pReader->getBufferLength(), pszOutFileName);
}

