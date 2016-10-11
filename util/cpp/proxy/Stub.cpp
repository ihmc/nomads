/* 
 * Stub.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Created on February 27, 2015, 1:09 PM
 */

#include "Stub.h"

#include "Protocol.h"

#include "NLFLib.h"
#include "SemClass.h"
#include "TCPSocket.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define CHECK_CONNECTION_OR_FAIL(mutexCode) if (!checkConnection()) { _mutex.unlock(()); return -99; }

using namespace NOMADSUtil;

Stub::Stub (uint16 ui16ADesiredApplicationId, StubUnmarshalFnPtr pUnmarshaller,
            const char *pszService, const char *pszVersion, bool bUseBackgroundReconnect)
    : _pCommHelper (NULL),
      _bUsingBackgroundReconnect (bUseBackgroundReconnect),
      _bReconnectStarted (false),
      _ui16ApplicationId (ui16ADesiredApplicationId),
      _ui16Port (0),
      _pListener (NULL),
      _pUnmarshaller (pUnmarshaller),
      _pHandler (NULL),
      _pReconnectSemaphore (new Semaphore(0)),
      _service (pszService),
      _version (pszVersion)
{
}

Stub::~Stub (void)
{
    if (_bUsingBackgroundReconnect) {
        //requestTermination();
        _pReconnectSemaphore->up();
        ManageableThread::requestTerminationAndWait();
    }
    _stubMutex.lock ();
    if (_pHandler != NULL) {
        //_pHandler->requestTerminationAndWait();
        delete _pHandler;
        _pHandler = NULL;
    }
    if (_pCommHelper != NULL) {
        delete _pCommHelper;
        _pCommHelper = NULL;
    }
    _stubMutex.unlock();
    _pListener = NULL;
    delete _pReconnectSemaphore;
    _pReconnectSemaphore = NULL;
}

uint16 Stub::getApplicationId (void)
{
    return _ui16ApplicationId;
}

int Stub::init (const char *pszHost, uint16 ui16Port)
{
    if (pszHost == NULL) {
        pszHost = "127.0.0.1";
    }
    if (ui16Port == 0) {
        ui16Port = 56488;
    }

    _sHost = pszHost;
    _ui16Port = ui16Port;

    if (_bUsingBackgroundReconnect) {
        start();
        if (tryConnect() != 0) {
            startReconnect();
        }
        return 0;
    }
    return tryConnect();
}

bool Stub::isConnected (void)
{
    return checkConnection();
}

bool Stub::startReconnect()
{
    if (!_bUsingBackgroundReconnect) {
        return false;
    }

    _mutexReconnect.lock();
    if (_bReconnectStarted) {
        _mutexReconnect.unlock();
        return true;
    }
    _bReconnectStarted = true;
    _mutexReconnect.unlock();

    // signal reconnect thread
    _pReconnectSemaphore->up();
    return true;
}

// Call this with _mutex held
bool Stub::checkConnection()
{
    // Eventually we should have the option to block here
    if (_pCommHelper == NULL) {
        return false;
    }

    return true;
}

void Stub::run()
{
    const char *pszMethodName = "Stub::run";
    started();
    char tmpbuf[80];
    sprintf (tmpbuf, "Stub %d %p", _ui16ApplicationId, this);
    Thread::setName (tmpbuf);
    while (!terminationRequested()) {
        _pReconnectSemaphore->down();

        if (terminationRequested()) {
            break;
        }
        if (_pListener != NULL) {
            _pListener->serverDisconnected();
        }
        checkAndLogMsg (pszMethodName, Logger::L_Info, "starting background connect\n");

        _stubMutex.lock();
        if (_pCommHelper != NULL)  {
            delete _pCommHelper;
            _pCommHelper = NULL;
        }
        if (_pHandler != NULL) {
            delete _pHandler;
            _pHandler = NULL;
        }
        _stubMutex.unlock();

        while (tryConnect() != 0) {
            sleepForMilliseconds(5000);
        }

        _mutexReconnect.lock();
        _bReconnectStarted = false;
        _mutexReconnect.unlock();
    }
    checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "id %d terminating\n", _ui16ApplicationId);
    terminating();
}

void Stub::requestTermination (void)
{
    if (terminationRequested()) {
        SimpleCommHelper2::Error error;
        _mutexReconnect.lock ();
        _pCommHelper->closeConnection (error);
        _mutexReconnect.unlock();
        _pReconnectSemaphore->up();
        ManageableThread::requestTermination();
    }
}

void Stub::requestTerminationAndWait (void)
{
    if (terminationRequested()) {
        SimpleCommHelper2::Error error;
        _mutexReconnect.lock ();
        _pCommHelper->closeConnection (error);
        _mutexReconnect.unlock();
        _pReconnectSemaphore->up ();
        ManageableThread::requestTerminationAndWait();
    }
}

