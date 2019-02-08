/**
 * DSProProxy.cpp
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

#include "DSProProxy.h"

#include "DSProProxyUnmarshaller.h"
#include "DSProListener.h"
#include "MatchmakingLogListener.h"
#include "NodePath.h"

#include "Listener.h"

#include "AVList.h"
#include "Logger.h"
#include "PtrLList.h"

#ifdef WIN32
    #define snprintf _snprintf
#endif

#define synchronized MutexUnlocker unlocker
#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

#define DISPRO_SVC_PROXY_SERVER_PORT_NUMBER 56487   // Also see DisServiceProProxyServer.h

namespace IHMC_ACI_DSPRO_PROXY
{
    SimpleCommHelper2::Error sendLineAndLog (const char *pszMethodName, SimpleCommHelper2 *pCommHelper)
    {
        if (pCommHelper == nullptr) {
            return SimpleCommHelper2::CommError;
        }
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        checkAndLogMsg ("NetworkMessageServiceProxy::sendLineAndLog", Logger::L_HighDetailDebug, "Invoking <%s>.\n", pszMethodName);
        pCommHelper->sendLine (error, pszMethodName);
        checkAndLogMsg ("NetworkMessageServiceProxy::sendLineAndLog", Logger::L_HighDetailDebug, "<%s> terminated.\n", pszMethodName);
        return error;
    }

    struct MetadataWriterFn
    {
        MetadataWriterFn (void) {}
        virtual ~MetadataWriterFn (void) {}
        virtual int operator()(SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error & error) = 0;
    };

    struct XMLMetadataWriterFn : public MetadataWriterFn
    {
        explicit XMLMetadataWriterFn (const char *pszXMLMetadata)
            : _pszXMLMetadata (pszXMLMetadata) {}
        ~XMLMetadataWriterFn (void) {}

        int operator()(SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error & error)
        {
            Writer *pWriter = pCommHelper->getWriterRef();
            if (pWriter->writeString (_pszXMLMetadata) < 0) {
                error = SimpleCommHelper2::CommError;
                return -1;
            }
            return 0;
        }

        private:
            const char *_pszXMLMetadata;
    };

    struct AVListMetadataWriterFn : public MetadataWriterFn
    {
        explicit AVListMetadataWriterFn (AVList *pMetadataList)
            : _pMetadataList (pMetadataList) {}
        ~AVListMetadataWriterFn (void) {}

        int operator()(SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error & error)
        {
            Writer *pWriter = pCommHelper->getWriterRef ();
            unsigned int uiNAttributes = _pMetadataList->getLength();
            if (pWriter->write32 (&uiNAttributes) < 0) {
                error = SimpleCommHelper2::CommError;
                return -2;
            }
            for (unsigned int i = 0; i < uiNAttributes; i++) {
                const String attribute (_pMetadataList->getAttribute (i));
                if (pWriter->writeString (attribute) < 0) {
                    error = SimpleCommHelper2::CommError;
                    return -3;
                }
                const String value (_pMetadataList->getValueByIndex (i));
                if (pWriter->writeString (value) < 0) {
                    error = SimpleCommHelper2::CommError;
                    return -4;
                }
            }
            return 0;
        }

        private:
            AVList *_pMetadataList;
    };

    struct NillMetadataWriterFn : public MetadataWriterFn
    {
        explicit NillMetadataWriterFn (void) {};
        ~NillMetadataWriterFn (void) {};

        int operator()(SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error & error)
        {
            return 0;
        }
    };

    int addOrChunkAndAddMessage (const char *pszCmd, SimpleCommHelper2 *pCommHelper,
                                 const char *pszObjectId, const char *pszInstanceId,
                                 const char *pszGroupName, MetadataWriterFn *writeMetadata,
                                 const void *pData, uint32 ui32DataLen, const char *pszDataMimeType,
                                 int64 i64ExpirationTime, char **ppszId)
    {
        if ((pszGroupName == nullptr) || (pData == nullptr) || (ui32DataLen == 0)) {
            return -1;
        }
        SimpleCommHelper2::Error error = sendLineAndLog (pszCmd, pCommHelper);
        if (error != SimpleCommHelper2::None) {
            return -2;
        }
        pCommHelper->sendLine (error, pszGroupName);
        if (error != SimpleCommHelper2::None) {
            return -3;
        }
        Writer *pWriter = pCommHelper->getWriterRef();
        if (pWriter->writeString (pszObjectId) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }
        if (pWriter->writeString (pszInstanceId) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }
        if ((*writeMetadata) (pCommHelper, error) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }
        if (pWriter->write32 (&ui32DataLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }
        if ((ui32DataLen > 0) && (pWriter->writeBytes (pData, ui32DataLen) < 0)) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }
        if ((DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE == pszCmd) ||
            (DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE_AV_LIST == pszCmd)) {
            if (pWriter->writeString (pszDataMimeType) < 0) {
                error = SimpleCommHelper2::CommError;
                return -4;
            }
        }
        if (pWriter->write64 (&i64ExpirationTime) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }

        pCommHelper->receiveMatch (error, "OK");
        if (error != SimpleCommHelper2::None) {
            return -5;
        }

        Reader *pReader = pCommHelper->getReaderRef();
        if (pReader->readString (ppszId) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }

        if (DSProProxyUnmarshaller::ADD_MESSAGE == pszCmd) {
            pCommHelper->sendLine (error, "OK");
            if (error != SimpleCommHelper2::None) {
                return -10;
            }
        }

        return 0;
    }
}

using namespace IHMC_ACI_DSPRO_PROXY;

DSProProxy::DSProProxy (uint16 ui16DesiredApplicationId, bool bUseBackgroundReconnect)
    : Stub (ui16DesiredApplicationId, &DSProProxyUnmarshaller::methodArrived,
            DSProProxyUnmarshaller::SERVICE, DSProProxyUnmarshaller::VERSION,
            bUseBackgroundReconnect),
      _dSProListeners (false),
      _matchmakingLogListeners (false),
      _searchListeners (false)
{
}

DSProProxy::~DSProProxy (void)
{
}

int DSProProxy::subscribe (const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable, bool bMsgReliable, bool bSequenced)
{
    // TODO: implement this
    return -1;
}

int DSProProxy::addUserId (const char *pszUserId)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == nullptr) || (pszUserId == nullptr)) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::ADD_USER_ID, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszUserId)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

int DSProProxy::addAreaOfInterest (const char *pszAreaName, float fUpperLeftLat, float fUpperLeftLon,
                                   float fLowerRightLat, float fLowerRightLon,
                                   int64 i64StatTime, int64 i64EndTime)
{
    // TODO: implement this
    return -1;
}

int DSProProxy::setRankingWeigths (float coordRankWeight, float timeRankWeight,
                                   float expirationRankWeight, float impRankWeight,
                                   float sourceReliabilityRankWeigth,
                                   float informationContentRankWeigth,
                                   float predRankWeight, float targetWeight,
                                   bool bStrictTarget,
                                   bool bConsiderFuturePathSegmentForMatchmacking)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == nullptr) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_RANKING_WEIGHTS, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeFloat (&coordRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    if (pWriter->writeFloat (&timeRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    if (pWriter->writeFloat (&expirationRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return -5;
    }

    if (pWriter->writeFloat (&sourceReliabilityRankWeigth) < 0) {
        error = SimpleCommHelper2::CommError;
        return -6;
    }
    if (pWriter->writeFloat (&informationContentRankWeigth) < 0) {
        error = SimpleCommHelper2::CommError;
        return -6;
    }
    if (pWriter->writeFloat (&impRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return -6;
    }
    if (pWriter->writeFloat (&predRankWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return -7;
    }
    if (pWriter->writeFloat (&targetWeight) < 0) {
        error = SimpleCommHelper2::CommError;
        return -8;
    }
    if (pWriter->writeBool (&bStrictTarget) < 0) {
        error = SimpleCommHelper2::CommError;
        return -9;
    }
    if (pWriter->writeBool (&bConsiderFuturePathSegmentForMatchmacking) < 0) {
        error = SimpleCommHelper2::CommError;
        return -10;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -11;
    }
    return 0;
}

int DSProProxy::registerPath (NodePath *pPath)
{
    if (pPath == nullptr) {
        return -1;
    }
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::REGISTER_PATH, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (DSProProxyUnmarshaller::write (pPath, pWriter) < 0) {
        error = SimpleCommHelper2::ProtocolError;
        return -3;
    }
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

/**
 * Choose what path the node is following now.
 */
