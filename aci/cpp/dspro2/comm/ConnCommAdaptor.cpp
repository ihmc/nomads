/*
 * CommCommAdaptor.cpp
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
 * Created on April 14, 2014, 7:14 PM
 */

#include "ConnCommAdaptor.h"

#include "Defs.h"

#include "SessionId.h"

#include "BufferWriter.h"
#include "Logger.h"
#include "NetUtils.h"
#include "NICInfo.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

ConnCommAdaptor::ConnCommAdaptor (AdaptorId uiId, AdaptorType adaptorType, bool bSupportsCaching,
                                  const char *pszNodeId, CommAdaptorListener *pListener, uint16 ui16Port)
    : CommAdaptor (uiId, adaptorType, bSupportsCaching, true, pszNodeId, pListener),
      _ui16Port (ui16Port)
{
}

ConnCommAdaptor::~ConnCommAdaptor (void)
{
    requestTerminationAndWait();
}

int ConnCommAdaptor::init (char **ppIfaces)
{
    bool bDeallocateIFs = false;
    if (ppIfaces == nullptr) {
        // If not network interfaces are specified, use all of them
        NICInfo **ppNICs = NetUtils::getNICsInfo();
        if (ppNICs != nullptr) {
            unsigned int i = 0;
            for (; ppNICs[i] != nullptr; i++);
            ppIfaces = (char **) calloc (i+1, sizeof (char *));
            for (i = 0; ppNICs[i] != nullptr; i++) {
                ppIfaces[i] = ppNICs[i]->getIPAddrAsString().r_str();
            }
            NetUtils::freeNICsInfo (ppNICs);
            bDeallocateIFs = true;
        }
    }
    if (ppIfaces != nullptr) {
        const String sessionId (SessionId::getInstance()->getSessionId());
        for (unsigned int i = 0; ppIfaces[i] != nullptr; i++) {
            ConnListener *pConnListener = getConnListener (ppIfaces[i], _ui16Port,
                                                           _nodeId.c_str(), sessionId,
                                                           _pListener, this);
            if (pConnListener == nullptr) {
                checkAndLogMsg ("ConnCommAdaptor::init", memoryExhausted);
            }
            else {
                checkAndLogMsg ("ConnCommAdaptor::init", Logger::L_Info,
                                "started ConnListener on interface %s:%u\n",
                                ppIfaces[i], _ui16Port);
                _listenersByInterfaceIP.put (ppIfaces[i], pConnListener);
            }
            if (bDeallocateIFs) {
                free (ppIfaces[i]);
            }
        }
    }

    if (bDeallocateIFs) {
        free (ppIfaces);
    }

    return 0;
}

void ConnCommAdaptor::addHandler (ConnHandler *pHandler)
{
    if (pHandler == nullptr) {
        return;
    }

    _mHandlers.lock();
    ConnHandler *pOldHandler = _handlersByPeerId.put (pHandler->getRemotePeerNodeId(), pHandler);
    _mHandlers.unlock();
    if (pOldHandler != nullptr) {
        pOldHandler->abortConnHandler();
        delete pOldHandler;
    }
    _pListener->newPeer (&_adptorProperties, pHandler->getRemotePeerNodeId(),
                         pHandler->getRemotePeerAddress(),
                         pHandler->getLocalPeerAddress());

    checkAndLogMsg ("ConnCommAdaptor::addHandler", Logger::L_Info,
                    "a ConnHandler for peer %s was added to the list of handlers\n",
                    pHandler->getRemotePeerNodeId());
}

