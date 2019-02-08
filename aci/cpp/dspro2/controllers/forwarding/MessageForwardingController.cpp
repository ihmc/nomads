/*
 * MessageForwardingController.cpp
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

#include "MessageForwardingController.h"

#include "CommAdaptorManager.h"
#include "DataStore.h"
#include "Defs.h"
#include "Topology.h"
#include "WaypointMessageHelper.h"

#include "Searches.h"
#include "SearchProperties.h"

#include "ConfigManager.h"
#include "FTypes.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    /*
     * Returns true if the node caches chunks that the remote node is missing,
     * false otherwise.
     */
    bool hasMoreChunks (DArray<uint8> *pLocallyCachedChunkIds, DArray<uint8> *pPeersCachedChunkIds)
    {
        if (pLocallyCachedChunkIds == nullptr) {
            return false;
        }
        if (pPeersCachedChunkIds == nullptr || pPeersCachedChunkIds->size() == 0) {
            return (pLocallyCachedChunkIds->size() > 0);
        }

        for (unsigned int i = 0; i < pLocallyCachedChunkIds->size(); i++) {
            bool bFound = false;
            for (unsigned int j = 0; j < pPeersCachedChunkIds->size(); j++) {
                if ((*pLocallyCachedChunkIds)[i] == (*pPeersCachedChunkIds)[j]) {
                    bFound = true;
                    break;
                }
            }
            if (!bFound) {
                return true;
            }
        }
        return false;
    }

    String getId (const char *pszMsgId, DArray<uint8> *pCachedChunks)
    {
        String id (pszMsgId);
        if (pCachedChunks != nullptr) {
            char buf[4];
            buf[3] = '\0';

            for (unsigned int i = 0; i < pCachedChunks->size(); i++) {
                int iChunkId = ((int) (*pCachedChunks)[i]);
                sprintf (buf, "%03d", iChunkId);
                id += buf;
            }
        }

        return id;
    }
}

MessageForwardingController::MessageForwardingController (const char *pszNodeId, CommAdaptorManager *pAdaptMgr,
                                                          DataStore *pDataStore, Topology *pTopology,
                                                          bool bContextForwardingEnabled)
    : _bContextForwardingEnabled (bContextForwardingEnabled),
      _nodeId (pszNodeId),
      _pAdaptMgr (pAdaptMgr),
      _pDataStore (pDataStore),
      _pTopology (pTopology),
      _recentlyRequestedMessages (3000)
{
}

MessageForwardingController::~MessageForwardingController()
{
}

int MessageForwardingController::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == nullptr) {
        return -1;
    }
    _bContextForwardingEnabled = pCfgMgr->getValueAsBool ("aci.dspro.dsprorepctrl.contextForwarding.enabled", true);
    checkAndLogMsg ("MessageForwardingController::init", Logger::L_Info, "context forwarding %s.\n",
                    _bContextForwardingEnabled ? "enabled" : "disabled");
    return 0;
}

int MessageForwardingController::messageRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                               const char *pszPublisherNodeId, const char *pszMsgId)
{
    const char *pszMethodName = "MessageForwardingController::messageRequestMessageArrived";

    if (pszMsgId == nullptr) {
        return -1;
    }

    String id = getId (pszMsgId, nullptr);

    if (!_bContextForwardingEnabled || _recentlyRequestedMessages.containsKey (id) || _pDataStore->hasData (pszMsgId)) {
        // Forwarding disabled, or no need to forward
        return 1;
    }

    Targets **ppTargets = _pTopology->getForwardingTargets (_nodeId, pszSenderNodeId);
    if (ppTargets == nullptr) {
        return 2;
    }
    if (ppTargets[0] == nullptr) {
        Targets::deallocateTargets (ppTargets);
        return 3;
    }

    // Forward
    int rc = _pAdaptMgr->sendMessageRequestMessage (pszMsgId, pszPublisherNodeId, ppTargets);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed to forward message request message "
                        "for %s, from %s and originated from %s. Return code %d.\n", pszMsgId, pszSenderNodeId,
                        pszPublisherNodeId, rc);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "message request message for %s, from %s and "
                        "originated from %s was forwarded.\n", pszMsgId, pszSenderNodeId, pszPublisherNodeId);
        logTopology (pszMethodName, Logger::L_Info, "%s --> %s\n", pszPublisherNodeId, pszSenderNodeId);
        _recentlyRequestedMessages.put (id);
    }

    Targets::deallocateTargets (ppTargets);
    return rc;
}

