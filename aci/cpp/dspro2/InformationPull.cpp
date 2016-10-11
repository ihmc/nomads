/*
 * InformationPull.cpp
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

#include "InformationPull.h"

#include "Defs.h"
#include "InformationStore.h"
#include "LocalNodeContext.h"
#include "NodeContextManager.h"
#include "MetaDataRanker.h"
#include "MetadataRankerConfiguration.h"
#include "MetaData.h"

#include "BufferReader.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "SequentialArithmetic.h"
#include "Writer.h"
#include "MetadataConfiguration.h"

#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const float InformationPull::INFO_PULL_RANK_THRESHOLD = 0;
const bool InformationPull::DEFAULT_ENFORCE_RANK_BY_TIME = false;

const char * InformationPull::RANK_THRESHOLD_PROPERTY = "aci.dspro.informationPull.rankThreshold";
const char * InformationPull::ENFORCE_TIMING_PROPERTY = "aci.dspro.informationPull.enforceTiming";

InformationPull::InformationPull (const char *pszNodeId,
                                  MetadataRankerLocalConfiguration *pMetaDataRankerLocalConf,
                                  NodeContextManager *pNodeContextManager,
                                  InformationStore *pInformationStore,
                                  float infoPullRankThreashold)
    : _nodeId (pszNodeId), _latestSearchIdRcvdByPeer (true, true, true, true)
{
    if (pNodeContextManager == NULL || pInformationStore == NULL) {
        checkAndLogMsg ("InformationPush::InformationPull (2)", coreComponentInitFailed);
        exit (-1);
    }

    _pNodeContextManager = pNodeContextManager;
    _pInformationStore = pInformationStore;
    _pMetaDataRankerLocalConf = pMetaDataRankerLocalConf;
    _infoPullRankThreashold = infoPullRankThreashold;
    _ui32RemoteSeachQuery = 0;
}

InformationPull::~InformationPull()
{
    _pNodeContextManager = NULL;
    _pInformationStore = NULL;
    _pMetaDataRankerLocalConf = NULL;
}

int InformationPull::configure (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return 0;
    }
    if (pCfgMgr->hasValue (RANK_THRESHOLD_PROPERTY)) {
        _infoPullRankThreashold = pCfgMgr->getValueAsFloat (RANK_THRESHOLD_PROPERTY);
    }
    checkAndLogMsg ("InformationPull::configure", Logger::L_Info,
                    "PushRankThreashold: %f\n", _infoPullRankThreashold);

    return 0;
}

MatchmakingIntrumentation * InformationPull::dataArrived (MetaData *pMetaData)
{
    if (pMetaData == NULL) {
        return NULL;
    }

    if (MetadataUtils::refersToDataFromSource (pMetaData, _nodeId.c_str())) {
        char *pszMsgId = NULL;
        if (pMetaData->getFieldValue (MetaData::MESSAGE_ID, &pszMsgId) == 0 && pszMsgId != NULL) {
            NodeIdSet matchingNode (_nodeId);
            RankByTargetMap rhdRankByTarget;
            Rank *pRank = new Rank (pszMsgId, matchingNode, rhdRankByTarget, true);
            pRank->_loggingInfo._comment = "The referred data was published by the node itself";
            // Do not pull data from self!  They are likely to be in the cache
            // already, and if they are not it means that the node has canceled them,
            // it is therefore likely that they will not be necessary
            MatchmakingIntrumentation *pInstrumentation = new MatchmakingIntrumentation (true, _nodeId.c_str(), pRank);
            return pInstrumentation;
        }
        checkAndLogMsg ("InformationPull::dataArrived", memoryExhausted);
        return NULL;
    }

    LocalNodeContext *pLocalContext = _pNodeContextManager->getLocalNodeContext();
    Rank *pRank = MetaDataRanker::rank (pMetaData, pLocalContext,
                                        MetadataConfiguration::getExistingConfiguration(),
                                        _pMetaDataRankerLocalConf);
    _pNodeContextManager->releaseLocalNodeContext();

    if (pRank->_fTotalRank < _infoPullRankThreashold) {
        pRank->_fTotalRank = 0.0f;
    }
    return NULL; // pRank; //(pRank < _infoPullRankThreashold ? 0 : DSLib::toPriority (rank));
}

const char ** InformationPull::remoteSearchForMetadata (Writer *pWriter, const char *pszGroupName,
                                                        const char **ppszReferencedDataIDs)
{
    if (ppszReferencedDataIDs == NULL || ppszReferencedDataIDs[0] == NULL) {
        return NULL;
    }

    String query (MetaData::REFERS_TO);
    query += " = '";
    query += ppszReferencedDataIDs[0];
    query += "'";
    for (int i = 1; ppszReferencedDataIDs[i] != NULL; i++) {
        query += " OR";
        query += MetaData::REFERS_TO;
        query += " = '";
        query += ppszReferencedDataIDs[i];
        query += "'";
    }

    return remoteSearch (pWriter, pszGroupName, query);
}

const char ** InformationPull::remoteSearchForMetadata (Writer *pWriter, const char *pszGroupName,
                                                        const char *pszReferencedDataID)
{
    // Build query
    String query (MetaData::REFERS_TO);
    query += " = '";
    query += pszReferencedDataID;
    query += "'";

    return remoteSearch (pWriter, pszGroupName, query);
}

const char ** InformationPull::remoteSearch (Writer *pWriter, const char *pszGroupName, const char *pszQuery)
{
    if (pWriter == NULL || pszQuery == NULL) {
        return NULL;
    }
    char **ppszReceiverNodeIDs = _pNodeContextManager->getActivePeerList();
    if (ppszReceiverNodeIDs != NULL) {
        pWriter->write32 (&_ui32RemoteSeachQuery);
        uint16 ui16Len = (pszGroupName != NULL ? strlen (pszGroupName) : 0);
        pWriter->write16 (&ui16Len);
        if (ui16Len > 0) {
            pWriter->writeBytes (pszGroupName, ui16Len);
        }
        ui16Len = strlen (pszQuery);
        pWriter->write16 (&ui16Len);
        pWriter->writeBytes (pszQuery, ui16Len);
    }
    _ui32RemoteSeachQuery++;
    return (const char **) ppszReceiverNodeIDs;
}

PtrLList<const char> *  InformationPull::remoteSearchArrived (const void *pData, uint32 ui32DataLength,
                                                              uint32 &ui32RcvdRemoteSeachQuery,
                                                              const char *pszSenderNodeId)
{
    // Read the message
    BufferReader br (pData, ui32DataLength);
    br.read32 (&ui32RcvdRemoteSeachQuery);
    uint16 ui16Len;
    br.read16 (&ui16Len);
    char *pszGroupName = new char[ui16Len+1];
    br.readBytes (pszGroupName, ui16Len);
    pszGroupName[ui16Len] = '\0';
    br.read16 (&ui16Len);
    char *pszQuery = new char[ui16Len+1];
    br.readBytes (pszQuery, ui16Len);
    pszQuery[ui16Len] = '\0';

    // Check the search seq id
    uint32 *pUI32PrevRcvdRemoteSeachQuery = _latestSearchIdRcvdByPeer.get (pszSenderNodeId);
    if (pUI32PrevRcvdRemoteSeachQuery == NULL) {
        pUI32PrevRcvdRemoteSeachQuery = new uint32;
        (*pUI32PrevRcvdRemoteSeachQuery) = ui32RcvdRemoteSeachQuery;
        _latestSearchIdRcvdByPeer.put (pszSenderNodeId, pUI32PrevRcvdRemoteSeachQuery);
    }
    else {
        if (SequentialArithmetic::lessThanOrEqual (ui32RcvdRemoteSeachQuery, (*pUI32PrevRcvdRemoteSeachQuery))) {
            // This is either a duplicate search or an old search.  Either way
            // it must not be served! (Actually the "equal" case should never
            // happen).
            return NULL;
        }
        else {
            (*pUI32PrevRcvdRemoteSeachQuery) = ui32RcvdRemoteSeachQuery;
        }
    }

    // Get the IDs of the messages matching the query except the ones specified
    // in ppszFilters (because they have already been sent)
    char **ppszFilters = (char **) malloc (sizeof(char*) * 2);
    ppszFilters[0] = (char *) pszSenderNodeId;
    ppszFilters[1] = NULL;
    PtrLList<const char> *pRet = _pInformationStore->getMessageIDs (pszGroupName, pszQuery);
    ppszFilters[0] = NULL;
    delete ppszFilters;
    ppszFilters = NULL;
    return pRet;
}

