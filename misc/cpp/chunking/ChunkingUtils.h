/*
 * ChunkingUtils.h
 *
 * This file is part of the IHMC Chunking Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 *
 * Created on January 30, 2012, 6:25 PM
 */

#ifndef INCL_UTILS_H
#define	INCL_UTILS_H

#include "Chunker.h"

namespace NOMADSUtil
{
    class BMPImage;
    class BufferReader;
    class BufferWriter;
    class Reader;
}

namespace IHMC_MISC
{
    class ChunkingUtils
    {
        public:
            static int toDimension (uint8 ui8, Chunker::Dimension &dimension);
            static NOMADSUtil::BMPImage * toBMPImage (NOMADSUtil::BufferReader *pReader);
            static NOMADSUtil::BufferReader * toReader (const NOMADSUtil::BMPImage *pBMPImage);
            static NOMADSUtil::BufferReader * toReader (const char *pszFileName);

            static unsigned int getPadding (unsigned int uiLength, uint8 ui8NChunks);

            static char * getMD5Checksum (NOMADSUtil::Reader *pReader);
            static char * getMD5Checksum (NOMADSUtil::BufferWriter *pWriter);
            static char * getMD5Checksum (const void *pBuf, uint32 ui32Len);
            static char * getMD5Checksum (void *pBuf, uint32 ui32Len);

            /**
             * Dumps content of buffer pBuf into file pszOutFileName
             */
            static int dump (const char *pszOutFileName, const NOMADSUtil::BufferReader *pReader);
    };
}

#endif	// INCL_UTILS_H

