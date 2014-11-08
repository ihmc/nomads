/*
 * Services.cpp
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

#include "Services.h"

#include "DataCacheInterface.h"
#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "DisseminationService.h"
#include "DSSFLib.h"
#include "Message.h"
#include "MessageInfo.h"
#include "NodeInfo.h"
#include "PropertyStoreInterface.h"
#include "RequestsState.h"
#include "TransmissionHistoryInterface.h"

#include "Logger.h"
#include "NLFLib.h"
#include "SequentialArithmetic.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

uint8 checkAngGetProperty (uint8 *pUi8Priorities, int i)
{
    if (pUi8Priorities == NULL) {
        return MessageHeader::DEFAULT_AVG_PRIORITY;
    }
    switch (*pUi8Priorities) {
        case MessageHeader::DEFAULT_MIN_PRIORITY:
            return MessageHeader::DEFAULT_MIN_PRIORITY;

        case MessageHeader::DEFAULT_MAX_PRIORITY:
            return MessageHeader::DEFAULT_MAX_PRIORITY;

        default:
            return MessageHeader::DEFAULT_AVG_PRIORITY;
    }
}

//------------------------------------------------------------------------------
// ServiceBase
//------------------------------------------------------------------------------

ServiceBase::ServiceBase (DisseminationService *pDisService)
{
    if (pDisService == NULL) {
        checkAndLogMsg ("ServiceBase::ServiceBase", Logger::L_SevereError,
                        "ServiceBase could not be initialized - pDisService is NULL\n");
        exit (-1);
    }
    _pDisService = pDisService;
}

ServiceBase::~ServiceBase (void)
{
    _pDisService = NULL;
}

//------------------------------------------------------------------------------
// DataCacheService
//------------------------------------------------------------------------------

DataCacheService::DataCacheService (DisseminationService *pDisService)
    : ServiceBase (pDisService)
{
    if (pDisService == NULL) {
        checkAndLogMsg ("DataCacheService::DataCacheService", Logger::L_SevereError,
                        "DataCacheService could not be initialized.  pDisService is NULL.\n");
        exit (-1);
    }
    _pDataCacheInterface = pDisService->getDataCacheInterface();
    if (_pDataCacheInterface == NULL) {
        checkAndLogMsg ("DataCacheService::DataCacheService", Logger::L_SevereError,
                        "DataCacheService could not be initialized.  _pDataCacheInterface is NULL.\n");
        exit (-1);
    }
    _pLocalNodeInfo = pDisService->getLocalNodeInfo();
    if (_pDataCacheInterface == NULL) {
        checkAndLogMsg ("DataCacheService::DataCacheService", Logger::L_SevereError,
                        "DataCacheService could not be initialized.  _pLocalNodeInfo is NULL.\n");
        exit (-1);
    }
    _pszNodeName = strDup (pDisService->getNodeId());
    if (_pDataCacheInterface == NULL) {
        checkAndLogMsg ("DataCacheService::DataCacheService", Logger::L_SevereError,
                        "DataCacheService could not be initialized.  _pszNodeName is NULL.\n");
        exit (-1);
    }
}

DataCacheService::~DataCacheService()
{
    _pDataCacheInterface = NULL;
    _pLocalNodeInfo = NULL;
    free ((char *)_pszNodeName);
    _pszNodeName = NULL;
}

bool DataCacheService::isMessageInCache (const char *pszMsgId)
{
    return _pDataCacheInterface->hasCompleteMessage (pszMsgId);
}

int DataCacheService::deleteMessage (const char *pszMsgId)
{
    if (_pDataCacheInterface == NULL) {
        return -1;
    }

    DArray2<String> aTokenizedKey;
    int rc = convertKeyToField (pszMsgId, aTokenizedKey, 3, MSG_ID_GROUP,
                                MSG_ID_SENDER, MSG_ID_SEQ_NUM);
    if (rc < 0 ||
        aTokenizedKey[MSG_ID_SENDER].length() == 0 ||
        aTokenizedKey[MSG_ID_GROUP].length() == 0) {
        checkAndLogMsg ("DataCacheService::deleteMessage", Logger::L_SevereError,
                        "convertKeyToField failed with code %d\n", rc);
        return -2;
    }

    bool bIsLatestMessagePushedByNode = false;
    if (aTokenizedKey[MSG_ID_SENDER] == _pszNodeName) {
        uint32 ui32GrpPubstate = _pLocalNodeInfo->getGroupPubState (aTokenizedKey[MSG_ID_GROUP]);
        if (SequentialArithmetic::greaterThanOrEqual (atoui32(aTokenizedKey[MSG_ID_SEQ_NUM]), ui32GrpPubstate)) {
            bIsLatestMessagePushedByNode = true;
        }
    }

    return _pDataCacheInterface->deleteDataAndMessageInfo (pszMsgId, bIsLatestMessagePushedByNode);
}

PtrLList<MessageHeader> * DataCacheService::lockAndQueryDataCache (const char *pszSQLStatement)
{
    if (_pDataCacheInterface == NULL) {
        return NULL;
    }
    return _pDataCacheInterface->getMessageInfos (pszSQLStatement);
}

PtrLList<MessageHeader> * DataCacheService::lockAndQueryDataCache (DisServiceDataCacheQuery *pQuery)
{
    if (_pDataCacheInterface == NULL) {
        return NULL;
    }
    return _pDataCacheInterface->getMessageInfos (pQuery);
}

DArray2<String> * DataCacheService::getSenderNodeIds (const char *pszGroupName)
{
    if (_pDataCacheInterface == NULL) {
        return NULL;
    }
    return _pDataCacheInterface->getSenderNodeIds (pszGroupName);
}

MessageHeader * DataCacheService::getMessageInfo (const char *pszId)
{
    if (_pDataCacheInterface == NULL) {
        return NULL;
    }
    return _pDataCacheInterface->getMessageInfo (pszId);
}

DArray2<String> * DataCacheService::getExpiredEntries (void)
{
    if (_pDataCacheInterface == NULL) {
        return NULL;
    }
    return _pDataCacheInterface->getExpiredEntries();
}

void DataCacheService::releaseQueryResults()
{
    if (_pDataCacheInterface != NULL) {
        _pDataCacheInterface->unlock();
    }
}

void DataCacheService::releaseQueryResults (MessageHeader *pMI)
{
    if (_pDataCacheInterface != NULL) {
        _pDataCacheInterface->unlock();
    }
    _pDataCacheInterface->release (pMI->getMsgId(), pMI);
}

void DataCacheService::releaseQueryResults (PtrLList<MessageHeader> *pMessageList)
{
    if (_pDataCacheInterface != NULL) {
        //_pDataCacheInterface
        _pDataCacheInterface->unlock();
    }
    // TODO : delete the results
}

DataCacheInterface * DataCacheService::getDataCacheInterface()
{
    return _pDataCacheInterface;
}

//------------------------------------------------------------------------------
// GroupMembershipDataCacheService
//------------------------------------------------------------------------------

GroupMembershipService::GroupMembershipService (DisseminationService *pDisService)
    : ServiceBase (pDisService)
{
    if (pDisService == NULL) {
        checkAndLogMsg ("GroupMembershipService::GroupMembershipService", Logger::L_SevereError,
                        "GroupMembershipService could not be initialized.  pDisService is NULL.\n");
        exit (-1);
    }
    _pLocalNodeInfo = pDisService->getLocalNodeInfo();
    _pPeerState = pDisService->getPeerState();
}

GroupMembershipService::~GroupMembershipService()
{
    _pLocalNodeInfo = NULL;
    _pPeerState = NULL;
}

PtrLList<String> * GroupMembershipService::getAllLocalSubscriptions()
{
    return _pLocalNodeInfo->getAllSubscribedGroups();
}

//------------------------------------------------------------------------------
// MessagingService
//------------------------------------------------------------------------------

const char * DEFAULT_LOG_MSG = "MessagingService message";

MessagingService::MessagingService (DisseminationService *pDisService)
    : ServiceBase (pDisService)
{
    _pDataCacheInterface = getDisService()->getDataCacheInterface();
    if (_pDataCacheInterface == NULL) {
        checkAndLogMsg ("MessagingService::MessagingService", Logger::L_SevereError,
                        "MessagingController could not be initialized.  _pDataCacheInterface is NULL.\n");
        exit (-1);
    }
}

MessagingService::~MessagingService()
{
    _pDataCacheInterface = NULL;
}

TransmissionHistoryInterface * MessagingService::getTrasmissionHistoryInterface()
{
    return getDisService()->getTrasmissionHistoryInterface();
}

TransmissionService * MessagingService::getTrasmissionService()
{
    return getDisService()->_pTrSvc;
}

bool MessagingService::clearToSend (const char *pszInterface)
{
    return getDisService()->clearToSend (pszInterface);
}

bool MessagingService::clearToSendOnAllInterfaces (void)
{
    return getDisService()->clearToSendOnAllInterfaces();
}

int MessagingService::broadcastCtrlMessage (DisServiceCtrlMsg *pCtrlMsg, const char **ppszOutgoingInterfaces,
                                            const char *pszLogMsg, const char *pszTargetAddr, const char *pszHints)
{
    if (getDisService() == NULL) {
        return -1;
    }
    if (pCtrlMsg->getSenderNodeId() == NULL) {
        pCtrlMsg->setSenderNodeId (getDisService()->getNodeId());
    }
    return getDisService()->broadcastDisServiceCntrlMsg (pCtrlMsg, ppszOutgoingInterfaces, pszLogMsg, pszTargetAddr, pszHints);
}

int MessagingService::broadcastCtrlMessage (DisServiceCtrlMsg *pCtrlMsg, const char *pszLogMsg, const char *pszTargetAddr, const char *pszHints)
{
    if (getDisService() == NULL) {
        return -1;
    }
    if (pCtrlMsg->getSenderNodeId() == NULL) {
        pCtrlMsg->setSenderNodeId (getDisService()->getNodeId());
    }
    return getDisService()->broadcastDisServiceCntrlMsg (pCtrlMsg, NULL, pszLogMsg, pszTargetAddr, pszHints);
}

int MessagingService::broadcastDataMessage (const char *pszMessageID, const char **pszTargetNodeIds, int64 i64TimeOut,
                                            uint8 *pUi8Priorities, bool bRequireAck, const char *pszLogMsg, const char *pszHints)
{
    if ((pszMessageID == NULL) || (pszTargetNodeIds == NULL)) {
        return -9;
    }
    int rc;
    for (int i = 0; pszTargetNodeIds[i]; i++) {
        if ((rc = broadcastDataMessage (pszMessageID, pszTargetNodeIds[i], i64TimeOut,
                                        checkAngGetProperty (pUi8Priorities, i),
                                        bRequireAck, pszLogMsg, pszHints)) < 0) {
            return rc;
        }
    }
    return 0;
}

int MessagingService::broadcastDataMessage (const char **ppszMessageIDs, const char *pszTargetNodeId, int64 i64TimeOut,
                                            uint8 *pUi8Priorities, bool bRequireAck, const char *pszLogMsg, const char *pszHints)
{
    if (ppszMessageIDs == NULL) {
        return -8;
    }
    int rc;
    for (int i = 0; ppszMessageIDs[i]; i++) {
        if ((rc = broadcastDataMessage (ppszMessageIDs[i], pszTargetNodeId, i64TimeOut,
                                        checkAngGetProperty (pUi8Priorities, i),
                                        bRequireAck, pszLogMsg, pszHints)) < 0) {
            return rc;
        }
    }
    return 0;
}

int MessagingService::broadcastDataMessage (const char *pszMessageID, const char *pszTargetNodeId, int64 i64TimeOut,
                                            uint8 ui8Priority, bool bRequireAck, const char *pszLogMsg, const char *pszHints)
{
    if (_pDataCacheInterface == NULL) {
        return -7;
    }
    MessageHeader *pMH = _pDataCacheInterface->getMessageInfo (pszMessageID);
    if (pMH == NULL) {
        checkAndLogMsg ("MessagingService::sendMessage", Logger::L_Warning,
                        "Can not send message %s; MessageInfo could not be found/instantiated\n",
                        pszMessageID);
        return -6;
    }

    pMH->setPriority (ui8Priority);
    pMH->setAcknowledgment (bRequireAck);

    int rc = broadcastDataMessage (pMH, pszTargetNodeId, i64TimeOut, pszLogMsg, pszHints);
    _pDataCacheInterface->release (pMH->getMsgId(), pMH);
    return rc;
}

int MessagingService::broadcastDataMessage (MessageHeader *pMH, const char *pszTargetNodeId,
                                            int64 i64TimeOut, const char *pszLogMsg, const char *pszHints)
{
    if (_pDataCacheInterface == NULL) {
        return -5;
    }

    if (pMH == NULL) {
        checkAndLogMsg ("MessagingService::sendMessage", Logger::L_Warning,
                        "pMH is NULL.\n");
        return -4;
    }

    const void *pData = _pDataCacheInterface->getData (pMH->getMsgId());
    if (pData == NULL) {
        checkAndLogMsg ("MessagingService::sendMessage", Logger::L_Warning,
                        "Can not send message %s; pData could not be found/instantiated\n",
                        pMH->getMsgId());
        return -3;
    }

    Message msg (pMH, pData);
    DisServiceDataMsg dsMsg (getDisService()->getNodeId(), &msg);
    if (pszTargetNodeId != NULL) {
        dsMsg.setTargetNodeId (pszTargetNodeId);
    }
    if ((pszHints != NULL) && (strstr (pszHints, "DoNotForward"))) {
        dsMsg.setDoNotForward (true);
    }

    int rc = broadcastDataMessage (&dsMsg, pszLogMsg, NULL, NULL, pszHints);
    _pDataCacheInterface->release (pMH->getMsgId(), (void*)pData);

    return rc;
}

int MessagingService::broadcastDataMessage (DisServiceDataMsg *pDSMsg, const char *pszLogMsg,
                                            const char **pszOutgoingInterfaces, const char *pszTargetAddr, const char *pszHints)
{
    if (pDSMsg == NULL) {
        return -1;
    }

    if (pDSMsg->getSenderNodeId() == NULL) {
        pDSMsg->setSenderNodeId (getDisService()->getNodeId());
    }
    return getDisService()->broadcastDisServiceDataMsg (pDSMsg, pszLogMsg, pszOutgoingInterfaces, pszTargetAddr, pszHints);
}

int MessagingService::transmitCtrlMessage (DisServiceCtrlMsg *pCtrlMsg, const char *pszLogMsg)
{
    if (pCtrlMsg == NULL) {
        return -1;
    }

    if (pCtrlMsg->getSenderNodeId() == NULL) {
        pCtrlMsg->setSenderNodeId (getDisService()->getNodeId());
    }
    return getDisService()->transmitDisServiceControllerMsg (pCtrlMsg, true, pszLogMsg);
}

int MessagingService::transmitCtrlToCtrlMessage (ControllerToControllerMsg *pCtrlMsg,
                                                 const char *pszLogMsg)
{
    if (pCtrlMsg == NULL) {
        return -1;
    }

    if (pCtrlMsg->getSenderNodeId() == NULL) {
        pCtrlMsg->setSenderNodeId (getDisService()->getNodeId());
    }
    return getDisService()->transmitDisServiceControllerMsg (pCtrlMsg, true, pszLogMsg);
}

int MessagingService::transmitUnreliableCtrlToCtrlMessage (ControllerToControllerMsg *pCtrlMsg,
                                                           const char *pszLogMsg)
{
    if (pCtrlMsg == NULL) {
        return -2;
    }

    if (pCtrlMsg->getSenderNodeId() == NULL) {
        pCtrlMsg->setSenderNodeId (getDisService()->getNodeId());
    }
    return getDisService()->transmitDisServiceControllerMsg (pCtrlMsg, false, pszLogMsg);
}

//------------------------------------------------------------------------------
// TopologyController
//------------------------------------------------------------------------------

TopologyService::TopologyService (DisseminationService *pDisService)
    : ServiceBase (pDisService)
{
    _pPeerState = pDisService->getPeerState();
}

TopologyService::~TopologyService()
{
    _pPeerState = NULL;
}

RemoteNodeInfo * TopologyService::getNeighborNodeInfo (const char *pszRemoteNodeId)
{
    // TODO: implement this
    return NULL;
}

//------------------------------------------------------------------------------
// SearchService
//------------------------------------------------------------------------------

SearchService::SearchService (DisseminationService *pDisService)
    : ServiceBase (pDisService),
      _notifiedSearches (pDisService->getNodeId(),
                         pDisService->_pDataCacheInterface->getStorageInterface()->getPropertyStore())
{
}

SearchService::~SearchService()
{
}

char * SearchService::getSearchId (const char *pszGroupName, const char *pszPublisherNodeId,
                                   PropertyStoreInterface *pPropertyStore)
{
    if (pszGroupName == NULL || pszPublisherNodeId ==  NULL) {
        return NULL;
    }

    // Retrieve search sequence id
    static Mutex m;
    static const char * CURRENT_SEARCH_SEQUENCE_ID_ATTRIBUTE = "search_seq_id";
    static const char * SEARCH_ID_SEPARATOR = ":";

    m.lock();
    const String sSearchSeqId (pPropertyStore->get (pszPublisherNodeId, CURRENT_SEARCH_SEQUENCE_ID_ATTRIBUTE));
    uint32 ui32SearchSeqId = 0;
    if (sSearchSeqId.length() > 0) {
        ui32SearchSeqId = atoui32 (sSearchSeqId);
        ui32SearchSeqId++;
    }

    // Create search id
    String id ("search:");
    id += pszGroupName;
    id += SEARCH_ID_SEPARATOR;
    id += pszPublisherNodeId;
    id += SEARCH_ID_SEPARATOR;
    char buf[23];
    i64toa (buf, ui32SearchSeqId);
    id += buf;

    // Store new search sequence id
    int rc;
    if (sSearchSeqId.length() > 0) {
        rc = pPropertyStore->update (pszPublisherNodeId, CURRENT_SEARCH_SEQUENCE_ID_ATTRIBUTE, buf);
    }
    else {
        rc = pPropertyStore->set (pszPublisherNodeId, CURRENT_SEARCH_SEQUENCE_ID_ATTRIBUTE, buf);
    }

    m.unlock();

    if (rc == 0) {
        return id.r_str();
    }

    return NULL;
}

bool SearchService::messageNotified (const char *pszMessageId, uint16 ui16ClientId, const char *pszQueryId)
{
    return _notifiedSearches.messageNotified (pszQueryId, ui16ClientId, pszMessageId);
}

void SearchService::notifyClients (const char *pszMessageId, const char *pszSearchId, uint16 ui16ClientId)
{
    if ((pszMessageId == NULL) || (pszSearchId == NULL)) {
        return;
    }
    DisseminationService *pDisService = getDisService();
    if (pDisService == NULL) {
        return;
    }
    if (_notifiedSearches.messageNotified (pszSearchId, ui16ClientId, pszMessageId)) {
        // The message has already been notified to the client as match to the
        // current query - no need to notify the client again
        return;
    }
    DisseminationService::MessageToNotifyToClient *pMsgToNtfy = new DisseminationService::MessageToNotifyToClient();
    if (pMsgToNtfy == NULL) {
        return;
    }
    pMsgToNtfy->ui16ClientId = ui16ClientId;
    pMsgToNtfy->msgId = pszMessageId;
    pMsgToNtfy->searchId = pszSearchId;

    pDisService->_mAsynchronousNotify.lock (1000);
    if (pDisService->_pMessagesToNotify->search (pMsgToNtfy) == NULL) {
        pDisService->_pMessagesToNotify->prepend (pMsgToNtfy);
    }
    else {
        delete pMsgToNtfy;
    }
    pDisService->_mAsynchronousNotify.unlock (1000);
}

void SearchService::setNotifiedClients (const char *pszMessageId, const char *pszSearchId, uint16 ui16ClientId)
{
    _notifiedSearches.put (pszSearchId, ui16ClientId, pszMessageId);
}

//------------------------------------------------------------------------------
// SearchService::NotifiedSearches
//------------------------------------------------------------------------------

const char * SearchService::NotifiedSearches::PROPERTY_NAME = "DS_NOTIFIED_SEARCHES";
const char * SearchService::NotifiedSearches::NOTIFIED = "1";

SearchService::NotifiedSearches::NotifiedSearches (const char *pszNodeId, PropertyStoreInterface *pPropertyStore)
    : _nodeId (pszNodeId),
      _pPropertyStore (pPropertyStore)
{
}

SearchService::NotifiedSearches::~NotifiedSearches (void)
{
    _pPropertyStore = NULL;
}

void SearchService::NotifiedSearches::put (const char *pszQueryId, uint16 ui16ClientId, const char *pszMessageId)
{
    if ((pszQueryId == NULL) || (pszMessageId == NULL)) {
        return;
    }

    String att (getAttribute (pszQueryId, ui16ClientId, pszMessageId));
    if (_pPropertyStore->set (_nodeId, att, NOTIFIED) < 0) {
        
    }
}

bool SearchService::NotifiedSearches::messageNotified (const char *pszQueryId, uint16 ui16ClientId, const char *pszMessageId)
{
    if ((pszQueryId == NULL) || (pszMessageId == NULL)) {
        return false;
    }

    String att (getAttribute (pszQueryId, ui16ClientId, pszMessageId));
    String val = _pPropertyStore->get (_nodeId, att);
    return (val == NOTIFIED);
}

String SearchService::NotifiedSearches::getAttribute (const char *pszQueryId, uint16 ui16ClientId, const char *pszMessageId)
{
    String val (PROPERTY_NAME);
    val += "-";
    val += pszQueryId;
    val += "-";
    val += (uint32) ui16ClientId;
    val += "-";
    val += pszMessageId;
    return val;
}

