/*
 * DisseminationServiceProxyCallbackHandler.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "DisseminationServiceProxyCallbackHandler.h"

#include "DisServiceDefs.h"

#include "Logger.h"

#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DisseminationServiceProxyCallbackHandler::DisseminationServiceProxyCallbackHandler (DisseminationServiceProxy *pProxy, CommHelper2 *pCommHelper)
{
    _pCommHelper = pCommHelper;
    _pProxy = pProxy;
}

DisseminationServiceProxyCallbackHandler::~DisseminationServiceProxyCallbackHandler (void)
{
    if (isRunning()) {
        requestTerminationAndWait();
    }
    if (_pCommHelper != NULL) {
        delete _pCommHelper;
        _pCommHelper = NULL;
    }
    _pProxy = NULL;
}

void DisseminationServiceProxyCallbackHandler::run()
{
    started();
    char tmpbuf[80];
    snprintf (tmpbuf, sizeof (tmpbuf)-1, "DSProxyCallback %d %p", _pProxy->_ui16ApplicationId, _pProxy);
    Thread::setName (tmpbuf);
    const char * const pszMethodName = "DisseminationServiceProxyCallbackHandler::run";
    while (!terminationRequested()) {
        try {
            const char **apszTokens = _pCommHelper->receiveParsed();
            if (terminationRequested()) {
                break;
            }
            else if (0 == strcmp (apszTokens[0], "dataArrivedCallback")) {
                doDataArrivedCallback();
            }
            else if (0 == strcmp (apszTokens[0], "chunkArrivedCallback")) {
                doChunkArrivedCallback();
            }
            else if (0 == strcmp (apszTokens[0], "metadataArrivedCallback")) {
                doMetadataArrivedCallback();
            }
            else if (0 == strcmp (apszTokens[0], "dataAvailableCallback")) {
                doDataAvailableCallback();
            }
            else if (0 == strcmp (apszTokens[0], "newPeerCallback")) {
                doNewPeerCallback();
            }
            else if (0 == strcmp (apszTokens[0], "deadPeerCallback")) {
                doDeadPeerCallback();
            }
            else if (0 == strcmp (apszTokens[0], "searchArrivedCallback")) {
                doSearchArrivedCallback();
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                  "failed operation unknown\n");
            }
        }
        catch (ProtocolException pe) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                              "failed with protocol exception %s\n", pe.getMsg());
        }
        catch (CommException ce) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                              "failed with comm exception %s\n", ce.getMsg());

            if (_pProxy->startReconnect()) {
                break;
            }
            else {
                sleepForMilliseconds(1000);
            }

        }
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info,
                    "id %d terminating\n", _pProxy->_ui16ApplicationId);
    terminating();
}

int DisseminationServiceProxyCallbackHandler::doDataArrivedCallback (void)
{
    char szSenderBuf [1024];
    char szGroupNameBuf [1024];

    memset (szSenderBuf, 0, sizeof (szSenderBuf));
    memset (szGroupNameBuf, 0, sizeof (szGroupNameBuf));
    _pCommHelper->receiveLine (szSenderBuf, sizeof (szSenderBuf)-1);   
    _pCommHelper->receiveLine (szGroupNameBuf, sizeof (szGroupNameBuf)-1);

    void *pDataBuf;
    uint32 ui32SeqNum;
    String objectId;
    String instanceId;
    String mimeType;
    uint32 ui32 = 0;
    uint32 ui32DataLength;
    uint32 ui32MetadataLength;
    uint16 ui16Tag;
    uint8 ui8Priority;
    String queryId;

    if (_pCommHelper->getReaderRef()->read32 (&ui32SeqNum) != 0) {
        return false;
    }

    char buf[128];
    // Read objectId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        objectId = buf;
    }
    else {
        return false;
    }

    // Read instanceId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        instanceId = buf;
    }
    else {
        return false;
    }

    // Read MIME type
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        mimeType = buf;
    }
    else {
        return false;
    }

    _pCommHelper->getReaderRef()->read32 (&ui32DataLength);
    _pCommHelper->getReaderRef()->read32 (&ui32MetadataLength);

    if (ui32DataLength > 0) {
        pDataBuf = malloc (ui32DataLength);
        if (pDataBuf == NULL) {
            return false;
        }
        _pCommHelper->receiveBlob (pDataBuf, ui32DataLength);
    }
    else {
        pDataBuf = NULL;
    }

    _pCommHelper->getReaderRef()->read16 (&ui16Tag);
    _pCommHelper->getReaderRef()->read8 (&ui8Priority);

    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                if (pDataBuf != NULL) {
                    free (pDataBuf);
                }
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
            buf[ui32] = '\0';
            queryId = buf;
        }
    }
    else {
        if (pDataBuf != NULL) {
            free (pDataBuf);
        }
        return false;
    }

    _pProxy->dataArrived (szSenderBuf, szGroupNameBuf, ui32SeqNum, objectId.c_str(), instanceId.c_str(),
                          mimeType.c_str(), pDataBuf, ui32DataLength, ui32MetadataLength, ui16Tag, ui8Priority,
                          queryId.c_str());

    _pCommHelper->sendLine ("OK");

    free (pDataBuf);

    return 0;
}

int DisseminationServiceProxyCallbackHandler::doChunkArrivedCallback (void)
{
    char szSenderBuf [1024];
    char szGroupNameBuf [1024];

    memset (szSenderBuf, 0, sizeof (szSenderBuf));
    memset (szGroupNameBuf, 0, sizeof (szGroupNameBuf));
    _pCommHelper->receiveLine (szSenderBuf, sizeof (szSenderBuf)-1);   
    _pCommHelper->receiveLine (szGroupNameBuf, sizeof (szGroupNameBuf)-1);

    void *pChunkBuf;
    uint32 ui32SeqNum;
    String objectId;
    String  instanceId;
    String mimeType;
    uint32 ui32 = 0;
    uint32 ui32ChunkLength;
    uint8 ui8NChunks;
    uint8 ui8TotNChunks;
    uint16 ui16Tag;
    uint8 ui8Priority;
    String queryId;

    if (_pCommHelper->getReaderRef()->read32 (&ui32SeqNum) != 0) {
        return false;
    }

    char buf[128];
    // Read objectId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        objectId = buf;
    }
    else {
        return false;
    }

    // Read instanceId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        instanceId = buf;
    }
    else {
        return false;
    }

    // Read MIME type
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        mimeType = buf;
    }
    else {
        return false;
    }

    if (_pCommHelper->getReaderRef()->read32 (&ui32ChunkLength) != 0) {
        return false;
    }

    if (ui32ChunkLength > 0) {
        pChunkBuf = malloc (ui32ChunkLength);
        if (pChunkBuf == NULL) {
            return false;
        }
        _pCommHelper->receiveBlob (pChunkBuf, ui32ChunkLength);
    }
    else {
        pChunkBuf = NULL;
    }

    if ((_pCommHelper->getReaderRef()->read8 (&ui8NChunks) != 0) ||
        (_pCommHelper->getReaderRef()->read8 (&ui8TotNChunks) != 0)) {
        if (pChunkBuf != NULL) {
            free (pChunkBuf);
        }
        return false;
    }
    String chunkId = _pCommHelper->receiveLine();
    if ((_pCommHelper->getReaderRef()->read16 (&ui16Tag) != 0) ||
         (_pCommHelper->getReaderRef()->read8 (&ui8Priority) != 0)) {
        if (pChunkBuf != NULL) {
            free (pChunkBuf);
        }
        return false;
    }

    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                if (pChunkBuf != NULL) {
                    free (pChunkBuf);
                }
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        queryId = buf;
    }
    else {
        if (pChunkBuf != NULL) {
            free (pChunkBuf);
        }
        return false;
    }

    _pProxy->chunkArrived (szSenderBuf, szGroupNameBuf, ui32SeqNum, objectId.c_str(),
                           instanceId.c_str(), mimeType.c_str(), pChunkBuf, ui32ChunkLength,
                           ui8NChunks, ui8TotNChunks, chunkId, ui16Tag, ui8Priority,
                           queryId.c_str());

    _pCommHelper->sendLine ("OK");

    if (pChunkBuf != NULL) {
        free (pChunkBuf);
    }

    return 0;
}

int DisseminationServiceProxyCallbackHandler::doMetadataArrivedCallback (void)
{
    char szSenderBuf [1024];
    char szGroupNameBuf [1024];

    memset (szSenderBuf, 0, sizeof (szSenderBuf));
    memset (szGroupNameBuf, 0, sizeof (szGroupNameBuf));
    _pCommHelper->receiveLine (szSenderBuf, sizeof (szSenderBuf)-1);   
    _pCommHelper->receiveLine (szGroupNameBuf, sizeof (szGroupNameBuf)-1);

    void *pDataBuf;
    uint32 ui32SeqNum;
    String objectId;
    String instanceId;
    String mimeType;
    uint32 ui32 = 0;
    uint32 ui32DataLength;
    uint16 ui16Tag;
    uint8 ui8Priority;
    String queryId;

    if (_pCommHelper->getReaderRef()->read32 (&ui32SeqNum) != 0) {
        return false;
    }

    char buf[128];
    // Read objectId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        objectId = buf;
    }
    else {
        return false;
    }

    // Read instanceId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        instanceId = buf;
    }
    else {
        return false;
    }

    // Read MIME type
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        mimeType = buf;
    }
    else {
        return false;
    }

    if (_pCommHelper->getReaderRef()->read32 (&ui32DataLength) != 0) {
        return false;
    }

    if (ui32DataLength > 0) {
        pDataBuf = malloc (ui32DataLength);
        if (pDataBuf == NULL) {
            return false;
        }
        _pCommHelper->receiveBlob (pDataBuf, ui32DataLength);
    }
    else {
        pDataBuf = NULL;
    }

    uint8 ui8;
    if (_pCommHelper->getReaderRef()->read8 (&ui8) != 0) {
        if (pDataBuf != NULL) { 
            free (pDataBuf);
        }
        return false;
    }
    bool bDataChunked = (ui8 == 1 ? true : false);
    if ((_pCommHelper->getReaderRef()->read16 (&ui16Tag) != 0) ||
        (_pCommHelper->getReaderRef()->read8 (&ui8Priority) != 0)) {
        if (pDataBuf != NULL) { 
            free (pDataBuf);
        }
        return false;
    }

    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                if (pDataBuf != NULL) { 
                    free (pDataBuf);
                }
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        queryId = buf;
    }
    else {
        if (pDataBuf != NULL) { 
            free (pDataBuf);
        }
        return false;
    }

    _pProxy->metadataArrived (szSenderBuf, szGroupNameBuf, ui32SeqNum, objectId.c_str(),
                              instanceId.c_str(), mimeType.c_str(), pDataBuf, ui32DataLength,
                              bDataChunked, ui16Tag, ui8Priority, queryId.c_str());

    _pCommHelper->sendLine ("OK");

    if (pDataBuf != NULL) { 
        free (pDataBuf);
    }

    return 0;
}

int DisseminationServiceProxyCallbackHandler::doDataAvailableCallback (void)
{
    char szSenderBuf [1024];
    char szGroupNameBuf [1024];

    memset (szSenderBuf, 0, sizeof (szSenderBuf));
    memset (szGroupNameBuf, 0, sizeof (szGroupNameBuf));
    _pCommHelper->receiveLine (szSenderBuf, sizeof (szSenderBuf)-1);   
    _pCommHelper->receiveLine (szGroupNameBuf, sizeof (szGroupNameBuf)-1);

    void *pDataBuf;
    uint32 ui32SeqNum;
    String objectId;
    String  instanceId;
    String mimeType;
    uint32 ui32 = 0;
    uint32 ui32DataLength;
    uint16 ui16Tag;
    uint8 ui8Priority;
    String queryId;

    if (_pCommHelper->getReaderRef()->read32 (&ui32SeqNum) != 0) {
        return false;
    }

    char buf[128];
    // Read objectId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        objectId = buf;
    }
    else {
        return false;
    }

    // Read instanceId
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        instanceId = buf;
    }
    else {
        return 0;
    }

    // Read MIME type
    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        mimeType = buf;
    }
    else {
        return 0;
    }

    char *pszId = NULL;
    if (_pCommHelper->getReaderRef()->read32 (&ui32) < 0) {
        if (ui32 > 0) {
            pszId = (char*) malloc (ui32+1);
            if (pszId == NULL) {
                return false;
            }
            _pCommHelper->getReaderRef()->readBytes (pszId, ui32);
            pszId[ui32] = '\0';
        }
    }
    else {
        return 0;
    }

    if (_pCommHelper->getReaderRef()->read32 (&ui32DataLength) == 0) {
        if (ui32DataLength > 0) {
            pDataBuf = malloc (ui32DataLength);
            if (pDataBuf == NULL) {
                if (pszId != NULL) {
                    free (pszId);
                }
                return false;
            }
            _pCommHelper->receiveBlob (pDataBuf, ui32DataLength);
        }
        else {
            pDataBuf = NULL;
        }
    }

    if ((_pCommHelper->getReaderRef()->read16 (&ui16Tag) != 0) ||
        (_pCommHelper->getReaderRef()->read8 (&ui8Priority) != 0)) {
        if (pszId != NULL) {
            free (pszId);
        }
        if (pDataBuf != NULL) {
            free (pDataBuf);
        }
        return false;
    }

    if (_pCommHelper->getReaderRef()->read32 (&ui32) == 0) {
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
        }
        buf[ui32] = '\0';
        queryId = buf;
    }
    else {
        if (pszId != NULL) {
            free (pszId);
        }
        if (pDataBuf != NULL) {
            free (pDataBuf);
        }
        return false;
    }

    _pProxy->dataAvailable (szSenderBuf, szGroupNameBuf, ui32SeqNum, objectId.c_str(), instanceId.c_str(),
                            mimeType.c_str(), pszId, pDataBuf, ui32DataLength, ui16Tag, ui8Priority,
                            queryId.c_str());

    _pCommHelper->sendLine ("OK");

    if (pszId != NULL) {
        free (pszId);
    }
    if (pDataBuf != NULL) {
        free (pDataBuf);
    }

    return 0;
}

int DisseminationServiceProxyCallbackHandler::doSearchArrivedCallback (void)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    char buf[128];
    String queryId;
    String groupName;
    String querier;
    String queryType;
    String queryQualifiers;
    unsigned int uiQueryLen;
    void *pQuery;
    uint32 ui32;

    try {
        // read the query ID
        if (pReader->read32 (&ui32) < 0) {
            return false;
        }
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
            buf[ui32] = '\0';
            queryId = buf;
        }

        // read the group name
        if (pReader->read32 (&ui32) < 0) {
            return false;
        }
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
            buf[ui32] = '\0';
            groupName = buf;
        }

        // read the group name
        if (pReader->read32 (&ui32) < 0) {
            return false;
        }
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
            buf[ui32] = '\0';
            querier = buf;
        }

        // read the query type
        if (pReader->read32 (&ui32) < 0) {
            return false;
        }
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
            buf[ui32] = '\0';
            queryType = buf;
        }

        // read the query qualifiers
        if (pReader->read32 (&ui32) < 0) {
            return false;
        }
        if (ui32 > 0) {
            if (ui32 > 127) {
                return false;
            }
            _pCommHelper->receiveBlob (buf, ui32);
            buf[ui32] = '\0';
            queryQualifiers = buf;
        }

        if (_pCommHelper->getReaderRef()->read32 (&uiQueryLen) < 0) {
            return false;
        }
        if (uiQueryLen > 0) {
            pQuery = malloc (uiQueryLen);
            if (pQuery == NULL) {
                return false;
            }
            _pCommHelper->receiveBlob (pQuery, uiQueryLen);
        }

        _pProxy->searchArrived (queryId.c_str(), groupName.c_str(), querier.c_str(),
                                queryType.c_str(), queryQualifiers.c_str(),  pQuery,
                                uiQueryLen);

        _pCommHelper->sendLine ("OK");
    }
    catch (Exception) {
        return false;
    }
    return true;
}

int DisseminationServiceProxyCallbackHandler::doNewPeerCallback (void)
{
    uint32 ui32Len;
    char *pszPeerNodeId = NULL;
    _pCommHelper->getReaderRef()->read32 (&ui32Len);
    if (ui32Len > 0) {
        pszPeerNodeId = new char [ui32Len+1];
        if (pszPeerNodeId == NULL) {
            return -1;
        }
    }
    else {
        return 0;
    }

    _pCommHelper->receiveBlob (pszPeerNodeId, ui32Len);
    pszPeerNodeId[ui32Len] = '\0';

    _pProxy->newPeer (pszPeerNodeId);
    delete[] pszPeerNodeId;

    _pCommHelper->sendLine ("OK");

    return 0;
}

int DisseminationServiceProxyCallbackHandler::doDeadPeerCallback (void)
{
    uint32 ui32Len;
    char *pszPeerNodeId = NULL;
    _pCommHelper->getReaderRef()->read32 (&ui32Len);
    if (ui32Len > 0) {
        pszPeerNodeId = new char [ui32Len+1];
        if (pszPeerNodeId == NULL) {
            return -1;
        }
    }
    else {
        return 0;
    }

    _pCommHelper->receiveBlob (pszPeerNodeId, ui32Len);
    pszPeerNodeId[ui32Len] = '\0';

    _pProxy->deadPeer (pszPeerNodeId);
    delete[] pszPeerNodeId;

    _pCommHelper->sendLine ("OK");

    return 0;
}

