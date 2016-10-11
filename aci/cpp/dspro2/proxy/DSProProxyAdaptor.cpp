/**
 * DSProProxyAdaptor.cpp
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

#include "DSProProxyAdaptor.h"

#include "Defs.h"
#include "DSPro.h"
#include "DSProImpl.h"
#include "DSProProxyServer.h"

#include "Instrumentator.h"
#include "NodePath.h"
#include "NodeContextManager.h"
#include "PeerNodeContext.h"

#include "BufferReader.h"
#include "Logger.h"
#include "Writer.h"
#include "ConfigManager.h"
#include "ControlMessageNotifier.h"

#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define MostGeneralException std::exception
#define checkAndLogMsg if (pLogger) pLogger->logMsg

DSProProxyAdaptor::DSProProxyAdaptor (DSProProxyServer *pDSPProxyServer)
    : SearchListener ("DSProProxyAdaptor")
{
    // _pDisSvcPro and _pCallbackCommHelper are initialized in the parent's constructor
    _pDSPro = pDSPProxyServer->getDisServiceProRef();
    _pDisSvcProProxyServer = pDSPProxyServer;
    _pCommHelper = NULL;
    _pCallbackCommHelper = NULL;
    _ui16ClientID = 0;
    _bListenerProRegistered = false;
    _bMatchmakingLogListenerRegistered = false;
    _bCtrlMsgListenerRegistered = false;
    _bSearchListenerRegistered = false;
}

DSProProxyAdaptor::~DSProProxyAdaptor()
{
    if (_bListenerProRegistered) {
        _pDSPro->deregisterDSProListener (_ui16PathRegisteredCbackClientId, this);
    }
    if (_bMatchmakingLogListenerRegistered && pInstrumentator != NULL) {
        pInstrumentator->deregisterAndDisableMatchmakingLogListener (_ui16MatchmakingCbackClientId);
    }
    if (_bCtrlMsgListenerRegistered && pCtrlMsgNotifier != NULL) {
        pCtrlMsgNotifier->deregisterAndDisableControllerMessageListener (_ui16CtrlMsgCbackClientId);
    }
    if (_bSearchListenerRegistered) {
        _pDSPro->deregisterSearchListener (_ui16SearchListenerClientID, this);
    }

    char szId[10];
    snprintf (szId, sizeof (szId)-1, "%d", _ui16ClientID);
    _pDisSvcProProxyServer->_proxies.remove (szId);

    // Close connections
    if (_pCommHelper != NULL) {
        CommHelperError error;
        _pCommHelper->closeConnection (error);
        if (error != SimpleCommHelper2::None) {
            delete _pCommHelper;
            _pCommHelper = NULL;
        }        
    }
    if (_pCallbackCommHelper != NULL) {
        CommHelperError error;
        _pCallbackCommHelper->closeConnection (error);
        if (error != SimpleCommHelper2::None) {
            delete _pCallbackCommHelper;
            _pCallbackCommHelper = NULL;
        }
    }

    checkAndLogMsg ("DSProProxyAdaptor::~DSProProxyAdaptor",
                    Logger::L_HighDetailDebug, "adaptor deleted.\n");
}

int DSProProxyAdaptor::init (SimpleCommHelper2 *pCommHelper, uint16 ui16ID)
{
    _pCommHelper = pCommHelper;
    _ui16ClientID = ui16ID;

    return 0;
}

uint16 DSProProxyAdaptor::getClientID()
{
    return _ui16ClientID;
}

void DSProProxyAdaptor::setCallbackCommHelper (SimpleCommHelper2 *pCommHelper)
{
    if (_pCallbackCommHelper) {
        delete _pCallbackCommHelper;
    }

    _pCallbackCommHelper = pCommHelper;
}

void DSProProxyAdaptor::run()
{
    const char *pszMethodName = "DSProProxyAdaptor::run";
    setName (pszMethodName);

    CommHelperError error = SimpleCommHelper2::None;
    char buf[128];
    int size;

    while (!terminationRequested()) {

        size = _pCommHelper->receiveLine (buf, sizeof (buf), error);
        if (error != SimpleCommHelper2::None) {
            break;
        }
        if (size <= 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "received line of size %d.\n", size);
            continue;
        }

        bool bSuccess;

        if (strcmp (buf, "addUserId") == 0) {
            bSuccess = doAddUserId (error);
        }
        else if (strcmp (buf, "setMissionId") == 0) {
            bSuccess = doSetMissionId (error);
        }
        else if (strcmp (buf, "addCustumPoliciesAsXM") == 0) {
            bSuccess = doAddCustumPoliciesAsXML (error);
        }
        else if (strcmp (buf, "cancel_str") == 0) {
            bSuccess = doCancel (error);
        }
        else if (strcmp (buf, "configureProperties") == 0) {
            bSuccess = doConfigureProperties (error);
        }
        else if (strcmp (buf, "registerPath") == 0) {
            bSuccess = doRegisterPath (error);
        }
        else if (strcmp (buf, "setCurrentPath") == 0) {
            bSuccess = doSetCurrentPath (error);
        }
        else if (strcmp (buf, "getCurrentPath") == 0) {
            bSuccess = doGetCurrentPath (error);
        }
        else if (strcmp (buf, "setCurrentPosition") == 0) {
            bSuccess = doSetCurrentPosition (error);
        }
        else if (strcmp (buf, "setBatteryLevel") == 0) {
            bSuccess = doSetBatteryLevel (error);
        }
        else if (strcmp (buf, "setMemoryAvailable") == 0) {
            bSuccess = doSetMemoryAvailable (error);
        }
        else if (strcmp (buf, "addPeer") == 0) {
            bSuccess = doAddPeer (error);
        }
        else if (strcmp (buf, "getPeerNodeContext") == 0) {
            bSuccess = doGetPeerNodeContext (error);
        }
        else if (strcmp (buf, "getPeerList") == 0) {
            bSuccess = doGetPeerList (error);
        }
        else if (strcmp (buf, "getAdaptorType") == 0) {
            bSuccess = doGetAdaptorType (error);
        }
        else if (strcmp (buf, "notUseful") == 0) {
            bSuccess = doNotUseful (error);
        }
        else if (strcmp (buf, "addMessage") == 0) {
            bSuccess = doAddMessage (error);
        }
        else if (strcmp (buf, "addMessage_AVList") == 0) {
            bSuccess = doAddMessage_AVList (error);
        }
        else if (strcmp (buf, "chunkAndAddMessage") == 0) {
            bSuccess = doChunkAndAddMessage (error);
        }
        else if (strcmp (buf, "chunkAndAddMessage_AVList") == 0) {
            bSuccess = doChunkAndAddMessage_AVList (error);
        }
        else if (strcmp (buf, "addAnnotation") == 0) {
            bSuccess = doAddAnnotation (error);
        }
        else if (strcmp (buf, "addAnnotationRef") == 0) {
            bSuccess = doAddAnnotationRef (error);
        }
        else if (strcmp (buf, "getData") == 0) {
            bSuccess = doGetData (error);
        }
        else if (strcmp (buf, "getMatchingMetaDataAsXML") == 0) {
            bSuccess = doGetMatchingMetaDataAsXML (error);
        }
        else if (strcmp (buf, "getDisService") == 0) {
            bSuccess = doGetDisService (error);
        }
        else if (strcmp (buf, "getNodeId") == 0) {
            bSuccess = doGetNodeId (error);
        }
        else if (strcmp (buf, "getPathForPeer") == 0) {
            bSuccess = doGetPathForPeer (error);
        }
        else if (strcmp (buf, "getSessionId") == 0) {
            bSuccess = doSessionId (error);
        }
        else if (strcmp (buf, "registerPathRegisteredCallback") == 0) {
            bSuccess = doRegisterPathRegisteredCallback();
        }
        else if (strcmp (buf, "registerCtrlMsgCallback") == 0) {
            bSuccess = doRegisterCtrlMsgCallback();
        }
        else if (strcmp (buf, "registerMatchmakingLogCallback") == 0) {
            bSuccess = doRegisterMatchmakingLogCallback();
        }
        else if (strcmp (buf, "registerSearchCallback") == 0) {
            bSuccess = doRegisterSearchCallback();
        }
        else if (strcmp (buf, "deregisterSearchCallback") == 0) {
            bSuccess = doDeregisterSearchCallback();
        }
        else if (strcmp (buf, "requestCustomAreaChunk") == 0) {
            bSuccess = doRequestCustomAreaChunk (error);
        }
        else if (strcmp (buf, "requestCustomTimeChunk") == 0) {
            bSuccess = doRequestCustomTimeChunk (error);
        }
        else if (strcmp (buf, "requestMoreChunks") == 0) {
            bSuccess = doRequestMoreChunks (error);
        }
        else if (strcmp (buf, "resetTransmissionCounters") == 0) {
            bSuccess = doResetTransmissionCounters (error);
        }
        else if (strcmp (buf, "getDSProId") == 0) {
            bSuccess = doGetDSProIds (error);
        }
        else if (strcmp (buf, "search") == 0) {
            bSuccess = doSearch (error);
        }
        else if (strcmp (buf, "replyToQuery") == 0) {
            bSuccess = doReplyToQuery (error);
        }
        else if (strcmp (buf, "volatileReplyToQuery") == 0) {
            bSuccess = doReplyToVolatileQuery (error);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "!!!! UNKNOWN OPERATION!!!! [%s]\n", buf);
            bSuccess = false;
        }

        if (error != SimpleCommHelper2::None) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "Command <%s> failed.  Communication error.\n", buf);
            break;
        }
        else if (bSuccess) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Command <%s> worked.\n", buf);
        }
        else {
            _pCommHelper->sendLine (error, "ERROR");
            if (error != SimpleCommHelper2::None) {
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "Communication error when notifying command error.\n");
            }
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Command <%s> failed.\n", buf);
        }

        if (error != SimpleCommHelper2::None) {
            break;
        }
    }

    if (error != SimpleCommHelper2::None) {
        buf[127] = '\0';
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "exception in handling client request %s.\n", buf);
    }

    // the destructor will take care of closing all the connections
    // and de-registering this Adaptor from the DisseminationServiceProxyServer.
    delete this;
}

bool DSProProxyAdaptor::doAddUserId (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    uint32 ui32Len = 0;
    if (_pCommHelper->getReaderRef()->read32 (&ui32Len) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32Len == 0) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }
    if (ui32Len > 511U) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    char buf[512];
    _pCommHelper->receiveBlob (buf, ui32Len, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    buf[ui32Len] = '\0';

    int rc = _pDSPro->addUserId (buf);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetMissionId (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    uint32 ui32Len = 0;
    if (_pCommHelper->getReaderRef()->read32 (&ui32Len) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32Len == 0) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }
    if (ui32Len > 511U) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    char buf[512];
    _pCommHelper->receiveBlob (buf, ui32Len, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    buf[ui32Len] = '\0';

    int rc = _pDSPro->setMissionId (buf);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doAddCustumPoliciesAsXML (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    uint16 ui16Count = 0;
    if (_pCommHelper->getReaderRef()->read16 (&ui16Count) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui16Count == 0) {
        return true;
    }
    bool bSuccess = true;
    char **ppszXMLDocs = (char **) calloc (ui16Count + 1, sizeof (char));
    if (ppszXMLDocs == NULL) {
        return false;
    }
    uint16 i = 0;
    for (; i < ui16Count; i++) {
        uint32 ui32Len = 0;
        if (_pCommHelper->getReaderRef()->read32 (&ui32Len) < 0) {
            error = SimpleCommHelper2::CommError;
            bSuccess = false;
            break;
        }
        if (ui32Len == 0) {
            error = SimpleCommHelper2::ProtocolError;
            bSuccess = false;
            break;
        }
        if (ui32Len > 2047U) {
            error = SimpleCommHelper2::ProtocolError;
            bSuccess = false;
            break;
        }

        char buf[2048];
        _pCommHelper->receiveBlob (buf, ui32Len, error);
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
            break;
        }
        buf[ui32Len] = '\0';
        ppszXMLDocs[i] = strDup (buf);
    }
    int rc = 0;
    if (bSuccess) {
        ppszXMLDocs[i] = NULL;
        rc = _pDSPro->addCustumPoliciesAsXML ((const char **) ppszXMLDocs);
        if (rc == 0) {
            _pCommHelper->sendLine (error, "OK");
        }
    }

    // Deallocate XML docs
    for (uint16 j = 0; j < i; j++) {
        free (ppszXMLDocs[j]);
    }
    free (ppszXMLDocs);

    return ((error != SimpleCommHelper2::None) && (rc == 0));
}

bool DSProProxyAdaptor::doCancel (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    char buf[256];

    _pCommHelper->receiveLine (buf, sizeof(buf), error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDSPro->cancel (buf);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    else {
        _pCommHelper->sendLine (error, "ERROR %d", rc);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }

    return true;
}

bool DSProProxyAdaptor::doConfigureProperties (CommHelperError &error)
{   
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    uint32 ui32PropertiesLength = 0;
    ConfigManager *pCfgMgr = new ConfigManager();
    if (pCfgMgr == NULL) {
        checkAndLogMsg ("DSProProxyAdaptor::doConfigureProperties", memoryExhausted);
        return false;
    }
    pCfgMgr->init();

    if (pReader->read32 (&ui32PropertiesLength) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    checkAndLogMsg ("DSProProxyAdaptor::doConfigureProperties", Logger::L_Info,
                    "Going to read buffer of size %u\n", ui32PropertiesLength);
    char *pBuf = (char*) calloc (ui32PropertiesLength+1, sizeof (char));

    _pCommHelper->receiveBlob (pBuf, ui32PropertiesLength, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    BufferReader br (pBuf, ui32PropertiesLength);
    pCfgMgr->read (&br, ui32PropertiesLength, false);
    pCfgMgr->display();
    _pDSPro->configureProperties (pCfgMgr);
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doRegisterPath (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();

    NodePath *pPath = new NodePath();
    if (pPath->read (pReader, 8092, false) < 0) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }
    pPath->display();

    int rc = _pDSPro->registerPath (pPath);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    else {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doSetCurrentPath (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    char buf[100];
    String pathId;

    // read path ID
    if (_pCommHelper->receiveLine (buf, sizeof (buf), error) > 0) {
        pathId = buf;
    }
    else {
        return false;
    }
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDSPro->setCurrentPath (pathId.c_str());
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    else {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetCurrentPath (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();

    NodePath *pCurrPath = _pDSPro->getCurrentPath();
    if (pCurrPath != NULL) {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        pCurrPath->write (pWriter);
    }
    else {
        _pCommHelper->sendLine (error, "NOPATH");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doSetCurrentPosition (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    char buf[1000];
    float fLatitude;
    float fLongitude;
    float fAltitude;
    char *pszLocation;
    char *pszNote;
    uint32 ui32Len;
    int rc;

    // read latitude
    if ((rc = _pCommHelper->receiveLine (buf, sizeof (buf), error)) > 0 && (error == SimpleCommHelper2::None)) {
        fLatitude = (float) atof (buf);
    }
    else {
        return false;
    }

    // read longitude
    if ((rc = _pCommHelper->receiveLine (buf, sizeof (buf), error)) > 0 && (error == SimpleCommHelper2::None)) {
        fLongitude = (float) atof (buf);
    }
    else {
        return false;
    }

    // read altitude
    if ((rc = _pCommHelper->receiveLine (buf, sizeof (buf), error)) > 0 && (error == SimpleCommHelper2::None)) {
        fAltitude = (float) atof (buf);
    }
    else {
        return false;
    }

    // read location's length and location
    if ((rc = pReader->read32 (&ui32Len))) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32Len > 0) {
        pszLocation = new char[ui32Len+1];
        pszLocation[ui32Len] = '\0';
        _pCommHelper->receiveBlob (pszLocation, ui32Len, error);
        if (error != SimpleCommHelper2::None) {
            delete[] pszLocation;
            return false;
        }
    }
    else {
        pszLocation = NULL;
    }

    // read note's length and note
    if ((rc = pReader->read32 (&ui32Len)) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pszLocation != NULL) {
            delete[] pszLocation;
        }
        return false;
    }
    if (ui32Len > 0) {
        pszNote = new char[ui32Len+1];
        pszNote[ui32Len] = '\0';
        _pCommHelper->receiveBlob (pszNote, ui32Len, error);
        if (error != SimpleCommHelper2::None) {
            if (pszLocation != NULL) {
                delete[] pszLocation;
            }
            if (pszNote != NULL) {
                delete[] pszNote;
            }
            return false;
        }
    }
    else {
        pszNote = NULL;
    }

    rc = _pDSPro->setCurrentPosition (fLatitude, fLongitude, fAltitude, pszLocation, pszNote);
    if (pszLocation != NULL) {
        delete[] pszLocation;
    }
    if (pszNote != NULL) {
        delete[] pszNote;
    }
    if (rc < 0) {
        return false;
    }

    _pCommHelper->sendLine (error, "OK");

    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doSetBatteryLevel (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    uint32 ui32BatLev;
    int rc;

    if ((rc = pReader->read32 (&ui32BatLev)) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    if (_pDSPro->setBatteryLevel (ui32BatLev) < 0) {
        error = SimpleCommHelper2::CommError;
        return false; 
    }

    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doSetMemoryAvailable (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    uint32 ui32AvMem;
    int rc;

    if ((rc = pReader->read32 (&ui32AvMem)) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    if (_pDSPro->setBatteryLevel (ui32AvMem) < 0) {
        error = SimpleCommHelper2::CommError;
        return false; 
    }

    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doAddPeer (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    AdaptorType ui8AdaptorType;
    char *pszNetworkInterface = NULL;
    char *pszRemoteAddress = NULL;
    uint16 ui16Port = 0;
    int rc;

    // read peer node id's length
    if ((rc = pReader->read16 (&ui8AdaptorType))) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    uint32 ui32Len = 0;
    if ((rc = pReader->read32 (&ui32Len))) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32Len > 0) {
        pszNetworkInterface = (char*) calloc (ui32Len + 1, sizeof (char));
        _pCommHelper->receiveBlob (pszNetworkInterface, ui32Len, error);
    }
    if (error != SimpleCommHelper2::None) {
        if (pszNetworkInterface != NULL) {
            free (pszNetworkInterface);
        }
        return false;
    }

    if ((rc = pReader->read32 (&ui32Len))) {
        error = SimpleCommHelper2::CommError;
        if (pszNetworkInterface != NULL) {
            free (pszNetworkInterface);
        }
        return false;
    }
    if (ui32Len > 0) {
        pszRemoteAddress = (char *) calloc (ui32Len + 1, sizeof (char));
        _pCommHelper->receiveBlob (pszRemoteAddress, ui32Len, error);
    }
    if (error != SimpleCommHelper2::None) {
        if (pszNetworkInterface != NULL) {
            free (pszNetworkInterface);
        }
        if (pszRemoteAddress != NULL) {
            free (pszRemoteAddress);
        }
        return false;
    }

    if ((rc = pReader->read16 (&ui16Port))) {
        error = SimpleCommHelper2::CommError;
        if (pszNetworkInterface != NULL) {
            free (pszNetworkInterface);
        }
        if (pszRemoteAddress != NULL) {
            free (pszRemoteAddress);
        }
        return false;
    }

    if (_pDSPro->addPeer (ui8AdaptorType, pszNetworkInterface, pszRemoteAddress, ui16Port) < 0) {
        if (pszNetworkInterface != NULL) {
            free (pszNetworkInterface);
        }
        if (pszRemoteAddress != NULL) {
            free (pszRemoteAddress);
        }
        return false;
    }

    _pCommHelper->sendLine (error, "OK");

    if (pszNetworkInterface != NULL) {
        free (pszNetworkInterface);
    }
    if (pszRemoteAddress != NULL) {
        free (pszRemoteAddress);
    }
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetPeerNodeContext (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    String sNodeId;
    uint32 ui32Len;
    bool bSuccess = false;
    int rc;

    // read peer node id's length
    if ((rc = pReader->read32 (&ui32Len))) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    // read peer node id
    if ((ui32Len == 0) || (ui32Len > (BUF_LEN - 1))) {
        return false;
    }
    else {
        _buf[ui32Len] = '\0';
        _pCommHelper->receiveBlob (_buf, ui32Len, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        sNodeId = _buf;
    }

    NodeContext *pNodeContext = NULL;
    if (sNodeId == _pDSPro->getNodeId()) {
        pNodeContext = (NodeContext *) _pDSPro->_pImpl->_pNodeContextMgr->getLocalNodeContext();
    }
    else { 
        pNodeContext = _pDSPro->_pImpl->_pNodeContextMgr->getPeerNodeContext (sNodeId);
    }

    if (pNodeContext != NULL) {
        _pCommHelper->sendLine (error, "OK");
        if (error == SimpleCommHelper2::None) {
            if (pNodeContext->writeForDSProListener (_pCommHelper->getWriterRef()) > 0) {
                bSuccess = true;
            }
        }
    }

    if (sNodeId == _pDSPro->getNodeId()) {
        _pDSPro->_pImpl->_pNodeContextMgr->releaseLocalNodeContext();
    }
    else {
        _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doGetAdaptorType (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    AdaptorId adaptorId;
    AdaptorType adaptorType;
    int rc;

    if ((rc = pReader->read16 (&adaptorId)) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    if (_pDSPro->getAdaptorType (adaptorId, adaptorType) < 0) {
        return false;
    }

    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->write8 (&adaptorType) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetData (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();
    void *pData = NULL;
    String messageId;
    uint32 ui32bufferLength = 0;

    // read the message ID
    if ((_pCommHelper->receiveLine (_buf, BUF_LEN, error) > 0) && (error == SimpleCommHelper2::None)) {
        messageId = _buf;
    }
    else {
        error = (error == SimpleCommHelper2::None ? error : SimpleCommHelper2::CommError);
        return false;
    }

    char *pszSearchId = NULL;
    if (_pCommHelper->getReaderRef()->readString (&pszSearchId) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    bool bHasMoreChunks = false;
    if (_pDSPro->getData (messageId.c_str(), pszSearchId, &pData, ui32bufferLength, bHasMoreChunks) < 0) {
        checkAndLogMsg ("DSProProxyAdaptor::doGetData", Logger::L_MildError,
                        "getData() returned an error\n");
        error = SimpleCommHelper2::None;
        return false;
    }
    if (pszSearchId != NULL) {
        free (pszSearchId);
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        _pDSPro->release (messageId.c_str(), pData);
        return false;
    }

    if (pData == NULL) {
        ui32bufferLength = 0;
    }

    if (pWriter->write32 (&ui32bufferLength) < 0) {
        error = SimpleCommHelper2::CommError;
        _pDSPro->release (messageId.c_str(), pData);
        return false;
    }
    if (ui32bufferLength > 0) {
        // send the data
        _pCommHelper->sendBlob (pData, ui32bufferLength, error);
        if (error != SimpleCommHelper2::None) {
            _pDSPro->release (messageId.c_str(), pData);
            return false;
        }

        // send the bHasMoreChunks flag
        uint8 ui8 = (bHasMoreChunks ? 1 : 0);
        if (pWriter->write8 (&ui8) < 0) {
            error = SimpleCommHelper2::CommError;
            _pDSPro->release (messageId.c_str(), pData);
            return false;
        }
    }

    _pDSPro->release (messageId.c_str(), pData);
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetMatchingMetaDataAsXML (CommHelperError &error)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    // Read the number of attributes
    uint32 ui32Attributes = 0;
    pReader->read32 (&ui32Attributes);
    SQLAVList aVList (ui32Attributes);
    error = readMetadataAsAvList (pReader, aVList, ui32Attributes);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the time range
    int64 i64BeginArrivalTimestamp = 0;
    int64 i64EndArrivalTimestamp = 0;
    if (pReader->read64 (&i64BeginArrivalTimestamp) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->read64 (&i64EndArrivalTimestamp) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    char **ppszMetadata = _pDSPro->getMatchingMetadataAsXML (&aVList, i64BeginArrivalTimestamp,
                                                             i64EndArrivalTimestamp);

    bool bSucceded = true;
    if (ppszMetadata != NULL) {
        for (unsigned int i = 0; ppszMetadata[i] != 0; i++) {
            if (bSucceded && (strlen (ppszMetadata[i]) > 0)) {
                _pCommHelper->sendStringBlock (ppszMetadata[i], error);
                if (error != SimpleCommHelper2::None) {
                    bSucceded = false;
                }
            }
            free (ppszMetadata[i]);
        }
        free (ppszMetadata);
    }

    if (bSucceded) {
        uint32 null = 0;
        if ( _pCommHelper->getWriterRef()->write32 (&null) != 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }

    assert (bSucceded);
    return true;
}

bool DSProProxyAdaptor::doNotUseful (CommHelperError &)
{
    // TODO: implement this!
    return false;
}

bool DSProProxyAdaptor::doSearch (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String queryType;
    String queryQualifiers;
    unsigned int uiQueryLen;
    void *pQuery = NULL;
    uint32 ui32;

    // read the group name
    if (pReader->read32 (&ui32) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32 > 0) {
        if (ui32 > (BUF_LEN-1)) {
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    _buf[ui32] = '\0';
    groupName = _buf;

    // read the query type
    if (pReader->read32 (&ui32) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32 > 0) {
        if (ui32 > (BUF_LEN-1)) {
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    _buf[ui32] = '\0';
    queryType = _buf;

    // read the query qualifiers
    if (pReader->read32 (&ui32) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32 > 0) {
        if (ui32 > (BUF_LEN-1)) {
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    _buf[ui32] = '\0';
    queryQualifiers = _buf;

    if (_pCommHelper->getReaderRef()->read32 (&uiQueryLen) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (uiQueryLen > 0) {
        pQuery = malloc (uiQueryLen);
        if (pQuery == NULL) {
            return false;
        }
        _pCommHelper->receiveBlob (pQuery, uiQueryLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pQuery);
            return false;
        }
    }

    int64 i64Timeout = 0;
    if (_pCommHelper->getReaderRef ()->read64 (&i64Timeout) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    char *pszQueryId = NULL;
    int rc = _pDSPro->search (groupName.c_str(), queryType.c_str(), queryQualifiers.c_str(),
                              pQuery, uiQueryLen, i64Timeout, &pszQueryId);

    bool bSucceded = ((rc == 0) && (pszQueryId != NULL));

    if (bSucceded) {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSucceded = false;
        }
    }

    if (bSucceded) {
        // write the query id assigned to the query
        _pCommHelper->sendStringBlock (pszQueryId, error);
        if (error != SimpleCommHelper2::None) {
            bSucceded = false;
        }
    }

    if (pQuery != NULL) {
        free (pQuery);
    }
    if (pszQueryId) {
        free (pszQueryId);
    }

    return bSucceded;
}

bool DSProProxyAdaptor::doReplyToQuery (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    String queryId;
    uint32 ui32;

    // read the queryId name
    if (pReader->read32 (&ui32) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32 > 0) {
        if (ui32 > (BUF_LEN-1)) {
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    _buf[ui32] = '\0';
    queryId = _buf;

    uint32 ui32NumberOfElements = 0;
    if (pReader->read32 (&ui32NumberOfElements) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32NumberOfElements == 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    char **ppszMsgIds = (char **) calloc (ui32NumberOfElements+1, sizeof (char *));
    if (ppszMsgIds == NULL) {
        return false;
    }

    bool bSucceded = true;
    uint32 uiRead = 0;
    for (uint32 i = 0; i < ui32NumberOfElements; i++) {
        if (pReader->read32 (&ui32) < 0) {
            error = SimpleCommHelper2::CommError;
            bSucceded = false;
            break;
        }
        if (ui32 > 0) {
            ppszMsgIds[uiRead] = (char *) calloc (ui32+1, sizeof (char));
            if (ppszMsgIds[uiRead] != NULL) {
                _pCommHelper->receiveBlob (ppszMsgIds[i], ui32, error);
                if (error != SimpleCommHelper2::None) {
                    bSucceded = false;
                    break;
                }
                ppszMsgIds[i][ui32] = '\0';
                uiRead++;
            }
            else {
                bSucceded = false;
                break;
            }
        }
        else {
            ppszMsgIds[uiRead] = NULL;
        }
    }
    ppszMsgIds[ui32NumberOfElements] = NULL;

    if (bSucceded) {
        int rc = _pDSPro->searchReply (queryId.c_str(), (const char **)ppszMsgIds);

        bSucceded = (rc == 0);
        if (bSucceded) {
            _pCommHelper->sendLine (error, "OK");
            if (error != SimpleCommHelper2::None) {
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

bool DSProProxyAdaptor::doReplyToVolatileQuery (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    String queryId;
    uint32 ui32;

    // read the queryId name
    if (pReader->read32 (&ui32) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (ui32 > 0) {
        if (ui32 > (BUF_LEN - 1)) {
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    _buf[ui32] = '\0';
    queryId = _buf;

    int rc;
    uint32 ui32Len = 0U;
    // read peer node id's length
    if ((rc = pReader->read32 (&ui32Len))) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    // read peer node id
    if ((ui32Len == 0) || (ui32Len > (BUF_LEN - 1))) {
        return false;
    }
    _pCommHelper->receiveBlob (_buf, ui32Len, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    rc = _pDSPro->volatileSearchReply (queryId.c_str(), _buf, ui32Len);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
        if (error == SimpleCommHelper2::None) {
            return true;
        }
    }
    return false;
}

bool DSProProxyAdaptor::doAddMessage (CommHelperError &error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    uint32 ui32DataLen = 0;
    void *pData = NULL;
    int64 i64ExpirationTime = 0;
    char *pMsgId = NULL;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsXMLString (pReader, sMetadata);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the data length
    if (pReader->read32 (&ui32DataLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the data
    if (ui32DataLen > 0) {
        pData = malloc (ui32DataLen);
        if (pData == NULL) {
            checkAndLogMsg ("DSProProxyAdaptor::doAddMessage", memoryExhausted);
        }
        _pCommHelper->receiveBlob (pData, ui32DataLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pData);
            return false;
        }
    }

    // read the expiration time
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->addMessage (groupName, objectId, instanceId, sMetadata,
                                  pData, ui32DataLen, i64ExpirationTime, &pMsgId);

    bool bSuccess = ((rc == 0) && (pMsgId != NULL));

    if (bSuccess) {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    // write the msg id of the pushed data
    if (bSuccess) {
        _pCommHelper->sendStringBlock (pMsgId, error);
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (bSuccess) {
        _pCommHelper->receiveMatch (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (pData != NULL) {
        delete[] (char*)pData;
    }
    if (pMsgId != NULL) {
        free (pMsgId);
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doAddMessage_AVList (CommHelperError &error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    uint32 ui32Attributes = 0;
    uint32 ui32DataLen = 0;
    void *pData = NULL;
    int64 i64ExpirationTime = 0;
    char *pMsgId = NULL;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the metadata
    // Read the number of attributes
    if (pReader->read32 (&ui32Attributes) != 0) {
        return false;
    }
    SQLAVList aVList (ui32Attributes);
    error = readMetadataAsAvList (pReader, aVList, ui32Attributes);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the data length
    if (pReader->read32 (&ui32DataLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the data
    if (ui32DataLen > 0) {
        pData = malloc (ui32DataLen);
        if (pData == NULL) {
            return false;
        }
        _pCommHelper->receiveBlob (pData, ui32DataLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pData);
            return false;
        }
    }

    // read the expiration time
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->addMessage (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                  &aVList, pData, ui32DataLen, i64ExpirationTime, &pMsgId);

    if (pData != NULL) {
        free (pData);
    }
    const bool bSuccess = ((rc == 0) && (pMsgId != NULL));

    if (error == SimpleCommHelper2::None) {
        if (bSuccess) {
            _pCommHelper->sendLine (error, "OK");
        }
        else {
            return bSuccess;
        }
        if (error != SimpleCommHelper2::None) {
            return bSuccess;
        }
    }

    // write the msg id of the pushed data
    if (error == SimpleCommHelper2::None) {
        _pCommHelper->sendStringBlock (pMsgId, error);
    }

    if (pMsgId != NULL) {
        free (pMsgId);
    }
    return bSuccess;
}

bool DSProProxyAdaptor::doChunkAndAddMessage (CommHelperError &error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    uint32 ui32DataLen;
    void *pData = NULL;
    String sDataMimeType;
    int64 i64ExpirationTime;
    char *pszMsgId = NULL;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsXMLString (pReader, sMetadata);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the data length
    if (pReader->read32 (&ui32DataLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the data
    if (ui32DataLen > 0) {
        pData = calloc (ui32DataLen, sizeof (char));
        if (pData == NULL) {
            return false;
        }
        _pCommHelper->receiveBlob (pData, ui32DataLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pData);
            return false;
        }
    }

    // read the metadata length
    uint32 ui32MimeTypeLen = 0;
    if (pReader->read32 (&ui32MimeTypeLen) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    // read the MIME type
    if (ui32MimeTypeLen > 0) {
        if (ui32MimeTypeLen > (BUF_LEN-1)) {
            if (pData != NULL) {
                free (pData);
            }
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32MimeTypeLen, error);
        if (error != SimpleCommHelper2::None) {
            if (pData != NULL) {
                free (pData);
            }
            return false;
        }
        _buf[ui32MimeTypeLen] = '\0';
        sDataMimeType = _buf;
    }

    // read the expiration time
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->chunkAndAddMessage (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                          sMetadata.c_str(), pData, ui32DataLen, sDataMimeType.c_str(),
                                          i64ExpirationTime, &pszMsgId);
	if (pData != NULL) {
		free(pData);
	}

    const bool bSuccess = ((rc == 0) && (pszMsgId != NULL));

	if (error == SimpleCommHelper2::None) {
		if (bSuccess) {
			_pCommHelper->sendLine(error, "OK");
		}
		else {
			return bSuccess;
		}
		if (error != SimpleCommHelper2::None) {
			return bSuccess;
		}
	}

	// write the msg id of the pushed data
	if (error == SimpleCommHelper2::None) {
		_pCommHelper->sendStringBlock(pszMsgId, error);
	}

	if (pszMsgId != NULL) {
		free(pszMsgId);
	}

    return (bSuccess && (error == SimpleCommHelper2::None));
}

bool DSProProxyAdaptor::doChunkAndAddMessage_AVList (CommHelperError &error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    uint32 ui32Attributes = 0;
    uint32 ui32DataLen;
    void *pData = NULL;
    String sDataMimeType;
    int64 i64ExpirationTime;
    char *pszMsgId = NULL;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the metadata
    // Read the number of attributes
    if (pReader->read32 (&ui32Attributes) != 0) {
        return false;
    }
    SQLAVList aVList (ui32Attributes);
    error = readMetadataAsAvList (pReader, aVList, ui32Attributes);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the data length
    if (pReader->read32 (&ui32DataLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the data
    if (ui32DataLen > 0) {
        pData = malloc (ui32DataLen);
        if (pData == NULL) {
            return false;
        }
        _pCommHelper->receiveBlob (pData, ui32DataLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pData);
            return false;
        }
    }

    // read the MIME type length
    uint32 ui32MimeTypeLen = 0;
    if (pReader->read32 (&ui32MimeTypeLen) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    // read the MIME type
    if (ui32MimeTypeLen > 0) {
        if (ui32MimeTypeLen > (BUF_LEN-1)) {
            if (pData != NULL) {
                free (pData);
            }
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32MimeTypeLen, error);
        if (error != SimpleCommHelper2::None) {
            if (pData != NULL) {
                free (pData);
            }
            return false;
        }
        _buf[ui32MimeTypeLen] = '\0';
        sDataMimeType = _buf;
    }

    // read the expiration time
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pData != NULL) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->chunkAndAddMessage (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                          &aVList, pData, ui32DataLen, sDataMimeType.c_str(), i64ExpirationTime,
                                          &pszMsgId);
 
    const bool bSuccess = ((rc == 0) && (pszMsgId != NULL));

	if (error == SimpleCommHelper2::None) {
		if (bSuccess) {
			_pCommHelper->sendLine (error, "OK");

			// write the msg id of the pushed data
			if (error == SimpleCommHelper2::None) {
				_pCommHelper->sendStringBlock(pszMsgId, error);
			}
		}
	}

    if (pData != NULL) {
        free (pData);
    }
    if (pszMsgId != NULL) {
        free (pszMsgId);
    }
    return (bSuccess && (error == SimpleCommHelper2::None));
}

bool DSProProxyAdaptor::doAddAnnotation (CommHelperError &error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    uint32 ui32RefObj = 0;
    String sReferredObj;
    int64 i64ExpirationTime;
    char *pMsgId = NULL;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsXMLString (pReader, sMetadata);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the referred object id's length
    if (pReader->read32 (&ui32RefObj) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the referred object id
    if (ui32RefObj > 0) {
        if (ui32RefObj > (BUF_LEN-1)) {
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32RefObj, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        _buf[ui32RefObj] = '\0';
        sReferredObj = _buf;
    }

    // read the expiration time
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // addAnnotation modifies pMetadata - no problem here, since pMetadata
    // is not used anymore, in fact, it is even deallocated
    int rc = _pDSPro->addAnnotation (groupName.c_str(), objectId.c_str(),
                                     instanceId.c_str(), sMetadata.c_str(),
                                     sReferredObj.c_str(), i64ExpirationTime,
                                     &pMsgId);

    bool bSuccess = ((rc == 0) && (pMsgId != NULL));

    if (bSuccess) {
        _pCommHelper->receiveMatch (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    // write the msg id of the pushed data
    if (bSuccess) {
        _pCommHelper->sendStringBlock (pMsgId, error);
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (bSuccess) {
        _pCommHelper->receiveMatch (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (pMsgId != NULL) {
        free (pMsgId);
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doAddAnnotationRef (CommHelperError &error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    String sReferredObj;
    int64 i64ExpirationTime;
    char *pMsgId = NULL;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsXMLString (pReader, sMetadata);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the metadata length
    uint32 ui32ReferredObjLen = 0;
    if (pReader->read32 (&ui32ReferredObjLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the MIME type
    if (ui32ReferredObjLen > 0) {
        if (ui32ReferredObjLen > (BUF_LEN-1)) {
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32ReferredObjLen, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        _buf[ui32ReferredObjLen] = '\0';
        sReferredObj = _buf;
    }

    // read the expiration time
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDSPro->addAnnotation (groupName.c_str(), objectId.c_str(),
                                     instanceId.c_str(), sMetadata.c_str(),
                                     sReferredObj.c_str(), i64ExpirationTime,
                                     &pMsgId);

    bool bSuccess = ((rc == 0) && (pMsgId != NULL));

    if (bSuccess) {
        _pCommHelper->receiveMatch (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    // write the msg id of the pushed data
    if (bSuccess) {
        _pCommHelper->sendStringBlock (pMsgId, error);
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (bSuccess) {
        _pCommHelper->receiveMatch (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (pMsgId != NULL) {
        free (pMsgId);
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doGetNodeId (CommHelperError &error)
{
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // write the msg id of the pushed data
    const char *pszNodeId = _pDSPro->getNodeId();
    if (pszNodeId == NULL) {
        return false;
    }
    _pCommHelper->sendStringBlock (pszNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetPathForPeer (CommHelperError &error)
{
    Writer *pWriter = _pCommHelper->getWriterRef();

    char line[512];
    _pCommHelper->receiveLine (line, 512, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    NodeContext *pNodeContext = _pDSPro->_pImpl->_pNodeContextMgr->getPeerNodeContext (line);
    if (pNodeContext == NULL) {
        _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
        checkAndLogMsg ("DSProProxyAdaptor::doGetPathForPeer", Logger::L_Info,
                        "no path for peer %s\n", line);
        _pCommHelper->sendLine (error, "NOPATH");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        return true;
    }

    NodePath *pActualPath = pNodeContext->getPath();
    if (pActualPath == NULL) {
        _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
        checkAndLogMsg ("DSProProxyAdaptor::doGetPathForPeer", Logger::L_Info,
                        "no path for peer %s\n", line);
        _pCommHelper->sendLine (error, "NOPATH");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        return true;
    }

    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
        return false;
    }

    checkAndLogMsg ("DSProProxyAdaptor::doGetPathForPeer", Logger::L_Info,
                    "sending path of length %d for peer %s\n",
                    pActualPath->getPathLength(), line);
    if (pActualPath->write (pWriter) < 0) {
        _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
        error = SimpleCommHelper2::CommError;
        return false;
    }

    _pCommHelper->receiveMatch (error, "OK");
    _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doSessionId (CommHelperError &error)
{
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // write the msg id of the pushed data
    const String sessionId (_pDSPro->getSessionId());
    _pCommHelper->sendStringBlock (sessionId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetPeerList (CommHelperError &error)
{
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == NULL) {
        return false;
    }

    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    char **ppszPeerList = _pDSPro->getPeerList();
    unsigned int uiPeerCounter = 0;
    if (ppszPeerList != NULL) {
        for (unsigned int i = 0; ppszPeerList[i] != NULL; i++) {
            if (strcmp (ppszPeerList[i], "") != 0) {
                uiPeerCounter++;
            }
        }
    }
    if (pWriter->write32 (&uiPeerCounter) != 0) {
        if (ppszPeerList != NULL) {
            for (int i = 0; ppszPeerList[i] != NULL; i++) {
                free (ppszPeerList[i]);
            }
            free (ppszPeerList);
        }
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (uiPeerCounter > 0) {
        for (unsigned int i = 0; ppszPeerList[i] != NULL; i++) {
            unsigned int uiStrLen = strlen (ppszPeerList[i]);
            if (uiStrLen > 0) {
                if ((pWriter->write32 (&uiStrLen) != 0) ||
                    (pWriter->writeBytes (ppszPeerList[i], uiStrLen) != 0)) {
                    // Free the current element and the remaining ones
                    free (ppszPeerList[i]);
                    for (i++; ppszPeerList[i] != NULL; i++) {
                        free (ppszPeerList[i]);
                    }
                    free (ppszPeerList);
                    error = SimpleCommHelper2::CommError;
                    return false;
                }
            }
            free (ppszPeerList[i]);
        }
        free (ppszPeerList);
        ppszPeerList = NULL;
    }
    else if (ppszPeerList !=  NULL) {
        for (int i = 0; ppszPeerList[i] != NULL; i++) {
            free (ppszPeerList[i]);
        }
        free (ppszPeerList);
    }

    error = SimpleCommHelper2::None;
    return true;
}

bool DSProProxyAdaptor::doRequestCustomAreaChunk (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader == NULL) {
        return false;
    }
    uint32 ui32StartXPixel = 0U;
    uint32 ui32EndXPixel = 0U;
    uint32 ui32StartYPixel = 0U;
    uint32 ui32EndYPixel = 0U;
    uint8 ui8CompressionQuality = 0;
    int64 i64Timeout = 0;
    char *pszMsgId = NULL;
    char *pszMIMEType = NULL;
    bool bSuccess = false;
    if ((pReader->readString (&pszMsgId) == 0) &&
        (pReader->readString (&pszMIMEType) == 0) &&
        (pReader->read32 (&ui32StartXPixel) == 0) &&
        (pReader->read32 (&ui32EndXPixel) == 0) &&
        (pReader->read32 (&ui32StartYPixel) == 0) &&
        (pReader->read32 (&ui32EndYPixel) == 0) &&
        (pReader->read8 (&ui8CompressionQuality) == 0) &&
        (pReader->read64 (&i64Timeout) == 0)) {
        if (_pDSPro->requestCustomAreaChunk (pszMsgId, pszMIMEType,
            ui32StartXPixel, ui32EndXPixel, ui32StartYPixel,
            ui32EndYPixel, ui8CompressionQuality, i64Timeout) == 0) {
            bSuccess = true;
        }
    }
    else {
        error = SimpleCommHelper2::CommError;
    }

    if (bSuccess) {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (pszMsgId != NULL) {
        free (pszMsgId);
    }
    if (pszMIMEType != NULL) {
        free (pszMIMEType);
    }
    return bSuccess;
}

bool DSProProxyAdaptor::doRequestCustomTimeChunk (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader == NULL) {
        return false;
    }
    int64 i64StartTime = 0U;
    int64 i64EndTime = 0U;
    uint8 ui8CompressionQuality = 0;
    char *pszMsgId = NULL;
    char *pszMIMEType = NULL;
    int64 i64Timeout = 0;
    bool bSuccess = false;
    if ((pReader->readString (&pszMsgId) == 0) &&
        (pReader->readString (&pszMIMEType) == 0) &&
        (pReader->read64 (&i64StartTime) == 0) &&
        (pReader->read64 (&i64EndTime) == 0) &&
        (pReader->read8 (&ui8CompressionQuality) == 0) &&
        (pReader->read64 (&i64Timeout) == 0)) {
        if (_pDSPro->requestCustomTimeChunk (pszMsgId, pszMIMEType,
                                             i64StartTime, i64EndTime,
                                             ui8CompressionQuality,
                                             i64Timeout) == 0) {
            bSuccess = true;
        }
    }
    else {
        error = SimpleCommHelper2::CommError;
    }

    if (bSuccess) {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    if (pszMsgId != NULL) {
        free (pszMsgId);
    }
    if (pszMIMEType != NULL) {
        free (pszMIMEType);
    }
    return bSuccess;
}

bool DSProProxyAdaptor::doRequestMoreChunks (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    char buf[100];
    String chunkedMsgId;

    if (_pCommHelper->receiveLine (buf, sizeof (buf), error) > 0) {
        chunkedMsgId = buf;
    }
    else if (error == SimpleCommHelper2::None) {
        error = SimpleCommHelper2::ProtocolError;
    }
    else {
        return false;
    }

    char *pszSearchId = NULL;
    if (_pCommHelper->getReaderRef ()->readString (&pszSearchId) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    if (_pDSPro->requestMoreChunks (chunkedMsgId, pszSearchId) < 0) {
        return false;
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doRegisterPathRegisteredCallback()
{
    if (_bListenerProRegistered) {
        return true;
    }

    _ui16PathRegisteredCbackClientId = _ui16ClientID;
    _pDSPro->registerDSProListener (_ui16ClientID, this, _ui16PathRegisteredCbackClientId);
    _bListenerProRegistered = true;

    return true;
}

bool DSProProxyAdaptor::doRegisterCtrlMsgCallback()
{
    if (_bCtrlMsgListenerRegistered) {
        return true;
    }
    if (pCtrlMsgNotifier == NULL) {
        return false;
    }

    _ui16CtrlMsgCbackClientId = _ui16ClientID;
    pCtrlMsgNotifier->registerAndEnableControllerMessageListener (_ui16ClientID, this, _ui16CtrlMsgCbackClientId);
    _bCtrlMsgListenerRegistered = true;

    return true;
}

bool DSProProxyAdaptor::doRegisterMatchmakingLogCallback()
{
    if (_bMatchmakingLogListenerRegistered) {
        return true;
    }
    if (pInstrumentator == NULL) {
        return false;
    }

    _ui16MatchmakingCbackClientId = _ui16ClientID;
    int rc = pInstrumentator->registerAndEnableMatchmakingLogListener (_ui16ClientID, this, _ui16MatchmakingCbackClientId);
    _bMatchmakingLogListenerRegistered = true;

    return (rc == 0);
}

bool DSProProxyAdaptor::doRegisterSearchCallback()
{
    if (_bSearchListenerRegistered) {
        return true;
    }

    _ui16SearchListenerClientID = _ui16ClientID;
    int rc = _pDSPro->registerSearchListener (_ui16ClientID, this, _ui16SearchListenerClientID);
    _bSearchListenerRegistered = true;
    return (rc == 0);
}

bool DSProProxyAdaptor::doDeregisterSearchCallback()
{
    if (_bSearchListenerRegistered) {
        int rc = _pDSPro->deregisterSearchListener (_ui16SearchListenerClientID, this);
        return rc == 0;
    }
    return true;
}

bool DSProProxyAdaptor::doResetTransmissionCounters (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == NULL) {
        return false;
    }

    _pDSPro->resetTransmissionCounters();
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetDSProIds (CommHelperError &error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == NULL) {
        return false;
    }

    uint32 ui32;
    String objectId;
    String instanceId;
    char **ppszIds = NULL;
    

    error = readObjectIdInstanceId (objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    ppszIds = _pDSPro->getDSProIds ((objectId.length() > 0 ? objectId.c_str() : NULL),
                                    (instanceId.length() > 0 ? instanceId.c_str() : NULL));

    bool bSuccess = true;
    _pCommHelper->sendLine (error, "OK");
    if (error == SimpleCommHelper2::None) {
        if (ppszIds != NULL) {
            for (unsigned int i = 0; ppszIds[i] != NULL; i++) {
                _pCommHelper->sendStringBlock (ppszIds[i], error);
                if (error != SimpleCommHelper2::None) {
                    bSuccess = false;
                    break;
                }
            }
        }
        if (bSuccess) {
            ui32 = 0; // write 0 to terminate ID list
            if (pWriter->write32 (&ui32) != 0) {
                error = SimpleCommHelper2::CommError;
                bSuccess = false;
            }
        }
        if (bSuccess) {
            _pCommHelper->sendLine (error, "OK");
            if (error != SimpleCommHelper2::None) {
                bSuccess = false;
            }
        }

    }
    else {
        bSuccess = false;
    }

    if (ppszIds != NULL) {
        for (unsigned int i = 0; ppszIds[i] != NULL; i++) {
            free (ppszIds[i]);
        }
        free (ppszIds);
    }

    return bSuccess;
}

int DSProProxyAdaptor::dataArrived (const char *pszId, const char *pszGroupName,
                                    const char *pszObjectId, const char *pszInstanceId,
                                    const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                    const void *pBuf, uint32 ui32Len, uint8 ui8NChunks,
                                    uint8 ui8TotNChunks, const char *pszQueryId)
{
    if (pszId == NULL || pBuf == NULL || ui32Len == 0) {
        return -1;
    }
    if (_pCallbackCommHelper == NULL) {
        return -2;
    }

    CommHelperError error = SimpleCommHelper2::None;
    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    _pCallbackCommHelper->sendLine (error, "dataArrivedCallback");
    if (error != SimpleCommHelper2::None) {
        return -3;
    }
    _pCallbackCommHelper->sendStringBlock (pszId, error);
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    _pCallbackCommHelper->sendStringBlock (pszGroupName, error);
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    _pCallbackCommHelper->sendStringBlock (pszObjectId, error);
    if (error != SimpleCommHelper2::None) {
        return -6;
    }
    _pCallbackCommHelper->sendStringBlock (pszInstanceId, error);
    if (error != SimpleCommHelper2::None) {
        return -7;
    }
    _pCallbackCommHelper->sendStringBlock (pszAnnotatedObjMsgId, error);
    if (error != SimpleCommHelper2::None) {
        return -8;
    }
    _pCallbackCommHelper->sendStringBlock (pszMimeType, error);
    if (error != SimpleCommHelper2::None) {
        return -9;
    }

    if (pWriter->write32 (&ui32Len) != 0) {
        return -10;
    }

    //send the data.
    _pCallbackCommHelper->sendBlob (pBuf, ui32Len, error);
    if (error != SimpleCommHelper2::None) {
        return -11;
    }

    //send the ui16HistoryWindow
    if (pWriter->write8 (&ui8NChunks) != 0) {
        return -12;
    }

    //send the ui16Tag
    if (pWriter->write8 (&ui8TotNChunks) != 0) {
        return -13;
    }

    _pCallbackCommHelper->sendStringBlock (pszQueryId, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }


    _pCallbackCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -15;
    }

    return 0;
}

int DSProProxyAdaptor::metadataArrived (const char *pszId, const char *pszGroupName,
                                        const char *pszObjectId, const char *pszInstanceId,
                                        const char *pszXMLMetadada, const char *pszReferredDataId,
                                        const char *pszQueryId)
{
    if (pszId == NULL || pszXMLMetadada == NULL || pszReferredDataId == NULL) {
        return -1;
    }
    uint32 ui32MetadataLen = strlen (pszXMLMetadada);
    if (ui32MetadataLen == 0) {
        return -2;
    }

    if (_pCallbackCommHelper == NULL) {
        return -3;
    }

    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "metadataArrivedCallback");
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszId, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszGroupName, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszObjectId, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszInstanceId, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszXMLMetadada, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszReferredDataId, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszQueryId, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }

    _pCallbackCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -14;
    }

    return 0;
}

void DSProProxyAdaptor::newPeer (const char *pszPeerNodeId)
{
    if (pszPeerNodeId == NULL || _pCallbackCommHelper == NULL) {
        return;
    }
    uint32 ui32PeerIdLen = strlen (pszPeerNodeId);
    if (ui32PeerIdLen == 0) {
        return;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "newPeerCallback");
    if (error != SimpleCommHelper2::None) {
        return;
    }

    if (pWriter->write32 (&ui32PeerIdLen) != 0) {
        return;
    }
    _pCallbackCommHelper->sendBlob (pszPeerNodeId, ui32PeerIdLen, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }

    _pCallbackCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return;
    }
    
}

void DSProProxyAdaptor::deadPeer (const char *pszPeerNodeId)
{
    if (pszPeerNodeId == NULL || _pCallbackCommHelper == NULL) {
        return;
    }
    uint32 ui32PeerIdLen = strlen (pszPeerNodeId);
    if (ui32PeerIdLen == 0) {
        return;
    }

    if (_pCallbackCommHelper == NULL) {
        return;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "deadPeerCallback");
    if (pWriter->write32 (&ui32PeerIdLen) != 0) {
        return;
    }
    _pCallbackCommHelper->sendBlob (pszPeerNodeId, ui32PeerIdLen, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }

    _pCallbackCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return;
    }
}

bool DSProProxyAdaptor::pathRegistered (NodePath *pNodePath, const char *pszNodeId, const char *pszTeam, const char *pszMission)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "pathRegisteredCallback");
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    if (pNodePath->write (pWriter) < 0) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszTeam, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszMission, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::positionUpdated (float latitude, float longitude, float altitude, const char *pszNodeId)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    if (pWriter == NULL) {
        return false;
    }

    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "positionUpdatedCallback");
    if ((pWriter->write32 (&latitude) != 0) || 
        (pWriter->write32 (&longitude)) ||
        (pWriter->write32 (&altitude))) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::informationMatched (const char *pszLocalNodeId, const char *pszPeerNodeId, const char *pszMatchedObjectId,
                                            const char *pszMatchedObjectName, const char **ppszRankDescriptors,
                                            float *pRanks, float *pWeights, uint8 ui8Len, const char *pszComment, const char *pszOperation)
{
    return matchmakingLogListenerCallback (pszLocalNodeId, pszPeerNodeId, pszMatchedObjectId,
                                           pszMatchedObjectName, ppszRankDescriptors, pRanks,
                                           pWeights, ui8Len, pszComment, false, pszOperation);
}

bool DSProProxyAdaptor::informationSkipped (const char *pszLocalNodeId, const char *pszPeerNodeId, const char *pszSkippedObjectId,
                                            const char *pszSkippedObjectName, const char **ppszRankDescriptors, float *pRanks, float *pWeights,
                                            uint8 ui8Len, const char *pszComment, const char *pszOperation)
{
    return matchmakingLogListenerCallback (pszLocalNodeId, pszPeerNodeId, pszSkippedObjectId,
                                           pszSkippedObjectName, ppszRankDescriptors, pRanks,
                                           pWeights, ui8Len, pszComment, true, pszOperation);
}

bool DSProProxyAdaptor::contextUpdateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("contextUpdateMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::contextVersionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("contextVersionMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::messageRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("messageRequestMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::chunkRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("chunkRequestMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::positionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)

{
    return ctrlMsgArrived ("positionMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::searchMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("searchMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::topologyReplyMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("topologyReplyMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::topologyRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("topologyRequestMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::updateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("updateMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::versionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("versionMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::waypointMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("waypointMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

bool DSProProxyAdaptor::wholeMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    return ctrlMsgArrived ("wholeMessageArrivedCallback", pszSenderNodeId, pszPublisherNodeId);
}

void DSProProxyAdaptor::searchArrived (const char *pszQueryId, const char *pszGroupName,
                                       const char *pszQuerier, const char *pszQueryType, const char *pszQueryQualifiers,
                                       const void *pQuery, unsigned int uiQueryLen)
{
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "searchArrivedCallback");
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszQueryId, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszGroupName, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszQuerier, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszQueryType, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszQueryQualifiers, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    if (_pCallbackCommHelper->getWriterRef()->write32 (&uiQueryLen) != 0) {
        return;
    }
    if (uiQueryLen > 0) {
        _pCallbackCommHelper->sendBlob (pQuery, uiQueryLen, error);
    }
}

void DSProProxyAdaptor::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId)
{
    if ((pszQueryId == NULL) || (ppszMatchingMessageIds == NULL) || (ppszMatchingMessageIds[0] == NULL) || (pszMatchingNodeId == NULL)) {
        return;
    }
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "searchReplyArrivedCallback");
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszQueryId, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    for (unsigned int i = 0; ppszMatchingMessageIds[i] != NULL; i++) {
        _pCallbackCommHelper->sendStringBlock (ppszMatchingMessageIds[i], error);
        if (error != SimpleCommHelper2::None) {
            return;
        }
    }
    uint32 ui32Len = 0U;
    if (_pCallbackCommHelper->getWriterRef()->write32 (&ui32Len) < 0) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszMatchingNodeId, error);
}

void DSProProxyAdaptor::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen, const char *pszMatchingNodeId)
{
    if ((pszQueryId == NULL) || (pReply == NULL) || (ui162ReplyLen == 0)) {
        return;
    }
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "volatileSearchReplyArrivedCallback");
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszQueryId, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendBlock (pReply, ui162ReplyLen, error);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCallbackCommHelper->sendStringBlock (pszMatchingNodeId, error);
}

bool DSProProxyAdaptor::ctrlMsgArrived (const char *pszType, const char *pszSenderNodeId, const char *pszPublisherNodeId)
{
    if (pszType == NULL || pszSenderNodeId == NULL || pszPublisherNodeId == NULL) {
        return false;
    }

    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, pszType);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszSenderNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszPublisherNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetDisService (CommHelperError &error)
{
    error = SimpleCommHelper2::None;

    // Read port
    int32 i32 = 0;
    if (_pCommHelper->getReaderRef()->read32 (&i32) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (i32 < 0) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }
    const uint16 ui16Port = (uint16) i32;

    // Read IPv4 address
    i32 = 0;
    if (_pCommHelper->getReaderRef()->read32 (&i32) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (i32 <= 0 || i32 > 16) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }
    char address[17];
    const uint16 ui16AddressLen = (uint16) i32;
    if (_pCommHelper->getReaderRef()->readBytes (address, ui16AddressLen) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    address[ui16AddressLen] = '\0';

    if (_pDisSvcProProxyServer->initDisseminationServiceProxyServer (_pDSPro->getDisService(), address, ui16Port) != 0) {
        return false;
    }
    
    _pCommHelper->sendLine (error, "OK");
    return (error == SimpleCommHelper2::None);
}

bool DSProProxyAdaptor::matchmakingLogListenerCallback (const char *pszLocalNodeId, const char *pszPeerNodeId, const char *pszObjectId,
                                                        const char *pszObjectName, const char **ppszRankDescriptors, float *pRanks, float *pWeights,
                                                        uint8 ui8Len, const char *pszComment, bool bSkipped, const char *pszOperation)
{
    if (_pCallbackCommHelper == NULL) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    CommHelperError error = SimpleCommHelper2::None;

    _pCallbackCommHelper->sendLine (error, bSkipped ? "informationSkippedCallback" : "informationMatchedCallback");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->sendStringBlock (pszLocalNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszPeerNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszObjectId, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszObjectName, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    pWriter->write8 (&ui8Len);
    for (int i = 0; i < ui8Len; i++) {
        if (ppszRankDescriptors[i] != NULL) {
            _pCallbackCommHelper->sendStringBlock (ppszRankDescriptors[i], error);
            if (error != SimpleCommHelper2::None) {
                return false;
            }
        }
        if ((pWriter->write32 (&pRanks[i]) != 0) ||
            (pWriter->write32 (&pWeights[i]) != 0)) {
            return false;
        }
    }

    _pCallbackCommHelper->sendStringBlock (pszComment, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }
    _pCallbackCommHelper->sendStringBlock (pszOperation, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    _pCallbackCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

CommHelperError DSProProxyAdaptor::readGroupObjectIdInstanceId (String &groupName, String &objectId, String &instanceId)
{
    CommHelperError error = SimpleCommHelper2::None;

    // read the group name
    if (_pCommHelper->receiveLine (_buf, BUF_LEN, error) > 0) {
        groupName = _buf;
    }
    else if (error == SimpleCommHelper2::None) {
        return SimpleCommHelper2::ProtocolError;
    }
    else {
        return error;
    }

    return readObjectIdInstanceId (objectId, instanceId);
}

CommHelperError DSProProxyAdaptor::readObjectIdInstanceId (String &objectId, String &instanceId)
{
    CommHelperError error = SimpleCommHelper2::None;
    uint32 ui32;

    // Read objectId
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
    objectId = _buf;

    // Read instanceId
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
    instanceId = _buf;

    return SimpleCommHelper2::None;
}

CommHelperError DSProProxyAdaptor::readMetadataAsAvList (Reader *pReader, SQLAVList &aVList, uint32 ui32Attributes)
{
    uint32 ui32;
    for (unsigned int i = 0; i < ui32Attributes; i++) {
        if (pReader->read32 (&ui32) != 0) {
            return SimpleCommHelper2::CommError;
        }
        char *pszAttribute = (char *) calloc (ui32+1, sizeof (char));
        if (pszAttribute == NULL) {
            return SimpleCommHelper2::None;
        }
        if (pReader->readBytes (pszAttribute, ui32) != 0) {
            free (pszAttribute);
            return SimpleCommHelper2::CommError;
        }
        pszAttribute[ui32] = '\0';

        if (pReader->read32 (&ui32) != 0) {
            free (pszAttribute);
            return SimpleCommHelper2::CommError;
        }
        char *pszValue = (char *) calloc (ui32+1, sizeof (char));
        if (pszValue == NULL) {
            free (pszAttribute);
            return SimpleCommHelper2::None;
        }
        if (pReader->readBytes (pszValue, ui32) != 0) {
            free (pszAttribute);
            if (pszValue != NULL) {
                free (pszValue);
            }
            return SimpleCommHelper2::CommError;
        }
        pszValue[ui32] = '\0';

        if (aVList.addPair (pszAttribute, pszValue) != 0) {
            free (pszAttribute);
            free (pszValue);
            return SimpleCommHelper2::None;
        }

        free (pszAttribute);  // AVList makes a copy of both
        free (pszValue);      // attribute and value
    }

    return SimpleCommHelper2::None;
}

CommHelperError DSProProxyAdaptor::readMetadataAsXMLString (NOMADSUtil::Reader *pReader, NOMADSUtil::String &sMetadata)
{
    uint32 ui32MetadataLen = 0;

    // read the metadata length
    if (pReader->read32 (&ui32MetadataLen) != 0) {
        return SimpleCommHelper2::CommError;
    }

    // instantiate buffer for metadata
    char *pszXMLMetadata = (char *) calloc (ui32MetadataLen + 1, sizeof (char));
    if (pszXMLMetadata == NULL) {
        return SimpleCommHelper2::None;
    }

    // read the metadata
    CommHelperError error = SimpleCommHelper2::None;
    _pCommHelper->receiveBlob (pszXMLMetadata, ui32MetadataLen, error);
    if (error != SimpleCommHelper2::None) {
        return error;
    }

    pszXMLMetadata[ui32MetadataLen] = '\0';
    sMetadata = pszXMLMetadata;

    return SimpleCommHelper2::None;
}