int Stub::reregisterListeners (void)
{
    const char *pszMethodName = "Stub::reregisterListeners";
    if (_pCommHelper == NULL) {
        return -1;
    }
    SimpleCommHelper2::Error error = SimpleCommHelper2::None;
    _pCommHelper->sendLine (error, "%s %d", Protocol::REGISTER_PROXY_CALLBACK.c_str(), static_cast<int>(_ui16ApplicationId));
    if (error == SimpleCommHelper2::CommError) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "request failed with a comm exception.\n");
        startReconnect();
    }
    return 0;
}

SimpleCommHelper2 * Stub::connectToServer (const char *pszHost, uint16 ui16Port)
{
    const char *pszMethodName = "Stub::connectToServer";

    int rc;
    TCPSocket *pSocket = new TCPSocket();
    if (0 != (rc = pSocket->connect (pszHost, ui16Port))) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to connect to "
                        "remote host %s on port %d; rc = %d\n", pszHost, ui16Port, rc);
        delete pSocket;
        return NULL;
    }
    if (pSocket->bufferingMode (false) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not disable Naggle's algorithm, proxy communication may be very slow.\n", rc);
    }
    Logger::Level loggingLevel = Logger::L_Warning;
    if (pLogger != NULL) {
        loggingLevel = static_cast<Logger::Level>(pLogger->getDebugLevel ());
    }
    SimpleCommHelper2 *pch = new SimpleCommHelper2 (loggingLevel);
    if (0 != (rc = pch->init (pSocket))) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to initialize CommHelper; rc = %d\n", rc);
        delete pSocket;
        delete pch;
        return NULL;
    }
    pch->setDeleteUnderlyingSocket (true);

    return pch;
}

int Stub::registerProxy (SimpleCommHelper2 *pch, SimpleCommHelper2 *pchCallback,
                         uint16 ui16DesiredApplicationId)
{
    const char *pszMethodName = "Stub::registerProxy";

    uint16 ui16ApplicationId;

    // First register the proxy using the desired application id
    // The ProxyServer will return the assigned application id
    SimpleCommHelper2::Error error = Protocol::doHandshake (pch, _service, _version);
    if (SimpleCommHelper2::None != error) {
        return -1;
    }
    pch->sendLine (error, "%s %d", Protocol::REGISTER_PROXY.c_str(), (int) ui16DesiredApplicationId);
    if (error == SimpleCommHelper2::None) {
        char szId[10];
        pch->receiveRemainingLine (error, szId, sizeof (szId), "OK");
        ui16ApplicationId = static_cast<uint16>(atoi (szId));
    }
    else {
        return -2;
    }
    // Now register the callback using the assigned application id
    error = Protocol::doHandshake (pchCallback, _service, _version);
    if (SimpleCommHelper2::None != error) {
        return -3;
    }
    if (error == SimpleCommHelper2::None) {
        pchCallback->sendLine (error, "%s %d", Protocol::REGISTER_PROXY_CALLBACK.c_str(), static_cast<int>(ui16ApplicationId));
    }
    if (error == SimpleCommHelper2::None) {
        pchCallback->receiveMatch (error, "OK");
    }

    if (error == SimpleCommHelper2::None) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "registered proxy with id %d.\n", ui16ApplicationId);
        return ui16ApplicationId;
    }
    if (error == SimpleCommHelper2::ProtocolError) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed with protocol exception.\n");
        return -4;
    }
    if (error == SimpleCommHelper2::CommError) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed with comm exception.\n");
        return -5;
    }
    // this case should never happen
    assert (false);
    return -6;
}

int Stub::tryConnect (void)
{
    const char *pszMethodName = "Stub::run";

    if (terminationRequested()) {
        return -1;
    }

    SimpleCommHelper2 *pchCallback = NULL;
    SimpleCommHelper2 *pch = connectToServer (_sHost.c_str(), _ui16Port);
    if (pch != NULL) {
        pchCallback = connectToServer (_sHost.c_str(), _ui16Port);
    }
    if ((pch == NULL) || (pchCallback == NULL)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to initialize CommHelpers\n");
        delete pch;
        delete pchCallback;
        return -2;
    }
    int rc = registerProxy (pch, pchCallback, _ui16ApplicationId);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to register proxy\n");
        delete pch;
        delete pchCallback;
        return -3;
    }
    _ui16ApplicationId = static_cast<uint16>(rc);   // The server may have assigned a different id than requested

    _stubMutex.lock();
    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "connected to proxy server, appid %d\n", _ui16ApplicationId);

    _pCommHelper = pch;
    _pHandler = new StubCallbackHandler (this, pchCallback, _pUnmarshaller);
    _pHandler->start();

    // reregisterListeners();

    _stubMutex.unlock();

    if (_pListener != NULL) {
        _pListener->serverConnected();
    }

    return 0;
}

