/* 
 * Scheduler.h
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

#ifndef INCL_SCHEDULER_H
#define	INCL_SCHEDULER_H

#include "MessageRequestServer.h"
#include "Rank.h"
#include "SchedulerCache.h"

#include "IterableStringHashtable.h"
#include "LoggingMutex.h"
#include "ManageableThread.h"
#include "SetUniquePtrLList.h"
#include "StringHashset.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class CommAdaptorManager;
    class Controller;
    class DataStore;
    class DSProImpl;
    class InformationStore;
    class MetadataMutationPolicy;
    class NodeContextManager;
    class QueueReplacementPolicy;
    class ReplicationInterface;
    class Targets;
    class Topology;
    class TransmissionHistoryInterface;

    class Scheduler : public NOMADSUtil::ManageableThread
    {
        public:
            static const unsigned int DEFAULT_MAX_N_OUTGOING_MSGS;
            static const unsigned int DEFAULT_OUTGOING_QUEUE_SIZE_THREASHOLD;
            static const bool DEFAULT_RUN_SCHEDULER_THREAD;
            static const bool DEFAULT_ENFORCE_RANK_BY_TIME;

            static const float DEFAULT_UNREPLACEBLE_ENQUEUED_MSG_THREASHOLD;

            static const char * ENFORCE_TIMING_PROPERTY;
            static const char * PRESTAGING_MESSAGE_THRESHOLD_PROPERTY;
            static const char * OUTGOING_MSG_THRESHOLD_STRICT_PROPERTY;
            static const char * RUN_SCHEDULER_PROPERTY;
            static const char * PREV_ID_MODE_PROPERTY;
            static const char * TIME_SENSITIVE_MIME_TYPES_PROPERTY;

            enum PrevPushedMsgInfoMode {
                PREV_PUSH_MSG_INFO_DISABLED       = 0x00,  // The ID of the last message sent is not stored
                PREV_PUSH_MSG_INFO_SESSION_AWARE  = 0x01,  // The ID of the last message sent is stored
                                                           // and reset every time a new pre-staging session
                                                           // starts (unless the message is still relevant
                PREV_PUSH_MSG_INFO_SESSIONLESS    = 0x02   // The ID of the last message sent is stored and
                                                           // never reset
            };

            virtual ~Scheduler (void);

            static Scheduler * getScheduler (NOMADSUtil::ConfigManager *pCfgMgr, DSProImpl *pDSPro,
                                             CommAdaptorManager *pAdaptorMgr, DataStore *pDataStore,
                                             NodeContextManager *pNodeCtxtMgr, InformationStore *pInfoStore,
                                             Topology *pTopology);

            /**
             * Changes the configuration at run-time.
             */
            void configure (NOMADSUtil::ConfigManager *pCfgMgr);

            int addMessageRequest (const char *pszRequestingPeer, const char *pszMsgId,
                                   NOMADSUtil::DArray<uint8> *pCachedChunks);

            /**
             * Add the message id to the current queue of the outgoing messages.
             */
            void addToCurrentPreStaging (Rank *pRank);

            /**
             * Reset the queue of the outgoing messages and set a new one.
             */
            void startNewPreStagingForPeer (const char *pszTargetNodeId, Ranks *pRanks);

            /**
             * NOTE: the returned ID must be deallocated by the caller
             */
            char * getLatestMessageReplicatedToPeer (const char *pszPeerId);

            void run (void);
            void send (void);

        private:
            static const float INDEX_UNSET;

            struct ChunkIds {
                uint8 *pIDs;
                uint8 uiSize;
            };

            struct ChunkIdWrapper {
                ChunkIdWrapper (const char *pszRequestingPeer, const char *pszMsgId);
                ~ChunkIdWrapper (void);

                bool operator > (const ChunkIdWrapper &rhsChunkIdWr);
                bool operator < (const ChunkIdWrapper &rhsChunkIdWr);
                bool operator == (const ChunkIdWrapper &rhsChunkIdWr);

                const NOMADSUtil::String requestingPeer;
                const NOMADSUtil::String msgId;
                ChunkIds *pChunkIds;
            };

            enum ReturnCode {
                RC_Ok   = 0,
                RC_Busy = 2
            };

            struct MsgProperties
            {
                MsgProperties (void);
                ~MsgProperties (void);

                uint8 ui8Priority;
                NOMADSUtil::String msgId;
                RankObjectInfo rankObjInfo;
                NodeIdSet matchingNodeIds;
                RankByTargetMap rankByTarget;
            };

            struct MsgIDWrapper {
                enum Type {
                    MonoIndex = 0,
                    BiIndex   = 1
                };

                MsgIDWrapper (Type type, Rank *pRank);
                virtual ~MsgIDWrapper (void);

                virtual float getFirstIndex (void) const = 0;
                char * relinquishMsgId (void);

                virtual bool operator > (const MsgIDWrapper &rhsMsgWr) const = 0;
                virtual bool operator < (const MsgIDWrapper &rhsMsgWr) const = 0;
                virtual bool operator == (const MsgIDWrapper &rhsMsgWr) const;

                const Type _type;
                NOMADSUtil::String _msgId;
                RankObjectInfo _rankObjInfo;
                NodeIdSet _matchingNodeIds;
                RankByTargetMap _rankByTarget;
            };

            struct MonoIndexMsgIDWrapper : public MsgIDWrapper {
                MonoIndexMsgIDWrapper (Rank *pRank, float fPrimaryIndex);
                virtual ~MonoIndexMsgIDWrapper (void);

                virtual float getFirstIndex (void) const;

                virtual bool operator > (const MsgIDWrapper &rhsMsgWr) const;
                virtual bool operator < (const MsgIDWrapper &rhsMsgWr) const;

                const float _fIndex1;

                protected:
                    MonoIndexMsgIDWrapper (Type type, Rank *pRank, float fPrimaryIndex);
            };

            struct BiIndexMsgIDWrapper : public MonoIndexMsgIDWrapper {
                BiIndexMsgIDWrapper (Rank *pRank, float fPrimaryIndex, float fSecondaryIndex);
                virtual ~BiIndexMsgIDWrapper (void);

                bool operator > (const MsgIDWrapper &rhsMsgWr) const;
                bool operator < (const MsgIDWrapper &rhsMsgWr) const;

                const float _fIndex2;
            };

            struct PeerQueue {
                PeerQueue (const QueueReplacementPolicy *pReplacementPolicy);
                virtual ~PeerQueue (void);

                void display (FILE *pFileOut);

                /**
                 * Returns the element, if it was not added, NULL otherwise 
                 */
                MsgIDWrapper * insert (MsgIDWrapper *pMsgIDWr);
                int getCount (void);
                bool isEmpty (void);
                void removeAll (void);
                void removeReplaceable (void);

                void lock (void);
                bool tryLock (void);
                void unlock (void);
                bool isLocked (void);

                bool _bLocked;
                NOMADSUtil::LoggingMutex _m;
                const QueueReplacementPolicy *_pReplacementPolicy;
                NOMADSUtil::SetUniquePtrLList<MsgIDWrapper> _msgIDs;
                NOMADSUtil::StringHashtable<ChunkIds> _requestedMsgIDs;
            };

            /**
             * - uiMaxNMsgPerSession: the maximum number of messages per peer
             *   that are given to the replication interface in one session
             *
             * - uiOutgoingQueueSizeThreashold: new messages are given to the
             *   replication interface only if the queue size is less or equal
             *   than this value
             */
            Scheduler (bool bEnforceTiming, bool bUseSizeOverUtilityRatio,
                       unsigned int uiMaxNMsgPerSession,
                       unsigned int uiOutgoingQueueSizeThreashold,
                       CommAdaptorManager *pAdaptorMgr,
                       DataStore *pDataStore, NodeContextManager *pNodeCtxtMgr,
                       Topology *pTopology, TransmissionHistoryInterface *pTrHistory,
                       const QueueReplacementPolicy *pReplacementPolicy,
                       const MetadataMutationPolicy *pMutatorPolicy,
                       PrevPushedMsgInfoMode preStagingSessionAwarePrevMsgID);

            int addMessageRequestInternal (const char *pszRequestingPeer, const char *pszMsgId,
                                           ChunkIds *pChunkIds);
            void addToCurrentPreStagingInternal (const char *pszTargetPeerNodeID, MsgIDWrapper *pMsgdIdWr);
            void startNewPreStaging (Rank *pRank, PeerQueue *pPeerQueue, unsigned int uiMsgSessionIndex);

            NOMADSUtil::String getLatestMessageSentToTargetInternal (const char *pszTargetNode);
            int replicateMessageInternal (MsgProperties *pszMsgId, const char *pszDestination,
                                          Targets **ppTargets);
            int replicateDataMessageInternal (MsgProperties *pszMsgId, ChunkIds *pIDs,
                                              const char *pszDestination, Targets **ppTargets);

            void sendInternal (void);
            void setIndexes (Rank *pRank, float &fPrimaryIndex, float &fSecondaryIndex);
            static void checkIndexes (float &fPrimaryIndex, float &fSecondaryIndex);

       private:
            friend class QueueReplacementPolicy;
            friend class ReplaceAllPolicy;
            friend class ReplaceNonePolicy;
            friend class ReplaceLowPriorityPolicy;

            friend class MetadataMutationPolicy;
            friend class DefaultMutationPolicy;
            friend class PrevMsgMutationPolicy;

            const bool _bEnforceTiming;
            const bool _bUseSizeOverUtilityRatio;
            const PrevPushedMsgInfoMode _preStagingSessionAwarePrevMsgID;
            const unsigned int _uiMaxNMsgPerSession;
            const unsigned int _uiOutgoingQueueSizeThreashold;
            const QueueReplacementPolicy *_pReplacementPolicy;
            const MetadataMutationPolicy *_pMMutationPolicy;
            CommAdaptorManager *_pAdaptorMgr;
            DataStore *_pDataStore;
            NodeContextManager *_pNodeCtxtMgr;
            Topology *_pTopology;
            TransmissionHistoryInterface *_pTrHistory;
            MessageRequestServer _msgReqSrv;
            NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::LoggingMutex _mQueues;
            NOMADSUtil::LoggingMutex _mEnqueuedRequests;
            NOMADSUtil::LoggingMutex _mEnqueuedMessageIdWrappers;
            NOMADSUtil::PtrLList<ChunkIdWrapper> _enqueuedMessageRequests;
            NOMADSUtil::StringHashtable<NOMADSUtil::PtrLList<MsgIDWrapper> >_enqueuedMessageIdWrappers;
            IterableStringHashtable<PeerQueue> _queues;
            SchedulerCache _latestMsgPushedByTarget;
            NOMADSUtil::StringHashset _timeSensitiveMIMETypes;
            NOMADSUtil::StringHashset _peersMatchmakingOnlyLocalData;
    };

    //==========================================================================
    // PeerQueue
    //==========================================================================

    inline Scheduler::MsgIDWrapper * Scheduler::PeerQueue::insert (MsgIDWrapper *pMsgIDWr)
    {
        return _msgIDs.insertUnique (pMsgIDWr);
    }

    inline int Scheduler::PeerQueue::getCount (void)
    {
        return _msgIDs.getCount();
    }

    inline bool Scheduler::PeerQueue::isEmpty (void)
    {
        return _msgIDs.getFirst() == NULL;
    }

    inline void Scheduler::PeerQueue::lock (void)
    {
        _m.lock (1078);
        _bLocked = true;
    }

    inline bool Scheduler::PeerQueue::tryLock (void)
    {
        switch (_m.tryLock (1078)) {
            case RC_Ok:   { _bLocked = true; return true; }
            case RC_Busy:
            default: return false;
        }
    }

    inline void Scheduler::PeerQueue::unlock (void)
    {
        _bLocked = false;
        _m.unlock (1078);
    }

    inline bool Scheduler::PeerQueue::isLocked (void)
    {
        return _bLocked;
    }

    //==========================================================================
    // MsgIDWrapper
    //==========================================================================

    inline Scheduler::MsgIDWrapper::MsgIDWrapper (Type type, Rank *pRank)
        : _type (type), _msgId (pRank->_msgId), _rankObjInfo (pRank->_objectInfo),
          _matchingNodeIds (pRank->_targetId), _rankByTarget (pRank->_rankByTarget)
    {
    }

    inline Scheduler::MsgIDWrapper::~MsgIDWrapper (void)
    {
    }

    inline bool Scheduler::MsgIDWrapper::operator == (const MsgIDWrapper &rhsMsgWr) const
    {
        return ((_msgId == rhsMsgWr._msgId) != 0);
    }

    inline char * Scheduler::MsgIDWrapper::relinquishMsgId (void)
    {
        return _msgId.r_str();
    }

    //==========================================================================
    // MonoIndexMsgIDWrapper
    //==========================================================================

    inline Scheduler::MonoIndexMsgIDWrapper::MonoIndexMsgIDWrapper (Rank *pRank, float fPrimaryIndex)
        : Scheduler::MsgIDWrapper (MonoIndex, pRank),
          _fIndex1 (fPrimaryIndex)
    {
    }

    inline Scheduler::MonoIndexMsgIDWrapper::MonoIndexMsgIDWrapper (Type type, Rank *pRank,
                                                                    float fPrimaryIndex)
        : Scheduler::MsgIDWrapper (type, pRank),
          _fIndex1 (fPrimaryIndex)
    {
    }

    inline Scheduler::MonoIndexMsgIDWrapper::~MonoIndexMsgIDWrapper (void)
    {
    }

    inline float Scheduler::MonoIndexMsgIDWrapper::getFirstIndex (void) const
    {
        return _fIndex1;
    }

    inline bool Scheduler::MonoIndexMsgIDWrapper::operator > (const MsgIDWrapper &rhsMsgWr) const
    {
        MonoIndexMsgIDWrapper *prhsMsgWr = (MonoIndexMsgIDWrapper *) &rhsMsgWr;
        return (_fIndex1 > prhsMsgWr->_fIndex1);
    }

    inline bool Scheduler::MonoIndexMsgIDWrapper::operator < (const MsgIDWrapper &rhsMsgWr) const
    {
        MonoIndexMsgIDWrapper *prhsMsgWr = (MonoIndexMsgIDWrapper *) &rhsMsgWr;
        return (_fIndex1 < prhsMsgWr->_fIndex1);
    }

    //==========================================================================
    // BiIndexMsgIDWrapper
    //==========================================================================

    inline Scheduler::BiIndexMsgIDWrapper::BiIndexMsgIDWrapper (Rank *pRank, float fPrimaryIndex, float fSecondaryIndex)
        : Scheduler::MonoIndexMsgIDWrapper (BiIndex, pRank, fPrimaryIndex),
          _fIndex2 (fSecondaryIndex)
    {
    }

    inline Scheduler::BiIndexMsgIDWrapper::~BiIndexMsgIDWrapper (void)
    {
    }

    //==========================================================================
    // ChunkIdWrapper
    //==========================================================================

    inline Scheduler::ChunkIdWrapper::ChunkIdWrapper (const char *pszRequestingPeer, const char *pszMsgId)
        : requestingPeer (pszRequestingPeer), msgId (pszMsgId)
    {
        pChunkIds = NULL;
    }

    inline Scheduler::ChunkIdWrapper::~ChunkIdWrapper (void)
    {
        pChunkIds = NULL;
    }

    inline bool Scheduler::ChunkIdWrapper::operator > (const ChunkIdWrapper &rhsChunkIdWr)
    {
        if (strcmp (requestingPeer.c_str(), rhsChunkIdWr.requestingPeer.c_str()) > 0) {
            return true;
        }
        if (strcmp (msgId.c_str(), rhsChunkIdWr.msgId.c_str()) > 0) {
            return true;
        }
        return false;
    }

    inline bool Scheduler::ChunkIdWrapper::operator < (const ChunkIdWrapper &rhsChunkIdWr)
    {
        if (strcmp (requestingPeer.c_str(), rhsChunkIdWr.requestingPeer.c_str()) < 0) {
            return true;
        }
        if (strcmp (msgId.c_str(), rhsChunkIdWr.msgId.c_str()) < 0) {
            return true;
        }
        return false;
    }

    inline bool Scheduler::ChunkIdWrapper::operator == (const ChunkIdWrapper &rhsChunkIdWr)
    {
        return (requestingPeer == rhsChunkIdWr.requestingPeer) && (msgId == rhsChunkIdWr.msgId);
    }
    
    inline Scheduler::MsgProperties::MsgProperties (void)
        : ui8Priority (0)
    {
    }

    inline Scheduler::MsgProperties::~MsgProperties (void)
    {
    }
}

#endif	// INCL_SCHEDULER_H

