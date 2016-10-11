/* 
 * DSLib.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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

#include "DSLib.h"

#include "BufferReader.h"
#include "FileReader.h"
#include "FileUtils.h"
#include "Writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
    #define snprintf _snprintf
#endif

using namespace IHMC_ACI;
using namespace NOMADSUtil;
 
int readIntoString (Reader *pReader, uint32 ui32BufLen, char **ppszString);

int DSLib::uint32ToString (char *pszBuf, int iBufLen, uint32 ui32Val)
{
    snprintf (pszBuf, iBufLen, "%u", ui32Val);
    return 0;
}

int DSLib::floatToString (char *pszBuf, int iBufLen, float fVal)
{
    snprintf (pszBuf, iBufLen, "%f", fVal);
    return 0;
}

int DSLib::readBool (Reader *pReader, uint32 ui32MaxSize, bool &bValue)
{
    if (ui32MaxSize < 1) {
        return -1;
    }
    uint8 ui8;
    if (pReader->read8 (&ui8) < 0) {
        return -2;
    }
    bValue = (ui8 == 1 ? true : false);
    return 0;
}

int DSLib::writeBool (Writer *pWriter, uint32 ui32MaxSize, bool bValue)
{
    if (ui32MaxSize < 1) {
        return -1;
    }
    uint8 ui8 = (bValue ? 1 : 0);
    if (pWriter->write8 (&ui8) < 0) {
        return -2;
    }
    return 0;
}

int DSLib::readString (Reader *pReader, uint32 ui32MaxSize, char *&pszStr, uint32 &ui32BytesRead)
{
    ui32BytesRead = 0;
    uint16 ui16Len;
    if (((uint32)2) > ui32MaxSize) {
        return -1;
    }
    if (pReader->read16 (&ui16Len) < 0) {
        return -2;
    }
    ui32BytesRead += 2;
    if (ui16Len > (uint16)0) {
        if ((ui32BytesRead + ui16Len) > ui32MaxSize) {
            return -3;
        }
        pszStr = (char *) calloc (sizeof (char), ui16Len + 1);
        if (pszStr != NULL && (pReader->readBytes (pszStr, ui16Len) >= 0)) {
            ui32BytesRead += ui16Len;
            return 0;
        }
    }
    return 0;
}

int DSLib::writeString (const char *pszStr, Writer *pWriter, uint32 ui32MaxSize, uint32 &ui32BytesWritten)
{
    return writeString (pszStr, pszStr == NULL ? 0 : strlen (pszStr), pWriter, ui32MaxSize, ui32BytesWritten);
}

int DSLib::writeString (const char *pszStr, uint16 ui16StrLen, Writer *pWriter,
                        uint32 ui32MaxSize, uint32 &ui32BytesWritten)
{
    ui32BytesWritten = 0;
    if ((pszStr == NULL) && (ui16StrLen > (uint16)0)) {
        return -1;
    }
    if (((uint16)(2 + ui16StrLen)) > ui32MaxSize) {
        return -2;
    }
    if (pWriter->write16 (&ui16StrLen) < 0) {
        return -3;
    }
    ui32BytesWritten += 2;
    if (ui16StrLen > 0) {
        if (pWriter->writeBytes (pszStr, ui16StrLen) < 0) {
            return -4;
        }
        ui32BytesWritten += ui16StrLen;
    }
    return 0;
}

int DSLib::readBufferIntoString (const void *pBuf, uint32 ui32BufLen, char **ppszString)
{
    if (pBuf == NULL || ui32BufLen == 0 || ppszString == NULL) {
        return -1;
    }
    *ppszString = NULL;
    BufferReader reader (pBuf, ui32BufLen);
    if (readIntoString (&reader, ui32BufLen, ppszString) < 0) {
        return -5;
    }

    return 0;
}

int DSLib::readFileIntoString (const char *pszFileName, char **ppszString)
{
    if (pszFileName == NULL || ppszString == NULL || ppszString == NULL) {
        return -1;
    }
     *ppszString = NULL;
    if (!FileUtils::fileExists (pszFileName)) {
        return -2;
    }
    int64 i64FileSize = FileUtils::fileSize (pszFileName);
    if (i64FileSize <= 0) {
        return -3;
    }
    FILE *pFile = fopen ((const char *) pszFileName, "r");
    if (pFile == NULL) {
        return -4;
    }
    FileReader reader (pFile, true);
    printf ("Reading file %s of length %lld\n", pszFileName, i64FileSize);
    if (readIntoString (&reader, (uint32) i64FileSize, ppszString) < 0) {
        return -5;
    }

    return 0;
}

int readIntoString (Reader *pReader, uint32 ui32BufLen, char **ppszString)
{
    if (pReader == NULL || ui32BufLen == 0 || ppszString == NULL) {
        return -1;
    }
    *ppszString = (char *) calloc (ui32BufLen + 1, sizeof (char));
    const int BUF_SIZE = 512;
    uint16 ui64ToRead;
    for (uint32 i = 0; i < ui32BufLen; i += ui64ToRead) {
        if ((ui64ToRead = (ui32BufLen - i)) > BUF_SIZE) {
            ui64ToRead = BUF_SIZE;
        }
        pReader->readBytes ((*ppszString)+i, (uint32)ui64ToRead);
    }
    (*ppszString)[ui32BufLen] = '\0';

    return 0;
}

