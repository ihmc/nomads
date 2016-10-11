/* 
 * Scheduler.cpp
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
 * Created on December 14, 2010, 6:00 PM
 */

#include "Scheduler.h"

#include "CommAdaptorManager.h"
#include "ConfigManager.h"
#include "Controller.h"
#include "DataStore.h"
#include "Defs.h"
#include "DSLib.h"
#include "MetadataRankerConfiguration.h"
#include "NodeContextManager.h"
#include "SchedulerPolicies.h"
#include "Targets.h"
#include "Topology.h"
#include "TransmissionHistoryInterface.h"
#include "Utils.h"

#include "Message.h"
#include "MessageInfo.h"

#include "Logger.h"
#include "StringTokenizer.h"

#define isInstrumented (pInstrumentator != NULL) && pInstrumentator->isEnabled()

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const unsigned int Scheduler::DEFAULT_MAX_N_OUTGOING_MSGS = 5;
const unsigned int Scheduler::DEFAULT_OUTGOING_QUEUE_SIZE_THREASHOLD = 0;
const bool Scheduler::DEFAULT_RUN_SCHEDULER_THREAD = true;
const bool Scheduler::DEFAULT_ENFORCE_RANK_BY_TIME = true;
const float Scheduler::DEFAULT_UNREPLACEBLE_ENQUEUED_MSG_THREASHOLD = -1.0f;

const float Scheduler::INDEX_UNSET = -1.0f;

const char * Scheduler::ENFORCE_TIMING_PROPERTY = "aci.dspro.scheduler.enforceTiming";
const char * Scheduler::PRESTAGING_MESSAGE_THRESHOLD_PROPERTY = "aci.dspro.scheduler.preStagingSession.messages.threashold";
const char * Scheduler::OUTGOING_MSG_THRESHOLD_STRICT_PROPERTY = "aci.dspro.scheduler.preStagingSession.messages.queue.outgoing.threashold";
const char * Scheduler::RUN_SCHEDULER_PROPERTY = "aci.dspro.scheduler.thread.run";
const char * Scheduler::PREV_ID_MODE_PROPERTY = "aci.dspro.scheduler.metadata.mutator.prevMsg";
const char * Scheduler::TIME_SENSITIVE_MIME_TYPES_PROPERTY = "aci.dspro.scheduler.timeSensitiveTypes";

Scheduler::Scheduler (bool bEnforceTiming, bool bUseSizeOverUtilityRatio,
                      unsigned int uiMaxNMsgPerSession,
                      unsigned int uiOutgoingQueueSizeThreashold,
                      CommAdaptorManager *pAdaptorMgr,
                      DataStore *pDataStore, NodeContextManager *pNodeCtxtMgr,
                      Topology *pTopology, TransmissionHistoryInterface *pTrHistory,
                      const QueueReplacementPolicy *pReplacementPolicy,
                      const MetadataMutationPolicy *pMutatorPolicy,
                      PrevPushedMsgInfoMode preStagingSessionAwarePrevMsgID)
    : _bEnforceTiming (bEnforceTiming),
      _bUseSizeOverUtilityRatio (bUseSizeOverUtilityRatio),
      _preStagingSessionAwarePrevMsgID (preStagingSessionAwarePrevMsgID),
      _uiMaxNMsgPerSession (uiMaxNMsgPerSession),
      _uiOutgoingQueueSizeThreashold (uiOutgoingQueueSizeThreashold),
      _msgReqSrv (pDataStore),
      _m (MutexId::Scheduler_m, LOG_MUTEX),
      _mQueues (MutexId::Scheduler_mQueues, LOG_MUTEX),
      _mEnqueuedRequests (MutexId::Scheduler_mRequests, LOG_MUTEX),
      _mEnqueuedMessageIdWrappers (MutexId::Scheduler_mMessageIdWrappers, LOG_MUTEX),
      _enqueuedMessageIdWrappers (true,  // bCaseSensitiveKeys
                                  true,  // bCloneKeys
                                  true,  // bDeleteKeys
                                  true), // bDeleteValues
      _queues (true,    // bCaseSensitiveKeys
               true,    // bCloneKeys
               true,    // bDeleteKeys
               true),   // bDeleteValues
      _latestMsgPushedByTarget (pDataStore->getPropertyStore()) // bDeleteValues
      
{
    _pAdaptorMgr = pAdaptorMgr;
    _pDataStore = pDataStore;
    _pNodeCtxtMgr = pNodeCtxtMgr;
    _pTopology = pTopology;
    _pTrHistory = pTrHistory;
    _pReplacementPolicy = pReplacementPolicy;
    _pMMutationPolicy = pMutatorPolicy;

    if (_pReplacementPolicy == NULL) {
        checkAndLogMsg ("Scheduler::Scheduler", Logger::L_SevereError,
                        "Can not set the QueueReplacementPolicy to NULL.  Quitting.\n");
        exit (-1);
    }
    else {
        checkAndLogMsg ("Scheduler::Scheduler", Logger::L_Info, "_bEnforceTiming = <%s>\n", (_bEnforceTiming ? "true" : "false"));
        String sPpreStagingSessionAwarePrevMsgID;
        switch (_preStagingSessionAwarePrevMsgID) {
            case PREV_PUSH_MSG_INFO_DISABLED: sPpreStagingSessionAwarePrevMsgID = "PREV_PUSH_MSG_INFO_DISABLED"; break;
            case PREV_PUSH_MSG_INFO_SESSION_AWARE: sPpreStagingSessionAwarePrevMsgID = "PREV_PUSH_MSG_INFO_SESSION_AWARE"; break;
            case PREV_PUSH_MSG_INFO_SESSIONLESS: sPpreStagingSessionAwarePrevMsgID = "PREV_PUSH_MSG_INFO_SESSIONLESS"; break;
            default: sPpreStagingSessionAwarePrevMsgID = "UNKNOWN"; exit (-1);
        }
        checkAndLogMsg ("Scheduler::Scheduler", Logger::L_Info, "_preStagingSessionAwarePrevMsgID = <%s>\n", sPpreStagingSessionAwarePrevMsgID.c_str());
        checkAndLogMsg ("Scheduler::Scheduler", Logger::L_Info, "_uiMaxNMsgPerSession = <%u>\n", _uiMaxNMsgPerSession);
        checkAndLogMsg ("Scheduler::Scheduler", Logger::L_Info, "_uiOutgoingQueueSizeThreashold = <%u>\n", _uiOutgoingQueueSizeThreashold);
    }
}

