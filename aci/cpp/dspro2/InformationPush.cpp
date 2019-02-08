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
#include "MetadataConfigurationImpl.h"
#include "MetaDataRanker.h"
#include "Scheduler.h"

#include "Voi.h"

#include "Logger.h"
#include "PtrLList.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace INFORMATION_PUSH
{
    Rank * toRank (Score *pScore)
    {
        if (pScore == nullptr) {
            return nullptr;
        }
        Score::Novelty novelty = pScore->novelty;
        Rank *pRank = pScore->pRank;
        pScore->pRank = nullptr;  // Otherwise the rank is deallocated with the call to Score's destructor!
        delete pScore;
        if (pRank == nullptr) {
            return nullptr;
        }
        switch (novelty) {
            case Score::CRITICAL:
                pRank->_fTotalRank = minimum (pRank->_fTotalRank * 1.5, 10.0f);
                break;

            case Score::INSIGNIFICANT:
                pRank->_fTotalRank = 0.0f;
                break;

            default:
                break;
        }
        return pRank;
    }

    Ranks * toRanks (ScoreList *pScores)
    {
        if (pScores == nullptr) {
            return nullptr;
        }
        Ranks *pRanks = new Ranks();
        if (pRanks != nullptr) {
            for (Score *pScore; (pScore = pScores->removeFirst ()) != nullptr;) {
                Rank *pRank = toRank (pScore);
                pRanks->append (pRank);
            }
        }
        delete pScores;
        return pRanks;
    }
}

InformationPush::InformationPush (const char *pszNodeId, Voi *pVoi,
                                  MetadataRankerLocalConfiguration *pMetaDataRankerLocalConf,
                                  NodeContextManager *pNodeContextManager,
                                  InformationPushPolicy *pPolicy, Scheduler *pScheduler)
    : _nodeId (pszNodeId),
      _pNodeContextManager (pNodeContextManager),
      _pPolicy (pPolicy),
      _pScheduler (pScheduler),
      _pMetaDataRankerLocalConf (pMetaDataRankerLocalConf),
      _pVoi (pVoi)
{
    assert (_pNodeContextManager != nullptr);
    assert (_pPolicy != nullptr);
    assert (_pScheduler != nullptr);
    assert (_pMetaDataRankerLocalConf != nullptr);
}

InformationPush::~InformationPush (void)
{
    _pNodeContextManager = nullptr;
    delete _pPolicy;
    _pPolicy = nullptr;
    _pMetaDataRankerLocalConf = nullptr;
    delete _pVoi;
}

Instrumentations * InformationPush::dataArrived (MetaData *pMetaData, PeerNodeContextList *pPeerNodeContextList)
{
    const char *pszMethodName = "InformationPush::dataArrived";
    if (pPeerNodeContextList == nullptr) {
        return nullptr;
    }

    unsigned int uiActivePeerNumber = _pNodeContextManager->getActivePeerNumber();
    checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "active peer number = %d\n",
                    uiActivePeerNumber);
    if (uiActivePeerNumber == 0) {
        return nullptr;
    }

    Instrumentations *pInstrumentations = new Instrumentations();
    Rank *pMergedRank = nullptr;
    unsigned int uiNMatchedPeers = 0;
    const int64 i64MatchmakingStartTime = getTimeInMilliseconds();
    for (PeerNodeContext *pCurrPeer = pPeerNodeContextList->getFirst(); pCurrPeer != nullptr;
            pCurrPeer = pPeerNodeContextList->getNext(), uiNMatchedPeers++) {

        if (pCurrPeer->isPeerActive()) {
            const String nodeId (pCurrPeer->getNodeId());
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Peer with ID = <%s> is active. "
                            "Calling single rank\n", nodeId.c_str());

            // Call ranker
            const float fMatchThreashold = pCurrPeer->getMatchmakingThreshold();
            Rank *pRank = INFORMATION_PUSH::toRank (_pVoi->getVoi (pMetaData, pCurrPeer,
                                                                   MetadataConfigurationImpl::getExistingConfiguration(),
                                                                   _pMetaDataRankerLocalConf));
            if (pRank != nullptr) {
                const bool bSkip = ((pRank->_bFiltered) || (pRank->_fTotalRank < fMatchThreashold));
                if (bSkip) {
                    MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pRank, bSkip, fMatchThreashold);
                    if (pInstrumentation != nullptr) {
                        pInstrumentations->prepend (pInstrumentation);
                    }
                }
                else {
                    if (pMergedRank == nullptr) {
                        pMergedRank = new Rank (*pRank);
                        if (pMergedRank == nullptr) {
                            checkAndLogMsg (pszMethodName, memoryExhausted);
                            break;
                        }
                    }
                    else {
                        (*pMergedRank) += (*pRank);
                    }
                    delete pRank;
                }
            }
        }
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "matchmaking for %u peers took %lld millisec.\n",
                    uiNMatchedPeers, (getTimeInMilliseconds() - i64MatchmakingStartTime));

    if (pMergedRank == nullptr) {
        return pInstrumentations;
    }
    MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pMergedRank, false, 0.0f);
    if (pInstrumentation != nullptr) {
        pInstrumentations->prepend (pInstrumentation);
    }

    // If the metadata is not skipped, add it to the scheduler
    Ranks *pProcessedRanks = _pPolicy->process (pMergedRank);
    if (pProcessedRanks == nullptr) {
        _pScheduler->addToCurrentPreStaging (pMergedRank);
    }
    else {
      //  delete pMergedRank;
      //  pMergedRank = nullptr;
         // _pPolicy may add some related messages to add to the scheduler
        for (Rank *pProcessRank = pProcessedRanks->getFirst();
             pProcessRank != nullptr; pProcessRank = pProcessedRanks->getNext()) {
            _pScheduler->addToCurrentPreStaging (pProcessRank);
        }
    }

    _pScheduler->send();

    return pInstrumentations;
}

