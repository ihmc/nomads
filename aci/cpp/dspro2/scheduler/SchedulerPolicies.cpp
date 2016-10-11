/* 
 * SchedulerQueueManagment.cpp
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
 * Created on July 11, 2011, 7:20 PM
 */

#include "SchedulerPolicies.h"

#include "Defs.h"
#include "MetadataGenerator.h"
#include "Scheduler.h"

#include "ConfigManager.h"
#include "Logger.h"
#include "StrClass.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//------------------------------------------------------------------------------
// QueueReplacementPolicy
//------------------------------------------------------------------------------

QueueReplacementPolicy::QueueReplacementPolicy()
{
}

QueueReplacementPolicy::~QueueReplacementPolicy()
{
}

//------------------------------------------------------------------------------
// ReplaceAllPolicy
//------------------------------------------------------------------------------

ReplaceAllPolicy::ReplaceAllPolicy()
{
}

ReplaceAllPolicy::~ReplaceAllPolicy()
{
}

bool ReplaceAllPolicy::isReplaceable (Scheduler::MsgIDWrapper *pMsgIdWr)
{
    return true;
}

//------------------------------------------------------------------------------
// ReplaceNonePolicy
//------------------------------------------------------------------------------

ReplaceNonePolicy::ReplaceNonePolicy()
{
}

ReplaceNonePolicy::~ReplaceNonePolicy()
{
}

bool ReplaceNonePolicy::isReplaceable (Scheduler::MsgIDWrapper *pMsgIdWr)
{
    return false;
}

//------------------------------------------------------------------------------
// ReplaceLowPriorityPolicy
//------------------------------------------------------------------------------

ReplaceLowPriorityPolicy::ReplaceLowPriorityPolicy (float *pThresholds, unsigned int iLen)
{
    _pThresholds = (float*) malloc (sizeof(float)*iLen);
    memcpy (_pThresholds, pThresholds, sizeof(float)*iLen);
    _uiLen = iLen;
}

ReplaceLowPriorityPolicy::~ReplaceLowPriorityPolicy (void)
{
    free (_pThresholds);
}

bool ReplaceLowPriorityPolicy::isReplaceable (Scheduler::MsgIDWrapper *pMsgIdWr)
{
    switch (pMsgIdWr->_type) {
        case Scheduler::MsgIDWrapper::BiIndex:
            if (isReplaceable (((Scheduler::BiIndexMsgIDWrapper*) pMsgIdWr)->_fIndex2, 1)) {
                return true;
            }

        case Scheduler::MsgIDWrapper::MonoIndex:
            return (isReplaceable (pMsgIdWr->getFirstIndex(), 0));

        default:
            return true;
    }
}

bool ReplaceLowPriorityPolicy::isReplaceable (float indexValue, unsigned int uiThresholdIndex)
{
    if (uiThresholdIndex <= _uiLen) {
        return (indexValue < _pThresholds[uiThresholdIndex]);
    }
    return false;
}

//------------------------------------------------------------------------------
// QueueReplacementPolicyFactory
//------------------------------------------------------------------------------

QueueReplacementPolicy * QueueReplacementPolicyFactory::getQueueReplacementPolicy (ConfigManager *pCfgMgr)
{
    const char *pszMethodName = "QueueReplacementPolicyFactory::getQueueReplacementPolicy";
    const char *pszProperty = "aci.dspro.scheduler.queue.replacement";

    if (pCfgMgr == NULL ||
        !pCfgMgr->hasValue (pszProperty)) {
        return new ReplaceAllPolicy();
    }

    const char *pszReplacementPolicy = pCfgMgr->getValue (pszProperty);
    if (0 == stringcasecmp (pszReplacementPolicy, "REPLACE_ALL")) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s policy loaded", pszReplacementPolicy);
        return new ReplaceAllPolicy();
    }
    else if (0 == stringcasecmp (pszReplacementPolicy, "REPLACE_NONE")) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s policy loaded", pszReplacementPolicy);
        return new ReplaceNonePolicy();
    }
    else if (0 == stringcasecmp (pszReplacementPolicy, "REPLACE_LOW_INDEXES")) {        
        const char *pszValue;

        float totalRankThreshold;
        String property = (String) pszProperty + ".rank.total";
        if ((pszValue = pCfgMgr->getValue (property)) != NULL) {
            totalRankThreshold = (float) atof (pszValue);
        }
        else {
            totalRankThreshold = Scheduler::DEFAULT_UNREPLACEBLE_ENQUEUED_MSG_THREASHOLD;
        }

        float timeRankThreshold;
        property = (String) pszProperty + ".rank.time";
        if ((pszValue = pCfgMgr->getValue (property)) != NULL) {
            timeRankThreshold = (float) atof (pszValue);
        }
        else {
            timeRankThreshold = Scheduler::DEFAULT_UNREPLACEBLE_ENQUEUED_MSG_THREASHOLD;
        }

        float thresholds[2];
        if (pCfgMgr->getValueAsBool ("aci.dspro.informationPush.enforceTiming",
                                     Scheduler::DEFAULT_ENFORCE_RANK_BY_TIME)) {
            // The MsgIdWrapper's first index is the rank by time, the total rank
            // is the second index
            thresholds[0] = timeRankThreshold;
            thresholds[1] = totalRankThreshold;
        }
        else {
            // The MsgIdWrapper's first index is the total rank, the rank by time
            // is the second index
            thresholds[0] = totalRankThreshold;
            thresholds[1] = timeRankThreshold;
        }

        checkAndLogMsg (pszMethodName, Logger::L_Info, "%s policy loaded", pszReplacementPolicy);
        return new ReplaceLowPriorityPolicy (thresholds, 2);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Default policy loaded");
        return new ReplaceAllPolicy();
    }
}

