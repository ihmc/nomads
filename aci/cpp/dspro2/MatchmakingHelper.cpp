/* 
 * InformationStoreHelper.cpp
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
 * Created on September 11, 2013, 5:06 PM
 */

#include "MatchmakingHelper.h"

#include "DSPro.h"
#include "InformationStore.h"
#include "NodeContext.h"
#include "NodePath.h"
#include "TransmissionHistoryInterface.h"
#include "Topology.h"

#include "PtrLList.h"
#include "StrClass.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

MatchmakingHelper::MatchmakingHelper (InformationStore *pInfoStore, Topology *pTopology,
                                      TransmissionHistoryInterface *pTrHistory)
    : _pInfoStore (pInfoStore),
      _pTopology (pTopology),
      _pTrHistory (pTrHistory)
{
}

MatchmakingHelper::~MatchmakingHelper()
{
}

MetadataList * MatchmakingHelper::getMetadataToMatchmake (NodeContext *pNodeContext, const char *pszSenderNodeId,
                                                          bool bEnableTopologyExchange)
{
    // Get the messages to be matched against the new node context
    const char **ppszMessageIdFilters = _pTrHistory->getMessageList (pNodeContext->getNodeId(), pszSenderNodeId);
    PtrLList<String> *pPedigreeFilters = NULL;
    if (bEnableTopologyExchange) {
        pPedigreeFilters = _pTopology->getNeighbors (pNodeContext->getNodeId());
        if (pPedigreeFilters == NULL || pPedigreeFilters->getFirst() == NULL) {
            pPedigreeFilters = new PtrLList<String>();
            if (pPedigreeFilters != NULL) {
                String *pSender = new String (pszSenderNodeId);
                if (pSender != NULL) {
                    pPedigreeFilters->prepend (pSender);
                }
            }
        }
    }

    float fMaxLat, fMinLat, fMaxLong, fMinLong;
    uint32 ui32UsefulDistance = pNodeContext->getMaximumUsefulDistance();
    NodePath *pNodePath = pNodeContext->getPath();
    MetadataList *pMetadataList = NULL;
    if ((pNodePath != NULL) &&
        (ui32UsefulDistance > 0) &&
        (pNodePath->getBoundingBox (fMaxLat, fMinLat, fMaxLong, fMinLong, (float) ui32UsefulDistance) == 0)) {
        pMetadataList = _pInfoStore->getAllMetadataInArea (pNodeContext->getMatchmakingQualifiers(),
                                                           ppszMessageIdFilters,
                                                           fMaxLat, fMinLat, fMaxLong, fMinLong);
        // pMetadataList = _pInfoStore->getAllMetadataInArea (pNodeContext->getMatchmakingQualifiers(),
        //                                                   ppszMessageIdFilters, pszSenderNodeId, pPedigreeFilters,
        //                                                   fMaxLat, fMinLat, fMaxLong, fMinLong);
    }
    else {
        pMetadataList = _pInfoStore->getAllMetadata (ppszMessageIdFilters, true);
        // pMetadataList = _pInfoStore->getAllMetadata (ppszMessageIdFilters, true, pszSenderNodeId,
        //                                              pPedigreeFilters);
    }

    _pTrHistory->releaseList (ppszMessageIdFilters);
    return pMetadataList;
}

void MatchmakingHelper::deallocateMetadataToMatchmake (MetadataList *pMetadataList)
{
    MetadataInterface *pNext = pMetadataList->getFirst();
    for (MetadataInterface *pCurr; (pCurr = pNext) != NULL;) {
        delete pMetadataList->remove (pCurr);
        pNext = pMetadataList->getNext();
    }
    delete pMetadataList;
    pMetadataList = NULL;
}