int DSProProxy::setCurrentPath (const char *pszPathId)
{
    if (pszPathId == nullptr) {
        return -1;
    }
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_CURRENT_PATH, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    _pCommHelper->sendLine (error, pszPathId);
    if (error != SimpleCommHelper2::None) {
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

/**
 * A time stamp is set automatically by DisServicePro.
 */
int DSProProxy::setCurrentPosition (float fLatitude, float fLongitude, float fAltitude,
                                    const char *pszLocation, const char *pszNote)
{
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_CURRENT_POSITION, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    static const unsigned int LEN = 12;
    char buf[LEN];

    snprintf (buf, LEN, "%f", fLatitude);
    _pCommHelper->sendLine (error, buf);
    if (error != SimpleCommHelper2::None) {
        return -3;
    }

    snprintf (buf, LEN, "%f", fLongitude);
    _pCommHelper->sendLine (error, buf);
    if (error != SimpleCommHelper2::None) {
        return -4;
    }

    snprintf (buf, LEN, "%f", fAltitude);
    _pCommHelper->sendLine (error, buf);
    if (error != SimpleCommHelper2::None) {
        return -5;
    }

    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszLocation) < 0) {
        error = SimpleCommHelper2::CommError;
        return -6;
    }

    if (pWriter->writeString (pszNote) < 0) {
        error = SimpleCommHelper2::CommError;
        return -7;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -8;
    }

    return 0;
}

