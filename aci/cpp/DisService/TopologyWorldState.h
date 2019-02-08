/*
 * TopologyWorldState.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#ifndef INCL_TOPOLOGYWORLDSTATE_H
#define INCL_TOPOLOGYWORLDSTATE_H

#include "DisseminationService.h"
#include "ForwardingController.h"
#include "Listener.h"
#include "NodeInfo.h"
#include "Subscription.h"
#include "SubscriptionList.h"

#include "CRC.h"
#include "FTypes.h"
#include "StringHashgraph.h"
#include "PtrLList.h"
#include "StringHashtable.h"

#include "WorldState.h"
#include "ConfigFileReader.h"
#include "StringFloatHashtable.h"

namespace ForwardingStrategy {
    static const uint8 TOPOLOGY_FORWARDING = 0;
    static const uint8 PROBABILISTIC_FORWARDING = 1;
    static const uint8 STATEFUL_FORWARDING = 2;
    static const uint8 FLOODING_FORWARDING = 3;
}

namespace NOMADSUtil
{
    class InstrumentedWriter;
    class Reader;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class DisServiceWorldStateSeqIdMsg;
    class DisServiceWorldStateMsg;
    class MessageInfo;
    class RemoteNodeInfo;

    class TopologyWorldState : public WorldState
    {
        public:
            TopologyWorldState (DisseminationService *pDisService);
            virtual ~TopologyWorldState (void);

            /*
             * UTILITIES MODULE
             */
            void setParameters (float _fProbContact, float _fProbThreshold,
                                float _fAddParam, float _fAgeParam);
            void setSubscriptionsExchangeEnabled (bool bSubscriptionsExchangeEnabled);
            void setTopologyExchangeEnabled (bool bTopologyExchangeEnabled);

            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, DisServiceMsg *pDisServiceMsg,
                                     uint32 ui32SourceIPAddress, const char *pszIncomingInterfaces);
            NodeInfo * retrieveNodeInfo (const char *pszNodeId);
            int printWorldStateInfo (void);

            /*
             * SUBSCRIPTION MODULE
             */
            void incrementSubscriptionStateSeqId (void);
            int sendSubscriptionStateMsg (const char *pszNodeId);
            int sendSubscriptionStateMsg (IHMC_ACI::SubscriptionList *pSubscriptions, const char *pszNodeId, uint32 *pui32SeqId);
            int sendSubscriptionState (void);
            int sendSubscriptionStateReqMsg (DisServiceSubscriptionStateReqMsg *pSSReqMsg);
            int updateSubscriptionState (NOMADSUtil::StringHashtable<IHMC_ACI::SubscriptionList> *pSubscriptionsTable, NOMADSUtil::StringHashtable<uint32> *pNodesTable);

            /*
             * TOPOLOGY MODULE
             */
            int ageProbabilities (void);
            int sendProbabilities (void);
            int updateIndirectProbabilities (const char *pszNeighborNodeId, NOMADSUtil::StringHashtable<NOMADSUtil::StringFloatHashtable> *pProbabilitiesTable);

            /*
             * FORWARDING MODULE
             */
            uint8 getForwardingStrategy (DisServiceMsg *pDSDMsg);
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getInterestedRemoteNodes (DisServiceDataMsg *pDSDMsg);
            const char * getBestGateway (const char *pszTargetNodeId);
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getTargetNodes (DisServiceDataMsg *pDSDMsg);

        private:
            friend class ForwardingController;

            int getTimeSinceLastAging (void);
            NOMADSUtil::StringFloatHashtable * sendProbabilitiesInternal (RemoteNodeInfo *pRNI);

            bool _bSubscriptionsExchangeEnabled;
            bool _bTopologyExchangeEnabled;
            bool _bSendReq;
            uint64 _iLastAging;
            float _fProbContact;
            float _fProbThreshold;
            float _fAddParam;
            float _fAgeParam;
            DisServiceImprovedSubscriptionStateMsg *_pISSMsg;
            DisServiceProbabilitiesMsg *_pPMsg;
    };
}

#endif   // #ifndef INCL_TOPOLOGYWORLDSTATE_H
