/*
 * DisseminationServiceProxyAdaptor.cpp
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

#include "DisseminationServiceProxyAdaptor.h"

/*
 * TODO: Check the size of static buffers being used to read parameters from the client
 */

#include "DisseminationService.h"
#include "DisServiceDefs.h"
#include "DisseminationServiceProxyServer.h"

#include "CommHelper2.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DisseminationServiceProxyAdaptor::DisseminationServiceProxyAdaptor (DisseminationServiceProxyServer *pDSProxyServer)
    : SearchListener ("DisseminationServiceProxyAdaptor"), _mutex (18)
{
    _pDissSvc = pDSProxyServer->getDisseminationServiceRef();
    _pDissSvcProxyServer = pDSProxyServer;
    _pCallbackCommHelper = NULL;
    _bListenerRegistered = false;
    _bPeerStatusListenerRegistered = false;
    _bSearchListenerRegistered = false;
    _uiSearchListenerIndex = 0;
}

DisseminationServiceProxyAdaptor::~DisseminationServiceProxyAdaptor()
{
    checkAndLogMsg ("DisseminationServiceProxyAdaptor::~DisseminationServiceProxyAdaptor",
		    Logger::L_HighDetailDebug, "deleting adaptor\n");

    // Deregister the proxies before closing the connections! Otherwise they
    // could get called when the connection has been/is being closed, leading to
    // an error.
    if (_bListenerRegistered) {
        _pDissSvc->deregisterDisseminationServiceListener (getClientID(), this);
        _bListenerRegistered = false;
    }
    if (_bPeerStatusListenerRegistered) {
        _pDissSvc->deregisterPeerStatusListener (getClientID(), this);
        _bPeerStatusListenerRegistered = false;
    }
    if (_bSearchListenerRegistered) {
        _pDissSvc->deregisterSearchListener (_uiSearchListenerIndex);
        _bSearchListenerRegistered = false;
    }

    char szId[10];
    snprintf (szId, sizeof(szId)-1, "%d", _ui16ClientID);
    _pDissSvcProxyServer->_proxies.remove (szId);

    // Close connections
    if (_pCommHelper != NULL) {
        CommHelperError error = SimpleCommHelper2::None;
         _pCommHelper->closeConnection (error);
        delete _pCommHelper;
        _pCommHelper = NULL;
    }
    if (_pCallbackCommHelper != NULL) {
        CommHelperError error = SimpleCommHelper2::None;
         _pCallbackCommHelper->closeConnection (error);
        delete _pCallbackCommHelper;
        _pCallbackCommHelper = NULL;
    }
}

int DisseminationServiceProxyAdaptor::init (SimpleCommHelper2 *pCommHelper, uint16 ui16ID)
{
    _pCommHelper = pCommHelper;
    _adptProtHelper.setCommHelper (pCommHelper);
    _ui16ClientID = ui16ID;

    return 0;
}

void DisseminationServiceProxyAdaptor::setCallbackCommHelper(SimpleCommHelper2 *pCommHelper)
{
    _mutex.lock (157);
    if (_pCallbackCommHelper) {
        delete _pCallbackCommHelper;
    }

    _pCallbackCommHelper = pCommHelper;
    _adptProtHelper.setCbackCommHelper (pCommHelper);
    _mutex.unlock (157);
}

void DisseminationServiceProxyAdaptor::run()
{
    const char *pszMethodName = "DisseminationServiceProxyAdaptor::run";
    char tmpbuf[50];
    snprintf (tmpbuf, sizeof (tmpbuf)-1, "%s-%d", pszMethodName, getClientID());
    setName (tmpbuf);

    SimpleCommHelper2::Error err = SimpleCommHelper2::None;
    char buf[128];
    int size;

    while (!terminationRequested()) {
        err = SimpleCommHelper2::None;
        size = _pCommHelper->receiveLine (buf, sizeof (buf), err);
        if (err != SimpleCommHelper2::None) {
            break;
        }
        if (size <= 0) {
            continue;
        }

        bool bSuccess;

        if (strcmp (buf, "getNodeId") == 0) {
            bSuccess = doGetNodeId (err);
        }
        else if (strcmp (buf, "getPeerList") == 0) {
            bSuccess = doGetPeerList (err);
        }
        else if (strcmp (buf, "getDisServiceId") == 0) {
            bSuccess = doGetDisServiceIds (err);
        }
        else if (strcmp (buf, "store") == 0) {
            bSuccess = doPushOrStore (err, DisseminationServiceProxyAdaptor::STORE);
        }
        else if (strcmp (buf, "pushById") == 0) {
            bSuccess = doPushById (err);
        }
        else if (strcmp (buf, "push") == 0) {
            bSuccess = doPushOrStore (err, DisseminationServiceProxyAdaptor::PUSH);
        }
        else if (strcmp (buf, "makeAvailable") == 0) {
            bSuccess = doPushOrStore (err, DisseminationServiceProxyAdaptor::MAKE_AVAILABLE);
        }
        else if (strcmp (buf, "makeAvailable_file") == 0) {
            bSuccess = doMakeAvailableFile (err);
        }
        else if (strcmp (buf, "cancel_int") == 0) {
            bSuccess = doCancel_int (err);
        }
        else if (strcmp (buf, "cancel_str") == 0) {
            bSuccess = doCancel_psz (err);
        }
        else if (strcmp (buf, "subscribe") == 0) {
            bSuccess = doSubscribe (err);
        }
        else if (strcmp (buf, "subscribe_tag") == 0) {
            bSuccess = doSubscribe_tag (err);
        }
        else if (strcmp (buf, "subscribe_predicate") == 0) {
            bSuccess = doSubscribe_predicate (err);
        }
        else if (strcmp (buf, "unsubscribe") == 0) {
            bSuccess = doUnsubscribe (err);
        }
        else if (strcmp (buf, "unsubscribe_tag") == 0) {
            bSuccess = doUnsubscribe_tag (err);
        }
        else if (strcmp (buf, "addFilter") == 0) {
            bSuccess = doAddFilter (err);
        }
        else if (strcmp (buf, "removeFilter") == 0) {
            bSuccess = doRemoveFilter (err);
        }
        else if (strcmp (buf, "retrieve") == 0) {
            bSuccess = doRetrieve (err);
        }
        else if (strcmp (buf, "retrieve_file") == 0) {
            bSuccess = doRetrieve_file (err);
        }
        else if (strcmp (buf, "request") == 0) {
            bSuccess = doHistoryRequest (err);
        }
        else if (strcmp (buf, "requestMoreChunks") == 0) {
            bSuccess = doRequestMoreChunks (err);
        }
        else if (strcmp (buf, "requestMoreChunksByID") == 0) {
            bSuccess = doRequestMoreChunksByID (err);
        }
        else if (strcmp (buf, "registerDataArrivedCallback") == 0) {
            bSuccess = doRegisterDataArrivedCallback (err);
        }
        else if (strcmp (buf, "registerChunkArrivedCallback") == 0) {
            bSuccess = doRegisterChunkArrivedCallback (err);
        }
        else if (strcmp (buf, "registerMetadataArrivedCallback") == 0) {
            bSuccess = doRegisterMetadataArrivedCallback (err);
        }
        else if (strcmp (buf, "registerDataAvailableCallback") == 0) {
            bSuccess = doRegisterDataAvailableCallback (err);
        }
        else if (strcmp (buf, "registerPeerStatusCallback") == 0) {
            bSuccess =  doRegisterPeerStatusCallback (err);
        }
        else if (strcmp (buf, "registerSearchListener") == 0) {
            bSuccess =  doRegisterSearchListener (err);
        }
        else if (strcmp (buf, "resetTransmissionHistory") == 0) {
            bSuccess =  doResetTransmissionHistory (err);
        }
        else if (strcmp (buf, "getNextPushId") == 0) {
            bSuccess = doGetNextPushId (err);
        }
        else if (strcmp (buf, "search") == 0) {
            bSuccess = doSearch (err);
        }
        else if (strcmp (buf, "replyToQuery") == 0) {
            bSuccess = doSearchReply (err);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "unknown operation <%s> requested by client\n", buf);
            _pCommHelper->sendLine (err, "ERROR");
            if (err != SimpleCommHelper2::None) {
                break;
            }
            bSuccess = false;
        }

        if ((!bSuccess) && (err == SimpleCommHelper2::None)) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "failed to execute operation <%s> on behalf of client\n", buf);
            _pCommHelper->sendLine (err, "ERROR");
            if (err != SimpleCommHelper2::None) {
                break;
            }
        }
    } //while(true);

    if (err != SimpleCommHelper2::None) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "exception in handling client request.\n");
    }

    // The destructor will take care of closing all the connections and
    // de-registering this Adaptor from the DisseminationServiceProxyServer
    terminating();
    delete this;
}

