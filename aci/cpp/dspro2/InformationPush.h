/*
 * InformationPush.h
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
 */

#ifndef INCL_INFORMATION_PUSH_H
#define INCL_INFORMATION_PUSH_H

#include "MatchmakingIntrumentation.h"
#include "MetadataInterface.h"
#include "PeerNodeContext.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Writer;
}

namespace IHMC_VOI
{
    struct MetadataRankerLocalConfiguration;
}

namespace IHMC_VOI
{
    class Voi;
}

namespace IHMC_ACI
{
    class InformationPushPolicy;
    class MetaData;
    class NodeContextManager;
    class Scheduler;

    class InformationPush
    {
        public:
            InformationPush (const char *pszNodeId, IHMC_VOI::Voi *pVoi,
                             IHMC_VOI::MetadataRankerLocalConfiguration *pMetaDataRankerLocalConf,
                             NodeContextManager *pNodeContextManager, InformationPushPolicy *pPolicy,
                             Scheduler *pScheduler);
            virtual ~InformationPush (void);

            /**
             * The second function let the caller set a list (nullptr terminated)
             * of node IDs which peer node context should not be returned.
             * Since dataArrived functions rank each peer node context, using the
             * latter function saves computational resources.
             *
             * NOTE: ppInstrumentations is being allocated in this function and
             *       MUST be deallocated by the caller
             */
            Instrumentations * dataArrived (MetaData *pMetaData, PeerNodeContextList *pPeerNodeContextList);

            /**
             * A peer node has sent an update of its node context.
             * Returns the list of messageIDs to send to the peer who sent the
             * update.
             *
             * - ppszFilters: the message IDs that MUST NOT be returned
             *
             * NOTE: ppInstrumentations is being allocated in this function and
             *       MUST be deallocated by the caller
             */
            Instrumentations * nodeContextChanged (IHMC_VOI::MetadataList *pMetadataList, PeerNodeContext *pPeerContext);

        private:
            void addToScheduler (const char *pszTargetPeerNodeId, char *pszMsgId,
                                 float fPrimaryIndex, float fSecondaryIndex);
            void addToScheduler (const char *pszTargetPeerNodeID, NOMADSUtil::PtrLList<IHMC_VOI::Rank> *pRanks);

            MatchmakingIntrumentation * createInstrumentation (IHMC_VOI::Rank *pRank, bool bSkipped, float fMatchThreashold);

        private:
            const NOMADSUtil::String _nodeId;
            NodeContextManager *_pNodeContextManager;
            InformationPushPolicy *_pPolicy;
            Scheduler *_pScheduler;
            IHMC_VOI::MetadataRankerLocalConfiguration *_pMetaDataRankerLocalConf;
            IHMC_VOI::Voi *_pVoi;
    };
}

#endif // INCL_INFORMATION_PUSH_H
