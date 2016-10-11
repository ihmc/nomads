/* 
 * SchedulerQueueManagment.h
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
 * Collection of policies (or strategies) to manage the Scheduler
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on July 11, 2011, 7:20 PM
 */

#ifndef INLC_SCHEDULER_POLICIES_H
#define	INLC_SCHEDULER_POLICIES_H

#include "Scheduler.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DSPro;
    class InformationStoreAdaptor;
    class MetadataGenerator;
    class Scheduler;
 
    //--------------------------------------------------------------------------
    // Queue Replacement Policies
    //--------------------------------------------------------------------------

    class QueueReplacementPolicy
    {
        public:
            QueueReplacementPolicy (void);
            virtual ~QueueReplacementPolicy (void);

            virtual bool isReplaceable (Scheduler::MsgIDWrapper *pMsgIdWr) = 0;            
    };

    class ReplaceAllPolicy : public QueueReplacementPolicy
    {
        public:
            ReplaceAllPolicy (void);
            virtual ~ReplaceAllPolicy (void);

            bool isReplaceable (Scheduler::MsgIDWrapper *pMsgIdWr);
    };

    class ReplaceNonePolicy : public QueueReplacementPolicy
    {
        public:
            ReplaceNonePolicy (void);
            virtual ~ReplaceNonePolicy (void);

            bool isReplaceable (Scheduler::MsgIDWrapper *pMsgIdWr);
    };

    class ReplaceLowPriorityPolicy : public QueueReplacementPolicy
    {
        public:
            /**- pThresholds: array that contains threshold.
             *   session, messages without any index with value greater than
             *   fUnreplacebleEnqueuedMessageIndex, will be removed from the
             *   queue.
             *   If fUnreplacebleEnqueuedMessageThreashold < 0, this feature is
             *   disabled, and every enqueued message will be removed from the
             *   queue upon the starting of a new replication session.
             */
            ReplaceLowPriorityPolicy (float *pThresholds, unsigned int iLen);
            virtual ~ReplaceLowPriorityPolicy (void);

            bool isReplaceable (Scheduler::MsgIDWrapper *pMsgIdWr);

        private:
            bool isReplaceable (float indexValue, unsigned int uiThresholdIndex);

        private:
            unsigned int _uiLen;
            float *_pThresholds;
    };

    //--------------------------------------------------------------------------
    // QueueReplacementPolicyFactory
    //--------------------------------------------------------------------------

    class QueueReplacementPolicyFactory
    {
        public:
            static QueueReplacementPolicy * getQueueReplacementPolicy (NOMADSUtil::ConfigManager *pCfgMgr);
    };

    //--------------------------------------------------------------------------
    // Metadata Message Mutation Policies
    //--------------------------------------------------------------------------

    class MetadataMutationPolicy
    {
        public:
            MetadataMutationPolicy (void);
            virtual ~MetadataMutationPolicy (void);

            /**
             * Returns the ID of the mutatad metadata message or NULL if the
             * metadata was not mutated
             */
            virtual char * mutate (const char *pszOriginalMetadataId,
                                   NodeIdSet &targetNodes,
                                   RankByTargetMap &ranksByTarget,
                                   Scheduler *pScheduler) = 0;  
    };

    class DefaultMutationPolicy : public MetadataMutationPolicy
    {
        public:
            DefaultMutationPolicy (void);
            virtual ~DefaultMutationPolicy (void);

            /**
             * Always returns NULL
             */
            virtual char * mutate (const char *pszOriginalMetadataId,
                                   NodeIdSet &targetNodes,
                                   RankByTargetMap &ranksByTarget,
                                   Scheduler *pScheduler);            
    };

    class PrevMsgMutationPolicy : public MetadataMutationPolicy
    {
        public:
            PrevMsgMutationPolicy (MetadataGenerator *pMutator);
            virtual ~PrevMsgMutationPolicy (void);

            /**
             * Replace the original metadata with a metadata that contains the
             * ID of the previous message that was sent to the target.
             *
             * NOTE: the returned ID should be deallocated by the caller
             */
            virtual char * mutate (const char *pszOriginalMetadataId,
                                   NodeIdSet &targetNodes,
                                   RankByTargetMap &ranksByTarget,
                                   Scheduler *pScheduler);

        private:
            MetadataGenerator *_pMutator;
    };

    //--------------------------------------------------------------------------
    // MetadataMutationPolicyPolicyFactory
    //--------------------------------------------------------------------------

    class MetadataMutationPolicyFactory
    {
        public:
            static MetadataMutationPolicy * getMetadataMutationPolicy (NOMADSUtil::ConfigManager *pCfgMgr, DSProImpl *pDSPro,
                                                                       InformationStore *pInfoStore,
                                                                       Scheduler::PrevPushedMsgInfoMode &prevPushedMsgInfo);
    };

};

#endif	// INLC_SCHEDULER_POLICIES_H

