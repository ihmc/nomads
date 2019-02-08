/*
 * DSPro.cpp
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
 */

#include "DSPro.h"

#include "ChunkQuery.h"
#include "CommAdaptor.h"
#include "Controller.h"
#include "ControlMessageNotifier.h"
#include "Defs.h"
#include "DisServiceAdaptor.h"
#include "DSProImpl.h"
#include "DSProListener.h"
#include "InformationStore.h"
#include "LocalNodeContext.h"
#include "MetaData.h"
#include "MetadataConfigurationImpl.h"
#include "NodeContextManager.h"
#include "NodePath.h"
#include "Reset.h"
#include "SessionId.h"
#include "SymbolCodeTemplate.h"
#include "Stats.h"

#include "Searches.h"

#include "BoundingBox.h"

#include "Chunker.h"
#include "MimeUtils.h"

#include "BufferWriter.h"
#include "ConfigManager.h"
#include "Json.h"
#include "Logger.h"
#include "PtrLList.h"
#include "SearchProperties.h"
#include "MetadataHelper.h"
#include "BufferReader.h"

#define checkInitialized if (!_bInitialized) return -9999
#define checkInitializedStr if (!_bInitialized) return String()
#define checkInitializedNill if (!_bInitialized) return nullptr
#define checkInitializedVoid if (!_bInitialized) return

#define checkAndLogAddStat if ((rc == 0) && (_pStats != nullptr))

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;
using namespace IHMC_MISC;
using namespace IHMC_MISC_MIL_STD_2525;

namespace IHMC_ACI
{
    static Logger::Level API_INVOKED_LOGGING_LEVEL = Logger::L_Info;
    static const char * API_INVOKED_MSG = "API invoked:";

    const char * safeString (const char *pszS)
    {
        return (pszS == nullptr ? "null" : pszS);
    }

    int sendCustumRequest (ChunkQuery &query, int64 i64TimeoutInMilliseconds, DSPro *pDSPro)
    {
        BufferWriter bw (1024, 1024);
        if (query.write (&bw) < 0) {
            return -1;
        }

        char *pszQueryId = nullptr;
        const char *pszQualifiers = "";
        int rc = pDSPro->search (ChunkQuery::GROUP, ChunkQuery::QUERY_TYPE, pszQualifiers,
                                 bw.getBuffer(), bw.getBufferLength(), i64TimeoutInMilliseconds,
                                 &pszQueryId);
        if (rc < 0) {
            rc = -2;
        }
        if (pszQueryId != nullptr) {
            free (pszQueryId);
        }
        else {
            rc = -3;
        }
        return rc;
    }

    Chunker::Fragment * toChunkFragment (const DSPro::Chunk *pChunk, const char *pszDataMimeType, uint8 ui8Part, uint8 ui8NChunks)
    {
        Chunker::Fragment *pFrag = new Chunker::Fragment;
        pFrag->pReader = new BufferReader (pChunk->pChunkData, pChunk->ui32ChunkLen, false);
        pFrag->ui64FragLen = pChunk->ui32ChunkLen;
        pFrag->src_type = pszDataMimeType;
        pFrag->out_type = pszDataMimeType;
        pFrag->ui8Part = ui8Part;
        pFrag->ui8TotParts = ui8NChunks;
        return pFrag;
    }

    void fillChunkPtrLList (const DSPro::Chunk **ppChunkedData, const char *pszDataMimeType, uint8 ui8NChunks, PtrLList<Chunker::Fragment> &chunks)
    {
        for (uint8 i = 0; (i < ui8NChunks) && (ppChunkedData[i] != nullptr); i++) {
            chunks.append (toChunkFragment (ppChunkedData[i], pszDataMimeType, i + 1, ui8NChunks));
        }
    }

    JsonObject ifaceMsgCountsToJson (const std::string & sInterface, const MsgCounts & msgCounts)
    {
        JsonObject jsonObject;
        jsonObject.setString (MsgCounts::MSG_COUNTS_INTERFACE_NAME, sInterface.c_str());
        jsonObject.setNumber (MsgCounts::MSG_COUNTS_UNICAST_COUNT_NAME, msgCounts.ui64UnicastMsgs);
        jsonObject.setNumber (MsgCounts::MSG_COUNTS_MULTICAST_COUNT_NAME, msgCounts.ui64MulticastMsgs);

        return jsonObject;
    }
}

const char * const DSPro::NODE_NAME = "aci.dspro.nodename";
const char * const DSPro::ENABLE_DISSERVICE_ADAPTOR = "aci.dspro.adaptor.disservice.enable";
const char * const DSPro::ENABLE_MOCKETS_ADAPTOR = "aci.dspro.adaptor.mockets.enable";
const char * const DSPro::ENABLE_TCP_ADAPTOR = "aci.dspro.adaptor.tcp.enable";
const char * const DSPro::ENABLE_UDP_ADAPTOR = "aci.dspro.adaptor.udp.enable";
const char * const DSPro::ENABLE_NATS_ADAPTOR = "aci.dspro.adaptor.nats.enable";
const char * const DSPro::ENABLE_TOPOPLOGY_EXCHANGE = "aci.dspro.topologyExchange.enable";
const char * const DSPro::PEER_ID_NAME = "peerId";
const char * const DSPro::PEER_MSG_COUNTS_JSON_ARRAY_NAME = "msgCountsByIface";

const int64 DSPro::DEFAULT_UPDATE_TIMEOUT = 5000;
const int64 DSPro::DEFAULT_REPLICATE_TIMEOUT = 1000;

const char * const DSPro::SQL_QUERY_TYPE = "sql";

DSPro::DSPro (const char *pszNodeId, const char *pszVersion)
    : _bInitialized (false),
      _pImpl (new DSProImpl (pszNodeId, pszVersion)),
      _pReset (new Reset()),
      _pMetadataConf (nullptr)
{
}

DSPro::~DSPro (void)
{
    if (_pStats != nullptr) {
        _pStats->requestTerminationAndWait();
    }
    delete _pImpl;
    delete _pMetadataConf;
}

const char * DSPro::getVersion (void) const
{
    checkInitializedNill;
    return _pImpl->getVersion();
}

