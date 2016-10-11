/*
 * TransmissionServiceListener.cpp
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

#include "TransmissionServiceListener.h"

#include "DataCacheInterface.h"
#include "DataRequestHandler.h"
#include "DisServiceDefs.h"
#include "DisseminationService.h"
#include "DisServiceMsg.h"
#include "DisServiceMsgHelper.h"
#include "DisServiceStats.h"
#include "Message.h"
#include "MessageInfo.h"
#include "MessageReassembler.h"
#include "SubscriptionState.h"
#include "WorldState.h"

#include "BufferReader.h"
#include "Logger.h"
#include "NLFLib.h"
#include "NetworkTrafficMemory.h"
#include "NetUtils.h"
#include "ChunkRetrievalController.h"
#include "NodeId.h"

#include <stdlib.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

extern Logger *pDataMsgLog;

TransmissionServiceListener::TransmissionServiceListener (DisseminationService *pDisService, DataRequestHandler *pDataReqHandler,
                                                          LocalNodeInfo *pLocalNodeInfo, MessageReassembler *pMsgReassembler,
                                                          SubscriptionState *pSubState, NetworkTrafficMemory *pTrafficMemory,
                                                          bool bOppListeningEnabled, bool bTargetFilteringEnabled)
    : NetworkMessageServiceListener ((uint16)0),
      _bOppListeningEnabled (bOppListeningEnabled),
      _bTargetFilteringEnabled (bTargetFilteringEnabled),
      _nodeId (pDisService->getNodeId()),
      _pDCI (pDisService->getDataCacheInterface()),
      _pDataReqHandler (pDataReqHandler),
      _pDisService (pDisService),
      _pLocalNodeInfo (pLocalNodeInfo),
      _pMsgReassembler (pMsgReassembler),
      _pNetTrafficMemory (pTrafficMemory),
      _pSubState (pSubState),
      _m (28)
{
    const char *pszMethodName = "TransmissionServiceListener::TransmissionServiceListener";
    checkAndLogMsg (pszMethodName, Logger::L_Info, "opp listening enabled: %s.\n", (_bOppListeningEnabled ? "true" : "false"));
    checkAndLogMsg (pszMethodName, Logger::L_Info, "target filtering enabled: %s.\n", (_bTargetFilteringEnabled ? "true" : "false"));
    srand((uint32)getTimeInMilliseconds());
    logMutexId("TransmissionServiceListener", "_m", _m.getId());
}

TransmissionServiceListener::~TransmissionServiceListener()
{
}

int TransmissionServiceListener::deregisterAllMessageListeners()
{
    return _notifier.deregisterAllListeners();
}

int TransmissionServiceListener::deregisterMessageListener (unsigned int uiIndex)
{
    return _notifier.deregisterListener (uiIndex);
}

int TransmissionServiceListener::registerMessageListener (MessageListener *pListener)
{
    return _notifier.registerListener (pListener);
}

int TransmissionServiceListener::messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                                 uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                                 const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                 const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp)
{
    _m.lock (260);
    int rc = messageArrivedInternal (pszIncomingInterface, ui32SourceIPAddress, ui8MsgType,
                                     ui16MsgId, ui8HopCount, ui8TTL, pMsgMetaData,
                                     ui16MsgMetaDataLen, pMsg, ui16MsgLen, i64Timestamp);
    _m.unlock (260);
    return rc;
}

int TransmissionServiceListener::messageArrivedInternal (const char *pszIncomingInterface,
                                                         uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                                         uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                         const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp)
{
    const char *pszMethodName = "TransmissionServiceListener::messageArrived";

    static uint64 ui64Bytes = 0;
    ui64Bytes += ui16MsgLen;
    checkAndLogMsg (pszMethodName, Logger::L_Info, "bytes: %lld\n", ui64Bytes);

    if (!DisServiceMsgHelper::isDisServiceMessage (ui8MsgType)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "this message is not for the Dissemination Service\n");
        return -1;
    }

    uint8 ui8DSMsgType;
    if (DisServiceMsgHelper::getMessageType (pMsgMetaData, ui16MsgMetaDataLen, ui8DSMsgType) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "can not read ui8DSMsgType.\n");
        return -2;
    }

    DisServiceMsg *pDSMsg = DisServiceMsgHelper::getInstance (ui8DSMsgType);
    if (pDSMsg == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "unknown message type %d\n", ui8DSMsgType);
        return -3;
    }

    int rc;
    BufferReader br (pMsg, ui16MsgLen);
    if (0 != (rc = pDSMsg->read (&br, ui16MsgLen))) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "reading a message of type %h failed with rc = %d\n",
                        ui8DSMsgType, rc);
        DisServiceMsgHelper::deallocatedDisServiceMsg (pDSMsg);
        return -4;
    }

    if (DisServiceMsgHelper::sentBy (_nodeId, pDSMsg)) {
        // Drop messages from itself
        DisServiceMsgHelper::deallocatedDisServiceMsg (pDSMsg);
        return 0;
    }

    if (!DisServiceMsgHelper::isInSession (_pDisService->getSessionId(), pDSMsg)) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "received message from peer %s "
                        "with wrong session key: %s while it should have been %s\n",
                        pDSMsg->getSenderNodeId(),
                        (pDSMsg->getSessionId() == NULL ? "NULL" : pDSMsg->getSessionId()),
                        (_pDisService->getSessionId() == NULL ? "NULL" : _pDisService->getSessionId()));
        return 0;
    }

    // Notify the arrival of a new message
    _notifier.newIncomingMessage (pMsgMetaData, ui16MsgMetaDataLen, pDSMsg,
                                  ui32SourceIPAddress, pszIncomingInterface);

    bool bTargetSpecified = false;
    bool bIsTarget = DisServiceMsgHelper::isTarget (_nodeId, pDSMsg, bTargetSpecified);

    switch (ui8DSMsgType) {
        //------------------------------------------------------------------
        // The received message contains data
        //------------------------------------------------------------------
        case DisServiceMsg::DSMT_Data:
        {
            DisServiceDataMsg *pDSDMsg = (DisServiceDataMsg*) pDSMsg;

            Message *pMessage = pDSDMsg->getMessage();
            if (pMessage == NULL) {
                delete pDSDMsg;
                pDSDMsg = NULL;
                return 0;
            }

            MessageHeader *pMH = pMessage->getMessageHeader();
            _pDisService->getStats()->dataFragmentReceived (pDSMsg->getSenderNodeId(), pMH->getGroupName(), pMH->getTag(), pMH->getFragmentLength());

            if ((pDSDMsg->isRepair()) && (_pNetTrafficMemory != NULL)) {
                int rc = _pNetTrafficMemory->add (pMH, getTimeInMilliseconds());
                if (rc < 0) {
                    checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                    "message %s could not be added to network traffic memory. Error code: %d\n",
                                    pMH->getMsgId(), rc);
                }
                else {
                    char *pszAddr = NetUtils::ui32Inetoa (ui32SourceIPAddress);
                    if (pszAddr != NULL) {
                        checkAndLogMsg (pszMethodName, Logger::L_Info,
                                        "message %s received from %s added to traffic memory\n",
                                        pMH->getMsgId(), pszAddr);
                        free (pszAddr);
                    }
                }
            }

            const String ctrl ("DSProCtrl");
            if ((ctrl != pMH->getGroupName()) && (!bTargetSpecified)) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "target id not specified for message %s.\n", pMH->getMsgId());
            }

            static uint64 ui64TargetedBytes = 0;
            static uint64 ui64TargetedToMeBytes = 0;
            static uint64 ui64UntargetedBytes = 0;
            if (bTargetSpecified) {
                if (bIsTarget) {
                    ui64TargetedToMeBytes += ui16MsgLen;
                }
                ui64TargetedBytes += ui16MsgLen;
            }
            else {
                ui64UntargetedBytes += ui16MsgLen;
            }
            checkAndLogMsg (pszMethodName, Logger::L_Info, "targeted %llu (%llu to me)\t untargeted %llu.\n",
                            ui64TargetedBytes, ui64TargetedToMeBytes, ui64UntargetedBytes);

            if ((_bTargetFilteringEnabled) && (bTargetSpecified) && (!bIsTarget)) {
                if (_bOppListeningEnabled) {
                    bool bHasCompleteMsg = _pDCI->hasCompleteMessage (pMH);
                    if (!bHasCompleteMsg && !_pDCI->containsMessage (pMH)) {
                        _pDisService->addDataToCache (pMH, pDSDMsg->getPayLoad());
                    }
                }
                free ((void *) pMessage->getData());
                delete pMessage->getMessageHeader();
                delete pMessage;
                pMessage = NULL;
                break;
            }

            if (1 == (_nodeId == pMH->getPublisherNodeId())) {
                // The message is from this node - ignore it
                checkAndLogMsg (pszMethodName, Logger::L_Info, "The message is from this node - ignoring it\n");
                free ((void *) pMessage->getData());
                delete pMessage->getMessageHeader();
                delete pMessage;
                pMessage = NULL;
                break;
            }

            bool bDrop = false;
            bool bAck = false;
            const bool bHasSubscription = _pLocalNodeInfo->hasSubscription (pMessage);
            if (bHasSubscription) {
                if (_pSubState->isRelevant (pMH->getGroupName(), pMH->getPublisherNodeId(),
                                            pMH->getMsgSeqId(), pMH->getChunkId())) {
                    // The message needs to be reassembled or given to the subscription state, unless the complete
                    // message has already been reassembled and it is the SubscriptionState, waiting to be delivered.
                    // Check the latter case first
                    if (_pSubState->containsMessage (pMH->getGroupName(), pMH->getPublisherNodeId(), pMH->getMsgSeqId())) {
                        bDrop = bAck = true;
                    }
                    else {
                        // It's not in the subscription state, therefore it has not been completely reassembled yet,
                        // give it to the message reassembler or to the subscription state

                        // Even if this node is not the target - assume that it is since there is an active subscription
                        bIsTarget = true;
                    }
                }
                else if (_pLocalNodeInfo->isInAnyHistory (pMessage, _pSubState->getSubscriptionState (pMH->getGroupName(), pMH->getPublisherNodeId()))) {
                    // Even if this node is not the target - assume that it is since the message would be otherwise delivered to a client
                    checkAndLogMsg ("WEIRD - trl", Logger::L_Info, "is in history.\n");
                    bIsTarget = true;
                }
                else {
                    // This message was either already received, or it is too old and not part of a history request
                    // If the whole message has already been delivered, it means that it is in the cache,
                    // or it was cached and expired. In this latter case the node may want to re-cache
                    // the message or its fragment or even try to reassemble it again???
                    bDrop = bAck = true;
                    // TODO: decide what to do here
                }
            }
            else if (bIsTarget) {
                // We are the target of this message - so do not drop it
                // NOTE: This is important when harvesting data, as there may be no client with subscriptions
                //       on this node, but the node must reassemble and acknowledge data that is received
                checkAndLogMsg ("WEIRD - trl", Logger::L_Info, "so, no subsription??? (1).\n");
                bDrop = false;
            }
            else if (!pMH->isCompleteMessage()) {
                // The node does not have a subscription for the message, so it
                // should be dropped, unless the message reassembler has been
                // configured to reassemble the opportunistically received
                // messages. In this case, the fragment has to be given to the
                // MessageReassembler, otherwise the message can be dropped
                if (!_pMsgReassembler->requestOpportunisticallyReceivedMessages()) {
                    bDrop = true;
                }
                checkAndLogMsg ("WEIRD - trl", Logger::L_Info, "so, no subsription??? (2).\n");
            }
            else {
                // The message is not of any interest for
                bDrop = bAck = true;
            }

            // Add to the data cache
            const bool bHasCompleteMsg = _pDCI->hasCompleteMessage (pMH);
            if (!bHasCompleteMsg && !_pDCI->containsMessage (pMH)) {
                _pDisService->addDataToCache (pMH, pDSDMsg->getPayLoad());
            }
            else {
                if (bIsTarget) {
                    checkAndLogMsg ("TransmissionServiceListener::messageArrived", Logger::L_Info,
                                    "receiving duplicate traffic for message %s on interface %s from peer %s\n",
                                    pMH->getMsgId(), pszIncomingInterface, pDSDMsg->getSenderNodeId());
                    _pDisService->getStats()->_ui32TargetedDuplicateTraffic += (ui16MsgMetaDataLen + ui16MsgLen);
                }
                else {
                    checkAndLogMsg ("TransmissionServiceListener::messageArrived", Logger::L_Info,
                                    "overhearing duplicate traffic for message %s on interface %s from peer %s\n",
                                    pMH->getMsgId(), pszIncomingInterface, pDSDMsg->getSenderNodeId());
                    _pDisService->getStats()->_ui32OverheardDuplicateTraffic += (ui16MsgMetaDataLen + ui16MsgLen);
                }
                if (!bIsTarget) {
                    if (bHasCompleteMsg) {
                        bAck = true;
                    }
                    bDrop = true;
                }
            }

            if (bDrop) {
                if (bAck) {
                    evaluateAcknowledgment (pMH);
                }
                delete pMH;
                pMH = NULL;
                free ((void *) pMessage->getData());
                delete pMessage;
                pMessage = NULL;
                break;
            }

            // Set the initial subscription state for a certain group-client
            if (bHasSubscription && _pSubState->isSubscriptionBySenderEmpty (pMH->getGroupName(), pMH->getPublisherNodeId())) {
                if (wildcardStringCompare (pMH->getGroupName(), "DSPro*")) {
                    // THIS IS A HACK: messages transmitted using DisServicePro
                    // may not be in sequence, since they are ordered by predicted
                    // utility.
                    // Setting the current subscription state to 0 avoids that
                    // messages belonging to pMH->getGroupName() with seq id lower
                    // than the seq id of the current message, are dropped.
                    _pSubState->setSubscriptionState (pMH->getGroupName(), pMH->getPublisherNodeId(), 0);
                }
                else {
                    _pSubState->setSubscriptionState (pMH->getGroupName(), pMH->getPublisherNodeId(), pMH->getMsgSeqId());
                }
            }

            if (pMH->isCompleteMessage()) {
                _pDisService->getStats()->dataMessageReceived (pDSMsg->getSenderNodeId(), pMH->getGroupName(),
                                                               pMH->getTag(), pMH->getFragmentLength());
                deliverCompleteMessage (pMessage, !bIsTarget);
            }
            else {
                // FRAGMENT
                void *pCachedData = NULL;
                char *pszTmp = pMH->getIdForCompleteMsg();
                String completeMsgId (pszTmp);
                if (pszTmp != NULL) {
                    free (pszTmp);
                }
                if (bHasCompleteMsg) {
                    pCachedData = (void*) _pDCI->getData (completeMsgId);
                }
                if (pCachedData != NULL) {
                    free ((void*) pMessage->getData());  // The message has not been inserted into the message
                                                         // reassembler therefore pData can be safely deallocated
                    void *pDataCopy = malloc (pMH->getTotalMessageLength());
                    memcpy (pDataCopy, pCachedData, pMH->getTotalMessageLength());
                    MessageHeader *pMHCompleteMessage = pMH->clone();
                    pMHCompleteMessage->setFragmentOffset (0);
                    pMHCompleteMessage->setFragmentLength (pMHCompleteMessage->getTotalMessageLength());
                    Message *pNewMessage = new Message (pMHCompleteMessage, pDataCopy);
                    _pDisService->getStats()->dataMessageReceived (pDSMsg->getSenderNodeId(), pMH->getGroupName(),
                                                                   pMH->getTag(), pMH->getTotalMessageLength());
                    deliverCompleteMessage (pNewMessage, !bIsTarget);

                    _pDCI->release (completeMsgId, pCachedData);
                }
                else if (MessageReassemblerUtils::loadOpportunisticallyCachedFragments (_pMsgReassembler, _pDCI, pMH, !bIsTarget)) {
                    free ((void*) pMessage->getData());  // The message has not been inserted into the message
                                                         // reassembler - therefore pData can be safely deallocated
                }
                else {
                    // The message that this fragment belongs to has not yet been delivered - so need to pass
                    // it to the reassembler
                    int rc = _pMsgReassembler->fragmentArrived (pMessage, !bIsTarget);
                    if (rc == 0) {
                        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                                        "fragment <%s> of length <%u> of <%u> stored by the MessageReassembler\n",
                                        pMH->getMsgId(), pMH->getFragmentLength(), pMH->getTotalMessageLength());
                    }
                    else {
                        free ((void*) pMessage->getData());  // The message has not been inserted into the message
                                                             // reassembler - therefore pData can be safely deallocated
                        if ((rc >= 1) && (!pMH->isChunk() && ((MessageInfo *) pMH)->getAcknowledgment())) {
                            // The message is not reassembled but needs to be acknowledged
                            pMH->setFragmentOffset (0);
                            pMH->setFragmentLength (pMH->getTotalMessageLength());
                            _pDisService->broadcastDisServiceCntrlMsg (new DisServiceAcknowledgmentMessage (_pDisService->getNodeId(), pMH->getMsgId()),
                                                                       NULL, "acknowledgment");
                            checkAndLogMsg (pszMethodName, Logger::L_Info,
                                            "sent an acknowledgement for message with id <%s>\n",
                                            pMH->getMsgId());
                        }
                    }
                }

                delete pMH;    // MessageReassembler makes its own copy
                pMH = NULL;
                delete pMessage;  // MessageReassembler does not need the message wrapper
                pMessage = NULL;
            }
            break;
        }

        //------------------------------------------------------------------
        // The received message is a request for data
        //------------------------------------------------------------------
        case DisServiceMsg::DSMT_DataReq:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_DataReq\n");
            if ((_pNetTrafficMemory == NULL) || ((getTimeInMilliseconds() - i64Timestamp) < _pNetTrafficMemory->_ui16IgnoreRequestTime)) {
                _pDataReqHandler->dataRequestMessageArrived ((DisServiceDataReqMsg *) pDSMsg, pszIncomingInterface);
                pDSMsg = NULL; //let the DatarequestHandler delete it
            }
            break;
        }

        //------------------------------------------------------------------
        // The received message is a World State message
        //------------------------------------------------------------------
        case DisServiceMsg::DSMT_WorldStateSeqId:
        {
            DisServiceWorldStateSeqIdMsg *pDSWSSIMsg = (DisServiceWorldStateSeqIdMsg*) pDSMsg;
            checkAndLogMsg (pszMethodName, Logger::L_Info,
                            "case DSMT_WorldStateSeqId - Node: %s, \tTopology SeqId %d, \tSubscription SeqId %d\n",
                            pDSWSSIMsg->getSenderNodeId(),
                            pDSWSSIMsg->getTopologyStateUpdateSeqId(),
                            pDSWSSIMsg->getSubscriptionStateCRC());
            _pDisService->handleWorldStateSequenceIdMessage (pDSWSSIMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_SubStateMessage:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_SubStateMessage\n");
            DisServiceSubscriptionStateMsg *pDSSMsg = (DisServiceSubscriptionStateMsg*) pDSMsg;
            _pDisService->handleSubscriptionStateMessage (pDSSMsg, ui32SourceIPAddress);
            pDSMsg = NULL;
            break;
        }

        case DisServiceMsg::DSMT_SubStateReq:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_SubStateReqestMessage\n");
            DisServiceSubscriptionStateReqMsg *pDSSRMsg = (DisServiceSubscriptionStateReqMsg*) pDSMsg;
            _pDisService->handleSubscriptionStateRequestMessage (pDSSRMsg, ui32SourceIPAddress);
            delete (pDSSRMsg->getSubscriptionStateTable());
            break;
        }

        /*case DisServiceMsg::DSMT_TopologyState:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_TopologyState\n");
            DisServiceTopologyStateMsg *pDSWSMsg = (DisServiceTopologyStateMsg*) pDSMsg;
            _pDisService->handleTopologyStateMsg (pDSWSMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_TopologyStateReq:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_TopologyStateReq\n");
            DisServiceTopologyStateReqMsg *pDSWSMsg = (DisServiceTopologyStateReqMsg*) pDSMsg;
            _pDisService->handleTopologyStateRequestMessage (pDSWSMsg, ui32SourceIPAddress);
            break;
        }*/

        case DisServiceMsg::DSMT_DataCacheQuery:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_DataCacheQuery\n");
            DisServiceDataCacheQueryMsg *pDSDCQMsg = (DisServiceDataCacheQueryMsg*) pDSMsg;
            bTargetSpecified = false;
            if (bTargetSpecified && !bIsTarget) {
                // The query is targeted to a different node
            }
            else {
                _pDisService->handleDataCacheQueryMessage (pDSDCQMsg, ui32SourceIPAddress);
            }
            break;
        }

        case DisServiceMsg::DSMT_DataCacheQueryReply:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_DataCacheQueryReply\n");
            DisServiceDataCacheQueryReplyMsg *pDSDCQRMsg = (DisServiceDataCacheQueryReplyMsg*) pDSMsg;
            _pDisService->handleDataCacheQueryReplyMessage (pDSDCQRMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_DataCacheMessagesRequest:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_DataCacheMessagesRequestMsg\n");
            DisServiceDataCacheMessagesRequestMsg *pDSDCMsgsReqMsg = (DisServiceDataCacheMessagesRequestMsg*) pDSMsg;
            _pDisService->handleDataCacheMessagesRequestMessage (pDSDCMsgsReqMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_AcknowledgmentMessage:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_AcknowledgmentMessage\n");
            DisServiceAcknowledgmentMessage *pDSAMsg = (DisServiceAcknowledgmentMessage*) pDSMsg;
            _pDisService->handleAcknowledgmentMessage(pDSAMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_CompleteMessageReq:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_CompleteMessageReq\n");
            DisServiceCompleteMessageReqMsg *pDSCMRMsg = (DisServiceCompleteMessageReqMsg*) pDSMsg;
            _pDisService->handleCompleteMessageRequestMessage (pDSCMRMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_CacheEmpty:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_CacheEmpty\n");
            DisServiceCacheEmptyMsg *pDSCEMsg = (DisServiceCacheEmptyMsg*) pDSMsg;
            _pDisService->handleCacheEmptyMessage (pDSCEMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_CtrlToCtrlMessage:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_CtrlToCtrlMessage\n");
            ControllerToControllerMsg *pCTCMsg = (ControllerToControllerMsg*) pDSMsg;
            _pDisService->handleCtrlToCtrlMessage (pCTCMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_HistoryReq:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_HistoryReq\n");
            DisServiceHistoryRequest *pHistoryReq = (DisServiceHistoryRequest*) pDSMsg;
            _pDisService->handleHistoryRequestMessage (pHistoryReq, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_HistoryReqReply:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_HistoryReqReply\n");
            DisServiceHistoryRequestReplyMsg *pDSHRRMsg = (DisServiceHistoryRequestReplyMsg*) pDSMsg;
            _pDisService->handleHistoryRequestReplyMessage (pDSHRRMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_SearchMsg:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_SearchMsg\n");
            SearchMsg *pSearchMsg = (SearchMsg*) pDSMsg;
            _pDisService->handleSearchMessage (pSearchMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_SearchMsgReply:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_SearchMsgReply\n");
            SearchReplyMsg *pSearchReplyMsg = (SearchReplyMsg*) pDSMsg;
            _pDisService->handleSearchReplyMessage (pSearchReplyMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_ImprovedSubStateMessage:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_ImprovedSubStateMessage\n");
            DisServiceImprovedSubscriptionStateMsg *pDSISMsg = (DisServiceImprovedSubscriptionStateMsg*) pDSMsg;
            _pDisService->handleImprovedSubscriptionStateMessage (pDSISMsg, ui32SourceIPAddress);
            break;
        }

        case DisServiceMsg::DSMT_ProbabilitiesMsg:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "case DSMT_ProbabilitiesMsg\n");
            DisServiceProbabilitiesMsg *pDSPMsg = (DisServiceProbabilitiesMsg*) pDSMsg;
            _pDisService->handleProbabilitiesMessage (pDSPMsg, ui32SourceIPAddress);
            break;
        }

        default:
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "unhandled message of type %d (%s)\n",
                            ui8DSMsgType, DisServiceMsgHelper::getMessageTypeAsString (ui8DSMsgType));
    }

    delete pDSMsg;
    pDSMsg = NULL;
    return 0;
}

void TransmissionServiceListener::evaluateAcknowledgment (MessageHeader *pMH)
{
    if (!pMH->isChunk() && ((MessageInfo *)pMH)->getAcknowledgment()) {
        // If already received and requiring an ack the message needs
        // to be acknowledged
        pMH->setFragmentOffset (0);
        pMH->setFragmentLength (pMH->getTotalMessageLength());

        DisServiceAcknowledgmentMessage ack (_pDisService->getNodeId(), pMH->getMsgId());
        _pDisService->broadcastDisServiceCntrlMsg (&ack, NULL, "Sending Acknowledgment");
        checkAndLogMsg ("TransmissionServiceListener::acknowledgeAndDropMessage", Logger::L_Info,
                        "Acknowledge for the MsgId sent<%s>\n", pMH->getGroupName());
    }
}

void TransmissionServiceListener::deliverCompleteMessage (Message *pMessage, bool bIsNotTarget)
{
    MessageHeader *pMH = pMessage->getMessageHeader();

    checkAndLogMsg ("TransmissionServiceListener::deliverCompleteMessage", Logger::L_Info,
                    "passing complete message <%s> to SubscriptionState\n", pMH->getMsgId());
    const uint32 ui32CurrentSubState = _pSubState->getSubscriptionState (pMH->getGroupName(), pMH->getPublisherNodeId());
    const bool bIsInHistory = _pLocalNodeInfo->isInAnyHistory (pMessage, ui32CurrentSubState);
    if (!bIsNotTarget) {
        RequestDetails *pDetails = _pMsgReassembler->messageArrived (pMessage);
        _pSubState->messageArrived (pMessage, bIsInHistory, pDetails);
        if (pDetails != NULL) {
            free (pDetails);
        }
    }
}