int ConnCommAdaptor::connectToPeer (const char *pszRemotePeerAddr, uint16 ui16Port)
{
    if (pszRemotePeerAddr == nullptr) {
        return -1;
    }

    _mHandlers.lock();
    for (ConnHandlers::Iterator handlers = _handlersByPeerId.getAllElements();
         !handlers.end(); handlers.nextElement()) {
        if (strcmp (handlers.getValue()->getRemotePeerAddress(), pszRemotePeerAddr) == 0) {
            checkAndLogMsg ("ConnCommAdaptor::connectToPeer", Logger::L_Warning,
                            "trying to add a handler that already exists for %s.\n",
                            pszRemotePeerAddr);
            // An active handler for pszRemotePeerAddr already exists - return
            _mHandlers.unlock();
            return -2;
        }
    }
    const DisconnectedPeer peerToReconnect (pszRemotePeerAddr, ui16Port);
    _handlersToReconnectToByIPAddr.add (peerToReconnect);
    _mHandlers.unlock();

    DisconnectedPeer discPeer (pszRemotePeerAddr, ui16Port);
    _mDisconnectedPeers.lock();
    if (_disconnectedPeers.find (discPeer)) {
        checkAndLogMsg ("ConnCommAdaptor::connectToPeer", Logger::L_Warning,
                        "trying to add a DisconnectedPeer that already exists for %s.\n",
                        pszRemotePeerAddr);
        _mDisconnectedPeers.unlock();
        return -2;
    }

    int rc = _disconnectedPeers.enqueue (discPeer);
    checkAndLogMsg ("ConnCommAdaptor::connectToPeer", Logger::L_Info,
                    "%s added to the list of disconnected peers.\n",
                    pszRemotePeerAddr);
    _mDisconnectedPeers.unlock();
    return rc;
}

void ConnCommAdaptor::run()
{
    setName ("IHMC_ACI::ConnCommAdaptor");

    started();

    for (DisconnectedPeer discPeer; !terminationRequested();) {
        _mDisconnectedPeers.lock();
        _mHandlers.lock();
        // Check whether any of the current handlers got disconnected
        ConnHandlers::Iterator handlerIter = _handlersByPeerId.getAllElements();
        for (; !handlerIter.end(); handlerIter.nextElement()) {
            if (!handlerIter.getValue()->isConnected()) {
                _handlersToReconnectToByIPAddr.resetGet();
                // If the handler got disconnected, check whether this is the end in charge of re-connecting
                for (DisconnectedPeer handlerToReconn; _handlersToReconnectToByIPAddr.getNext (handlerToReconn) == 1; ) {
                    if (handlerToReconn.peerAddr == handlerIter.getValue()->getRemotePeerAddress()) {
                        if (_disconnectedHandlers.search (handlerToReconn) == 0) {
                            checkAndLogMsg ("ConnCommAdaptor::run", Logger::L_Info, "%s (%s) added to the list of disconnected handlers.\n",
                                            handlerToReconn.peerAddr.c_str(), handlerIter.getValue()->getRemotePeerNodeId());
                            _disconnectedHandlers.add (handlerToReconn);
                        }
                    }
                }
            }
        }

        for (DisconnectedPeer tmp; _disconnectedHandlers.getFirst (discPeer) == 1;) {
            tmp = discPeer;
            if (!_disconnectedPeers.find (tmp)) {
                _disconnectedPeers.enqueue (discPeer);
            }
            _disconnectedHandlers.remove (discPeer);
        }
        _mHandlers.unlock();

        int iDisconnectedPeerSize = _disconnectedPeers.size();
        for (int i = 0; (i < iDisconnectedPeerSize) && (_disconnectedPeers.dequeue (discPeer) == 0); i++) {
            checkAndLogMsg ("ConnCommAdaptor::run", Logger::L_Info, "connecting to peer %s.\n",
                            discPeer.peerAddr.c_str());
            if (connectToPeerInternal (discPeer.peerAddr.c_str(), discPeer.ui16PeerPort) != 0) {
                _disconnectedPeers.enqueue (discPeer);
            }
        }
        _mDisconnectedPeers.unlock();

        sleepForMilliseconds (5000);
    }

    terminating();
}

int ConnCommAdaptor::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    // TODO: implement this
    return 0;
}

int ConnCommAdaptor::subscribe (Subscription &sub)
{
    return 0;
}

int ConnCommAdaptor::startAdaptor (void)
{
    start();

    ConnListeners::Iterator iter = _listenersByInterfaceIP.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        iter.getValue()->start();
    }

    return 0;
}

