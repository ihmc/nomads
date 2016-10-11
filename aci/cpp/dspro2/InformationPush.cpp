/*
 * InformationPush.cpp
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

#include "InformationPush.h"

#include "Defs.h"
#include "NodeContextManager.h"
#include "InformationPushPolicy.h"
#include "MetaData.h"
#include "MetadataConfiguration.h"
#include "MetaDataRanker.h"
#include "Scheduler.h"

#include "Logger.h"
#include "PtrLList.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

InformationPush::InformationPush (const char *pszNodeId,
                                  MetadataRankerLocalConfiguration *pMetaDataRankerLocalConf,
                                  NodeContextManager *pNodeContextManager,
                                  InformationPushPolicy *pPolicy, Scheduler *pScheduler)
    : _nodeId (pszNodeId)
{
    if (pNodeContextManager == NULL || pPolicy == NULL || pScheduler == NULL) {
        checkAndLogMsg ("InformationPush::InformationPush", Logger::L_SevereError,
                        "InformationPush could not be initialized. Quitting.\n");
        exit (-1);
    }

    _pNodeContextManager = pNodeContextManager;
    _pPolicy = pPolicy;
    _pScheduler = pScheduler;
    _pMetaDataRankerLocalConf = pMetaDataRankerLocalConf;
}

InformationPush::~InformationPush()
{
    _pNodeContextManager = NULL;
    delete _pPolicy;
    _pPolicy = NULL;
    _pMetaDataRankerLocalConf = NULL;
}

Instrumentations * InformationPush::dataArrived (MetaData *pMetaData, PeerNodeContextList *pPeerNodeContextList)
{
    const char *pszMethodName = "InformationPush::dataArrived";
    if (pPeerNodeContextList == NULL) {
        return NULL;
    }

    unsigned int uiActivePeerNumber = _pNodeContextManager->getActivePeerNumber();
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "active peer number = %d\n",
                    uiActivePeerNumber);
    if (uiActivePeerNumber == 0) {
        return NULL;
    }

    Instrumentations *pInstrumentations = new Instrumentations();
    Rank *pMergedRank = NULL;
    unsigned int uiNMatchedPeers = 0;
    const int64 i64MatchmakingStartTime = getTimeInMilliseconds();
    for (PeerNodeContext *pCurrPeer = pPeerNodeContextList->getFirst(); pCurrPeer != NULL;
            pCurrPeer = pPeerNodeContextList->getNext(), uiNMatchedPeers++) {

        if (pCurrPeer->isPeerActive()) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Peer with ID = <%s> is active. "
                            "Calling single rank\n", pCurrPeer->getNodeId());

            // Call ranker
            const float fMatchThreashold = pCurrPeer->getMatchmakingThreshold();
            Rank *pRank = MetaDataRanker::rank (pMetaData, pCurrPeer,
                                                MetadataConfiguration::getExistingConfiguration(),
                                                _pMetaDataRankerLocalConf);
            if (pRank != NULL) {
                const bool bSkip = ((pRank->_bFiltered) || (pRank->_fTotalRank < fMatchThreashold));
                if (bSkip) {
                    MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pRank, bSkip, fMatchThreashold);
                    if (pInstrumentation != NULL) {
                        pInstrumentations->prepend (pInstrumentation);
                    }
                }
                else {
                    if (pMergedRank == NULL) {
                        pMergedRank = new Rank (*pRank);
                        if (pMergedRank == NULL) {
                            checkAndLogMsg (pszMethodName, memoryExhausted);
                            break;
                        }
                    }
                    else {
                        (*pMergedRank) += (*pRank);
                    }
                }
            }
        }
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "matchmaking for %u peers took %lld millisec.\n",
                    uiNMatchedPeers, (getTimeInMilliseconds() - i64MatchmakingStartTime));

    if (pMergedRank == NULL) {
        return pInstrumentations;
    }
    MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pMergedRank, false, 0.0f);
    if (pInstrumentation != NULL) {
        pInstrumentations->prepend (pInstrumentation);
    }

    // If the metadata is not skipped, add it to the scheduler
    Ranks *pProcessedRanks = _pPolicy->process (pMergedRank);
    if (pProcessedRanks == NULL) {
        _pScheduler->addToCurrentPreStaging (pMergedRank);
    }
    else {
      //  delete pMergedRank;
      //  pMergedRank = NULL;
         // _pPolicy may add some related messages to add to the scheduler
        for (Rank *pProcessRank = pProcessedRanks->getFirst();
             pProcessRank != NULL; pProcessRank = pProcessedRanks->getNext()) {
            _pScheduler->addToCurrentPreStaging (pProcessRank);
        }
    }

    _pScheduler->send();

    return pInstrumentations;
}

Instrumentations * InformationPush::nodeContextChanged (MetadataList *pMetadataList,
                                                        PeerNodeContext *pPeerContext)
{
    const char *pszMethodName = "InformationPush::nodeContextChanged";

    if ((pMetadataList == NULL) || (pMetadataList->getFirst() == NULL)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "NULL or empty pFields list.\n");
        return NULL;
    }
    if (pPeerContext == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "NULL PeerNodeContext.\n");
        return NULL;
    }

    Ranks *pRanks = MetaDataRanker::rank (pMetadataList, pPeerContext,
                                          MetadataConfiguration::getExistingConfiguration(),
                                          _pMetaDataRankerLocalConf);
    if (pRanks == NULL) {
        return NULL;
    }

    const String targetNodeId (pPeerContext->getNodeId());
    Instrumentations *pInstrumentations = new Instrumentations();
    Rank *pNext = pRanks->getFirst();
    unsigned int uiNMatchedMessages = 0;
    const float fMatchThreashold = pPeerContext->getMatchmakingThreshold();
    const int64 i64MatchmakingStartTime = getTimeInMilliseconds();
    for (Rank *pRank; (pRank = pNext) != NULL; uiNMatchedMessages++) {
        pNext = pRanks->getNext();
        const bool bSkip = ((pRank->_bFiltered) || (pRank->_fTotalRank < fMatchThreashold));
        if (bSkip) {
            pRanks->remove (pRank);
            // Only add ranks for skipped messages for now.  The ranks for the
            // matched messages will be added later (after processing, if needed)
            MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pRank, bSkip, fMatchThreashold);
            if (pInstrumentation != NULL) {
                pInstrumentations->prepend (pInstrumentation);
            }
        }
        else if (targetNodeId != ((const char *)pRank->_targetId)) {
            // Sanity check
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "pRank->_targetId was set "
                            "to <%s> instead of <%s>.\n",  (const char *) pRank->_targetId,
                            (const char *) targetNodeId);           
        }
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "matchmaking %u messages for peer %s took %lld millisec.\n",
                    uiNMatchedMessages, targetNodeId.c_str(), (getTimeInMilliseconds() - i64MatchmakingStartTime));

    Ranks *pProcessedRanks = _pPolicy->process (pRanks);
    Ranks *pRanksToAddToScheduler = (pProcessedRanks == NULL ? pRanks : pProcessedRanks);
    _pScheduler->startNewPreStagingForPeer (targetNodeId, pRanksToAddToScheduler);
    for (Rank *pRank = pRanks->getFirst(); pRank != NULL; pRank = pRanks->getNext()) {
        MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pRank, false, fMatchThreashold);
        if (pInstrumentation != NULL) {
            pInstrumentations->prepend (pInstrumentation);
        }
    }
    if (pProcessedRanks != NULL) {
        Rank *pNext = pProcessedRanks->getFirst();
        for (Rank *pRank; (pRank = pNext) != NULL;) {
            pNext = pProcessedRanks->getNext();
            delete pProcessedRanks->remove (pRank);
        }
    }

    _pScheduler->send();

    return pInstrumentations;
}

MatchmakingIntrumentation * InformationPush::createInstrumentation (Rank *pRank, bool bSkipped, float fMatchThreashold)
{
    if (pRank == NULL) {
        return NULL;
    }
    return new MatchmakingIntrumentation (bSkipped, _nodeId, fMatchThreashold, pRank);
}

