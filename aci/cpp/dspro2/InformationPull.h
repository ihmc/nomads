/*
 * InformationPull.h
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

#ifndef INCL_INFORMATION_PULL
#define INCL_INFORMATION_PULL

#include "MatchmakingIntrumentation.h"

#include "DArray2.h"
#include "FTypes.h"
#include "PtrLList.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Writer;
}

namespace IHMC_VOI
{
    struct MetadataRankerLocalConfiguration;
    struct Rank;
}

namespace IHMC_ACI
{
    class InformationStore;
    class MetaData;
    class NodeContextManager;

    class InformationPull
    {
        public:
            static const float INFO_PULL_RANK_THRESHOLD;
            static const bool DEFAULT_ENFORCE_RANK_BY_TIME;

            static const char * RANK_THRESHOLD_PROPERTY;
            static const char * ENFORCE_TIMING_PROPERTY;
            static const char * LIMIT_PRESTAGING_TO_LOCAL_DATA_PROPERTY;

            InformationPull (const char *pszNodeId, IHMC_VOI::MetadataRankerLocalConfiguration *pMetaDataRankerLocalConf,
                             NodeContextManager *pNodeContextManager, InformationStore *pInformationStore,
                             float infoPullRankThreashold);
            virtual ~InformationPull (void);

            /**
             * Changes the configuration at run-time.
             */
            int configure (NOMADSUtil::ConfigManager *pConfigManager);

            /**
             * Returns true if the data is interesting for the node, returns
             * false otherwise.
             */
            MatchmakingIntrumentation * dataArrived (MetaData *pMetaData);

            /**
             * Returns the list of the peers to send the message to.
             */
            NOMADSUtil::DArray2<NOMADSUtil::String> * remoteSearchForMetadata (NOMADSUtil::Writer *pWriter, const char *pszGroupName, const char **ppszReferencedDataIDs);
            NOMADSUtil::DArray2<NOMADSUtil::String> * remoteSearchForMetadata (NOMADSUtil::Writer *pWriter, const char *pszGroupName, const char *pszReferencedDataID);
            NOMADSUtil::DArray2<NOMADSUtil::String> * remoteSearch (NOMADSUtil::Writer *pWriter, const char *pszGroupName, const char *pszQuery);

            /**
             * pData is the payload of the ControllerToController message, and
             * ui32DataLength is its length.
             * The method returns the list of the IDs of the messages matching
             * the query contained in pData.
             */
            NOMADSUtil::PtrLList<const char> * remoteSearchArrived (const void *pData, uint32 ui32DataLength,
                                                                    uint32 &ui32RcvdRemoteSeachQuery,
                                                                    const char *pszSenderNodeId);

        private:
            float _infoPullRankThreashold;
            uint32 _ui32RemoteSeachQuery;

            const NOMADSUtil::String _nodeId;
            NodeContextManager *_pNodeContextManager;
            InformationStore *_pInformationStore;
            IHMC_VOI::MetadataRankerLocalConfiguration *_pMetaDataRankerLocalConf;
            NOMADSUtil::StringHashtable<uint32> _latestSearchIdRcvdByPeer;
    };
}

#endif  // INCL_INFORMATION_PULL