int ConnCommAdaptor::stopAdaptor (void)
{
    requestTerminationAndWait();

    ConnListeners::Iterator iter = _listenersByInterfaceIP.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        iter.getValue()->requestTerminationAndWait();
    }

    cleanHandlers();
    return 0;
}

void ConnCommAdaptor::cleanHandlers (void)
{
    DisconnectedPeer discPeer;
    _disconnectedHandlers.resetGet();
    while (_disconnectedHandlers.getNext (discPeer) == 1) {
        delete _handlersByPeerId.get (discPeer.peerAddr.c_str());
    }
}

int ConnCommAdaptor::sendMessage (MessageHeaders::MsgType type,
                                  const void *pBuf, uint32 ui32BufLen,
                                  const char *pszPublisherNodeId,
                                  const char **ppszRecipientNodeIds,
                                  const char **ppszInterfaces,
                                  uint8 ui8Priority)
{
    const char *pszMethodName = "ConnCommAdaptor::sendMessage";

    if (pBuf == nullptr || ui32BufLen == 0 || ppszRecipientNodeIds == nullptr) {
        return -1;
    }

    // Add the publisher - if necessary
    uint32 ui32PubLen = (pszPublisherNodeId == nullptr ? 0 : strlen (pszPublisherNodeId));
    BufferWriter bw (1U + 4U + ui32PubLen + ui32BufLen, 128U);

    bw.reset();

    uint8 ui8Type = type;
    if (bw.write8 (&ui8Type) < 0) {
        return -2;
    }
    if (bw.write32 (&ui32PubLen) < 0) {
        return -3;
    }
    if (ui32PubLen > 0 && bw.writeBytes (pszPublisherNodeId, ui32PubLen) < 0) {
        return -4;
    }

    // Write data
    if (bw.writeBytes (pBuf, ui32BufLen) != 0) {
        return -5;
    }

    _mHandlers.lock();
    for (unsigned int i = 0; ppszRecipientNodeIds[i] != nullptr; i++) {
        ConnHandler *pHandler = _handlersByPeerId.get (ppszRecipientNodeIds[i]);
        if (pHandler != nullptr) {
            int rc = pHandler->send (bw.getBuffer(), bw.getBufferLength(), ui8Priority);
            uint8 ui8Type = type;
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not send %s message to peer %s (%s). "
                                "Returned code: %d\n", MessageHeaders::getMetadataAsString (&ui8Type), pHandler->getRemotePeerNodeId(),
                                pHandler->getRemotePeerAddress(), rc);
                DisconnectedPeer dp (pHandler->getRemotePeerAddress(), pHandler->getRemotePeerAddressPort());
                _disconnectedHandlers.add (dp);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "sent %s message.\n",
                                MessageHeaders::getMetadataAsString (&ui8Type));
            }
        }
    }

    cleanHandlers();
    _mHandlers.unlock();

    return 0;
}

// ===========================================================================
// struct DisconnectedPeer
// ===========================================================================

ConnCommAdaptor::DisconnectedPeer::DisconnectedPeer (void)
    : ui16PeerPort (0),
      peerAddr ("0.0.0.0")
{

}

ConnCommAdaptor::DisconnectedPeer::DisconnectedPeer (const char *pszRemotePeerAddr, uint16 ui16Port)
    : ui16PeerPort (ui16Port),
      peerAddr (pszRemotePeerAddr)
{
}

ConnCommAdaptor::DisconnectedPeer::~DisconnectedPeer (void)
{
}

ConnCommAdaptor::DisconnectedPeer & ConnCommAdaptor::DisconnectedPeer::operator = (const DisconnectedPeer &rhsDisconnectedPeer)
{
    peerAddr = rhsDisconnectedPeer.peerAddr;
    ui16PeerPort = rhsDisconnectedPeer.ui16PeerPort;
    return *this;
}

bool ConnCommAdaptor::DisconnectedPeer::operator == (const DisconnectedPeer &rhsDisconnectedPeer)
{
    return ((peerAddr == rhsDisconnectedPeer.peerAddr) == 1);
}

