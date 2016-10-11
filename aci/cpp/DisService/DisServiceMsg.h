/*
 * DisServiceMsg.h
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
 */

#ifndef INCL_DIS_SERVICE_MESSAGE_H
#define INCL_DIS_SERVICE_MESSAGE_H

#include "SubscriptionList.h"

#include "BufferReader.h"
#include "BufferWriter.h"
#include "DArray2.h"
#include "PtrQueue.h"
#include "ReceivedMessages.h"
#include "StringFloatHashtable.h"

namespace NOMADSUtil
{
   class InstrumentedWriter;
   class String;
   class Reader;
   class Writer;
   class UInt32RangeDLList;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class DisServiceDataCacheQuery;
    class LocalNodeInfo;
    class Message;
    class MessageInfo;
    class MessageRequestScheduler;
    class ChunkMsgInfo;
    class MessageHeader;
    class RemoteNodeInfo;
    class WorldState;
    class SubscriptionList;

    //==========================================================================
    //  DisServiceMsg BASE CLASS
    //==========================================================================
    class DisServiceMsg
    {
        public:
            enum Type {
                DSMT_Unknown = 0x00,
                DSMT_Data = 0x01,
                DSMT_DataReq = 0x02,
                DSMT_WorldStateSeqId = 0x03,
                DSMT_SubStateMessage = 0x04,
                DSMT_SubStateReq = 0x05,
                DSMT_TopologyStateMessage = 0x06,
                DSMT_DataCacheQuery = 0x07,
                DSMT_DataCacheQueryReply = 0x08,
                DSMT_DataCacheMessagesRequest = 0x09,
                DSMT_AcknowledgmentMessage = 0x10,
                DSMT_CompleteMessageReq = 0x11,
                DSMT_CacheEmpty = 0x12,
                DSMT_CtrlToCtrlMessage = 0x13,
                DSMT_ChunkReq = 0x14,
                DSMT_HistoryReq = 0x15,
                DSMT_HistoryReqReply = 0x16,

                CRMT_Query = 0x17,
                CRMT_QueryHits = 0x18,
                DSMT_SubAdvMessage = 0x19,

                DSMT_SearchMsg = 0x20,
                DSMT_SearchMsgReply = 0x2A,
                DSMT_VolatileSearchMsgReply = 0x2B,

                DSMT_ImprovedSubStateMessage = 0x2C,
                DSMT_ProbabilitiesMsg = 0x2D
            };

            struct Range {
                public:
                    Range (uint32 ui32From, uint32 ui32To);
                    ~Range (void);

                    bool operator == (const Range &range) const;

                    // < and > operators return random values in order to have a
                    // randomly ordered list
                    bool operator > (const Range &range) const;
                    bool operator < (const Range &range) const;

                    uint32 getFrom (void) const;
                    uint32 getTo (void) const;

                private:
                    const uint32 from;
                    const uint32 to;
            };

            virtual ~DisServiceMsg (void);

            virtual int display (FILE *pFileOut);

            /**
             * Returns the number of bytes have been written in the buffer of that
             * have been read from the buffer.
             * NOTE: Make sure to call it after the read or the write has been
             * called (otherwise this method will always return 0)
             */
            virtual uint16 getSize (void) const;
            virtual void flush (void);

            virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            virtual int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);

            Type getType (void) const;
            const char * getSenderNodeId (void) const;
            const char * getSessionId (void) const;
            const char * getTargetNodeId (void) const;

            void setSenderNodeId (const char *pszId);
            void setSessionId (const char *pszSessionId);
            void setTargetNodeId (const char *pszNodeId);

        protected:
            DisServiceMsg (Type type);
            DisServiceMsg (Type type, const char *pszSenderNodeId);
            DisServiceMsg (Type type, const char *pszSenderNodeId, const char *pszTargetNodeId);