Scheduler::~Scheduler (void)
{
    if (this->isRunning()) {
        requestTerminationAndWait();
    }
}

Scheduler * Scheduler::getScheduler (ConfigManager *pCfgMgr, DSProImpl *pDSPro,
                                     CommAdaptorManager *pAdaptorMgr, DataStore *pDataStore,
                                     NodeContextManager *pNodeCtxtMgr, InformationStore *pInfoStore,
                                     Topology *pTopology)
{
    if (pCfgMgr == NULL || pInfoStore == NULL || pTopology == NULL) {
        return NULL;
    }

    QueueReplacementPolicy *pReplacementPolicy = QueueReplacementPolicyFactory::getQueueReplacementPolicy (pCfgMgr);
    if (pReplacementPolicy == NULL) {
        return NULL;
    }

    Scheduler::PrevPushedMsgInfoMode prevPushedMsgInfo;
    MetadataMutationPolicy *pMutatorPolicy = MetadataMutationPolicyFactory::getMetadataMutationPolicy (pCfgMgr, pDSPro,
                                                                                                       pInfoStore,
                                                                                                       prevPushedMsgInfo);
    if (pMutatorPolicy == NULL) {
        delete pReplacementPolicy;
        return NULL;
    }

    bool bEnforceTiming = DEFAULT_ENFORCE_RANK_BY_TIME;
    if (pCfgMgr->hasValue (ENFORCE_TIMING_PROPERTY)) {
        bEnforceTiming = pCfgMgr->getValueAsBool (ENFORCE_TIMING_PROPERTY);
    }
    else if (pCfgMgr->hasValue ("aci.dspro.informationPush.enforceTiming")) {
        // obsolete property name - also check this for back-compatibility
        bEnforceTiming = pCfgMgr->getValueAsBool ("aci.dspro.informationPush.enforceTiming");
    }
    bool bUseSizeOverUtilityRatio = pCfgMgr->getValueAsBool ("aci.dspro.scheduler.sortBySize", false);

    checkAndLogMsg ("Scheduler::getScheduler", Logger::L_Info,
                    "EnforceTiming: %s\n", bEnforceTiming ? "true" : "false");

    unsigned int maxNMsgPerSess = pCfgMgr->getValueAsUInt32 (PRESTAGING_MESSAGE_THRESHOLD_PROPERTY,
                                                             DEFAULT_MAX_N_OUTGOING_MSGS);
    unsigned int uiOutgoingQueueSizeThreashold = pCfgMgr->getValueAsUInt32 (OUTGOING_MSG_THRESHOLD_STRICT_PROPERTY,
                                                                            DEFAULT_OUTGOING_QUEUE_SIZE_THREASHOLD);

    TransmissionHistoryInterface *pTrHistory = TransmissionHistoryInterface::getTransmissionHistory();
    if (pTrHistory == NULL) {
        return NULL;
    }

    Scheduler *pSched = new Scheduler (bEnforceTiming, bUseSizeOverUtilityRatio, maxNMsgPerSess,
                                       uiOutgoingQueueSizeThreashold, pAdaptorMgr, pDataStore,
                                       pNodeCtxtMgr, pTopology, pTrHistory, pReplacementPolicy,
                                       pMutatorPolicy, prevPushedMsgInfo);
    if (pSched == NULL) {
        delete pReplacementPolicy;
        delete pMutatorPolicy;
    }

    pSched->configure (pCfgMgr);
    if (pCfgMgr->getValueAsBool (RUN_SCHEDULER_PROPERTY, DEFAULT_RUN_SCHEDULER_THREAD)) {
        pSched->start();
    }

    return pSched;
}

void Scheduler::configure (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return;
    }
    _m.lock (1070);

    if (pCfgMgr->hasValue (PREV_ID_MODE_PROPERTY)) {
        PrevPushedMsgInfoMode preStagingSessionAwarePrevMsgID = PREV_PUSH_MSG_INFO_DISABLED;
        const char *pszPrevPushedMsgInfoMode = pCfgMgr->getValue (PREV_ID_MODE_PROPERTY);
        if (0 == stringcasecmp (pszPrevPushedMsgInfoMode, "SESSION_AWARE")) {
            preStagingSessionAwarePrevMsgID = PREV_PUSH_MSG_INFO_SESSION_AWARE;
        }
        else if (0 == stringcasecmp (pszPrevPushedMsgInfoMode, "SESSIONLESS")) {
            preStagingSessionAwarePrevMsgID = PREV_PUSH_MSG_INFO_SESSIONLESS;
        }

        if (_preStagingSessionAwarePrevMsgID != preStagingSessionAwarePrevMsgID) {
            checkAndLogMsg ("Scheduler::configure", Logger::L_Warning,
                            "cannot change metadata mutator configuration at run-time\n");
        }
    }

    String timeSensitiveMIMETypes (pCfgMgr->getValue (TIME_SENSITIVE_MIME_TYPES_PROPERTY));
    if (timeSensitiveMIMETypes.length() > 0) {
        StringTokenizer tokenizer (timeSensitiveMIMETypes, ';', ';');
        for (const char *pszToken; (pszToken = tokenizer.getNextToken()) != NULL;) {
            _timeSensitiveMIMETypes.put (pszToken);
        }
    }

    _m.unlock (1070);
}

