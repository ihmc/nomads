/*
 * SubscriptionForwardingController.cpp
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

#include "SubscriptionForwardingController.h"
#include "DisServiceMsg.h"
#include "DisseminationService.h"
#include "NodeInfo.h"
#include "Logger.h"
#include "DisServiceDefs.h"
#include "NLFLib.h"


using namespace IHMC_ACI;
using namespace NOMADSUtil;


SubscriptionForwardingController::SubscriptionForwardingController (DisseminationService *pDisService)
    : MessageListener()
{
    _ui16AdvThreshold = DEFAULT_ADV_THRESHOLD;
    _i64LastAdvTime = 0;
    _ui16LiveNeighbors = 0;
    _pDisService = pDisService;
}

SubscriptionForwardingController::~SubscriptionForwardingController()
{
    
}

void SubscriptionForwardingController::newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, 
                                                           DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                                           const char *pszIncomingInterface)

{
    const char *pszMethodName = "newIncomingMessage";
    if (pDisServiceMsg != NULL && pDisServiceMsg->getType() == DisServiceMsg::DSMT_SubAdvMessage) {
        DisServiceSubscribtionAdvertisement *pSubMsg = (DisServiceSubscribtionAdvertisement*)pDisServiceMsg;
        CtrlSeqNo *pCtrlNo = _lastSeenCtrlSeqNoHashtable.get (pSubMsg->getSenderNodeId());
        // Update the last seen control message and return since this message has been
        // received already
        if (pCtrlNo != NULL && pCtrlNo->_ui32SeqNo == pSubMsg->getCtrlMsgSeqNo()) {
            pCtrlNo = _lastSeenCtrlSeqNoHashtable.put (pSubMsg->getSenderNodeId(),
                                                       new CtrlSeqNo (pSubMsg->getCtrlMsgSeqNo(), getTimeInMilliseconds()));
            
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Control Message from %s with SeqId = %d  has been received already. Drop it.\n",
                            pSubMsg->getSenderNodeId(), pSubMsg->getCtrlMsgSeqNo());
            delete pCtrlNo;
            return;
        }
        
        // Check if the current node is in the path. If so drop it because it is a loop.
        NOMADSUtil::String *pNodeInPath = pSubMsg->getPath()->getFirst();
        for (; pNodeInPath != NULL; pNodeInPath = pSubMsg->getPath()->getNext()) {
            if (strcmp (_pDisService->getNodeId(), pNodeInPath->c_str()) == 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "Control Message contains the current node in the Path. Drop it.\n");
                return;
            }
        }
        // Keep track of the control message
        pCtrlNo = _lastSeenCtrlSeqNoHashtable.put (pSubMsg->getSenderNodeId(),
                                                   new CtrlSeqNo (pSubMsg->getCtrlMsgSeqNo(), getTimeInMilliseconds()));
        if (pCtrlNo != NULL) {
            delete pCtrlNo;
        }
        
        // Remove expired Control Sequence Number
        NOMADSUtil::DArray2<const char *> seqNumExpired;
        NOMADSUtil::StringHashtable<CtrlSeqNo>::Iterator i = _lastSeenCtrlSeqNoHashtable.getAllElements();
        for (unsigned int k = 0; !i.end(); i.nextElement(), k++) {
            CtrlSeqNo *pSeqNum = i.getValue();
            if ((getTimeInMilliseconds() - pSeqNum->_i64TimeStamp) > _i64SeqNoLifetimeExpirationThreshold) {
                seqNumExpired[k] = i.getKey();
            }
        }
        
        for (unsigned int k = 0; k < seqNumExpired.size(); k++) {
            if (seqNumExpired.used (k)) {
                CtrlSeqNo *pSeqNum = _lastSeenCtrlSeqNoHashtable.remove (seqNumExpired[k]);
                if (pSeqNum != NULL) {
                    delete pSeqNum;
                }
            }
        }

        // Update the Subscription Advertisement Table
        int iRet = updateTable (pSubMsg->getOriginatorNodeId(), pSubMsg->getSubscriptions(), pSubMsg->getPath());
        // Display the SubscriptionAdvTable
        printf ("********************** UPDATED TABLE *****************************************\n");
        _subAdvTable.display();
        printf ("\n***********************************************************************************\n");
        if (iRet < 0) {
            printf ("\nUpdate Subscription Table Fails.\n");
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Update Subscription Table Fails.\n");
        }
        // Re broadcast the message preappending the node id.
        const char *pszSenderNodeId = _pDisService->getNodeId();
        if (pSubMsg->prependNode (pszSenderNodeId) < 0) {
             checkAndLogMsg (pszMethodName, Logger::L_Info, "Path update failure... Returning.\n");
             return;
        }
        
        pSubMsg->setSenderNodeId (pszSenderNodeId);
        const char *pszPurpose = "Sending Subscription Advertisement Message";
        _pDisService->broadcastDisServiceCntrlMsg (pSubMsg, NULL, pszPurpose, NULL);
    }
}

void SubscriptionForwardingController::newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                                    const char *pszIncomingInterface)

{
    _ui16LiveNeighbors++;
}

void SubscriptionForwardingController::deadNeighbor (const char *pszNodeUID)
{
    _ui16LiveNeighbors--;
    _subAdvTable.deadNode (pszNodeUID);
}

void SubscriptionForwardingController::newLinkToNeighbor (const char *pszNodeUID,
                                                          const char *pszPeerRemoteAddr,
                                                          const char *pszIncomingInterface)
{
}

void SubscriptionForwardingController::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{
}
           
void SubscriptionForwardingController::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
}

void SubscriptionForwardingController::setAdvertisementThreshold (uint16 ui16AdvThreshold)
{
    _ui16AdvThreshold = ui16AdvThreshold;
}

void SubscriptionForwardingController::setAdvertisementPeriod (int64 i64AdvPeriod)
{
    _i64AdvPeriod = i64AdvPeriod;
}

int SubscriptionForwardingController::updateTable (const char *pszSenderNodeId, NOMADSUtil::PtrLList<NOMADSUtil::String> *pSubList,
                                                   NOMADSUtil::PtrLList<NOMADSUtil::String> *pNodeList)
 
{
    // Refresh the table to delete expired entries
    if (!_subAdvTable.isEmpty()) {
        _subAdvTable.refreshTable();
    }
    
    for (NOMADSUtil::String *pszSub = pSubList->getFirst(); pszSub != NULL;
            pszSub = pSubList->getNext()) {

        // If the current subscription is not in the table yet
        // add the Sender Node Id and its path (if any) to the table at the
        // corresponding subscription.
        if (!_subAdvTable.hasSubscription (pszSub->c_str())) {
            // Add the Path if any
            if (pNodeList->getCount() > 0) {
                NOMADSUtil::String *pszPath = new NOMADSUtil::String();
                for (NOMADSUtil::String *pszNode = pNodeList->getFirst(); pszNode != NULL;
                     pszNode = pNodeList->getNext()) {

                    (*pszPath) += (*pszNode);
                    (*pszPath) += ":";
                }
                if (_subAdvTable.addSubscribingNodePath (pszSub->c_str(), pszSenderNodeId, pszPath) < 0) {
                    return -1;
                }
            } 
            else {
                if (_subAdvTable.addSubscribingNode (pszSub->c_str(), pszSenderNodeId) < 0) {
                    return -1;
                }
            }
        } 
        else {
            // Get the Subscription and update the Entry.
            SubscriptionAdvTableEntry *pEntry = _subAdvTable.getTableEntry (pszSub->c_str());
            // If the node is not bound to the current subscription, add it along its path (if any)
            if (!pEntry->hasSubscribingNode (pszSenderNodeId)) { 
                if (pNodeList->getCount() > 0) {
                    NOMADSUtil::String *pszPath = new NOMADSUtil::String();
                    for (NOMADSUtil::String *pszNode = pNodeList->getFirst(); pszNode != NULL;
                            pszNode = pNodeList->getNext()) {

                        (*pszPath) += (*pszNode);
                        (*pszPath) += ":";
                    }
                    if (_subAdvTable.addSubscribingNodePath (pszSub->c_str(),
                        pszSenderNodeId, pszPath) < 0) {

                        return -1;
                    }
                } 
                else {
                    if (_subAdvTable.addSubscribingNode (pszSub->c_str(), pszSenderNodeId) < 0) {
                        return -1;                            
                    }
                }
            } 
            else {
                // Check if the message contains a Path. If so check whether this path is
                // included in the table already. If not, add it to the table.
                if (pNodeList->getCount() > 0) {
                    NOMADSUtil::PtrLList<NOMADSUtil::String> *pPathList = pEntry->getSubscribingNode (pszSenderNodeId)->getPathList();
                    NOMADSUtil::String *pszPath = new NOMADSUtil::String();
                    for (NOMADSUtil::String *pszNode = pNodeList->getFirst(); pszNode != NULL;
                            pszNode = pNodeList->getNext()) {

                        (*pszPath) += (*pszNode);
                        (*pszPath) += ":";
                    }
                    // If the path doesn't exist, add it.
                    if (pPathList->search (pszPath) == NULL) {
                        if (pEntry->addPathToNode (pszSenderNodeId, pszPath) < 0) {
                            return -1;
                        }
                    }
                    pEntry->refreshLifeTime (pszSenderNodeId);
                }
            }            
        }
    }
    return 0;
}

void SubscriptionForwardingController::advertiseSubscriptions()
{
    if ((getTimeInMilliseconds() - _i64LastAdvTime > _i64AdvPeriod) && 
         _ui16LiveNeighbors >= _ui16AdvThreshold) {
        
        _i64LastAdvTime = getTimeInMilliseconds();
        // Get all the Subscriptions
        NOMADSUtil::PtrLList<NOMADSUtil::String> *pSubList = _pDisService->_pLocalNodeInfo->getAllSubscriptions();
        if (pSubList != NULL) {
            // Create the SubscriptionAdvertisement Message and broadcast it
            const char *pszPurpose = "Sending Subscription Advertisement Message";
            const char *pszNodeId = _pDisService->getNodeId();
            DisServiceSubscribtionAdvertisement disServiceSubAdvMsg (pszNodeId, pszNodeId, pSubList);
            _pDisService->broadcastDisServiceCntrlMsg (&disServiceSubAdvMsg, NULL, pszPurpose, NULL);
            // Delete SubscriptionList
            NOMADSUtil::String *pCurr, *pNext, *pDel;
            pNext = pSubList->getFirst();
            while ((pCurr = pNext) != NULL) {
                pNext = pSubList->getNext();
                pDel = pSubList->remove (pCurr);
                delete pDel;
            }
            
            delete pSubList;
        }        
    }
}

void SubscriptionForwardingController::configureTable (int64 i64LifeTimeUpdateThreshold, int64 i64SeqNoLifetimeExpirationThreshold)
{
    _subAdvTable.configureTimers (i64LifeTimeUpdateThreshold);
    _i64SeqNoLifetimeExpirationThreshold = i64SeqNoLifetimeExpirationThreshold;
}

SubscriptionForwardingController::CtrlSeqNo::CtrlSeqNo (uint32 ui32SeqNo, int64 i64TimeStamp)
{
    _ui32SeqNo = ui32SeqNo;
    _i64TimeStamp = i64TimeStamp;
}


