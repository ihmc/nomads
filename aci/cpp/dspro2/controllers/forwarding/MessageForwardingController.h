/*
 * ForwardingController.h
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
 * Created on December 18, 2013, 16:11 AM
 */

#ifndef INCL_MESSAGE_FORWARDING_CONTROLLER_H
#define	INCL_MESSAGE_FORWARDING_CONTROLLER_H

#include "Defs.h"

#include "DArray2.h"
#include "StrClass.h"
#include "TimeBoundedStringHashset.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class CommAdaptorManager;
    class DataStore;
    struct SearchProperties;
    class Topology;

    class MessageForwardingController
    {
        public:
            MessageForwardingController (const char *pszNodeId, CommAdaptorManager *pAdaptMgr,
                                         DataStore *pDataStore, Topology *pTopology,
                                         bool bContextForwardingEnabled);
            ~MessageForwardingController (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            int messageRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                              const char *pszPublisherNodeId, const char *pszMsgId);
            int chunkRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                            const char *pszPublisherNodeId, const char *pszMsgId,
                                            NOMADSUtil::DArray<uint8> *pCachedChunks);
            int searchMessageArrived (unsigned int uiAdaptorId, const char *pszSenderNodeId,
                                      SearchProperties *pSearchProperties);
            int searchReplyMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                           const char *pszQueryId, const char **ppszMatchingMsgIds,
                                           const char *pszTarget, const char *pszMatchingNodeId);
            int topologyReplyMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                             const void *pBuf, uint32 ui32Len);

            int waypointMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                        const char *pszPublisherNodeId, const void *pBuf,
                                        uint32 ui32Len);

        private:
            bool _bContextForwardingEnabled;
            NOMADSUtil::String _nodeId;
            CommAdaptorManager *_pAdaptMgr;
            DataStore *_pDataStore;
            Topology *_pTopology;
            NOMADSUtil::TimeBoundedStringHashset _recentlyRequestedMessages;
    };
}

#endif    // INCL_MESSAGE_FORWARDING_CONTROLLER_H