int Scheduler::addMessageRequest (const char *pszRequestingPeer, const char *pszMsgId,
                                  DArray<uint8> *pCachedChunks)
{
    if (pszRequestingPeer == NULL || pszMsgId == NULL) {
        checkAndLogMsg ("Scheduler::addMessageRequest", Logger::L_Warning,
                        "pszID or pszMsgId is NULL.\n");
        return -1;
    }

    checkAndLogMsg ("Scheduler::addMessageRequest", Logger::L_Info,
                    "called for requesting node %s\n", pszRequestingPeer);

    ChunkIdWrapper *pWr = new ChunkIdWrapper (pszRequestingPeer, pszMsgId);
    if (pWr == NULL) {
        checkAndLogMsg ("Scheduler::addMessageRequest", memoryExhausted);
        return -2;
    }

    pWr->pChunkIds = new ChunkIds;
    if (pWr->pChunkIds == NULL) {
        checkAndLogMsg ("Scheduler::addMessageRequest", memoryExhausted);
        return -3;
    }

    if (pCachedChunks == NULL) {
        pWr->pChunkIds->uiSize = 0;
        pWr->pChunkIds->pIDs = NULL;
    }
    else {
        pWr->pChunkIds->pIDs = (uint8*) calloc (pCachedChunks->size()+1, sizeof (uint8));
        if (pWr->pChunkIds->pIDs == NULL) {
            checkAndLogMsg ("Scheduler::addMessageRequest", memoryExhausted);
            pWr->pChunkIds->uiSize = 0U;
        }
        else {
            memcpy (pWr->pChunkIds->pIDs, pCachedChunks->getData(), pCachedChunks->size()*sizeof (uint8));
            pWr->pChunkIds->uiSize = pCachedChunks->size();
        }
    }

    _mEnqueuedRequests.lock (1074);
    ChunkIdWrapper *pOldWr = _enqueuedMessageRequests.search (pWr);
    if (pOldWr != NULL) {
        if (pOldWr->pChunkIds->pIDs != NULL) {
            free (pOldWr->pChunkIds->pIDs);
        }
        pOldWr->pChunkIds->uiSize = pWr->pChunkIds->uiSize;
        pOldWr->pChunkIds->pIDs = pWr->pChunkIds->pIDs;
        delete pWr;
        pWr = NULL;
    }
    else {
        _enqueuedMessageRequests.insert (pWr);
    }
    _mEnqueuedRequests.unlock (1074);

    return 0;
}

void Scheduler::addToCurrentPreStaging (Rank *pRank)
{
    if (pRank == NULL) {
        checkAndLogMsg ("Scheduler::addToCurrentPreStaging", Logger::L_Warning,
                        "pRank is NULL.\n");
        return;
    }

    float fPrimaryIndex, fSecondaryIndex;
    setIndexes (pRank, fPrimaryIndex, fSecondaryIndex);
    if (pRank->_msgId.length() <= 0) {
        checkAndLogMsg ("Scheduler::addToCurrentPreStaging", Logger::L_Warning,
                        "pszID is NULL.\n");
        return;
    }
    checkIndexes (fPrimaryIndex, fSecondaryIndex);

    checkAndLogMsg ("Scheduler::addToCurrentPrestaging", Logger::L_Info,
                    "called for target node %s and for message %s\n",
                    (const char *) pRank->_targetId, pRank->_msgId.c_str());

    for (NodeIdIterator iter = pRank->_targetId.getIterator(); !iter.end(); iter.nextElement()) {
        // Create proper MsgIDWrapper
        MsgIDWrapper *pMsgdIdWr;
        if (fSecondaryIndex == INDEX_UNSET) {
            pMsgdIdWr = new MonoIndexMsgIDWrapper (pRank, fPrimaryIndex);
        }
        else {
            pMsgdIdWr = new BiIndexMsgIDWrapper (pRank, fPrimaryIndex, fSecondaryIndex);
        }
        if (pMsgdIdWr == NULL) {
            checkAndLogMsg ("Scheduler::addToCurrentPreStaging", memoryExhausted);
            return;
        }

        _mEnqueuedMessageIdWrappers.lock (1068);
        PtrLList<MsgIDWrapper> *pWrappers = _enqueuedMessageIdWrappers.get (iter.getKey());
        if (pWrappers == NULL) {
            pWrappers = new PtrLList<MsgIDWrapper>();
            if (pWrappers == NULL) {
                _mEnqueuedMessageIdWrappers.unlock (1068);
                checkAndLogMsg ("Scheduler::addToCurrentPreStaging", memoryExhausted);
                return;
            }
            _enqueuedMessageIdWrappers.put (iter.getKey(), pWrappers);
            checkAndLogMsg ("Scheduler::addToCurrentPrestaging", Logger::L_Info,
                            "message %s added to the queue for peer %s.\n",
                            pRank->_msgId.c_str(), iter.getKey());
        }
        pWrappers->prepend (pMsgdIdWr);
        _mEnqueuedMessageIdWrappers.unlock (1068);
    }
}

void Scheduler::startNewPreStagingForPeer (const char *pszTargetNodeId, Ranks *pRanks)
{
    const char *pszMethodName = "Scheduler::startNewPreStagingForPeer";

    if (pszTargetNodeId == NULL || pRanks == NULL || pRanks->getFirst() == NULL) {
        return;
    }

    _mQueues.lock (1072);
    // Get queue
    PeerQueue *pPeerQueue = _queues.get (pszTargetNodeId);
    _mQueues.unlock (1072);

    bool bNewPeerQueue = false;
    if (pPeerQueue == NULL) {        
        // Create new queue for the peer if it does not already exist
        pPeerQueue = new PeerQueue (_pReplacementPolicy);
        if (pPeerQueue == NULL) {
            return;
        }
        bNewPeerQueue = true;
    }
    else {
        // If the queue existed already, it may contain messages of previous
        // pre-staging sessions that may be obsolete by now, therefore the
        // queue should be reset.
        // However, in order to guarantee important messages to be sent, these
        // may be not removed from the queue (pPeerQueue->removeReplaceable()
        // only removes relatively unimportant messages. Look at SchedulerPolicies
        // for more information on how the importance is evaluated ), therefore,
        // it must be ensured not to add duplicate
        // elements. SetUniquePtrLList does not add duplicates to the list and
        // AUTOMATICALLY DEALLOCATES DUPLICATE ENTRIES.
        //
        // Furthermore this queue is in the hashtable
        // containing all the queues, therefore it has to be locked
        bNewPeerQueue = false;
        checkAndLogMsg (pszMethodName, Logger::L_Info, "target node %s already "
                        "has a queue - locking it...\n", pszTargetNodeId);
        pPeerQueue->lock();
        checkAndLogMsg (pszMethodName, Logger::L_Info, "got the lock on the "
                        "queue for target node %s\n", pszTargetNodeId);
        pPeerQueue->removeReplaceable();
    }

    String prevMsgId = _latestMsgPushedByTarget.getLatestMessageIdPushedToTarget (pszTargetNodeId);
    bool bResetPrevMsgId = true;
    unsigned int uiMsgSessionIndex = 0;
    for (Rank *pRank = pRanks->getFirst(); pRank != NULL; pRank = pRanks->getNext()) {

        // Sanity check
        if (!(pRank->_targetId.contains (pszTargetNodeId))) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "Adding message for "
                            "target %s while the target is %s.\n",
                            (const char *) pRank->_targetId, pszTargetNodeId);
            continue;
        }

        if (bResetPrevMsgId && (prevMsgId.length() > 0) && (pRank->_msgId == prevMsgId)) {
            bResetPrevMsgId = false;
        }

        startNewPreStaging (pRank, pPeerQueue, uiMsgSessionIndex);
        uiMsgSessionIndex++;
    }

    if (_preStagingSessionAwarePrevMsgID == PREV_PUSH_MSG_INFO_SESSION_AWARE) {
        _latestMsgPushedByTarget.resetLatestMessageIdPushedToTarget (pszTargetNodeId);
    }

    if (bNewPeerQueue) {
        _mQueues.lock (1076);
        _queues.put (pszTargetNodeId, pPeerQueue);
        _mQueues.unlock (1076);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "put the new queue into _queues\n");
    }
    else {
        // If the queue already existed, it has been locked, therefore it needs
        // to be released
        pPeerQueue->unlock();
    }
}

