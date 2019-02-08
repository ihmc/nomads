/*
 * MocketConnListener.cpp
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

#include "MocketConnListener.h"

#include "MocketsAdaptor.h"
#include "MocketConnHandler.h"
#include "MocketsEndPoint.h"

#include "Mocket.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace MOCKET_CONN_LISTENER
{
    bool enableDtls (const char *pszCertFile, const char *pszKeyFile)
    {
        return (pszCertFile != nullptr) && (pszKeyFile != nullptr);
    }
}

using namespace MOCKET_CONN_LISTENER;

MocketConnListener::MocketConnListener (const char *pszListenAddr, uint16 ui16Port,
                                        const char *pszNodeId, const char *pszSessionId,
                                        const char *pszMocketConfigFile,
                                        CommAdaptorListener *pListener,
                                        ConnCommAdaptor *pConnCommAdaptor,
                                        const char *pszCertFile, const char *pszKeyFile)
    : ConnListener (pszListenAddr, ui16Port, pszNodeId, pszSessionId, pListener, pConnCommAdaptor),
      _servMocket (pszMocketConfigFile, nullptr, false, enableDtls (pszCertFile, pszKeyFile), pszCertFile, pszKeyFile)
{
}

MocketConnListener::~MocketConnListener()
{
    _servMocket.close();
}

void MocketConnListener::requestTermination (void)
{
    _servMocket.close();
    if (isRunning()) {
        ConnListener::requestTermination();
    }
}

void MocketConnListener::requestTerminationAndWait (void)
{
    _servMocket.close();
    if (isRunning()) {
        ConnListener::requestTerminationAndWait();
    }
}

void MocketConnListener::run()
{
    String sThreadName ("IHMC_ACI::MocketConnListener_");
    sThreadName += _listenAddr;
    setName (sThreadName.c_str());

    // Then listen on the socket
    _servMocket.listen (_ui16Port, _listenAddr.c_str());

    ConnListener::run();
}

ConnHandler * MocketConnListener::getConnHandler (ConnEndPoint *pEndPoint, const String &remotePeer)
{
    if (pEndPoint == nullptr) {
        return nullptr;
    }
    MocketEndPoint *pMocketEndPoint = dynamic_cast<MocketEndPoint *> (pEndPoint);
    if (pMocketEndPoint == nullptr) {
        return nullptr;
    }
    assert (pMocketEndPoint->_pMocket != nullptr);

    const AdaptorProperties *pAdaptProp = _pCommAdaptor->getAdaptorProperties();
    MocketConnHandler *pHandler = new MocketConnHandler (*pAdaptProp, remotePeer, _pListener, pMocketEndPoint->_pMocket);
    if (pHandler == nullptr) {
        checkAndLogMsg ("MocketConnListener::getConnHandler", memoryExhausted);
    }

    return pHandler;
}

ConnEndPoint * MocketConnListener::acceptConnection (void)
{
    // Receive connection
    Mocket *pMocket = _servMocket.accept();
    if (pMocket == nullptr) {
        checkAndLogMsg ("MocketConnListener::getEndPoint", memoryExhausted);
        return nullptr;
    }
    ConnEndPoint *pEndPoint = new MocketEndPoint (pMocket, ConnEndPoint::DEFAULT_TIMEOUT);
    if (pEndPoint == nullptr) {
        checkAndLogMsg ("MocketConnListener::getEndPoint", memoryExhausted);
    }
    return pEndPoint;
}

