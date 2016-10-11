/* 
 * ConnListener.cpp
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
 * Created on April 14, 2014, 6:19 PM
 */

#include "ConnListener.h"

#include "CommAdaptorListener.h"
#include "ConnCommAdaptor.h"
#include "ConnEndPoint.h"
#include "ConnHandler.h"
#include "Defs.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

ConnListener::ConnListener (const char *pszListenAddr, uint16 ui16Port,
                            const char *pszNodeId, const char *pszSessionId,
                            CommAdaptorListener *pListener,
                            ConnCommAdaptor *pCommAdaptor)
    : _ui16Port (ui16Port),
      _listenAddr (pszListenAddr),
      _pCommAdaptor (pCommAdaptor),
      _pListener (pListener),
      _nodeId (pszNodeId),
      _sessionId (pszSessionId)
{
}

ConnListener::~ConnListener (void)
{
}

void ConnListener::run (void)
{
    // Thread name is set in the implementing classes

    const char *pszMethodName = "ConnListener::run";

    started();

    // Prepare buffer with session and node ID
    int uiNodeIdLen = _nodeId.length();
    if (uiNodeIdLen <= 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "peer id not set. Terminating.\n");
        terminating();
        return;
    }

    while (!terminationRequested()) {

        // Receive connection
        ConnEndPoint *pEndPoint = acceptConnection();
        if (pEndPoint == NULL) {
            continue;
        }

        // Do handshake
        const ConnHandler::HandshakeResult handshake (ConnHandler::doHandshake (pEndPoint, _nodeId, _sessionId, _pListener));
        if (handshake._remotePeerId.length() <= 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "handshake failed\n");
            pEndPoint->close();
            delete pEndPoint;
            continue;
        }

        // Handshake successful: create handler and delegate connection handling
        ConnHandler *pHandler = getConnHandler (pEndPoint, handshake._remotePeerId);
        if (pHandler == NULL) {
            checkAndLogMsg (pszMethodName, memoryExhausted);
            pEndPoint->close();
        }
        else {
            pHandler->init();
            pHandler->start();

            checkAndLogMsg (pszMethodName, Logger::L_Info, "a ConnHandler was "
                            "created for peer %s.\n", pHandler->getRemotePeerNodeId());
            _pCommAdaptor->addHandler (pHandler);
        }
        delete pEndPoint;
    }

    terminating();
}