int MessageForwardingController::chunkRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                             const char *pszPublisherNodeId, const char *pszMsgId,
                                                             DArray<uint8> *pCachedChunks)
{
    const char *pszMethodName = "MessageForwardingController::chunkRequestMessageArrived";

    if (pszMsgId == nullptr) {
        return -1;
    }

    String id = getId (pszMsgId, pCachedChunks);

    uint8 ui8TotalNumberOfChunks = 0;
    DArray<uint8> *pLocallyCachedChunkIds = _pDataStore->getCachedChunkIDs (pszMsgId, ui8TotalNumberOfChunks);

    if (!_bContextForwardingEnabled || _recentlyRequestedMessages.containsKey (id) || hasMoreChunks (pLocallyCachedChunkIds, pCachedChunks)) {
        // Forwarding disabled, or no need to forward
        delete pLocallyCachedChunkIds;
        return 1;
    }

    Targets **ppTargets = _pTopology->getForwardingTargets (_nodeId, pszSenderNodeId);
    if (ppTargets == nullptr) {
        delete pLocallyCachedChunkIds;
        return 2;
    }
    if (ppTargets[0] == nullptr) {
        delete pLocallyCachedChunkIds;
        Targets::deallocateTargets (ppTargets);
        return 3;
    }

    // Forward
    int rc = _pAdaptMgr->sendChunkRequestMessage (pszMsgId, pCachedChunks, pszPublisherNodeId, ppTargets);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed to forward chunk request message "
                        "for %s, from %s and originated from %s. Return code %d.\n", pszMsgId, pszSenderNodeId,
                        pszPublisherNodeId, rc);
    }
    else {
        String chunkIds ("[");
        if (pCachedChunks != nullptr) {
            for (unsigned int i = 0; i < pCachedChunks->size(); i++) {
                chunkIds += ((uint32) (*pCachedChunks)[i]);
                chunkIds += " ";
            }
        }
        chunkIds += "]";
        String myChunkIds ("[");
        if (pLocallyCachedChunkIds != nullptr) {
            for (unsigned int i = 0; i < pLocallyCachedChunkIds->size(); i++) {
                myChunkIds += ((uint32) (*pLocallyCachedChunkIds)[i]);
                myChunkIds += " ";
            }
        }
        myChunkIds += "]";
        checkAndLogMsg (pszMethodName, Logger::L_Info, "chunk request message for %s, from %s and originated "
                        "from %s was forwarded. The node owns the following chunks %s, while I own the following: %s\n", pszMsgId, pszSenderNodeId,
                        pszPublisherNodeId, chunkIds.c_str(), myChunkIds.c_str());
        logTopology (pszMethodName, Logger::L_Info, "%s --> %s\n", pszPublisherNodeId, pszSenderNodeId);
        _recentlyRequestedMessages.put (id);
    }

    delete pLocallyCachedChunkIds;
    Targets::deallocateTargets (ppTargets);
    return rc;
}

int MessageForwardingController::searchMessageArrived (unsigned int uiAdaptorId, const char *pszSenderNodeId,
                                                       SearchProperties *pSearchProperties)
{
    const char *pszMethodName = "MessageForwardingController::searchMessageArrived";
    if (Searches::getSearches()->hasSearchInfo (pSearchProperties->pszQueryId)) {
        // The search has already been received, no need to forward it
        return 1;
    }

    Targets **ppTargets = _pTopology->getForwardingTargets (_nodeId, pszSenderNodeId);
    if (ppTargets != nullptr) {
        return 2;
    }
    if (ppTargets[0] == nullptr) {
        Targets::deallocateTargets (ppTargets);
        return 3;
    }

    // Forward
    int rc = _pAdaptMgr->sendSearchMessage (*pSearchProperties, ppTargets);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "failed to forward search message from %s and originated from %s. Return code %d.\n",
                        pszSenderNodeId, pSearchProperties->pszQuerier, rc);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "search message from %s and originated from %s was forwarded.\n",
                        pszSenderNodeId, pSearchProperties->pszQuerier);
    }

    Targets::deallocateTargets (ppTargets);
    return rc;
}