int DSProProxy::setBatteryLevel (unsigned int uiBatteryLevel)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::setMemoryAvailable (unsigned int uiMemoryAvailable)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::addPeer (AdaptorType protocol, const char *pszNetworkInterface,
                         const char *pszRemoteAddress, uint16 ui16Port)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::getAdaptorType (AdaptorId adaptorId, AdaptorType &adaptorType)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::notUseful (const char *pszMessageID)
{
    // TODO: implement this
    return 0;
}

void DSProProxy::resetTransmissionCounters (void)
{
}

const char* DSProProxy::getVersion (void) const
{
    // TODO: implement this
    return nullptr;
}

int DSProProxy::configureProperties (ConfigManager *pCfgMgr)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::setSelectivity (float matchingThreshold)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == nullptr) {
        return -1;
    }
    if ((matchingThreshold < 0.0f) || (matchingThreshold > 10.0f)) {
        return -2;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_SELECTIVITY, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -3;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->write32 (&matchingThreshold) < 0) {
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    return 0;
}

int DSProProxy::setRangeOfInfluence (const char *pszMilStd2525Symbol, uint32 ui32RangeInMeters)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == nullptr) || (pszMilStd2525Symbol == nullptr)) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_RANGE_OF_INFLUENCE, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef ();
    if (pWriter->writeString (pszMilStd2525Symbol) < 0) {
        return -3;
    }
    if (pWriter->write32 (&ui32RangeInMeters) < 0) {
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    return 0;
}

int DSProProxy::setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters)
{
    synchronized (&_stubMutex);
    if (_pCommHelper == nullptr) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_DEFAULT_USEFUL_DISTANCE, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef ();
    if (pWriter->write32 (&ui32UsefulDistanceInMeters) < 0) {
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    return 0;
}

int DSProProxy::setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == nullptr) || (pszDataMIMEType == nullptr)) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_USEFUL_DISTANCE, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszDataMIMEType) < 0) {
        return -3;
    }
    if (pWriter->write32 (&ui32UsefulDistanceInMeters) < 0) {
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    return 0;
}

int DSProProxy::addCustumPoliciesAsXML (const char **ppszCustomPoliciesXML)
{
    if (ppszCustomPoliciesXML == nullptr) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::ADD_CUSTUMS_POLICIES_AS_XML, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    uint16 ui16Count = 0;
    for (; ppszCustomPoliciesXML[ui16Count] != nullptr; ui16Count++);
    if (pWriter->write16 (&ui16Count) < 0) {
        return -3;
    }
    for (uint16 i = 0; i < ui16Count; i++) {
        if (pWriter->writeString (ppszCustomPoliciesXML[i]) < 0) {
            return -4;
        }
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    return 0;
}

int DSProProxy::setMissionId (const char *pszMissionName)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == nullptr) || (pszMissionName == nullptr)) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_MISSION_ID, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszMissionName)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

int DSProProxy::setRole (const char *pszRole)
{
    synchronized (&_stubMutex);
    if ((_pCommHelper == nullptr) || (pszRole == nullptr)) {
        return -1;
    }

    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SET_ROLE, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszRole)) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    return 0;
}