void Scheduler::startNewPreStaging (Rank *pRank, PeerQueue *pPeerQueue, unsigned int uiMsgSessionIndex)
{
    const char *pszMethodName = "Scheduler::startNewPreStaging";

    float fPrimaryIndex, fSecondaryIndex;
    setIndexes (pRank, fPrimaryIndex, fSecondaryIndex);
    for (NodeIdIterator iter = pRank->_targetId.getIterator(); !iter.end(); iter.nextElement()) {

        MsgIDWrapper *pMsgdIdWr = new BiIndexMsgIDWrapper (pRank, fPrimaryIndex, fSecondaryIndex);
        if (pMsgdIdWr == NULL) {
            break;
        }
        MsgIDWrapper *pReturnedMsgdIdWr = pPeerQueue->insert (pMsgdIdWr);

        if (pLogger != NULL) {
            static StringHashtable<unsigned int> sessionByTarget (true, true, true, true);
            unsigned int *pSession = sessionByTarget.get (iter.getKey());
            if (pSession == NULL) {
                pSession = (unsigned int *) malloc (1*sizeof (unsigned int));
                *pSession = 1;
                sessionByTarget.put (iter.getKey(), pSession);
            }
            else if (uiMsgSessionIndex == 0) {
                *pSession = (*pSession) + 1;
            }

            if (pReturnedMsgdIdWr == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "%d) added element %d with message ID %s and rank %f "
                                "to the queue for target node %s\n", *pSession, uiMsgSessionIndex, pRank->_msgId.c_str(),
                                pMsgdIdWr->getFirstIndex(), (const char *) pRank->_targetId);
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "%d) element %d with message ID %s and rank %f for target "
                                "node %s was not added to the queue because already contained\n", *pSession, uiMsgSessionIndex,
                                pRank->_msgId.c_str(), pMsgdIdWr->getFirstIndex(), (const char *) pRank->_targetId);
            }
        }

        if (pReturnedMsgdIdWr != NULL) {
            delete pReturnedMsgdIdWr;
            pReturnedMsgdIdWr = pMsgdIdWr = NULL;
        }
    }
}

char * Scheduler::getLatestMessageReplicatedToPeer (const char *pszPeerId)
{
    if (pszPeerId == NULL) {
        return NULL;
    }
    _m.lock (1079);
    String msgId (getLatestMessageSentToTargetInternal (pszPeerId));
    _m.unlock (1079);

    if (msgId.length() <= 0) {
        return NULL;
    }
    return msgId.r_str();
}

