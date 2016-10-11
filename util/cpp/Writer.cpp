/*
 * Writer.cpp
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

#include "Writer.h"

#include <string.h>

using namespace NOMADSUtil;

int Writer::writeBool (const bool *pbVal)
{
    if (pbVal == NULL) {
        return -1;
    }
    uint8 ui8Val = (*pbVal ? 1 : 0);
    return write8 (&ui8Val);
}

int Writer::write8 (const void *pBuf)
{
    if (writeBytes (pBuf, 1)) {
        return -1;
    }
    return 0;
}

int Writer::write16 (void *pBuf)
{
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap16 (pBuf);
    #endif
    if (writeBytes (pBuf, 2)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap16 (pBuf);
    #endif
    return 0;
}

int Writer::writeLE16 (void *pBuf)
{
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap16 (pBuf);
    #endif
    if (writeBytes (pBuf, 2)) {
        return -1;
    }
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap16 (pBuf);
    #endif
    return 0;
}

int Writer::write32 (void *pBuf)
{
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap32 (pBuf);
    #endif
    if (writeBytes (pBuf, 4)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap32 (pBuf);
    #endif
    return 0;
}

int Writer::writeLE32 (void *pBuf)
{
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap32 (pBuf);
    #endif
    if (writeBytes (pBuf, 4)) {
        return -1;
    }
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap32 (pBuf);
    #endif
    return 0;
}

int Writer::write64 (void *pBuf)
{
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap64 (pBuf);
    #endif
    if (writeBytes (pBuf, 8)) {
        return -1;
    }
    #if defined (LITTLE_ENDIAN_SYSTEM)
        byteSwap64 (pBuf);
    #endif
    return 0;
}

int Writer::writeLE64 (void *pBuf)
{
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap64 (pBuf);
    #endif
    if (writeBytes (pBuf, 8)) {
        return -1;
    }
    #if defined (BIG_ENDIAN_SYSTEM)
        byteSwap64 (pBuf);
    #endif
    return 0;
}

int Writer::writeUI8 (void *pBuf)
{
    return write8 (pBuf);
}

int Writer::writeUI16 (void *pBuf)
{
    return write16 (pBuf);
}

int Writer::writeUI32 (void *pBuf)
{
    return write32 (pBuf);
}

int Writer::writeUI64 (void *pBuf)
{
    return write64 (pBuf);
}

int Writer::writeString (const char *pBuf)
{
    uint32 ui32Len = (pBuf == NULL ? 0U : strlen (pBuf));
    if (writeUI32 (&ui32Len) < 0) {
        return -1;
    }
    if ((ui32Len > 0) && (writeBytes (pBuf, ui32Len) < 0)) {
        return -2;
    }
    return 0;
}

int Writer::close (void)
{
    return -1;
}
