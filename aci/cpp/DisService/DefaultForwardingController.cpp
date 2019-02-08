/*
 * DefaultForwardingController.cpp
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

#include "DefaultForwardingController.h"

#include "DisseminationService.h"
#include "DisServiceMsg.h"
#include "MessageInfo.h"
#include "TransmissionService.h"

#include "ConfigManager.h"
#include "StringTokenizer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

DefaultForwardingController::DefaultForwardingController (DisseminationService *pDisService, float fForwardProbability)
    : ForwardingController (FC_Default, pDisService), _msgHistory (DEFAULT_MESSAGE_HISTORY_DURATION),
      _unicastOverrides (false, true, true, true),
      _groupsToForward (true, true, true)
{
    _bPurelyProbForwarding = true;
    _fForwardProbability = fForwardProbability;
    _ui16NumberOfActiveNeighbors = 0;
    _bForwardOnlySpecifiedGroups = false;
    init();
}

DefaultForwardingController::~DefaultForwardingController()
{
}

void DefaultForwardingController::init (void)
{
    _bRelayDataMsgs = _pConfigManager->getValueAsBool ("aci.disService.relaying.dataMsgs", false);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "relaying for data messages %s\n", _bRelayDataMsgs ? "enabled" : "disabled");
    _bForwardDataMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.dataMsgs", true);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "forwarding for data messages %s\n", _bForwardDataMsgs ? "enabled" : "disabled");
    _bForwardDataRequestMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.dataReqMsgs", false);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "forwarding for data request messages %s\n", _bForwardDataRequestMsgs ? "enabled" : "disabled");
    _bForwardChunkQueryMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.chunkQueryMsgs", false);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "forwarding for chunk query messages %s\n", _bForwardChunkQueryMsgs ? "enabled" : "disabled");
    _bForwardChunkQueryHitsMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.chunkQueryHitsMsgs", false);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "forwarding for chunk query hits messages %s\n", _bForwardChunkQueryHitsMsgs ? "enabled" : "disabled");
    _bForwardTargetedMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.targetedMsgs", true);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "forwarding for targeted messages %s\n", _bForwardTargetedMsgs ? "enabled" : "disabled");
    _bForwardSearchMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.searchMsgs", true);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "forwarding for search messages %s\n", _bForwardSearchMsgs ? "enabled" : "disabled");
    _bForwardSearchReplyMsgs = _pConfigManager->getValueAsBool ("aci.disService.forwarding.enable.searchReplyMsgs", true);
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "forwarding for search reply messages %s\n", _bForwardSearchReplyMsgs ? "enabled" : "disabled");
    if (_pConfigManager->hasValue ("aci.disService.forwarding.historyWindowTime")) {
        _msgHistory.configure (_pConfigManager->getValueAsUInt32 ("aci.disService.forwarding.historyWindowTime"));
    }
    checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                    "set history window time to %lu ms\n", _msgHistory.getStorageDuration());
    const char *pszUnicastOverrides = _pConfigManager->getValue ("aci.disService.forwarding.unicastOverrides");
    if (pszUnicastOverrides != NULL) {
        checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                        "configuring unicast overrides with the following settings: <%s>\n", pszUnicastOverrides);
        StringTokenizer stOverrides (pszUnicastOverrides, ';');
        const char *pszEntry;
        while (NULL != (pszEntry = stOverrides.getNextToken())) {
            StringTokenizer stOneEntry (pszEntry, '-');
            const char *pszGroupName = stOneEntry.getNextToken();
            const char *pszAddr = stOneEntry.getNextToken();
            if ((pszGroupName != NULL) && (pszAddr != NULL)) {
                _unicastOverrides.put (pszGroupName, (char*) pszAddr);
                checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                                "added relay for group <%s> to address <%s>\n", pszGroupName, pszAddr);
            }
        }
    }
    const char *pszGroupsToForward = _pConfigManager->getValue ("aci.disService.forwarding.groupList");
    if (pszGroupsToForward != NULL) {
        _bForwardOnlySpecifiedGroups = true;
        checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                        "only forwarding specified groups - configuring with the following list: <%s>\n", pszGroupsToForward);
        StringTokenizer stGroupList (pszGroupsToForward, ';');
        const char *pszGroupName;
        while (NULL != (pszGroupName = stGroupList.getNextToken())) {
            _groupsToForward.put (pszGroupName);
            checkAndLogMsg ("DefaultForwardingController::init", Logger::L_Info,
                            "added group <%s> to be forwarded\n", pszGroupName);
        }
    }
}

void DefaultForwardingController::newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                                               const char *pszIncomingInterface)
{
    _ui16NumberOfActiveNeighbors++;
}

void DefaultForwardingController::deadNeighbor (const char *pszNodeUUID)
{
    if (_ui16NumberOfActiveNeighbors > 0) {
        _ui16NumberOfActiveNeighbors--;
    }
}

void DefaultForwardingController::newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                                     const char *pszIncomingInterface)
{
}

void DefaultForwardingController::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{
}

void DefaultForwardingController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

void DefaultForwardingController::newIncomingMessage (const void *, uint16, DisServiceMsg *pDSMsg,
                                                      uint32 ui32SourceIPAddress, const char *pszIncomingInterface)
{
    bool bRelay = false;
    if (pDSMsg->getType() == DisServiceMsg::DSMT_Data) {
        DisServiceDataMsg *pDSDataMsg = (DisServiceDataMsg*) pDSMsg;
        if (!_bForwardDataMsgs) {
            return;
        }
        if (pDSDataMsg->doNotForward()) {
            return;
        }
        if ((_bForwardOnlySpecifiedGroups) && (!_groupsToForward.containsKey (pDSDataMsg->getMessageHeader()->getGroupName()))) {
            checkAndLogMsg ("DefaultForwardingController::newIncomingMessage", Logger::L_LowDetailDebug,
                            "not forwarding data message in group <%s> because it is not in the list of groups to be forwarded\n",
                            pDSDataMsg->getMessageHeader()->getGroupName());
            return;
        }
        if (_bRelayDataMsgs) {
            bRelay = true;
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

    if ((pDSMsg->getTargetNodeId() != NULL) && (strlen (pDSMsg->getTargetNodeId()) > 0)) {
        if (!_bForwardTargetedMsgs) {
            checkAndLogMsg ("DefaultForwardingController::newIncomingMessage", Logger::L_LowDetailDebug,
                            "not forwarding message because target node id is set to <%s> and this node is configured not to forward targeted messages\n",
                            pDSMsg->getTargetNodeId());
            return;
        }
    }

    char **pszOutgoingInterfaces = NULL;
    if (!bRelay) {
        // If message relaying is _not_ allowed, I need to filter out the incoming interface from the outoing ones
        char **pszOutgoingInterfaces = getTrasmissionService()->getActiveInterfacesAddress();
        if ((pszOutgoingInterfaces != NULL) && (pszIncomingInterface != NULL)) {
            bool bFound = false;
            for (int i = 0; pszOutgoingInterfaces[i] != NULL; i++) {
                if (strcmp (pszOutgoingInterfaces[i], pszIncomingInterface) == 0) {
                    free (pszOutgoingInterfaces[i]);
                    pszOutgoingInterfaces[i] = NULL;
                    bFound = true;
                }
                else if (bFound) {
                    pszOutgoingInterfaces[i-1] = pszOutgoingInterfaces[i];
                    pszOutgoingInterfaces[i] = NULL;
                }
            }
            if (pszOutgoingInterfaces[0] == NULL) {
                free (pszOutgoingInterfaces);
                return;
            }
        }
    }

    if (_bPurelyProbForwarding) {
        doProbabilisticForwarding (pDSMsg, ui32SourceIPAddress, (const char **) pszOutgoingInterfaces);
    }
    else {
        doStatefulForwarding (pDSMsg, (const char **) pszOutgoingInterfaces);
    }

    if (pszOutgoingInterfaces != NULL) {
        for (int i = 0; pszOutgoingInterfaces[i] != NULL; i++) {
            free (pszOutgoingInterfaces[i]);
        }
        free (pszOutgoingInterfaces);
    }
}

void DefaultForwardingController::doProbabilisticForwarding (DisServiceMsg *pDSMsg, uint32 ui32SourceIPAddress, const char **pszOutgoingInterfaces)
{
    if (pDSMsg->getType() == DisServiceMsg::DSMT_Data) {
        doProbabilisticForwardingDataMsg ((DisServiceDataMsg*) pDSMsg, ui32SourceIPAddress, pszOutgoingInterfaces);
    }
    else {
        // This is a control message
        doProbabilisticForwardingCtrlMsg ((DisServiceCtrlMsg*) pDSMsg, ui32SourceIPAddress, pszOutgoingInterfaces);
    }
}

void DefaultForwardingController::doProbabilisticForwardingCtrlMsg (DisServiceCtrlMsg *pDSCtrlMsg, uint32 ui32SourceIPAddress, const char **pszOutgoingInterfaces)
{
    const char *pszOriginatorNodeId = pDSCtrlMsg->getSenderNodeId();
    if (pszOriginatorNodeId == NULL) {
        // Cannot handle this message
        checkAndLogMsg ("DefaultForwardingController::doProbabilisticForwardingCtrlMsg", Logger::L_Warning,
                        "getSenderNodeId() returned NULL - cannot handle\n");
        return;
    }
    if (0 != stricmp (pszOriginatorNodeId, _pDisService->getNodeId())) {
        // This message did not originate here
        char *pszMsgId = (char*) calloc (strlen(pszOriginatorNodeId) + 1 + 10 + 1, sizeof (char));
        sprintf (pszMsgId, "%s:%u", pszOriginatorNodeId, pDSCtrlMsg->getCtrlMsgSeqNo());
        if (_msgHistory.put (pszMsgId)) {
            // This message does not exist in the message history hashtable - forward it based on the probability
            if ((rand() % 100) < (int)_fForwardProbability) {
                // Forward it - with a hint to exclude the source of the message (currently only realized by the ARL CSR Radio)
                String hints = "exclude=";
                hints += InetAddr (ui32SourceIPAddress).getIPAsString();
                broadcastCtrlMessage (pDSCtrlMsg, "forwarding ctrl", NULL, hints);
            }
        }
        free (pszMsgId);
    }
}

void DefaultForwardingController::doProbabilisticForwardingDataMsg (DisServiceDataMsg *pDSDMsg, uint32 ui32SourceIPAddress, const char **pszOutgoingInterfaces)
{
    MessageHeader *pMH = pDSDMsg->getMessageHeader();
    // const char *pszActualSenderNodeId = pDSDMsg->getSenderNodeId();
    // const char *pForwardingTarget = NULL;

    if (_ui16NumberOfActiveNeighbors < 2) {
        // No need to forward - there are no other neighbors besides the one that sent the message
        checkAndLogMsg ("DefaultForwardingController::doProbabilisticForwardingDataMsg", Logger::L_Warning,
                        "IGNORE - not forwarding message because there are not at least two neighbors\n");
        //return;
    }
    const char *pszMsgId = pMH->getMsgId();
    if (pszMsgId == NULL) {
        checkAndLogMsg ("DefaultForwardingController::doProbabilisticForwardingDataMsg", Logger::L_Warning,
                        "getMsgId() returned NULL - cannot forward\n");
        return;
    }
    const char *pszOriginatorNodeId = pMH->getPublisherNodeId();
    if (pszOriginatorNodeId == NULL) {
        checkAndLogMsg ("DefaultForwardingController::doProbabilisticForwardingDataMsg", Logger::L_Warning,
                        "getSenderNodeId() returned NULL - cannot handle\n");
        return;
    }
    const char *pszGroupName = pMH->getGroupName();
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DefaultForwardingController::doProbabilisticForwardingDataMsg", Logger::L_Warning,
                        "getGroupName() returned NULL - cannot handle\n");
        return;
    }
    if (0 == stricmp (pszOriginatorNodeId, _pDisService->getNodeId ())) {
        checkAndLogMsg ("DefaultForwardingController::doProbabilisticForrwardingDataMsg", Logger::L_LowDetailDebug,
            "not forwarding data message <%s> because it originated at this node\n", pszMsgId);
    }
    // This message did not originated here
    if (_msgHistory.put (pszMsgId)) {
        // This message does not exist in the message history hashtable - forward it based on the probability
        if ((rand() % 100) < (int)_fForwardProbability) {
            // Check if the group name is in the Unicast Overrides hashtable - this is a hack for TS14 to force
            // certain groups to be unicast to a single destination instead of neighborcast as usual
            const char *pszUnicastAddr = _unicastOverrides.get (pszGroupName);
            if (pszUnicastAddr != NULL) {
                // Add a hint to unicast this message to the specified address (currently only realized by the ProxyDatagramSocket and the ARL CSR Radio)
                String hints = "unicast=";
                hints += pszUnicastAddr;
                broadcastDataMessage (pDSDMsg, "forwarding data via unicast", pszOutgoingInterfaces, NULL, hints);
            }
            else {
                // Forward it - with a hint to exclude the source of the message (currently only realized by the ARL CSR Radio)
                String hints = "exclude=";
                hints += InetAddr (ui32SourceIPAddress).getIPAsString();
                broadcastDataMessage (pDSDMsg, "forwarding data", pszOutgoingInterfaces, NULL, hints);
                checkAndLogMsg ("DefaultForwardingController::doProbabilisticForwardingDataMsg", Logger::L_LowDetailDebug,
                                "forwarding data message <%s>\n", pszMsgId);
            }
        }
        else {
            checkAndLogMsg ("DefaultForwardingController::doProbabilisticForwardingDataMsg", Logger::L_LowDetailDebug,
                "not forwarding data message <%s> because of probability\n", pszMsgId);
        }
    }
    else {
        checkAndLogMsg ("DefaultForwardingController::doProbabilisticForrwardingDataMsg", Logger::L_LowDetailDebug,
                        "not forwarding data message <%s> because it was already forwarded\n", pszMsgId);
    }
}

void DefaultForwardingController::doStatefulForwarding (DisServiceMsg *pDSMsg, const char **pszOutgoingInterfaces)
{
    if (_ui16NumberOfActiveNeighbors > 1) {
        if (pDSMsg->getType() == DisServiceMsg::DSMT_Data) {
            DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg*) pDSMsg;
            float fProbability = 100.0f / _ui16NumberOfActiveNeighbors;
            if (fProbability < 10.0f) {
                fProbability = 10.0f;
            }
            if ((rand() % 100) < fProbability) {
                // Forward it
                broadcastDataMessage (pDSDMsg, "forwarding", pszOutgoingInterfaces);
            }
        }
    }
}