int Scheduler::replicateMessageInternal (Scheduler::MsgProperties *pMsgProp,
                                         const char *pszFinalDestination, Targets **ppNextHops)
{
    const char *pszMethodName = "Scheduler::replicateMessageInternal";
    if (pMsgProp == NULL || pszFinalDestination == NULL || ppNextHops == NULL) {
        return -1;
    }

    const bool bTimeSensitiveMessage = (pMsgProp->rankObjInfo._mimeType.length() > 0) &&
                                        (_timeSensitiveMIMETypes.containsKey (pMsgProp->rankObjInfo._mimeType));

    // Filter out the "final destinations" that have already been pre-staged the
    // message, or that have already been pre-staged a more recent message.
    RankByTargetMap finalRankByTarget;
    NodeIdSet finalDestinations;
    if (!finalDestinations.contains (pszFinalDestination) && !_pTrHistory->hasTarget (pMsgProp->msgId, pszFinalDestination)) {
        finalDestinations.add (pszFinalDestination);
        finalRankByTarget.add (pszFinalDestination, pMsgProp->rankByTarget.get (pszFinalDestination));
    }

    for (NodeIdIterator iter = pMsgProp->matchingNodeIds.getIterator(); !iter.end(); iter.nextElement()) {
        const String currDestination (iter.getKey());
        if (currDestination == pszFinalDestination) {
            continue;
        }
        if (_pTrHistory->hasTarget (pMsgProp->msgId, currDestination)) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "message %s has already been "
                            "replicated to %s.\n", pMsgProp->msgId.c_str(), currDestination.c_str());
            continue;
        }
        if (bTimeSensitiveMessage) {
            // If the MIME type of the message identifies a "time sensitive" message,
            // check whether a more recent message with the same object has already been
            // sent to the destination, in which case there is not need to send the
            // message
            const int64 i64MostRecentTimestamp = _latestMsgPushedByTarget.getMostRecentMessageTimestamp (currDestination, pMsgProp->rankObjInfo._objectId);
            if (pMsgProp->rankObjInfo._i64SourceTimestamp < i64MostRecentTimestamp) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "message %s will not be replicated to %s, because a more recent "
                                "message has already been replicated to it.\n", pMsgProp->msgId.c_str(), currDestination.c_str());
                continue;
            }
        }
        finalDestinations.add (currDestination);
        if (pMsgProp->rankByTarget.contains (currDestination)) {
            finalRankByTarget.add (currDestination, pMsgProp->rankByTarget.get (currDestination));
        }
    }
    if (finalDestinations.isEmpty()) {
        return 0;
    }

    // For each message, check whether some of the "next hops" have already been
    // replicated the message.  There is no need to replicate the message again
    // if this is the case, the "next hop" will take care of replicating it if
    // it is still necessary
    unsigned int uiShift = 0;
    String nextHops;
    for (unsigned int i = 0; ppNextHops[i] != NULL; i++) {
        for (unsigned int j = 0; j < ppNextHops[i]->aTargetNodeIds.size(); j++) {
            if (nextHops.length() > 0) {
                nextHops += ' ';
            }
            nextHops += ppNextHops[i]->aTargetNodeIds[j];
        }

        if (bTimeSensitiveMessage) {
            // If the MIME type of the message identifies a "time sensitive" message,
            // check whether a more recent message with the same object has already been
            // sent to the destination, in which case there is not need to send the
            // message
            const int64 i64MostRecentTimestamp = _latestMsgPushedByTarget.getMostRecentMessageTimestamp (ppNextHops[i]->aTargetNodeIds[0], pMsgProp->rankObjInfo._objectId);
            if (pMsgProp->rankObjInfo._i64SourceTimestamp < i64MostRecentTimestamp) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "message %s will not be replicated to %s, because "
                                "a more recent message has already been replicated to %s which is on the path to %s.\n",
                                pMsgProp->msgId.c_str(), (const char*) finalDestinations, ppNextHops[i]->aTargetNodeIds[0],
                                (const char*) finalDestinations);
                Targets::deallocateTarget (ppNextHops[i]);
                ppNextHops[i] = NULL;
                uiShift++;
                continue;
            }
        }

        bool bSkip = _pTrHistory->hasTarget (pMsgProp->msgId, ppNextHops[i]->aTargetNodeIds[0]);
        if (bSkip) {
            // If one of the nodes on the path was sent the message, but it was configured to only
            // perform local matchmaking, I still need to replicate
            bSkip = !_peersMatchmakingOnlyLocalData.containsKey (ppNextHops[i]->aTargetNodeIds[0]);
        }

        if (bSkip) {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "message %s has already been replicated to %s which is a next hop. "
                           "Therefore it will removed.\n", pMsgProp->msgId.c_str(), ppNextHops[i]->aTargetNodeIds[0]);
            Targets::deallocateTarget (ppNextHops[i]);
            ppNextHops[i] = NULL;
            uiShift++;
        }
        else if (uiShift > 0) {
            ppNextHops[i-uiShift] = ppNextHops[i];
            ppNextHops[i] = NULL;
        }
    }
    if (ppNextHops[0] == NULL) {
        // All the targets have been removed
        checkAndLogMsg (pszMethodName, Logger::L_Info, "message %s will not be replicated to %s, because the message, "
                        "or a more recent version of it, has already been replicated to all the next hops (%s) on the route to %s\n",
                        pMsgProp->msgId.c_str(), (const char*) finalDestinations, nextHops.c_str(), pMsgProp->msgId.c_str());
        return -3;
    }

    // If necessary, _pMMutationPolicy creates and stores a
    // target-and-transmission-specific metadata message. The ID of such a message
    // is returned by mutate(), and it should be used to retrieve the message to
    // replicate
    char *pszMutatedMessageId = (((MetadataMutationPolicy*) _pMMutationPolicy)->mutate (pMsgProp->msgId, finalDestinations,
                                                                                        finalRankByTarget, this));
    const char *pszMessageToReplicateId, *pszBaseMetadataId;
    if (pszMutatedMessageId == NULL) {
        pszMessageToReplicateId = pMsgProp->msgId;
        pszBaseMetadataId = NULL;
    }
    else {
        pszMessageToReplicateId = pszMutatedMessageId;
        pszBaseMetadataId = pMsgProp->msgId;
    }

    Message *pMsg = _pDataStore->getCompleteMessage (pszMessageToReplicateId);
    if (pMsg == NULL) {
        if (pszMutatedMessageId != NULL) {
            free (pszMutatedMessageId);
        }
        return -3;
    }

    MessageHeader *pMH = pMsg->getMessageHeader();
    void *pData = (void*)pMsg->getData();
    if (pMH == NULL || pData == NULL) {
        delete pMH;
        free (pData);
        delete pMsg;
        if (pszMutatedMessageId != NULL) {
            free (pszMutatedMessageId);
        }
        return -4;
    }

    pMH->setPriority (pMsgProp->ui8Priority);
    pMH->setAcknowledgment (false);

    Message msg (pMH, pData);    
    int rc = _pAdaptorMgr->sendDataMessage (&msg, ppNextHops);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not send message to peer %s; MessageId = %s (%s%s); "
                        "Priority = %d; Return code: %d.\n", (const char*) finalDestinations, pszMessageToReplicateId,
                        (pszBaseMetadataId == NULL ? "base" : "mutated from "),
                        (pszBaseMetadataId == NULL ? "" : pszBaseMetadataId), pMsgProp->ui8Priority, rc);
    }
    else {
        for (NodeIdIterator iter = pMsgProp->matchingNodeIds.getIterator(); !iter.end(); iter.nextElement()) {
            const String currDestination (iter.getKey());
            _m.lock (1008);
            _latestMsgPushedByTarget.setLatestMessageIdPushedToTarget (currDestination, pszMessageToReplicateId);
            _m.unlock (1008);
            if (bTimeSensitiveMessage) {
                _latestMsgPushedByTarget.setMostRecentMessageTimestamp (currDestination, pMsgProp->rankObjInfo._objectId,
                                                                        pMsgProp->rankObjInfo._i64SourceTimestamp);
            }
        }
        for (unsigned int i = 0; ppNextHops[i] != NULL; i++) {
            _pTrHistory->addMessageTarget (pMsgProp->msgId, ppNextHops[i]->aTargetNodeIds[0]);
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "adding message %s (%s%s) to the tranmission history for peer %s\n",
                            pszMessageToReplicateId, (pszBaseMetadataId == NULL ? "base" : "mutated from "),
                            (pszBaseMetadataId == NULL ? "" : pszBaseMetadataId), ppNextHops[i]->aTargetNodeIds[0]);
        }

        checkAndLogMsg (pszMethodName, Logger::L_Info, "sent message to peer %s; MessageId = %s (%s%s); Priority = %d;\n",
                        (const char*) finalDestinations, pszMessageToReplicateId, (pszBaseMetadataId == NULL ? "base" : "mutated from "),
                        (pszBaseMetadataId == NULL ? "" : pszBaseMetadataId), pMsgProp->ui8Priority);
    }

    delete pMH;
    free (pData);
    delete pMsg;
    if (pszMutatedMessageId != NULL) {
        free (pszMutatedMessageId);
    }
    return (rc < 0 ? -5 : 0);
}