/* TODO: */
Instrumentations * InformationPush::nodeContextChanged (MetadataList *pMetadataList,
                                                        PeerNodeContext *pPeerContext)
{
    const char *pszMethodName = "InformationPush::nodeContextChanged";

    if ((pMetadataList == nullptr) || (pMetadataList->getFirst() == nullptr)) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "pMetadataList is nullptr or an empty list.\n");
        return nullptr;
    }
    if (pPeerContext == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "PeerNodeContext is nullptr.\n");
        return nullptr;
    }

    Ranks *pRanks = INFORMATION_PUSH::toRanks (_pVoi->getVoi (pMetadataList, pPeerContext,
                                                              MetadataConfigurationImpl::getExistingConfiguration(),
                                                              _pMetaDataRankerLocalConf));
    if (pRanks == nullptr) {
        return nullptr;
    }

    const String targetNodeId (pPeerContext->getNodeId());
    Instrumentations *pInstrumentations = new Instrumentations();
    Rank *pNext = pRanks->getFirst();
    unsigned int uiNMatchedMessages = 0;
    const float fMatchThreashold = pPeerContext->getMatchmakingThreshold();
    const int64 i64MatchmakingStartTime = getTimeInMilliseconds();
    for (Rank *pRank; (pRank = pNext) != nullptr; uiNMatchedMessages++) {
        pNext = pRanks->getNext();
        const bool bSkip = ((pRank->_bFiltered) || (pRank->_fTotalRank < fMatchThreashold));
        if (bSkip) {
            pRanks->remove (pRank);
            // Only add ranks for skipped messages for now.  The ranks for the
            // matched messages will be added later (after processing, if needed)
            MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pRank, bSkip, fMatchThreashold);
            if (pInstrumentation != nullptr) {
                pInstrumentations->prepend (pInstrumentation);
            }
        }
        else if (targetNodeId != ((const char *)pRank->_targetId)) {
            // Sanity check
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "pRank->_targetId was set "
                            "to <%s> instead of <%s>.\n",  (const char *) pRank->_targetId,
                            targetNodeId.c_str());
        }
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "matchmaking %u messages for peer %s took %lld millisec.\n",
                    uiNMatchedMessages, targetNodeId.c_str(), (getTimeInMilliseconds() - i64MatchmakingStartTime));

    Ranks *pProcessedRanks = _pPolicy->process (pRanks);
    Ranks *pRanksToAddToScheduler = (pProcessedRanks == nullptr ? pRanks : pProcessedRanks);
    _pScheduler->startNewPreStagingForPeer (targetNodeId, pRanksToAddToScheduler);
    for (Rank *pRank = pRanks->getFirst(); pRank != nullptr; pRank = pRanks->getNext()) {
        MatchmakingIntrumentation *pInstrumentation = createInstrumentation (pRank, false, fMatchThreashold);
        if (pInstrumentation != nullptr) {
            pInstrumentations->prepend (pInstrumentation);
        }
    }
    if (pProcessedRanks != nullptr) {
        Rank *pNext = pProcessedRanks->getFirst();
        for (Rank *pRank; (pRank = pNext) != nullptr;) {
            pNext = pProcessedRanks->getNext();
            delete pProcessedRanks->remove (pRank);
        }
    }

    _pScheduler->send();

    return pInstrumentations;
}

MatchmakingIntrumentation * InformationPush::createInstrumentation (Rank *pRank, bool bSkipped, float fMatchThreashold)
{
    if (pRank == nullptr) {
        return nullptr;
    }
    return new MatchmakingIntrumentation (bSkipped, _nodeId, fMatchThreashold, pRank);
}

