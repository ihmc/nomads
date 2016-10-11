/*
 * RequestsState.h
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on September 7, 2011, 3:55 PM
 */

#ifndef INCL_REQUESTS_STATE_H
#define	INCL_REQUESTS_STATE_H

#include "LoggingMutex.h"
#include "PtrLList.h"
#include "RangeDLList.h"
#include "StringHashtable.h"
#include "UInt32Hashtable.h"
#include "MessageInfo.h"

namespace IHMC_ACI
{
    class ChunkMsgInfo;
    class Message;
    class MessageHeader;
    class MessageInfo;
    class MessageReassembler;
    class MessageRequestScheduler;

    struct RequestInfo
    {
        RequestInfo (uint16 ui16ClientId);
        ~RequestInfo (void);

        const uint16 _ui16ClientId;
        const char *_pszGroupName;
        const char *_pszSenderNodeId;
        int64 _ui64ExpirationTime;
        const char *_pszQueryId;
    };

    struct RequestDetails
    {
        RequestDetails (const char *pszQueryId, uint16 ui16ClientId, int64 i64ExpirationTime);
        ~RequestDetails (void);

        void addDetails (const char *pszQueryId, uint16 ui16ClientId, int64 i64ExpirationTime);
        bool isReplyToSearch (void);

        struct QueryDetails
        {
            QueryDetails (const char *pszQueryId, uint16 ui16ClientId, int64 i64ExpirationTime);
            ~QueryDetails (void);

            int operator == (const QueryDetails &rhsDetails) const;

            const uint16 _ui16ClientId;
            int64 _i64ExpirationTime;
            const NOMADSUtil::String _queryId;
        };

        int64 _i64ExpirationTime;
        NOMADSUtil::PtrLList<QueryDetails> _details;
    };

    //--------------------------------------------------------------------------
    // RequestsInterface
    //--------------------------------------------------------------------------

    class RequestsInterface
    {
        public:
            virtual ~RequestsInterface (void);
        
        protected:
            RequestsInterface (void);
            virtual void getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                                    MessageReassembler *pMessageReassembler) = 0;
    };

    //--------------------------------------------------------------------------
    // AbstractRequestsState
    //--------------------------------------------------------------------------

    class AbstractRequestsState : public RequestsInterface
    {
        public:
            ~AbstractRequestsState (void);

        protected:
            AbstractRequestsState (void);

            static const bool RESET_GET = true;

            enum StateType {
                MSG, CHUNKED_MSG
            };

            struct AbstractState
            {
                virtual ~AbstractState (void);

                StateType getType (void);

                protected:
                    AbstractState (StateType type);

                private:
                    StateType _type;
            };

            struct ByGroup
            {
                ByGroup (void);
                virtual ~ByGroup (void);

                NOMADSUtil::StringHashtable<AbstractState> _statesBySender;
            };

            NOMADSUtil::StringHashtable<ByGroup> _states;
    };
   
    //--------------------------------------------------------------------------
    // MessageRequestState
    //--------------------------------------------------------------------------

    class MessageRequestState : public AbstractRequestsState
    {
        public:
            MessageRequestState (void);
            virtual ~MessageRequestState (void);

            int addRequest (RequestInfo &reqInfo, NOMADSUtil::UInt32RangeDLList *pMsgSeqIDs,
                            MessageReassembler *pMsgReassembler);

            RequestDetails * messageArrived (MessageInfo *pMI);

            void getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                            MessageReassembler *pMessageReassembler);

            bool wasRequested (const char *pszGroupName, const char *pszPublisherNodeI,
                               uint32 ui32MsgSeqId);

        protected:
            int addRequestInternal (RequestInfo &reqInfo, uint32 ui32MsgSeqIdStart,
                                    uint32 ui32MsgSeqIdEnd,
                                    MessageReassembler *pMsgReassembler);

            struct State : public AbstractState
            {
                State (void);
                virtual ~State (void);

                void fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                            MessageReassembler *pMessageReassembler,
                                            const char *pszGroupName, const char *pszSenderNodeId);

                NOMADSUtil::UInt32Hashtable<RequestDetails> _expTimes;
                NOMADSUtil::UInt32RangeDLList _msgSeqIDs;
            };

    };

    //--------------------------------------------------------------------------
    // ChunkRequestsState
    //--------------------------------------------------------------------------

    class ChunkRequestsState : public AbstractRequestsState
    {
        public:
            ChunkRequestsState (void);
            virtual ~ChunkRequestsState (void);

            int addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                            NOMADSUtil::UInt8RangeDLList *pChunkIDs,
                            MessageReassembler *pMsgReassembler);

            RequestDetails * messageArrived (ChunkMsgInfo *pCMI);

            void getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                            MessageReassembler *pMessageReassembler);

            bool wasRequested (const char *pszGroupName, const char *pszPublisherNodeId,
                               uint32 ui32MsgSeqId, uint8 ui8ChunkId);

        protected:
            int addRequestInternal (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                                    uint8 ui8ChunkIdStart, uint8 ui8ChunkIdEnd,
                                    MessageReassembler *pMsgReassembler);

            struct ChunkState
            {
                ChunkState (void);
                ~ChunkState (void);

                void fillUpMissingMessages (MessageRequestScheduler *pReqScheduler,
                                            MessageReassembler *pMessageReassembler,
                                            const char *pszGroupName, const char *pszSenderNodeId,
                                            uint32 ui32SeqId);

                NOMADSUtil::UInt32Hashtable<RequestDetails> _expTimes;
                NOMADSUtil::UInt8RangeDLList _chunkIDs;
            };

            struct ChunkedMessageState : public AbstractState
            {
                ChunkedMessageState (void);
                virtual ~ChunkedMessageState (void);

                NOMADSUtil::UInt32Hashtable<ChunkState> _msgSeqIDs;
            };
    };

    //--------------------------------------------------------------------------
    // RequestsState
    //--------------------------------------------------------------------------

    class RequestsState : public RequestsInterface
    {
        public:
            RequestsState (void);
            ~RequestsState (void);

            int addRequest (RequestInfo &reqInfo, NOMADSUtil::UInt32RangeDLList *pMsgSeqIDs,
                            MessageReassembler *pMsgReassembler);

            int addRequest (RequestInfo &reqInfo, uint32 ui32MsgSeqId,
                            NOMADSUtil::UInt8RangeDLList *pChunkIDs,
                            MessageReassembler *pMsgReassembler);

            /**
             * Returns the query ID with which the message was requested.
             */
            RequestDetails * messageArrived (Message *pMsg);

            void getMissingMessageRequests (MessageRequestScheduler *pReqScheduler,
                                            MessageReassembler *pMessageReassembler);

            bool wasRequested (const char *pszGroupName, const char *pszPublisherNodeId,
                               uint32 ui32MsgSeqId, uint8 ui8ChunkId = MessageHeader::UNDEFINED_CHUNK_ID);

       private:
           MessageRequestState _msgReqState;
           ChunkRequestsState _chunkReqState;
    };
}

#endif	// INCL_REQUESTS_STATE_H