int Scheduler::replicateDataMessageInternal (Scheduler::MsgProperties *pMsgProp, ChunkIds *pIds,
                                             const char *pszDestination, Targets **ppTargets)
{
    const char *pszMethodName = "Scheduler::replicateDataMessageInternal";
    if (pMsgProp == NULL || pMsgProp->msgId.length() <= 0) {
        return -1;
    }

    Message *pMsg = _msgReqSrv.getRequestReply (pMsgProp->msgId, pIds->pIDs, pIds->uiSize);
    if (pMsg == NULL) {
        return -2;
    }

    MessageHeader *pMH = pMsg->getMessageHeader();
    void *pData = (void*)pMsg->getData();
    if (pMH == NULL || pData == NULL) {
        delete pMH;
        free (pData);
        delete pMsg;
        return -3;
    }

    pMH->setAcknowledgment (false);

    int rc = pMH->isChunk() ? _pAdaptorMgr->sendChunkedMessage (pMsg, pMH->getMimeType(), ppTargets)
                            : _pAdaptorMgr->sendDataMessage (pMsg, ppTargets);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not send requested data message to peer %s; "
                        "MessageId = %s; Priority = %d; Return code: %d.\n", pszDestination, pMsgProp->msgId.c_str(),
                        pMH->getPriority(), rc);
    }
    else {
        for (unsigned int i = 0; ppTargets[i] != NULL; i++) {
            _pTrHistory->addMessageTarget (pMsgProp->msgId.c_str(), ppTargets[i]->aTargetNodeIds[0]);
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "adding requested message %s to "
                            "the tranmission history for peer %s\n", pMsgProp->msgId.c_str(),
                            ppTargets[i]->aTargetNodeIds[0]);
        }

        checkAndLogMsg (pszMethodName, Logger::L_Info, "sent requested message to peer %s; "
                        "MessageId = %s; Priority = %d;\n", pszDestination, pMsgProp->msgId.c_str(),
                        pMH->getPriority());
    }

    delete pMH;
    free (pData);
    delete pMsg;
    return (rc < 0 ? -4 : 0);
}

void Scheduler::run()
{
    const char *pszMethodName = "Scheduler::run";
    setName (pszMethodName);

    started();
    checkAndLogMsg (pszMethodName, Logger::L_Info, "thread started.\n");

    while (!terminationRequested()) {
        PeerNodeContextList *pPeerNodeContextList = _pNodeCtxtMgr->getPeerNodeContextList();
        if (pPeerNodeContextList != NULL) {
            for (PeerNodeContext *pNodeContext = pPeerNodeContextList->getFirst();
                pNodeContext != NULL; pNodeContext = pPeerNodeContextList->getNext()) {
                if (pNodeContext->getLimitToLocalMatchmakingOnly()) {
                    _peersMatchmakingOnlyLocalData.put (pNodeContext->getNodeId());
                }
                else {
                    _peersMatchmakingOnlyLocalData.remove (pNodeContext->getNodeId());
                }
            }
        }
        _pNodeCtxtMgr->releasePeerNodeContextList();
        sendInternal();
        sleepForMilliseconds (300);
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "the thread is terminating.\n");
    terminating();
}

void Scheduler::send()
{
    _m.lock (1073);

    if (!this->isRunning()) {
        // If the Scheduler thread is running, it
        // will take care of sending the messages
        sendInternal();
    }

    _m.unlock (1073);
}