int DSPro::init (ConfigManager *pCfgMgr,
                 const char *pszMetadataExtraAttributes,
                 const char *pszMetadataValues)
{
    const char *pszMethodName = "DSPro::init";
    if ((pCfgMgr == nullptr) || (_pImpl == nullptr)) {
        return -1;
    }

    // Load Metadata Configuration
    _pMetadataConf = (pszMetadataExtraAttributes == nullptr ?
                      MetadataConfigurationImpl::getConfiguration() :
                      MetadataConfigurationImpl::getConfiguration (pszMetadataExtraAttributes));
    if (_pMetadataConf == nullptr) {
        return -3;
    }
    if (pszMetadataValues != nullptr) {
        int rc = _pMetadataConf->setMetadataFields (pszMetadataValues);
        if (rc < 0) {
            return -4;
        }
    }

    // Init stats, if necessary
    int64 i64Timeout = pCfgMgr->getValueAsInt64 ("aci.dspro.stats.timeout", 0);
    String stats (pCfgMgr->getValue ("aci.dspro.stats.file"));
    if ((stats.length() > 0) || (i64Timeout > 0)) {
        if (i64Timeout <= 0) {
            i64Timeout = Stats::DEFAULT_TIMEOUT;
        }
        _pStats = Stats::getInstance (_pImpl->getNodeId(), i64Timeout);
    }
    if (_pStats != nullptr) {
        _pStats->start();
        checkAndLogMsg (pszMethodName, Logger::L_Info, "started Stats thread.\n");
    }

    // Init DSPro Implementation
    if (_pImpl->init (pCfgMgr, _pMetadataConf) < 0) {
        return -5;
    }

    _bInitialized = true;

    uint16 ui16AssignedClientId = CallbackHandler::Reset;
    if (registerDSProListener (CallbackHandler::Reset, _pReset, ui16AssignedClientId) < 0) {
        return -6;
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "registered \"reset\" message handler.\n");
    }

    return 0;
}

int DSPro::configureProperties (ConfigManager *pCfgMgr)
{
    checkInitialized;
    // TODO: implement this
    return 0;
}

int DSPro::changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len)
{
    const char *pszMethodName = "DSPro::changeEncryptionKey";
    const int rc = _pImpl->changeEncryptionKey (pchKey, ui32Len);
    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Changed encryption key\n");
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Did not change encryption key. Return code: %d\n", rc);
    }
    return rc;
}

int DSPro::setRankingWeigths (float coordRankWeight, float timeRankWeight,
                              float expirationRankWeight, float impRankWeight,
                              float sourceReliabilityRankWeigth,
                              float informationContentRankWeigth, float predRankWeight,
                              float targetWeight, bool bStrictTarget,
                              bool bConsiderFuturePathSegmentForMatchmacking)
{
    checkInitialized;
    LocalNodeContext *pLocalNodeContext = _pImpl->getNodeContextManager()->getLocalNodeContext();
    int rc = pLocalNodeContext->configureMetadataRanker (coordRankWeight, timeRankWeight,
                                                         expirationRankWeight, impRankWeight,
                                                         sourceReliabilityRankWeigth,
                                                         informationContentRankWeigth, predRankWeight,
                                                         targetWeight, bStrictTarget,
                                                         bConsiderFuturePathSegmentForMatchmacking);
    _pImpl->getNodeContextManager()->releaseLocalNodeContext();
    if (rc < 0) {
        checkAndLogMsg ("DSPro::setRankingWeigths", Logger::L_Warning,
                        "could not configure ranking weight in local node context. "
                        "The returned code is %d.\n", rc);
        return -2;
    }

    return 0;
}

int DSPro::setSelectivity (float matchingThreshold)
{
    checkAndLogMsg ("DSPro::setSelectivity", API_INVOKED_LOGGING_LEVEL, "%s %f\n", API_INVOKED_MSG, matchingThreshold);
    checkInitialized;
    return _pImpl->setMatchingThreshold (matchingThreshold);
}

int DSPro::setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeInMeters)
{
    checkAndLogMsg ("DSPro::setRangeOfInfluence", API_INVOKED_LOGGING_LEVEL, "%s %s %u\n", API_INVOKED_MSG, safeString (pszNodeType), ui32RangeInMeters);
    if (pszNodeType == nullptr) {
        return -1;
    }
    checkInitialized;
    return _pImpl->setRangeOfInfluence (pszNodeType, ui32RangeInMeters);
}

int DSPro::setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters)
{
    checkAndLogMsg ("DSPro::setDefaultUsefulDistance", API_INVOKED_LOGGING_LEVEL, "%s %u\n",
                    API_INVOKED_MSG, ui32UsefulDistanceInMeters);
    checkInitialized;
    return _pImpl->setDefaultUsefulDistance (ui32UsefulDistanceInMeters);
}

int DSPro::setUsefulDistance (const char *pszMIMEType, uint32 ui32UsefulDistanceInMeters)
{
    checkAndLogMsg ("DSPro::setUsefulDistance", API_INVOKED_LOGGING_LEVEL, "%s %s %u\n", API_INVOKED_MSG,
                    safeString (pszMIMEType), ui32UsefulDistanceInMeters);
    checkInitialized;
    return _pImpl->setUsefulDistance (pszMIMEType, ui32UsefulDistanceInMeters);
}

int DSPro::addCustumPoliciesAsXML (const char **ppszCustomPoliciesXML)
{
    checkAndLogMsg ("DSPro::addCustumPoliciesAsXML", API_INVOKED_LOGGING_LEVEL, "%s\n", API_INVOKED_MSG);
    checkInitialized;
    LocalNodeContext *pLocalNodeContext = _pImpl->getNodeContextManager()->getLocalNodeContext();
    int rc = pLocalNodeContext->addCustomPolicies (ppszCustomPoliciesXML);
    _pImpl->getNodeContextManager()->releaseLocalNodeContext();
    if (rc < 0) {
        checkAndLogMsg ("DSPro::addCustumPoliciesAsXML", Logger::L_Warning,
                        "could not configure ranking custom policies in local node context. "
                        "The returned code is %d.\n", rc);
        return -2;
    }

    return 0;
}

