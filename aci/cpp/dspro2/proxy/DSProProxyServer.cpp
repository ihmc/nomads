/**
 * DSProProxyServer.cpp
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
 */

#include "DSProProxyServer.h"

#include "DSPro.h"
#include "DSProProxyAdaptor.h"

#include "SimpleCommHelper2.h"
#include "Logger.h"
#include "TCPSocket.h"

using namespace NOMADSUtil;
using namespace IHMC_ACI;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

const String DSProProxyServer::SERVICE = "dspro";
const String DSProProxyServer::VERSION = "20170206";

DSProProxyServer::~DSProProxyServer (void)
{
    if (isRunning()) {
        requestTerminationAndWait();
    }
    if (_disServiceProxySrv.isRunning()) {
        _disServiceProxySrv.requestTerminationAndWait();
    }
    delete _pServerSock;
    _pServerSock = nullptr;

    // Close adaptors' connections, and deallocated them
    StringHashtable<DSProProxyAdaptor>::Iterator iter = _proxies.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        iter.getValue()->requestTermination();
    }
    for (iter = _proxies.getAllElements(); !iter.end(); iter.nextElement()) {
        iter.getValue()->requestTerminationAndWait();
    }
    _proxies.removeAll();
}

int DSProProxyServer::init (DSPro *pDisSvcPro, const char *pszProxyServerInterface, uint16 ui16ProxyServerPortNum)
{
    if (pDisSvcPro == nullptr) {
        return -1;
    }

    // Initialize server socket
    int rc = 0;
    uint32 ui32ProxyServerInterface = INADDR_ANY;
    if (pszProxyServerInterface) {
        ui32ProxyServerInterface = InetAddr (pszProxyServerInterface).getIPAddress();
    }
    _pServerSock = new TCPSocket();
    if ((rc = _pServerSock->setupToReceive (ui16ProxyServerPortNum, 5, ui32ProxyServerInterface))) {
        checkAndLogMsg ("DSProProxyServer::init", Logger::L_MildError,
                        "failed to bind socket to port %d error num: %d\n", (int) ui16ProxyServerPortNum, rc);
        return -2;
    }

    _pDSPro = pDisSvcPro;
    return 0;
}

int DSProProxyServer::initDisseminationServiceProxyServer (DisseminationService *pDisService,
                                                           const char *pszProxyServerInterface,
                                                           uint16 ui16PortNum)
{
    if (pDisService == nullptr) {
        return -1;
    }
    if ((_pServerSock != nullptr) && (_pServerSock->getLocalPort() == ui16PortNum)) {
        // Port conflict!
        return -2;
    }
    if (!_disServiceProxySrv.isRunning()) {
        int rc = _disServiceProxySrv.init (pDisService, ui16PortNum, pszProxyServerInterface);
        if (rc == 0) {
            _disServiceProxySrv.start();
        }
        return rc;
    }
    return 0;
}

void DSProProxyServer::run (void)
{
    setName ("DSProProxyServer::run");

    started();
    while (!terminationRequested()) {
        // Get next connection, or abort if it fails
        TCPSocket *pClientSock = (TCPSocket*) _pServerSock->accept();

        if (nullptr == pClientSock) {
            checkAndLogMsg ("DSProProxyServer::run", Logger::L_MildError,
                            "failed to accept a connection\n");
        }
        else {
            if (pClientSock->setLingerOptions (1, 30)) { // Enable SO_LINGER and set the timeout to 30 seconds
                checkAndLogMsg ("DSProProxyServer::run", Logger::L_Warning,
                                "failed to set the linger option for the socket\n");
            }
            pClientSock->bufferingMode (false);

            SimpleCommHelper2 *pCommHelper = new SimpleCommHelper2();
            int rc = pCommHelper->init (pClientSock);

            if (rc != 0) {
                checkAndLogMsg ("DSProProxyServer::run", Logger::L_MildError,
                                "could not initialize CommHelper; rc = %d\n", rc);
                delete pCommHelper;
                pCommHelper = nullptr;
                terminating();
                return;
            }

            auto * pConnHandler = new DSPProxyServerConnHandler (this, pCommHelper, _bStrictHandshake);
            pConnHandler->start();
        }
    }
    terminating();
}

void DSProProxyServer::requestTermination (void)
{
    if (_pServerSock != nullptr) {
        _pServerSock->disableReceive();
    }
    ManageableThread::requestTermination();
}

void DSProProxyServer::requestTerminationAndWait (void)
{
    if (_pServerSock != nullptr) {
        _pServerSock->disableReceive();
    }
    ManageableThread::requestTerminationAndWait();
}

// //////////////////////////////////////////////////////////////////////////////////////
// DisServiceProProxyAdaptor        /////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////

