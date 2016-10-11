/*
 * StorageInterface.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 13, 2009, 2:13 PM
 */

#ifndef INCL_STORAGE_INTERFACE_H
#define	INCL_STORAGE_INTERFACE_H

#include "DisServiceDataCacheQuery.h"

#include "Message.h"

#include "DArray2.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class String;
}

namespace IHMC_ACI
{
    class MessageHeader;
    class MessageId;
    class PropertyStoreInterface;

    class StorageInterface
    {
        public:
            explicit StorageInterface (const char *pszStorageFile);
            virtual ~StorageInterface (void);

            virtual int init (void) = 0;

            /**
             * Returns the property store
             */
            virtual PropertyStoreInterface * getPropertyStore (void) = 0;

            /**
             * Returns the number of chunks that have been completely received
             * for the message identified by pszGroupName, pszSenderNodeID and
             * ui32MsgSeqId.
             * uiTotNChunks is set by the method to the total number of chunks
             * in which the message was chunked.
             *
             * NOTE: if the method returns an error, or 0, then uiTotNChunks
             *       is not properly initialized.
             */
            virtual int countChunks (const char *pszGroupName, const char *pszPublisherNodeId,
                                     uint32 ui32MsgSeqId, unsigned int &uiTotNChunks) = 0;

            virtual MessageHeader * getMsgInfo (const char *pszKey) = 0;
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMsgInfo (DisServiceDataCacheQuery *pQuery) = 0;

            virtual char ** getDisseminationServiceIds (const char *pszObjectId, const char *pszInstanceId) = 0;
            virtual void * getAnnotationMetadata (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, uint32 &ui32Len) = 0;

            /**
             * Returns the list of messages that have not been previously replicated to the specified target
             * The list is further filtered by an optional set of already received messages
             *
             * NOTE: Caller is responsible for deallocating each MessageId as well as the PtrLList
             */
            virtual NOMADSUtil::PtrLList<MessageId> * getNotReplicatedMsgList (const char *pszTargetPeer, unsigned int uiLimit,
                                                                               NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pFilters) = 0;
            /**
             * Returns the sequence ID of the latest message sent by pszPublisherNodeId
             * that belongs to a certain <pszGroupName, ui6Tag> subscription.
             * If no message is found 0 is returned.
             *
             * TODO: sequence IDs my wrap around!!!
             */
            virtual uint32 getNextExpectedSeqId (const char *pszGroupName, const char *pszPublisherNodeId, uint16 ui6Tag) = 0;

            virtual bool hasCompleteMessage (const char *pszKey) = 0;
            virtual bool hasCompleteMessage (const char *pszGroupName, const char *pszPublisherNodeId,
                                             uint32 ui32MsgSeqId, uint8 ui8ChunkId) = 0;
            virtual bool hasCompleteMessage (MessageHeader *pMH) = 0;

            virtual bool hasCompleteMessageOrAnyCompleteChunk (const char *pszGroupName,
                                                               const char *pszSenderNodeID,
                                                               uint32 ui32MsgSeqId) = 0;

            virtual int insert (Message *pMsg) = 0;

            struct RetrievedSubscription
            {
                bool operator == (RetrievedSubscription &rhsRetrievedSubscription);

                const char *pszGroupName;
                uint16 ui16Tag;
            };
            virtual NOMADSUtil::PtrLList<RetrievedSubscription> * retrieveSubscriptionGroups (const char *pszPublisherNodeId) = 0;

            /**
             * Returns all the messages (not necessarily  completed), that are identified by pszGroupName, pszPublisherNodeId, ui32MsgSeqId,
             * and ui32ChunkSeqId (if specified.).
             * If ui32StartOffset and ui32EndOffset are set, only the fragments or messages matching the specified interval will be
             * returned.
             */
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                                uint32 ui32MsgSeqId)=0;
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                                uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId)=0;
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                                uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId,
                                                                                uint32 ui32StartOffset, uint32 ui32EndOffset)=0;
            virtual NOMADSUtil::PtrLList<MessageHeader> * getCompleteChunkMessageInfos (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                                        uint32 ui32MsgSeqId) = 0;
            virtual NOMADSUtil::PtrLList<MessageHeader> * getAnnotationsOnMsgMessageInfos (const char *pszAnnotatedObjMsgId) = 0;

            // Returns the message ids of those messages whose fragments are present in storage
            virtual NOMADSUtil::PtrLList<MessageId> * getMessageIdsForFragments (void) = 0;

            //=== Obsolete methods, try not to use them!!! ================================================================================
            virtual NOMADSUtil::PtrLList<MessageHeader> * execSelectMsgInfo (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                             uint8 ui8ClientType, uint16 ui6Tag, uint32 ui32From,
                                                                             uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                                             uint16 ui16Limit=0, const char *pszOrderBy=NULL,
                                                                             bool bDescOrder=false,
                                                                             NOMADSUtil::DArray2<NOMADSUtil::String> *pDAArgs=NULL) = 0;

            virtual NOMADSUtil::PtrLList<MessageHeader> * execSelectMsgInfo (const char *pszSQLStmt, uint16 ui16Limit) = 0;

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * execSelectID (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                            uint8 ui8ClientType, uint16 ui6Tag, uint32 ui32From,
                                                                            uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                                            uint16 ui16Limit=0, const char *pszOrderBy=NULL,
                                                                            bool bDescOrder=false,
                                                                            NOMADSUtil::DArray2<NOMADSUtil::String> *pDAArgs=NULL) = 0;

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * execSender (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                          uint8 ui8ClientType, uint16 ui6Tag, uint32 ui32SeqFrom,
                                                                          uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                                          uint16 ui16Limit=0, const char *pszOrderBy=NULL,
                                                                          bool bDescOrder=false,
                                                                          NOMADSUtil::DArray2<NOMADSUtil::String> *pDAArgs=NULL) = 0;

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getSenders (const char *pszGroupName, uint16 ui16Limit) = 0;

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * execSelectID (const char *pszSQLStatement, uint16 ui16Limit=0) = 0;
            //=============================================================================================================================

        protected:
            friend class DataCache;
            friend class DataCacheInterface;
            friend class PersistentDataCache;
            friend class DataCacheReplicationController;

            /**
             * Methods to eliminate data form cache.
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getExpiredEntries (void) = 0;
            virtual int eliminateAllTheMessageFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                         uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                                         NOMADSUtil::DArray2<NOMADSUtil::String> *pDeleteMessageIDs) = 0;

            virtual int eliminate (const char *pszKey) = 0;

            /**
             * Methods to compute the Utility of the resources in cache.
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getResourceOrderedByUtility (void) = 0;

            const NOMADSUtil::String _dbName;
    };
}

#endif  // INCL_STORAGE_INTERFACE_H