int DSPro::registerPath (NodePath *pPath)
{
    checkAndLogMsg ("DSPro::registerPath", API_INVOKED_LOGGING_LEVEL, "%s %s %d\n",
                    API_INVOKED_MSG, pPath == nullptr ? "null" : safeString (pPath->getPathId()),
                    pPath == nullptr ? 0 : pPath->getPathLength());

    checkInitialized;
    return _pImpl->registerPath (pPath);
}

int DSPro::addUserId (const char *pszUserName)
{
    checkAndLogMsg ("DSPro::registerPath", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszUserName));

    checkInitialized;
    return _pImpl->addUserId (pszUserName);
}

int DSPro::setMissionId (const char *pszMissionName)
{
    checkAndLogMsg ("DSPro::setMissionId", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszMissionName));

    checkInitialized;
    if (pszMissionName == nullptr) {
        return -1;
    }
    return _pImpl->setMissionId (pszMissionName);
}

int DSPro::setRole (const char *pszRole)
{
    checkAndLogMsg ("DSPro::setRole", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszRole));

    checkInitialized;
    if (pszRole == nullptr) {
        return -1;
    }
    return _pImpl->setRole (pszRole);
}

int DSPro::setTeamId (const char *pszTeamId)
{
    checkAndLogMsg ("DSPro::setTeamId", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszTeamId));

    checkInitialized;
    if (pszTeamId == nullptr) {
        return -1;
    }
    return _pImpl->setTeamId (pszTeamId);
}

int DSPro::setNodeType (const char *pszType)
{
    checkAndLogMsg ("DSPro::setNodeType", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszType));

    checkInitialized;
    if (pszType == nullptr) {
        return -1;
    }
    return _pImpl->setNodeType (pszType);
}

int DSPro::addAreaOfInterest (const char *pszAreaName, float fUpperLeftLat, float fUpperLeftLon,
                              float fLowerRightLat, float fLowerRightLon,
                              int64 i64StatTime, int64 i64EndTime)
{
    checkAndLogMsg ("DSPro::addAreaOfInterest", API_INVOKED_LOGGING_LEVEL, "%s %s %f %f %f %f\n",
                    API_INVOKED_MSG, safeString (pszAreaName),
                    fUpperLeftLat, fUpperLeftLon, fLowerRightLat, fLowerRightLon);

    checkInitialized;
    BoundingBox bb (fUpperLeftLat, fUpperLeftLon, fLowerRightLat, fLowerRightLon);
    return _pImpl->addAreaOfInterest (pszAreaName, bb, i64StatTime, i64EndTime);
}

int DSPro::setCurrentPath (const char *pszPathId)
{
    checkAndLogMsg ("DSPro::setCurrentPath", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszPathId));

    checkInitialized;
    if (pszPathId == nullptr) {
        return -1;
    }
    return _pImpl->setCurrentPath (pszPathId);
}

int DSPro::setCurrentPosition (float fLatitude, float fLongitude, float fAltitude,
                               const char *pszLocation, const char *pszNote)
{
    checkAndLogMsg ("DSPro::setCurrentPosition", API_INVOKED_LOGGING_LEVEL, "%s %f, %f, %f\n",
                    API_INVOKED_MSG, fLatitude, fLongitude, fAltitude);

    checkInitialized;
    return _pImpl->setCurrentPosition (fLatitude, fLongitude, fAltitude,
                                       pszLocation, pszNote);
}

int DSPro::setBatteryLevel (unsigned int uiBatteryLevel)
{
    checkInitialized;
    _pImpl->getNodeContextManager()->setBatteryLevel (uiBatteryLevel);
    return 0;
}

int DSPro::setMemoryAvailable(unsigned int uiMemoryAvailable)
{
    checkInitialized;
    _pImpl->getNodeContextManager()->setMemoryAvailable (uiMemoryAvailable);
    return 0;
}

int DSPro::addPeer (AdaptorType adaptorType, const char *pszNetworkInterface,
                    const char *pszRemoteAddress, uint16 ui16Port)
{
    checkAndLogMsg ("DSPro::addPeer", API_INVOKED_LOGGING_LEVEL, "%s, %s, %s, %s:%d\n", API_INVOKED_MSG,
                    getAdaptorTypeAsString (adaptorType), safeString (pszNetworkInterface),
                    safeString (pszRemoteAddress), ui16Port);

    checkInitialized;
    return _pImpl->addPeer (adaptorType, pszNetworkInterface,
                            pszRemoteAddress, ui16Port);
}

int DSPro::getAdaptorType (AdaptorId adaptorId, AdaptorType &adaptorType)
{
    checkInitialized;
    adaptorType = _pImpl->getCommAdaptorManager()->getAdaptorType (adaptorId);
    if (adaptorType == UNKNOWN) {
        return -1;
    }
    return 0;
}

NodePath * DSPro::getCurrentPath (void)
{
    checkAndLogMsg ("DSPro::getCurrentPath", API_INVOKED_LOGGING_LEVEL, "%s\n", API_INVOKED_MSG);
    checkInitializedNill;
    NodePath *pPath = _pImpl->getNodeContextManager()->getLocalNodeContext()->getPath();
    _pImpl->getNodeContextManager()->releaseLocalNodeContext();

    // TODO: make a copy!

    return pPath;
}

char ** DSPro::getPeerList (void)
{
    checkAndLogMsg ("DSPro::getPeerList", API_INVOKED_LOGGING_LEVEL, "%s\n", API_INVOKED_MSG);
    checkInitializedNill;
    DArray2<String> *pPeerList = _pImpl->getPeerList();
    if (pPeerList == nullptr) {
        return nullptr;
    }
    const unsigned int uiListSize = pPeerList->size();
    if (uiListSize == 0) {
        delete pPeerList;
        return nullptr;
    }
    char **ppszPeerList = (char **)calloc (uiListSize + 1, sizeof (char*));
    if (ppszPeerList == nullptr) {
        delete pPeerList;
        return nullptr;
    }
    unsigned int iCount = 0;
    for (unsigned int j = 0; j < uiListSize; j++) {
        if ((*pPeerList)[j].length() > 0) {
            ppszPeerList[iCount] = (*pPeerList)[j].r_str();
            iCount++;
        }
    }
    ppszPeerList[iCount] = nullptr;
    delete pPeerList;
    return ppszPeerList;
}

