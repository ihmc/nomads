/*
 * MatchmakingHelper.cpp
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
#include "NodeContextImpl.h"
#include "NodePath.h"
#include "TransmissionHistoryInterface.h"
#include "Topology.h"

#include "PtrLList.h"
#include "StrClass.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace MATCHMAKING_HELPER
{
    BoundingBox getMaximumArea (NodeContext *pNodeContext)
    {
        const uint32 ui32Padding = pNodeContext->getMaximumUsefulDistance() +
            pNodeContext->getMaximumRangeOfInfluence();
        AreaOfInterestList *pAreasOfInterest = pNodeContext->getAreasOfInterest();
        NodePath *pNodePath = pNodeContext->getPath();
        if ((pAreasOfInterest == nullptr) || (pAreasOfInterest->isEmpty())) {
            if (pAreasOfInterest != nullptr) delete pAreasOfInterest;
            if (pNodePath == nullptr) {
                return BoundingBox();
            }
            // Only NodePath
            return pNodePath->getBoundingBox (ui32Padding);
        }
        BoundingBoxAccumulator acc (pAreasOfInterest->getFirst()->getBoundingBox());
        for (AreaOfInterest *pArea; (pArea = pAreasOfInterest->getNext()) != nullptr;) {
            const BoundingBox tmp (pArea->getBoundingBox());
            acc += tmp;
        }
        if (pNodePath != nullptr) {
            const BoundingBox tmp (pNodePath->getBoundingBox (ui32Padding));
            acc += tmp;
        }
        pAreasOfInterest->removeAll (true);
        delete pAreasOfInterest;
        return acc.getBoundingBox();
    }
}

MatchmakingHelper::MatchmakingHelper (InformationStore *pInfoStore, Topology *pTopology,
                                      TransmissionHistoryInterface *pTrHistory)
    : _pInfoStore (pInfoStore),
      _pTopology (pTopology),
      _pTrHistory (pTrHistory)
{
}

MatchmakingHelper::~MatchmakingHelper (void)
{
}

MetadataList * MatchmakingHelper::getMetadataToMatchmake (NodeContextImpl *pNodeContext, const char *pszSenderNodeId,
                                                          bool bEnableTopologyExchange, bool bLocalMatchmakingOnly) const
{
    // Get the messages to be matched against the new node context
    const String nodeId (pNodeContext->getNodeId());
    const char **ppszMessageIdFilters = _pTrHistory->getMessageList (nodeId, pszSenderNodeId);
    PtrLList<String> *pPedigreeFilters = nullptr;
    if (bEnableTopologyExchange) {
        pPedigreeFilters = _pTopology->getNeighbors (nodeId);
        if (pPedigreeFilters == nullptr || pPedigreeFilters->getFirst() == nullptr) {
            pPedigreeFilters = new PtrLList<String>();
            if (pPedigreeFilters != nullptr) {
                String *pSender = new String (pszSenderNodeId);
                if (pSender != nullptr) {
                    pPedigreeFilters->prepend (pSender);
                }
            }
        }
    }

    MetadataList *pMetadataList = nullptr;
    BoundingBox area (MATCHMAKING_HELPER::getMaximumArea (pNodeContext));
    if (area.isValid()) {
        pMetadataList = _pInfoStore->getAllMetadataInArea (pNodeContext->getMatchmakingQualifiers(),
                                                           ppszMessageIdFilters, area, bLocalMatchmakingOnly);
        // pMetadataList = _pInfoStore->getAllMetadataInArea (pNodeContext->getMatchmakingQualifiers(),
        //                                                   ppszMessageIdFilters, pszSenderNodeId, pPedigreeFilters,
        //                                                   area);
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
    for (MetadataInterface *pCurr; (pCurr = pNext) != nullptr;) {
        delete pMetadataList->remove (pCurr);
        pNext = pMetadataList->getNext();
    }
    delete pMetadataList;
    pMetadataList = nullptr;
}