NodePath * DSProProxy::getCurrentPath (void)
{
    // TODO: implement this
    return nullptr;
}

char ** DSProProxy::getPeerList (void)
{
    // TODO: implement this
    return nullptr;
}

int DSProProxy::getData (const char *pszId, const char *pszCbackParams, void **ppData, uint32 &ui32DataLen, bool &bHasMoreChunks)
{
    if (ppData == nullptr) {
        return -1;
    }

    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::GET_DATA, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }

    _pCommHelper->sendLine (error, pszId);
    if (error != SimpleCommHelper2::None) {
        return -3;
    }

    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter->writeString (pszCbackParams) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }

    Reader *pReader = _pCommHelper->getReaderRef();
    uint32 ui32Len = 0U;
    if (pReader->read32 (&ui32Len) < 0) {
        error = SimpleCommHelper2::CommError;
        return -6;
    }

    uint8 ui8HasMoreChunks = 0;
    if (ui32Len > 0) {
        *ppData = malloc (ui32Len);
        if (*ppData == nullptr) {
            return -7;
        }
        if (pReader->readBytes (*ppData, ui32Len) < 0) {
            error = SimpleCommHelper2::CommError;
            free (*ppData);
            return -8;
        }
        if (pReader->read8 (&ui8HasMoreChunks) < 0) {
            error = SimpleCommHelper2::CommError;
            free (*ppData);
            return -9;
        }
    }

    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -10;
    }

    ui32DataLen = ui32Len;
    bHasMoreChunks = ui8HasMoreChunks == 1;
    return 0;
}

int DSProProxy::release (const char *pszMessageId, void *pData)
{
    // TODO: implement this
    return 0;
}

char** DSProProxy::getDSProIds (const char* pszObjectId, const char* pszInstanceId)
{
    // TODO: implement this
    return nullptr;
}

char** DSProProxy::getMatchingMetadata (NOMADSUtil::AVList* pAVQueryList, int64 i64BeginArrivalTimestamp, int64 i64EndArrivalTimestamp)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::requestCustomAreaChunk (const char* pszChunkedObjMsgId, const char* pszMIMEType, uint32 ui32StartXPixel, uint32 ui32EndXPixel, uint32 ui32StartYPixel, uint32 ui32EndYPixel, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::requestCustomTimeChunk (const char* pszChunkedObjMsgId, const char* pszMIMEType, int64 i64StartTimeInMillisec, int64 i64EndTimeInMillisec, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::requestMoreChunks (const char* pszChunkedMsgId, const char* pszCallbackParameter)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::search (const char* pszGroupName, const char* pszQueryType, const char *pszQueryQualifiers, const void *pszQuery, unsigned uiQueryLen, int64 i64TimeoutInMilliseconds, char** ppszQueryId)
{
    if (ppszQueryId != nullptr) {
        *ppszQueryId = nullptr;
    }
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SEARCH, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == nullptr) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    if (pWriter->writeString (pszGroupName) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    if (pWriter->writeString (pszQueryType) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    if (pWriter->writeString (pszQueryQualifiers) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }

    if (pWriter->write32 (&uiQueryLen) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    if ((uiQueryLen > 0) && (pWriter->writeBytes (pszQuery, uiQueryLen) < 0)) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }

    if (pWriter->write64 (&i64TimeoutInMilliseconds) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }

    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -2;
    }

    static const uint32 buf_len = 128;
    char searchIdBuf[buf_len];
    uint32 ui32SearchIdLen = _pCommHelper->receiveBlock (searchIdBuf, buf_len, error);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    String searchId (searchIdBuf, ui32SearchIdLen);

    if (ppszQueryId != nullptr) {
        *ppszQueryId = searchId.r_str();
    }

    return 0;
}

int DSProProxy::searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds)
{
    if (ppszMatchingMsgIds == nullptr) {
        return -1;
    }
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::SEARCH_REPLY, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == nullptr) {
        error = SimpleCommHelper2::CommError;
        return -3;
    }
    if (pWriter->writeString (pszQueryId) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    uint32 ui32Count = 0;
    for (; ppszMatchingMsgIds[ui32Count]; ui32Count++);
    if (pWriter->write32 (&ui32Count) < 0) {
        error = SimpleCommHelper2::CommError;
        return -4;
    }
    for (uint32 i = 0; i < ui32Count; i++) {
        if (pWriter->writeString (ppszMatchingMsgIds[ui32Count]) < 0) {
            error = SimpleCommHelper2::CommError;
            return -4;
        }
    }

    return 0;
}

int DSProProxy::volatileSearchReply (const char* pszQueryId, const void* pReply, uint16 ui162ReplyLen)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::addMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const char* pszJsonMetadata,
                            const void* pData, uint32 ui32DataLen, int64 i64ExpirationTime, char** ppszId)
{
    if ((pszGroupName == nullptr) || (pszJsonMetadata == nullptr) || (pData == nullptr) || (ui32DataLen == 0)) {
        return -1;
    }
    XMLMetadataWriterFn writeMetadata (pszJsonMetadata);
    return addOrChunkAndAddMessage (DSProProxyUnmarshaller::ADD_MESSAGE, _pCommHelper,
                                    pszObjectId, pszInstanceId, pszGroupName, &writeMetadata, pData, ui32DataLen,
                                    nullptr, i64ExpirationTime, ppszId);
}

