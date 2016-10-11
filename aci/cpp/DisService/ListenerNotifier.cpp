/*
 * ListenerNotifier.cpp
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

#include "ListenerNotifier.h"

#include "MessageInfo.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//------------------------------------------------------------------------------
// DataCacheListenerNotifier
//------------------------------------------------------------------------------

DataCacheListenerNotifier::DataCacheListenerNotifier()
{
}

DataCacheListenerNotifier::~DataCacheListenerNotifier()
{
}

void DataCacheListenerNotifier::dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->dataCacheUpdated (pMH, pPayLoad);
        }
    }
}

void DataCacheListenerNotifier::dataCacheUpdatedNoNotify (MessageHeader *pMH, const void *pPayLoad, unsigned int uiListenerID)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL && (i != uiListenerID)) {
            _listeners[i].pListener->dataCacheUpdated (pMH, pPayLoad);
        }
    }
}

void DataCacheListenerNotifier::capacityReached()
{
    _m.lock (162);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->capacityReached();
        }
    }
    _m.unlock (162);
}

void DataCacheListenerNotifier::thresholdCapacityReached (uint32 ui32Length)
{
    _m.lock (163);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->thresholdCapacityReached (ui32Length);
        }
    }
    _m.unlock (163);
}

void DataCacheListenerNotifier::spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo, void *pIncomingData)
{
    _m.lock (164);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->spaceNeeded (ui32bytesNeeded, pIncomingMgsInfo, pIncomingData);
        }
    }
    _m.unlock (164);
}

int DataCacheListenerNotifier::cacheCleanCycle()
{
    _m.lock (165);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->cacheCleanCycle();
        }
    }
    _m.unlock (165);
    return 0;
}

//------------------------------------------------------------------------------
// MessageListenerNotifier
//------------------------------------------------------------------------------

MessageListenerNotifier::MessageListenerNotifier()
{
}

MessageListenerNotifier::~MessageListenerNotifier()
{  
}

void MessageListenerNotifier::newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                  DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                                  const char *pszIncomingInterface)
{
    _m.lock (166);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->newIncomingMessage (pMsgMetaData, ui16MsgMetaDataLen, pDisServiceMsg,
                                                         ui32SourceIPAddress, pszIncomingInterface);
        }
    }
    _m.unlock (166);
}

//------------------------------------------------------------------------------
// GroupMembershipListenerNotifier
//------------------------------------------------------------------------------

GroupMembershipListenerNotifier::GroupMembershipListenerNotifier()
{
}

GroupMembershipListenerNotifier::~GroupMembershipListenerNotifier()
{  
}

void GroupMembershipListenerNotifier::newSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
    _m.lock (167);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->newSubscriptionForPeer (pszPeerNodeId, pSubscription);
        }
    }
    _m.unlock (167);
}

void GroupMembershipListenerNotifier::removedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
    _m.lock (168);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->removedSubscriptionForPeer (pszPeerNodeId, pSubscription);
        }
    }
    _m.unlock (168);
}

void GroupMembershipListenerNotifier::modifiedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription)
{
    _m.lock (169);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->modifiedSubscriptionForPeer (pszPeerNodeId, pSubscription);
        }
    }
    _m.unlock (169);
}

//------------------------------------------------------------------------------
// NetworkStateListenerNotifier
//------------------------------------------------------------------------------

NetworkStateListenerNotifier::NetworkStateListenerNotifier()
{
}

NetworkStateListenerNotifier::~NetworkStateListenerNotifier()
{  
}

void NetworkStateListenerNotifier::networkQuiescent (const char **ppszInterfaces)
{
    _m.lock (170);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->networkQuiescent (ppszInterfaces);
        }
    }
    _m.unlock (170);
}

//------------------------------------------------------------------------------
// PeerStateListenerNotifier
//------------------------------------------------------------------------------

PeerStateListenerNotifier::PeerStateListenerNotifier()
{
}

PeerStateListenerNotifier::~PeerStateListenerNotifier()
{  
}

void PeerStateListenerNotifier::newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                             const char *pszIncomingInterface)
{
    _m.lock (171);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->newNeighbor (pszNodeUID, pszPeerRemoteAddr,
                                                  pszIncomingInterface);
        }
    }
    _m.unlock (171);
}

void PeerStateListenerNotifier::deadNeighbor (const char *pszNodeUID)
{
    _m.lock (172);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->deadNeighbor (pszNodeUID);
        }
    }
    _m.unlock (172);
}

void PeerStateListenerNotifier::newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                                   const char *pszIncomingInterface)
{
    _m.lock (171);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->newLinkToNeighbor (pszNodeUID, pszPeerRemoteAddr,
                                                        pszIncomingInterface);
        }
    }
    _m.unlock (171);
}

void PeerStateListenerNotifier::droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr)
{
    _m.lock (171);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->droppedLinkToNeighbor (pszNodeUID, pszPeerRemoteAddr);
        }
    }
    _m.unlock (171);
}

void PeerStateListenerNotifier::stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate)
{
    _m.lock (173);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->stateUpdateForPeer (pszNodeUID, pUpdate);
        }
    }
    _m.unlock (173);
}

//------------------------------------------------------------------------------
// SearchNotifier
//------------------------------------------------------------------------------

SearchNotifier::SearchNotifier (void)
    : SearchListener ("SearchNotifier")
{
}

SearchNotifier::~SearchNotifier (void)
{
    
}

void SearchNotifier::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                    const char *pszQuerier, const char *pszQueryType,
                                    const char *pszQueryQualifiers,
                                    const void *pszQuery, unsigned int uiQueryLen)
{
    _m.lock (174);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->searchArrived (pszQueryId, pszGroupName, pszQuerier, pszQueryType,
                                                    pszQueryQualifiers, pszQuery, uiQueryLen);
        }
    }
    _m.unlock (174);
}

void SearchNotifier::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds,
                                         const char *pszMatchingNodeId)
{
    _m.lock (174);
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->searchReplyArrived (pszQueryId, ppszMatchingMessageIds,
                                                         pszMatchingNodeId);
        }
    }
    _m.unlock (174);
}

void SearchNotifier::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen,
                                                 const char *pszMatchingNodeId)
{
    _m.lock (174);
    for (unsigned int i = 0; i < _listeners.size (); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != NULL) {
            _listeners[i].pListener->volatileSearchReplyArrived (pszQueryId, pReply, ui162ReplyLen, pszMatchingNodeId);
        }
    }
    _m.unlock (174);
}

