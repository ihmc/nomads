/**
 * SubscriptionState.h
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
 * SubscriptionState records the next expected sequence id for each couple
 * <groupName, senderNodeId>. Plus, generates a list of all the missing
 * comlete messages that need to be requested.
 *
 * SubscriptionState caches out-of-order messages that need to be buffered as
 * long as the preceding ones are not received.
 * This is done by bufferMessage().
 *
 * In case the transmission is non-sequenced, there is no need to buffer
 * out-of-order messages, however it is still necessary to keep track of the
 * previous messages that have not been received yet.
 * This is done by addMissingMessage().
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#ifndef INCL_SUBSCRIPTION_STATE_H
#define INCL_SUBSCRIPTION_STATE_H

#include "DisServiceMsg.h"

#include "DArray.h"
#include "FTypes.h"
#include "LoggingMutex.h"
#include "PtrLList.h"
#include "RangeDLList.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"

namespace IHMC_ACI
{
    class DisseminationService;
    class LocalNodeInfo;
    class Message;
    class MessageReassembler;
    class MessageRequestScheduler;
}

namespace IHMC_ACI
{
    struct RequestDetails;

    class SubscriptionState
    {
        public:
            SubscriptionState (DisseminationService *pDisService, LocalNodeInfo *pLocalNodeInfo);
            virtual ~SubscriptionState (void);

            /**
             * NOTE: for efficiency reasons, SubscriptionState DOES NOT MAKE A
             *       COPY of pMsg. Thus it must NOT be deallocated by the caller
             *
             * NOTE: pMsg's inner structures are passed to the client application,
             *       so DisService can not prevent them from being modified
             */
            int messageArrived (Message *pMsg, RequestDetails *pDetails);
            int messageArrived (Message *pMsg, bool bIsInHistory, RequestDetails *pDetails);

            /**
             * Returns true if the _complete_ message identified by pszGroupName,
             * pszSenderNodeID and ui32SeqIdSubscriptionState is buffered in
             * SubscriptionState.
             * NOTE: if it is only the metadata part that is buffered, then the
             * method returns false
             */
            bool containsMessage (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId);

            /**
             * Returns a list containing the messages for which no one fragments
             * has been received.
             * NOTE: the list and its elements must be deallocated after use.
             */
            virtual void getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                                    MessageReassembler *pMessageReassembler);

            virtual uint32 getSubscriptionState (const char *pszGroupName, const char *pszSenderNodeID);

            /**
             * Return true if the message is still relevant for the application.
             * Depending on the kind of subscription different policies are used
             * to evaluate relevance.
             *
             * WARNING: this method does not check weather a subscriptions
             * exists for pszGroupName, it only check a seqId-wise relevance,
             * thus, in case no state exists for pszGroupName, true is returned.
             */
            bool isRelevant (const char *pszGroupName, const char *pszSenderNodeID,
                             uint32 ui32IncomingMsgSeqId, uint8 ui8ChunkId);

            /**
             * Returns true if there has not been received any message that
             * belongs to the specified group and that was published by the
             * specified sender.
             */
            bool isSubscriptionBySenderEmpty (const char *pszGroupName, const char *pszSenderNodeID);

            virtual void setSubscriptionState (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32NewNextExpectedSeqId);

        protected:
            struct State;

            uint32 getNumberOfMissingFragments (State *pState);

            // Non-thread-safe version of messageArrived.
            uint32 getSubscriptionStateInternal (const char *pszGroupName, const char *pszSenderNodeID);
            bool isRelevantInternal (const char *pszGroupName, const char *pszSenderNodeID,
                                     uint32 ui32IncomingMsgSeqId, uint8 ui8ChunkId);

            //////////////// SEQUENCED RELIABLE TRANSMISSION ///////////////////

            /*
             * Buffer an out-of-order message.
             *
             * NOTE: Use this if the communication is sequential and reliable
             */
            void bufferMessage (Message *pMsg);

            /*
             * Returns the wanted message or NULL if the message is
             * not buffered AND delete the message (if any) from the buffer.
             */
            Message * unbufferMessage (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId);

            ////////////// UNSEQUENCED UNRELIABLE TRANSMISSION /////////////////

            void nonSequentialUnreliableMessageArrived (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId);

            /////////////// UNSEQUENCED RELIABLE TRANSMISSION //////////////////

            /*
             * Remove the arrived message from the list of missing messages. If
             * this method is not called upon the arrival of a message, it may
             * be requested again
             */
            void nonSequentialReliableMessageArrived (const char *pszGroupName, const char *pszSenderNodeID, uint32 ui32SeqId);

            /*
             * Notifies the DisseminationService that a message is ready to be
             * delivered
             */
            void notifyDisseminationServiceAndDeallocateMessage (Message *pMessage, RequestDetails *pDetails);
            void notifyDisseminationService (Message *pMessage, RequestDetails *pDetails);
            void deallocateMessage (Message *pMessage);

            enum StateType {
                SEQ_REL_STATE = 0x00,       // SEQUENCED       RELIABLE
                NONSEQ_REL_STATE = 0x01,    // NON SEQUENTIAL  RELIABLE
                NONSEQ_UNREL_STATE = 0x02,  // NON SEQUENTIAL  UNRELIABLE
                SEQ_UNREL = 0x03            // SEQUENCED       UNRELIABLE
            };

            struct State
            {
                State (uint8 ui8Type, SubscriptionState *pParent);
                virtual ~State (void);

                // Passing pszGroupName and pszSenderId is necessary to create the search template
                virtual void fillUpMissingMessages (MessageRequestScheduler *pReqScheduler, MessageReassembler *pMessageReassembler,
                                                    const char *pszGroupName, const char *pszSenderNodeId,
                                                    uint32 ui32MissingFragmentTimeout);
                virtual bool isRelevant (uint32 ui32IncomingMsgSeqId)=0;

                bool bExpectedSeqIdSet;
                uint32 ui32ExpectedSeqId;
                int64 i64LastNewMessageArrivalTime;

                SubscriptionState *pParent;
                uint8 _ui8Type;
            };

            struct ReliableState : public State
            {
                ReliableState (uint8 ui8Type, SubscriptionState *pParent);
                virtual ~ReliableState (void);

                NOMADSUtil::UInt32Hashtable<int> requestCounters; // "Key" is the message ID,
                                                                  // "value" is the counter
            };

            struct NonSequentialUnreliableCommunicationState : public State
            {
                NonSequentialUnreliableCommunicationState (SubscriptionState *pParent);
                virtual ~NonSequentialUnreliableCommunicationState (void);

                virtual bool isRelevant (uint32 ui32IncomingMsgSeqId);

                NOMADSUtil::UInt32RangeDLList receivedSeqId;
            };

            struct NonSequentialReliableCommunicationState : public ReliableState
            {
                NonSequentialReliableCommunicationState (SubscriptionState *pParent);
                virtual ~NonSequentialReliableCommunicationState (void);

                virtual void fillUpMissingMessages (MessageRequestScheduler *pReqScheduler, MessageReassembler *pMessageReassembler,
                                                    const char *pszGroupName, const char *pszSenderNodeId,
                                                    uint32 ui32MissingFragmentTimeout);
                virtual bool isRelevant (uint32 ui32IncomingMsgSeqId);

                bool bHighestSeqIdRcvdSet;
                uint32 ui32HighestSeqIdReceived;
                int64 i64LastMissingMessageRequestTime;

                NOMADSUtil::PtrLList<uint32> missingSeqId;
            };

            struct SequentialUnreliableCommunicationState : public State
            {
                SequentialUnreliableCommunicationState (SubscriptionState *pParent);
                virtual ~SequentialUnreliableCommunicationState (void);

                virtual bool isRelevant (uint32 ui32IncomingMsgSeqId);
            };

            struct SequentialReliableCommunicationState : public ReliableState
            {
                SequentialReliableCommunicationState (SubscriptionState *pParent);
                virtual ~SequentialReliableCommunicationState (void);

                virtual void fillUpMissingMessages (MessageRequestScheduler *pReqScheduler, MessageReassembler *pMessageReassembler,
                                                    const char *pszGroupName, const char *pszSenderNodeId,
                                                    uint32 ui32MissingFragmentTimeout);
                virtual bool isRelevant (uint32 ui32IncomingMsgSeqId);

                /*
                 * After a certain time, or number of attempts, SubsriptionState
                 * does not wait for the missing message to arrive and delivers
                 * the buffered messages that are kept in hold because of it.
                 */
                void skipMessage (uint32 ui32MsgSeqId);

                NOMADSUtil::UInt32Hashtable<Message> bufferedCompleteMessages;
                uint32 ui32HighestSeqIdBuffered;
                int64 i64LastMissingMessageRequestTime;
            };

            struct ByGroup
            {
                ByGroup (void);
                virtual ~ByGroup (void);
                NOMADSUtil::StringHashtable<State> _statesBySender;
            };

            class ReceivedChunks
            {
                public:
                    ReceivedChunks (void);
                    ~ReceivedChunks (void);

                    int put (const char *pszGroupName, const char *pszSenderNodeId,
                             uint32 ui32MsgSeqId, uint8 ui8ChunkId);

                    bool contains (const char *pszGroupName, const char *pszSenderNodeId,
                                   uint32 ui32MsgSeqId, uint8 ui8ChunkId);

                private:
                    typedef NOMADSUtil::UInt8RangeDLList ChunkIdList;
                    typedef NOMADSUtil::UInt32Hashtable<ChunkIdList> BySeqId;
                    typedef NOMADSUtil::StringHashtable<BySeqId> BySender;

                    NOMADSUtil::StringHashtable<BySender> _byGroupName;
            };

            NOMADSUtil::StringHashtable<ByGroup> _states;
            ReceivedChunks _rcvdChunks;

            DisseminationService *_pDisService;
            LocalNodeInfo *_pLocalNodeInfo;
            MessageRequestScheduler *_pMsgReqScheduler;

            NOMADSUtil::LoggingMutex _m;
    };
}

#endif  // end INCL_SUBSCRIPTION_STATE_H