int DSPro::getData (const char *pszId, const char *pszCallbackParameter,
                    void **ppData, uint32 &ui32DataLen, bool &bHasMoreChunks)
{
    checkAndLogMsg ("DSPro::getData", API_INVOKED_LOGGING_LEVEL, "%s %s %s\n", API_INVOKED_MSG,
                    safeString (pszId), safeString (pszCallbackParameter));
    checkInitialized;
    if (_pStats != nullptr) {
        _pStats->getData (pszId);
    }
    return _pImpl->getData (pszId, pszCallbackParameter, ppData, ui32DataLen, bHasMoreChunks);
}

int DSPro::release (const char *pszMessageId, void *pData)
{
    checkAndLogMsg ("DSPro::release", API_INVOKED_LOGGING_LEVEL, "%s %s\n", API_INVOKED_MSG, safeString (pszMessageId));
    checkInitialized;
    if (pData != nullptr) {
        free (pData);
        pData = nullptr;
    }
    // TODO: implement this

    /*if (isDSProGroup (pszGroupName)) {
        // DsPro always returns a copy of pData (because it has to remove its own
        // metadata
        free (pData);
    }
    else {
        // TODO: ask the DataCache to release
    }
    free ((char*)pszGroupName);*/
    return 0;
}

char ** DSPro::getDSProIds (const char *pszObjectId, const char *pszInstanceId)
{
    checkAndLogMsg ("DSPro::getDSProIds", API_INVOKED_LOGGING_LEVEL, "%s %s\n", API_INVOKED_MSG,
                    safeString (pszObjectId), safeString (pszInstanceId));

    checkInitializedNill;
    if (pszObjectId == nullptr) {
        return nullptr;
    }
    return _pImpl->getInformationStore()->getDSProIds (pszObjectId, pszInstanceId);
}

char ** DSPro::getMatchingMetadata (NOMADSUtil::AVList *pAVQueryList,
                                          int64 i64BeginArrivalTimestamp,
                                          int64 i64EndArrivalTimestamp)
{
    checkAndLogMsg ("DSPro::getMatchingMetadata", API_INVOKED_LOGGING_LEVEL, "%s\n", API_INVOKED_MSG);
    checkInitializedNill;
    return _pImpl->getMatchingMetadataAsJson (pAVQueryList, i64BeginArrivalTimestamp,
                                             i64EndArrivalTimestamp);
}

int DSPro::notUseful (const char *pszMessageId)
{
    checkAndLogMsg ("DSPro::notUseful", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString(pszMessageId));

    checkInitialized;
    return _pImpl->notUseful (pszMessageId);
}

int DSPro::requestCustomAreaChunk (const char *pszAnnotatedObjMsgId, const char *pszMIMEType,
                                   uint32 ui32StartXPixel, uint32 ui32EndXPixel, uint32 ui32StartYPixel,
                                   uint32 ui32EndYPixel, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds)
{
    checkAndLogMsg ("DSPro::requestCustomAreaChunk", API_INVOKED_LOGGING_LEVEL, "%s %s %s %u %u %u %u\n",
                    API_INVOKED_MSG, safeString (pszAnnotatedObjMsgId), safeString (pszMIMEType),
                    ui32StartXPixel, ui32EndXPixel, ui32StartYPixel, ui32EndYPixel);

    checkInitialized;
    if ((pszAnnotatedObjMsgId == nullptr) || (pszMIMEType == nullptr)) {
        return -1;
    }
    if ((ui32StartXPixel != 0U) && (ui32StartXPixel == ui32EndXPixel)) {
        // Line: not allowed
        return -2;
    }
    if ((ui32StartYPixel != 0U) && (ui32StartYPixel == ui32EndYPixel)) {
        // Line: not allowed
        return -3;
    }
    if ((ui32StartXPixel == ui32StartYPixel) && (ui32EndYPixel == ui32StartYPixel) && (ui32StartYPixel == 0U)) {
        // All 0s
        return -4;
    }
    const Chunker::Type type = MimeUtils::mimeTypeToFragmentType (pszMIMEType);
    if (type == Chunker::UNSUPPORTED) {
        return -5;
    }
    if (_pStats != nullptr) {
        _pStats->requestMoreChunks (pszAnnotatedObjMsgId);
    }

    ChunkQuery query (pszAnnotatedObjMsgId, type, type, ui8CompressionQuality);

    Chunker::Interval interval;
    interval.dimension = Chunker::X;
    interval.uiStart = ui32StartXPixel;
    interval.uiEnd = ui32EndXPixel;
    query.addInterval (interval);

    interval.dimension = Chunker::Y;
    interval.uiStart = ui32StartYPixel;
    interval.uiEnd = ui32EndYPixel;
    query.addInterval (interval);

    if (sendCustumRequest (query, i64TimeoutInMilliseconds, this) < 0) {
        return -6;
    }
    return 0;
}

int DSPro::requestCustomTimeChunk (const char *pszAnnotatedObjMsgId, const char *pszMIMEType,
                                   int64 i64StartTimeInMillisec, int64 i64EndTimeInMillisec,
                                   uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds)
{
    checkAndLogMsg ("DSPro::requestCustomTimeChunk", API_INVOKED_LOGGING_LEVEL, "%s %s %s %lld %lld\n",
                    API_INVOKED_MSG, safeString (pszAnnotatedObjMsgId), safeString (pszMIMEType),
                    i64StartTimeInMillisec, i64EndTimeInMillisec);

    checkInitialized;
    if ((pszAnnotatedObjMsgId == nullptr) || (pszMIMEType == nullptr)) {
        return -1;
    }
    if ((i64StartTimeInMillisec < 0) || (i64EndTimeInMillisec < 0) || (i64EndTimeInMillisec <= i64StartTimeInMillisec)) {
        return -2;
    }

    const Chunker::Type type = MimeUtils::mimeTypeToFragmentType (pszMIMEType);
    if (type == Chunker::UNSUPPORTED) {
        return -3;
    }
    if (_pStats != nullptr) {
        _pStats->requestMoreChunks (pszAnnotatedObjMsgId);
    }

    ChunkQuery query (pszAnnotatedObjMsgId, type, type, ui8CompressionQuality);
    Chunker::Interval interval;
    interval.dimension = Chunker::T;
    interval.uiStart = i64StartTimeInMillisec;
    interval.uiEnd = i64EndTimeInMillisec;
    query.addInterval (interval);

    if (sendCustumRequest (query, i64TimeoutInMilliseconds, this) < 0) {
        return - 4;
    }
    return 0;
}