uint16 DisseminationServiceProxyAdaptor::getClientID (void)
{
    return _ui16ClientID;
}

bool DisseminationServiceProxyAdaptor::doGetNodeId (CommHelperError &err)
{
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == NULL) {
        return false;
    }

    const char *pszNodeId = _pDissSvc->getNodeId();
    if (pszNodeId == NULL) {
        return false;
    }

    _pCommHelper->sendLine (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }
    _pCommHelper->sendLine (err, pszNodeId);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DisseminationServiceProxyAdaptor::doGetPeerList (CommHelperError &err)
{
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == NULL) {
        return false;
    }

    char **ppszPeerList = NULL;

    _pCommHelper->sendLine (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }
    unsigned int uiPeerCounter = 0;
    ppszPeerList = _pDissSvc->getPeerList();
    if (ppszPeerList != NULL) {
        for (unsigned int i = 0; ppszPeerList[i] != NULL; i++) {
            if (strcmp (ppszPeerList[i], "") != 0) {
                uiPeerCounter++;
            }
        }
    }

    bool bSuccess = true;
    if (pWriter->write32 (&uiPeerCounter) < 0) {
        bSuccess = false;
    }
    if (bSuccess) {
        unsigned int uiPeerCounterWritten = 0;
        for (unsigned int i = 0; (uiPeerCounterWritten < uiPeerCounter) && (ppszPeerList[i] != NULL); i++) {
            unsigned int uiStrLen = (unsigned int) strlen (ppszPeerList[i]);
            if (uiStrLen > 0) {
                if ((pWriter->write32 (&uiStrLen) != 0) ||
                    (pWriter->writeBytes (ppszPeerList[i], uiStrLen) != 0)) {
                    bSuccess = false;
                    break;
                }
                uiPeerCounterWritten++;
            }
        }
    }

    if (ppszPeerList != NULL) {
        for (unsigned int i = 0; (ppszPeerList[i] != NULL); i++) {
            free (ppszPeerList[i]);
        }
        free (ppszPeerList);
    }

    return bSuccess;
}

