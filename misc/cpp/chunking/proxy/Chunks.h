/*
 * Chunks.h
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
 */

#ifndef INCL_CHUNKS_H
#define	INCL_CHUNKS_H

#include "BufferReader.h"
#include "DArray2.h"

namespace NOMADSUtil
{
    class Writer;
}

namespace IHMC_MISC
{
    class Chunks : public NOMADSUtil::DArray2<NOMADSUtil::BufferReader>
    {
        public:
            Chunks (bool bDeallocatedBuffers);
            Chunks (bool bDeallocatedBuffers, unsigned int uiSize);
            Chunks (bool bDeallocatedBuffers, NOMADSUtil::DArray2<NOMADSUtil::BufferReader> &SourceArray);

            int read (NOMADSUtil::Reader *pReader);
            int write (NOMADSUtil::Writer *pWriter);

        private:
            const bool _bDeallocatedBuffers;
    };
}

#endif  /* INCL_CHUNKS_H */