int DSPro::requestMoreChunks (const char *pszChunkedMsgId, const char *pszCallbackParameter)
{
    checkAndLogMsg ("DSPro::requestMoreChunks", API_INVOKED_LOGGING_LEVEL, "%s %s %s\n",
                    API_INVOKED_MSG, safeString (pszChunkedMsgId), safeString (pszCallbackParameter));

    checkInitialized;
    if (_pStats != nullptr) {
        _pStats->requestMoreChunks (pszChunkedMsgId);
    }
    return _pImpl->requestMoreChunks (pszChunkedMsgId, pszCallbackParameter);
}

int DSPro::search (const char *pszGroupName, const char *pszQueryType,
                   const char *pszQueryQualifiers, const void *pQuery,
                   unsigned int uiQueryLen, int64 i64TimeoutInMilliseconds,
                   char **ppszQueryId)
{
    checkAndLogMsg ("DSPro::search", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %s, %u\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszQueryType),
                    safeString (pszQueryQualifiers), safeString (pszQueryType), uiQueryLen);

    checkInitialized;
    if (pszGroupName == nullptr || pszQueryType == nullptr || pszQueryQualifiers == nullptr ||
        pQuery == nullptr || uiQueryLen == 0 || ppszQueryId == nullptr || (i64TimeoutInMilliseconds < 0)) {
        return -1;
    }

    SearchProperties searchProp;
    searchProp.pszGroupName = pszGroupName;
    searchProp.pszQuerier = getNodeId();
    searchProp.pszQueryType = pszQueryType;
    searchProp.pszQueryQualifiers = pszQueryQualifiers;
    searchProp.pQuery = pQuery;
    searchProp.uiQueryLen = uiQueryLen;
    searchProp.i64TimeoutInMillis = i64TimeoutInMilliseconds;

    if ((_pImpl->search (searchProp, ppszQueryId) < 0) || (*ppszQueryId == nullptr)) {
        return -2;
    }

    // Notify the search listeners
    searchProp.pszQueryId = *ppszQueryId;
    _pImpl->getCallbackHandler()->searchArrived (&searchProp);

    return 0;
}

int DSPro::searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds)
{
    checkAndLogMsg ("DSPro::searchReply", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszQueryId));

    checkInitialized;
    if (pszQueryId == nullptr || strlen (pszQueryId) == 0 || ppszMatchingMsgIds == nullptr) {
        return -1;
    }

    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if (Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) == 0) {
        _pImpl->getCallbackHandler()->searchReplyArrived (pszQueryId, ppszMatchingMsgIds, getNodeId());
    }

    return 0;
}

int DSPro::volatileSearchReply (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen)
{
    checkAndLogMsg ("DSPro::volatileSearchReply", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszQueryId));

    checkInitialized;
    if ((pszQueryId == nullptr) || (strlen (pszQueryId)) == 0 || (pReply == nullptr) || (ui162ReplyLen == 0)) {
        return -1;
    }

    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if (Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) == 0) {
        _pImpl->getCallbackHandler()->volatileSearchReplyArrived (pszQueryId, pReply, ui162ReplyLen, getNodeId());
    }

    return 0;
}

int DSPro::addMessage (const char *pszGroupName, const char *pszObjectId,
                       const char *pszInstanceId, const char *pszJsonMetadada,
                       const void *pData, uint32 ui32DataLen,
                       int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSPro::addMessage";
    checkAndLogMsg (pszMethodName, API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %u %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui32DataLen, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if ((pszGroupName == nullptr) || (pszJsonMetadada == nullptr)) {
        return -1;
    }

    MetaData metadata;
    if (metadata.fromString (pszJsonMetadada) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse metadata: %s.\n", pszJsonMetadada);
        return -2;
    }

    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, &metadata,
                                 pData, ui32DataLen, i64ExpirationTime, ppszId);

    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, &metadata);
    }

    return (rc < 0 ? -4 : 0);
}

int DSPro::addMessage (const char *pszGroupName, const char *pszObjectId,
                       const char *pszInstanceId, AVList *pMedatadaAttrList,
                       const void *pData, uint32 ui32DataLen,
                       int64 i64ExpirationTime, char **ppszId)
{
    checkAndLogMsg ("DSPro::addMessage", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %u %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui32DataLen, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pMedatadaAttrList == nullptr) {
        return -1;
    }

    MetaData *pMetadata = toMetadata (pMedatadaAttrList);
    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                 pData, ui32DataLen, i64ExpirationTime, ppszId);

    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, pMetadata);
    }
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                           const char *pszJsonMetadata, const Chunk *pChunkedData, uint8 ui8ChunkId, uint8 ui8NChunks,
                           const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSPro::addChunkedData";
    checkAndLogMsg (pszMethodName, API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %d %d %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui8ChunkId, ui8NChunks, i64ExpirationTime);

    checkInitialized;
    if (pChunkedData == nullptr) {
        return -1;
    }

    const Chunk *ppChunkedData[2] = { pChunkedData, nullptr };
    PtrLList<Chunker::Fragment> chunks;
    fillChunkPtrLList (ppChunkedData, pszDataMimeType, ui8NChunks, chunks);
    chunks.getFirst()->ui8Part = ui8ChunkId;    // override part number with the one specified by the application

    MetaData metadata;
    if (metadata.fromString (pszJsonMetadata) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse metadata: %s.\n", pszJsonMetadata);
        return -2;
    }

    String mimeType;
    metadata.getFieldValue (MetadataInterface::DATA_FORMAT, mimeType);

    int rc = _pImpl->addChunkedMessage (pszGroupName, pszObjectId, pszInstanceId, &metadata,
                                        &chunks, pszDataMimeType, i64ExpirationTime, ppszId);

    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, &metadata);
    }

    return (rc < 0 ? -4 : 0);
}