        protected:
            Type _type;
            uint16 _ui16Size;
            NOMADSUtil::String _senderNodeId;
            NOMADSUtil::String _targetNodeId;
            NOMADSUtil::String _sessionId;
    };

    //=================================================================
    // DisServiceMsg DATA
    //=================================================================

    class DisServiceDataMsg : public DisServiceMsg
    {
        public:
            DisServiceDataMsg (void);
            DisServiceDataMsg (const char *pszSenderNodeId, Message *pMsg);
            DisServiceDataMsg (const char *pszSenderNodeId, Message *pMsg, const char *pszTargetNodeId);
            virtual ~DisServiceDataMsg (void);

            // NOTE: This class will NOT delete pMsg in the destructor
            void setMessage (Message *pMsg);

            MessageHeader * getMessageHeader (void);
            const void * getPayLoad (void);
            Message * getMessage (void);
            bool isChunk (void) const;

            /**
             * Returns true, if the peer sending the DisServiceDataMsg is sending
             * the whole message or just this fragment.
             * This information can be used for varius purposes, for instance,
             * the peer may wait for the sender to finish sending the message
             * before requesting the missing fragments.
             * Alternatively, it may be used to start monitoring the rate with
             * which the fragments are received (assuming that the fragments are
             * sent in order)
             */
            bool getSendingCompleteMsg (void) const;
            void setSendingCompleteMsg (bool bSendingCompleteMsg);

            bool isRepair (void) const;
            void setRepair (bool bIsRepair);

            bool hasSendRate (void) const;
            bool hasRateEstimate (void) const;
            void setSendRate (uint32 ui32SendRate);
            void setRateEstimate (uint32 ui32RateEstimate);

            uint32 getRateEstimationInfo (void) const;

            void setDoNotForward (bool bDoNotForward);
            bool doNotForward (void) const;

            // NOTE: This method will allocate a new Message object but it will not be deleted in the destructor
            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        private:
            // Binary Masks
            enum BinaryFlags {
                IS_CHUNK = 0x01,
                SENDING_COMPL_MSG = 0x02,
                HAS_STATS = 0x04,
                IS_REPAIR = 0x08,
                HAS_SEND_RATE = 0x10,
                HAS_RATE_ESTIMATE = 0x20,
                DO_NOT_FORWARD = 0x40
            };

            // Flags
            bool _bChunk;
            bool _bSendingComplete;
            bool _hasStats;
            bool _bIsRepair;
            bool _bHasSendRate;
            bool _bHasRateEstimate;
            bool _bDoNotForward;

            uint32 _ui32RateEstimationInfo;

            Message *_pMsg;
    };

    //=================================================================
    // DisServiceCtrlMsg CONTROL
    //=================================================================

    class DisServiceCtrlMsg : public DisServiceMsg
    {
        public:
            uint32 getCtrlMsgSeqNo (void);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);

        protected:
            DisServiceCtrlMsg (Type type);
            DisServiceCtrlMsg (Type type, const char *pszSenderNodeId);
            DisServiceCtrlMsg (Type type, const char *pszSenderNodeId, const char * pszTargetNodeId);

            static uint32 allocateNextSeqNo (void);

        private:
            uint32 _ui32CtrlMsgSeqNo;

        private:
            static uint32 _ui32NextCtrlMsgSeqNo;
            static NOMADSUtil::Mutex _mCtrlMsgSeqNo;
    };

    //=================================================================
    // DisServiceDataReqMsg CONTROL
    //=================================================================

    class DisServiceDataReqMsg : public DisServiceCtrlMsg
    {
        public:
            struct FragmentRequest
            {
                /**
                 * FragmentRequest is just a wrapper, it does not make a copy of
                 * pMH and pRanges thus it does not deallocate them in the
                 * constructor
                 */
                FragmentRequest (MessageHeader *pMH, NOMADSUtil::PtrLList<Range>* pRanges);
                FragmentRequest (MessageHeader *pMH, NOMADSUtil::PtrLList<Range>* pRanges,
                                 uint32 ui32NextExpectedOffset, uint32 ui32TotalMessageLength);
                virtual ~FragmentRequest (void);

                MessageHeader *pMsgHeader;
                NOMADSUtil::PtrLList<Range> *pRequestedRanges;
                bool bRequestMessageTail;
                uint32 ui32NextExpectedOffset;
                uint32 ui32TotalMessageLength;
                bool bDelValues;

                bool operator == (FragmentRequest &toCompare);
                // < and > operators return random values in order to have a
                // randomly ordered list
                bool operator > (FragmentRequest &toCompare);
                bool operator < (FragmentRequest &toCompare);
            };

            struct MessageRequest : public FragmentRequest
            {
                /**
                 * MessageRequest makes a copy of pszGroupName, pszSenderNodeId
                 * and ui32MsgSeqId
                 */
                MessageRequest (const char *pszGroupName, const char *pszSenderNodeId,
                                uint32 ui32MsgSeqId, uint8 ui8Priority=0);
                virtual ~MessageRequest (void);
            };

            struct ChunkMessageRequest : public FragmentRequest
            {
                /**
                 * ChunkMessageRequest makes a copy of pszGroupName, pszSenderNodeId
                 * and ui32MsgSeqId
                 */
                ChunkMessageRequest (const char *pszGroupName, const char *pszSenderNodeId,
                                     uint32 ui32MsgSeqId, uint8 ui8ChunkId, uint8 ui8Priority=0);
                virtual ~ChunkMessageRequest (void);
            };

            DisServiceDataReqMsg (void);
            /**
             * NB: For efficiency reasons DisServiceDataReqMsg does NOT make a
             * copy of pMessageRequests nor pFragmentRequests and it does not
             * deallocate them.
             */
            DisServiceDataReqMsg (const char *pszSenderNodeId, const char* pszQueryTargetNodeId,
                                  uint16 ui16NumberOfNeighbors,
                                  NOMADSUtil::PtrLList<FragmentRequest> *pMessageRequests,
                                  NOMADSUtil::PtrLList<FragmentRequest> *pFragmentRequests);

            /**
             * NB: For efficiency reasons DisServiceDataReqMsg does NOT make a
             * copy of any of the elements passed to the constructor and the
             * destructor does NOT delete on them.
             */
            virtual ~DisServiceDataReqMsg (void);

            virtual int display (FILE *pFileOut);
            virtual int displayCompleteMessageReq (void);

            void setFragmentRequests (NOMADSUtil::PtrLList<FragmentRequest> *pFragmentedRequests);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            const char * getTargetNodeId (void);
            uint16 getNumberOfActiveNeighbors (void);

            NOMADSUtil::PtrLList<DisServiceDataReqMsg::FragmentRequest> * getRequests (void);
            NOMADSUtil::PtrLList<DisServiceDataReqMsg::FragmentRequest> * relinquishRequests (void);

        protected:
            static const uint8 HAS_NEXT;
            static const uint8 DOES_NOT_HAVE_NEXT;

            /**
             * Return 0 if at least 1 range was written, -1 otherwise
             */
            int writeRequest (FragmentRequest *pFragmentRequest, NOMADSUtil::BufferWriter &bw,
                              uint32 ui32MaxSize, uint32 ui32WrittenBytes);

            virtual int display (FragmentRequest *pFragRequest, FILE *pFileOut);

            uint16 _ui16NumberOfNeighbors;

        private:
            int writeRequests (NOMADSUtil::PtrLList<FragmentRequest> *pRequests,
                               NOMADSUtil::InstrumentedWriter *pIW,
                               uint32 ui32MaxSize, bool &bBufferFull);

            void writeRange (NOMADSUtil::Writer *pWriter, uint32 ui32Start, uint32 ui32Len);
            enum BinaryFlags {
                WRITE_CHUNK = 0x01
            };

            enum ReadingMask {
                READ_CHUNK = 0xFE
            };

            NOMADSUtil::PtrLList<FragmentRequest> *_pFragmentRequests;
            NOMADSUtil::PtrLList<FragmentRequest> *_pCompleteMessageRequests;
            NOMADSUtil::String _queryTargetNodeId;
    };

    //=================================================================
    // DisServiceIncrementalDataReqMsg CONTROL
    //=================================================================

    class DisServiceIncrementalDataReqMsg : public DisServiceDataReqMsg
    {
        public:
            DisServiceIncrementalDataReqMsg (const char *pszSenderNodeId,
                                             const char* pszQueryTargetNodeId,
                                             uint16 ui16NumberOfNeighbors,
                                             MessageRequestScheduler *pReqScheduler,
                                             uint32 ui32MaxSize);
            virtual ~DisServiceIncrementalDataReqMsg (void);

            virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        private:
            /**
             * Return 0 if the request was successfully written, a negative
             * number otherwise
             */
            int add (NOMADSUtil::InstrumentedWriter *pIW, FragmentRequest *pFragReq);

            uint32 _ui32MaxSize;
            NOMADSUtil::BufferWriter _bw;
            MessageRequestScheduler *_pReqScheduler;
    };

    //=================================================================
    // DisServiceWorldStateSeqIdMsg CONTROL
    //=================================================================

    class DisServiceWorldStateSeqIdMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceWorldStateSeqIdMsg (void);
            DisServiceWorldStateSeqIdMsg (const char *pszSenderNodeId,
                                          uint32 ui32TopologyStateUpdateSeqId,
                                          uint16 ui16SubscriptionStateCRC,
                                          uint32 ui32DataCacheStateUpdateSeqId,
                                          uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType);
            DisServiceWorldStateSeqIdMsg (const char *pszSenderNodeId,
                                          uint32 ui32TopologyStateUpdateSeqId,
                                          uint16 ui16SubscriptionStateCRC,
                                          uint32 ui32DataCacheStateUpdateSeqId,
                                          uint8 ui8RepCtrlType, uint8 ui8FwdCtrlType, uint8 ui8NodeImportance);
            virtual ~DisServiceWorldStateSeqIdMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int writeNodeParameters (NOMADSUtil::Writer *pWriter);
            int readNodeParameters (NOMADSUtil::Reader *pReader);

            uint32 getTopologyStateUpdateSeqId (void);
            uint16 getSubscriptionStateCRC (void);
            uint32 getDataCacheStateUpdateSeqId (void);
            uint8 getRepCtrlType (void);
            uint8 getFwdCtrlType (void);
            uint8 getNumberOfActiveNeighbors (void);
            uint8 getMemorySpace (void);
            uint8 getBandwidth (void);
            uint8 getNodesInConnectivityHistory (void);
            uint8 getNodesRepetitivity (void);
            uint8 getNodeImportance (void);
            void setNumberOfActiveNeighbors (uint16 ui16NumberOfActiveNeighbors);
            void setMemorySpace (uint8 ui8MemorySpace);
            void setBandwidth (uint8 ui8Bandwidth);
            void setNodesInConnectivityHistory (uint8 ui8NodesInConnectivityHistory);
            void setNodesRepetitivity (uint8 ui8NodesRepetitivity);

        private:
            uint32 _ui32TopologyStateUpdateSeqId;
            uint16 _ui16SubscriptionStateCRC;
            uint16 _ui16NumberOfActiveNeighbors;
            uint32 _ui32DataCacheStateUpdateSeqId;
            uint8 _ui8repCtrlType;
            uint8 _ui8fwdCtrlType;
            uint8 _ui8MemorySpace;
            uint8 _ui8Bandwidth;
            uint8 _ui8NodesInConnectivityHistory;
            uint8 _ui8NodesRepetitivity;
            uint8 _ui8NodeImportance;
    };

    //==========================================================================
    // DisServiceSubscriptionStateMsg CONTROL
    //==========================================================================

    class DisServiceSubscriptionStateMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceSubscriptionStateMsg (void);

            /**
             * pSubscriptionsTable and pNodesTable are not deleted by
             * DisServiceSubscriptionStateMsg
             */
            DisServiceSubscriptionStateMsg (const char* pszSenderNodeId,
                                            NOMADSUtil::StringHashtable<NOMADSUtil::DArray2<NOMADSUtil::String> > *pSubscriptionsTable,
                                            NOMADSUtil::StringHashtable<uint32> *pNodesTable);
            virtual ~DisServiceSubscriptionStateMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            NOMADSUtil::StringHashtable<NOMADSUtil::DArray2<NOMADSUtil::String> > * getSubscriptionsTable (void);
            NOMADSUtil::StringHashtable<uint32> * getNodesTable (void);

        private:
            NOMADSUtil::StringHashtable<NOMADSUtil::DArray2<NOMADSUtil::String> > *_pSubscriptionsTable;
            NOMADSUtil::StringHashtable<uint32> *_pNodesTable;
    };

    //==========================================================================
    // DisServiceSubscriptionStateReqMsg CONTROL
    //==========================================================================

    class DisServiceSubscriptionStateReqMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceSubscriptionStateReqMsg (void);
            DisServiceSubscriptionStateReqMsg (const char *pszSenderNodeId, const char *pszTargetNodeId = NULL);
            virtual ~DisServiceSubscriptionStateReqMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            void setSubscriptionStateTable (NOMADSUtil::StringHashtable<uint32> *pSubscriptionStateTable);
            NOMADSUtil::StringHashtable<uint32> * getSubscriptionStateTable (void);

        private:
            NOMADSUtil::StringHashtable<uint32> *_pSubscriptionStateTable;
    };

    //=================================================================
    // DisServiceDataCacheQueryMsg CONTROL
    //=================================================================

    class DisServiceDataCacheQueryMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceDataCacheQueryMsg (void);
            DisServiceDataCacheQueryMsg (const char *pszSenderNodeId, const char *pszQueryTargetNodeId);
            virtual ~DisServiceDataCacheQueryMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            /**
             * Add a query to be executed on the cache of the receiving node(s).
             *
             * NOTE: DisServiceDataCacheQueryMsg serializes the passed object/
             * parameters and makes a _copy_ of them
             *
             * TODO: remove addQuery (const char *pszGroupName, ...) it obsolete!
             * (but before doing that make sure DisServiceDataCacheQuery is
             * actually working properly!)
             */
            int addQuery (DisServiceDataCacheQuery *pQuery);
            int addQuery (const char *pszGroupName, const char *pszSenderNodeId, uint8 ui8ClientType, uint16 ui16Tag, uint32 ui32MessageSequenceIdFrom, uint32 ui32MessageSequenceIdTo);

            uint16 getQueryCount (void);
            const char * getQuery (uint16 ui16Index);

        private:
            NOMADSUtil::DArray2<NOMADSUtil::String> _queries;
    };

    //=================================================================
    // DisServiceDataCacheQueryReplyMsg CONTROL
    //=================================================================

    class DisServiceDataCacheQueryReplyMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceDataCacheQueryReplyMsg (void);
            // Constructor called from the DisServiceHistoryRequestReplyMsg to set the proper type
            DisServiceDataCacheQueryReplyMsg (const char *pszSenderId, Type type);
            // NOTE: pMessageIDs is not deleted by the destructor when passed in using the following constructor
            DisServiceDataCacheQueryReplyMsg (const char *pszSenderNodeId, NOMADSUtil::DArray2<NOMADSUtil::String> *pMessageIDs);

            virtual ~DisServiceDataCacheQueryReplyMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            uint16 getIDsCount (void);

            // NOTE: If this object has been read from a reader, the class creates a new
            // instance of the following object in read, which is automatically
            // deleted by the destructor
            NOMADSUtil::DArray2<NOMADSUtil::String> * getIDs (void);

            const char * getID (uint16 ui16Index);

        protected:
            DisServiceDataCacheQueryReplyMsg (Type type);

        protected:
            NOMADSUtil::DArray2<NOMADSUtil::String> *_pMessageIDs;
            bool _bDeleteMessageIDs;
    };

    //=================================================================
    // DisServiceDataCacheMessagesRequestMsg CONTROL
    //=================================================================

    class DisServiceDataCacheMessagesRequestMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceDataCacheMessagesRequestMsg (void);

            // NOTE: pMessageIDs is not deleted by the destructor when passed in using the following constructor
            DisServiceDataCacheMessagesRequestMsg (const char *pszSenderNodeId, const char *pszQueryTargetNodeId,
                                                   NOMADSUtil::DArray2<NOMADSUtil::String> *pMessageIDs);

            virtual ~DisServiceDataCacheMessagesRequestMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            uint16 getIDsCount();

            // NOTE: If this object has been read from a reader, the class creates a new
            // instance of the following object in read, which is automatically
            // deleted by the destructor
            const char * getID (uint16 ui16Index);

        private:
            NOMADSUtil::DArray2<NOMADSUtil::String> *_pMessageIDs;
            bool _bDeleteMessageIDs;
            NOMADSUtil::String _targetNodeId;
    };

    //=================================================================
    // DisServiceAcknowledgmentMessage CONTROL
    //================================================================

    class DisServiceAcknowledgmentMessage : public DisServiceCtrlMsg
    {
        public:
            DisServiceAcknowledgmentMessage (void);
            DisServiceAcknowledgmentMessage (const char *pszSenderNodeId, const char *pszMsgToAckId);
            virtual ~DisServiceAcknowledgmentMessage (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            const char * getAckedMsgId (void);

        private:
            NOMADSUtil::String _msgToAckId;
    };

    //=================================================================
    // DisServiceCompleteMessageReqMsg CONTROL
    //=================================================================

    class DisServiceCompleteMessageReqMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceCompleteMessageReqMsg (void);
            DisServiceCompleteMessageReqMsg (const char *pszSenderNodeId,
                                             const char * MsgId);
            virtual ~DisServiceCompleteMessageReqMsg (void);

            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);

            const char * getMsgId (void);

        private:
            NOMADSUtil::String _pMsgId;
    };

    //=================================================================
    // DisServiceCacheEmptyMsg CONTROL
    //=================================================================

    class DisServiceCacheEmptyMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceCacheEmptyMsg (void);
            DisServiceCacheEmptyMsg (const char *pszSenderNodeId);
            virtual ~DisServiceCacheEmptyMsg (void);

            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
    };

    //=================================================================
    // DisServiceHistoryRequest CONTROL
    //=================================================================

    class DisServiceHistoryRequest : public DisServiceCtrlMsg
    {
       public:
            struct HReq;
            struct SenderState;

            DisServiceHistoryRequest (void);
            DisServiceHistoryRequest (const char *pszSenderNodeID, const char *pszTargetNodeID, HReq *pReq);
            virtual ~DisServiceHistoryRequest (void);

            int display (FILE *pFileOut);
            HReq * getHistoryRequest (void);
            int setSenderState (SenderState **ppSenderState, uint8 ui8SenderStateLength);
            SenderState ** getSenderState (void);
            uint8 getSenderStateLength (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            enum HistoryType {
                HT_WIN = 0x01,
                HT_RANGE = 0x02,
                HT_TMP_RANGE = 0x03
            };

            struct HReq {
                HReq (uint8 ui8Type, const char *pszGroupName, uint16 ui16Tag, int64 i64RequestTimeout);
                HReq (uint8 ui8Type);
                virtual ~HReq (void);

                virtual int display (FILE *pFileOut);
                virtual int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
                virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

                NOMADSUtil::String _groupName;
                uint16 _ui16Tag;
                uint8 _ui8Type;
                int64 _i64RequestTimeout;
            };

            struct SenderState {
                SenderState (void);
                SenderState (const char *pszSenderId, uint32 ui32State);
                virtual ~SenderState (void);
                NOMADSUtil::String _senderId;

                uint32 _ui32State;
            };

            struct HWinReq : public HReq {
                public:
                    HWinReq (const char *pszGroupName, uint16 ui16Tag, int64 i64RequestTimeout, uint16 ui16HistoryLength);
                    HWinReq (void);
                    virtual ~HWinReq (void);

                    int display (FILE *pFileOut);
                    int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
                    int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

                    uint16 _ui16HistoryLength;
            };

            struct HRangeReq : public HReq {
                public:
                    HRangeReq (const char *pszGroupName, const char *pszSenderNodeId, uint16 ui16Tag, int64 i64RequestTimeout,
                               uint32 ui32Begin, uint32 ui32End, NOMADSUtil::UInt32RangeDLList *pRanges);
                    HRangeReq (void);
                    virtual ~HRangeReq (void);

                    int display (FILE *pFileOut);
                    int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
                    int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

                    NOMADSUtil::String _senderNodeId;
                    uint32 _ui32Begin;
                    uint32 _ui32End;
                    NOMADSUtil::UInt32RangeDLList *_pRanges;
            };

            struct HTmpRangeReq : public HReq {
                public:
                    HTmpRangeReq (const char *pszGroupName, uint16 ui16Tag, int64 i64RequestTimeout, int64 i64PublishTimeStart, int64 i64PublishTimeEnd);
                    HTmpRangeReq (void);
                    virtual ~HTmpRangeReq (void);

                    int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
                    int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

                    int64 _i64PublishTimeStart;
                    int64 _i64PublishTimeEnd;
            };

        private:
            HReq *_pReq;
            SenderState **_pSenderState;
            uint8 _ui8SenderStateLength;
    };

    //=================================================================
    // DisServiceHistoryRequestReplyMsg
    //=================================================================

    class DisServiceHistoryRequestReplyMsg : public DisServiceDataCacheQueryReplyMsg
    {
        public:
            DisServiceHistoryRequestReplyMsg (void);
            DisServiceHistoryRequestReplyMsg (const char *pszSenderNodeId,
                                              NOMADSUtil::DArray2<NOMADSUtil::String> *pMessageIDs);

            virtual ~DisServiceHistoryRequestReplyMsg (void);
    };

    class MessageHeader;

    //==========================================================================
    // ChunkRetrievalMsgQuery CONTROL
    //==========================================================================

    /**
     * Implements the message Query.
     * Basic mechanism for searching the distributed network.
     */
    class ChunkRetrievalMsgQuery: public DisServiceCtrlMsg
    {
        public:
            ChunkRetrievalMsgQuery (void);
            ChunkRetrievalMsgQuery (const char *pszSenderNodeId, const char *pszQueryId,
                                    NOMADSUtil::PtrLList<MessageHeader> *pLocallyCachedChunks);
            virtual ~ChunkRetrievalMsgQuery (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            NOMADSUtil::DArray<uint8> * getLocallyCachedChunks (void);
            const char * getQueryId (void) const;

        private:           
            NOMADSUtil::String _queryId;
            NOMADSUtil::DArray<uint8> _locallyCachedChunks;
    };

    //==========================================================================
    // ChunkRetrievalMsgQueryHits CONTROL
    //==========================================================================

    /**
     * Implements the message QueryHits.
     * The Message responds to the Query for results.
     */
    class ChunkRetrievalMsgQueryHits: public DisServiceCtrlMsg
    {
        public:
            ChunkRetrievalMsgQueryHits (bool bDeallocateChunks=true);   									
            ChunkRetrievalMsgQueryHits (const char *pszSenderNodeId, 
                                        const char *pszTargetNodeId,
                                        NOMADSUtil::PtrLList<MessageHeader> *pMH,
                                        const char *pszQueryId, bool bDeallocateChunks=true);                                                                     
            virtual ~ChunkRetrievalMsgQueryHits (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);  
            NOMADSUtil::PtrLList<MessageHeader> *getMessageHeaders (void);
            const char *getQueryId (void);

        private:
            const bool _bDeallocateChunks;
            NOMADSUtil::PtrLList<MessageHeader> *_pCMHs;
            NOMADSUtil::String _queryId;
    };
    
    //==========================================================================
    // DisServiceSubscribtionAdvertisement CONTROL
    //==========================================================================
    
    /*
     * This message advertises what subscriptions a Node has subscribed.
     * It includes the list of nodes the message traverse.
     */
    class DisServiceSubscribtionAdvertisement : public DisServiceCtrlMsg
    {
        public:
            /* Allocates the Subscription list and the Path data structures */
            DisServiceSubscribtionAdvertisement (void);
            
            /* It creates a new DSSA message with the given SubList */
            DisServiceSubscribtionAdvertisement (const char *pszSenderNodeId, const char *pszOriginatorNodeId,
                                                 NOMADSUtil::PtrLList<NOMADSUtil::String> *pSubList);
            
            /* It creates a new DSSA message with the given SubList and Path */
            DisServiceSubscribtionAdvertisement (const char *pszSenderNodeId, const char *pszOriginatorNodeId,
                                                 NOMADSUtil::PtrLList<NOMADSUtil::String> *pSubList,
                                                 NOMADSUtil::PtrLList<NOMADSUtil::String> *pPath);
            /* Deallocate the Subscription List and Path data structures */
            ~DisServiceSubscribtionAdvertisement (void);
            
            int addSubscription (const char *pszSubscription);
            int removeSubscription (const char *pszSubscription);
            int prependNode (const char *pszNodeId);            
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getSubscriptions (void);
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getPath (void);
            const char * getOriginatorNodeId (void);
             
            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            
            void display (void);
            
        private:
            NOMADSUtil::String _originatorNodeId;
            NOMADSUtil::PtrLList<NOMADSUtil::String> _subscriptionList;
            NOMADSUtil::PtrLList<NOMADSUtil::String> _path;
    };

    
    //==================================================================
    // ControllerToControllerMsg
    //==================================================================

    class ControllerToControllerMsg : public DisServiceMsg
    {
        public:
            ControllerToControllerMsg (void);
            ControllerToControllerMsg (const char *pszSenderNodeID, const char *pszReceiverNodeID,
                                       uint8 ui8CtrlType, uint8 ui8CtrlVersion, void *pMetaData,
                                       uint32 ui32MetaDataLength, void *pData, uint32 ui32DataLength);

            virtual ~ControllerToControllerMsg (void);

            enum CtlrToCtlrMsgType {
                DSCTCMT_RepStartReq = 0x01,
                DSCTCMT_RepStartReply = 0x02,
                DSCTCMT_RepEnd = 0x03,
                DSCTCMT_RepAck = 0x04
            };

            virtual int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);

            /**
             * Serialized the ControllerToControllerMsg
             * The first version does not check for the length of the message,
             * the second does.
             */
            virtual int write (NOMADSUtil::Writer *pWriter);
            virtual int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            const char * getReceiverNodeID (void);
            uint8 getControllerType (void);
            uint8 getControllerVersion (void);
            void * getMetaData (void);
            uint32 getMetaDataLength (void);
            void * getData (void);
            uint32 getDataLength (void);    

        private:
            int writeInternal (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, bool bCheckSize);
    
        protected:
            NOMADSUtil::String _receiverNodeID;
            uint8 _ui8CtrlType;
            uint8 _ui8CtrlVersion;
            void *_pMetaData;
            uint32 _ui32MetaDataLength;
            void *_pData;
            uint32 _ui32DataLength;
    };


    //==============================================================================
    // TargetBasedReplicationController Messages
    //==============================================================================
    class ReplicationStartReqMsg : public ControllerToControllerMsg
    {
        public:
            ReplicationStartReqMsg (ControllerToControllerMsg *pMsg);
            ReplicationStartReqMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                                    uint8 ui8CtrlType, uint8 ui8CtrlVersion, bool bSendCurrentDataList, bool bRequireAcks);

            bool sendCurrentDataList (void);
            bool requireAcks (void);

        private:
            bool _bSendCurrentDataList;
            bool _bRequireAcks;
    };

    class ReplicationStartReplyMsg : public ControllerToControllerMsg
    {
        public:
            ReplicationStartReplyMsg (ControllerToControllerMsg *pMsg);

            // Note: The destructor will delete the ReceivedMessagesList
            ReplicationStartReplyMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                                      uint8 ui8CtrlType, uint8 ui8CtrlVersion,
                                      NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pReceivedMsgsList = NULL);

            ~ReplicationStartReplyMsg (void);

            NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> * relinquishReceivedMsgsList (void);

            int write (NOMADSUtil::Writer *pWriter);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        private:
            /**
             * Serializes a given PtrLList<ReceivedMsgsByGrp> into the BufferWriter pWriter.
             * Returns 0 if the operation was successful, a negative number otherwise.
             */
            static int writeMessageList (NOMADSUtil::BufferWriter *pWriter, NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pReceivedMsgsList);

            /**
             * Extracts a PtrLList<ReceivedMsgsByGrp> from a given BufferReader.
             * Returns 0 if the operation was successful, a negative number otherwise.
             */
            static int readMessageList (NOMADSUtil::BufferReader *pReader, NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pReceivedMsgsList);

            NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *_pReceivedMsgsList;
    };

    class ReplicationEndMsg : public ControllerToControllerMsg
    {
        public:
            ReplicationEndMsg (void);
            ReplicationEndMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                               uint8 ui8CtrlType, uint8 ui8CtrlVersion);
    };

    class ReplicationAckMsg : public ControllerToControllerMsg
    {
        public:
            ReplicationAckMsg (ControllerToControllerMsg *pCtrlMsg);
            ReplicationAckMsg (const char *pszSenderNodeId, const char *pszReceiverNodeId,
                               uint8 ui8CtrlType, uint8 ui8CtrlVersion, NOMADSUtil::PtrQueue<NOMADSUtil::String> *pMsgIDsToAck);

            NOMADSUtil::PtrQueue<NOMADSUtil::String> * getMsgIDs (void);

        private:
            NOMADSUtil::PtrQueue<NOMADSUtil::String> _msgIDs;
    };

    class SearchMsg : public DisServiceCtrlMsg
    {
        public:
            SearchMsg (void);
            SearchMsg (const char *pszSenderNodeId, const char *pszTargetNodeId);
            ~SearchMsg (void);

            const void * getQuery (unsigned int &uiQueryLen) const;
            const char * getQueryId (void) const;
            const char * getQuerier (void) const;
            const char * getGroupName (void) const;
            const char * getQueryType (void) const;
            const char * getQueryQualifier (void) const;

            int setQuery (const void *pQuery, unsigned int uiQueryLen);
            int setQuerier (const char *pszQuerier);
            int setQueryId (const char *pszQueryId);
            int setGroupName (const char *pszGroupName);
            int setQueryType (const char *pszQueryType);
            int setQueryQualifier (const char *pszQueryQualifiers);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        private:
            void *_pQuery;
            unsigned int _uiQueryLen;
            char *_pszQueryId;
            char *_pszQuerier;
            char *_pszGroupName;
            char *_pszQueryType;
            char *_pszQueryQualifiers;
    };

    class BaseSearchReplyMsg : public DisServiceCtrlMsg
    {
        public:
            virtual ~BaseSearchReplyMsg (void);

            const char * getQueryId (void) const;
            const char * getQuerier (void) const;
            const char * getQueryType (void) const;
            const char * getMatchingNode (void) const;

            int setQueryId (const char *pszQueryId);
            int setQueryType (const char *pszQueryType);
            int setQuerier (const char *pszQuerier);
            int setMatchingNode (const char *pszMatchingNode);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        protected:
            BaseSearchReplyMsg (Type type);
            BaseSearchReplyMsg (Type type, const char *pszSenderNodeId, const char *pszTargetNodeId);

        private:
            char *_pszQueryId;
            char *_pszQuerier;
            char *_pszQueryType;
            char *_pszMatchingNode;
    };

    class SearchReplyMsg : public BaseSearchReplyMsg
    {
        public:
            SearchReplyMsg (void);
            SearchReplyMsg (const char *pszSenderNodeId, const char *pszTargetNodeId);
            ~SearchReplyMsg (void);

            const char ** getMatchingMsgIds (void) const;

            int setMatchingMsgIds (const char **ppszMatchingIds);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        private:
            char **_ppszMatchingIds;
    };

    class VolatileSearchReplyMsg : public BaseSearchReplyMsg
    {
        public:
            VolatileSearchReplyMsg (void);
            VolatileSearchReplyMsg (const char *pszSenderNodeId, const char *pszTargetNodeId);
            ~VolatileSearchReplyMsg (void);

            const void * getReply (uint16 &ui16ReplyLen) const;

            int setReply (const void *pReply, uint16 ui16ReplyLen);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        private:
            uint16 _ui16ReplyLen;
            void *_pReply;
    };

    //==========================================================================
    // DisServiceImprovedSubscriptionStateMsg CONTROL
    //==========================================================================

    class DisServiceImprovedSubscriptionStateMsg : public DisServiceCtrlMsg
    {
        public:
            DisServiceImprovedSubscriptionStateMsg (void);

            /**
             * pSubscriptionsTable and pNodesTable are not deleted by
             * DisServiceSubscriptionStateMsg
             */
            DisServiceImprovedSubscriptionStateMsg (const char *pszSenderNodeId,
                                                    NOMADSUtil::StringHashtable<IHMC_ACI::SubscriptionList> *pSubscriptionsTable,
                                                    NOMADSUtil::StringHashtable<uint32> *pNodesTable);
            virtual ~DisServiceImprovedSubscriptionStateMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            NOMADSUtil::StringHashtable<IHMC_ACI::SubscriptionList> * getSubscriptionsTable (void);
            NOMADSUtil::StringHashtable<uint32> * getNodesTable (void);

        private:
            NOMADSUtil::StringHashtable<IHMC_ACI::SubscriptionList> *_pSubscriptionsTable;
            NOMADSUtil::StringHashtable<uint32> *_pNodesTable;
    };
    
    //================================================================
    // DisServiceProbabilitiesMsg CONTROL
    //================================================================

    class DisServiceProbabilitiesMsg : public DisServiceCtrlMsg 
    {
        public:
            DisServiceProbabilitiesMsg (void);
            DisServiceProbabilitiesMsg (const char *pszSenderNodeId, 
                                        NOMADSUtil::StringHashtable<NOMADSUtil::StringFloatHashtable> *pProbabilitiesTable);
            virtual ~DisServiceProbabilitiesMsg (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            
            NOMADSUtil::StringHashtable<NOMADSUtil::StringFloatHashtable> * getProbabilitiesTable(void);
            
        private:
            NOMADSUtil::StringHashtable<NOMADSUtil::StringFloatHashtable> *_pProbabilitiesTable;
    };
    
}

#endif   // #ifndef INCL_DIS_SERVICE_MESSAGE_H