//--------------------------------------------------------------------------
// Metadata Message Mutation Policies
//--------------------------------------------------------------------------

MetadataMutationPolicy::MetadataMutationPolicy()
{
}

MetadataMutationPolicy::~MetadataMutationPolicy()
{
}

DefaultMutationPolicy::DefaultMutationPolicy()
{
    
}

DefaultMutationPolicy::~DefaultMutationPolicy()
{
    
}

char * DefaultMutationPolicy::mutate (const char *, NodeIdSet &,
                                      RankByTargetMap &, Scheduler *)
{
    return NULL;
}

PrevMsgMutationPolicy::PrevMsgMutationPolicy (MetadataGenerator *pMutator)
{
    _pMutator = pMutator;
}

PrevMsgMutationPolicy::~PrevMsgMutationPolicy()
{
    
}

char * PrevMsgMutationPolicy::mutate (const char *pszOriginalMetadataId,
                                      NodeIdSet &targetNodes, RankByTargetMap &ranksByTarget,
                                      Scheduler *pScheduler)
{
    if ((pszOriginalMetadataId == NULL) || (targetNodes.isEmpty()) || (pScheduler == NULL)) {
        return NULL;
    }
    PreviousMessageIds prevMsgIds;
    for (NodeIdIterator iter = targetNodes.getIterator(); !iter.end(); iter.nextElement()) {
        prevMsgIds.add (iter.getKey(), pScheduler->getLatestMessageSentToTargetInternal (iter.getKey()));
    }
    return _pMutator->addPreviousMessageValue (pszOriginalMetadataId, prevMsgIds, ranksByTarget);
}

MetadataMutationPolicy * MetadataMutationPolicyFactory::getMetadataMutationPolicy (NOMADSUtil::ConfigManager *pCfgMgr, DSProImpl *pDSPro,
                                                                                   InformationStore *pInfoStore,
                                                                                   Scheduler::PrevPushedMsgInfoMode &prevPushedMsgInfo)
{
    const char *pszMethodName = "MetadataMutationPolicyFactory::getMetadataMutationPolicy";
    const char *pszProperty = "aci.dspro.scheduler.metadata.mutator";

    prevPushedMsgInfo = Scheduler::PREV_PUSH_MSG_INFO_DISABLED;

    if (pCfgMgr != NULL && pCfgMgr->hasValue (pszProperty)) {
        const char *pszMutationPolicy = pCfgMgr->getValue (pszProperty);
        if (0 == stringcasecmp (pszMutationPolicy, "PREV_MSG")) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "%s policy loaded\n", pszMutationPolicy);
            String prevPushedMsgInfoMode (pszProperty);
            prevPushedMsgInfoMode += ".prevMsg";
            const char *pszPrevPushedMsgInfoMode = pCfgMgr->getValue (prevPushedMsgInfoMode.c_str(), "DISABLED");
            if (0 == stringcasecmp (pszPrevPushedMsgInfoMode, "SESSION_AWARE")) {
                prevPushedMsgInfo = Scheduler::PREV_PUSH_MSG_INFO_SESSION_AWARE;
            }
            else if (0 == stringcasecmp (pszPrevPushedMsgInfoMode, "SESSIONLESS")) {
                prevPushedMsgInfo = Scheduler::PREV_PUSH_MSG_INFO_SESSIONLESS;
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "misconfiguration! If Metadata "
                                "Mutation Policy %s is set to PREV_MSG, then the %s property has to "
                                "be configured either as SESSION_AWARE or SESSIONLESS\n", pszProperty,
                                prevPushedMsgInfoMode.c_str());
                exit (-1);
            }

            return new PrevMsgMutationPolicy (new MetadataGenerator (pDSPro, pInfoStore));
        }
        if (0 != stringcasecmp (pszMutationPolicy, "DEFAULT")) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "unknown metadata mutation "
                            "policy %s. Setting default.\n", pszMutationPolicy);
        }
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "Default policy loaded\n");
    return new DefaultMutationPolicy();
}

