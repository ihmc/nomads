/*
 * DisseminationServiceProxy.cpp
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

#include "DisseminationServiceProxy.h"

#include "DisseminationServiceProxyCallbackHandler.h"
#include "DisseminationServiceProxyListener.h"
#include "DisServiceDefs.h"
#include "PeerStatusListener.h"

#include "Logger.h"
#include "NLFLib.h"

#include <string.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define DIS_SVC_PROXY_SERVER_PORT_NUMBER 56487   // Also see DisseminationServiceProxyServer.h

#define CHECK_CONNECTION_OR_FAIL(mutexCode) if (!checkConnection()) { _mutex.unlock ((mutexCode)); return -99; }

DisseminationServiceProxy::DisseminationServiceProxy (uint16 ui16ApplicationId)
    : _mutex (16), _mutexReconnect (17)
{
    _pCommHelper = NULL;
    _pHandler = NULL;
    _pListener = NULL;
    _pPeerStatusListener = NULL;
    _ui16ApplicationId = ui16ApplicationId;
    _bUsingBackgroundReconnect = false;
    _bReconnectStarted = false;
    _pReconnectSemaphore = new Semaphore(0);
}

DisseminationServiceProxy::~DisseminationServiceProxy (void)
{
    //printf("~DisseminationServiceProxy deleting %d\n", _ui16ApplicationId);
    if (_bUsingBackgroundReconnect) {
        //requestTermination();
        _pReconnectSemaphore->up();
        requestTerminationAndWait();
    }

    if (_pHandler) {
        //_pHandler->requestTerminationAndWait();
        delete _pHandler;
        _pHandler = NULL;
    }
    if (_pCommHelper) {
        delete _pCommHelper;
        _pCommHelper = NULL;
    }
    delete _pReconnectSemaphore;
    _pReconnectSemaphore = 0;

    // delete all SubscriptionInfo objects
    SubscriptionInfo *tmp = 0;
    SubscriptionInfo *tmp2 = 0;
    int status = _llSubscriptionInfoList.getFirst(tmp);
    while (status != 0)
        {
        tmp2 = tmp;
        status = _llSubscriptionInfoList.getNext(tmp);
        _llSubscriptionInfoList.remove(tmp2);
        delete tmp2;
        }

    _pListener = NULL;
    _pPeerStatusListener = NULL;
    _pReconnectSemaphore = 0;
}

int DisseminationServiceProxy::init (const char *pszHost, uint16 ui16Port, bool bUseBackgroundReconnect)
{
    if (pszHost == NULL) {
        pszHost = "127.0.0.1";
    }
    if (ui16Port == 0) {
        ui16Port = DIS_SVC_PROXY_SERVER_PORT_NUMBER;
    }

    _sHost = pszHost;
    _ui16Port = ui16Port;
    _bUsingBackgroundReconnect = bUseBackgroundReconnect;

    if (bUseBackgroundReconnect) {
        start();
        if (tryConnect() != 0) {
            startReconnect();
        }
        return 0;
    }
    else {
        return tryConnect();
    }
}

bool DisseminationServiceProxy::startReconnect()
{
    if (!_bUsingBackgroundReconnect) {
        return false;
    }

    _mutexReconnect.lock (135);
    if (_bReconnectStarted) {
        _mutexReconnect.unlock (135);
        return true;
    }
    _bReconnectStarted = true;
    _mutexReconnect.unlock (135);

    // signal reconnect thread
    _pReconnectSemaphore->up();
    return true;
}

// Call this with _mutex held
bool DisseminationServiceProxy::checkConnection()
{
    // Eventually we should have the option to block here
    if (_pCommHelper == 0) {
        return false;
    }

    return true;
}

void DisseminationServiceProxy::run()
{
    started();
    char tmpbuf[80];
    snprintf (tmpbuf, sizeof (tmpbuf)-1, "DSProxyClient %d %p", _ui16ApplicationId, this);
    Thread::setName (tmpbuf);
    while (!terminationRequested()) {
        _pReconnectSemaphore->down();

        if (terminationRequested()) {
            break;
        }
        if (_pListener != 0) {
            _pListener->serverDisconnected();
        }
        checkAndLogMsg ("DisseminationServiceProxy:run", Logger::L_Info, "starting background connect\n");

        _mutex.lock (136);
        if (_pCommHelper != 0)  {
            delete _pCommHelper;
            _pCommHelper = 0;
        }
        if (_pHandler != NULL) {
            delete _pHandler;
            _pHandler = NULL;
        }
        _mutex.unlock (136);

        while (tryConnect() != 0) {
            sleepForMilliseconds(5000);
        }

        _mutexReconnect.lock (137);
        _bReconnectStarted = false;
        _mutexReconnect.unlock (137);
    }
    checkAndLogMsg ("DisseminationServiceProxy:run", Logger::L_LowDetailDebug, "id %d terminating\n", _ui16ApplicationId);
    terminating();
}

int DisseminationServiceProxy::tryConnect()
{
    CommHelper2 *pch = NULL;
    CommHelper2 *pchCallback = NULL;

    pch = connectToServer (_sHost.c_str(), _ui16Port);
    if (pch != NULL) {
        pchCallback = connectToServer (_sHost.c_str(), _ui16Port);
    }
    if ((pch == NULL) || (pchCallback == NULL)) {
        checkAndLogMsg ("DisseminationServiceProxy:tryConnect", Logger::L_MildError,
                        "failed to initialize CommHelpers\n");
        delete pch;
        delete pchCallback;
        return -1;
    }
    int rc = registerProxy (pch, pchCallback, _ui16ApplicationId);
    if (rc < 0) {
        checkAndLogMsg ("DisseminationServiceProxy:tryConnect", Logger::L_MildError,
                        "failed to register proxy\n");
        delete pch;
        delete pchCallback;
        return -2;
    }
    else {
        _ui16ApplicationId = (uint16) rc;   // The server may have assigned a different id than requested
    }

    _mutex.lock (138);
    checkAndLogMsg ("DisseminationServiceProxy:tryConnect", Logger::L_Info,
                    "connected to proxy server, appid %d\n", _ui16ApplicationId);

    _pCommHelper = pch;
    _pHandler = new DisseminationServiceProxyCallbackHandler (this, pchCallback);
    _pHandler->start();

    if (_pListener != 0) {
        registerDisseminationServiceProxyListenerWithServer();
    }
    if (_pPeerStatusListener != 0) {
        registerPeerStatusListenerWithServer();
    }

    // Reregister any subscriptions we may have
    SubscriptionInfo *info;
    int status = _llSubscriptionInfoList.getFirst(info);
    while (status != 0 && info != 0) {
        checkAndLogMsg("DisseminationServiceProxy:tryConnect", Logger::L_Info,
                       "resubscribing to %s\n", info->_sGroupName.c_str());
        registerSubscriptionWithServer(info);
        status = _llSubscriptionInfoList.getNext(info);
    }

    _mutex.unlock (138);

    if (_pListener != 0) {
        _pListener->serverConnected();
    }

    return 0;
}

int DisseminationServiceProxy::getNodeId (char *&pszNodeId)
{
    pszNodeId = NULL;
    _mutex.lock (261);
    CHECK_CONNECTION_OR_FAIL (139);
    try {
        _pCommHelper->sendLine ("getNodeId");
        _pCommHelper->receiveMatch ("OK");
        const char *pszTmp = _pCommHelper->receiveLine();
        if (pszTmp == NULL) {
            _mutex.unlock (261);
            return -1;
        }
        pszNodeId = strDup (pszTmp);
        _mutex.unlock (261);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::getNodeId", Logger::L_MildError,
                        "push failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (261);
        return -2;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::getNodeId", Logger::L_MildError,
                        "push failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (261);
        return -3;
    }
}

int DisseminationServiceProxy::getPeerList (char **&ppszPeerList)
{
    ppszPeerList = NULL;
    _mutex.lock (262);
    CHECK_CONNECTION_OR_FAIL (139);
    try {
        _pCommHelper->sendLine ("getPeerList");
        _pCommHelper->receiveMatch ("OK");

        Reader *pReader = _pCommHelper->getReaderRef();
        uint32 ui32NPeers = 0;
        pReader->read32 (&ui32NPeers);
        if (ui32NPeers > 0) {
            ppszPeerList = (char **) calloc (ui32NPeers+1, sizeof (char*));
            for (unsigned int i = 0; i < ui32NPeers; i++) {
                uint32 ui32StrLen = 0;
                pReader->read32 (&ui32StrLen);
                if (ui32StrLen > 0) {
                    ppszPeerList[i] = (char *) calloc (ui32StrLen+1, sizeof (char));
                    pReader->readBytes (ppszPeerList[i], ui32StrLen);
                }
            }
        }

        _mutex.unlock (262);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::getNodeId", Logger::L_MildError,
                        "push failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (262);
        return -2;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::getNodeId", Logger::L_MildError,
                        "push failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (262);
        return -3;
    }
}

int DisseminationServiceProxy::store (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                                      const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length,
                                      int64 i64ExpirationTime, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority,
                                      char *pszIdBuf, uint32 ui32IdBufLen)
{
    _mutex.lock (139);
    CHECK_CONNECTION_OR_FAIL (139);
    try {
        _pCommHelper->sendLine ("store");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->sendStringBlock (pszObjectId);
        _pCommHelper->sendStringBlock (pszInstanceId);
        _pCommHelper->sendStringBlock (pszMimeType);
        _pCommHelper->getWriterRef()->write32 (&ui32MetadataLength);
        if (ui32MetadataLength > 0) {
            _pCommHelper->sendBlob (pMetadata, ui32MetadataLength);
        }
        _pCommHelper->getWriterRef()->write32 (&ui32Length);
        _pCommHelper->sendBlob (pData, ui32Length);
        _pCommHelper->getWriterRef()->write64 (&i64ExpirationTime);
        _pCommHelper->getWriterRef()->write16 (&ui16HistoryWindow);
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);
        _pCommHelper->getWriterRef()->write8 (&ui8Priority);

        _pCommHelper->receiveMatch ("OK");
        const char *pszMsgId = _pCommHelper->receiveLine();
        if (pszIdBuf != NULL) {
            strncpy (pszIdBuf, pszMsgId, ui32IdBufLen);
            pszIdBuf [ui32IdBufLen-1] = '\0';
        }
        _mutex.unlock (139);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::store", Logger::L_MildError,
                        "store failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (139);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::store", Logger::L_MildError,
                        "store failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (139);
        return -2;
    }
}

int DisseminationServiceProxy::push (char *pszMsgId)
{
    _mutex.lock (139);
    CHECK_CONNECTION_OR_FAIL (139);
    try {
        _pCommHelper->sendLine ("pushById");
        _pCommHelper->sendStringBlock (pszMsgId);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (139);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::push", Logger::L_MildError,
                        "push failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (139);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::push", Logger::L_MildError,
                        "push failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (139);
        return -2;
    }
}

int DisseminationServiceProxy::push (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                                     const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length,
                                     int64 i64ExpirationTime, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority,
                                     char *pszIdBuf, uint32 ui32IdBufLen)
{
    _mutex.lock (139);
    CHECK_CONNECTION_OR_FAIL (139);
    try {
        _pCommHelper->sendLine ("push");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->sendStringBlock (pszObjectId);
        _pCommHelper->sendStringBlock (pszInstanceId);
        _pCommHelper->sendStringBlock (pszMimeType);
        _pCommHelper->getWriterRef()->write32 (&ui32MetadataLength);
        if (ui32MetadataLength > 0) {
            _pCommHelper->sendBlob (pMetadata, ui32MetadataLength);
        }
        _pCommHelper->getWriterRef()->write32 (&ui32Length);
        _pCommHelper->sendBlob (pData, ui32Length);
        _pCommHelper->getWriterRef()->write64 (&i64ExpirationTime);
        _pCommHelper->getWriterRef()->write16 (&ui16HistoryWindow);
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);
        _pCommHelper->getWriterRef()->write8 (&ui8Priority);

        _pCommHelper->receiveMatch ("OK");
        const char *pszMsgId = _pCommHelper->receiveLine();
        if (pszIdBuf != NULL) {
            strncpy (pszIdBuf, pszMsgId, ui32IdBufLen);
            pszIdBuf [ui32IdBufLen-1] = '\0';
        }
        _mutex.unlock (139);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::push", Logger::L_MildError,
                        "push failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (139);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::push", Logger::L_MildError,
                        "push failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (139);
        return -2;
    }
}

int DisseminationServiceProxy::makeAvailable (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                              const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32Length,
                                              const char *pszDataMimeType, int64 i64Expiration, uint16 ui16HistoryWindow, uint16 ui16Tag,
                                              uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen)
{
    _mutex.lock (140);
    CHECK_CONNECTION_OR_FAIL (140);
    try {
        _pCommHelper->sendLine ("makeAvailable");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->sendStringBlock (pszObjectId);
        _pCommHelper->sendStringBlock (pszInstanceId);
        _pCommHelper->getWriterRef()->write32 (&ui32MetadataLength);
        _pCommHelper->sendBlob (pMetadata, ui32MetadataLength);
        _pCommHelper->getWriterRef()->write32 (&ui32Length);
        _pCommHelper->sendBlob (pData, ui32Length);
        _pCommHelper->sendStringBlock (pszDataMimeType);
        _pCommHelper->getWriterRef()->write64 (&i64Expiration);
        _pCommHelper->getWriterRef()->write16 (&ui16HistoryWindow);
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);
        _pCommHelper->getWriterRef()->write8 (&ui8Priority);

        _pCommHelper->receiveMatch ("OK");
        const char *pszMsgId = _pCommHelper->receiveLine();
        if (pszIdBuf != NULL) {
            strncpy (pszIdBuf, pszMsgId, ui32IdBufLen);
            pszIdBuf [ui32IdBufLen-1] = '\0';
        }
        _mutex.unlock (140);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::makeAvailable", Logger::L_MildError,
                        "makeAvailable failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (140);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::makeAvailable", Logger::L_MildError,
                        "makeAvailable failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (140);
        return -2;
    }
}

int DisseminationServiceProxy::makeAvailable (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                              const void *pMetadata, uint32 ui32MetadataLength, const char *pszFilePath,
                                              const char *pszDataMimeType, int64 i64Expiration, uint16 ui16HistoryWindow,
                                              uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen)
{
    _mutex.lock (142);
    CHECK_CONNECTION_OR_FAIL(142);
    try {
        _pCommHelper->sendLine ("makeAvailable");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->sendStringBlock (pszObjectId);
        _pCommHelper->sendStringBlock (pszInstanceId);
        _pCommHelper->getWriterRef()->write32 (&ui32MetadataLength);
        _pCommHelper->sendBlob (pMetadata, ui32MetadataLength);
        _pCommHelper->sendLine (pszFilePath);
        _pCommHelper->sendStringBlock (pszDataMimeType);
        _pCommHelper->getWriterRef()->write64 (&i64Expiration);
        _pCommHelper->getWriterRef()->write16 (&ui16HistoryWindow);
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);
        _pCommHelper->getWriterRef()->write8 (&ui8Priority);

        _pCommHelper->receiveMatch ("OK");
        const char *pszMsgId = _pCommHelper->receiveLine();
        if (pszIdBuf != NULL) {
            strncpy (pszIdBuf, pszMsgId, ui32IdBufLen);
            pszIdBuf [ui32IdBufLen-1] = '\0';
        }
        _mutex.unlock (142);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::makeAvailable", Logger::L_MildError,
                        "makeAvailable failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (142);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::makeAvailable", Logger::L_MildError,
                        "makeAvailable failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (142);
        return -2;
    }
}

int DisseminationServiceProxy::cancel (const char *pszId)
{
    _mutex.lock (143);
    CHECK_CONNECTION_OR_FAIL(143);
    try {
        _pCommHelper->sendLine ("cancel_str");
        _pCommHelper->sendLine (pszId);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (143);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::cancel", Logger::L_MildError,
                        "cancel failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (143);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::cancel", Logger::L_MildError,
                        "cancel failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (143);
        return -2;
    }
}

int DisseminationServiceProxy::cancel (uint16 ui16Tag)
{
    _mutex.lock (144);
    CHECK_CONNECTION_OR_FAIL(144);
    try {
        _pCommHelper->sendLine ("cancel_str");
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (144);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::cancel", Logger::L_MildError,
                        "cancel failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (144);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::cancel", Logger::L_MildError,
                        "cancel failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (144);
        return -2;
    }
}

int DisseminationServiceProxy::addFilter (const char *pszGroupName, uint16 ui16Tag)
{
    _mutex.lock (145);
    CHECK_CONNECTION_OR_FAIL(145);
    try {
        _pCommHelper->sendLine ("addFilter");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (145);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::addFilter", Logger::L_MildError,
                        "addFilter failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (145);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::addFilter", Logger::L_MildError,
                        "addFilter failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (145);
        return -2;
    }
}

int DisseminationServiceProxy::removeFilter (const char *pszGroupName, uint16 ui16Tag)
{
    _mutex.lock (146);
    CHECK_CONNECTION_OR_FAIL(146);
    try {
        _pCommHelper->sendLine ("removeFilter");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (146);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::removeFilter", Logger::L_MildError,
                        "removeFilter failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (146);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::removeFilter", Logger::L_MildError,
                        "removeFilter failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (146);
        return -2;
    }
}

// Call this with mutex held
int DisseminationServiceProxy::registerSubscriptionWithServer (SubscriptionInfo *sub)
{
    uint8 ui8Reliable;
    uint8 ui8MsgReliable;
    uint8 ui8Sequenced;

    if (_pCommHelper == 0) {
        return -1;
    }
    try {
        if (sub->bUsingTag) {
            _pCommHelper->sendLine ("subscribe_tag");
            _pCommHelper->sendLine (sub->_sGroupName.c_str());
            uint16 ui16 = sub->_ui16Tag;
            _pCommHelper->getWriterRef()->write16 (&ui16);
            uint8 ui8 = sub->ui8Priority;
            _pCommHelper->getWriterRef()->write8 (&ui8);
            const bool bRel = sub->bReliable;
            _pCommHelper->getWriterRef()->writeBool (&bRel);

            if ((sub->bReliable) || (sub->bMsgReliable)) {
                ui8MsgReliable = 1;
            }
            else {
                ui8MsgReliable = 0;
            }
            _pCommHelper->getWriterRef()->write8 (&ui8MsgReliable);

            if (sub->bSequenced) {
                ui8Sequenced = 1;
            }
            else {
                ui8Sequenced = 0;
            }
            _pCommHelper->getWriterRef()->write8 (&ui8Sequenced);
        }
        else {
            _pCommHelper->sendLine ("subscribe");
            _pCommHelper->sendLine (sub->_sGroupName.c_str());
            _pCommHelper->getWriterRef()->write8 (&sub->ui8Priority);
            if (sub->bReliable) {
                ui8Reliable = 1;
            }
            else {
                ui8Reliable = 0;
            }
            _pCommHelper->getWriterRef()->write8 (&ui8Reliable);

            if ((sub->bReliable) || (sub->bMsgReliable)) {
                ui8MsgReliable = 1;
            }
            else {
                ui8MsgReliable = 0;
            }
            _pCommHelper->getWriterRef()->write8 (&ui8MsgReliable);

            if (sub->bSequenced) {
                ui8Sequenced = 1;
            }
            else {
                ui8Sequenced = 0;
            }
            _pCommHelper->getWriterRef()->write8 (&ui8Sequenced);
        }

        _pCommHelper->receiveMatch ("OK");
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::registerSubscriptionWithServer", Logger::L_MildError,
                        "subscribe failed with a protocol exception; msg = %s\n",
            pe.getMsg());
        return -2;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::registerSubscriptionWithServer", Logger::L_MildError,
                        "subscribe failed with a comm exception; msg = %s\n",
            ce.getMsg());
        startReconnect();
        return -3;
    }
}

int DisseminationServiceProxy::subscribe (const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced)
{
    SubscriptionInfo *pInfo = new SubscriptionInfo (pszGroupName, 0, ui8Priority, false, bGroupReliable, bMsgReliable, bSequenced);
    _mutex.lock (147);
    // FIXME: should probably check for duplicate subscription
    _llSubscriptionInfoList.add (pInfo);
    int rc = registerSubscriptionWithServer (pInfo);
    _mutex.unlock (147);

    return rc;
}

int DisseminationServiceProxy::unsubscribe (const char *pszGroupName)
{
    _mutex.lock (148);
    SubscriptionInfo *info;
    int status = _llSubscriptionInfoList.getFirst(info);
    while (status != 0 && info != 0) {
        if (info->_sGroupName == pszGroupName && info->bUsingTag == false) {
            _llSubscriptionInfoList.remove(info);
            delete info;
            info = 0;
        }
        else {
            status = _llSubscriptionInfoList.getNext(info);
        }
    }

    if (_pCommHelper != 0) {
        try {
            _pCommHelper->sendLine ("unsubscribe");
            _pCommHelper->sendLine (pszGroupName);

            _pCommHelper->receiveMatch ("OK");
            _mutex.unlock (148);
            return 0;
        }
        catch (ProtocolException pe) {
            checkAndLogMsg ("DisseminationServiceProxy::unsubscribe", Logger::L_MildError,
                            "unsubscribe failed with a protocol exception; msg = %s\n",
                pe.getMsg());
            _mutex.unlock (148);
            return -1;
        }
        catch (CommException ce) {
            checkAndLogMsg ("DisseminationServiceProxy::unsubscribe", Logger::L_MildError,
                "unsubscribe failed with a comm exception; msg = %s\n",
                ce.getMsg());
            startReconnect();
            _mutex.unlock (148);
            return -2;
        }
    }
    return 0;
}

int DisseminationServiceProxy::subscribe (const char *pszGroupName, uint16 ui16Tag, uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced)
{
    SubscriptionInfo *pInfo = new SubscriptionInfo (pszGroupName, ui16Tag, ui8Priority, true, bGroupReliable, bMsgReliable, bSequenced);
    _mutex.lock (149);
    // FIXME: should probably check for duplicate subscription
    _llSubscriptionInfoList.add (pInfo);
    int rc = registerSubscriptionWithServer (pInfo);
    _mutex.unlock (149);

    return rc;
}

int DisseminationServiceProxy::subscribeWithXPathPredicate (const char *pszGroupName, const char *pszXPathPredicate,
                                                            uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced)
{
   // TODO: implement this!!!
   checkAndLogMsg ("DisseminationServiceProxy::subscribeWithXPathPredicate", Logger::L_SevereError,
                   "Method not implemented yet");
   return -1;
}

int DisseminationServiceProxy::unsubscribe (const char *pszGroupName, uint16 ui16Tag)
{
    _mutex.lock (150);
    SubscriptionInfo *info;
    int status = _llSubscriptionInfoList.getFirst(info);
    while (status != 0 && info != 0) {
        if (info->_sGroupName == pszGroupName && info->bUsingTag == true && info->_ui16Tag == ui16Tag) {
            _llSubscriptionInfoList.remove(info);
            delete info;
            info = 0;
        }
        else {
            status = _llSubscriptionInfoList.getNext(info);
        }
    }

    if (_pCommHelper != 0) {
        try {
            _pCommHelper->sendLine ("unsubscribe_tag");
            _pCommHelper->sendLine (pszGroupName);
            _pCommHelper->getWriterRef()->write16 (&ui16Tag);

            _pCommHelper->receiveMatch ("OK");
            _mutex.unlock (150);
            return 0;
        }
        catch (ProtocolException pe) {
            checkAndLogMsg ("DisseminationServiceProxy::unsubscribe_tag", Logger::L_MildError,
                            "unsubscribe_tag failed with a protocol exception; msg = %s\n",
                            pe.getMsg());
            _mutex.unlock (150);
            return -1;
        }
        catch (CommException ce) {
            checkAndLogMsg ("DisseminationServiceProxy::unsubscribe_tag", Logger::L_MildError,
                            "unsubscribe_tag failed with a comm exception; msg = %s\n",
                           ce.getMsg());
            startReconnect();
            _mutex.unlock (150);
            return -2;
        }
    }
    return 0;
}

int DisseminationServiceProxy::registerDisseminationServiceProxyListener (DisseminationServiceProxyListener *pListener)
{
    if (pListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:registerDisseminationServiceProxyListener",
                        Logger::L_MildError, "pListener is NULL\n");
        return -1;
    }
    _mutex.lock (151);
    _pListener = pListener;
    registerDisseminationServiceProxyListenerWithServer();
    _mutex.unlock (151);

    return 0;
}

// Call this with _mutex held
void DisseminationServiceProxy::registerDisseminationServiceProxyListenerWithServer()
{
    if (_pCommHelper == 0) {
        return;
    }
    try {
        _pCommHelper->sendLine ("registerDataArrivedCallback");
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::registerDisseminationServiceProxyListener", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
    }
}

int DisseminationServiceProxy::registerPeerStatusListener (PeerStatusListener *pListener)
{
    if (pListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:registerPeerStatusListener", Logger::L_MildError,
                        "pListener is NULL\n");
        return -1;
    }
    _mutex.lock (152);
    _pPeerStatusListener = pListener;
    registerPeerStatusListenerWithServer();
    _mutex.unlock (152);

    return 0;
}

int DisseminationServiceProxy::registerSearchListener (SearchListener *pListener)
{
    if (pListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:registerSearchListener", Logger::L_MildError,
                        "pListener is NULL\n");
        return -1;
    }
    _mutex.lock (152);
    _pSearchListener = pListener;
    registerSearchListenerWithServer();
    _mutex.unlock (152);

    return 0;
}

// Call this with _mutex held
void DisseminationServiceProxy::registerPeerStatusListenerWithServer()
{
    try {
        _pCommHelper->sendLine ("registerPeerStatusListener");
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::registerPeerStatusListener", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
    }
}

void DisseminationServiceProxy::registerSearchListenerWithServer()
{
    try {
        _pCommHelper->sendLine ("registerSearchListener");
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::registerSearchListenerWithServer", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
    }
}

int DisseminationServiceProxy::retrieve (const char *pszId, void **ppBuf, uint32 *pui32BufSize, int64 i64Timeout)
{
    _mutex.lock (153);
    CHECK_CONNECTION_OR_FAIL(153);
    try {
        _pCommHelper->sendLine ("retrieve");
        _pCommHelper->sendLine (pszId);
        _pCommHelper->getWriterRef()->write64 (&i64Timeout);
        _pCommHelper->receiveMatch ("OK");

        // Read length of object being retrieved
        _pCommHelper->getReaderRef()->read32 (pui32BufSize);

        if (*pui32BufSize > 0) {
            // Allocate buffer of proper length and read large object
            void * pBuf = malloc ((*pui32BufSize));
            _pCommHelper->receiveBlob (pBuf, (*pui32BufSize));
            _pCommHelper->receiveMatch ("OK");
            (*ppBuf) = pBuf;
        }
        else {
            // Nothing is available to retrieve
            _pCommHelper->receiveMatch ("OK");
            (*ppBuf) = NULL;
        }
        _mutex.unlock (153);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::retrieve", Logger::L_MildError,
                        "request failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        ppBuf = NULL;
        *pui32BufSize = 0;
        _mutex.unlock (153);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::retrieve", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        ppBuf = NULL;
        *pui32BufSize = 0;
        startReconnect();
        _mutex.unlock (153);
        return -2;
    }
}

int DisseminationServiceProxy::retrieve (const char *pszId, const char *pszFilePath, int64 i64Timeout)
{
    _mutex.lock (154);
    CHECK_CONNECTION_OR_FAIL(154);
    try {
        _pCommHelper->sendLine ("retrieve_file");
        _pCommHelper->sendLine (pszId);
         _pCommHelper->sendLine (pszFilePath);
        _pCommHelper->getWriterRef()->write64 (&i64Timeout);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (154);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::retrieve", Logger::L_MildError,
                        "request failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (154);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::retrieve", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (154);
        return -2;
    }
}

int DisseminationServiceProxy::request (const char *pszGroupName, uint16 ui16Tag, uint16 ui16HistoryLength, int64 i64RequestTimeout)
{
    _mutex.lock (155);
    CHECK_CONNECTION_OR_FAIL(155);
    try {
        _pCommHelper->sendLine ("request");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->getWriterRef()->write16 (&ui16Tag);
        _pCommHelper->getWriterRef()->write16 (&ui16HistoryLength);
        _pCommHelper->getWriterRef()->write64 (&i64RequestTimeout);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (155);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::request", Logger::L_MildError,
                        "request failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (155);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::request", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (155);
        return -2;
    }
}

int DisseminationServiceProxy::requestMoreChunks (const char *pszMsgId)
{
    _mutex.lock (156);
    CHECK_CONNECTION_OR_FAIL(156);
    try {
        _pCommHelper->sendLine ("requestMoreChunksByID");
        _pCommHelper->sendLine (pszMsgId);
        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (156);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::requestMoreChunks2", Logger::L_MildError,
                        "request failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (156);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::requestMoreChunks2", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (156);
        return -2;
    }
}

int DisseminationServiceProxy::requestMoreChunks (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId)
{
    _mutex.lock (157);
    CHECK_CONNECTION_OR_FAIL(157);
    try {
        _pCommHelper->sendLine ("requestMoreChunks");
        _pCommHelper->sendLine (pszGroupName);
        _pCommHelper->sendLine (pszSenderNodeId);
        _pCommHelper->getWriterRef()->write32 (&ui32MsgSeqId);

        _pCommHelper->receiveMatch ("OK");
        _mutex.unlock (157);
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::requestMoreChunks2", Logger::L_MildError,
                        "request failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (157);
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::requestMoreChunks2", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (157);
        return -2;
    }
}

int DisseminationServiceProxy::search (const char *pszGroupName, const char *pszQueryType,
                                       const char *pszQueryQualifiers, const void *pszQuery,
                                       unsigned int uiQueryLen, char **ppszQueryId)
{
    if (ppszQueryId == NULL) {
        return -1;
    }

    _mutex.lock (157);
    CHECK_CONNECTION_OR_FAIL(157);
    try {
        _pCommHelper->sendLine ("search");
        _pCommHelper->sendStringBlock (pszGroupName);
        _pCommHelper->sendStringBlock (pszQueryType);
        _pCommHelper->sendStringBlock (pszQueryQualifiers);
        _pCommHelper->getWriterRef()->write32 (&uiQueryLen);
        if (uiQueryLen > 0) {
            _pCommHelper->sendBlob (pszQuery, uiQueryLen);
        }

        _pCommHelper->receiveMatch ("OK");

        // read the ID assigned to the query
        uint32 ui32;
        char buf[256];
        _pCommHelper->getReaderRef()->read32 (&ui32);
        String queryId;
        if (ui32 > 0) {
            if (ui32 > 127) {
                return -2;
            }
            _pCommHelper->receiveBlob (buf, ui32);
            buf[ui32] = '\0';
            queryId = buf;
        }

        *ppszQueryId = queryId.r_str();
        return 0;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy::search", Logger::L_MildError,
                        "request failed with a protocol exception; msg = %s\n",
                        pe.getMsg());
        _mutex.unlock (157);
        return -3;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::search", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (157);
        return -4;
    }
    return 0;
}

int DisseminationServiceProxy::searchReply (const char *pszQueryId, const char **ppszMatchingQueryIds)
{
    if ((pszQueryId == NULL) || (ppszMatchingQueryIds == NULL)) {
        return -1;
    }

    _mutex.lock (157);
    CHECK_CONNECTION_OR_FAIL(157);
    try {
        _pCommHelper->sendLine ("replyToQuery");
        _pCommHelper->sendStringBlock (pszQueryId);
        unsigned int uiCount = 0;
        for (; ppszMatchingQueryIds[uiCount] != NULL; uiCount++);
        _pCommHelper->getWriterRef()->write32 (&uiCount);
        for (unsigned int i = 0; i < uiCount; i++) {
            _pCommHelper->sendStringBlock (ppszMatchingQueryIds[i]);
        }
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy::searchReply", Logger::L_MildError,
                        "request failed with a comm exception; msg = %s\n",
                        ce.getMsg());
        startReconnect();
        _mutex.unlock (157);
        return -4;
    }
    return 0;
}

bool DisseminationServiceProxy::isConnected()
{
    return (_pCommHelper != 0);
}

CommHelper2 * DisseminationServiceProxy::connectToServer (const char *pszHost, uint16 ui16Port)
{
    int rc;
    TCPSocket *pSocket = new TCPSocket();
    if (0 != (rc = pSocket->connect (pszHost, ui16Port))) {
        checkAndLogMsg ("DisseminationServiceProxy:connectToServer", Logger::L_MildError,
                        "failed to connect to remote host %s on port %d; rc = %d\n", pszHost, ui16Port, rc);
        delete pSocket;
        return NULL;
    }

    CommHelper2 *pch = new CommHelper2();
    if (0 != (rc = pch->init (pSocket))) {
        checkAndLogMsg ("DisseminationServiceProxy:connectToServer", Logger::L_MildError,
                        "failed to initialize CommHelper; rc = %d\n", rc);
        delete pSocket;
        delete pch;
        return NULL;
    }
    pch->setDeleteUnderlyingSocket (true);

    return pch;
}

int DisseminationServiceProxy::registerProxy (CommHelper2 *pch, CommHelper2 *pchCallback, uint16 ui16DesiredApplicationId)
{
    uint16 ui16ApplicationId;
    try {
        // First register the proxy using the desired application id
        // The ProxyServer will return the assigned application id
        pch->sendLine ("RegisterProxy %d", (int) ui16DesiredApplicationId);
        char szId[10];
        pch->receiveRemainingLine (szId, sizeof (szId), "OK");
        ui16ApplicationId = (uint16) atoi (szId);

        // Now register the callback using the assigned application id
        pchCallback->sendLine ("RegisterProxyCallback %d", (int) ui16ApplicationId);
        pchCallback->receiveMatch ("OK");
        return ui16ApplicationId;
    }
    catch (ProtocolException pe) {
        checkAndLogMsg ("DisseminationServiceProxy:initCommHelper", Logger::L_MildError,
                        "failed with protocol exception %s\n", pe.getMsg());
        return -1;
    }
    catch (CommException ce) {
        checkAndLogMsg ("DisseminationServiceProxy:initCommHelper", Logger::L_MildError,
                        "failed with comm exception %s\n", ce.getMsg());
        return -2;
    }
}

bool DisseminationServiceProxy::dataArrived (const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                             const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                                             const void *pData, uint32 ui32Length, uint32 ui32MetadataLength,
                                             uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)

{
    if (_pListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:dataArrived", Logger::L_MildError,
                        "DisseminationServiceProxy listener is null\n");
        return false;
    }
    return _pListener->dataArrived (pszSender, pszGroupName, ui32SeqId, pszObjectId,
                                    pszInstanceId, pszMimeType, pData, ui32Length, ui32MetadataLength,
                                    ui16Tag, ui8Priority, pszQueryId);
}

bool DisseminationServiceProxy::chunkArrived (const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                              const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                                              const void *pChunk, uint32 ui32Length, uint8 ui8NChunks, uint8 ui8TotNChunks,
                                              const char *pszChunkedMsgId, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    if (_pListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy::chunkArrived", Logger::L_MildError,
                        "DisseminationServiceProxy listener is null\n");
        return false;
    }
    return _pListener->chunkArrived (pszSender, pszGroupName, ui32SeqId, pszObjectId,
                                     pszInstanceId, pszMimeType, pChunk, ui32Length, ui8NChunks,
                                     ui8TotNChunks, pszChunkedMsgId, ui16Tag, ui8Priority, pszQueryId);
}

bool DisseminationServiceProxy::metadataArrived (const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                                 const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType,
                                                 const void *pMetadata, uint32 ui32MetadataLength, bool bDataChunked,
                                                 uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    if (_pListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:metadataArrived", Logger::L_MildError,
                        "DisseminationServiceProxy listener is null\n");
        return false;
    }
    return _pListener->metadataArrived (pszSender, pszGroupName, ui32SeqId, pszObjectId,
                                        pszInstanceId, pszMimeType, pMetadata, ui32MetadataLength,
                                        bDataChunked, ui16Tag, ui8Priority, pszQueryId);
}

bool DisseminationServiceProxy::dataAvailable (const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                               const char *pszObjectId, const char *pszMimeType, const char *pszInstanceId,
                                               const char *pszId, const void *pMetadata, uint32 ui32MetadataLength,
                                               uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    if (_pListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:dataAvailable", Logger::L_MildError,
                          "DisseminationServiceProxy listener is null\n");
        return false;
    }
    return _pListener->dataAvailable (pszSender, pszGroupName, ui32SeqId,
                                      pszObjectId, pszInstanceId, pszMimeType,
                                      pszId, pMetadata, ui32MetadataLength,
                                      ui16Tag, ui8Priority, pszQueryId);
}

bool DisseminationServiceProxy::newPeer (const char *pszPeerId)
{
    if (_pPeerStatusListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:newPeer", Logger::L_MildError,
                          "DisseminationServiceProxy listener is null\n");
        return false;
    }
    return _pPeerStatusListener->newPeer (pszPeerId);
}

bool DisseminationServiceProxy::deadPeer (const char *pszPeerId)
{
    if (_pPeerStatusListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:deadPeer", Logger::L_MildError,
                          "DisseminationServiceProxy listener is null\n");
        return false;
    }
    return _pPeerStatusListener->deadPeer (pszPeerId);
}

bool DisseminationServiceProxy::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                               const char *pszQuerier, const char *pszQueryType,
                                               const char *pszQueryQualifiers,
                                               const void *pszQuery, unsigned int uiQueryLen)
{
    if (_pSearchListener == NULL) {
        checkAndLogMsg ("DisseminationServiceProxy:searchArrived", Logger::L_MildError,
                          "DisseminationServiceProxy listener is null\n");
        return false;
    }
    _pSearchListener->searchArrived (pszQueryId, pszGroupName, pszQuerier, pszQueryType,
                                     pszQueryQualifiers, pszQuery, uiQueryLen);
    return true;
}

