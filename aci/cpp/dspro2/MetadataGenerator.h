/**
 * MetadataGenerator.h
 *
 * MetadataGenerator provides methods to create a dspro metadata message from
 * another dspro metadata.
 *
 * The new dspro metadata message will be stored in DisService and dspro's caches.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on November 7, 2011, 5:13 PM
 */

#ifndef INCL_METADATA_MUTATOR_H
#define INCL_METADATA_MUTATOR_H

#include "PreviousMessageIds.h"
#include "RankByTargetMap.h"

namespace IHMC_VOI
{
    class MetadataInterface;
}

namespace IHMC_ACI
{
    class DSProImpl;
    class InformationStore;

    struct GenMetadataWrapper
    {
        GenMetadataWrapper (void);
        ~GenMetadataWrapper (void);

        NOMADSUtil::String msgId;
        IHMC_VOI::MetadataInterface *pMetadata;
    };

    class MetadataGenerator
    {
        public:
            MetadataGenerator (DSProImpl *pDspro, InformationStore *pInfoStore);
            virtual ~MetadataGenerator (void);

            /**
             * Modifies the metadata. Being the metadata immutable, it generates
             * a new metadata and stores it in the caches.
             * It returns the ID of the newly created message.
             *
             * NOTE: it makes a copy of pszPreviousMsgID
             * NOTE: the caller should deallcate the returned
             */
            GenMetadataWrapper * addPreviousMessageValue (const char *pszBaseMetadataMsgID,
                                                          PreviousMessageIds &prevMsgIds,
                                                          IHMC_VOI::RankByTargetMap &ranksByTarget);

        private:
            GenMetadataWrapper * addPreviousMessageValueInternal (IHMC_VOI::MetadataInterface *pBaseMetadata,
                                                                  const char *pszBaseMetadataMsgID,
                                                                  PreviousMessageIds &prevMsgIds,
                                                                  IHMC_VOI::RankByTargetMap &ranksByTarget);
            DSProImpl *_pDSPro;
            InformationStore *_pInfoStore;
    };
}

#endif    // INCL_METADATA_MUTATOR_H