bool DisseminationServiceProxyAdaptor::doGetDisServiceIds (CommHelperError &err)
{
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == NULL) {
        return false;
    }

    String objectId;
    String instanceId;
    err = _adptProtHelper.readIds (objectId, instanceId);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // Get IDs
    char **ppszIds = _pDissSvc->getDisseminationServiceIds ((objectId.length() > 0 ? objectId.c_str() : NULL),
                                                            (instanceId.length() > 0 ? instanceId.c_str() : NULL));

    bool bSuccess = true;
    _pCommHelper->sendLine (err, "OK");
    if (err != SimpleCommHelper2::None) {
        bSuccess = false;
    }

    // Send IDs
    if (bSuccess) {
        if (ppszIds != NULL) {
            for (unsigned int i = 0; ppszIds[i] != NULL; i++) {
                _pCommHelper->sendStringBlock (ppszIds[i], err);
                if (err != SimpleCommHelper2::None) {
                    bSuccess = false;
                    break;
                }
            }
        }
    }
    if (bSuccess) {
        uint32 ui32 = 0; // write 0 to terminate ID list
        if (pWriter->write32 (&ui32) != 0) {
            bSuccess = false;
        }
    }
    if (bSuccess) {
        _pCommHelper->sendLine (err, "OK");
        if (err != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (ppszIds != NULL) {
        for (unsigned int i = 0; ppszIds[i] != NULL; i++) {
            free (ppszIds[i]);
        }
        free (ppszIds);
    }

    return bSuccess;
}

/*
int push (const char *pszGroupName, const void *pData, uint32 ui32Length, int64 i64ExpirationTime,
          uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority, char *pszIdBuf, uint32 ui32IdBufLen);
*/
bool DisseminationServiceProxyAdaptor::doPushOrStore (CommHelperError &err, PublishOption pubOpt)
{
    String groupName;
    String objectId;
    String instanceId;
    String mimeType;
    err = _adptProtHelper.readGroupIdsMIMEType (groupName, objectId, instanceId, mimeType, (pubOpt != MAKE_AVAILABLE));
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32MetaDataLen = 0U;
    void *pMetaData = NULL;
    err = _adptProtHelper.readUI32Blob (pMetaData, ui32MetaDataLen);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32DataLen = 0U;
    void *pData = NULL;
    err = _adptProtHelper.readUI32Blob (pData, ui32DataLen);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    String dataMimeType;
    if (pubOpt == MAKE_AVAILABLE) {
        int rc = _pCommHelper->receiveBlock (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err);
        if (rc > 0) {
            _adptProtHelper._buf[rc] = '\0';
            dataMimeType = _adptProtHelper._buf;
        }
        else {
            if (pMetaData != NULL) {
                free (pMetaData);
            }
            if (pData != NULL) {
                free (pData);
            }
            return false;
        }
    }

    Reader *pReader = _pCommHelper->getReaderRef();

    // Read the ui64Expiration
    uint64 ui64Expiration;
    if (0 != pReader->read64 (&ui64Expiration)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    // Read the ui16HistoryWindow
    uint16 ui16HistoryWindow;
    if (0 != pReader->read16 (&ui16HistoryWindow)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    // Read the ui16Tag field
    uint16 ui16Tag;
    if (0 != pReader->read16 (&ui16Tag)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    // Read the ui8Priority field
    uint8 ui8Priority;
    if (0 != pReader->read8 (&ui8Priority)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    const uint32 ui32BufLen = 512;
    char buf[ui32BufLen];
    int rc = 0;
    switch (pubOpt) {
        case PUSH:
            rc = _pDissSvc->push (getClientID(), groupName, objectId, instanceId, mimeType,
                                  pMetaData, ui32MetaDataLen, pData, ui32DataLen, ui64Expiration,
                                  ui16HistoryWindow, ui16Tag, ui8Priority, buf, ui32BufLen);
            break;

        case STORE:
            rc = _pDissSvc->store (getClientID(), groupName, objectId, instanceId, mimeType,
                                   pMetaData, ui32MetaDataLen, pData, ui32DataLen, ui64Expiration,
                                   ui16HistoryWindow, ui16Tag, ui8Priority, buf, ui32BufLen);
            break;

        case MAKE_AVAILABLE:
            rc = _pDissSvc->makeAvailable (getClientID(), groupName, objectId, instanceId,
                                           pMetaData, ui32MetaDataLen, pData, ui32DataLen, dataMimeType,
                                           ui64Expiration, ui16HistoryWindow, ui16Tag, ui8Priority,
                                           buf, ui32BufLen);
            break;

		default:
            err = SimpleCommHelper2::ProtocolError;
			return false;
    }

    if (pMetaData != NULL) {
        free (pMetaData);
    }
    if (pData != NULL) {
        free (pData);
    }
    if (rc != 0) {
        _pCommHelper->sendLine (err, "ERROR");
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }
    else {
        _pCommHelper->sendLine (err, "OK");
        if (err != SimpleCommHelper2::None) {
            return false;
        }
        _pCommHelper->sendLine (err, buf);
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }

    return true;
} //doStore()

/*
int pushById (const char *pszMsgId);
*/
bool DisseminationServiceProxyAdaptor::doPushById (CommHelperError &err)
{
    String messageId;
    err = _adptProtHelper.readUI32String (messageId);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDissSvc->push (getClientID(), messageId);
    if (rc != 0) {
        _pCommHelper->sendLine (err, "ERROR");
    }
    else {
        _pCommHelper->sendLine (err, "OK");
    }

    if (err != SimpleCommHelper2::None) {
        return false;
    }
    return true;
} //doPushById()

bool DisseminationServiceProxyAdaptor::doMakeAvailableFile (CommHelperError &err)
{
    String groupName;
    String objectId;
    String instanceId;
    String mimeType;
    err = _adptProtHelper.readGroupIdsMIMEType (groupName, objectId, instanceId, mimeType);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32MetaDataLen = 0U;
    void *pMetaData = NULL;
    err = _adptProtHelper.readUI32Blob (pMetaData, ui32MetaDataLen);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    Reader *pReader = _pCommHelper->getReaderRef();

    // read the filename length.
    uint16 ui16FilenameLen = 0;
    if (0 != pReader->read16 (&ui16FilenameLen)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        return false;
    }
    String fileName;
    if (ui16FilenameLen > 0) {
        // read the data
        _pCommHelper->receiveBlob (_adptProtHelper._buf, ui16FilenameLen, err);
        if (err != SimpleCommHelper2::None) {
            if (pMetaData != NULL) {
                free (pMetaData);
            }
            return false;
        }
        _adptProtHelper._buf[ui16FilenameLen] = '\0';
        fileName = _adptProtHelper._buf;
    }

    // read data MIME type
    String dataMimeType;
    int rc = _pCommHelper->receiveBlock (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err);
    if (rc > 0) {
        _adptProtHelper._buf[rc] = '\0';
        dataMimeType = _adptProtHelper._buf;
    }
    else {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        return false;
    }

    // Read the ui64Expiration
    uint64 ui64Expiration;
    if (0 != pReader->read64 (&ui64Expiration)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        return false;
    }

    // Read the ui16HistoryWindow
    uint16 ui16HistoryWindow;
    if (0 != pReader->read16 (&ui16HistoryWindow)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        return false;
    }

    // Read the ui16Tag field
    uint16 ui16Tag;
    if (0 != pReader->read16 (&ui16Tag)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        return false;
    }

    // Read the ui8Priority field
    uint8 ui8Priority;
    if (0 != pReader->read8 (&ui8Priority)) {
        if (pMetaData != NULL) {
            free (pMetaData);
        }
        return false;
    }

    char buf[512];
    rc = _pDissSvc->makeAvailable (getClientID(), groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                   pMetaData, ui32MetaDataLen, fileName, dataMimeType, ui64Expiration,
                                   ui16HistoryWindow, ui16Tag, ui8Priority, buf, sizeof (buf));
    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
        if (err != SimpleCommHelper2::None) {
            return false;
        }
        _pCommHelper->sendLine (err, buf);
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }
    else {
        _pCommHelper->sendLine (err, "ERROR");
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }

    if (pMetaData != NULL) {
        free (pMetaData);
    }

    return true;
} //doMakeAvailableFile()

/*
int cancel (const char *pszId);
*/
bool DisseminationServiceProxyAdaptor::doCancel_psz (CommHelperError &err)
{
    _pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDissSvc->cancel (getClientID(), _adptProtHelper._buf);
    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doCancel_psz

/*
int cancel (uint16 ui16Tag);
*/
bool DisseminationServiceProxyAdaptor::doCancel_int (CommHelperError &err)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    uint16 ui16Tag;

    if (0 != pReader->read16 (&ui16Tag)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->cancel (getClientID(), ui16Tag);
    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doCancel_int

/*
int subscribe (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8Priority, bool bReliable, bool bSequenced);
*/
bool DisseminationServiceProxyAdaptor::doSubscribe (CommHelperError &err)
{
    String groupName;
    uint8 ui8Priority;
    bool bGroupReliable;
    bool bMsgReliable;
    bool bSequenced;

    err = _adptProtHelper.readSubscription (groupName, ui8Priority);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    err = _adptProtHelper.readSubscriptionParameters (bGroupReliable, bMsgReliable, bSequenced);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDissSvc->subscribe (getClientID(), groupName, ui8Priority,
                                   bGroupReliable, bMsgReliable, bSequenced);

    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doSubscribe()

bool DisseminationServiceProxyAdaptor::doSubscribe_tag (CommHelperError &err)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    String groupName;
    uint16 ui16Tag;
    uint8 ui8Priority;
    bool bGroupReliable;
    bool bMsgReliable;
    bool bSequenced;

    err = _adptProtHelper.readSubscription (groupName, ui8Priority);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // read the tag.
    if (0 != pReader->read16 (&ui16Tag)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    err = _adptProtHelper.readSubscriptionParameters (bGroupReliable, bMsgReliable, bSequenced);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDissSvc->subscribe (getClientID(), groupName, ui16Tag, ui8Priority,
                                   bGroupReliable, bMsgReliable, bSequenced);

    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doSubscribe_tag()

/*

int subscribe (uint16 ui16ClientId, const char *pszGroupName, uint8 ui8PredicateType, const char *predicate, uint8 ui8Priority, bool bReliable, bool bSequenced);
*/
bool DisseminationServiceProxyAdaptor::doSubscribe_predicate (CommHelperError &err)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    char buf[1024];
    String groupName;
    uint8 ui8Priority;
    uint8 ui8PredicateType;
    String predicate;
    bool bGroupReliable;
    bool bMsgReliable;
    bool bSequenced;

    err = _adptProtHelper.readSubscription (groupName, ui8PredicateType);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // read the predicate.
    if (_pCommHelper->receiveLine (buf, sizeof(buf), err) > 0) {
        predicate = buf;
    }
    else {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    // read the priority.
    if (0 != pReader->read8 (&ui8Priority)) {
        return false;
    }

    err = _adptProtHelper.readSubscriptionParameters (bGroupReliable, bMsgReliable, bSequenced);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDissSvc->subscribe (getClientID(), groupName, ui8PredicateType, predicate,
                                   ui8Priority, bGroupReliable, bMsgReliable, bSequenced);

    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doSubscribe_predicate()

/*
int unsubscribe (const char *pszGroupName);
*/
bool DisseminationServiceProxyAdaptor::doUnsubscribe (CommHelperError &err)
{
    // read the group name.
    String groupName;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        groupName = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    int rc = _pDissSvc->unsubscribe (getClientID(), groupName);
    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doUnsubscribe()

/*
int unsubscribe (const char *pszGroupName, uint16 ui16Tag);
*/
bool DisseminationServiceProxyAdaptor::doUnsubscribe_tag (CommHelperError &err)
{
    // read the group name.
    String groupName;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        groupName = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    //read the tag.
    Reader *pReader = _pCommHelper->getReaderRef();
    uint16 ui16Tag;
    if (pReader->read16 (&ui16Tag) < 0) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->unsubscribe (getClientID(), groupName, ui16Tag);
    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doUnsubscribe_tag

/*
int addFilter (const char *pszGroupName, uint16 ui16Tag);
*/
bool DisseminationServiceProxyAdaptor::doAddFilter (CommHelperError &err)
{
    // read the group name.
    String groupName;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        groupName = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    //read the tag.
    Reader *pReader = _pCommHelper->getReaderRef();
    uint16 ui16Tag;
    if (pReader->read16(&ui16Tag) < 0) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->addFilter (getClientID(), groupName, ui16Tag);
    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doAddFilter()

/*
int removeFilter (const char *pszGroupName, uint16 ui16Tag);
*/
bool DisseminationServiceProxyAdaptor::doRemoveFilter (CommHelperError &err)
{
    // read the group name.
    String groupName;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        groupName = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    //read the tag.
    Reader *pReader = _pCommHelper->getReaderRef();
    uint16 ui16Tag;
    if (pReader->read16(&ui16Tag) < 0) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->removeFilter (getClientID(), groupName, ui16Tag);
    if (rc == 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} //doRemoveFilter()


/*
int retrieve (const char *pszId, void *pBuf, uint32 ui32BufSize, int64 i64Timeout);
*/
bool DisseminationServiceProxyAdaptor::doRetrieve (CommHelperError &err)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    String msgID;
    uint32 ui33RetrievedDataLen;
    uint64 ui64Timeout;
    void *pRetrievedData = NULL;

    String msgId;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        msgId = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    // read the timeout
    if (pReader->read64 (&ui64Timeout)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->retrieve (msgID, &pRetrievedData, &ui33RetrievedDataLen, ui64Timeout);
    if (rc > 0) {
        if (_pCommHelper->getWriterRef()->write32 (&ui33RetrievedDataLen) < 0) {
            err = SimpleCommHelper2::ProtocolError;
            if (pRetrievedData != NULL) {
                free (pRetrievedData);
            }
            return false;
        }
        _pCommHelper->sendBlob (pRetrievedData, rc, err);
        if (err != SimpleCommHelper2::None) {
            if (pRetrievedData != NULL) {
                free (pRetrievedData);
            }
            return false;
        }
        _pCommHelper->sendLine (err, "OK");
    }
    else if (rc == 0) {
        assert (ui33RetrievedDataLen == 0);
        if (_pCommHelper->getWriterRef()->write32 (&ui33RetrievedDataLen) < 0) {
            err = SimpleCommHelper2::ProtocolError;
            if (pRetrievedData != NULL) {
                free (pRetrievedData);
            }
            return false;
        }
        _pCommHelper->sendLine (err, "OK");
    }

    if (pRetrievedData != NULL) {
        free (pRetrievedData);
    }
    return (err == SimpleCommHelper2::None);
} //doRetrieve

/*
int retrieve (const char *pszId, const char *pszFilePath, int64 i64Timeout);
*/
bool DisseminationServiceProxyAdaptor::doRetrieve_file (CommHelperError &err)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    String msgID;
    uint32 ui33RetrievedDataLen;
    uint64 ui64Timeout;
    void *pRetrievedData = NULL;

    // Read message Id
    String msgId;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        msgId = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    // Read file name
    String fileName;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        fileName = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    // read the timeout
    if (pReader->read64 (&ui64Timeout)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->retrieve (msgID, &pRetrievedData, &ui33RetrievedDataLen, ui64Timeout);
    if (rc >= 0) {
        _pCommHelper->sendLine (err, "OK");
    }

    if (pRetrievedData != NULL) {
        free (pRetrievedData);
    }
    return (err == SimpleCommHelper2::None);
} // doRetrieve_file

/*
int request (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint16 ui16HistoryLength, int64 i64RequestTimeout);
*/
bool DisseminationServiceProxyAdaptor::doHistoryRequest (CommHelperError &err)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    String groupName;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        groupName = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        return false;
    }

    // read the ui16Tag
    uint16 ui16Tag;
    if (pReader->read16 (&ui16Tag)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    // read the ui16HistoryLength
    uint16 ui16HistoryLength;
    if (pReader->read16 (&ui16HistoryLength)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    // read the timeout
    uint64 ui64Timeout;
    if (pReader->read64 (&ui64Timeout)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->historyRequest (getClientID(), groupName, ui16Tag,
                                        ui16HistoryLength, ui64Timeout);

    if (rc >= 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} // doRequest()

bool DisseminationServiceProxyAdaptor::doRequestMoreChunks (CommHelperError &err)
{
    String groupName;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        groupName = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    String sendeNodeId;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        sendeNodeId = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    // read the sequence ID
    uint32 ui32MsgId;
    if (_pCommHelper->getReaderRef()->read32 (&ui32MsgId)) {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->requestMoreChunks (getClientID(), groupName, sendeNodeId, ui32MsgId);
    if (rc >= 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
} // doRequestMoreChunks()

bool DisseminationServiceProxyAdaptor::doRequestMoreChunksByID (CommHelperError &err)
{
    String msgId;
    if (_pCommHelper->receiveLine (_adptProtHelper._buf, _adptProtHelper.BUF_LEN, err) > 0) {
        msgId = _adptProtHelper._buf;
    }
    else if (err == SimpleCommHelper2::None) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }
    else {
        err = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDissSvc->requestMoreChunks (getClientID(), msgId);
    if (rc >= 0) {
        _pCommHelper->sendLine (err, "OK");
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
    }

    return (err == SimpleCommHelper2::None);
}

bool DisseminationServiceProxyAdaptor::doRegisterDataArrivedCallback (CommHelperError &err)
{
    if (_bListenerRegistered) {
        return true;
    }
    _pDissSvc->registerDisseminationServiceListener (getClientID(), this);
    _bListenerRegistered = true;

    return true;
} // doRegisterDataArrivedCallback()

bool DisseminationServiceProxyAdaptor::doRegisterChunkArrivedCallback (CommHelperError &err)
{
    if (_bListenerRegistered) {
        return true;
    }
    _pDissSvc->registerDisseminationServiceListener (getClientID(), this);
    _bListenerRegistered = true;

    return true;
} // doRegisterChunkArrivedCallback()

bool DisseminationServiceProxyAdaptor::doRegisterMetadataArrivedCallback (CommHelperError &err)
{
    if (_bListenerRegistered) {
        return true;
    }
    _pDissSvc->registerDisseminationServiceListener (getClientID(), this);
    _bListenerRegistered = true;

    return true;
} //doRegisterMetadataArrivedCallback()

bool DisseminationServiceProxyAdaptor::doRegisterDataAvailableCallback (CommHelperError &err)
{
    if (_bListenerRegistered) {
        return true;
    }
    _pDissSvc->registerDisseminationServiceListener (getClientID(), this);
    _bListenerRegistered = true;

    return true;
} //doRegisterDataAvailableCallback()

bool DisseminationServiceProxyAdaptor::doRegisterPeerStatusCallback (CommHelperError &err)
{
    if (_bPeerStatusListenerRegistered) {
        return true;
    }
    _pDissSvc->registerPeerStatusListener (getClientID(), this);
    _bPeerStatusListenerRegistered = true;

    return true;
} //doRegisterPeerStatusCallback()

bool DisseminationServiceProxyAdaptor::doRegisterSearchListener (CommHelperError &err)
{
    if (_bSearchListenerRegistered) {
        return true;
    }
    _uiSearchListenerIndex = getClientID();
    _pDissSvc->registerSearchListener (this, _uiSearchListenerIndex);
    _bSearchListenerRegistered = true;

    return true;
} //doRegisterSearchListener()

bool DisseminationServiceProxyAdaptor::doResetTransmissionHistory (CommHelperError &err)
{

    int rc = _pDissSvc->resetTransmissionHistory();
    if (_pDissSvc->resetTransmissionHistory() == 0) {
        _pCommHelper->sendLine (err, "OK");
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }

    return (err == SimpleCommHelper2::None);
}

bool DisseminationServiceProxyAdaptor::doGetNextPushId (CommHelperError &err)
{
    // Read the group name
    char buf[1024];
    String groupName;
    if (_pCommHelper->receiveLine (buf, sizeof(buf), err) > 0) {
        groupName = buf;
    }
    else {
        return false;
    }

    int rc = _pDissSvc->getNextPushId (groupName, buf, sizeof (buf));
    if (rc >= 0) {
        _pCommHelper->sendLine (err, "OK");
        if (err != SimpleCommHelper2::None) {
            return false;
        }
        _pCommHelper->sendLine (err, buf);
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }
    else {
        _pCommHelper->sendLine (err, "ERROR %d", rc);
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }

    return true;
}

bool DisseminationServiceProxyAdaptor::doSearch (CommHelperError &err)
{
    // read the group name
    String groupName;
    err = _adptProtHelper.readUI32String (groupName);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // read the query type
    String queryType;
    err = _adptProtHelper.readUI32String (queryType);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // read the query qualifiers
    String queryQualifiers;
    err = _adptProtHelper.readUI32String (queryQualifiers);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // read the query
    uint32 uiQueryLen = 0;
    void *pQuery = NULL;
    err = _adptProtHelper.readUI32Blob (pQuery, uiQueryLen);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    char *pszQueryId = NULL;
    int rc = _pDissSvc->search (getClientID(), groupName.c_str(), queryType.c_str(),
                                queryQualifiers.c_str(), pQuery, uiQueryLen, &pszQueryId);

    if (rc != 0 || pszQueryId == NULL) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }

    _pCommHelper->sendLine (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // write the message id of the search
    _pCommHelper->sendStringBlock (pszQueryId, err);

    return (err == SimpleCommHelper2::None);
}

bool DisseminationServiceProxyAdaptor::doSearchReply (CommHelperError &err)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    // read the queryId name
    String queryId;
    err = _adptProtHelper.readUI32String (queryId);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32NumberOfElements = 0;
    if (pReader->read32 (&ui32NumberOfElements) < 0) {
        err = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32NumberOfElements == 0) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }

    char **ppszMsgIds = (char **) calloc (ui32NumberOfElements+1, sizeof (char *));
    if (ppszMsgIds == NULL) {
        err = SimpleCommHelper2::ProtocolError;
        return false;
    }

    bool bSucceded = true;
    for (uint32 i = 0; i < ui32NumberOfElements; i++) {
        String msgId;
        err = _adptProtHelper.readUI32String (msgId);
        if (err != SimpleCommHelper2::None) {
            bSucceded = false;
            break;
        }
        ppszMsgIds[i] = msgId.r_str();
    }
    ppszMsgIds[ui32NumberOfElements] = NULL;

    if (bSucceded) {
        int rc = _pDissSvc->searchReply (queryId, (const char **)ppszMsgIds);
        bSucceded = (rc == 0);
        if (bSucceded) {
            _pCommHelper->sendLine (err, "OK");
            if (err != SimpleCommHelper2::None) {
                bSucceded = false;
            }
        }
    }

    // Deallocate msg Ids
    if (ppszMsgIds != NULL) {
        for (uint32 i = 0; ppszMsgIds[i] != NULL ; i++) {
            free (ppszMsgIds[i]);
        }
        free (ppszMsgIds);
    }

    return bSucceded;
}

CommHelperError DisseminationServiceProxyAdaptor::messageArrivedCallback (const char *pszCallbackId, const char *pszOriginator,
                                                                          const char *pszGroupName, uint32 ui32SeqId,
                                                                          const char *pszObjectId, const char *pszInstanceId,
                                                                          const char *pszMimeType)
{
    if (pszCallbackId == NULL) {
        return SimpleCommHelper2::ProtocolError;
    }
    CommHelperError err = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (err, pszCallbackId);
    if (err != SimpleCommHelper2::None) {
        return err;
    }

    _pCallbackCommHelper->sendLine (err, pszOriginator);
    if (err != SimpleCommHelper2::None) {
        return err;
    }

    _pCallbackCommHelper->sendLine (err, pszGroupName);
    if (err != SimpleCommHelper2::None) {
        return err;
    }

    if (_pCallbackCommHelper->getWriterRef()->write32 (&ui32SeqId) < 0) {
        return SimpleCommHelper2::CommError;
    }

    _pCallbackCommHelper->sendStringBlock (pszObjectId, err);
    if (err != SimpleCommHelper2::None) {
        return err;
    }

    _pCallbackCommHelper->sendStringBlock (pszInstanceId, err);
    if (err != SimpleCommHelper2::None) {
        return err;
    }

    _pCallbackCommHelper->sendStringBlock (pszMimeType, err);
    if (err != SimpleCommHelper2::None) {
        return err;
    }

    return SimpleCommHelper2::None;
}

bool DisseminationServiceProxyAdaptor::dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                    uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                    const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                                    const void *pData, uint32 ui32Length, uint32 ui32MetadataLength,
                                                    uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    CommHelperError err = messageArrivedCallback ("dataArrivedCallback", pszSender, pszGroupName,
                                                  ui32SeqId, pszObjectId, pszInstanceId, pszMimeType);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    //send the data size.
    if (0 != pWriter->write32 (&ui32Length)) {
        return false;
    }

    //send the metadata length
    if (0 != pWriter->write32 (&ui32MetadataLength)) {
        return false;
    }

    //send the data.
    _pCallbackCommHelper->sendBlob (pData, ui32Length, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    //send the ui16Tag
    if (0 != pWriter->write16 (&ui16Tag)) {
        return false;
    }

    //send the ui8Priority
    if (0 != pWriter->write8 (&ui8Priority)) {
        return false;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryId, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->receiveMatch (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    return true;
} //dataArrivedCallback()

bool DisseminationServiceProxyAdaptor::chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                     uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                     const char *pszMimeType, const void *pChunk, uint32 ui32Length, uint8 ui8NChunks,
                                                     uint8 ui8TotNChunks, const char *pszChunkedMsg, uint16 ui16Tag, uint8 ui8Priority,
                                                     const char *pszQueryId)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    CommHelperError err = messageArrivedCallback ("chunkArrivedCallback", pszSender, pszGroupName,
                                                  ui32SeqId, pszObjectId, pszInstanceId, pszMimeType);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    //send the data size.
    if (0 != pWriter->write32(&ui32Length)) {
        return false;
    }

    //send the data.
    _pCallbackCommHelper->sendBlob (pChunk, ui32Length, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    //send the ui8NChunks
    if (0 != pWriter->write8 (&ui8NChunks)) {
        return false;
    }

    //send the ui8TotNChunks
    if (0 != pWriter->write8 (&ui8TotNChunks)) {
        return false;
    }

    _pCallbackCommHelper->sendLine (err, pszChunkedMsg);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    //send the ui16Tag
    if (0 != pWriter->write16 (&ui16Tag)) {
        return false;
    }

    //send the ui8Priority
    if (0 != pWriter->write8 (&ui8Priority)) {
        return false;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryId, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->receiveMatch (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    return true;
} //chunkArrivedCallback()

bool DisseminationServiceProxyAdaptor::metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                        uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                        const char *pszMimeType, const void *pData, uint32 ui32Length,
                                                        bool bDataChunked, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    CommHelperError err = messageArrivedCallback ("metadataArrivedCallback", pszSender, pszGroupName,
                                                  ui32SeqId, pszObjectId, pszInstanceId, pszMimeType);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // Send the data size.
    if (0 != pWriter->write32 (&ui32Length)) {
        return false;
    }

    // Send the data.
    _pCallbackCommHelper->sendBlob (pData, ui32Length, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    // Send the bDataChunked
    uint8 ui8 = bDataChunked ? 1 : 0;
    if (0 != pWriter->write8 (&ui8)) {
        return false;
    }

    // Send the ui16Tag
    if (0 != pWriter->write16 (&ui16Tag)) {
        return false;
    }

    // Send the ui8Priority
    if (0 != pWriter->write8 (&ui8Priority)) {
        return false;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryId, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->receiveMatch (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    return true;
} // metadataArrivedCallback()

bool DisseminationServiceProxyAdaptor::dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                      uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                      const char *pszMimeType, const char *pszId, const void *pData,
                                                      uint32 ui32Length, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    CommHelperError err = messageArrivedCallback ("dataAvailableCallback", pszSender, pszGroupName,
                                                  ui32SeqId, pszObjectId, pszInstanceId, pszMimeType);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32Len = (pszId == NULL ? 0 : (uint32) strlen (pszId));
    _pCallbackCommHelper->getWriterRef()->write32 (&ui32Len);
    if (ui32Len > 0) {
        _pCallbackCommHelper->sendBlob (pszId, ui32Len, err);
        if (err != SimpleCommHelper2::None) {
            return false;
        }
    }

    //send the data size.
    if (0 != pWriter->write32 (&ui32Length)) {
        return false;
    }

    //send the data.
    _pCallbackCommHelper->sendBlob (pData, ui32Length, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    //send the ui16Tag
    if (0 != pWriter->write16 (&ui16Tag)) {
        return false;
    }

    //send the ui8Priority
    if (0 != pWriter->write8 (&ui8Priority)) {
        return false;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryId, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->receiveMatch (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    return true;
} //dataAvailable()

bool DisseminationServiceProxyAdaptor::newPeer (const char * pszPeerId)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    CommHelperError err = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (err, "newPeerCallback");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32Len = (uint32) strlen (pszPeerId);
    if (pWriter->write32 (&ui32Len) < 0) {
        return false;
    }
    _pCallbackCommHelper->sendBlob (pszPeerId, ui32Len, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->receiveMatch (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DisseminationServiceProxyAdaptor::deadPeer (const char * pszPeerId)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    CommHelperError err = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (err, "deadPeerCallback");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32Len = (uint32) strlen (pszPeerId);
    if (0 != pWriter->write32 (&ui32Len)) {
        return false;
    }

    _pCallbackCommHelper->sendBlob (pszPeerId, ui32Len, err);
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->receiveMatch (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

void DisseminationServiceProxyAdaptor::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                                      const char *pszQuerier, const char *pszQueryType,
                                                      const char *pszQueryQualifiers,
                                                      const void *pszQuery, unsigned int uiQueryLen)
{
    if (_pCallbackCommHelper == NULL) {
        return;
    }

    CommHelperError err = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (err, "searchArrivedCallback");
    if (err != SimpleCommHelper2::None) {
        return;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryId, err);
    if (err != SimpleCommHelper2::None) {
        return;
    }

    _pCallbackCommHelper->sendStringBlock (pszGroupName, err);
    if (err != SimpleCommHelper2::None) {
        return;
    }

    _pCallbackCommHelper->sendStringBlock (pszQuerier, err);
    if (err != SimpleCommHelper2::None) {
        return;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryType, err);
    if (err != SimpleCommHelper2::None) {
        return;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryQualifiers, err);
    if (err != SimpleCommHelper2::None) {
        return;
    }

    if (_pCallbackCommHelper->getWriterRef()->write32 (&uiQueryLen) < 0) {
        return;
    }

    if (uiQueryLen > 0) {
        _pCallbackCommHelper->sendBlob (pszQuery, uiQueryLen, err);
        if (err != SimpleCommHelper2::None) {
            return;
        }
    }

    _pCallbackCommHelper->receiveMatch (err, "OK");
    if (err != SimpleCommHelper2::None) {
        return;
    }
}

void DisseminationServiceProxyAdaptor::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds,
                                                           const char *pszMatchingNodeId)
{
    // TODO: decide whether the application should be notified as well of this event, implement this method in case
}

void DisseminationServiceProxyAdaptor::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId)
{
    // TODO: decide whether the application should be notified as well of this event, implement this method in case
}

//------------------------------------------------------------------------------
// DisseminationServiceProxyProtocolHelper
//------------------------------------------------------------------------------

DisseminationServiceProxyProtocolHelper::DisseminationServiceProxyProtocolHelper (void)
{
}

DisseminationServiceProxyProtocolHelper::~DisseminationServiceProxyProtocolHelper()
{
}

void DisseminationServiceProxyProtocolHelper::setCommHelper (NOMADSUtil::SimpleCommHelper2 *pCommHelper)
{
    _pCommHelper = pCommHelper;
}

void DisseminationServiceProxyProtocolHelper::setCbackCommHelper (NOMADSUtil::SimpleCommHelper2 *pCommHelper)
{
    _pCallbackCommHelper = pCommHelper;
}

CommHelperError DisseminationServiceProxyProtocolHelper::readUI32Blob (void *&pBuf, uint32 &uiBufLen)
{
    if (_pCommHelper == NULL) {
        return SimpleCommHelper2::CommError;
    }

    // read the data length
    uint32 ui32DataLen = 0;
    if (_pCommHelper->getReaderRef()->read32 (&ui32DataLen) != 0) {
        return SimpleCommHelper2::CommError;
    }

    // read the data
    if (ui32DataLen > 0) {
        pBuf = malloc (ui32DataLen);
        if (pBuf == NULL) {
            checkAndLogMsg ("DisseminationServiceProxyProtocolHelper::readUI32Blob", memoryExhausted);
        }
        CommHelperError error = SimpleCommHelper2::None;
        _pCommHelper->receiveBlob (pBuf, ui32DataLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pBuf);
            return error;
        }
    }

    uiBufLen = ui32DataLen;
    return SimpleCommHelper2::None;
}

CommHelperError DisseminationServiceProxyProtocolHelper::readUI32String (String &str)
{
    if (_pCommHelper == NULL) {
        return SimpleCommHelper2::CommError;
    }

    uint32 ui32 = 0U;
    CommHelperError error = SimpleCommHelper2::None;
    if (_pCommHelper->getReaderRef()->read32 (&ui32) != 0) {
        return SimpleCommHelper2::CommError;
    }
    if (ui32 > 0) {
        if (ui32 > (BUF_LEN-1)) {
            return SimpleCommHelper2::ProtocolError;
        }
        _pCommHelper->receiveBlob (_buf, ui32, error);
        if (error != SimpleCommHelper2::None) {
            return error;
        }
    }
    _buf[ui32] = '\0';
    str = _buf;
    return error;
}

CommHelperError DisseminationServiceProxyProtocolHelper::readIds (String &objectId, String &instanceId)
{
    CommHelperError error = readUI32String (objectId);
    if (error != SimpleCommHelper2::None) {
        return error;
    }

    error = readUI32String (instanceId);
    if (error != SimpleCommHelper2::None) {
        return error;
    }

    return SimpleCommHelper2::None;
}

CommHelperError DisseminationServiceProxyProtocolHelper::readGroupIdsMIMEType (String &groupName, String &objectId,
                                                                               String &instanceId, String &mimeType,
                                                                               bool bReadMIMEType)
{
    CommHelperError error = SimpleCommHelper2::None;
    if (_pCommHelper->receiveLine (_buf, BUF_LEN, error) > 0) {
        groupName = _buf;
    }
    else if (error == SimpleCommHelper2::None) {
        return SimpleCommHelper2::ProtocolError;
    }
    else {
        return error;
    }

    error = readIds (objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return error;
    }

    if (bReadMIMEType) {
        error = readUI32String (mimeType);
        if (error != SimpleCommHelper2::None) {
            return error;
        }
    }

    return SimpleCommHelper2::None;
}

CommHelperError DisseminationServiceProxyProtocolHelper::readSubscription (String &groupName, uint8 &ui8Priority)
{
    if (_pCommHelper == NULL) {
        return SimpleCommHelper2::CommError;
    }
    CommHelperError error = SimpleCommHelper2::None;

    // read the group name.
    if (_pCommHelper->receiveLine (_buf, BUF_LEN, error) > 0) {
        groupName = _buf;
    }
    else if (error == SimpleCommHelper2::None) {
        return SimpleCommHelper2::ProtocolError;
    }
    else {
        return error;
    }

    // read the priority.
    if (0 != _pCommHelper->getReaderRef()->read8 (&ui8Priority)) {
        return SimpleCommHelper2::CommError;
    }

    return error;
}

CommHelperError DisseminationServiceProxyProtocolHelper::readSubscriptionParameters (bool &bGroupReliable, bool &bMsgReliable, bool &bSequenced)
{
    if (_pCommHelper == NULL) {
        return SimpleCommHelper2::CommError;
    }

    // read reliable flag
    if (0 != _pCommHelper->getReaderRef()->readBool (&bGroupReliable)) {
        return SimpleCommHelper2::CommError;
    }

    // read reliable flag
    if (0 != _pCommHelper->getReaderRef()->readBool (&bMsgReliable)) {
        return SimpleCommHelper2::CommError;
    }

    // read sequenced flag
    if (0 != _pCommHelper->getReaderRef()->readBool (&bSequenced)) {
        return SimpleCommHelper2::CommError;
    }

    return SimpleCommHelper2::None;
}

CommHelperError DisseminationServiceProxyProtocolHelper::writeUI32Blob (const void *pBuf, uint32 uiBufLen)
{

    //send the metadata length
    if (0 != _pCallbackCommHelper->getWriterRef()->write32 (&uiBufLen)) {
        return SimpleCommHelper2::CommError;
    }

    //send the data.
    CommHelperError err = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendBlob (pBuf, uiBufLen, err);

    return err;
}

