/*
 * DSLib.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on January 27, 2011, 7:30 PM
 */

#ifndef INCL_STRING_UTILS_H
#define INCL_STRING_UTILS_H

#include "FTypes.h"

#include "NLFLib.h"

#include <stdlib.h>

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class DSLib
    {
        public:
            static int uint32ToString (char *pszBuf, int iBufLen, uint32 ui32Val);
            static int floatToString (char *pszBuf, int iBufLen, float fVal);

            /**
             * Read/Write a \0-terminated string. The \0 character is not written.
             * The string is preceded by its length as a 2 byte value.
             *
             * Return 0 if the string was read/written correctly. A negative
             * number otherwise
             */
            static int readString (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize, char *&pszStr,
                                   uint32 &ui32BytesRead);
            static int writeString (const char *pszStr, NOMADSUtil::Writer *pWriter,
                                    uint32 ui32MaxSize, uint32 &ui32BytesWritten);
            static int writeString (const char *pszStr, uint16 ui16StrLen,
                                    NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize,
                                    uint32 &ui32BytesWritten);

            static int readBufferIntoString (const void *pBuf, uint32 ui32BufLen, char **ppszString);
            static int readFileIntoString (const char *pszFileName, char **ppszString);

            static uint8 toPriority (float fPriority);
    };

    inline uint8 DSLib::toPriority (float fPriority)
    {
        return (uint8) abs (NOMADSUtil::roundUpOrDown (fPriority));
    }
}

#endif    // INCL_STRING_UTILS_H
