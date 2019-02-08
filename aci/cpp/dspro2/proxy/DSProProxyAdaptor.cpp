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
#include "DSProProxyUnmarshaller.h"
#include "DSProProxyServer.h"

#include "Instrumentator.h"
#include "NodePath.h"
#include "NodeContextManager.h"
#include "PeerNodeContext.h"

#include "AVList.h"
#include "BufferReader.h"
#include "Json.h"
#include "Logger.h"
#include "Writer.h"
#include "ConfigManager.h"
#include "ControlMessageNotifier.h"

#include <string.h>
#include "MimeUtils.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

namespace DSPRO_PROXY_ADAPTOR
{
    void deallocateChunks (DSPro::Chunk **ppChunks, uint8 ui8NChunks)
    {
        if (ppChunks == nullptr) {
            return;
        }
        for (uint8 i = 0; i < ui8NChunks; i++) {
            if (ppChunks[i] != nullptr) {
                if (ppChunks[i]->pChunkData != nullptr) {
                    free (ppChunks[i]->pChunkData);
                }
                free (ppChunks[i]);
            }
        }
        free (ppChunks);
    }

    void read (String &id, uint32 &ui32, SimpleCommHelper2 *pCommHelper, CommHelperError & error)
    {
        Reader *pReader = pCommHelper->getReaderRef();
        char *psztmp = nullptr;
        if (pReader->readString (&psztmp) < 0) {
            error = SimpleCommHelper2::CommError;
            return;
        }
        if (psztmp == nullptr) {
            error = SimpleCommHelper2::ProtocolError;
            return;
        }
        id = psztmp;  // String makes a copy
        free (psztmp);

        if (pReader->read32 (&ui32) < 0) {
            error = SimpleCommHelper2::CommError;
            return;
        }
    }