void DSPProxyServerConnHandler::run()
{
    const char *pszMethodName = "DSPProxyServerConnHandler::run";

    // Initial handshake
    SimpleCommHelper2::Error error = doHandshake (_pCommHelper, _bStrictHandshake);
    if (error == SimpleCommHelper2::None) {
        // Register the client
        const char **ppszBuf = _pCommHelper->receiveParsedSpecific ("1 1", error);
        if (error == SimpleCommHelper2::None) {
            if (0 == stricmp (ppszBuf[0], "RegisterProxy")) {
                error = doRegisterProxy (_pCommHelper, atoi (ppszBuf[1]));
            }
            else if (0 == stricmp (ppszBuf[0], "RegisterProxyCallback")) {
                error = doRegisterProxyCallback (_pCommHelper, atoi (ppszBuf[1]));
            }
        }

        switch (error) {
            case SimpleCommHelper2::None:
                break;

            case SimpleCommHelper2::CommError:
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "Communication error during client registration.\n");
                _pCommHelper->closeConnection (error);
                delete _pCommHelper;
                break;

            case SimpleCommHelper2::ProtocolError:
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "Protocol error during client registration.\n");
                _pCommHelper->closeConnection (error);
                delete _pCommHelper;
                break;

            default:
                assert (false);
        }
    }

    terminating();
}

SimpleCommHelper2::Error DSPProxyServerConnHandler::doHandshake (SimpleCommHelper2 *pCommHelper, bool bStrict)
{
    const char *pszMethodName = "DSPProxyServerConnHandler::doHandshake";
    SimpleCommHelper2::Error error = SimpleCommHelper2::None;


    // Check whether the client is expecting to connect to this service, and that
    // the protocol is the same
    int iCount = 0;
    const char **ppszBuf = pCommHelper->receiveParsed (error, &iCount);
    if (error != SimpleCommHelper2::None) {
        return error;
    }
    if (iCount != 2) {
        return SimpleCommHelper2::ProtocolError;
    }

    String svc (bStrict ? DSProProxyServer::SERVICE.c_str() : ppszBuf[0]);
    String ver (bStrict ? DSProProxyServer::VERSION.c_str() : ppszBuf[1]);
    svc.trim();
    ver.trim();

    pCommHelper->sendLine (error, "%s %s", svc.c_str(), ver.c_str());
    if (error != SimpleCommHelper2::None) {
        return error;
    }
    if ((DSProProxyServer::SERVICE != svc) || (DSProProxyServer::VERSION != ver)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "service mismatch: received %s %s, while expecting %s %s.\n",
                        svc.c_str(), ver.c_str(), DSProProxyServer::SERVICE.c_str() , DSProProxyServer::VERSION.c_str());
        if (bStrict) {
            return SimpleCommHelper2::ProtocolError;
        }
    }
    return error;
}

CommHelperError DSPProxyServerConnHandler::doRegisterProxy (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationId)
{
    const char *pszMethodName = "DSPProxyServerConnHandler::doRegisterProxy";
    char szProxyId[10];
    sprintf (szProxyId, "%d", ui16ApplicationId);
    CommHelperError error = SimpleCommHelper2::None;

    while (_pDisSvcProProxyServer->_proxies.containsKey (szProxyId)) {
        sprintf (szProxyId, "%d", ++ui16ApplicationId);
    }

    DSProProxyAdaptor *pAdaptor = new DSProProxyAdaptor (_pDisSvcProProxyServer);
    if (pAdaptor->init (pCommHelper, ui16ApplicationId) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "adaptor init failed\n");
        delete pAdaptor;
        pCommHelper->closeConnection (error);
        delete pCommHelper;
    }
    else {
        _pDisSvcProProxyServer->_proxies.put (szProxyId, pAdaptor);
        pCommHelper->sendLine (error, "OK %d", ui16ApplicationId);
        pAdaptor->start();
    }

    return error;
}

CommHelperError DSPProxyServerConnHandler::doRegisterProxyCallback (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationId)
{
    char szProxyId[10];
    sprintf (szProxyId, "%d", ui16ApplicationId);
    CommHelperError error = SimpleCommHelper2::None;

    DSProProxyAdaptor *pAdaptor = _pDisSvcProProxyServer->_proxies.get (szProxyId);

    if (pAdaptor != nullptr) {
        pAdaptor->setCallbackCommHelper (pCommHelper);
        pCommHelper->sendLine (error, "OK");
    }
    else {
        checkAndLogMsg ("DSPProxyServerConnHandler::doRegisterProxyCallback", Logger::L_MildError,
                        "did not find proxy with id %d to register a callback handler\n", (int) ui16ApplicationId);
        pCommHelper->sendLine (error, "ERROR: proxy with id %d not found", ui16ApplicationId);
        if (error == SimpleCommHelper2::None) {
            pCommHelper->closeConnection (error);
        }
        delete pCommHelper;
    }

    return error;
}