int DSPro::addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                           AVList *pMedatadaAttrList, const Chunk *pChunkedData, uint8 ui8ChunkId, uint8 ui8NChunks,
                           const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    checkAndLogMsg ("DSPro::addChunkedData", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %d %d %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui8ChunkId, ui8NChunks, i64ExpirationTime);

    checkInitialized;
    if (pChunkedData == nullptr) {
        return -1;
    }
    const Chunk *ppChunkedData[2] = { pChunkedData, nullptr };
    PtrLList<Chunker::Fragment> chunks;
    fillChunkPtrLList (ppChunkedData, pszDataMimeType, ui8NChunks, chunks);
    chunks.getFirst ()->ui8Part = ui8ChunkId;    // override part number with the one specified by the application
    MetaData *pMetadata = toMetadata (pMedatadaAttrList);
    int rc = _pImpl->addChunkedMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                        &chunks, pszDataMimeType, i64ExpirationTime, ppszId);
    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, pMetadata);
    }
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::addAdditionalChunkedData (const char *pszId, const Chunk *pChunkedData, uint8 ui8ChunkId, const char *pszDataMimeType)
{
    checkAndLogMsg ("DSPro::addAdditionalChunkedData", API_INVOKED_LOGGING_LEVEL, "%s %s %s %d\n",
                    API_INVOKED_MSG, safeString (pszId), safeString (pszDataMimeType), ui8ChunkId);

    const char *pszMethodName = "DSPro::addAdditionalChunkedData";
    checkInitialized;
    bool bHasMoreChunks = false;
    uint32 ui32DataLen = 0U;
    void *pData = nullptr;
    int rc = _pImpl->getData (pszId, nullptr, &pData, ui32DataLen, bHasMoreChunks);
    if ((rc < 0) || (pData == nullptr) || (ui32DataLen == 0U)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not find metadata with id %s. Can't add related chunk.\n", pszId);
        return -1;
    }
    MetaData *pMetadata = toMetadata (pData, ui32DataLen);
    free (pData);
    if (pMetadata == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse metadata with id %s. Can't add related chunk.\n", pszId);
        return -2;
    }
    String dataId;
    pMetadata->getReferredDataMsgId (dataId);
    String objectId;
    pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_OBJECT_ID, objectId);
    String instanceId;
    pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_INSTANCE_ID, instanceId);
    int64 i64ExpirationTime = 0;
    pMetadata->getFieldValue (MetadataInterface::EXPIRATION_TIME, &i64ExpirationTime);
    Chunker::Fragment *pChunk = toChunkFragment (pChunkedData, pszDataMimeType, ui8ChunkId, 0);
    if (pChunk == nullptr) {
        return -3;
    }
    rc = _pImpl->addAdditionalChunk (pszId, dataId, objectId, instanceId, pChunk, i64ExpirationTime);
    delete pChunk;
    return (rc < 0 ? -4 : 0);
}

int DSPro::addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                           const char *pszJsonMetadada, const Chunk **ppChunkedData, uint8 ui8NChunks,
                           const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSPro::addChunkedData";
    checkAndLogMsg (pszMethodName, API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %d %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui8NChunks, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || pszJsonMetadada == nullptr || ppChunkedData == nullptr || ui8NChunks == 0U || pszDataMimeType == nullptr) {
        return -1;
    }
    PtrLList<Chunker::Fragment> chunks;
    fillChunkPtrLList (ppChunkedData, pszDataMimeType, ui8NChunks, chunks);

    MetaData metadata;
    if (metadata.fromString (pszJsonMetadada) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse metadata: %s.\n", pszJsonMetadada);
        return -2;
    }

    int rc = _pImpl->addChunkedMessage (pszGroupName, pszObjectId, pszInstanceId, &metadata,
                                        &chunks, pszDataMimeType, i64ExpirationTime, ppszId);
    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, &metadata);
    }

    return (rc < 0 ? -4 : 0);
}

int DSPro::addChunkedData (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                           AVList *pMedatadaAttrList, const Chunk **ppChunkedData, uint8 ui8NChunks,
                           const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    checkAndLogMsg ("DSPro::addChunkedData", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %d %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui8NChunks, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || pMedatadaAttrList == nullptr || ppChunkedData == nullptr || ui8NChunks == 0U || pszDataMimeType == nullptr) {
        return -1;
    }
    PtrLList<Chunker::Fragment> chunks;
    fillChunkPtrLList (ppChunkedData, pszDataMimeType, ui8NChunks, chunks);
    MetaData *pMetadata = toMetadata (pMedatadaAttrList);
    int rc = _pImpl->addChunkedMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                        &chunks, pszDataMimeType, i64ExpirationTime, ppszId);
    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, pMetadata);
    }
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, const char *pszJsonMetadada,
                               const void *pData, uint32 ui32DataLen,
                               const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSPro::chunkAndAddMessage";
    checkAndLogMsg (pszMethodName, API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %s %u %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), safeString (pszDataMimeType), ui32DataLen, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || pszJsonMetadada == nullptr || pData == nullptr || ui32DataLen == 0U || pszDataMimeType == nullptr) {
        return -1;
    }

    MetaData metadata;
    if (metadata.fromString (pszJsonMetadada) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse metadata: %s.\n", pszJsonMetadada);
        return -2;
    }

    int rc = _pImpl->chunkAndAddMessage (pszGroupName, pszObjectId, pszInstanceId, &metadata,
                                         pData, ui32DataLen, pszDataMimeType, i64ExpirationTime, ppszId);
    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, &metadata);
    }

    return (rc < 0 ? -4 : 0);
}

