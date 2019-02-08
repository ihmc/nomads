/*
 * CommAdaptorListenerNotifier.cpp
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

#include "CommAdaptorListenerNotifier.h"

#include "MessageProperties.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

CommAdaptorListenerNotifier::CommAdaptorListenerNotifier()
{
}

CommAdaptorListenerNotifier::~CommAdaptorListenerNotifier()
{
}

int CommAdaptorListenerNotifier::registerCommAdaptorListener (CommAdaptorListener *pListener, unsigned int &uiListenerId)
{
    if (pListener == nullptr) {
        return -1;
    }
    int iListenerId = _listeners.firstFree();
    if (iListenerId < 0) {
        return -2;
    }
    _listeners[iListenerId].pListener = pListener;
    uiListenerId = (unsigned int) iListenerId;
    return 0;
}

int CommAdaptorListenerNotifier::deregisterCommAdaptorListener (unsigned int uiListenerId)
{
    return _listeners.clear (uiListenerId);
}

int CommAdaptorListenerNotifier::contextUpdateMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                              const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->contextUpdateMessageArrived (uiAdaptorId, pszSenderNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::contextVersionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                               const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->contextVersionMessageArrived (uiAdaptorId, pszSenderNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::dataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                                              const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->dataArrived (pAdaptorProperties, pProperties, pBuf, ui32Len, ui8NChunks, ui8TotNChunks);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::metadataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                                                  const void *pBuf, uint32 ui32Len, const char *pszReferredDataId)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->metadataArrived (pAdaptorProperties, pProperties, pBuf, ui32Len, pszReferredDataId);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::messageRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                               const char *pszPublisherNodeId, const char *pszMsgId)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->messageRequestMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId, pszMsgId);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::chunkRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                             const char *pszPublisherNodeId, const char *pszMsgId,
                                                             DArray<uint8> *pCachedChunks)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
           _listeners[i].pListener->chunkRequestMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId,
                                                                pszMsgId, pCachedChunks);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::positionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                         const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->positionMessageArrived (uiAdaptorId, pszSenderNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::searchMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                       SearchProperties *pSearchProp)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->searchMessageArrived (uiAdaptorId, pszSenderNodeId, pSearchProp);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                            const char *pszQueryId , const char **ppszMatchingMsgIds,
                                                            const char *pszTarget, const char *pszMatchingNodeId)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->searchReplyMessageArrived (uiAdaptorId, pszSenderNodeId, pszQueryId,
                                                                ppszMatchingMsgIds, pszTarget, pszMatchingNodeId);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                            const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                                            const char *pszTarget, const char *pszMatchingNodeId)
{
    for (unsigned int i = 0; i < _listeners.size (); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->searchReplyMessageArrived (uiAdaptorId, pszSenderNodeId, pszQueryId,
                                                                pReply, ui16ReplyLen, pszTarget, pszMatchingNodeId);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::topologyReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                              const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->topologyReplyMessageArrived (uiAdaptorId, pszSenderNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::topologyRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                                const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->topologyRequestMessageArrived (uiAdaptorId, pszSenderNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::updateMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                       const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->updateMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::versionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                        const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->versionMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::waypointMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                         const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->waypointMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

int CommAdaptorListenerNotifier::wholeMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                      const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->wholeMessageArrived (uiAdaptorId, pszSenderNodeId, pszPublisherNodeId, pBuf, ui32Len);
        }
    }
    return 0;
}

void CommAdaptorListenerNotifier::newPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId, const char *pszPeerRemoteAddress,
                                           const char *pszIncomingInterface)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->newPeer (pAdaptorProperties, pszDstPeerId, pszPeerRemoteAddress, pszIncomingInterface);
        }
    }
}

void CommAdaptorListenerNotifier::deadPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->deadPeer (pAdaptorProperties, pszDstPeerId);
        }
    }
}

void CommAdaptorListenerNotifier::newLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                                 const char *pszIncomingInterface)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->newLinkToPeer (pAdaptorProperties, pszNodeUID, pszPeerRemoteAddr, pszIncomingInterface);
        }
    }
}

void CommAdaptorListenerNotifier::droppedLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID, const char *pszPeerRemoteAddr)
{
    for (unsigned int i = 0; i < _listeners.size(); i++) {
        if (_listeners.used (i) && _listeners[i].pListener != nullptr) {
            _listeners[i].pListener->droppedLinkToPeer (pAdaptorProperties, pszNodeUID, pszPeerRemoteAddr);
        }
    }
}

CommAdaptorListenerNotifier::ListenerWrapper::ListenerWrapper (void)
{
    pListener = nullptr;
}

CommAdaptorListenerNotifier::ListenerWrapper::~ListenerWrapper (void)
{
}
