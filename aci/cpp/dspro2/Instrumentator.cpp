/*
 * Instrumentator.cpp
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

#include "Instrumentator.h"

#include "MatchmakingLogListener.h"

#include "Logger.h"
#include "Rank.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_ACI
{
    Instrumentator *pInstrumentator;
    NOMADSUtil::Logger *pNetLog;
}

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//==============================================================================
// Instrumentator
//==============================================================================

Instrumentator::Instrumentator (NOMADSUtil::LoggingMutex *pmCallback)
    : _matchmakerClients(1)
{
    _uiNListeners = 0;
    _pmCallback = pmCallback;
}

Instrumentator::~Instrumentator()
{
}

int Instrumentator::registerAndEnableMatchmakingLogListener (uint16 ui16ClientId, MatchmakingLogListener *pMatchmakingListener, uint16 &ui16AssignedClientId)
{
    if (_matchmakerClients.used (ui16ClientId) && _matchmakerClients[ui16ClientId].pMatchmakerListener != NULL) {
        checkAndLogMsg ("Instrumentator::registerAndEnableMatchmakingLogListener", Logger::L_SevereError,
                        "Client ID %d in use.  Register client using a different ui16ClientId.\n",
                        ui16ClientId);
        ui16ClientId = _matchmakerClients.firstFree();
    }
    _matchmakerClients[ui16ClientId].pMatchmakerListener = pMatchmakingListener;
    _uiNListeners++;
    ui16AssignedClientId = ui16ClientId;
    return 0;
}

int Instrumentator::deregisterAndDisableMatchmakingLogListener (uint16 ui16ClientId)
{
    if (_uiNListeners > 0) {
        _uiNListeners--;
    }
    return _matchmakerClients.clear(ui16ClientId);
}

void Instrumentator::notify (MatchmakingIntrumentation *pIntrumentation)
{
    if (pIntrumentation == NULL) {
        return;
    }

    unsigned int uiExpectedRanks = pIntrumentation->_pRank->_partialRanks.size();
    const char **ppszRankDescriptors;
    float *pRanks, *pWeights;
    if (uiExpectedRanks > 0) {
        ppszRankDescriptors = new const char*[uiExpectedRanks];
        pRanks = new float[uiExpectedRanks];
        pWeights = new float[uiExpectedRanks];
    }
    else {
        ppszRankDescriptors = NULL;
        pRanks = pWeights = NULL;
    }

    unsigned int uiActualRanks = 0;
    for (unsigned int i = 0; i < uiExpectedRanks; i++) {
        if ((pIntrumentation->_pRank->_partialRanks.used (i) != 0) &&
                (pIntrumentation->_pRank->_partialRanks[i]._partialRankDescription.length() > 0)) {

            ppszRankDescriptors[uiActualRanks] = pIntrumentation->_pRank->_partialRanks[i]._partialRankDescription.c_str();
            pRanks[uiActualRanks] = pIntrumentation->_pRank->_partialRanks[i]._partialRank;
            pWeights[uiActualRanks] = pIntrumentation->_pRank->_partialRanks[i]._partialRankWeigth;
            uiActualRanks++;
        }
    }

    // Notify each listener
    MatchmakingLogListener *pListener;
    _pmCallback->lock (2030);
    for (unsigned int i = 0; i < _matchmakerClients.size(); i++) {
        if (!_matchmakerClients.used (i)) {
            continue;
        }

        pListener = _matchmakerClients[i].pMatchmakerListener;
        if (pListener == NULL) {
            continue;
        }

        if (pIntrumentation->_bSkipped) {
            pListener->informationSkipped (pIntrumentation->_localNodeId,
                                           pIntrumentation->_pRank->_targetId,
                                           pIntrumentation->_pRank->_msgId,
                                           pIntrumentation->_pRank->_loggingInfo._objectName,
                                           ppszRankDescriptors, pRanks, pWeights,
                                           uiActualRanks,
                                           pIntrumentation->_pRank->_loggingInfo._comment,
                                           pIntrumentation->getType());
        }
        else {
            pListener->informationMatched (pIntrumentation->_localNodeId,
                                           pIntrumentation->_pRank->_targetId,
                                           pIntrumentation->_pRank->_msgId,
                                           pIntrumentation->_pRank->_loggingInfo._objectName,
                                           ppszRankDescriptors, pRanks, pWeights,
                                           uiActualRanks,
                                           pIntrumentation->_pRank->_loggingInfo._comment,
                                           pIntrumentation->getType());
        }
    }
    _pmCallback->unlock (2030);

    delete[] ppszRankDescriptors;   // I do not have to delete its elements!
    ppszRankDescriptors = NULL;     // pIntrumentation's destructor will take
    delete[] pRanks;                // care of it
    pRanks = NULL;
    delete[] pWeights;
    pWeights = NULL;
}

void Instrumentator::notifyAndRelease (PtrLList<MatchmakingIntrumentation> *pInstrumentations) 
{
    if (pInstrumentations != NULL) {
        MatchmakingIntrumentation *pNext = pInstrumentations->getFirst();
        for (MatchmakingIntrumentation *pCurr; (pCurr = pNext) != NULL;) {
            pNext = pInstrumentations->getNext();
 
            // ppInstrumentations[i]->setNextHopNodeId (pszNextHopNodeId);
            if (pInstrumentator != NULL) {
                pInstrumentator->notify (pCurr);
            }
            delete pInstrumentations->remove (pCurr);
        } 
        delete pInstrumentations;
    } 
}

//==============================================================================
// Instrumentator::MatchmakerClientInfoPro
//==============================================================================

Instrumentator::MatchmakerClientInfoPro::MatchmakerClientInfoPro()
{
    pMatchmakerListener = NULL;
}

Instrumentator::MatchmakerClientInfoPro::~MatchmakerClientInfoPro()
{
    pMatchmakerListener = NULL;
}
