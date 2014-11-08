/*
 * ChunkingAdaptor.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * ChunkingAdaptor is a collection of utility methods
 * to handle chunks, and to convert the data structure
 * that describe the chunks, from/to the formats required
 * by the chunking library (in misc/), and DisService.
 *
 * Created on January 12, 2012, 2:44 PM
 */

#ifndef INCL_CHUNKING_ADAPTOR_H
#define	INCL_CHUNKING_ADAPTOR_H

#include "Message.h"

#include "Chunker.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Reader;
}

namespace IHMC_ACI
{
    class ChunkMsgInfo;

    class ChunkingConfiguration
    {
        public:
            ChunkingConfiguration (void);
            ~ChunkingConfiguration (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);
            uint8 getNumberofChunks (uint64 ui64Bytes);

        private:
            struct Range
            {
                Range (uint64 ui64Bytes, uint8 ui8NChunks);
                ~Range (void);

                bool operator > (const Range &range) const;
                bool operator < (const Range &range) const;
                bool operator == (const Range &range) const;

                uint8 _ui8NChunks;
                uint64 _ui64Bytes;
            };
            NOMADSUtil::PtrLList<Range> _sizeToNchunks;
    };

    class ChunkingAdaptor
    {
        public:
            /**
             * NOTE: the returned Reader should be deallocated by the caller.
             */
            static NOMADSUtil::Reader * reassemble (NOMADSUtil::PtrLList<Message> *pFragments, uint32 &ui32LargeObjLen);

            /**
             * Method to generate a ChunkMsgInfo, given am instance of
             * IHMC_MISC::Chunker::Fragment, and other parameters.
             *
             * NOTE: the returned ChunkMsgInfo should be deallocated by the caller.
             */
            static ChunkMsgInfo * toChunkMsgInfo (const char *pszGroupName, const char *pszSenderNodeId,
                                                  uint32 ui32MsgSeqId, const char *pszObjectId, const char *pszInstanceID,
                                                  uint16 ui16Tag, uint16 ui16ClientId, uint8 ui8ClientType,
                                                  const char *pszMimeType, const char *pszChecksum,
                                                  uint16 ui16HistoryWindow, uint8 ui8Priority, int64 i64ExpirationTime,
                                                  IHMC_MISC::Chunker::Fragment *pChunkFragment);

            /**
             * Creates a IHMC_MISC::Chunker::Fragment containing the specified data.
             *
             * NOTE: the returned IHMC_MISC::Chunker::Fragment should be deallocated by the caller.
             */
            static IHMC_MISC::Chunker::Fragment * getChunkerFragment (const void *pData, uint32 ui32DataLenght);
    };
}

#endif	/* INCL_CHUNKING_ADAPTOR_H */

