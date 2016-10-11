/*
 * ChunkQuery.h
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
 */

#ifndef INCL_CHUNK_QUERY_H
#define INCL_CHUNK_QUERY_H

#include "ChunkingAdaptor.h"

#include  "StrClass.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class ChunkQuery
    {
        public:
            ChunkQuery (void);
            ChunkQuery (const char *pszBaseObjMsgId, IHMC_MISC::Chunker::Type inputType,
                        IHMC_MISC::Chunker::Type outputType, uint8 ui8CompressionQuality);
            ~ChunkQuery (void);

            int addInterval (IHMC_MISC::Chunker::Interval &interval);

            int read (NOMADSUtil::Reader *pReader);
            int write (NOMADSUtil::Writer *pWriter);

        public:
             static const NOMADSUtil::String GROUP;
             static const NOMADSUtil::String QUERY_TYPE;

             uint8 _ui8CompressionQuality;
             CustomChunkDescription _chunkDescr;
             NOMADSUtil::String _objectMsgId;
    };
}

#endif // INCL_CHUNK_QUERY_H

