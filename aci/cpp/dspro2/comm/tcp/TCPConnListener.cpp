/*
 * TCPConnListener.cpp
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
 * Created on April 15, 2014, 3:09 PM
 */

#include "TCPConnListener.h"

#include "TCPAdaptor.h"
#include "TCPConnHandler.h"
#include "TCPEndPoint.h"

#include "InetAddr.h"
#include "TCPSocket.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

TCPConnListener::TCPConnListener (const char *pszListenAddr, uint16 ui16Port,
                                  const char *pszNodeId, const char *pszSessionId,
                                  CommAdaptorListener *pListener, ConnCommAdaptor *pConnCommAdaptor)
    : ConnListener (pszListenAddr, ui16Port, pszNodeId, pszSessionId, pListener, pConnCommAdaptor)
{
}

TCPConnListener::~TCPConnListener()
{
    _servSocket.disconnect();
}

void TCPConnListener::requestTermination (void)
{
    _servSocket.disableReceive();
    if (isRunning()) {
        ConnListener::requestTerminationAndWait();
    }
}

void TCPConnListener::requestTerminationAndWait (void)
{
    _servSocket.disableReceive();
    if (isRunning()) {
        ConnListener::requestTerminationAndWait();
    }
}

void TCPConnListener::run()
{
    String sThreadName ("IHMC_ACI::TCPConnListener_");
    sThreadName += _listenAddr;
    setName (sThreadName.c_str());

    // Then listen on the socket
    InetAddr inet (_listenAddr);
    _servSocket.setupToReceive (_ui16Port, 10, inet.getIPAddress());

    ConnListener::run();
}

ConnHandler * TCPConnListener::getConnHandler (ConnEndPoint *pEndPoint, const String &remotePeer)
{
    if (pEndPoint == nullptr) {
        return nullptr;
    }
    TCPEndPoint *pTCPEndPoint = dynamic_cast<TCPEndPoint *> (pEndPoint);
    if (pTCPEndPoint == nullptr) {
        return nullptr;
    }
    assert (pTCPEndPoint->_pSocket != nullptr);

    const AdaptorProperties *pAdaptProp = _pCommAdaptor->getAdaptorProperties();
    TCPConnHandler *pHandler = new TCPConnHandler (*pAdaptProp, remotePeer, _pListener, pTCPEndPoint->_pSocket, _listenAddr);
    if (pHandler == nullptr) {
        checkAndLogMsg ("TCPConnListener::getConnHandler", memoryExhausted);
    }

    return pHandler;
}

ConnEndPoint * TCPConnListener::acceptConnection (void)
{
    // Receive connection
    TCPSocket *pSocket = (TCPSocket *) _servSocket.accept();
    if (pSocket == nullptr) {
        checkAndLogMsg ("TCPConnListener::getEndPoint", memoryExhausted);
        return nullptr;
    }
    ConnEndPoint *pEndPoint = new TCPEndPoint (pSocket, ConnEndPoint::DEFAULT_TIMEOUT);
    if (pEndPoint == nullptr) {
        checkAndLogMsg ("TCPConnListener::getEndPoint", memoryExhausted);
    }
    return pEndPoint;
}