    CommHelperError readMetadataAsAvList (Reader *pReader, AVList &aVList, uint32 ui32Attributes)
    {
        uint32 ui32;
        for (unsigned int i = 0; i < ui32Attributes; i++) {
            if (pReader->read32 (&ui32) != 0) {
                return SimpleCommHelper2::CommError;
            }
            char *pszAttribute = (char *)calloc (ui32 + 1, sizeof (char));
            if (pszAttribute == nullptr) {
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
            char *pszValue = (char *)calloc (ui32 + 1, sizeof (char));
            if (pszValue == nullptr) {
                free (pszAttribute);
                return SimpleCommHelper2::None;
            }
            if ((ui32 > 0) && (pReader->readBytes (pszValue, ui32) != 0)) {
                free (pszAttribute);
                if (pszValue != nullptr) {
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
}

#define MostGeneralException std::exception
#define checkAndLogMsg if (pLogger) pLogger->logMsg

DSProProxyAdaptor::DSProProxyAdaptor (DSProProxyServer *pDSPProxyServer)
    : SearchListener ("DSProProxyAdaptor")
{
    // _pDisSvcPro and _pCallbackCommHelper are initialized in the parent's constructor
    _pDSPro = pDSPProxyServer->getDisServiceProRef();
    _pDisSvcProProxyServer = pDSPProxyServer;
    _pCommHelper = nullptr;
    _pCallbackCommHelper = nullptr;
    _ui16ClientID = 0;
    _bListenerProRegistered = false;
    _bMatchmakingLogListenerRegistered = false;
    _bCtrlMsgListenerRegistered = false;
    _bSearchListenerRegistered = false;
}

DSProProxyAdaptor::~DSProProxyAdaptor (void)
{
    if (_bListenerProRegistered) {
        _pDSPro->deregisterDSProListener (_ui16PathRegisteredCbackClientId, this);
    }
    if (_bMatchmakingLogListenerRegistered && pInstrumentator != nullptr) {
        pInstrumentator->deregisterAndDisableMatchmakingLogListener (_ui16MatchmakingCbackClientId);
    }
    if (_bCtrlMsgListenerRegistered && pCtrlMsgNotifier != nullptr) {
        pCtrlMsgNotifier->deregisterAndDisableControllerMessageListener (_ui16CtrlMsgCbackClientId);
    }
    if (_bSearchListenerRegistered) {
        _pDSPro->deregisterSearchListener (_ui16SearchListenerClientID, this);
    }
    if (_bSearchListenerRegistered) {
        _pDSPro->deregisterSearchListener (_ui16SearchListenerClientID, this);
    }

    _mChunkers.lock();
    for (StringHashset::Iterator iter = _registeredChunkers.getAllElements();
        !iter.end(); iter.nextElement()) {
        _pDSPro->deregisterChunkFragmenter (iter.getKey());
    }
    _mChunkers.unlock();

    _mReassemblers.lock();
    for (StringHashset::Iterator iter = _registeredReassemblers.getAllElements();
        !iter.end(); iter.nextElement()) {
        _pDSPro->deregisterChunkReassembler (iter.getKey());
    }
    _mReassemblers.unlock();

    char szId[10];
    snprintf (szId, sizeof (szId)-1, "%d", _ui16ClientID);
    _pDisSvcProProxyServer->_proxies.remove (szId);

    // Close connections
    if (_pCommHelper != nullptr) {
        CommHelperError error;
        _pCommHelper->closeConnection (error);
        if (error != SimpleCommHelper2::None) {
            delete _pCommHelper;
            _pCommHelper = nullptr;
        }
    }
    if (_pCallbackCommHelper != nullptr) {
        CommHelperError error;
        _pCallbackCommHelper->closeConnection (error);
        if (error != SimpleCommHelper2::None) {
            delete _pCallbackCommHelper;
            _pCallbackCommHelper = nullptr;
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

void DSProProxyAdaptor::run (void)
{
    started();

    const char *pszMethodName = "DSProProxyAdaptor::run";
    setName (pszMethodName);

    int size;
    char buf[128];
    CommHelperError error = SimpleCommHelper2::None;
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
        if (strcmp (buf, DSProProxyUnmarshaller::ADD_USER_ID) == 0) {
            bSuccess = doAddUserId (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_MISSION_ID) == 0) {
            bSuccess = doSetMissionId (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_ROLE) == 0) {
            bSuccess = doSetRole (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_TEAM_ID) == 0) {
            bSuccess = doSetTeam (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_NODE_TYPE) == 0) {
            bSuccess = doSetNodeType (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_SELECTIVITY) == 0) {
            bSuccess = doSetSelectivity (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_RANGE_OF_INFLUENCE) == 0) {
            bSuccess = doSetRangeOfInluence (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_DEFAULT_USEFUL_DISTANCE) == 0) {
            bSuccess = doSetDefUsefulDistance (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_USEFUL_DISTANCE) == 0) {
            bSuccess = doSetUsefulDistance (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_RANKING_WEIGHTS) == 0) {
            bSuccess = doSetRankingWeights (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::CHANGE_ENCRYPTION_KEY) == 0) {
            bSuccess = doChangeEncryptionKey (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_CUSTUMS_POLICIES_AS_XML) == 0) {
            bSuccess = doAddCustumPoliciesAsXML (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::CANCEL) == 0) {
            bSuccess = doCancel (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::CANCEL_BY_OBJECT_INSTANCE_ID) == 0) {
            bSuccess = doCancelByObjectAndInstanceId (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::DISSEMINATE) == 0) {
            bSuccess = doDisseminate (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::DISSEMINATE_METADATA) == 0) {
            bSuccess = doDisseminateMetadata (error);
        }
        else if (strcmp (buf, "configureProperties") == 0) {
            bSuccess = doConfigureProperties (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::REGISTER_PATH) == 0) {
            bSuccess = doRegisterPath (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_CURRENT_PATH) == 0) {
            bSuccess = doSetCurrentPath (error);
        }
        else if (strcmp (buf, "getCurrentPath") == 0) {
            bSuccess = doGetCurrentPath (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::SET_CURRENT_POSITION) == 0) {
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
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_MESSAGE) == 0) {
            bSuccess = doAddMessage (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_MESSAGE_AV_LIST) == 0) {
            bSuccess = doAddMessage_AVList (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_CHUNK) == 0) {
            bSuccess = doAddChunk (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_CHUNK_AV_LIST) == 0) {
            bSuccess = doAddChunk_AVList (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_ADDITIONAL_CHUNK) == 0) {
            bSuccess = doAddAdditionalChunk (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_CHUNKED_MESSAGE) == 0) {
            bSuccess = doAddChunkedMessage (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::ADD_CHUNKED_MESSAGE_AV_LIST) == 0) {
            bSuccess = doAddChunkedMessage_AVList (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE) == 0) {
            bSuccess = doChunkAndAddMessage (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE_AV_LIST) == 0) {
            bSuccess = doChunkAndAddMessage_AVList (error);
        }
        else if (strcmp (buf, "addAnnotation") == 0) {
            bSuccess = doAddAnnotation (error);
        }
        else if (strcmp (buf, "addAnnotationRef") == 0) {
            bSuccess = doAddAnnotationRef (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::GET_DATA) == 0) {
            bSuccess = doGetData (error);
        }
        else if (strcmp (buf, "getMatchingMetadata") == 0) {
            bSuccess = doGetMatchingMetaDataAsJson (error);
        }
        else if (strcmp (buf, "getDisService") == 0) {
            bSuccess = doGetDisService (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::GET_NODE_CONTEXT) == 0) {
            bSuccess = doGetNodeContext (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::GET_NODE_ID) == 0) {
            bSuccess = doGetNodeId (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::GET_PEER_MSG_COUNTS) == 0) {
            bSuccess = doGetPeerMsgCounts (error);
        }
        else if (strcmp (buf, "getPathForPeer") == 0) {
            bSuccess = doGetPathForPeer (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::GET_SESSION_ID) == 0) {
            bSuccess = doGetSessionId (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::REGISTER_DSPRO_LISTENER_METHOD) == 0) {
            bSuccess = doRegisterPathRegisteredCallback();
        }
        else if (strcmp (buf, "registerCtrlMsgCallback") == 0) {
            bSuccess = doRegisterCtrlMsgCallback();
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::REGISTER_MATCHMAKING_LISTENER_METHOD) == 0) {
            bSuccess = doRegisterMatchmakingLogCallback();
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::REGISTER_SEARCH_LISTENER_METHOD) == 0) {
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
        else if (strcmp (buf, DSProProxyUnmarshaller::RESET_TRANSMISSION_COUNTERS) == 0) {
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
        else if (strcmp (buf, DSProProxyUnmarshaller::SUBSCRIBE) == 0) {
            bSuccess = doSubscribe (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::REGISTER_CHUNK_FRAGMENTER) == 0) {
            bSuccess = doRegisterChunkFragmenter (error);
        }
        else if (strcmp (buf, DSProProxyUnmarshaller::REGISTER_CHUNK_REASSEMBLER) == 0) {
            bSuccess = doRegisterChunkReassembler (error);
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
        if (bSuccess) {
            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "Command <%s> worked.\n", buf);
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

    terminating();

    // the destructor will take care of closing all the connections
    // and de-registering this Adaptor from the DisseminationServiceProxyServer.
    delete this;
}

bool DSProProxyAdaptor::doAddUserId (CommHelperError & error)
{
    error = SimpleCommHelper2::None;

    char *pszUserId = nullptr;
    if (_pCommHelper->getReaderRef()->readString (&pszUserId) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pszUserId == nullptr) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    int rc = _pDSPro->addUserId (pszUserId);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetSelectivity (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    float fSelectivity = 0.0f;
    if (_pCommHelper->getReaderRef()->read32 (&fSelectivity) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if ((fSelectivity < 0.0f) || (fSelectivity > 10.0f)) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    int rc = _pDSPro->setSelectivity (fSelectivity);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetRangeOfInluence (CommHelperError & error)
{
    error = SimpleCommHelper2::None;

    String milStd2525Symbol;
    uint32 ui32Range = 0U;
    DSPRO_PROXY_ADAPTOR::read (milStd2525Symbol, ui32Range, _pCommHelper, error);

    int rc = _pDSPro->setRangeOfInfluence (milStd2525Symbol, ui32Range);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetDefUsefulDistance (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    uint32 ui32Distance = 0U;
    if (pReader->read32 (&ui32Distance) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDSPro->setDefaultUsefulDistance (ui32Distance);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetUsefulDistance (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    String mimeType;
    uint32 ui32Distance = 0U;
    DSPRO_PROXY_ADAPTOR::read (mimeType, ui32Distance, _pCommHelper, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    int rc = _pDSPro->setUsefulDistance (mimeType, ui32Distance);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetRankingWeights (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    float coordRankWeight, timeRankWeight, expirationRankWeight,
        impRankWeight, sourceReliabilityRankWeigth, informationContentRankWeigth, predRankWeight, targetWeight;
    coordRankWeight = timeRankWeight = expirationRankWeight = impRankWeight =
        sourceReliabilityRankWeigth = informationContentRankWeigth = predRankWeight = targetWeight = 0.0f;
    bool bStrictTarget, bConsiderFuturePathSegmentForMatchmacking;
    bStrictTarget = true;
    bConsiderFuturePathSegmentForMatchmacking = false;

    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readFloat (&coordRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readFloat (&timeRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readFloat (&expirationRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readFloat (&impRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readFloat (&sourceReliabilityRankWeigth) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readFloat (&informationContentRankWeigth) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readFloat (&predRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readFloat (&targetWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readBool (&bStrictTarget) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readBool (&bConsiderFuturePathSegmentForMatchmacking) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDSPro->setRankingWeigths (coordRankWeight, timeRankWeight, expirationRankWeight,
                                         impRankWeight, sourceReliabilityRankWeigth,
                                         informationContentRankWeigth, predRankWeight, targetWeight,
                                         bStrictTarget, bConsiderFuturePathSegmentForMatchmacking);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetMissionId (CommHelperError & error)
{
    error = SimpleCommHelper2::None;

    char *pszMissionId = nullptr;
    if (_pCommHelper->getReaderRef()->readString (&pszMissionId) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pszMissionId == nullptr) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    int rc = _pDSPro->setMissionId (pszMissionId);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetRole (CommHelperError & error)
{
    error = SimpleCommHelper2::None;

    char *pszRole = nullptr;
    if (_pCommHelper->getReaderRef ()->readString (&pszRole) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pszRole == nullptr) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    int rc = _pDSPro->setRole (pszRole);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetTeam (CommHelperError & error)
{
    error = SimpleCommHelper2::None;

    char *pszTeamId = nullptr;
    if (_pCommHelper->getReaderRef()->readString (&pszTeamId) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pszTeamId == nullptr) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    int rc = _pDSPro->setTeamId (pszTeamId);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doSetNodeType (CommHelperError & error)
{
    error = SimpleCommHelper2::None;

    char *pszNodeType = nullptr;
    if (_pCommHelper->getReaderRef()->readString (&pszNodeType) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pszNodeType == nullptr) {
        error = SimpleCommHelper2::ProtocolError;
        return false;
    }

    int rc = _pDSPro->setNodeType (pszNodeType);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    return (rc == 0);
}

bool DSProProxyAdaptor::doAddCustumPoliciesAsXML (CommHelperError & error)
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
    char **ppszXMLDocs = static_cast<char **>(calloc (ui16Count + 1, sizeof (char*)));
    if (ppszXMLDocs == nullptr) {
        return false;
    }
    ppszXMLDocs[ui16Count] = nullptr;
    for (uint16 i = 0; i < ui16Count; i++) {
        char *pszXMLDoc = nullptr;
        if (_pCommHelper->getReaderRef()->readString (&pszXMLDoc) < 0) {
            error = SimpleCommHelper2::CommError;
            bSuccess = false;
            break;
        }
        if (pszXMLDoc == nullptr) {
            error = SimpleCommHelper2::ProtocolError;
            bSuccess = false;
            break;
        }
        ppszXMLDocs[i] = pszXMLDoc;
    }
    int rc = 0;
    if (bSuccess) {
        rc = _pDSPro->addCustumPoliciesAsXML ((const char **)ppszXMLDocs);
        if (rc == 0) {
            _pCommHelper->sendLine (error, "OK");
        }
    }

    // Deallocate XML docs
    for (uint16 j = 0; j < ui16Count; j++) {
        if (ppszXMLDocs[j] != nullptr) {
            free (ppszXMLDocs[j]);
        }
    }
    free (ppszXMLDocs);

    return ((error == SimpleCommHelper2::None) && (rc == 0));
}

bool DSProProxyAdaptor::doCancel (CommHelperError & error)
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

bool DSProProxyAdaptor::doCancelByObjectAndInstanceId (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    char *pszObjectId = nullptr;
    char *pszInstanceId = nullptr;

    if (_pCommHelper->getReaderRef()->readString (&pszObjectId) < 0) {
         error = SimpleCommHelper2::CommError;
         return false;
    }
    if (_pCommHelper->getReaderRef()->readString (&pszInstanceId) < 0) {
         error = SimpleCommHelper2::CommError;
         if (pszObjectId) free (pszObjectId);
         return false;
    }

    int rc = _pDSPro->cancelByObjectAndInstanceId (pszObjectId, pszInstanceId);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    else {
        _pCommHelper->sendLine (error, "ERROR %d", rc);
    }

    if (pszObjectId) free (pszObjectId);
    if (pszInstanceId) free (pszInstanceId);

    return (error == SimpleCommHelper2::None);
}

bool DSProProxyAdaptor::doChangeEncryptionKey (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    uint32 ui32KeyLen = 0;
    unsigned char *pKey = nullptr;
    if (pReader->read32 (&ui32KeyLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the data
    if (ui32KeyLen > 0) {
        pKey = (unsigned char *) malloc (ui32KeyLen);
        if (pKey == nullptr) {
            checkAndLogMsg ("DSProProxyAdaptor::doChangeEncryptionKey", memoryExhausted);
        }
        _pCommHelper->receiveBlob (pKey, ui32KeyLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pKey);
            return false;
        }
    }

    int rc = _pDSPro->changeEncryptionKey (pKey, ui32KeyLen);
    bool bSuccess = (rc == 0);
    free (pKey);

    if (bSuccess) {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            bSuccess = false;
        }
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doDisseminate (CommHelperError & error)
{
    return doDisseminateMetadata (error, false);
}

bool DSProProxyAdaptor::doDisseminateMetadata (CommHelperError & error, bool bReadMimeTypeAndMetadata)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String instanceId;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    uint32 ui32MetadataLen = 0;
    void *pMetadata = nullptr;
    if (bReadMimeTypeAndMetadata) {
        // read the metadata length
        if (pReader->read32 (&ui32MetadataLen) != 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        if (ui32MetadataLen == 0) {
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }

        // read the metadata
        if (ui32MetadataLen > 0) {
            pMetadata = malloc (ui32MetadataLen);
            if (pMetadata == nullptr) {
                checkAndLogMsg ("DSProProxyAdaptor::doDisseminateMetadata", memoryExhausted);
                error = SimpleCommHelper2::ProtocolError;
                return false;
            }
            else {
                _pCommHelper->receiveBlob (pMetadata, ui32MetadataLen, error);
                if (error != SimpleCommHelper2::None) {
                    free (pMetadata);
                    return false;
                }
            }
        }
    }

    // read the data length
    uint32 ui32DataLen = 0;
    void *pData = nullptr;
    if (pReader->read32 (&ui32DataLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the data
    if (ui32DataLen > 0) {
        pData = malloc (ui32DataLen);
        if (pData == nullptr) {
            checkAndLogMsg ("DSProProxyAdaptor::doDisseminateMetadata", memoryExhausted);
        }
        _pCommHelper->receiveBlob (pData, ui32DataLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pData);
            return false;
        }
    }

    char *pszMimeType = nullptr;
    if (bReadMimeTypeAndMetadata && (pReader->readString (&pszMimeType) < 0)) {
        if (pData != nullptr) free (pData);
        if (pMetadata != nullptr) free (pMetadata);
    }

    // read the expiration time
    int64 i64ExpirationTime = 0;
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pData != nullptr) free (pData);
        if (pMetadata != nullptr) free (pMetadata);
        if (pszMimeType != nullptr) free (pszMimeType);
        return false;
    }

    char *pMsgId = nullptr;
    int rc = 0;
    if (bReadMimeTypeAndMetadata) {
        rc = _pDSPro->disseminatedMessageMetadata (groupName, objectId, instanceId,
                                                   pMetadata, ui32MetadataLen,
                                                   pData, ui32DataLen, pszMimeType, i64ExpirationTime,
                                                   &pMsgId);
    }
    else {
        rc = _pDSPro->disseminateMessage (groupName, objectId, instanceId, pData, ui32DataLen,
                                          i64ExpirationTime, &pMsgId);
    }

    bool bSuccess = ((rc == 0) && (pMsgId != nullptr));
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

    if (pData != nullptr) free (pData);
    if (pMetadata != nullptr) free (pMetadata);
    if (pMsgId != nullptr) free (pMsgId);
    if (pszMimeType != nullptr) free (pszMimeType);

    return bSuccess;
}

bool DSProProxyAdaptor::doConfigureProperties (CommHelperError & error)
{
    const char *pszMethodName = "DSProProxyAdaptor::doConfigureProperties";
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    uint32 ui32PropertiesLength = 0;
    ConfigManager *pCfgMgr = new ConfigManager();
    if (pCfgMgr == nullptr) {
        checkAndLogMsg (pszMethodName, memoryExhausted);
        return false;
    }
    pCfgMgr->init();

    if (pReader->read32 (&ui32PropertiesLength) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "Going to read buffer of size %u\n", ui32PropertiesLength);
    char *pBuf = static_cast<char*>(calloc (ui32PropertiesLength+1, sizeof (char)));

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

bool DSProProxyAdaptor::doRegisterPath (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    NodePath *pPath = DSProProxyUnmarshaller::readNodePath (_pCommHelper->getReaderRef());
    if (pPath != nullptr) {
        pPath->display();
        int rc = _pDSPro->registerPath (pPath);
        if (rc == 0) {
            _pCommHelper->sendLine (error, "OK");
        }
    }
    else {
        return false;
    }
    return true;
}

bool DSProProxyAdaptor::doSetCurrentPath (CommHelperError & error)
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

bool DSProProxyAdaptor::doGetCurrentPath (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();

    NodePath *pCurrPath = _pDSPro->getCurrentPath();
    if (pCurrPath == nullptr) {
        _pCommHelper->sendLine (error, "NOPATH");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }
    else {
        _pCommHelper->sendLine (error, "OK");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        if (DSProProxyUnmarshaller::write (pCurrPath, pWriter) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doSetCurrentPosition (CommHelperError & error)
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
        pszLocation = nullptr;
    }

    // read note's length and note
    if ((rc = pReader->read32 (&ui32Len)) != 0) {
        error = SimpleCommHelper2::CommError;
        if (pszLocation != nullptr) {
            delete[] pszLocation;
        }
        return false;
    }
    if (ui32Len > 0) {
        pszNote = new char[ui32Len+1];
        pszNote[ui32Len] = '\0';
        _pCommHelper->receiveBlob (pszNote, ui32Len, error);
        if (error != SimpleCommHelper2::None) {
            if (pszLocation != nullptr) {
                delete[] pszLocation;
            }
            if (pszNote != nullptr) {
                delete[] pszNote;
            }
            return false;
        }
    }
    else {
        pszNote = nullptr;
    }

    rc = _pDSPro->setCurrentPosition (fLatitude, fLongitude, fAltitude, pszLocation, pszNote);
    if (pszLocation != nullptr) {
        delete[] pszLocation;
    }
    if (pszNote != nullptr) {
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

bool DSProProxyAdaptor::doSetBatteryLevel (CommHelperError & error)
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

bool DSProProxyAdaptor::doSetMemoryAvailable (CommHelperError & error)
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

bool DSProProxyAdaptor::doAddPeer (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    AdaptorType ui8AdaptorType;
    char *pszNetworkInterface = nullptr;
    char *pszRemoteAddress = nullptr;
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
        if (pszNetworkInterface != nullptr) {
            free (pszNetworkInterface);
        }
        return false;
    }

    if ((rc = pReader->read32 (&ui32Len))) {
        error = SimpleCommHelper2::CommError;
        if (pszNetworkInterface != nullptr) {
            free (pszNetworkInterface);
        }
        return false;
    }
    if (ui32Len > 0) {
        pszRemoteAddress = (char *) calloc (ui32Len + 1, sizeof (char));
        _pCommHelper->receiveBlob (pszRemoteAddress, ui32Len, error);
    }
    if (error != SimpleCommHelper2::None) {
        if (pszNetworkInterface != nullptr) {
            free (pszNetworkInterface);
        }
        if (pszRemoteAddress != nullptr) {
            free (pszRemoteAddress);
        }
        return false;
    }

    if ((rc = pReader->read16 (&ui16Port))) {
        error = SimpleCommHelper2::CommError;
        if (pszNetworkInterface != nullptr) {
            free (pszNetworkInterface);
        }
        if (pszRemoteAddress != nullptr) {
            free (pszRemoteAddress);
        }
        return false;
    }

    if (_pDSPro->addPeer (ui8AdaptorType, pszNetworkInterface, pszRemoteAddress, ui16Port) < 0) {
        if (pszNetworkInterface != nullptr) {
            free (pszNetworkInterface);
        }
        if (pszRemoteAddress != nullptr) {
            free (pszRemoteAddress);
        }
        return false;
    }

    _pCommHelper->sendLine (error, "OK");

    if (pszNetworkInterface != nullptr) {
        free (pszNetworkInterface);
    }
    if (pszRemoteAddress != nullptr) {
        free (pszRemoteAddress);
    }
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetPeerNodeContext (CommHelperError & error)
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

    NodeContextImpl *pNodeContext = nullptr;
    if (sNodeId == _pDSPro->getNodeId()) {
        pNodeContext = (NodeContextImpl *) _pDSPro->_pImpl->_pNodeContextMgr->getLocalNodeContext();
    }
    else {
        pNodeContext = _pDSPro->_pImpl->_pNodeContextMgr->getPeerNodeContext (sNodeId);
    }

    if (pNodeContext != nullptr) {
        _pCommHelper->sendLine (error, "OK");
        if (error == SimpleCommHelper2::None) {
            JsonObject *pJson = (JsonObject *) pNodeContext->toJson (nullptr);
            if (pJson != nullptr) {
                const String json (pJson->toString());
                if (_pCommHelper->getWriterRef()->writeString (json) >= 0) {
                    bSuccess = true;
                }
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

bool DSProProxyAdaptor::doGetAdaptorType (CommHelperError & error)
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

bool DSProProxyAdaptor::doGetData (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();
    void *pData = nullptr;
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

    char *pszSearchId = nullptr;
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
    if (pszSearchId != nullptr) {
        free (pszSearchId);
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        _pDSPro->release (messageId.c_str(), pData);
        return false;
    }

    if (pData == nullptr) {
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

bool DSProProxyAdaptor::doGetMatchingMetaDataAsJson (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    // Read the number of attributes
    uint32 ui32Attributes = 0;
    pReader->read32 (&ui32Attributes);
    AVList aVList (ui32Attributes);
    error = DSPRO_PROXY_ADAPTOR::readMetadataAsAvList (pReader, aVList, ui32Attributes);
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

    char **ppszMetadata = _pDSPro->getMatchingMetadata (&aVList, i64BeginArrivalTimestamp,
                                                        i64EndArrivalTimestamp);

    bool bSucceded = true;
    if (ppszMetadata != nullptr) {
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

bool DSProProxyAdaptor::doSearch (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String queryType;
    String queryQualifiers;
    unsigned int uiQueryLen;
    void *pQuery = nullptr;
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
        if (pQuery == nullptr) {
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

    char *pszQueryId = nullptr;
    int rc = _pDSPro->search (groupName, queryType, queryQualifiers,
                              pQuery, uiQueryLen, i64Timeout, &pszQueryId);

    bool bSucceded = ((rc == 0) && (pszQueryId != nullptr));

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

    if (pQuery != nullptr) {
        free (pQuery);
    }
    if (pszQueryId) {
        free (pszQueryId);
    }

    return bSucceded;
}

bool DSProProxyAdaptor::doReplyToQuery (CommHelperError & error)
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
    if (ppszMsgIds == nullptr) {
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
            if (ppszMsgIds[uiRead] != nullptr) {
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
            ppszMsgIds[uiRead] = nullptr;
        }
    }
    ppszMsgIds[ui32NumberOfElements] = nullptr;

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
    if (ppszMsgIds != nullptr) {
        for (uint32 i = 0; ppszMsgIds[i] != nullptr ; i++) {
            free (ppszMsgIds[i]);
        }
        free (ppszMsgIds);
    }

    return bSucceded;
}

bool DSProProxyAdaptor::doReplyToVolatileQuery (CommHelperError & error)
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

bool DSProProxyAdaptor::doRegisterChunkFragmenter (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    char *pszMimeType = nullptr;
    if (pReader->readString (&pszMimeType) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        free (pszMimeType);
        return false;
    }
    if (0 == _pDSPro->registerChunkFragmenter (pszMimeType, this)) {
        _mChunkers.lock();
        _registeredChunkers.put (pszMimeType);
        _mChunkers.unlock();
    }
    free (pszMimeType);
    return true;
}

bool DSProProxyAdaptor::doRegisterChunkReassembler (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef ();
    char *pszMimeType = nullptr;
    if (pReader->readString (&pszMimeType) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        free (pszMimeType);
        return false;
    }
    if (0 == _pDSPro->registerChunkReassembler (pszMimeType, this)) {
        _mReassemblers.lock();
        _registeredReassemblers.put (pszMimeType);
        _mReassemblers.unlock();
    }
    free (pszMimeType);
    return true;
}

bool DSProProxyAdaptor::doAddMessage (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    uint32 ui32DataLen = 0;
    void *pData = nullptr;
    int64 i64ExpirationTime = 0;
    char *pMsgId = nullptr;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsString (pReader, sMetadata);
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
        if (pData == nullptr) {
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
        if (pData != nullptr) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->addMessage (groupName, objectId, instanceId, sMetadata,
                                  pData, ui32DataLen, i64ExpirationTime, &pMsgId);

    bool bSuccess = ((rc == 0) && (pMsgId != nullptr));

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

    if (pData != nullptr) {
        free (pData);
    }
    if (pMsgId != nullptr) {
        free (pMsgId);
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doAddMessage_AVList (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    uint32 ui32Attributes = 0;
    uint32 ui32DataLen = 0;
    void *pData = nullptr;
    int64 i64ExpirationTime = 0;
    char *pMsgId = nullptr;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the metadata
    // Read the number of attributes
    if (pReader->read32 (&ui32Attributes) != 0) {
        return false;
    }
    AVList aVList;
    error = DSPRO_PROXY_ADAPTOR::readMetadataAsAvList (pReader, aVList, ui32Attributes);
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
        if (pData == nullptr) {
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
        if (pData != nullptr) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->addMessage (groupName, objectId, instanceId, &aVList,
                                  pData, ui32DataLen, i64ExpirationTime, &pMsgId);

    if (pData != nullptr) {
        free (pData);
    }
    const bool bSuccess = ((rc == 0) && (pMsgId != nullptr));

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

    if (pMsgId != nullptr) {
        free (pMsgId);
    }
    return bSuccess;
}

bool DSProProxyAdaptor::doAddChunk (CommHelperError & error)
{
    return doAddChunkedMessage (error, true);
}

bool DSProProxyAdaptor::doAddChunk_AVList (CommHelperError & error)
{
    return doAddChunkedMessage_AVList (error, true);
}

bool DSProProxyAdaptor::doAddAdditionalChunk (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    char *pszMsgId = nullptr;
    int rc = pReader->readString (&pszMsgId);
    String messageId (pszMsgId);
    if (pszMsgId != nullptr) {
        free (pszMsgId);
    }
    if ((rc < 0) || (messageId.length() <= 0)) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    uint8 ui8ChunkId = 0;
    if (pReader->read8 (&ui8ChunkId) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the metadata length
    uint32 ui32MimeTypeLen = 0;
    if (pReader->read32 (&ui32MimeTypeLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the MIME type
    String sDataMimeType;
    if (ui32MimeTypeLen > 0) {
        if (ui32MimeTypeLen > (BUF_LEN - 1)) {
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32MimeTypeLen, error);
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        _buf[ui32MimeTypeLen] = '\0';
        sDataMimeType = _buf;
    }

    // read the chunk
    uint32 ui32DataLen = 0U;
    if (pReader->read32 (&ui32DataLen) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the data
    void *pData = nullptr;
    if (ui32DataLen > 0) {
        pData = calloc (ui32DataLen, sizeof (char));
        if (pData == nullptr) {
            return false;
        }
        _pCommHelper->receiveBlob (pData, ui32DataLen, error);
        if (error != SimpleCommHelper2::None) {
            free (pData);
            return false;
        }
    }
    DSPro::Chunk *pChunk = new DSPro::Chunk;
    if (pChunk != nullptr) {
        pChunk->ui32ChunkLen = ui32DataLen;
        pChunk->pChunkData = pData;
    }

    rc = _pDSPro->addAdditionalChunkedData (messageId.c_str(), pChunk, ui8ChunkId, sDataMimeType);

    const bool bSuccess = (rc == 0);

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

    return true;
}

bool DSProProxyAdaptor::doAddChunkedMessage (CommHelperError & error, bool bSingleChunk)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    String groupName, objectId, instanceId;
    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    String sMetadata;
    error = readMetadataAsString (pReader, sMetadata);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read chunkId, if necessary
    uint8 ui8ChunkId = 0;
    if (bSingleChunk  && (pReader->read8 (&ui8ChunkId) != 0)) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the number of chunks
    uint8 ui8NChunks = 0;
    if (pReader->read8 (&ui8NChunks) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    DSPro::Chunk **ppChunks = (DSPro::Chunk **) calloc (ui8NChunks, sizeof (DSPro::Chunk*));
    for (uint8 i = 0; i < (bSingleChunk ? 1 : ui8NChunks); i++) {
        // read the data length
        uint32 ui32DataLen = 0U;
        if (pReader->read32 (&ui32DataLen) != 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        // read the data
        void *pData = nullptr;
        if (ui32DataLen > 0) {
            pData = calloc (ui32DataLen, sizeof (char));
            if (pData == nullptr) {
                return false;
            }
            _pCommHelper->receiveBlob (pData, ui32DataLen, error);
            if (error != SimpleCommHelper2::None) {
                free (pData);
                return false;
            }
        }
        ppChunks[i] = new DSPro::Chunk;
        if (ppChunks[i] != nullptr) {
            ppChunks[i]->ui32ChunkLen = ui32DataLen;
            ppChunks[i]->pChunkData = pData;
        }
    }

    // read the metadata length
    uint32 ui32MimeTypeLen = 0;
    if (pReader->read32 (&ui32MimeTypeLen) != 0) {
        error = SimpleCommHelper2::CommError;
        DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
        return false;
    }

    // read the MIME type
    String sDataMimeType;
    if (ui32MimeTypeLen > 0) {
        if (ui32MimeTypeLen > (BUF_LEN - 1)) {
            DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32MimeTypeLen, error);
        if (error != SimpleCommHelper2::None) {
            DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
            return false;
        }
        _buf[ui32MimeTypeLen] = '\0';
        sDataMimeType = _buf;
    }

    // read the expiration time
    int64 i64ExpirationTime;
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
        return false;
    }

    char *pszMsgId = nullptr;
    int rc = bSingleChunk ?
        _pDSPro->addChunkedData (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                 sMetadata, (const DSPro::Chunk*) ppChunks[0], ui8ChunkId, ui8NChunks,
                                 sDataMimeType.c_str(), i64ExpirationTime, &pszMsgId) :
        _pDSPro->addChunkedData (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                 sMetadata.c_str(), (const DSPro::Chunk**) ppChunks, ui8NChunks,
                                 sDataMimeType.c_str(), i64ExpirationTime, &pszMsgId);

    DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);

    const bool bSuccess = ((rc == 0) && (pszMsgId != nullptr));

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
        _pCommHelper->sendStringBlock (pszMsgId, error);
    }
    if (pszMsgId != nullptr) {
        free (pszMsgId);
    }

    return (bSuccess && (error == SimpleCommHelper2::None));
}

bool  DSProProxyAdaptor::doAddChunkedMessage_AVList (CommHelperError & error, bool bSingleChunk)
{
    Reader *pReader = _pCommHelper->getReaderRef ();

    String groupName, objectId, instanceId;
    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the metadata
    // Read the number of attributes
    uint32 ui32Attributes = 0U;
    if (pReader->read32 (&ui32Attributes) != 0) {
        return false;
    }
    AVList aVList (ui32Attributes);
    error = DSPRO_PROXY_ADAPTOR::readMetadataAsAvList (pReader, aVList, ui32Attributes);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read chunkId, if necessary
    uint8 ui8ChunkId = 0;
    if (bSingleChunk && (pReader->read8 (&ui8ChunkId) != 0)) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    // read the number of chunks
    uint8 ui8NChunks = 0;
    if (pReader->read8 (&ui8NChunks) != 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    DSPro::Chunk **ppChunks = (DSPro::Chunk **) calloc (ui8NChunks, sizeof (DSPro::Chunk*));
    for (uint8 i = 0; i < (bSingleChunk ? 1 : ui8NChunks); i++) {
        // read the data length
        uint32 ui32DataLen = 0U;
        if (pReader->read32 (&ui32DataLen) != 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        // read the data
        void *pData = nullptr;
        if (ui32DataLen > 0) {
            pData = calloc (ui32DataLen, sizeof (char));
            if (pData == nullptr) {
                return false;
            }
            _pCommHelper->receiveBlob (pData, ui32DataLen, error);
            if (error != SimpleCommHelper2::None) {
                free (pData);
                return false;
            }
        }
        ppChunks[i] = new DSPro::Chunk;
        if (ppChunks[i] != nullptr) {
            ppChunks[i]->ui32ChunkLen = ui32DataLen;
            ppChunks[i]->pChunkData = pData;
        }
    }

    // read the metadata length
    uint32 ui32MimeTypeLen = 0;
    if (pReader->read32 (&ui32MimeTypeLen) != 0) {
        error = SimpleCommHelper2::CommError;
        DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
        return false;
    }

    // read the MIME type
    String sDataMimeType;
    if (ui32MimeTypeLen > 0) {
        if (ui32MimeTypeLen > (BUF_LEN - 1)) {
            DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32MimeTypeLen, error);
        if (error != SimpleCommHelper2::None) {
            DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
            return false;
        }
        _buf[ui32MimeTypeLen] = '\0';
        sDataMimeType = _buf;
    }

    // read the expiration time
    int64 i64ExpirationTime;
    if (pReader->read64 (&i64ExpirationTime) != 0) {
        error = SimpleCommHelper2::CommError;
        DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);
        return false;
    }

    char *pszMsgId = nullptr;
    int rc = bSingleChunk ?
        _pDSPro->addChunkedData (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                 &aVList, (const DSPro::Chunk*) ppChunks[0], ui8ChunkId, ui8NChunks,
                                 sDataMimeType.c_str(), i64ExpirationTime, &pszMsgId) :
        _pDSPro->addChunkedData (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                 &aVList, (const DSPro::Chunk**) ppChunks, ui8NChunks,
                                 sDataMimeType.c_str(), i64ExpirationTime, &pszMsgId);

    DSPRO_PROXY_ADAPTOR::deallocateChunks (ppChunks, ui8NChunks);

    const bool bSuccess = ((rc == 0) && (pszMsgId != nullptr));

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
        _pCommHelper->sendStringBlock (pszMsgId, error);
    }

    if (pszMsgId != nullptr) {
        free (pszMsgId);
    }

    return (bSuccess && (error == SimpleCommHelper2::None));
}

bool DSProProxyAdaptor::doChunkAndAddMessage (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    uint32 ui32DataLen;
    void *pData = nullptr;
    String sDataMimeType;
    int64 i64ExpirationTime;
    char *pszMsgId = nullptr;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsString (pReader, sMetadata);
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
        if (pData == nullptr) {
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
        if (pData != nullptr) {
            free (pData);
        }
        return false;
    }

    // read the MIME type
    if (ui32MimeTypeLen > 0) {
        if (ui32MimeTypeLen > (BUF_LEN-1)) {
            if (pData != nullptr) {
                free (pData);
            }
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32MimeTypeLen, error);
        if (error != SimpleCommHelper2::None) {
            if (pData != nullptr) {
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
        if (pData != nullptr) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->chunkAndAddMessage (groupName.c_str(), objectId.c_str(), instanceId.c_str(),
                                          sMetadata.c_str(), pData, ui32DataLen, sDataMimeType.c_str(),
                                          i64ExpirationTime, &pszMsgId);
    if (pData != nullptr) {
        free(pData);
    }

    const bool bSuccess = ((rc == 0) && (pszMsgId != nullptr));
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
        _pCommHelper->sendStringBlock (pszMsgId, error);
    }

    if (pszMsgId != nullptr) {
        free(pszMsgId);
    }

    return (bSuccess && (error == SimpleCommHelper2::None));
}

bool DSProProxyAdaptor::doChunkAndAddMessage_AVList (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    uint32 ui32Attributes = 0;
    uint32 ui32DataLen;
    void *pData = nullptr;
    String sDataMimeType;
    int64 i64ExpirationTime;
    char *pszMsgId = nullptr;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // read the metadata
    // Read the number of attributes
    if (pReader->read32 (&ui32Attributes) != 0) {
        return false;
    }
    AVList aVList (ui32Attributes);
    error = DSPRO_PROXY_ADAPTOR::readMetadataAsAvList (pReader, aVList, ui32Attributes);
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
        if (pData == nullptr) {
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
        if (pData != nullptr) {
            free (pData);
        }
        return false;
    }

    // read the MIME type
    if (ui32MimeTypeLen > 0) {
        if (ui32MimeTypeLen > (BUF_LEN-1)) {
            if (pData != nullptr) {
                free (pData);
            }
            return false;
        }
        _pCommHelper->receiveBlob (_buf, ui32MimeTypeLen, error);
        if (error != SimpleCommHelper2::None) {
            if (pData != nullptr) {
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
        if (pData != nullptr) {
            free (pData);
        }
        return false;
    }

    int rc = _pDSPro->chunkAndAddMessage (groupName, objectId, instanceId,
                                          &aVList, pData, ui32DataLen, sDataMimeType, i64ExpirationTime,
                                          &pszMsgId);

    const bool bSuccess = ((rc == 0) && (pszMsgId != nullptr));

    if (error == SimpleCommHelper2::None) {
        if (bSuccess) {
            _pCommHelper->sendLine (error, "OK");
            // write the msg id of the pushed data
            if (error == SimpleCommHelper2::None) {
                _pCommHelper->sendStringBlock(pszMsgId, error);
            }
        }
    }

    if (pData != nullptr) {
        free (pData);
    }
    if (pszMsgId != nullptr) {
        free (pszMsgId);
    }
    return (bSuccess && (error == SimpleCommHelper2::None));
}

bool DSProProxyAdaptor::doAddAnnotation (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    uint32 ui32RefObj = 0;
    String sReferredObj;
    int64 i64ExpirationTime;
    char *pMsgId = nullptr;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsString (pReader, sMetadata);
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

    bool bSuccess = ((rc == 0) && (pMsgId != nullptr));

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

    if (pMsgId != nullptr) {
        free (pMsgId);
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doAddAnnotationRef (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    String groupName;
    String objectId;
    String  instanceId;
    String sMetadata;
    String sReferredObj;
    int64 i64ExpirationTime;
    char *pMsgId = nullptr;

    error = readGroupObjectIdInstanceId (groupName, objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    error = readMetadataAsString (pReader, sMetadata);
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

    bool bSuccess = ((rc == 0) && (pMsgId != nullptr));

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

    if (pMsgId != nullptr) {
        free (pMsgId);
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doGetNodeContext (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader == nullptr) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    char *pszNodeId = nullptr;
    if (pReader->readString (&pszNodeId) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    const String nodeId (pszNodeId);
    if (pszNodeId != nullptr) {
        free (pszNodeId);
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // write the node context of the specified node (or
    // the context of the local node if nodeId is null)
    const String ctxtAsJson (_pDSPro->getNodeContext (nodeId));
    _pCommHelper->sendStringBlock (ctxtAsJson, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetNodeId (CommHelperError & error)
{
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // write the msg id of the pushed data
    const char *pszNodeId = _pDSPro->getNodeId();
    if (pszNodeId == nullptr) {
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

bool DSProProxyAdaptor::doGetPeerMsgCounts (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader == nullptr) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    char *pszNodeId = nullptr;
    if (pReader->readString (&pszNodeId) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    const String nodeId (pszNodeId);
    if (pszNodeId != nullptr) {
        free (pszNodeId);
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    // write the message counts for the specified node
    const String sPeerMsgCountsAsJson{_pDSPro->getPeerMsgCounts (nodeId)};
    _pCommHelper->sendStringBlock (sPeerMsgCountsAsJson, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetPathForPeer (CommHelperError & error)
{
    Writer *pWriter = _pCommHelper->getWriterRef();

    char line[512];
    _pCommHelper->receiveLine (line, 512, error);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    int iPathLen = 0;
    bool bSuccess = false;

    NodeContext *pNodeContext = _pDSPro->_pImpl->_pNodeContextMgr->getPeerNodeContext (line);
    if (pNodeContext == nullptr) {
        _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
        checkAndLogMsg ("DSProProxyAdaptor::doGetPathForPeer", Logger::L_Info,
                        "no path for peer %s\n", line);
        _pCommHelper->sendLine (error, "NOPATH");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
        return true;
    }
    NodePath *pCurrPath = pNodeContext->getPath();
    if ((pCurrPath == nullptr) || ((iPathLen = pCurrPath->getPathLength()) <= 0)) {
        _pCommHelper->sendLine (error, "NOPATH");
        checkAndLogMsg ("DSProProxyAdaptor::doGetPathForPeer", Logger::L_Info,
                        "no path for peer %s\n", line);
        if (error != SimpleCommHelper2::None) {
            _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();
            return false;
        }
        bSuccess = true;
    }
    else {
        _pCommHelper->sendLine (error, "OK");
        if (error == SimpleCommHelper2::None) {
            checkAndLogMsg ("DSProProxyAdaptor::doGetPathForPeer", Logger::L_Info,
                            "sending path of length %d for peer %s\n", iPathLen, line);
            if (DSProProxyUnmarshaller::write (pCurrPath, pWriter) < 0) {
                error = SimpleCommHelper2::CommError;
            }
            else {
                bSuccess = true;
            }
        }
    }

    _pDSPro->_pImpl->_pNodeContextMgr->releasePeerNodeContextList();

    if (bSuccess) {
        _pCommHelper->receiveMatch (error, "OK");
        if (error != SimpleCommHelper2::None) {
            return false;
        }
    }

    return bSuccess;
}

bool DSProProxyAdaptor::doGetSessionId (CommHelperError & error)
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

bool DSProProxyAdaptor::doGetPeerList (CommHelperError & error)
{
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == nullptr) {
        return false;
    }

    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    char **ppszPeerList = _pDSPro->getPeerList();
    unsigned int uiPeerCounter = 0;
    if (ppszPeerList != nullptr) {
        for (unsigned int i = 0; ppszPeerList[i] != nullptr; i++) {
            if (strcmp (ppszPeerList[i], "") != 0) {
                uiPeerCounter++;
            }
        }
    }
    if (pWriter->write32 (&uiPeerCounter) != 0) {
        if (ppszPeerList != nullptr) {
            for (int i = 0; ppszPeerList[i] != nullptr; i++) {
                free (ppszPeerList[i]);
            }
            free (ppszPeerList);
        }
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (uiPeerCounter > 0) {
        for (unsigned int i = 0; ppszPeerList[i] != nullptr; i++) {
            unsigned int uiStrLen = strlen (ppszPeerList[i]);
            if (uiStrLen > 0) {
                if ((pWriter->write32 (&uiStrLen) != 0) ||
                    (pWriter->writeBytes (ppszPeerList[i], uiStrLen) != 0)) {
                    // Free the current element and the remaining ones
                    free (ppszPeerList[i]);
                    for (i++; ppszPeerList[i] != nullptr; i++) {
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
        ppszPeerList = nullptr;
    }
    else if (ppszPeerList !=  nullptr) {
        for (int i = 0; ppszPeerList[i] != nullptr; i++) {
            free (ppszPeerList[i]);
        }
        free (ppszPeerList);
    }

    error = SimpleCommHelper2::None;
    return true;
}

bool DSProProxyAdaptor::doRequestCustomAreaChunk (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader == nullptr) {
        return false;
    }
    uint32 ui32StartXPixel = 0U;
    uint32 ui32EndXPixel = 0U;
    uint32 ui32StartYPixel = 0U;
    uint32 ui32EndYPixel = 0U;
    uint8 ui8CompressionQuality = 0;
    int64 i64Timeout = 0;
    char *pszMsgId = nullptr;
    char *pszMIMEType = nullptr;
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

    if (pszMsgId != nullptr) {
        free (pszMsgId);
    }
    if (pszMIMEType != nullptr) {
        free (pszMIMEType);
    }
    return bSuccess;
}

bool DSProProxyAdaptor::doRequestCustomTimeChunk (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader == nullptr) {
        return false;
    }
    int64 i64StartTime = 0U;
    int64 i64EndTime = 0U;
    uint8 ui8CompressionQuality = 0;
    char *pszMsgId = nullptr;
    char *pszMIMEType = nullptr;
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

    if (pszMsgId != nullptr) {
        free (pszMsgId);
    }
    if (pszMIMEType != nullptr) {
        free (pszMIMEType);
    }
    return bSuccess;
}

bool DSProProxyAdaptor::doRequestMoreChunks (CommHelperError & error)
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

    char *pszSearchId = nullptr;
    if (_pCommHelper->getReaderRef()->readString (&pszSearchId) < 0) {
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

bool DSProProxyAdaptor::doSubscribe (CommHelperError & error)
{
    Reader *pReader = _pCommHelper->getReaderRef();

    char *pszGroupName = nullptr;
    if (pReader->readString (&pszGroupName) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    const String groupName (pszGroupName);
    free (pszGroupName);

    uint8 ui8Priority;
    if (pReader->read8(&ui8Priority) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    bool bGroupReliable, bMsgReliable, bSequenced;
    if (pReader->readBool (&bGroupReliable) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readBool (&bMsgReliable) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }
    if (pReader->readBool (&bSequenced) < 0) {
        error = SimpleCommHelper2::CommError;
        return false;
    }

    int rc = _pDSPro->subscribe (groupName, ui8Priority, bGroupReliable, bMsgReliable, bSequenced);
    if (rc == 0) {
        _pCommHelper->sendLine (error, "OK");
    }
    else {
        _pCommHelper->sendLine (error, "ERROR %d", rc);
    }

    return (error == SimpleCommHelper2::None);
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
    if (pCtrlMsgNotifier == nullptr) {
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
    if (pInstrumentator == nullptr) {
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

bool DSProProxyAdaptor::doResetTransmissionCounters (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == nullptr) {
        return false;
    }

    _pDSPro->resetTransmissionCounters();
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    return true;
}

bool DSProProxyAdaptor::doGetDSProIds (CommHelperError & error)
{
    error = SimpleCommHelper2::None;
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == nullptr) {
        return false;
    }

    uint32 ui32;
    String objectId;
    String instanceId;
    char **ppszIds = nullptr;


    error = readObjectIdInstanceId (objectId, instanceId);
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    ppszIds = _pDSPro->getDSProIds ((objectId.length() > 0 ? objectId.c_str() : nullptr),
                                    (instanceId.length() > 0 ? instanceId.c_str() : nullptr));

    bool bSuccess = true;
    _pCommHelper->sendLine (error, "OK");
    if (error == SimpleCommHelper2::None) {
        if (ppszIds != nullptr) {
            for (unsigned int i = 0; ppszIds[i] != nullptr; i++) {
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

    if (ppszIds != nullptr) {
        for (unsigned int i = 0; ppszIds[i] != nullptr; i++) {
            free (ppszIds[i]);
        }
        free (ppszIds);
    }

    return bSuccess;
}

int DSProProxyAdaptor::dataArrived (const char *pszId, const char *pszGroupName,
                                    const char *pszObjectId, const char *pszInstanceId,
                                    const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                    const void *pBuf, uint32 ui32Len, uint8 ui8ChunkIndex,
                                    uint8 ui8TotNChunks, const char *pszQueryId)
{
    if (pszId == nullptr || pBuf == nullptr || ui32Len == 0) {
        return -1;
    }
    if (_pCallbackCommHelper == nullptr) {
        return -2;
    }

    CommHelperError error = SimpleCommHelper2::None;
    Writer *pWriter = _pCallbackCommHelper->getWriterRef();

    _pCallbackCommHelper->sendLine (error, DSProProxyUnmarshaller::DATA_ARRIVED);
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
    if (pWriter->write8 (&ui8ChunkIndex) != 0) {
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
    if (pszId == nullptr || pszXMLMetadada == nullptr || pszReferredDataId == nullptr) {
        return -1;
    }
    uint32 ui32MetadataLen = strlen (pszXMLMetadada);
    if (ui32MetadataLen == 0) {
        return -2;
    }

    if (_pCallbackCommHelper == nullptr) {
        return -3;
    }

    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, DSProProxyUnmarshaller::METADATA_ARRIVED);
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

int DSProProxyAdaptor::dataAvailable (const char *pszId, const char *pszGroupName,
                                      const char *pszObjectId, const char *pszInstanceId,
                                      const char *pszReferredDataId, const char *pszMimeType,
                                      const void *pMetadata, uint32 ui32MetadataLength,
                                      const char *pszQueryId)
{
    if (pszId == nullptr || pMetadata == nullptr || ui32MetadataLength == 0U || pszReferredDataId == nullptr) {
        return -1;
    }

    if (_pCallbackCommHelper == nullptr) {
        return -3;
    }

    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, DSProProxyUnmarshaller::DATA_AVAILABLE);
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
    _pCallbackCommHelper->sendStringBlock (pszReferredDataId, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    _pCallbackCommHelper->sendStringBlock (pszMimeType, error);
    if (error != SimpleCommHelper2::None) {
        return -14;
    }
    if (_pCallbackCommHelper->getWriterRef()->write32 (&ui32MetadataLength) != 0) {
        return -10;
    }
    //send the metadata.
    _pCallbackCommHelper->sendBlob (pMetadata, ui32MetadataLength, error);
    if (error != SimpleCommHelper2::None) {
        return -11;
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
    if (pszPeerNodeId == nullptr || _pCallbackCommHelper == nullptr) {
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
    if (pszPeerNodeId == nullptr || _pCallbackCommHelper == nullptr) {
        return;
    }
    uint32 ui32PeerIdLen = strlen (pszPeerNodeId);
    if (ui32PeerIdLen == 0) {
        return;
    }

    if (_pCallbackCommHelper == nullptr) {
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
    if ((_pCallbackCommHelper == nullptr) || (pNodePath == nullptr)) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    if (pWriter == nullptr) {
        return false;
    }

    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "pathRegisteredCallback");
    if (error != SimpleCommHelper2::None) {
        return false;
    }

    if (DSProProxyUnmarshaller::write (pNodePath, pWriter) < 0) {
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

    return true;
}

bool DSProProxyAdaptor::positionUpdated (float latitude, float longitude, float altitude, const char *pszNodeId)
{
    if (_pCallbackCommHelper == nullptr) {
        return false;
    }

    Writer *pWriter = _pCallbackCommHelper->getWriterRef();
    if (pWriter == nullptr) {
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
    if ((pszQueryId == nullptr) || (ppszMatchingMessageIds == nullptr) || (ppszMatchingMessageIds[0] == nullptr) || (pszMatchingNodeId == nullptr)) {
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
    for (unsigned int i = 0; ppszMatchingMessageIds[i] != nullptr; i++) {
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
    if ((pszQueryId == nullptr) || (pReply == nullptr) || (ui162ReplyLen == 0)) {
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
    if (pszType == nullptr || pszSenderNodeId == nullptr || pszPublisherNodeId == nullptr) {
        return false;
    }

    if (_pCallbackCommHelper == nullptr) {
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

PtrLList<Chunker::Fragment> * DSProProxyAdaptor::fragmentBuffer (const void *pBuf, uint32 ui32Len,
                                                                 const char *pszInputChunkMimeType, uint8 ui8NoOfChunks,
                                                                 const char *pszOutputChunkMimeType, uint8 ui8ChunkCompressionQuality)
{
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "fragmentFromBuffer");
    if (error != SimpleCommHelper2::None) {
        return nullptr;
    }
    ChunkerMarshaling marshaller (_pCallbackCommHelper);
    return marshaller.fragmentBuffer (pBuf, ui32Len, pszInputChunkMimeType, ui8NoOfChunks, pszOutputChunkMimeType, ui8ChunkCompressionQuality);
}

PtrLList<Chunker::Fragment> * DSProProxyAdaptor::fragmentFile (const char *pszFileName, const char *pszInputChunkMimeType,
                                                               uint8 ui8NoOfChunks, const char *pszOutputChunkMimeType,
                                                               uint8 ui8ChunkCompressionQuality)
{
    // TODO: implement this
    return nullptr;
}

Chunker::Fragment * DSProProxyAdaptor::extractFromBuffer (const void *pBuf, uint32 ui32Len, const char *pszInputChunkMimeType,
                                                          const char *pszOutputChunkMimeType, uint8 ui8ChunkCompressionQuality,
                                                          Chunker::Interval **ppPortionIntervals)
{
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "extractFromBuffer");
    if (error != SimpleCommHelper2::None) {
        return nullptr;
    }
    ChunkerMarshaling marshaller (_pCallbackCommHelper);
    return marshaller.extractFromBuffer (pBuf, ui32Len, pszInputChunkMimeType, pszOutputChunkMimeType,
                                         ui8ChunkCompressionQuality, ppPortionIntervals);
}
Chunker::Fragment * DSProProxyAdaptor::extractFromFile (const char *pszFileName, const char *pszInputChunkMimeType,
                                                        const char *pszOutputChunkMimeType, uint8 ui8ChunkCompressionQuality,
                                                        Chunker::Interval **ppPortionIntervals)
{
    // TODO: implement this
    return nullptr;
}

BufferReader * DSProProxyAdaptor::reassemble (DArray2<BufferReader> *pFragments, Annotations *pAnnotations,
                                              const char *pszChunkMimeType, uint8 ui8NoOfChunks, uint8 ui8CompressionQuality)
{
    CommHelperError error = SimpleCommHelper2::None;
    _pCallbackCommHelper->sendLine (error, "reassemble");
    if (error != SimpleCommHelper2::None) {
        return nullptr;
    }
    ChunkReassemblerMarshaling marshaller (_pCallbackCommHelper);
    return marshaller.reassemble (pFragments, pAnnotations, pszChunkMimeType, ui8NoOfChunks, ui8CompressionQuality);
}

bool DSProProxyAdaptor::doGetDisService (CommHelperError & error)
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
    if (_pCallbackCommHelper == nullptr) {
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
        if (ppszRankDescriptors[i] != nullptr) {
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

CommHelperError DSProProxyAdaptor::readMetadataAsString (NOMADSUtil::Reader *pReader, NOMADSUtil::String &sMetadata)
{
    uint32 ui32MetadataLen = 0;

    // read the metadata length
    if (pReader->read32 (&ui32MetadataLen) != 0) {
        return SimpleCommHelper2::CommError;
    }

    // instantiate buffer for metadata
    char *pszXMLMetadata = (char *) calloc (ui32MetadataLen + 1, sizeof (char));
    if (pszXMLMetadata == nullptr) {
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

