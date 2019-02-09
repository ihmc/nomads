/*
 * VoiImpl.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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
 * Created on Febraury 15, 2017, 2:38 PM
 */

#include "VoiImpl.h"

#include "MetadataImpl.h"
#include "MetaDataRanker.h"
#include "Comparator.h"
#include "Voi.h"

#include "Json.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace VOI_IMPL
{
    MetadataConfiguration * set (MetadataConfiguration *pMetadataCfg, MetadataConfiguration *pDef)
    {
        return (pMetadataCfg == NULL ? pDef : pMetadataCfg);
    }

    MetadataRankerLocalConfiguration * set (MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg, MetadataRankerLocalConfiguration *pDef)
    {
        return (pMetadataRankerLocalCfg == NULL ? pDef : pMetadataRankerLocalCfg);
    }

    void logMatchmaking (Score *pScore, const String &nodeId)
    {
        if ((pScore == NULL) || (pScore->pRank == NULL) || (pNetLog == NULL)) {
            return;
        }
        const char *pszMsgId = pScore->pRank->_msgId.c_str();
        if (pszMsgId == NULL) pszMsgId = "";
        if (pScore->pRank->_bFiltered) {
            pNetLog->logMsg ("VoiImpl::logMatchmaking", Logger::L_LowDetailDebug, "Message <%s> filtered.\n", pszMsgId);
            return;
        }
        pNetLog->logMsg ("VoiImpl::logMatchmaking", Logger::L_Info, "Msg: <%s>. Node <%s>. Rank: %f. Novelty: %s. Comment %s\n",
                         pszMsgId, nodeId.c_str(), pScore->pRank->_fTotalRank, toString (pScore->novelty),
                         pScore->pRank->_loggingInfo._comment.c_str());
    }
}

using namespace VOI_IMPL;

VoiImpl::VoiImpl (const char *pszSessionId, bool bInstrumented)
    : _bInstrumented (bInstrumented),
      _cache (pszSessionId),
      _pMetadataCfg (NULL),
      _pMetadataRankerLocalCfg (NULL)
{
}

VoiImpl::~VoiImpl (void)
{
}

int VoiImpl::init (void)
{
    return _cache.init();
}

void VoiImpl::addMetadataForPeer (const char *pszNodeId, InformationObject *pIO)
{
    if (pIO == NULL) {
        return;
    }
    MetadataInterface *pMetadata = pIO->getMetadata();
    if (pMetadata == NULL) {
        return;
    }
    EntityInfo ei;
    pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_OBJECT_ID, ei._objectId);
    if (ei._objectId.length()<= 0) {
        return;
    }
    pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_INSTANCE_ID, ei._instanceId);
    if (ei._instanceId.length() <= 0) {
        return;
    }
    if (!pMetadata->isFieldUnknown (MetadataInterface::SOURCE_TIME_STAMP)) {
        // Otherwise assume the instance id is sequential
        pMetadata->getFieldValue (MetadataInterface::SOURCE_TIME_STAMP, &ei._i64SourceTimeStamp);
    }

    uint32 ui32DataLen = 0U;
    const void *pData = pIO->getData (ui32DataLen);
    _cache.add (pszNodeId, ei, pMetadata, pData, ui32DataLen);
}

Score * VoiImpl::getVoi (InformationObject *pIo, NodeContext *pNodeCtxt,
                         MetadataConfiguration *pMetadataCfg,
                         MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg) const
{
    MetadataInterface *pMetadata = pIo->getMetadata();
    Rank *pRank = MetaDataRanker::rank (pMetadata, pNodeCtxt, pMetadataCfg, pMetadataRankerLocalCfg);
    Score *pScore = new Score (Score::SIGNIFICANT, pRank);
    if ((pRank != NULL) && (pScore != NULL)) {
        InformationObjects ioHistory;
        String objectId;
        pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_OBJECT_ID, objectId);
        if (objectId.length() <= 0) {
            return pScore;
        }
        const String nodeId (pNodeCtxt->getNodeId());
        _cache.getPreviousVersions (nodeId, objectId, 1, ioHistory);
        InformationObject *pOld = ioHistory.getFirst();
        const MetadataInterface *pOldMetadata = pOld == NULL ? NULL : pOld->getMetadata();
        if (pScore->pRank->_bFiltered) {
            pScore->novelty = Score::INSIGNIFICANT;
        }
        else {
            pScore->novelty = Comparator::compare (pOldMetadata, pIo->getMetadata(), pNodeCtxt, pMetadataRankerLocalCfg);
        }
        logMatchmaking (pScore, nodeId);
    }
    return pScore;
}

ScoreList * VoiImpl::getVoi (InformationObjects *pIOList, NodeContext *pNodeContext,
                             MetadataConfiguration *pMetadataCfg,
                             MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg) const
{
    ScoreList *pScores = new ScoreList();
    if (pScores != NULL) {
        for (InformationObject *pIO = pIOList->getFirst(); pIO != NULL; pIO = pIOList->getNext()) {
            Score *pScore = getVoi (pIO, pNodeContext, pMetadataCfg, pMetadataRankerLocalCfg);
            if (pScore != NULL) {
                pScores->insert (pScore);
            }
        }
    }
    return pScores;
}

ScoreList * VoiImpl::getVoi (InformationObjects *pIOList, NodeContextList *pNodeCtxtList,
                             MetadataConfiguration *pMetadataCfg,
                             MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg) const
{
    ScoreList *pScores = new ScoreList();
    if (pScores != NULL) {
        for (NodeContext *pNodeCtxt = pNodeCtxtList->getFirst(); pNodeCtxt != NULL; pNodeCtxt = pNodeCtxtList->getNext()) {
            ScoreList *pPartialScores = getVoi (pIOList, pNodeCtxt, pMetadataCfg, pMetadataRankerLocalCfg);
            if (pPartialScores != NULL) {
                pScores->insertAll (pPartialScores);
            }
        }
    }
    return pScores;
}

