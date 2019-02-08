/*
 * SubscriptionStateCapped.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Extends SubscriptionState, putting a limit on the amount of messages added to the
 * MessageRequestScheduler. The limit (ML) is on the total of messages, so it's split between the various subscriptions
 * with the following method:
 * 0) put all the subscriptions on a list
 * 1) for each subscription (the State of SubscriptionState), consider the number of missing messages
 * 2) multiply that by a weight factor (now determined by the type of subscription, but can be changed with more options) to obtain
 *    the weighted priority (WP) of the subscription
 * 3) sum up all the results from point 2 to get the weighted messages total (WMT)
 * 4) do ML * WP / WMT to obtain the quota (amount of messages that are going to be added to the MessageRequestScheduler ) for each Subscription
 * 5) if some subscriptions have a quota higher than what they need (i.e. because their priority is high), set their quota to the number of missing messages
 *    and remove them from the list
 * 6) if 5 is true, start again from point 1, considering only the remaining subscriptions
 *    the algorithm ends when all the subscriptions have quota <=(their missing messages) or when the list is empty
 *
 * Author: Andrea Rossi    (arossi@ihmc.us)
 * Created on: Mar 15, 2011
 */

#ifndef INCL_SUBSCRIPTION_STATE_CAP_H
#define INCL_SUBSCRIPTION_STATE_CAP_H

#include "SubscriptionState.h"

namespace IHMC_ACI
{
    class SubscriptionStateCapped : public SubscriptionState
    {
        public:
            SubscriptionStateCapped (DisseminationService *pDisService, LocalNodeInfo *pLocalNodeInfo);
            virtual ~SubscriptionStateCapped (void);

            void setMaxMissingMessagesToFillUp (uint32 ui32Max);

            uint32 getFillUpAmount (const char *pszGroupName, const char *pszSenderNodeId);

            static const uint16 DEFAULT_MAX_MISSING_MESSAGES_TO_FILL_UP = 800;

        private:
            /*
            struct CapsByGroup {
                CapsByGroup (void);
                virtual ~CapsByGroup (void);
                NOMADSUtil::StringHashtable<uint32> _capsBySender;
            };*/

            struct NonSequentialReliableCommunicationStateCapped : public NonSequentialReliableCommunicationState
            {
                NonSequentialReliableCommunicationStateCapped (SubscriptionState *pParent);
                virtual ~NonSequentialReliableCommunicationStateCapped (void);

                virtual void fillUpMissingMessages (MessageRequestScheduler *pReqScheduler, MessageReassembler *pMessageReassembler,
                                                    const char *pszGroupName, const char *pszSenderNodeId,
                                                    uint32 ui32MissingFragmentTimeout);
            };

            struct SequentialReliableCommunicationStateCapped : public SequentialReliableCommunicationState
            {
                SequentialReliableCommunicationStateCapped (SubscriptionState *pParent);
                virtual ~SequentialReliableCommunicationStateCapped (void);

                virtual void fillUpMissingMessages (MessageRequestScheduler *pReqScheduler, MessageReassembler *pMessageReassembler,
                                                    const char *pszGroupName, const char *pszSenderNodeId,
                                                    uint32 ui32MissingFragmentTimeout);
            };

            virtual uint32 getSubscriptionState (const char * pszGroupName, const char * pszSenderNodeID);
            virtual void setSubscriptionState (const char * pszGroupName, const char * pszSenderNodeID, uint32 ui32NewNextExpectedSeqId);

            float getStateTypeBasedPriorityFactor (State* pState );

            //NOMADSUtil::StringHashtable<uint32> _capsBySender;

            uint32 _MaxNumberOfMissingMessagesToFillUp;
    };
}

#endif /* INCL_SUBSCRIPTION_STATE_H */
