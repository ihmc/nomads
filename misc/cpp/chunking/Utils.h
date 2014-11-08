/*
 * Utils.h
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
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on January 30, 2012, 6:25 PM
 */

#ifndef INCL_UTILS_H
#define	INCL_UTILS_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class BufferWriter;
    class Reader;
}

namespace IHMC_MISC
{
    class Utils
    {
        public:
            static unsigned int getPadding (unsigned int uiLength, uint8 ui8NChunks);

            static char * getMD5Checksum (NOMADSUtil::Reader *pReader);
            static char * getMD5Checksum (NOMADSUtil::BufferWriter *pWriter);
            static char * getMD5Checksum (const void *pBuf, uint32 ui32Len);
            static char * getMD5Checksum (void *pBuf, uint32 ui32Len);

            /**
             * Dumps content of buffer pBuf into file pszOutFileName
             */
            static void dump (const char *pszOutFileName, const void *pBuf, uint64 ui64Len);
    };
}

#endif	// INCL_UTILS_H

