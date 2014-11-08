/*
 * DataCacheReplicationController.cpp
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

#include "DataCacheReplicationController.h"

#include "ControllerFactory.h"
#include "DataCacheInterface.h"
#include "DisseminationService.h"
#include "DisseminationServiceProxyServer.h"
#include "DisServiceDataCacheQuery.h"
#include "DisServiceDefs.h"
#include "DisServiceMsg.h"
#include "Message.h"
#include "MessageInfo.h"
#include "NodeInfo.h"
#include "PeerState.h"

#include "Logger.h"
#include "StringHashtable.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#if defined (ANDROID)
    //Logger *pNetLog;
#else
    //extern Logger *pNetLog;
#endif

const bool DataCacheReplicationController::DEFAULT_REQUIRE_ACK = false;

DataCacheReplicationController::DataCacheReplicationController (Type type, DisseminationService *pDisService, bool bRequireAck)
    : DataCacheService (pDisService), MessagingService (pDisService),
      _mx (5)
{
    _type = type;
    _bRequireAck = bRequireAck;
    _pAck = _bRequireAck ? ControllerFactory::getAckControllerAndRegisterListeners() :
                           NULL;
    _pDataCacheInterface = DataCacheService::getDataCacheInterface();
}

DataCacheReplicationController::DataCacheReplicationController (Type type, DisseminationServiceProxyServer *pDisServiceProxy,
                                                                bool bRequireAck)
    : DataCacheService (pDisServiceProxy->getDisseminationServiceRef()),
      MessagingService (pDisServiceProxy->getDisseminationServiceRef()),
      _mx (5)
{
    _type = type;
    _bRequireAck = bRequireAck;
    _pAck = _bRequireAck ? ControllerFactory::getAckControllerAndRegisterListeners() :
                           NULL;
}

DataCacheReplicationController::~DataCacheReplicationController()
{
    if (_pAck != NULL) {
        if (_pAck->isRunning()) {
            _pAck->requestTerminationAndWait();
        }
        delete _pAck;
        _pAck = NULL;
    }
}

void DataCacheReplicationController::newIncomingMessage (const void *, uint16,
                                                         DisServiceMsg *pDisServiceMsg, uint32,
                                                         const char *pszIncomingInterface)
{
    _mx.lock (13);
    switch (pDisServiceMsg->getType()) {

        case DisServiceMsg::DSMT_Data: {
            disServiceDataMsgArrived ((DisServiceDataMsg*) pDisServiceMsg);
            break;
        }

        case DisServiceMsg::DSMT_CtrlToCtrlMessage: {
            disServiceControllerMsgArrived ((ControllerToControllerMsg*)pDisServiceMsg);
            break;
        }

        default: {
            // Ctrl Message
            disServiceControlMsgArrived ((DisServiceCtrlMsg*)pDisServiceMsg);
            break;
        }
    }
    _mx.unlock (13);
}

void DataCacheReplicationController::capacityReached()
{
}

void DataCacheReplicationController::thresholdCapacityReached (uint32)
{
}

void DataCacheReplicationController::spaceNeeded (uint32, MessageHeader *, void *)
{
}

int DataCacheReplicationController::cacheCleanCycle()
{
    return 0;
}
    
PtrLList<MessageHeader> * DataCacheReplicationController::lockAndQueryDataCache (const char *pszSQLStatement)
{
    _mx.lock (14);
    PtrLList<MessageHeader> *pRet = DataCacheService::lockAndQueryDataCache (pszSQLStatement);
    _mx.unlock (14);
    return pRet;
}

PtrLList<MessageHeader> * DataCacheReplicationController::lockAndQueryDataCache (DisServiceDataCacheQuery *pQuery)
{
    _mx.lock (15);
    PtrLList<MessageHeader> *pRet = DataCacheService::lockAndQueryDataCache (pQuery);
    _mx.unlock (15);
    return pRet;
}

void DataCacheReplicationController::releaseQueryResults()
{
    _mx.lock (16);
    DataCacheService::releaseQueryResults();
    _mx.unlock (16);
}

void DataCacheReplicationController::releaseQueryResults (NOMADSUtil::PtrLList<MessageHeader> *pMessageList)
{
    _mx.lock (17);
    DataCacheService::releaseQueryResults (pMessageList);
    _mx.unlock (17);
}

int DataCacheReplicationController::replicateMessages (const char **ppszMessageIDs, const char *pszReplicateOnNodeId,
                                                       int64 i64TimeOut, uint8 *pui8Priorities)
{
    int rc = 0;
    MessageHeader *pMI;
    for (int i = 0; ppszMessageIDs[i] != NULL; i++) {
        pMI = getMessageInfo (ppszMessageIDs[i]);
        if (pMI != NULL) {
            pMI->setPriority (pui8Priorities[i]);
            rc = replicateMessage (pMI, pszReplicateOnNodeId, i64TimeOut);
            DataCacheService::releaseQueryResults (pMI);
            if (rc < 0) {
                return rc;
            }
        }
    }
    return rc;
}

int DataCacheReplicationController::replicateMessage (const char *pszMessageID, const char **ppszReplicateOnNodeIds,
                                                      int64 i64TimeOut, uint8 *pui8Priorities)
{
    MessageHeader *pMI = getMessageInfo (pszMessageID);
    if (pMI == NULL) {
        return -1;
    }

    int rc = 0;
    for (int i = 0; ppszReplicateOnNodeIds[i] != NULL; i++) {
        if (pMI != NULL) {
            pMI->setPriority (pui8Priorities[i]);
            rc = replicateMessage (pMI, ppszReplicateOnNodeIds[i], i64TimeOut);
            if (rc < 0) {
                DataCacheService::releaseQueryResults (pMI);
                return rc;
            }
        }
    }
    DataCacheService::releaseQueryResults (pMI);
    return rc;
}

int DataCacheReplicationController::replicateMessage (const char *pszMessageId, const char *pszReplicationTargetNodeId, int64 i64TimeOut)
{
    if (pszMessageId == NULL) {
        return -1;
    }
    if (pszReplicationTargetNodeId == NULL) {
        return -2;
    }
    MessageHeader *pMH = getMessageInfo (pszMessageId);
    if (pMH == NULL) {
        return -3;
    }
    int rc;
    if (0 != (rc = replicateMessage (pMH, pszReplicationTargetNodeId, i64TimeOut))) {
        return -4;
    }
    return 0;
}

int DataCacheReplicationController::replicateMessage (MessageHeader *pMI, const char *pszReplicateOnNodeId, int64 i64TimeOut)
{
    int rc = 0;
    pMI->setAcknowledgment (_bRequireAck);
    if (_bRequireAck) {
        _pAck->ackRequest (pszReplicateOnNodeId, pMI);
    }
    else {
        String transmissionHints = "DoNotForward";
        RemoteNodeInfo *pTargetNodeInfo = MessagingService::getDisService()->getPeerState()->getPeerNodeInfo (pszReplicateOnNodeId);
        if (pTargetNodeInfo != NULL) {
            const char *pszTargetAddr = pTargetNodeInfo->getDefaultIPAddressAsString();
            if (pszTargetAddr != NULL) {
                transmissionHints += ";include=";
                transmissionHints += pszTargetAddr;
            }
        }
        rc = broadcastDataMessage (pMI, pszReplicateOnNodeId, i64TimeOut, "Replicate Data Message", transmissionHints);
    }

    if (rc == 0 && pNetLog != NULL) {
        // Extra debugging
        pNetLog->logMsg ("DataCacheReplicationController::replicateMessage", Logger::L_Info,
                         "broadcasted message %s to node %s\n",
                         pMI->getMsgId(), pszReplicateOnNodeId?pszReplicateOnNodeId : "<NULL>");
    }
    checkAndLogMsg ("DataCacheReplicationController::replicateMessage", Logger::L_Info,
                    "broadcasted message %s to node %s\n",
                    pMI->getMsgId(), pszReplicateOnNodeId?pszReplicateOnNodeId : "<NULL>");
    return rc;
}

void DataCacheReplicationController::lockDataCache()
{
    _pDataCacheInterface->lock();
}

void DataCacheReplicationController::releaseDataCache()
{
    _pDataCacheInterface->unlock();
}