int DSPro::chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, AVList *pMedatadaAttrList,
                               const void *pData, uint32 ui32DataLen, const char *pszDataMimeType,
                               int64 i64ExpirationTime, char **ppszId)
{
    checkAndLogMsg ("DSPro::chunkAndAddMessage", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %s %u %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), safeString (pszDataMimeType), ui32DataLen, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || pMedatadaAttrList == nullptr || pData == nullptr || ui32DataLen == 0U || pszDataMimeType == nullptr) {
        return -1;
    }

    MetaData *pMetadata = toMetadata (pMedatadaAttrList);
    int rc = _pImpl->chunkAndAddMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                         pData, ui32DataLen, pszDataMimeType, i64ExpirationTime, ppszId);
    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName, pMetadata);
    }
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::disseminateMessage (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId,
                               const void *pData, uint32 ui32DataLen,
                               int64 i64ExpirationTime, char **ppszId)
{
    checkAndLogMsg ("DSPro::disseminateMessage", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %u %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui32DataLen, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || ui32DataLen == 0U) {
        return -1;
    }

    int rc = _pImpl->disseminateMessage (pszGroupName, pszObjectId, pszInstanceId,
                                         pData, ui32DataLen, i64ExpirationTime, ppszId);
    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName);
    }
    return (rc < 0 ? -2 : 0);
}

int DSPro::disseminatedMessageMetadata (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId,
                                        const void *pMetadata, uint32 ui32MetadataLength, const void *pData, uint32 ui32DataLen,
                                        const char *pszMimeType, int64 i64ExpirationTime, char **ppszId)
{
    checkAndLogMsg ("DSPro::disseminatedMessageMetadata", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %u %lld\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), ui32DataLen, i64ExpirationTime);

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pszGroupName == nullptr || ui32DataLen == 0U) {
        return -1;
    }

    int rc = _pImpl->disseminateMessageMetadata (pszGroupName, pszObjectId, pszInstanceId, pMetadata, ui32MetadataLength,
                                                 pData, ui32DataLen, pszMimeType, i64ExpirationTime, ppszId);

    checkAndLogAddStat{
        _pStats->addMessage (pszGroupName);
    }
    return (rc < 0 ? -2 : 0);
}

int DSPro::subscribe (const char *pszGroupName, uint8 ui8Priority, bool bGroupReliable,
                      bool bMsgReliable, bool bSequenced)
{
    checkAndLogMsg ("DSPro::subscribe", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszGroupName));

    CommAdaptor::Subscription sub (pszGroupName);
    sub.bGroupReliable = bGroupReliable;
    sub.bMsgReliable = bMsgReliable;
    sub.bSequenced = bSequenced;
    return _pImpl->subscribe (sub);
}

int DSPro::addAnnotation (const char *pszGroupName, const char *pszObjectId,
                          const char *pszInstanceId, const char *pszJsonMetadada,
                          const char *pszReferredObject,
                          int64 i64ExpirationTime, char **ppszId)
{
    const char *pszMethodName = "DSPro::addAnnotation";
    checkAndLogMsg (pszMethodName, API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %s\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), safeString (pszReferredObject));

    checkInitialized;
    if (pszJsonMetadada == nullptr || pszReferredObject == nullptr || ppszId == nullptr) {
        return -1;
    }
    *ppszId = nullptr;

    MetaData metadata;
    if (metadata.fromString (pszJsonMetadada) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse metadata: %s.\n", pszJsonMetadada);
        return -2;
    }

    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, &metadata,
                                 nullptr, 0, i64ExpirationTime, ppszId);

    return (rc < 0 ? -4 : 0);
}

int DSPro::addAnnotation (const char *pszGroupName, const char *pszObjectId,
                          const char *pszInstanceId, AVList *pMedatadaAttrList,
                          const char *pszReferredObject,
                          int64 i64ExpirationTime, char **ppszId)
{
    checkAndLogMsg ("DSPro::addAnnotation", API_INVOKED_LOGGING_LEVEL, "%s %s %s %s %s\n",
                    API_INVOKED_MSG, safeString (pszGroupName), safeString (pszObjectId),
                    safeString (pszInstanceId), safeString (pszReferredObject));

    checkInitialized;
    if (ppszId != nullptr) {
        *ppszId = nullptr;
    }
    if (pMedatadaAttrList == nullptr || pszReferredObject == nullptr || ppszId == nullptr) {
        return -1;
    }
    *ppszId = nullptr;

    MetaData *pMetadata = toMetadata (pMedatadaAttrList);
    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                 nullptr, 0, i64ExpirationTime, ppszId);
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::cancel (const char *pszId)
{
    checkAndLogMsg ("DSPro::cancel", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszId));

    checkInitialized;
    if (pszId == nullptr) {
        return -1;
    }

    MetadataList *pMetadataList = _pImpl->getInformationStore()->getMetadataForData (pszId);
    if (pMetadataList == nullptr || pMetadataList->getFirst() == nullptr) {
        // It may be the case that the object to cancel is a metadata-only object,
        // in which case InfoStore::getMetadataForData() will always return null,
        // therefore I also need to check whether there is a metadata with id pszId
        MetadataInterface *pMetadata = _pImpl->getInformationStore()->getMetadata (pszId);
        if (pMetadata != nullptr) {
            if (pMetadataList == nullptr) {
                pMetadataList = new MetadataList();
            }
            if (pMetadataList != nullptr) {
                pMetadataList->prepend (pMetadata);
            }
        }
    }

    // Remove all the elements contained in pMetadataList
    if (pMetadataList != nullptr) {
        MetadataInterface *pCurr, *pNext;
        pNext = pMetadataList->getFirst();
        for (unsigned int i = 0; ((pCurr = pNext) != nullptr); i++) {
            pNext = pMetadataList->getNext();
            char *pszMetadataID = nullptr;
            if (pCurr->getFieldValue (MetaData::MESSAGE_ID, &pszMetadataID) == 0 &&
                pszMetadataID != nullptr) {
                _pImpl->getInformationStore()->deleteMetadataFromDB (pszMetadataID);
            }
            delete pMetadataList->remove (pCurr);
        }
        delete pMetadataList;
    }

    return 0;
}

int DSPro::cancelByObjectId (const char *pszObjectId)
{
    return cancelByObjectAndInstanceId (pszObjectId, nullptr);
}

int DSPro::cancelByObjectAndInstanceId (const char *pszObjectId, const char *pszInstanceId)
{
    return _pImpl->getInformationStore()->deleteMetadata (pszObjectId, pszInstanceId);
}

DisseminationService * DSPro::getDisService (void)
{
    checkInitializedNill;
    CommAdaptor *pCommAdaptor = _pImpl->getCommAdaptorManager()->getAdaptorByType (DISSERVICE);
    if (pCommAdaptor == nullptr) {
        return nullptr;
    }
    return static_cast<DisServiceAdaptor *>(pCommAdaptor)->getDisseminationService();
}

