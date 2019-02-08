/*
 * IncomingMessageForwardingPolicy.cpp
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
 * Created on June 20, 2013, 2:40 PM
 */

#include "MessageForwardingPolicy.h"

#include "MetaData.h"
#include "NodeContext.h"
#include "Targets.h"
#include "Topology.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MessageForwardingPolicy::MessageForwardingPolicy (const char *pszNodeId)
    : _nodeId (pszNodeId)
{
}

MessageForwardingPolicy::~MessageForwardingPolicy()
{
}

PeerNodeContextList * MessageForwardingPolicy::getNodesToMatch (PeerNodeContextList *pPeerNodeContextList,
                                                                const char *pszMsgSource, Topology *pTopology,
                                                                const char *, MetaData *)
{
    if (pPeerNodeContextList == nullptr || pTopology == nullptr) {
        checkAndLogMsg ("MessageForwardingPolicy::getNodesToMatch", Logger::L_Warning,
                        "one of the parameter is nullptr\n");
        return nullptr;
    }
    Targets **ppFwdTargets = (pszMsgSource == nullptr ? pTopology->getNeighborsAsTargets() :
                                                     pTopology->getForwardingTargets (_nodeId.c_str(), pszMsgSource));
    if (ppFwdTargets == nullptr) {
        // if there aren't forwarding targets, the previous hop and the current
        // node share all their neighbors, therefore the current node does not
        // need to do matchmaking for any of them
        return nullptr;
    }

    // Neighboring peers that are also forwarding nodes need to be matched
    PeerNodeContextList *pNodesToMatch = new PeerNodeContextList();
    if (pNodesToMatch == nullptr) {
        return nullptr;
    }
    for (PeerNodeContext *pNodeContext = pPeerNodeContextList->getFirst();
         pNodeContext != nullptr; pNodeContext = pPeerNodeContextList->getNext()) {
        const String nodeId (pNodeContext->getNodeId());
        if (pNodeContext->isPeerActive() && pTopology->isNeighbor (nodeId) &&
            isForwardingTarget (ppFwdTargets, nodeId)) {

            pNodesToMatch->prepend (pNodeContext);
        }
    }

    // Non-neighboring peers that are reachable through forwarding peers should be matched
    PeerNodeContextList *pNonNeighboringNodesToMatch = new PeerNodeContextList();
    if (pNonNeighboringNodesToMatch != nullptr) {
        for (PeerNodeContext *pNodeContext = pPeerNodeContextList->getFirst();
             pNodeContext != nullptr; pNodeContext = pPeerNodeContextList->getNext()) {
            const String nodeId (pNodeContext->getNodeId());
            if (!pTopology->isNeighbor (nodeId)) {
                bool bReachableThroughForwardingTarget = false;
                TargetPtr targets[2] = {pTopology->getNextHopAsTarget (nodeId), nullptr};
                for (unsigned int i = 0; targets[i] != nullptr && !bReachableThroughForwardingTarget; i++) {
                    for (unsigned int j = 0; j < targets[i]->aTargetNodeIds.size() && !bReachableThroughForwardingTarget; j++) {
                        if (isActiveForwardingTarget (pNodesToMatch, targets[i]->aTargetNodeIds[j])) {
                            pNonNeighboringNodesToMatch->prepend (pNodeContext);
                            bReachableThroughForwardingTarget = true;
                        }
                    }
                }
                delete targets[0];
            }
        }

        // Move pNonNeighboringNodesToMatch into pNodesToMatch
        for (PeerNodeContext *pNodeContext = pNonNeighboringNodesToMatch->getFirst();
             pNodeContext != nullptr; pNodeContext = pNonNeighboringNodesToMatch->getNext()) {
            pNodesToMatch->prepend (pNodeContext);
        }
        delete pNonNeighboringNodesToMatch;
        pNonNeighboringNodesToMatch = nullptr;
    }

    Targets::deallocateTargets (ppFwdTargets);
    ppFwdTargets = nullptr;
    if (pNodesToMatch->getFirst() == nullptr) {
        delete pNodesToMatch;
        pNodesToMatch = nullptr;
    }
    return pNodesToMatch;
}

bool MessageForwardingPolicy::isForwardingTarget (Targets **ppTargets, const char *pszNodeId)
{
    if (ppTargets == nullptr || pszNodeId == nullptr) {
        return false;
    }

    for (unsigned int i = 0; ppTargets[i] != nullptr; i++) {
        for (unsigned int j = 0; j < ppTargets[i]->aTargetNodeIds.size(); j++) {
            if (ppTargets[i]->aTargetNodeIds[j] != nullptr &&
                strcmp (ppTargets[i]->aTargetNodeIds[j], pszNodeId) == 0) {
                return true;
            }
        }
    }

    return false;
}

bool MessageForwardingPolicy::isActiveForwardingTarget (PeerNodeContextList *pActiveForwardingPeers, const char *pszNodeId)
{
    if (pActiveForwardingPeers == nullptr || pszNodeId == nullptr) {
        return false;
    }

    for (PeerNodeContext *pNodeContext = pActiveForwardingPeers->getFirst(); pNodeContext != nullptr;
         pNodeContext = pActiveForwardingPeers->getNext()) {
        String activeFwdPeerNodeId (pNodeContext->getNodeId());
        if (activeFwdPeerNodeId == pszNodeId) {
            return true;
        }
    }
    return false;
}

