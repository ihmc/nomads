/*
 * InformationPushPolicy.cpp
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

#include "InformationPushPolicy.h"

#include "Defs.h"
#include "InformationStore.h"
#include "MetadataInterface.h"
#include "Pedigree.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const char * InformationPushPolicy::PROACTIVE_REPLICATOR = "DATA_REPLICATOR";
const char * InformationPushPolicy::REACTIVE_REPLICATOR = "METADATA_REPLICATOR";

InformationPushPolicy::InformationPushPolicy (InformationStore *pInformationStore)
{
    if (pInformationStore == nullptr) {
        checkAndLogMsg ("InformationPushPolicy::InformationPushPolicy", coreComponentInitFailed);
        exit (-1);
    }
    _pInformationStore = pInformationStore;
}

InformationPushPolicy::~InformationPushPolicy()
{
}

MetadataInterface * InformationPushPolicy::getMetadata (const char *pszMessageID)
{
    return _pInformationStore->getMetadata (pszMessageID);
}

Ranks * InformationPushPolicy::process (Ranks *pRanks)
{
    if (pRanks == nullptr || pRanks->getFirst() == nullptr) {
        return nullptr;
    }

    Ranks *pRetRanks = new Ranks();
    for (Rank *pRank = pRanks->getFirst(); pRank != nullptr; pRank = pRanks->getNext()) {
        PtrLList<Rank> *pRanksInner = process (pRank);
        if (pRanksInner != nullptr) {
            for (Rank *pRankInner = pRanksInner->getFirst(); pRankInner != nullptr; pRankInner = pRanksInner->getNext()) {
                pRetRanks->insert (pRankInner);
            }
            delete pRanksInner;
        }
    }

    return pRetRanks;
}

InformationPushPolicy * InformationPushPolicy::getPolicy (const char *pszType, InformationStore *pInformationStore)
{
    const char *pszMethodName = "InformationPushPolicy::getPolicy";
    if (pszType != nullptr) {
        if (0 == stringcasecmp (pszType, PROACTIVE_REPLICATOR)) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "loading %s\n", pszType);
            return new ProactivePush (pInformationStore);
        }
        if (0 == stringcasecmp (pszType, REACTIVE_REPLICATOR)) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "loading %s\n", pszType);
            return new ReactivePush (pInformationStore);
        }
    }

    checkAndLogMsg (pszMethodName, Logger::L_Warning, "ReplicationManager type unknown %s.  "
                    "Loading the default %s\n", pszType, REACTIVE_REPLICATOR);
    return new ReactivePush (pInformationStore);
}

//------------------------------------------------------------------------------
// ProactiveReplicator
//------------------------------------------------------------------------------

ProactivePush::ProactivePush (InformationStore *pInformationStore)
    : InformationPushPolicy (pInformationStore)
{
}

ProactivePush::~ProactivePush (void)
{
}

Ranks * ProactivePush::process (Rank *pRank)
{
    MetadataInterface *pMetadata = getMetadata (pRank->_msgId.c_str());
    if (pMetadata == nullptr) {
        return nullptr;
    }

    // Get the referred data's ID
    String sReferredMessageId;
    if (pMetadata->getReferredDataMsgId (sReferredMessageId) < 0) {
        delete pMetadata;
        return nullptr;
    }
    delete pMetadata;
    pMetadata = nullptr;

    Ranks *pRanks = new Ranks();
    if (pRanks != nullptr) {
        Rank *pNewRank = new Rank (sReferredMessageId, pRank->_targetId, pRank->_rankByTarget,
                                   pRank->_bFiltered, pRank->_fTotalRank, pRank->_fTimeRank);
        if (pNewRank != nullptr) {
            pRanks->prepend (pNewRank);
        }

        pNewRank = pRank->clone();
        if (pNewRank != nullptr) {
            pRanks->prepend (pNewRank);
        }
    }

    if (pRanks != nullptr && pRanks->getFirst() == nullptr) {
        delete pRanks;
        pRanks = nullptr;
    }
    return pRanks;
}

//------------------------------------------------------------------------------
// ReactiveReplicator
//------------------------------------------------------------------------------

ReactivePush::ReactivePush (InformationStore *pInformationStore)
    : InformationPushPolicy (pInformationStore)
{
}

ReactivePush::~ReactivePush()
{
}

Ranks *  ReactivePush::process (Rank *pRank)
{
    Ranks *pRanks = new Ranks();
    if (pRanks != nullptr) {
        Rank *pRankClone = pRank->clone();
        if (pRankClone == nullptr) {
            delete pRanks;
            pRanks = nullptr;
        }
        else {
            pRanks->prepend (pRankClone);
        }
    }

    return pRanks;
}

