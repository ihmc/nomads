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

#include "ApplicationQueryController.h"
#include "ChunkQuery.h"
#include "CommAdaptor.h"
#include "Controller.h"
#include "ControlMessageNotifier.h"
#include "Defs.h"
#include "DisServiceAdaptor.h"
#include "DSSFLib.h"
#include "DSProImpl.h"
#include "DSProListener.h"
#include "InformationStore.h"
#include "LocalNodeContext.h"
#include "MetaData.h"
#include "MetadataConfiguration.h"
#include "NodeContextManager.h"
#include "NodePath.h"
#include "SQLAVList.h"

#include "Searches.h"

#include "Chunker.h"
#include "MimeUtils.h"

#include "BufferWriter.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "PtrLList.h"
#include "SearchProperties.h"

#define checkInitialized if (!_bInitialized) return -9999
#define checkInitializedNill if (!_bInitialized) return NULL
#define checkInitializedVoid if (!_bInitialized) return

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace IHMC_MISC;

namespace IHMC_ACI
{
    int sendCustumRequest (ChunkQuery &query, int64 i64TimeoutInMilliseconds, DSPro *pDSPro)
    {
        BufferWriter bw (1024, 1024);
        if (query.write (&bw) < 0) {
            return -1;
        }

        char *pszQueryId = NULL;
        const char *pszQualifiers = "";
        int rc = pDSPro->search (ChunkQuery::GROUP, ChunkQuery::QUERY_TYPE, pszQualifiers,
                                 bw.getBuffer(), bw.getBufferLength(), i64TimeoutInMilliseconds,
                                 &pszQueryId);
        if (rc < 0) {
            rc = -2;
        }
        if (pszQueryId != NULL) {
            free (pszQueryId);
        }
        else {
            rc = -3;
        }
        return rc;
    }
}

const char * const DSPro::NODE_NAME = "aci.dspro.nodename";
const char * const DSPro::ENABLE_DISSERVICE_ADAPTOR = "aci.dspro.adaptor.disservice.enable";
const char * const DSPro::ENABLE_MOCKETS_ADAPTOR = "aci.dspro.adaptor.mockets.enable";
const char * const DSPro::ENABLE_TCP_ADAPTOR = "aci.dspro.adaptor.tcp.enable";
const char * const DSPro::ENABLE_TOPOPLOGY_EXCHANGE = "aci.dspro.topologyExchange.enable";

const int64 DSPro::DEFAULT_UPDATE_TIMEOUT = 5000;
const int64 DSPro::DEFAULT_REPLICATE_TIMEOUT = 1000;

const char * const DSPro::SQL_QUERY_TYPE = "sql";

DSPro::DSPro (const char *pszNodeId, const char *pszVersion)
    : _bInitialized (false),
      _pImpl (new DSProImpl (pszNodeId, pszVersion)),
      _pMetadataConf (NULL)
{
}

DSPro::~DSPro (void)
{
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
    if ((pCfgMgr == NULL) || (_pImpl == NULL)) {
        return -1;
    }

    // Load Metadata Configuration
    _pMetadataConf = (pszMetadataExtraAttributes == NULL ?
                      MetadataConfiguration::getConfiguration() :
                      MetadataConfiguration::getConfiguration (pszMetadataExtraAttributes));
    if (_pMetadataConf == NULL) {
        return -3;
    }
    if (pszMetadataValues != NULL) {
        int rc = _pMetadataConf->setMetadataFields (pszMetadataValues);
        if (rc < 0) {
            return -4;
        }
    }

    if (_pImpl->init (pCfgMgr, _pMetadataConf) < 0) {
        return -5;
    }

    _bInitialized = true;
    return 0;
}