void Scheduler::sendInternal()
{
    ChunkIdWrapper *pWr;
    _mEnqueuedRequests.lock (1069);
    ChunkIdWrapper *pWrTmp = _enqueuedMessageRequests.getFirst();
    while ((pWr = pWrTmp) != NULL) {
        pWrTmp = _enqueuedMessageRequests.getNext();
        if (addMessageRequestInternal (pWr->requestingPeer.c_str(), pWr->msgId.c_str(), pWr->pChunkIds) == 0) {
            _enqueuedMessageRequests.remove (pWr);
            delete pWr;
        }
    }
    _mEnqueuedRequests.unlock (1069);

    _mEnqueuedMessageIdWrappers.lock (1067);
    StringHashtable<PtrLList<MsgIDWrapper> >::Iterator enqMsgWrIter = _enqueuedMessageIdWrappers.getAllElements();
    for (; !enqMsgWrIter.end(); enqMsgWrIter.nextElement()) {
        PtrLList<MsgIDWrapper> *pWrappers = enqMsgWrIter.getValue();
        if (pWrappers != NULL) {
            MsgIDWrapper *pMsgIdWr;
            MsgIDWrapper *pMsgIdWrTmp = pWrappers->getFirst();
            while ((pMsgIdWr = pMsgIdWrTmp) != NULL) {
                pMsgIdWrTmp = pWrappers->getNext();
                pWrappers->remove (pMsgIdWr);
                addToCurrentPreStagingInternal (enqMsgWrIter.getKey(), pMsgIdWr);
            }
        }
    }
    _enqueuedMessageIdWrappers.removeAll();
    _mEnqueuedMessageIdWrappers.unlock (1067);

    MsgProperties *pMsgProps = new MsgProperties[_uiMaxNMsgPerSession];
    if (pMsgProps == NULL) {
        checkAndLogMsg ("Scheduler::sendInternal", memoryExhausted);
        return;
    }

    for (bool bAllEmptyQueues = false; !bAllEmptyQueues;) {
        bAllEmptyQueues = true;
        static unsigned int sessionCounter = 0;
        sessionCounter++;
        String currKey;
        _queues.resetGetNext();
        for (PeerQueue *pPeerQueue; (pPeerQueue = _queues.getNext (currKey)) != NULL;) {
            if (pPeerQueue->tryLock()) {
                DArray2<String> requestsToServe;
                DArray2<ChunkIds> requestsToServeChunkFilters;

                // Get message to replicate
                MsgIDWrapper *pCurr, *pNext;
                pNext = pPeerQueue->_msgIDs.getFirst();
                for (unsigned int i = 0; (i < _uiMaxNMsgPerSession) && (pCurr = pNext) != NULL; i++) {
                    pNext = pPeerQueue->_msgIDs.getNext();
                    pPeerQueue->_msgIDs.remove (pCurr);
                    pMsgProps[i].msgId = pCurr->_msgId;
                    pMsgProps[i].rankObjInfo = pCurr->_rankObjInfo;
                    pMsgProps[i].matchingNodeIds = pCurr->_matchingNodeIds;
                    pMsgProps[i].rankByTarget = pCurr->_rankByTarget;
                    pMsgProps[i].ui8Priority = DSLib::toPriority (pCurr->getFirstIndex());
                    delete pCurr;
                }

                // Get requested messages to serve
                StringHashtable<ChunkIds>::Iterator reqMsgId = pPeerQueue->_requestedMsgIDs.getAllElements();
                for (unsigned int i = 0; !reqMsgId.end(); reqMsgId.nextElement()) {
                    requestsToServe[i] = reqMsgId.getKey();
                    requestsToServeChunkFilters[i].pIDs = reqMsgId.getValue()->pIDs;
                    requestsToServeChunkFilters[i].uiSize = reqMsgId.getValue()->uiSize;
                    i++;
                }

                // Remove requested messages that will be served
                for (unsigned int i = 0; i < requestsToServe.size(); i++) {
                    delete pPeerQueue->_requestedMsgIDs.remove (requestsToServe[i]);
                }

                pPeerQueue->unlock();

                if ((pNext != NULL) && bAllEmptyQueues) {
                    bAllEmptyQueues = false;
                }

                // Replicate the messages identified by ppszMsgIDs
                for (unsigned int i = 0; i < _uiMaxNMsgPerSession; i++) {
                    if (pMsgProps[i].msgId.length() > 0) {
                        NodeIdSet matchedNodes (pMsgProps[i].matchingNodeIds);
                        Targets **ppTargets = _pTopology->getNextHopsAsTarget (matchedNodes);
                        if (ppTargets != NULL && ppTargets[0] != NULL) {                                
                            replicateMessageInternal (&(pMsgProps[i]),
                                                        currKey,    // pszFinalDestination
                                                        ppTargets         // ppNextHops
                            );
                        }
                        Targets::deallocateTargets (ppTargets);
                    }
                }

                TargetPtr targets[2] = { _pTopology->getNextHopAsTarget (currKey), NULL };
                if (targets[0] != NULL) {
                    // Replicate the messages explicitely requested by the peer
                    MsgProperties msgProp;
                    for (unsigned int i = 0; i < requestsToServe.size(); i++) {
                        msgProp.msgId = requestsToServe[i];
                        ChunkIds cIds;
                        cIds.pIDs = requestsToServeChunkFilters[i].pIDs;
                        cIds.uiSize = requestsToServeChunkFilters[i].uiSize;
                        replicateDataMessageInternal (&msgProp, &cIds, currKey, targets);
                        delete pPeerQueue->_requestedMsgIDs.remove (requestsToServe[i]);
                    }                       
                }
                delete targets[0];
            }
            else {
                // queue it's being filled out
            }
        }
    }

    delete[] pMsgProps;
}

int Scheduler::addMessageRequestInternal (const char *pszRequestingPeer, const char *pszMsgId,
                                          ChunkIds *pChunkIds)
{
    if (pszRequestingPeer == NULL || pszMsgId == NULL || pChunkIds == NULL) {
        checkAndLogMsg ("Scheduler::addMessageRequestInternal", Logger::L_Warning,
                        "pszID, or pszMsgId, or pChunkIds is NULL.\n");
        return -1;
    }

    checkAndLogMsg ("Scheduler::addMessageRequestInternal", Logger::L_Info,
                    "called for requesting node %s\n", pszRequestingPeer);

    _mQueues.lock (1078);

    PeerQueue *pPeerQueue = _queues.get (pszRequestingPeer);
    if (pPeerQueue == NULL) {
        // Create a new PeerQueue for the peer
        pPeerQueue = new PeerQueue (_pReplacementPolicy);
        if (pPeerQueue == NULL) {
            checkAndLogMsg ("Scheduler::addMessageRequestInternal", memoryExhausted);
            _mQueues.unlock (1078);
            return -2;
        }

        pPeerQueue->_requestedMsgIDs.put (pszMsgId, pChunkIds);

        _queues.put (pszRequestingPeer, pPeerQueue);
        _mQueues.unlock (1078);
    }
    else {
        pPeerQueue->lock();
        _mQueues.unlock (1078);

        if (pPeerQueue->_requestedMsgIDs.containsKey (pszMsgId)) {
             checkAndLogMsg ("Scheduler::addMessageRequestInternal", Logger::L_Info,
                             "message %s for requesting node %s was not added to the scheduler "
                             "because already scheduled for delivery\n",
                             pszMsgId, pszRequestingPeer);
        }
        else {
            pPeerQueue->_requestedMsgIDs.put (pszMsgId, pChunkIds);
            checkAndLogMsg ("Scheduler::addMessageRequestInternal", Logger::L_Info,
                            "message %s for requesting node %s was added to the scheduler\n",
                            pszMsgId, pszRequestingPeer); 
       }
       pPeerQueue->unlock();
    }

    return 0;
}

void Scheduler::addToCurrentPreStagingInternal (const char *pszTargetPeerNodeID, MsgIDWrapper *pMsgdIdWr)
{
    _mQueues.lock (1075);

    PeerQueue *pPeerQueue = _queues.get (pszTargetPeerNodeID);
    if (pPeerQueue == NULL) {
        // Create a new PeerQueue for the peer
        pPeerQueue = new PeerQueue (_pReplacementPolicy);
        if (pPeerQueue == NULL) {
            checkAndLogMsg ("Scheduler::addToCurrentPreStagingInternal", memoryExhausted);
            delete pMsgdIdWr;
            pMsgdIdWr = NULL;
             _mQueues.unlock (1075);
            return;
        }
        pPeerQueue->insert (pMsgdIdWr);

        // Add the new queue to the collection of queues
        _queues.put (pszTargetPeerNodeID, pPeerQueue);
         _mQueues.unlock (1075);
    }
    else {
        pPeerQueue->lock();
        _mQueues.unlock (1075);

        MsgIDWrapper *pReturnedMsgdIdWr = pPeerQueue->insert (pMsgdIdWr);
        if (pReturnedMsgdIdWr != NULL) {
            checkAndLogMsg ("Scheduler::addToCurrentPreStagingInternal", Logger::L_Info,
                            "message %s for target node %s was not added to the scheduler "
                            "because already scheduled for delivery\n",
                            pReturnedMsgdIdWr->_msgId.c_str(), pszTargetPeerNodeID);
            delete pReturnedMsgdIdWr;
            pReturnedMsgdIdWr = NULL;
        }
        else {
            checkAndLogMsg ("Scheduler::addToCurrentPreStagingInternal", Logger::L_Info,
                            "message %s for target node %s was added to the scheduler\n",
                            pMsgdIdWr->_msgId.c_str(), pszTargetPeerNodeID);
        }
        pPeerQueue->unlock();
    }
}

