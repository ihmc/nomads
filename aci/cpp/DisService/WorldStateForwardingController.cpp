/*
 * WorldStateForwardingController.cpp
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
 */

#include "WorldStateForwardingController.h"

#include "DisseminationService.h"
#include "DisServiceMsg.h"
#include "MessageInfo.h"
#include "WorldState.h"

#include "PtrLList.h"
#include "StrClass.h"
#include "StringTokenizer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

WorldStateForwardingController::WorldStateForwardingController (DisseminationService *pDisService)
    : ForwardingController (FC_WS, pDisService)
{
}        

WorldStateForwardingController::~WorldStateForwardingController()
{
}

void WorldStateForwardingController::newNeighbor (const char *pszNodeUUID,
                                                  const char *pszPeerRemoteAddr,
                                                  const char *pszIncomingInterface)
{
}

void WorldStateForwardingController::deadNeighbor (const char *pszNodeUUID)
{
}

void WorldStateForwardingController::newLinkToNeighbor (const char *pszNodeUID,
                                                        const char *pszPeerRemoteAddr,
                                                        const char *pszIncomingInterface)
{
    
}

void WorldStateForwardingController::droppedLinkToNeighbor (const char *pszNodeUID,
                                                            const char *pszPeerRemoteAddr)
{
    
}

void WorldStateForwardingController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
    if (pUpdate != NULL && pUpdate->_type == PeerStateListener::SUBSCRIPTION_STATE) {
        DisServiceSubscriptionStateReqMsg msg (_pDisService->getNodeId(), pszNodeUID);
        broadcastCtrlMessage (&msg, "Sending DisServiceSubscriptionStateReqMsg");
    }
}

void WorldStateForwardingController::newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, DisServiceMsg *pDSMsg,
                                                         uint32 ui32SourceIPAddress, const char *pszIncomingInterface)
{
    if (pDSMsg->getType() != DisServiceMsg::DSMT_Data) {
        return;
    }
    DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg*)pDSMsg;     
    MessageHeader *pMH = pDSDMsg->getMessageHeader();
    const char *pszActualSenderNodeId = pDSDMsg->getSenderNodeId();
    const char *pForwardingTarget = NULL;       // Fix this

    // TODO: fix this!  It's unsafe!!!
    WorldState *pWS = (WorldState*) lockAndGetWorldState();
    //STATEFUL FORWARDING EVALUATION
    bool bFound = false;
    if (pForwardingTarget != NULL) {
        char separator = ':';
        StringTokenizer st (pForwardingTarget, separator);
        for (const char * pNodeId = st.getNextToken(); pNodeId && !bFound; pNodeId = st.getNextToken()) {
            if (0 == stricmp (pNodeId, _pDisService->getNodeId())) {
                bFound = true;
            }
        }
    }
    if (pForwardingTarget == NULL || bFound) {
        // If the message is for everyone or for me
        if (pWS->isDirectForwarding (pMH->getGroupName())) {
            // Direct forwarding
            // TODO
        }
        else {
            // Probabilistic forwarding
            const char *pTarget = pWS->getProbTarget (pszActualSenderNodeId);
            if (pTarget != NULL) {
                // Forward it
                pDSDMsg->setTargetNodeId (pTarget);
                broadcastDataMessage (pDSDMsg, "Forwarding message");
                delete[] pTarget;
            }
        }
    }
    else {
        // If the message is not for me but there is some isolated neighbors forward the message to them
        PtrLList<String> *pIsolatedNodes = pWS->getIsolatedNeighbors();
        if (pIsolatedNodes != NULL) {
            String target;
            for (String *pNodeId = pIsolatedNodes->getFirst(); pNodeId; pNodeId = pIsolatedNodes->getNext()) {
                if (target) {
                    target = target + ":" + pNodeId->c_str();
                }
                else {
                    target = pNodeId->c_str();
                }
                delete pNodeId;
            }
            pDSDMsg->setTargetNodeId ((const char *)target);
            broadcastDataMessage (pDSDMsg, "Forwarding message");
            delete pIsolatedNodes;
        }
    }
    releaseWorldState (pWS);
}

void WorldStateForwardingController::newSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
}

void WorldStateForwardingController::removedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
}

void WorldStateForwardingController::modifiedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
}