int MessageForwardingController::searchReplyMessageArrived (AdaptorId, const char *pszSenderNodeId,
                                                            const char *pszQueryId, const char **ppszMatchingMsgIds,
                                                            const char *pszTarget, const char *pszMatchingNodeId)
{
    const char *pszMethodName = "MessageForwardingController::searchReplyMessageArrived";
    if (Searches::getSearches()->hasQueryReply (pszQueryId, pszMatchingNodeId)) {
        // The search reply for the peer has already been received, no need to forward it
        return 1;
    }

    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if (Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) < 0 || (querier.length() <= 0)) {
        return -1;
    }

    // Forward message
    int rc;
    if (querier.length() <= 0) {
        Targets **ppTargets = _pTopology->getForwardingTargets (_nodeId, pszSenderNodeId);
        if (ppTargets == nullptr) {
            return 2;
        }
        if (ppTargets[0] == nullptr) {
            Targets::deallocateTargets (ppTargets);
            return 3;
        }
        rc = _pAdaptMgr->sendSearchReplyMessage (pszQueryId, ppszMatchingMsgIds,
                                                 querier, pszMatchingNodeId,
                                                 ppTargets);
        Targets::deallocateTargets (ppTargets);
    }
    else {
        TargetPtr targets[2] = {_pTopology->getNextHopAsTarget (querier), nullptr};
        if (targets[0] == nullptr) {
            return 3;
        }
        rc = _pAdaptMgr->sendSearchReplyMessage (pszQueryId, ppszMatchingMsgIds,
                                                 querier.c_str(), pszMatchingNodeId,
                                                 targets);
        delete targets[0];
    }

    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "failed to forward search message "
                        "from %s and originated from %s. Return code %d.\n", pszSenderNodeId,
                        querier.c_str(), rc);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "search message from %s and originated "
                        "from %s was forwarded.\n", pszSenderNodeId, querier.c_str());
    }

    return rc;
}

int MessageForwardingController::topologyReplyMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                                              const void *pBuf, uint32 ui32Len)
{
    // Do not forward local topology
    return 0;
}

int MessageForwardingController::waypointMessageArrived (AdaptorId adaptorId, const char *pszSenderNodeId,
                                                         const char *pszPublisherNodeId, const void *pBuf,
                                                         uint32 ui32Len)
{
    const char *pszMethodName = "MessageForwardingController::waypointMessageArrived";
    if (!_bContextForwardingEnabled) {
        return 0;
    }

    // Waypoint messages are sent periodically and unreliably, therefore they always need to be forwarded
    int rc = 0;
    Targets **ppTargets = _pTopology->getForwardingTargets (_nodeId, pszSenderNodeId);
    if ((ppTargets != nullptr) && (ppTargets[0] != nullptr)) {
        uint32 ui32TotalLen = 0;
        static PreviousMessageIds LATEST_MESSAGE_SENT_TO_TARGETS_UNSET; // the latest message that was sent is only set between neighboring
                                                                        // nodes. Even if a node sends a message to a non-neighboring node
                                                                        // it is only the last hop before the target that has the last word
                                                                        // on whether sending the data message to the target or not, therefore
                                                                        // it is the only one that can know what was the laster message that was
                                                                        // sent to it
        void *pData = WaypointMessageHelper::writeWaypointMessageForTarget (LATEST_MESSAGE_SENT_TO_TARGETS_UNSET, pBuf, ui32Len, ui32TotalLen);
        if (pData != nullptr) {
            rc = _pAdaptMgr->sendWaypointMessage (pData, ui32TotalLen, pszPublisherNodeId, ppTargets);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning,  "failed to forward waypoint message from %s and "
                                "originated from %s. Return code %d.\n", pszSenderNodeId, pszPublisherNodeId, rc);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "waypoint message from %s and originated "
                                "from %s was forwarded.\n", pszSenderNodeId, pszPublisherNodeId);
            }
            free (pData);
        }
    }
    Targets::deallocateTargets (ppTargets);
    return rc;
}