String Scheduler::getLatestMessageSentToTargetInternal (const char *pszTargetNode)
{
    return _latestMsgPushedByTarget.getLatestMessageIdPushedToTarget (pszTargetNode);
}

void Scheduler::setIndexes (Rank *pRank, float &fPrimaryIndex, float &fSecondaryIndex)
{
    float fRank = pRank->_fTotalRank;
    if ((_bUseSizeOverUtilityRatio) && (pRank->_ui32RefDataSize > 0)) {
        fRank /= static_cast<float>(pRank->_ui32RefDataSize);
    }
    fPrimaryIndex = _bEnforceTiming ? pRank->_fTimeRank : fRank;
    fSecondaryIndex = _bEnforceTiming ? fRank : pRank->_fTimeRank;
}

void Scheduler::checkIndexes (float &fPrimaryIndex, float &fSecondaryIndex)
{
    if (fPrimaryIndex == INDEX_UNSET) {
        if (fSecondaryIndex == INDEX_UNSET) {
            fPrimaryIndex = MetadataRankerLocalConfiguration::DEF_RANK;
            checkAndLogMsg ("Scheduler::checkIndexes", Logger::L_MildError,
                            "Either Indexes have not been set. Setting primary index to %f\n", fPrimaryIndex);
        }
        else {
            fPrimaryIndex = fSecondaryIndex;
            fSecondaryIndex = INDEX_UNSET;
            checkAndLogMsg ("Scheduler::checkIndexes", Logger::L_MildError,
                            "Primary index has not been set. Setting it to the value of the secondary index (%f).\n",
                            fPrimaryIndex);
        }
    }
}

//------------------------------------------------------------------------------
// PeerQueue
//------------------------------------------------------------------------------

Scheduler::PeerQueue::PeerQueue (const QueueReplacementPolicy *pReplacementPolicy)
    : _m (MutexId::SchedulerPeerQueue_m, LOG_MUTEX),
      _msgIDs (true), // descendingOrder
      _requestedMsgIDs (true, // bCaseSensitiveKeys
                        true, // bCloneKeys
                        true) // bDeleteKeys
{
    _pReplacementPolicy = pReplacementPolicy;
    _bLocked = false;
}

Scheduler::PeerQueue::~PeerQueue()
{
    removeAll();
}

void Scheduler::PeerQueue::removeAll()
{
    MsgIDWrapper *pCurr, *pNext;
    pNext = _msgIDs.getFirst();
    while ((pCurr = pNext) != NULL) {
        pNext = _msgIDs.getNext();
        delete _msgIDs.remove (pCurr);
    }
}

void Scheduler::PeerQueue::removeReplaceable()
{
    MsgIDWrapper *pCurr, *pNext;
    pNext = _msgIDs.getFirst();
    while ((pCurr = pNext) != NULL) {
        pNext = _msgIDs.getNext();
        if (((QueueReplacementPolicy *)_pReplacementPolicy)->isReplaceable (pCurr)) {
            delete _msgIDs.remove (pCurr);
        }
    }
}

bool Scheduler::BiIndexMsgIDWrapper::operator > (const MsgIDWrapper &rhsMsgWr) const
{
    if (rhsMsgWr._type == BiIndex) {
        BiIndexMsgIDWrapper *prhsMsgWr = (BiIndexMsgIDWrapper *) &rhsMsgWr;
        if (Scheduler::MonoIndexMsgIDWrapper::operator > (rhsMsgWr)) {
            return true;
        }
        if (Scheduler::MonoIndexMsgIDWrapper::operator < (rhsMsgWr)) {
            return false;
        }
        // _fIndex1 == rhsMsgWr._fIndex1
        return (_fIndex2 > prhsMsgWr->_fIndex2);
    }
    else {
        checkAndLogMsg ("Scheduler::BiIndexMsgIDWrapper::operator >", Logger::L_MildError,
                        "Trying to compare a MsgIDWrapper of type %d with one of type %d\n",
                        _type, rhsMsgWr._type);
        return false;
    }
}

bool Scheduler::BiIndexMsgIDWrapper::operator < (const MsgIDWrapper &rhsMsgWr) const
{
    if (rhsMsgWr._type == BiIndex) {
        BiIndexMsgIDWrapper *prhsMsgWr = (BiIndexMsgIDWrapper *) &rhsMsgWr;
        if (Scheduler::MonoIndexMsgIDWrapper::operator < (rhsMsgWr)) {
            return true;
        }
        if (Scheduler::MonoIndexMsgIDWrapper::operator > (rhsMsgWr)) {
            return false;
        }
        // _fIndex1 == rhsMsgWr._fIndex1
        return (_fIndex2 < prhsMsgWr->_fIndex2);
    }
    else {
        checkAndLogMsg ("Scheduler::BiIndexMsgIDWrapper::operator <", Logger::L_MildError,
                        "Trying to compare a MsgIDWrapper of type %d with one of type %d\n",
                        _type, rhsMsgWr._type);
        return false;
    }
}

void Scheduler::PeerQueue::display (FILE *pFileOut)
{
    MsgIDWrapper *pWr = _msgIDs.getFirst();
    bool bFirstEl = true;
    while (pWr != NULL) {
        if (!bFirstEl) {
            fprintf (pFileOut, ", ");
            bFirstEl = false;
        }
        fprintf (pFileOut, "%s", pWr->_msgId.c_str());
        fflush (pFileOut);
        pWr= _msgIDs.getNext();
    }
    fprintf (pFileOut, "\n");
}