int DSProProxy::addMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, AVList* pMetadataAttrList,
                            const void* pData, uint32 ui32DataLen, int64 i64ExpirationTime, char** ppszId)
{
    if ((pszGroupName == nullptr) || (pMetadataAttrList == nullptr) || (pMetadataAttrList->getLength() <= 0U) || (pData == nullptr) || (ui32DataLen == 0)) {
        return -1;
    }
    AVListMetadataWriterFn writeMetadata (pMetadataAttrList);
    return addOrChunkAndAddMessage (DSProProxyUnmarshaller::ADD_MESSAGE_AV_LIST, _pCommHelper,
                                    pszObjectId, pszInstanceId, pszGroupName, &writeMetadata, pData, ui32DataLen,
                                    nullptr, i64ExpirationTime, ppszId);
}

int DSProProxy::chunkAndAddMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const char* pszJsonMetadata,
                                    const void* pData, uint32 ui32DataLen, const char* pszDataMimeType, int64 i64ExpirationTime, char** ppszId)
{
    if ((pszGroupName == nullptr) || (pszJsonMetadata == nullptr) || (pData == nullptr) || (ui32DataLen == 0)) {
        return -1;
    }
    XMLMetadataWriterFn writeMetadata (pszJsonMetadata);
    return addOrChunkAndAddMessage (DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE, _pCommHelper,
                                    pszObjectId, pszInstanceId, pszGroupName, &writeMetadata, pData, ui32DataLen,
                                    pszDataMimeType, i64ExpirationTime, ppszId);
}

int DSProProxy::chunkAndAddMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, AVList* pMetadataAttrList,
                                    const void* pData, uint32 ui32DataLen, const char* pszDataMimeType, int64 i64ExpirationTime, char** ppszId)
{
    if ((pszGroupName == nullptr) || (pMetadataAttrList == nullptr) || (pMetadataAttrList->getLength() <= 0U) || (pData == nullptr) || (ui32DataLen == 0)) {
        return -1;
    }
    synchronized (&_stubMutex);
    AVListMetadataWriterFn writeMetadata (pMetadataAttrList);
    return addOrChunkAndAddMessage (DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE_AV_LIST, _pCommHelper,
                                    pszObjectId, pszInstanceId, pszGroupName, &writeMetadata, pData, ui32DataLen,
                                    pszDataMimeType, i64ExpirationTime, ppszId);
}

int DSProProxy::disseminateMessage (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const void *pData, uint32 ui32DataLen, int64 i64ExpirationTime, char **ppszId)
{
    if ((pszGroupName == nullptr) || (pData == nullptr) || (ui32DataLen == 0)) {
        return -1;
    }
    synchronized (&_stubMutex);
    NillMetadataWriterFn writeMetadata;
    return addOrChunkAndAddMessage (DSProProxyUnmarshaller::DISSEMINATE, _pCommHelper,
                                    pszObjectId, pszInstanceId, pszGroupName, &writeMetadata,
                                    pData, ui32DataLen, nullptr, i64ExpirationTime, ppszId);
}

