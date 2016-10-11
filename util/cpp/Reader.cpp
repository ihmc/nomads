/*
 * Reader.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include "Reader.h"

#include <stdlib.h>

using namespace NOMADSUtil;

int Reader::readBool (bool *pBuf)
{
    uint8 ui8;
    if (read8 (&ui8) < 0) {
        return -1;
    }
    switch (ui8) {
        case 0:
            *pBuf = false;
            return 0;

        case 1:
            *pBuf = true;
            return 0;

        default:
            return -2;
    }
}

int Reader::read8 (void *pBuf)
{
    if (readBytes (pBuf, 1)) {
        return -1;
    }
    return 0;
}

int Reader::read16 (void *pBuf)
{
    if (readBytes (pBuf, 2)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap16 (pBuf);
    #endif
    return 0;
}

int Reader::readLE16 (void *pBuf)
{
    if (readBytes (pBuf, 2)) {
        return -1;
    }
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap16 (pBuf);
    #endif
    return 0;
}

int Reader::read32 (void *pBuf)
{
    if (readBytes (pBuf, 4)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap32 (pBuf);
    #endif
    return 0;
}

int Reader::readLE32 (void *pBuf)
{
    if (readBytes (pBuf, 4)) {
        return -1;
    }
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap32 (pBuf);
    #endif
    return 0;
}

int Reader::read64 (void *pBuf)
{
    if (readBytes (pBuf, 8)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap64 (pBuf);
    #endif
    return 0;
}

int Reader::readLE64 (void *pBuf)
{
    if (readBytes (pBuf, 8)) {
        return -1;
    }
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap64 (pBuf);
    #endif
    return 0;
}

int Reader::readBytes (void *pBuf, uint32 ui32Count)
{
    int rc;
    uint32 ui32BytesRead = 0;
    while (ui32BytesRead < ui32Count) {
        rc = read (((int8*)pBuf)+ui32BytesRead, ui32Count - ui32BytesRead);
        if (rc <= 0) {
            return -1;
        }
        ui32BytesRead += (uint32) rc;
    }
    return 0;
}

int Reader::readUI8 (void *pBuf)
{
    return read8 (pBuf);
}

int Reader::readUI16 (void *pBuf)
{
    return read16 (pBuf);
}

int Reader::readUI32 (void *pBuf)
{
    return read32 (pBuf);
}

int Reader::readUI64 (void *pBuf)
{
    return read64 (pBuf);
}
           
int Reader::readString (char **pBuf)
{
    uint32 ui32Len = 0;
    if (readUI32 (&ui32Len) < 0) {
        return -1;
    }
    if (ui32Len > 0) {
        *pBuf = static_cast<char *>(calloc (ui32Len + 1, sizeof (char)));
        if (readBytes (*pBuf, ui32Len) < 0) {
            free (*pBuf);
            *pBuf = NULL;
            return -2;
        }
        (*pBuf)[ui32Len] = '\0';
    }
    else {
        *pBuf = NULL;
    }
    return 0;
}

int Reader::close (void)
{
    return -1;
}

void Reader::byteSwap16 (void *pBuf)
{
    unsigned char uchByte;
    uchByte = ((unsigned char *) pBuf) [0];
    ((unsigned char *) pBuf) [0] = ((unsigned char *) pBuf) [1];
    ((unsigned char *) pBuf) [1] = uchByte;
}

void Reader::byteSwap32 (void *pBuf)
{
    unsigned char uchByte;
    uchByte = ((unsigned char *) pBuf) [0];
    ((unsigned char *) pBuf) [0] = ((unsigned char *) pBuf) [3];
    ((unsigned char *) pBuf) [3] = uchByte;
    uchByte = ((unsigned char *) pBuf) [1];
    ((unsigned char *) pBuf) [1] = ((unsigned char *) pBuf) [2];
    ((unsigned char *) pBuf) [2] = uchByte;
}

void Reader::byteSwap64 (void *pBuf)
{
    unsigned char uchByte;
    uchByte = ((unsigned char *) pBuf) [0];
    ((unsigned char *) pBuf) [0] = ((unsigned char *) pBuf) [7];
    ((unsigned char *) pBuf) [7] = uchByte;
    uchByte = ((unsigned char *) pBuf) [1];
    ((unsigned char *) pBuf) [1] = ((unsigned char *) pBuf) [6];
    ((unsigned char *) pBuf) [6] = uchByte;
    uchByte = ((unsigned char *) pBuf) [2];
    ((unsigned char *) pBuf) [2] = ((unsigned char *) pBuf) [5];
    ((unsigned char *) pBuf) [5] = uchByte;
    uchByte = ((unsigned char *) pBuf) [3];
    ((unsigned char *) pBuf) [3] = ((unsigned char *) pBuf) [4];
    ((unsigned char *) pBuf) [4] = uchByte;
}