int DSPro::configureProperties (ConfigManager *pCfgMgr)
{
    checkInitialized;
    // TODO: implement this
    return 0;
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

int DSPro::addCustumPoliciesAsXML (const char **ppszCustomPoliciesXML)
{
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
    checkInitialized;
    return _pImpl->registerPath (pPath);
}

int DSPro::addUserId (const char *pszUserName)
{
    checkInitialized;
    return _pImpl->addUserId (pszUserName);
}

int DSPro::setMissionId (const char *pszMissionName)
{
    checkInitialized;
    if (pszMissionName == NULL) {
        return -1;
    }

    LocalNodeContext *pLocalNodeContext = _pImpl->getNodeContextManager()->getLocalNodeContext();
    int rc = pLocalNodeContext->setMissionId (pszMissionName);
    _pImpl->getNodeContextManager()->releaseLocalNodeContext();
    if (rc == 0) {
        checkAndLogMsg ("DSPro::addMissionId", Logger::L_Info, "added mission id %s.\n",
                        pszMissionName);
    }
    else if (rc < 0) {
        checkAndLogMsg ("DSPro::addMissionId", Logger::L_MildError, "could not add "
                        "user id %s. Return code %d.\n", pszMissionName, rc);
    }

    return (rc < 0 ? -4 : 0);
}

int DSPro::setCurrentPath (const char *pszPathID)
{
    checkInitialized;
    return _pImpl->setCurrentPath (pszPathID);
}

int DSPro::setCurrentPosition (float fLatitude, float fLongitude, float fAltitude,
                               const char *pszLocation, const char *pszNote)
{
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

NodePath * DSPro::getCurrentPath()
{
    checkInitializedNill;
    NodePath *pPath = _pImpl->getNodeContextManager()->getLocalNodeContext()->getPath();
    _pImpl->getNodeContextManager()->releaseLocalNodeContext();

    // TODO: make a copy!

    return pPath;
}

char ** DSPro::getPeerList (void)
{
    checkInitializedNill;
    return _pImpl->getPeerList();
}

int DSPro::getData (const char *pszId, const char *pszCallbackParameter,
                    void **ppData, uint32 &ui32DataLen, bool &bHasMoreChunks)
{
    checkInitialized;
    return _pImpl->getData (pszId, pszCallbackParameter, ppData, ui32DataLen, bHasMoreChunks);
}

int DSPro::release (const char *pszMessageID, void *pData)
{
    checkInitialized;
    if (pData != NULL) {
        free (pData);
        pData = NULL;
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
    checkInitializedNill;
    if (pszObjectId == NULL) {
        return NULL;
    }
    return _pImpl->getInformationStore()->getDSProIds (pszObjectId, pszInstanceId);
}

char ** DSPro::getMatchingMetadataAsXML (NOMADSUtil::AVList *pAVQueryList,
                                         int64 i64BeginArrivalTimestamp,
                                         int64 i64EndArrivalTimestamp)
{
    checkInitializedNill;
    return _pImpl->getMatchingMetadataAsXML (pAVQueryList, i64BeginArrivalTimestamp,
                                             i64EndArrivalTimestamp);
}

int DSPro::notUseful (const char *pszMessageId)
{
    checkInitialized;
    return _pImpl->notUseful (pszMessageId);
}

int DSPro::requestCustomAreaChunk (const char *pszAnnotatedObjMsgId, const char *pszMIMEType,
                                   uint32 ui32StartXPixel, uint32 ui32EndXPixel, uint32 ui32StartYPixel,
                                   uint32 ui32EndYPixel, uint8 ui8CompressionQuality, int64 i64TimeoutInMilliseconds)
{
    checkInitialized;
    if ((pszAnnotatedObjMsgId == NULL) || (pszMIMEType == NULL)) {
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
    checkInitialized;
    if ((pszAnnotatedObjMsgId == NULL) || (pszMIMEType == NULL)) {
        return -1;
    }
    if ((i64StartTimeInMillisec < 0) || (i64EndTimeInMillisec < 0) || (i64EndTimeInMillisec <= i64StartTimeInMillisec)) {
        return -2;
    }

    const Chunker::Type type = MimeUtils::mimeTypeToFragmentType (pszMIMEType);
    if (type == Chunker::UNSUPPORTED) {
        return -3;
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
    checkInitialized;
    return _pImpl->requestMoreChunks (pszChunkedMsgId, pszCallbackParameter);
}

int DSPro::search (const char *pszGroupName, const char *pszQueryType,
                   const char *pszQueryQualifiers, const void *pQuery,
                   unsigned int uiQueryLen, int64 i64TimeoutInMilliseconds,
                   char **ppszQueryId)
{
    checkInitialized;
    if (pszGroupName == NULL || pszQueryType == NULL || pszQueryQualifiers == NULL ||
        pQuery == NULL || uiQueryLen == 0 || ppszQueryId == NULL || (i64TimeoutInMilliseconds < 0)) {
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

    if ((_pImpl->search (searchProp, ppszQueryId) < 0) || (*ppszQueryId == NULL)) {
        return -2;
    }

    // Notify the search listeners
    searchProp.pszQueryId = *ppszQueryId;
    _pImpl->getCallbackHandler()->searchArrived (&searchProp);

    return 0;
}

int DSPro::searchReply (const char *pszQueryId, const char **ppszMatchingMsgIds)
{
    checkInitialized;
    if (pszQueryId == NULL || strlen (pszQueryId) == 0 || ppszMatchingMsgIds == NULL) {
        return -1;
    }

    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if (Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) == 0) {
        _pImpl->getCallbackHandler()->searchReplyArrived (pszQueryId, ppszMatchingMsgIds, getNodeId ());
    }

    return 0;
}

int DSPro::volatileSearchReply (const char *pszQueryId, const void *pReply, uint16 ui162ReplyLen)
{
    checkInitialized;
    if ((pszQueryId == NULL) || (strlen (pszQueryId)) == 0 || (pReply == NULL) || (ui162ReplyLen == 0)) {
        return -1;
    }

    uint16 ui16ClientId = 0;
    String queryType;
    String querier;
    if (Searches::getSearches()->getSearchInfo (pszQueryId, queryType, querier, ui16ClientId) == 0) {
        _pImpl->getCallbackHandler()->volatileSearchReplyArrived (pszQueryId, pReply, ui162ReplyLen, getNodeId ());
    }

    return 0;
}

int DSPro::addMessage (const char *pszGroupName, const char *pszObjectId,
                       const char *pszInstanceId, const char *pszXmlMedatada,
                       const void *pData, uint32 ui32DataLen,
                       int64 i64ExpirationTime, char **ppszId)
{
    checkInitialized;
    if (ppszId != NULL) {
        *ppszId = NULL;
    }
    if ((pszGroupName == NULL) || (pszXmlMedatada == NULL)) {
        return -1;
    }

    MetaData *pMetadata = _pMetadataConf->createMetadataFromXML (pszXmlMedatada);
    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                 pData, ui32DataLen, i64ExpirationTime, ppszId);
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::addMessage (const char *pszGroupName, const char *pszObjectId,
                       const char *pszInstanceId, SQLAVList *pMedatadaAttrList,
                       const void *pData, uint32 ui32DataLen,
                       int64 i64ExpirationTime, char **ppszId)
{
    checkInitialized;
    if (ppszId != NULL) {
        *ppszId = NULL;
    }
    if (pMedatadaAttrList == NULL) {
        return -1;
    }

    MetaData *pMetadata = _pMetadataConf->createNewMetadata (pMedatadaAttrList);
    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                 pData, ui32DataLen, i64ExpirationTime, ppszId);
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, const char *pszXmlMedatada,
                               const void *pData, uint32 ui32DataLen,
                               const char *pszDataMimeType, int64 i64ExpirationTime, char **ppszId)
{
    checkInitialized;
    if (ppszId != NULL) {
        *ppszId = NULL;
    }
    if (pszGroupName == NULL || pszXmlMedatada == NULL || pData == NULL || ui32DataLen == 0U || pszDataMimeType == NULL) {
        return -1;
    }

    MetaData *pMetadata = _pMetadataConf->createMetadataFromXML (pszXmlMedatada);
    int rc = _pImpl->chunkAndAddMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                         pData, ui32DataLen, pszDataMimeType, i64ExpirationTime, ppszId);
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::chunkAndAddMessage (const char *pszGroupName, const char *pszObjectId,
                               const char *pszInstanceId, SQLAVList *pMedatadaAttrList,
                               const void *pData, uint32 ui32DataLen, const char *pszDataMimeType,
                               int64 i64ExpirationTime, char **ppszId)
{
    checkInitialized;
    if (ppszId != NULL) {
        *ppszId = NULL;
    }
    if (pszGroupName == NULL || pMedatadaAttrList == NULL || pData == NULL || ui32DataLen == 0U || pszDataMimeType == NULL) {
        return -1;
    }

    MetaData *pMetadata = _pMetadataConf->createNewMetadata (pMedatadaAttrList);
    int rc = _pImpl->chunkAndAddMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                         pData, ui32DataLen, pszDataMimeType, i64ExpirationTime, ppszId);
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::addAnnotation (const char *pszGroupName, const char *pszObjectId,
                          const char *pszInstanceId, const char *pszXmlMedatada,
                          const char *pszReferredObject,
                          int64 i64ExpirationTime, char **ppszId)
{
    checkInitialized;
    if (pszXmlMedatada == NULL || pszReferredObject == NULL || ppszId == NULL) {
        return -1;
    }
    *ppszId = NULL;

    MetaData *pMetadata = _pMetadataConf->createMetadataFromXML (pszXmlMedatada);
    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                 NULL, 0, i64ExpirationTime, ppszId);
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::addAnnotation (const char *pszGroupName, const char *pszObjectId,
                          const char *pszInstanceId, SQLAVList *pMedatadaAttrList,
                          const char *pszReferredObject,
                          int64 i64ExpirationTime, char **ppszId)
{
    checkInitialized;
    if (ppszId != NULL) {
        *ppszId = NULL;
    }
    if (pMedatadaAttrList == NULL || pszReferredObject == NULL || ppszId == NULL) {
        return -1;
    }
    *ppszId = NULL;

    MetaData *pMetadata = _pMetadataConf->createNewMetadata (pMedatadaAttrList);
    int rc = _pImpl->addMessage (pszGroupName, pszObjectId, pszInstanceId, pMetadata,
                                 NULL, 0, i64ExpirationTime, ppszId);
    delete pMetadata;
    return (rc < 0 ? -4 : 0);
}

int DSPro::cancel (const char *pszId)
{
    checkInitialized;
    if (pszId == NULL) {
        return -1;
    }

    MetadataList *pMetadataList = _pImpl->getInformationStore()->getMetadataForData (pszId);
    if (pMetadataList == NULL || pMetadataList->getFirst() == NULL) {
        // It may be the case that the object to cancel is a metadata-only object,
        // in which case InfoStore::getMetadataForData() will always return null,
        // therefore I also need to check whether there is a metadata with id pszId
        MetadataInterface *pMetadata = _pImpl->getInformationStore()->getMetadata (pszId);
        if (pMetadata != NULL) {
            if (pMetadataList == NULL) {
                pMetadataList = new MetadataList();
            }
            if (pMetadataList != NULL) {
                pMetadataList->prepend (pMetadata);
            }
        }
    }

    // Remove all the elements contained in pMetadataList 
    if (pMetadataList != NULL) {
        MetadataInterface *pCurr, *pNext;
        pNext = pMetadataList->getFirst();
        for (unsigned int i = 0; ((pCurr = pNext) != NULL); i++) {
            pNext = pMetadataList->getNext();    
            char *pszMetadataID = NULL;
            if (pCurr->getFieldValue (MetaData::MESSAGE_ID, &pszMetadataID) == 0 &&
                pszMetadataID != NULL) {
                _pImpl->getInformationStore()->deleteMetadataFromDB (pszMetadataID);
            }
            delete pMetadataList->remove (pCurr);
        }
        delete pMetadataList;
    }

    return 0;
}

DisseminationService * DSPro::getDisService (void)
{
    checkInitializedNill;
    CommAdaptor *pCommAdaptor = _pImpl->getCommAdaptorManager()->getAdaptorByType (DISSERVICE);
    if (pCommAdaptor == NULL) {
        return NULL;
    }
    return static_cast<DisServiceAdaptor *>(pCommAdaptor)->getDisseminationService();
}

const char * DSPro::getNodeId (void) const
{
    checkInitializedNill;
    return _pImpl->getNodeId();
}

NOMADSUtil::String DSPro::getSessionId (void) const
{
    return _pImpl->getSessionId();
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

