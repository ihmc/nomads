/*
 * ImageCodec.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#ifndef INCL_IMAGE_CODEC_H
#define	INCL_IMAGE_CODEC_H

#include "Chunker.h"

namespace NOMADSUtil
{
    class BufferReader;
    class BMPImage;
}

namespace IHMC_MISC
{
    class ImageCodec
    {
        public:
            static NOMADSUtil::BMPImage * decode (const void *pBuf, uint32 ui32Len, Chunker::Type inputObjectType);
            static NOMADSUtil::BufferReader * encode (const NOMADSUtil::BMPImage *pBMPImage, Chunker::Type outputChunkType,
                                                      uint8 ui8ChunkCompressionQuality);
            static bool supports (Chunker::Type type);
    };
}

#endif    // INCL_IMAGE_CODEC_H

