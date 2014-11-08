/*
 * MessageReassembler.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#ifndef INCL_MESSAGE_REASSEMBLER_H
#define INCL_MESSAGE_REASSEMBLER_H

#include "DisServiceMsg.h"
#include "MessageInfo.h"
#include "MessageRequestScheduler.h"
#include "RequestsState.h"

#include "LoggingMutex.h"
#include "ManageableThread.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"

namespace IHMC_ACI
{
    class DataCacheInterface;
    class DisseminationService;
    class LocalNodeInfo;
    class Message;
    class MessageId;
    class PeerState;
    class SubscriptionState;
    class TransmissionService;

    class MessageReassembler : public NOMADSUtil::ManageableThread
    {
        public:
            /**
             * - bExpBackoff: set on true to multiplicatively decrease the rate
             *   missing fragment for a certain incomplete message are requested
             */
            MessageReassembler (DisseminationService *pDisService, PeerState *pPeerState,
                                SubscriptionState *pSubState, LocalNodeInfo *pLocalNodeInfo,
                                float fDefaultReqProb=DEFAULT_RANGE_REQUEST_PROB,
                                float fReceiveRateThreshold=DEFAULT_RECEIVE_RATE_THRESHOLD,
                                bool bExpBackoff=DEFAULT_ENABLE_EXPONENTIAL_BACKOFF,
                                bool bReqFragmentsForOpporAcquiredMsgs=DEFAULT_REQUEST_FRAGMENTS_FOR_OPPORTUNISTICALLY_ACQUIRED_MESSAGES);
            virtual ~MessageReassembler (void);

            bool usingExponentialBackOff (void);
            bool requestOpportunisticallyReceivedMessages (void);

            /**
             * Set/get the maximum number of requests that a message, or part of
             * it, can be requested.
             * -1 is equivalent to "no limit".
             *
             * NOTES:
             * - this method are _not_ thread safe, however, after that the
             *   object has been initialized, the are always called only by the
             *   MessageReassembler thread.
             * - setting the limit to UNLIMITED_MAX_NUMBER_OF_REQUESTS is equivalent
             *   to setting no limit.
             * - by default the limit is set to DEFAULT_MAX_NUMBER_OF_REQUESTS.
             * - any value lower than -1 will returns an error.
             */
            int setRequestLimit (int iLimit);
            int getRequestLimit (void);

            /**
             * Set a mapping between the priority tag and the probability with
             * which fragments/messages market with such a tag will be requested
             *
             * NOTES:
             * - this method are _not_ thread safe, however, after that the
             *   object has been initialized, the are always called only by the
             *   MessageReassembler thread.
             */
            void setRequestProbability (uint8 ui8Priority, float lReqProb);
            float getRequestProbability (uint8 ui8Priority) const;

            int addRequest (RequestInfo &reqInfo, NOMADSUtil::UInt32RangeDLList *pMsgSeqIDs);
            int addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                            NOMADSUtil::UInt8RangeDLList *pChunkIDs);

            /**
             * Return true if the message - or the part of the message - is contained in
             * the MessageReassembler. If bSpecificFragment is set to true, then the
             * exact Fragment is searched, otherwise it only checks if any fragment of
             * the message has been received.
             */
            bool containsMessage (const char *pszGroupName, const char *pszPublisherNodeId,
                                  uint32 ui32MsgSeqId);
            bool containsMessage (const char *pszGroupName, const char *pszPublisherNodeId,
                                  uint32 ui32MsgSeqId, uint8 ui8ChunkId);
            bool containsMessage (MessageId *pMsgId);
            bool containsMessage (MessageHeader *pMI, bool bSpecificFragment=true);

            /**
             * Returns true if the fragment specified is fully contained in the MessageReassembler
             */
            bool hasFragment (const char *pszGroupName, const char *pszPublisherNodeId,
                              uint32 ui32MsgSeqId, uint8 ui8ChunkId, uint32 ui32FragOffset,
                              uint16 ui16FragLength);

            /**
             * Returns true whether the message is in the message reassembler.
             * NOTE: the fact that the message information is in the message
             * reassembler does not meant that it is being requested. It is up
             * to the MessageReassembler policies to decide.
             */
            bool isBeingReassembled (const char *pszGroupName, const char *pszPublisherNodeId,
                                     uint32 ui32MsgSeqId, uint8 ui8ChunkId = MessageHeader::UNDEFINED_CHUNK_ID);

            /**
             * NOTE: The caller should NOT deallocate pMessage->_pData.
             * pMessage->_pMI and pMessage can be deallocated though, since
             * pMessage->_pMI is copied if needed and pMessage's destructor does
             * not delete the fields.
             *
             * Returns 0 is the fragment was added to the MessageReassebler, positive values
             * if the fragment is not relevant for the peer (and hence was not stored), or
             * negative values in case of error (also indicating that it was not stored)
             */
            int fragmentArrived (Message *pMessage, bool bIsNotTarget);

            /**
             * Removes the received message from the queue of requested messages.
             *
             * Returns the ID of the query with which the message was requested,
             * if any, null otherwise.
             */
            RequestDetails * messageArrived (Message *pMsg);

            void newPeer (void);

            void registerTransmissionService (TransmissionService *pTrSvc);

            void run (void);

            int sendRequest (RequestInfo &reqInfo, NOMADSUtil::UInt32RangeDLList *pMsgSeqIDs);
            int sendRequest (RequestInfo &reqInfo, uint32 ui32BeginEl, uint32 ui32EndEl,
                             NOMADSUtil::PtrLList<DisServiceDataReqMsg::FragmentRequest> *pMessageRequests);

            int sendRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId, NOMADSUtil::UInt8RangeDLList *pChunkIDs);
            int sendRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId, uint8 ui8BeginEl, uint8 ui8EndEl,
                             NOMADSUtil::PtrLList<DisServiceDataReqMsg::FragmentRequest> *pMessageRequests);

            static const bool DEFAULT_ENABLE_EXPONENTIAL_BACKOFF;
            static const int UNLIMITED_MAX_NUMBER_OF_REQUESTS;
            static const int DEFAULT_MAX_NUMBER_OF_REQUESTS;
            static const float DEFAULT_RANGE_REQUEST_PROB;
            static const float DEFAULT_RECEIVE_RATE_THRESHOLD;

            // The number of missing fragments below which missing fragments for
            // the message are always requested
            // TODO: this would be more meaningful if expressed in number of
            // missing bytes rather then number of missing fragments...
            static const uint8 ALWAYS_REQUEST_RANGES_THREASHOLD;

            // If the incoming messages queue is greater than this threshold,
            // missing fragment/message requests will not be sent.
            static const uint32 DEFAULT_INCOMING_QUEUE_SIZE_REQUEST_THRESHOLD;

            static const bool DEFAULT_REQUEST_FRAGMENTS_FOR_OPPORTUNISTICALLY_ACQUIRED_MESSAGES;

        private:
            void loadOpportunisticallyCachedFragment (void);

        private:
            typedef DisServiceDataReqMsg::FragmentRequest FragmentRequest;
            typedef DisServiceMsg::Range Range;

            struct FragmentWrapper
            {
                FragmentWrapper (uint32 ui32FragOffset, uint16 ui16FragLength, bool bIsChunkFrag);
                // NOTE: the destructor does not free pFragment
                ~FragmentWrapper (void);

                bool operator > (const FragmentWrapper &rhsFragment);
                bool operator == (const FragmentWrapper &rhsFragment);
                bool operator < (const FragmentWrapper &rhsFragment);

                const uint32 ui32FragmentOffset;
                const uint16 ui16FragmentLength;
                const bool bIsChunk;
                void *pFragment;
            };

            struct FragmentedMessage
            {
                // Construct a new instance of a FragmentedMessage for the message identified by pMI
                // NOTE: The new instance will keep a reference to pMI - so the caller should
                //       not use it anymore (or at least, not modify it)
                // NOTE: The constructor CHANGES the fragment offset and fragment length fields on pMI
                //       so the caller should not count on those values being preserved
                FragmentedMessage (MessageHeader *pMI, bool isReliable, bool isSequenced, bool bIsInHistory, bool bNotTarget);

                // NOTE: The following constructor is only used when initializing the object
                //       for the purpose of searching in the PtrLList
                FragmentedMessage (uint32 ui32MsgSeqId, uint8 ui8NewMsgChunkId);

                virtual ~FragmentedMessage (void);

                void addFragment (FragmentWrapper *pFragmentWrap);
                uint64 getCachedBytes (void) const;
                FragmentWrapper * getFirstFragment (void);
                FragmentWrapper * getNextFragment (void);
                FragmentWrapper * searchFragment (FragmentWrapper &fragment);

                void resetMissingFragmentsList (void);
                void updateNextExpectedValue (void);

                bool operator > (const FragmentedMessage &rhsMessage);
                bool operator == (const FragmentedMessage &rhsMessage);
                bool operator < (const FragmentedMessage &rhsMessage);                

                MessageHeader *pMH;
                uint32 ui32NextExpected;
                int64 i64LastNewDataArrivalTime;
                int64 i64LastMissingFragmentsRequestTime;  // Keeps track of the last time when missing
                                                           // fragments were requested for this message
                const uint32 ui32MsgSeqId;
                const uint8 ui8ChunkID;
                const bool bReliabilityRequired;
                const bool bSequentialTransmission;
                bool bMetaDataDelivered;
                const bool bIsHistoryMsg;
                int iRequested;         // The number of times that the message
                                        // was requested

                const bool bIsNotTarget;  // True only if the target for the message
                                          // was set, the node did not match it

                RequestDetails *pRequestDetails;
                NOMADSUtil::PtrLList<Range> missingFragments;

                private:
                    uint64 ui64CachedBytes;
                    NOMADSUtil::PtrLList<FragmentWrapper> fragments;                    
            };

            struct MessagesByPublisher
            {
                MessagesByPublisher (void);
                ~MessagesByPublisher (void);
                NOMADSUtil::PtrLList<FragmentedMessage> messages;
            };

            struct MessagesByGroup
            {
                MessagesByGroup (void);
                ~MessagesByGroup (void);
                NOMADSUtil::StringHashtable<MessagesByPublisher> messagesInGroup;
            };

            /**
             * Return true if the missing fragment must be added, false otherwise
             * ADDING POLICY:
             * A missing fragment must be added to the missing fragment list if:
             * -
             */
            bool addingRangeRequired (FragmentedMessage *pFragMsg);

            /**
             * Deliver a complete message or a complete metadata part of a message
             * to DisseminationService
             */
            int deliverCompleteMessage (MessageHeader *pMI, void *pData, uint32 ui32DataLength,
                                        MessagesByPublisher *pMS, FragmentedMessage *pFragMsg,
                                        bool bIsMetaDataPart, bool bIsNotTarget);

            /**
             * Return all the FragmentedMessage's contained in the MessageReassembler
             * wrapped in a PtrLList
             */
            void fillMessageRequestScheduler (void);

            int64 getMissingFragmentRequestTimeOut (int64 i64CurrentTime, int64 i64FragmentLastNewDataArrivalTime);

            /**
             * Check to see if all the fragments that comprise a fragmented message
             * have been received and hence the message is ready to be reassembled
             * and delivered.
             *
             * Returns true if the message is complete, false otherwise.
             *
             * NOTE: The method also updates the missingFragments list in the
             *       FragmentedMessage
             * NOTE: If data is missing at the end (i.e., from the end of the
             *       last fragment that has been received until the end of the
             *       message), that is not added as a range to the
             *       missingFragments list.
             */
            enum {
                UNCOMPLETE = 0x00,
                MSG_COMPLETE = 0x01,
                METADATA_COMPLETE = 0x02,
                DUMMY_MSG = 0x03
            };

            bool getAndResetNewPeer (void);
            bool isBeingReassembledInternal (const char *pszGroupName, const char *pszSenderNodeId,
                                             uint32 ui32MsgSeqId, uint8 ui8ChunkId, bool bCheckRequestState);
            void loadOpportunisticallyCachedFragments (void);
            uint8 messageComplete (FragmentedMessage *pFragMsg);

            /**
             * Reassemble a fragmented message (or only the metadata portion) for
             * which all fragments have been received
             *
             * NOTE: The caller must have already verified that all fragments are present
             * NOTE: The caller must free the buffer that is allocated and returned
             */
            void * reassemble (FragmentedMessage *pFragMsg, bool bReassembleOnlyMetaDataPart,
                               uint32 &ui32BytesWritten);

            /**
             * If the node is not interested in reassembling the message, the
             * method returns 0, it returns a negative number otherwise.
             */
            int getSubscriptionParameters (Message *pMessage, bool bIsNotTarget,
                                           bool &isSequenced, bool &isReliable);

            /**
             * Updates the missing fragments list
             */
            int updateMissingFragmentsList (FragmentedMessage *pFMH);
            int addToRequestScheduler (FragmentedMessage *pFragMsg, int64 i64CurrentTime);

            int sendRequestInternal (RequestInfo &reqInfo, uint32 ui32BeginEl, uint32 ui32EndEl,
                                     NOMADSUtil::PtrLList<FragmentRequest> *pMessageRequests);

            int sendRequestInternal (RequestInfo &reqInfo, uint32 ui32MsgSeqId, uint8 ui8BeginEl,
                                     uint8 ui8EndEl, NOMADSUtil::PtrLList<FragmentRequest> *pMessageRequests);

        private:
            NOMADSUtil::StringHashtable<MessagesByGroup> _receivedMessages;
            NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::LoggingMutex _newPeerMutex;
            bool _bNewPeer;
            bool _bExpBackoff;
            bool _bReqFragmentsForOpporAcquiredMsgs; // Request missing fragments
                                                     // for opportunistically
                                                     // acquired messages
            int _iMaxNumberOfReqs;
            float _fRangeRequestProb;
            float _fReceiveRateThreshold;
            uint32 _ui32IncomingQueueSizeRequestThreshold;
            LocalNodeInfo *_pLocalNodeInfo;
            SubscriptionState *_pSubState;
            PeerState *_pPeerState;
            DisseminationService *_pDisService;
            TransmissionService *_pTrSvc;
            MessageRequestScheduler _requestSched;
            RequestsState _requestState;
    };

    class MessageReassemblerUtils
    {
        public:
            /**
             * If the message reassembler does not have any fragments related to the message identified by
             * the MessageHeader (as defined by the GroupName, SenderNodeId, and MsgSeqId), this method checks
             * the data cache to see if there are any fragments for the message (identified by MessageHeader)
             * and loads those fragments into the data cache.
             *
             * Returns true if the message was also completed and delivered by the reassembler (implying that this
             * message fragment no longer needs to be passed to the MessageReassembler).
             *
             * Returns false if the message is still missing fragments.
             */
            static bool loadOpportunisticallyCachedFragments (MessageReassembler *pMsgReassembler, DataCacheInterface *pDCI,
                                                              MessageHeader *pMH, bool bNotTarget);
            static bool loadAllOpportunisticallyCachedFragments (MessageId *pMsgId, MessageReassembler *pMsgReassembler,
                                                                 DataCacheInterface *pDCI);

        private:
            static bool loadOpportunisticallyCachedFragmentsInternal (MessageId *pMsgId, MessageReassembler *pMsgReassembler,
                                                                      DataCacheInterface *pDCI, MessageHeader *pMHFilter, bool bNotTarget);
    };

    //==========================================================================
    // MessageReassembler
    //==========================================================================
    inline bool MessageReassembler::requestOpportunisticallyReceivedMessages()
    {
        return _bReqFragmentsForOpporAcquiredMsgs;
    }

    //==========================================================================
    //  STRUCT FragmentWrapper inline functions
    //==========================================================================
    inline MessageReassembler::FragmentWrapper::FragmentWrapper (uint32 ui32FragOffset, uint16 ui16FragLength, bool bIsChunkFrag)
        : ui32FragmentOffset (ui32FragOffset),
          ui16FragmentLength (ui16FragLength),
          bIsChunk (bIsChunkFrag)
    {
        pFragment = NULL;
    }

    inline MessageReassembler::FragmentWrapper::~FragmentWrapper()
    {
    }

    //==========================================================================
    //  STRUCT MessagesBySender inline functions
    //==========================================================================
    inline MessageReassembler::MessagesByPublisher::MessagesByPublisher (void)
        : messages (false)
    {
    }

    //==========================================================================
    //  STRUCT MessagesByGroup inline functions
    //==========================================================================
    inline MessageReassembler::MessagesByGroup::MessagesByGroup (void)
        : messagesInGroup (true,  // bCaseSensitiveKeys
                           true,  // bCloneKeys
                           true,  // bDeleteKeys
                           true)  // bDeleteValues
    {
    }

    //==========================================================================
    //  STRUCT FragmentWrapper inline functions
    //==========================================================================
    inline bool MessageReassembler::FragmentWrapper::operator > (const FragmentWrapper &rhsFragment)
    {
        return (ui32FragmentOffset > rhsFragment.ui32FragmentOffset);
    }

    inline bool MessageReassembler::FragmentWrapper::operator < (const FragmentWrapper &rhsFragment)
    {
        return (ui32FragmentOffset < rhsFragment.ui32FragmentOffset);
    }

    inline bool MessageReassembler::FragmentWrapper::operator == (const FragmentWrapper &rhsFragment)
    {
        return (ui32FragmentOffset == rhsFragment.ui32FragmentOffset &&
                rhsFragment.ui16FragmentLength <= ui16FragmentLength);
    }

    //==========================================================================
    //  STRUCT FragmentedMessage inline functions
    //==========================================================================
    inline bool MessageReassembler::FragmentedMessage::operator > (const FragmentedMessage &rhsMessage)
    {
        if (ui32MsgSeqId > rhsMessage.ui32MsgSeqId) {
            return true;
        }
        else if (ui32MsgSeqId == rhsMessage.ui32MsgSeqId) {
            return ui8ChunkID > rhsMessage.ui8ChunkID;
        }
        else {
            return false;
        }
    }

    inline bool MessageReassembler::FragmentedMessage::operator < (const FragmentedMessage &rhsMessage)
    {
        if (ui32MsgSeqId < rhsMessage.ui32MsgSeqId) {
            return true;
        }
        else if (ui32MsgSeqId == rhsMessage.ui32MsgSeqId) {
            return ui8ChunkID < rhsMessage.ui8ChunkID;
        }
        else {
            return false;
        }
    }

    inline bool MessageReassembler::FragmentedMessage::operator == (const FragmentedMessage &rhsMessage)
    {
        return ((ui32MsgSeqId == rhsMessage.ui32MsgSeqId) && (ui8ChunkID == rhsMessage.ui8ChunkID));
    }    
}

#endif   // #ifndef INCL_MESSAGE_REASSEMBLER_H