const char * DSPro::getNodeId (void) const
{
    checkAndLogMsg ("DSPro::getNodeId", API_INVOKED_LOGGING_LEVEL, "%s\n", API_INVOKED_MSG);
    checkInitializedNill;
    return _pImpl->getNodeId();
}

NOMADSUtil::String DSPro::getPeerMsgCounts (const char * pszNodeId) const
{
    if (_pStats == nullptr) {
        return "";
    }

    JsonObject json;
    json.setString (PEER_ID_NAME, pszNodeId);

    JsonArray jsonArray;
    auto msgCountsByInterface = _pStats->getPeerMsgCountsByInterface (pszNodeId);
    for (const auto & pIfaceMsgCounts : msgCountsByInterface) {
        auto jsonObject = ifaceMsgCountsToJson (pIfaceMsgCounts.first, pIfaceMsgCounts.second);
        jsonArray.addObject (&jsonObject);
    }
    json.setObject (PEER_MSG_COUNTS_JSON_ARRAY_NAME, &jsonArray);

    return json.toString();
}

String DSPro::getSessionId (void) const
{
    return SessionId::getInstance()->getSessionId();
}

String DSPro::getNodeContext (const char *pszNodeId) const
{
    checkAndLogMsg ("DSPro::getNodeContext", API_INVOKED_LOGGING_LEVEL, "%s %s\n",
                    API_INVOKED_MSG, safeString (pszNodeId));
    checkInitializedStr;
    const String nodeId (getNodeId());
    NodeContextImpl *pNodeCtxt = nullptr;
    const bool bGetLocalNodeCtxt = (pszNodeId == nullptr) || (nodeId == pszNodeId);
    if (bGetLocalNodeCtxt) {
        pNodeCtxt = _pImpl->getNodeContextManager()->getLocalNodeContext();
    }
    else {
        pNodeCtxt = _pImpl->getNodeContextManager()->getPeerNodeContext (pszNodeId);
    }

    String ctxt;
    if (pNodeCtxt != nullptr) {
        JsonObject *pJson = pNodeCtxt->toJson (nullptr);
        if (pJson != nullptr) {
            JsonObject *pNodeInfo = pJson->getObject ("nodeInfo");
            if (pNodeInfo != nullptr) {
                const String sessionId (getSessionId());
                if (sessionId.length () > 0) {
                    pNodeInfo->setString ("sessionId", sessionId);
                }
            }
            ctxt = pJson->toString();
            if (pNodeInfo != nullptr) {
                delete pNodeInfo;
            }
            delete pJson;
        }
    }

    if (bGetLocalNodeCtxt) {
        _pImpl->getNodeContextManager()->releaseLocalNodeContext();
    }
    else {
        _pImpl->getNodeContextManager()->releasePeerNodeContextList();
    }

    return ctxt;
}

int DSPro::registerDSProListener (uint16 ui16ClientId, DSProListener *pListener, uint16 &ui16AssignedClientId)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->registerDSProListener (ui16ClientId, pListener, ui16AssignedClientId);
}

int DSPro::deregisterDSProListener (uint16 ui16ClientId, DSProListener *pListener)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->deregisterDSProListener (ui16ClientId, pListener);
}

int DSPro::registerMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener, uint16 &ui16AssignedClientId)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->registerMatchmakingLogListener (ui16ClientId, pListener, ui16AssignedClientId);
}

int DSPro::deregisterMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pListener)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->deregisterMatchmakingLogListener (ui16ClientId, pListener);
}

int DSPro::registerCommAdaptorListener (uint16 ui16ClientId, CommAdaptorListener *pListener, uint16 &ui16AssignedClientId)
{
    checkInitialized;
    return _pImpl->registerCommAdaptorListener (ui16ClientId, pListener, ui16AssignedClientId);
}

int DSPro::deregisterCommAdaptorListener (uint16 ui16ClientId, CommAdaptorListener *pListener)
{
    checkInitialized;
    return _pImpl->deregisterCommAdaptorListener (ui16ClientId, pListener);
}

int DSPro::registerControlMessageListener (uint16 ui16ClientId, ControlMessageListener *pListener, uint16 &ui16AssignedClientId)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->registerControlMessageListener (ui16ClientId, pListener, ui16AssignedClientId);
}

int DSPro::deregisterControlMessageListener (uint16 ui16ClientId, ControlMessageListener *pListener)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->deregisterControlMessageListener (ui16ClientId, pListener);
}

int DSPro::registerSearchListener (uint16 ui16ClientId, SearchListener *pListener, uint16 &ui16AssignedClientId)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->registerSearchListener (ui16ClientId, pListener, ui16AssignedClientId);
}

int DSPro::deregisterSearchListener (uint16 ui16ClientId, SearchListener *pListener)
{
    checkInitialized;
    return _pImpl->getCallbackHandler()->deregisterSearchListener (ui16ClientId, pListener);
}

int DSPro::registerChunkFragmenter (const char *pszMimeType, ChunkerInterface *pChunker)
{
    checkInitialized;
    return _pImpl->registerChunkFragmenter (pszMimeType, pChunker);
}

int DSPro::registerChunkReassembler (const char *pszMimeType, ChunkReassemblerInterface *pReassembler)
{
    checkInitialized;
    return _pImpl->registerChunkReassembler (pszMimeType, pReassembler);
}

int DSPro::deregisterChunkFragmenter (const char *pszMimeType)
{
    checkInitialized;
    return _pImpl->deregisterChunkFragmenter (pszMimeType);
}

int DSPro::deregisterChunkReassembler (const char *pszMimeType)
{
    checkInitialized;
    return _pImpl->deregisterChunkReassembler (pszMimeType);
}

int DSPro::reloadCommAdaptors (void)
{
    checkInitialized;
    return 0;
}

void DSPro::resetTransmissionCounters (void)
{
    checkInitializedVoid;
    _pImpl->getCommAdaptorManager()->resetTransmissionCounters();
}

void DSPro::resetTransmissionHistory (void)
{
    checkInitializedVoid;
    // TODO: implement this
}