int DSProProxy::addAnnotation (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, const char* pszJsonMetadata, const char* pszReferredObject, int64 i64ExpirationTime, char** ppszId)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::addAnnotation (const char* pszGroupName, const char* pszObjectId, const char* pszInstanceId, AVList* pszMetadata, const char* pszReferredObject, int64 i64ExpirationTime, char** ppszId)
{
    // TODO: implement this
    return 0;
}

int DSProProxy::cancel (const char* pszId)
{
    if (pszId == nullptr) {
        return -1;
    }
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::CANCEL, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    _pCommHelper->sendLine (error, pszId);
    if (error != SimpleCommHelper2::None) {
        return -3;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -3;
    }
    return 0;
}

int DSProProxy::cancelByObjectAndInstanceId (const char *pszObjectId, const char *pszInstanceId)
{
    if (pszObjectId == nullptr) {
        return -1;
    }
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::CANCEL, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    _pCommHelper->sendLine (error, pszObjectId);
    if (error != SimpleCommHelper2::None) {
        return -3;
    }
    _pCommHelper->sendLine (error, pszInstanceId == nullptr ? "" : pszInstanceId);
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -5;
    }
    return 0;
}

String DSProProxy::getNodeId (void)
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::GET_NODE_ID, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return String();
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return String();
    }
    char *pszNodeId = nullptr;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readString (&pszNodeId) < 0) {
        error = SimpleCommHelper2::CommError;
        return String();
    }
    String nodeId (pszNodeId);
    if (pszNodeId != nullptr) {
        free (pszNodeId);
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return nodeId;
    }
    return nodeId;
}

String DSProProxy::getSessionId (void) const
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::GET_SESSION_ID, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return String ();
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return String ();
    }
    char *pszNodeId = nullptr;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readString (&pszNodeId) < 0) {
        error = SimpleCommHelper2::CommError;
        return String ();
    }
    String nodeId (pszNodeId);
    if (pszNodeId != nullptr) {
        free (pszNodeId);
    }
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return nodeId;
    }
    return nodeId;
}

String DSProProxy::getNodeContext (const char *pszNodeId) const
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::GET_NODE_CONTEXT, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -1;
    }
    Writer *pWriter = _pCommHelper->getWriterRef();
    if (pWriter == nullptr) {
        return -2;
    }
    _pCommHelper->sendStringBlock (pszNodeId, error);
    if (error != SimpleCommHelper2::None) {
        return -1;
    }
    _pCommHelper->receiveMatch (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -4;
    }
    char *pszString = nullptr;
    Reader *pReader = _pCommHelper->getReaderRef();
    if (pReader->readString (&pszString) < 0) {
        error = SimpleCommHelper2::CommError;
        return -5;
    }
    String ctxt (pszString);
    _pCommHelper->sendLine (error, "OK");
    if (error != SimpleCommHelper2::None) {
        return -6;
    }
    free (pszString);
    return ctxt;
}

int DSProProxy::addAreaOfInterest (const char *pszAreaName, float fUpperLeftLat, float fUpperLeftLon,
                                   float fLowerRightLat, int fLowerRightLon,
                                   int64 i64StatTime, int64 i64EndTime)
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::ADD_AREA_OF_INTEREST, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -1;
    }

    // TO-DO implement this method
    return 0;
}

int DSProProxy::registerDSProListener (uint16 ui16ClientId, DSProListener* pListener, uint16& ui16AssignedClientId)
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::REGISTER_DSPRO_LISTENER_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    uint16 ui16CurrClientId = ui16ClientId;
    for (; _dSProListeners.contains (ui16CurrClientId); ui16CurrClientId++);
    _dSProListeners.put (ui16CurrClientId, pListener);
    ui16AssignedClientId = ui16CurrClientId;
    return 0;
}

int DSProProxy::deregisterDSProListener (uint16 ui16ClientId, DSProListener* pListener)
{
    synchronized (&_stubMutex);
    _dSProListeners.remove (ui16ClientId);
    return 0;
}

int DSProProxy::registerMatchmakingLogListener(uint16 ui16ClientId, MatchmakingLogListener* pListener, uint16& ui16AssignedClientId)
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::REGISTER_MATCHMAKING_LISTENER_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    uint16 ui16CurrClientId = ui16ClientId;
    for (; _matchmakingLogListeners.contains (ui16CurrClientId); ui16CurrClientId++);
    _matchmakingLogListeners.put (ui16CurrClientId, pListener);
    ui16AssignedClientId = ui16CurrClientId;
    return 0;
}

