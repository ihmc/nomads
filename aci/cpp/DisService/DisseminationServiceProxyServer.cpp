/*
 * DisseminationServiceProxyServer.cpp
 *
 *This file is part of the IHMC DisService Library/Component
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

#include "DisseminationServiceProxyServer.h"

#include "DisServiceDefs.h"
#include "DisseminationService.h"
#include "DisseminationServiceProxyAdaptor.h"

#include "Exceptions.h"
#include "InetAddr.h"
#include "Logger.h"
#include "SimpleCommHelper2.h"
#include "TCPSocket.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//==============================================================================
// DisseminationServiceProxyServer
//==============================================================================

#define checkAndLogMsg if (pLogger) pLogger->logMsg

DisseminationServiceProxyServer::DisseminationServiceProxyServer()
    : _proxies (true, true, true, false)
{
    _pServerSock = NULL;
    _pDissSvc = NULL;
}

DisseminationServiceProxyServer::~DisseminationServiceProxyServer()
{
    checkAndLogMsg ("DisseminationServiceProxyServer::~DisseminationServiceProxyServer",
                    Logger::L_Info, "DisseminationServiceProxyServer is quitting\n");
    if (_pServerSock != NULL) {
        _pServerSock->disableReceive();
    }
    if (isRunning()) {
        requestTerminationAndWait();
    }
    delete _pServerSock;
    _pServerSock = NULL;
}

int DisseminationServiceProxyServer::init (DisseminationService *pDissSvc, uint16 ui16PortNum, const char *pszProxyServerInterface)
{
    const char * const pszMethodName = "DisseminationServiceProxyServer::init";
    if (pDissSvc == NULL) {
        return -1;
    }

    // Initialize server socket
    int rc = 0;
    if (_pServerSock != NULL) {
        delete _pServerSock;
    }
    unsigned long ulIPAddr = INADDR_ANY;
    if (pszProxyServerInterface != NULL) {
        ulIPAddr = InetAddr (pszProxyServerInterface).getIPAddress();;
    }

    _pServerSock = new TCPSocket();
    if ((rc = _pServerSock->setupToReceive (ui16PortNum, 5, ulIPAddr)) == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "socket bound to port %d\n", (int) ui16PortNum);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to bind socket to port %d error num: %d\n",
                        (int) ui16PortNum, rc);
        return -2;
    }

    _pDissSvc = pDissSvc;
    return 0;
}

void DisseminationServiceProxyServer::run()
{
    const char * const pszMethodName = "DisseminationServiceProxyServer::run";
    setName (pszMethodName);

    started();
    while (!terminationRequested()) {
        // Get next connection, or abort if it fails
        TCPSocket *pClientSock = (TCPSocket*) _pServerSock->accept();

        if (NULL == pClientSock) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                              "failed to accept a connection\n");
        }
        else {
            if (pClientSock->setLingerOptions (1, 30)) { // Enable SO_LINGER and set the timeout to 30 seconds
                checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                  "failed to set the linger option for the socket\n");
            }

            pClientSock->bufferingMode (false);

            SimpleCommHelper2 *pCommHelper = new SimpleCommHelper2();
            int rc = pCommHelper->init (pClientSock);

            if (rc != 0) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                  "could not initialize CommHelper; rc = %d\n", rc);
                delete pCommHelper;
                pCommHelper = NULL;
                terminating();
                return;
            }

            DSProxyServerConnHandler *pConnHandler;
            pConnHandler = new DSProxyServerConnHandler (this, pCommHelper);
            pConnHandler->start();
        }
    }
    terminating();
}

void DisseminationServiceProxyServer::requestTermination()
{
    ManageableThread::requestTermination();
    if (_pServerSock) {
        _pServerSock->disableReceive();
    }
}

void DisseminationServiceProxyServer::requestTerminationAndWait (void)
{
    if (_pServerSock) {
        _pServerSock->disableReceive();
    }
    ManageableThread::requestTerminationAndWait();
}

//==============================================================================
// DSProxyServerConnHandler
//==============================================================================

DSProxyServerConnHandler::DSProxyServerConnHandler (DisseminationServiceProxyServer *pDissSvcPS, SimpleCommHelper2 *pCommHelper)
{
    _pCommHelper = pCommHelper;
    _pDissSvcProxyServer = pDissSvcPS;
}

DSProxyServerConnHandler::~DSProxyServerConnHandler()
{
    if (isRunning()) {
        requestTerminationAndWait();
    }
}

void DSProxyServerConnHandler::run()
{
    const char * const pszMethodName = "DSProxyServerConnHandler::run";

    // Initial handshake
    SimpleCommHelper2::Error err = SimpleCommHelper2::None;
    const char **ppszBuf = _pCommHelper->receiveParsedSpecific ("1 1", err);
    if (err != SimpleCommHelper2::None) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "CommException during "
                        "initial handshake; error code: %d\n", (int) err);
        _pCommHelper->closeConnection (err);
        delete _pCommHelper;
    }
    else if (0 == stricmp (ppszBuf[0], "RegisterProxy")) {
        doRegisterProxy (_pCommHelper, atoi(ppszBuf[1]));
    }
    else if (0 == stricmp (ppszBuf[0], "RegisterProxyCallback")) {
        doRegisterProxyCallback (_pCommHelper, atoi(ppszBuf[1]));
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Expected RegisterProxy "
                        "or RegisterProxyCallback but got something else\n");
    }

    delete this;
}

void DSProxyServerConnHandler::doRegisterProxy (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationId)
{
    char szProxyId[10];
    snprintf (szProxyId, sizeof(szProxyId)-1, "%d", ui16ApplicationId);

    while (_pDissSvcProxyServer->_proxies.containsKey (szProxyId)) {
        snprintf (szProxyId, sizeof(szProxyId)-1, "%d", ++ui16ApplicationId);
    }

    DisseminationServiceProxyAdaptor *pAdaptor = new DisseminationServiceProxyAdaptor (_pDissSvcProxyServer);
    if (pAdaptor->init (pCommHelper, ui16ApplicationId) != 0) {
        checkAndLogMsg ("DSProxyServerConnHandler::doRegisterProxy",
                          Logger::L_SevereError, "adaptor init failed\n");
        delete pAdaptor;
        SimpleCommHelper2::Error err = SimpleCommHelper2::None;
        pCommHelper->closeConnection (err);
        delete pCommHelper;
    }
    else {
        _pDissSvcProxyServer->_proxies.put (szProxyId, pAdaptor);
        SimpleCommHelper2::Error err = SimpleCommHelper2::None;
        pCommHelper->sendLine (err, "OK %d", ui16ApplicationId);
        if (err != SimpleCommHelper2::None) {
            checkAndLogMsg ("DSProxyServerConnHandler::doRegisterProxy",
                          Logger::L_SevereError, "sendLine failed\n");
            pCommHelper->closeConnection (err);
            delete pCommHelper;
        }
        else {
            pAdaptor->start();
        }
    }
}

void DSProxyServerConnHandler::doRegisterProxyCallback (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationId)
{
    char szProxyId[10];
    snprintf (szProxyId, sizeof(szProxyId)-1, "%d", ui16ApplicationId);

    DisseminationServiceProxyAdaptor *pAdaptor = _pDissSvcProxyServer->_proxies.get (szProxyId);

    if (pAdaptor != NULL) {
        pAdaptor->setCallbackCommHelper (pCommHelper);
        SimpleCommHelper2::Error err = SimpleCommHelper2::None;
        pCommHelper->sendLine (err, "OK");
        if (err != SimpleCommHelper2::None) {
            checkAndLogMsg ("DSProxyServerConnHandler::doRegisterProxy",
                          Logger::L_SevereError, "sendLine failed\n");
            pCommHelper->closeConnection (err);
            delete pCommHelper;
        }
    }
    else {
        checkAndLogMsg ("DSProxyServerConnHandler::doRegisterProxyCallback", Logger::L_MildError,
                        "did not find proxy with id %d to register a callback handler\n",
                        (int) ui16ApplicationId);
        SimpleCommHelper2::Error err = SimpleCommHelper2::None;
        pCommHelper->sendLine (err, "ERROR: proxy with id %d not found", ui16ApplicationId);
        pCommHelper->closeConnection (err);
        delete pCommHelper;
    }
}

