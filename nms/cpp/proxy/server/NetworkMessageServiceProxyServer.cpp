/*
 * NetworkMessageServiceProxyServer.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Created on February 17, 2015, 2:01 AM
 */

#include "NetworkMessageServiceProxyServer.h"

#include "Reader.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

NetworkMessageServiceProxyServer::NetworkMessageServiceProxyServer (NetworkMessageService *pNMS, bool bDeallocateNMS)
    : Registry<NetworkMessageService> (pNMS, &NetworkMessageServiceUnmarshaller::methodInvoked,
                                       NetworkMessageServiceUnmarshaller::SERVICE,
                                       NetworkMessageServiceUnmarshaller::VERSION, bDeallocateNMS)
{
}

NetworkMessageServiceProxyServer::~NetworkMessageServiceProxyServer (void)
{
}

bool NetworkMessageServiceProxyServer::registerCallback (uint16 ui16ApplicationId, SimpleCommHelper2 *pCommHelper,
                                                         SimpleCommHelper2 *pCallbackCommHelper, SimpleCommHelper2::Error &error)
{
    const char *pszMethodName = "NetworkMessageServiceProxyServer::registerCallback";
    if (pCommHelper == NULL) {
        return false;
    }
    Reader *pReader = pCommHelper->getReaderRef();
    if (pReader == NULL) {
        return false;
    }

    uint8 ui8MsgType = 0x00;
    if (pReader->read8 (&ui8MsgType) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    NetworkMessageServiceCallbackManager *pCbackMarshaller = new NetworkMessageServiceCallbackManager (ui16ApplicationId, pCallbackCommHelper);
    if (pCbackMarshaller == NULL) {
        return false;
    }
    if (_pSvc->registerHandlerCallback (ui8MsgType, pCbackMarshaller) < 0) {
        delete pCbackMarshaller;
        return false;
    }

    ByMsgType *pByMsgType = _cbackHandlersByApplicationId.get (ui16ApplicationId);
    if (pByMsgType == NULL) {
        pByMsgType = new ByMsgType();
        _cbackHandlersByApplicationId.put (ui16ApplicationId, pByMsgType);
    }
    if (pByMsgType->put (ui8MsgType, pCbackMarshaller) != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "added duplicate callback handler "
                        "for application id %d and message type %d", (int) ui16ApplicationId, (int) ui8MsgType);
    }

    return true;
}

bool NetworkMessageServiceProxyServer::deregisterCallback (uint16 ui16ApplicationId, CallbackExecutor *pCBackExec,
                                                           SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
{
    if (pCommHelper == NULL) {
        return false;
    }
    Reader *pReader = pCommHelper->getReaderRef();
    if (pReader == NULL) {
        return false;
    }

    uint8 ui8MsgType = 0x00;
    if (pReader->read8 (&ui8MsgType) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    ByMsgType *pByMsgType = _cbackHandlersByApplicationId.get (ui16ApplicationId);
    if (pByMsgType == NULL) {
        return false;
    }
    NetworkMessageServiceCallbackManager *pUnMarshaller = pByMsgType->get (ui8MsgType);
    if (pUnMarshaller == NULL) {
        return false;
    }
    if (_pSvc->deregisterHandlerCallback (ui8MsgType, pUnMarshaller) < 0) {
        return false;
    }

    delete pByMsgType->remove (ui8MsgType);
    return true;
}

bool NetworkMessageServiceProxyServer::deregisterAllCallbacks (uint16 ui16ApplicationId)
{
    UInt32Hashtable<ByMsgType>::Iterator byAppId = _cbackHandlersByApplicationId.getAllElements();
    for (; !byAppId.end(); byAppId.nextElement()) {
	if (byAppId.getKey() != ui16ApplicationId) continue;
        ByMsgType::Iterator byMsgType = byAppId.getValue()->getAllElements();
        for (; !byMsgType.end(); byMsgType.nextElement()) {
            if (_pSvc->deregisterHandlerCallback ((uint8) byMsgType.getKey(), byMsgType.getValue()) < 0) {
                return false;
            }
        }
    }
    return true;
}

