/*
 * TopologyForwardingController.cpp
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

#include "TopologyForwardingController.h"

#include "DisseminationService.h"
#include "DisServiceMsg.h"
#include "MessageInfo.h"
#include "WorldState.h"
#include "TopologyWorldState.h"

#include "ConfigManager.h"

#include "PtrLList.h"
#include "StrClass.h"
#include "StringTokenizer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg 

TopologyForwardingController::TopologyForwardingController (DisseminationService *pDisService)
    : ForwardingController (FC_Default, pDisService), _msgHistory (DEFAULT_MESSAGE_HISTORY_DURATION)
{
    _ui16NumberOfActiveNeighbors = 0;
    init();
}

TopologyForwardingController::~TopologyForwardingController (void)
{
}

void TopologyForwardingController::init (void)
{
    _bForwardDataMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.dataMsgs", true);
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "forwarding for data messages %s\n", _bForwardDataMsgs ? "enabled" : "disabled");
    _bForwardDataRequestMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.dataReqMsgs", false);
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "forwarding for data request messages %s\n", _bForwardDataRequestMsgs ? "enabled" : "disabled");
    _bForwardChunkQueryMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.chunkQueryMsgs", false);
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "forwarding for chunk query messages %s\n", _bForwardChunkQueryMsgs ? "enabled" : "disabled");
    _bForwardChunkQueryHitsMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.chunkQueryHitsMsgs", false);
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "forwarding for chunk query hits messages %s\n", _bForwardChunkQueryHitsMsgs ? "enabled" : "disabled");
    _bForwardTargetedMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.targetedMsgs", true);
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "forwarding for targeted messages %s\n", _bForwardTargetedMsgs ? "enabled" : "disabled");
    _bForwardSearchMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.searchMsgs", true);
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "forwarding for search messages %s\n", _bForwardSearchMsgs ? "enabled" : "disabled");
    _bForwardSearchReplyMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.searchReplyMsgs", true);
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "forwarding for search reply messages %s\n", _bForwardSearchReplyMsgs ? "enabled" : "disabled");
    if (_pConfigManager->hasValue ("aci.disService.forwarding.historyWindowTime")) {
        _msgHistory.configure (_pConfigManager->getValueAsUInt32 ("aci.disService.forwarding.historyWindowTime"));
    }
    checkAndLogMsg ("TopologyForwardingController::init", Logger::L_Info,
                    "set history window time to %lu ms\n", _msgHistory.getStorageDuration());
}

void TopologyForwardingController::newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                                                const char *pszIncomingInterface)
{
    _ui16NumberOfActiveNeighbors++;
}

void TopologyForwardingController::deadNeighbor (const char *pszNodeUUID)
{
    if (_ui16NumberOfActiveNeighbors > 0) {
        _ui16NumberOfActiveNeighbors--;
    }
}

void TopologyForwardingController::newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                                      const char *pszIncomingInterface)
{
}

void TopologyForwardingController::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{
}

void TopologyForwardingController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

void TopologyForwardingController::newIncomingMessage (const void *, uint16, DisServiceMsg *pDSMsg,
                                                       uint32 ui32SourceIPAddress, const char *)
{
    if (pDSMsg->getType() == DisServiceMsg::DSMT_Data) {
        if (!_bForwardDataMsgs) {
            return;
        }
        else if (((DisServiceDataMsg *) pDSMsg)->doNotForward()) {
            return;
        }
    }
    else if (pDSMsg->getType() == DisServiceMsg::DSMT_DataReq) {
        if (!_bForwardDataRequestMsgs) {
            return;
        }
    }
    else if (pDSMsg->getType() == DisServiceMsg::CRMT_Query) {
        if (!_bForwardChunkQueryMsgs) {
            return;
        }
    }
    else if (pDSMsg->getType() == DisServiceMsg::CRMT_QueryHits) {
        if (!_bForwardChunkQueryHitsMsgs) {
            return;
        }
    }
    else if (pDSMsg->getType() == DisServiceMsg::DSMT_SearchMsg) {
        if (!_bForwardSearchMsgs) {
            return;
        }
    }
    else if (pDSMsg->getType() == DisServiceMsg::DSMT_SearchMsgReply) {
        if (!_bForwardSearchReplyMsgs) {
            return;
        }
    }
    else {
        // Do not want to forward anything except Data, DataRequest, Query, and QueryHits
        return;
    }
    if (pDSMsg->getTargetNodeId() != NULL) {
        if (!_bForwardTargetedMsgs) {
            return;
        }
    }
    
    if (_ui16NumberOfActiveNeighbors < 2) {
        // No need to forward - there are no other neighbors besides the one that sent the message
        return;
    }

    // Choose forwarding strategy
    TopologyWorldState *pWS = (TopologyWorldState *) lockAndGetWorldState(); // Retrieve TopologyWorldState
    switch (pWS->getForwardingStrategy (pDSMsg)) {
        case ForwardingStrategy::TOPOLOGY_FORWARDING: 
            doTopologyForwarding (pDSMsg);
            break;
        case ForwardingStrategy::STATEFUL_FORWARDING: 
            doStatefulForwarding (pDSMsg);
            break;
        case ForwardingStrategy::FLOODING_FORWARDING: 
            doFloodingForwarding (pDSMsg);
            break;
        case ForwardingStrategy::PROBABILISTIC_FORWARDING: 
            //TODO
            break;
    }
    releaseWorldState (pWS);
}

void TopologyForwardingController::doTopologyForwarding (DisServiceMsg *pDSMsg) 
{
    // Forwarding strategy that exploits TopologyWorldState information
    DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg*) pDSMsg;
    MessageHeader *pMH = pDSDMsg->getMessageHeader();
    const char *pszSenderNodeId = pDSDMsg->getSenderNodeId();
    if (pszSenderNodeId == NULL) { // Cannot handle this message
        return;
    }
    const char *pszTargetNodeId = pDSDMsg->getTargetNodeId();
    const char *pszMsgId = pMH->getMsgId();
    const char *pszPublisherNodeId = pMH->getPublisherNodeId();
    if (pszPublisherNodeId == NULL) { // Cannot handle this message
        return;
    }
    TopologyWorldState *pWS = (TopologyWorldState*) lockAndGetWorldState(); // Retrieve TopologyWorldState
    if (0 != stricmp (pszPublisherNodeId, _pDisService->getNodeId())) { // I'm not the publisher
        if (_msgHistory.put (pszMsgId)) {
            if (pszTargetNodeId == NULL || isMsgTargetNode (_pDisService->getNodeId(), pszTargetNodeId)) {
                PtrLList<String> *pTargetNodes = pWS->getTargetNodes (pDSDMsg); 
                if (pTargetNodes) {
                    String pTarget;
                    for (String *pNodeId = pTargetNodes->getFirst(); pNodeId; pNodeId = pTargetNodes->getNext()) {
                        if (0 != stricmp (pNodeId->c_str(), pszPublisherNodeId) && 0 != stricmp (pNodeId->c_str(), pszSenderNodeId) 
                            && !isMsgTargetNode (pNodeId->c_str(), pszTargetNodeId)) {
                            // Don't forward to publisher node, sender node or other nodes that were targets of the msg
                            if (pTarget) {
                                pTarget = pTarget + ":" + pNodeId->c_str();
                            } else {
                                pTarget = pNodeId->c_str();
                            }
                        }
                        delete pNodeId;
                    }
                    if (pTarget) {
                        pDSDMsg->setTargetNodeId (pTarget.c_str());
                        broadcastDataMessage (pDSDMsg, "Forwarding message");
                    }
                    delete pTargetNodes;
                }
            }
        }
    }
    releaseWorldState (pWS);
}

bool TopologyForwardingController::isMsgTargetNode (const char *pszNodeId, const char *pszTargetNodeId) 
{
    // Returns true if pszNodeId is contained in pszTargetNodeId
    bool bFound = false;
    if (pszTargetNodeId) {
        char separator = ':';
        StringTokenizer st (pszTargetNodeId, separator);
        for (const char * pszTarget = st.getNextToken(); pszTarget && !bFound; pszTarget = st.getNextToken()) {
            if (0 == stricmp (pszTarget, pszNodeId)) {
                bFound = true;
            }
        }
    }
    return bFound;
}

void TopologyForwardingController::doStatefulForwarding (DisServiceMsg *pDSMsg)
{
    if (pDSMsg->getType() == DisServiceMsg::DSMT_Data) { // This is a data msg
        DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg *) pDSMsg;
        MessageHeader *pMH = pDSDMsg->getMessageHeader();
        const char *pszMsgId = pMH->getMsgId();
        if (pszMsgId == NULL) {
            return;
        }
        const char *pszPublisherNodeId = pMH->getPublisherNodeId();
        if (pszPublisherNodeId == NULL) {
            return;
        }
        if (0 != stricmp (pszPublisherNodeId, _pDisService->getNodeId())) {
            if (_msgHistory.put (pszMsgId)) {
                float fProbability = 100.0f / _ui16NumberOfActiveNeighbors;
                if (fProbability < 10.0f) {
                    fProbability = 10.0f;
                }
                if ((rand() % 100) < fProbability) {
                    broadcastDataMessage (pDSDMsg, "forwarding data");
                }
            }
        }
    } else { // This is a control message
        DisServiceCtrlMsg *pDSCtrlMsg = (DisServiceCtrlMsg *) pDSMsg;
        const char *pszSenderNodeId = pDSCtrlMsg->getSenderNodeId();
        if (pszSenderNodeId == NULL) {
            return;
        }
        else if (0 != stricmp (pszSenderNodeId, _pDisService->getNodeId())) {
            char *pszMsgId = (char*) calloc (strlen (pszSenderNodeId) + 1 + 10 + 1, sizeof (char));
            sprintf (pszMsgId, "%s:%u", pszSenderNodeId, pDSCtrlMsg->getCtrlMsgSeqNo());
            if (_msgHistory.put (pszMsgId)) {
                float fProbability = 100.0f / _ui16NumberOfActiveNeighbors;
                if (fProbability < 10.0f) {
                    fProbability = 10.0f;
                }
                if ((rand() % 100) < fProbability) {
                    broadcastCtrlMessage (pDSCtrlMsg, "forwarding ctrl");
                }
            }
            free (pszMsgId);
        }
    }
}

void TopologyForwardingController::doFloodingForwarding (DisServiceMsg *pDSMsg) 
{
    if (pDSMsg->getType() == DisServiceMsg::DSMT_Data) { // This is a data msg
        DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg *) pDSMsg;
        MessageHeader *pMH = pDSDMsg->getMessageHeader();
        const char *pszMsgId = pMH->getMsgId();
        if (pszMsgId == NULL) {
            return;
        }
        const char *pszPublisherNodeId = pMH->getPublisherNodeId();
        if (pszPublisherNodeId == NULL) {
            return;
        }
        if (0 != stricmp (pszPublisherNodeId, _pDisService->getNodeId())) {
            if (_msgHistory.put (pszMsgId)) {
                broadcastDataMessage (pDSDMsg, "forwarding data");
            }
        }
    } else { // This is a control message
        DisServiceCtrlMsg *pDSCtrlMsg = (DisServiceCtrlMsg *) pDSMsg;
        const char *pszSenderNodeId = pDSCtrlMsg->getSenderNodeId();
        if (pszSenderNodeId == NULL) {
            return;
        }
        else if (0 != stricmp (pszSenderNodeId, _pDisService->getNodeId())) {
            char *pszMsgId = (char*) calloc (strlen (pszSenderNodeId) + 1 + 10 + 1, sizeof (char));
            sprintf (pszMsgId, "%s:%u", pszSenderNodeId, pDSCtrlMsg->getCtrlMsgSeqNo());
            if (_msgHistory.put (pszMsgId)) {
                broadcastCtrlMessage (pDSCtrlMsg, "forwarding ctrl");
            }
            free (pszMsgId);
        }
    }
}