int DSProProxy::deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener* pListener)
{
    synchronized (&_stubMutex);
    _matchmakingLogListeners.remove (ui16ClientId);
    return 0;
}

int DSProProxy::registerSearchListener (uint16 ui16ClientId, SearchListener *pListener, uint16 &ui16AssignedClientId)
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::REGISTER_SEARCH_LISTENER_METHOD, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return -2;
    }
    uint16 ui16CurrClientId = ui16ClientId;
    for (; _searchListeners.contains (ui16CurrClientId); ui16CurrClientId++);
    _searchListeners.put (ui16CurrClientId, pListener);
    ui16AssignedClientId = ui16CurrClientId;
    return 0;
}

int DSProProxy::deregisterSearchListener (uint16 ui16ClientId, SearchListener *pListener)
{
    synchronized (&_stubMutex);
    _searchListeners.remove (ui16ClientId);
    return 0;
}

void DSProProxy::resetTransmissionHistory (void)
{
    synchronized (&_stubMutex);
    SimpleCommHelper2::Error error = sendLineAndLog (DSProProxyUnmarshaller::RESET_TRANSMISSION_COUNTERS, _pCommHelper);
    if (error != SimpleCommHelper2::None) {
        return;
    }
    _pCommHelper->receiveMatch (error, "OK");
}

int DSProProxy::dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                             const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                             const void *pBuf, uint32 ui32Len, uint8 ui8ChunkIndex, uint8 ui8TotNChunks,
                             const char *pszCallbackParameter)
{
    synchronized (&_stubMutex);
    if (_dSProListeners.getCount() == 0) {
        return -1;
    }
    UInt32Hashtable<DSProListener>::Iterator iter = _dSProListeners.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        iter.getValue()->dataArrived (pszId, pszGroupName, pszObjectId, pszInstanceId, pszAnnotatedObjMsgId, pszMimeType,
                                      pBuf, ui32Len, ui8ChunkIndex, ui8TotNChunks, pszCallbackParameter);
    }
    return 0;
}

int DSProProxy::metadataArrived (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId,
                                 const char *pszReferredDataInstanceId, const char *pszXMLMetadada,
                                 const char *pszReferredDataId, const char *pszQueryId)
{
    synchronized (&_stubMutex);
    if (_dSProListeners.getCount() == 0) {
        return -1;
    }
    UInt32Hashtable<DSProListener>::Iterator iter = _dSProListeners.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        iter.getValue()->metadataArrived (pszId, pszGroupName, pszReferredDataObjectId, pszReferredDataInstanceId,
                                          pszXMLMetadada, pszReferredDataId, pszQueryId);
    }
    return 0;
}

int DSProProxy::searchArrived (const char *pszQueryId, const char *pszGroupName,
                               const char *pszQuerier, const char *pszQueryType,
                               const char *pszQueryQualifiers,
                               const void *pszQuery, unsigned int uiQueryLen)
{
    synchronized (&_stubMutex);
    if (_dSProListeners.getCount () == 0) {
        return -1;
    }
    UInt32Hashtable<SearchListener>::Iterator iter = _searchListeners.getAllElements ();
    for (; !iter.end (); iter.nextElement ()) {
        iter.getValue ()->searchArrived (pszQueryId, pszGroupName, pszQuerier, pszQueryType,
                                         pszQueryQualifiers, pszQuery, uiQueryLen);
    }
    return 0;
}

int DSProProxy::searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds,
                                    const char *pszMatchingNodeId)
{
    synchronized (&_stubMutex);
    if (_dSProListeners.getCount() == 0) {
        return -1;
    }
    UInt32Hashtable<SearchListener>::Iterator iter = _searchListeners.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        iter.getValue()->searchReplyArrived (pszQueryId, ppszMatchingMessageIds, pszMatchingNodeId);
    }
    return 0;
}

int DSProProxy::volatileSearchReplyArrived (const char *pszQueryId, const void *pReply,
                                            uint16 ui162ReplyLen, const char *pszMatchingNodeId)
{
    synchronized (&_stubMutex);
    if (_dSProListeners.getCount() == 0) {
        return -1;
    }
    UInt32Hashtable<SearchListener>::Iterator iter = _searchListeners.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        iter.getValue()->volatileSearchReplyArrived (pszQueryId, pReply, ui162ReplyLen, pszMatchingNodeId);
    }
    return 0;
}

