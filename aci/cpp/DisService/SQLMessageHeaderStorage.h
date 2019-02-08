/*
 * SQLMessageHeaderStorage.h
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
 * Implementation of StorageInterface to store the
 * MessageHeader part of a message in a database.
 */

#ifndef INCL_SQL_MESSAGE_HEADER_STORAGE_H
#define INCL_SQL_MESSAGE_HEADER_STORAGE_H

#include "StorageInterface.h"

#include "DArray2.h"
#include "FTypes.h"
#include "LoggingMutex.h"
#include "PtrLList.h"
#include "StringHashtable.h"

#include "Database.h"

namespace IHMC_MISC
{
    class Database;
    class PreparedStatement;
    class Row;
}

namespace IHMC_ACI
{
    class DisServiceDataCacheQuery;
    class SQLPropertyStore;

    class SQLMessageHeaderStorage : public StorageInterface
    {
        public:
            SQLMessageHeaderStorage (const char *pszDBName = NULL);
            virtual ~SQLMessageHeaderStorage (void);

            virtual int init (void);

            virtual PropertyStoreInterface * getPropertyStore (void);

            /**
             * Returns the number of complete chunks that have been receive
             * for the message identified by pszGroupName, pszPublisherNodeId,
             * and ui32MsgSeqId.
             */
            int countChunks (const char *pszGroupName, const char *pszPublisherNodeId,
                             uint32 ui32MsgSeqId, unsigned int &uiTotNChunks);

            /**
             * Returns the MessageHeader associated to the key
             */
            virtual MessageHeader * getMsgInfo (const char *pszKey);

            virtual NOMADSUtil::PtrLList<MessageHeader> * getMsgInfo (DisServiceDataCacheQuery *pQuery);
            virtual NOMADSUtil::PtrLList<MessageId> * getNotReplicatedMsgList (const char *pszTargetPeer, unsigned int uiLimit,
                                                                               NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pFilters);
            virtual uint32 getNextExpectedSeqId (const char *pszGroupName, const char *pszSenderNodeId, uint16 ui6Tag);

            virtual bool hasCompleteMessage (const char *pszKey);
            virtual bool hasCompleteMessage (const char *pszGroupName, const char *pszSenderNodeID,
                                             uint32 ui32MsgSeqId, uint8 ui8ChunkSeqId);
            virtual bool hasCompleteMessage (MessageHeader *pMH);

            virtual bool hasCompleteMessageOrAnyCompleteChunk (const char *pszGroupName, const char *pszSenderNodeID,
                                                               uint32 ui32MsgSeqId);

            virtual int insert (Message *pMsg);

            virtual NOMADSUtil::PtrLList<RetrievedSubscription> * retrieveSubscriptionGroups (const char *pszSenderNodeId);

            virtual NOMADSUtil::PtrLList<MessageHeader> * getMatchingFragments (const char *pszGroupName, const char *pszSender,
                                                                                uint32 ui32MsgSeqId);
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMatchingFragments (const char *pszGroupName, const char *pszSender,
                                                                                uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId);
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMatchingFragments (const char *pszGroupName, const char *pszSender,
                                                                                uint32 ui32MsgSeqId, uint32 ui32ChunkSeqId,
                                                                                uint32 ui32StartOffset, uint32 ui32EndOffset);
            NOMADSUtil::PtrLList<MessageHeader> * getCompleteChunkMessageInfos (const char *pszGroupName, const char *pszSender,
                                                                                uint32 ui32MsgSeqId);
            NOMADSUtil::PtrLList<MessageHeader> * getAnnotationsOnMsgMessageInfos (const char *pszAnnotatedObjMsgId);
            /**
             * It returns a copy of the object.  It musy be deallocated by the caller.
             */
            void * getAnnotationMetadata (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, uint32 &ui32Len);

            NOMADSUtil::PtrLList<MessageId> * getMessageIdsForFragments (void);

            char ** getDisseminationServiceIds (const char *pszObjectId, const char *pszInstanceId);

            /**
             * This methods offers to execute a sql statement and returns a PtrLList
             * of the messages matching the statement
             * NOTE: int parameters set on 0 or NULL will not be considered in the query.
             * NOTE: iGrouping set on 0 means that there is not any GROUP BY clause,
             *       if iGrouping >= 1 then a from 1 to 15 flags are accepted.
             * TODO: the THIRD is NOT IMPLEMENTED yet!!!
             */
            virtual NOMADSUtil::PtrLList<MessageHeader> * execSelectMsgInfo (const char *pszGroupName, const char *pszSenderNodeId,
                                                                             uint8 ui8ClientType, uint16 ui6Tag, uint32 ui32From,
                                                                             uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                                             uint16 ui16Limit=0, const char * pszOrderBy=NULL,
                                                                             bool bDescOrder=false,
                                                                             NOMADSUtil::DArray2<NOMADSUtil::String> *pDAArgs=NULL);

            virtual NOMADSUtil::PtrLList<MessageHeader> * execSelectMsgInfo (const char *pszSQLStmt, uint16 ui16Limit);

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * execSelectID (const char *pszGroupName, const char *pszSenderNodeId,
                                                                            uint8 ui8ClientType, uint16 ui6Tag, uint32 ui32From,
                                                                            uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                                            uint16 ui16Limit=0, const char * pszOrderBy=NULL,
                                                                            bool bDescOrder=false,
                                                                            NOMADSUtil::DArray2<NOMADSUtil::String> *pDAArgs=NULL);

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * execSender (const char *pszGroupName, const char *pszSenderNodeId,
                                                                          uint8 ui8ClientType, uint16 ui6Tag, uint32 ui32SeqFrom,
                                                                          uint32 ui32To, uint32 ui32OffsetStart, uint32 ui32OffsetEnd,
                                                                          uint16 ui16Limit=0, const char * pszOrderBy=NULL,
                                                                          bool bDescOrder=false,
                                                                          NOMADSUtil::DArray2<NOMADSUtil::String> *pDAArgs=NULL);

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getSenders (const char *pszGroupName, uint16 ui16Limit);

            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * execSelectID (const char *pszSQLStatement, uint16 ui16Limit=0);

            void clear (void);

        public:
            static const NOMADSUtil::String TABLE_NAME;
            static const NOMADSUtil::String CHUNK_TABLE_NAME;
            static const NOMADSUtil::String INDEX_GROUP;
            static const NOMADSUtil::String INDEX_TAG;

            //==================================================================
            //  COLUMN NAMES AND COLUM INDEXES
            //==================================================================

            static const NOMADSUtil::String FIELD_ROW_ID;

            static const NOMADSUtil::String FIELD_GROUP_NAME;
            static const uint8 FIELD_GROUP_NAME_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_SENDER_ID;
            static const uint8 FIELD_SENDER_ID_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_MSG_SEQ_ID;
            static const uint8 FIELD_MSG_SEQ_ID_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_CHUNK_ID;
            static const uint8 FIELD_CHUNK_ID_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_OBJECT_ID;
            static const uint8 FIELD_OBJECT_ID_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_INSTANCE_ID;
            static const uint8 FIELD_INSTANCE_ID_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_ANNOTATION_MSG_ID;
            static const uint8 FIELD_ANNOTATION_MSG_ID_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_ANNOTATION_METADATA;
            static const uint8 FIELD_ANNOTATION_METADATA_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_TAG;
            static const uint8 FIELD_TAG_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_CLIENT_ID;
            static const uint8 FIELD_CLIENT_ID_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_CLIENT_TYPE;
            static const uint8 FIELD_CLIENT_TYPE_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_MIME_TYPE;
            static const uint8 FIELD_MIME_TYPE_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_CHECKSUM;
            static const uint8 FIELD_CHECKSUM_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_TOT_MSG_LENGTH;
            static const uint8 FIELD_TOT_MSG_LENGTH_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_FRAGMENT_LENGTH;
            static const uint8 FIELD_FRAGMENT_LENGTH_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_FRAGMENT_OFFSET;
            static const uint8 FIELD_FRAGMENT_OFFSET_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_METADATA_LENGTH;
            static const uint8 FIELD_METADATA_LENGTH_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_TOT_N_CHUNKS;
            static const uint8 FIELD_TOT_N_CHUNKS_NUMBER;

            static const NOMADSUtil::String FIELD_HISTORY_WINDOW;
            static const uint8 FIELD_HISTORY_WINDOW_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_PRIORITY;
            static const uint8 FIELD_PRIORITY_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_EXPIRATION;
            static const uint8 FIELD_EXPIRATION_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_ACKNOLEDGMENT;
            static const uint8 FIELD_ACKNOLEDGMENT_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_ARRIVAL_TIMESTAMP;
            static const uint8 FIELD_ARRIVAL_TIMESTAMP_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_METADATA;
            static const uint8 FIELD_METADATA_COLUMN_NUMBER;

            static const NOMADSUtil::String FIELD_DATA;
            static const uint8 FIELD_DATA_COLUMN_NUMBER;

            static const NOMADSUtil::String METAINFO_FIELDS;
            static const NOMADSUtil::String ALL;
            static const NOMADSUtil::String ALL_PERSISTENT;

            static const NOMADSUtil::String PRIMARY_KEY;

            static const uint8 MSG_ID_N_FIELDS;

            static const uint8 DEFAULT_MAX_UTILITY = 255;

        protected:
            friend class DataCache;
            friend class DataCacheInterface;
            friend class PersistentDataCache;
            friend class DataCacheReplicationController;
            friend class DataCacheInterfaceTest;

            /**
             * Returns the MessageHeader for the specified message or message fragment
             * If the fragment offset and fragment length are not specified, the search is for a complete
             * message, using the constraint fragment length == total message length.
             */
            MessageHeader * getMsgInfo (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId,
                                        uint8 ui8Chunk, uint32 ui32FragOffset = 0, uint32 ui32FragLength = 0);

            /**
             * NOTE: this method is not thread-safe.
             */
            NOMADSUtil::PtrLList<MessageHeader> * getMessageInfo (IHMC_MISC::PreparedStatement *pStmt, int &iElements, bool bMaintainOrder=false);
            MessageHeader * getMessageInfo (IHMC_MISC::Row *pRow);
            const char * getId (IHMC_MISC::Row *pRow);

            /**
             * Returns the list of MessageIds that are a result of the specified query.
             *
             * NOTE: MessagId does not include fragment offset and length information
             *       Therefore, the query must be specific enough to not contain multiple results for the same message id
             *
             * NOTE: Caller is responsible for deallocating the MessageIds as well as the PtrLList
             */
            NOMADSUtil::PtrLList<MessageId> * getMessageIds (IHMC_MISC::PreparedStatement *pStmt, int *piResultCount, bool bMantainOrder = false);

            /**
             * Returns the MessageId that corresponds to the specified row of the result set
             *
             * NOTE: Caller is responsible for deallocating the MessageId
             */
            MessageId * getMessageId (IHMC_MISC::Row *pRow);

            /**
             * Methods to eliminate data from cache.
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getExpiredEntries (void);

            virtual NOMADSUtil::String getCreateTableSQLStatement (void);
            virtual NOMADSUtil::String getInsertIntoTableSQLStatement (void);

            virtual int eliminateAllTheMessageFragments (const char *pszGroupName, const char *pszSenderNodeId,
                                                         uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                                         NOMADSUtil::DArray2<NOMADSUtil::String> *pDeleteMessageIDs);
            virtual int eliminate (const char *pszKey);

            /**
             * Methods to compute the Utility of the resources in cache.
             */
            NOMADSUtil::DArray2<NOMADSUtil::String> * getResourceOrderedByUtility (void);

            /**
             * GENERAL METHOD WHICH ALLOWS TO EXEC A QUERY AND RETURN THE ID IN
             * FORM OF STRING. If the key is multi-column the different values are
             * concatenated in compliance with the standard described in MessageInfo
             * (look at MessageInfo::getMsgId)
             *  for further detail about the way to concatenate them.
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * execSelectQuery (const char *pszWhat, const char *pszGroupName,
                                                                               const char *pszSenderNodeId, uint8 ui8ClientType,
                                                                               uint16 ui6Tag, uint32 ui32From, uint32 ui32To,
                                                                               uint32 ui32OffsetStart, uint32 offsetEnd,
                                                                               uint16 ui16Limit=0, const char *pszOrderBy=NULL,
                                                                               bool bDescOrder=false,
                                                                               NOMADSUtil::DArray2<NOMADSUtil::String> *pDAArgs=NULL);

            /**
             * By default ui16Limit is set to the highest uint16
             */
            NOMADSUtil::DArray2<NOMADSUtil::String> * execQueryAndReturnKey (const char *pszSQLStmt, uint16 ui16Limit=65535);

            /*
             * Insert the MessageInfo fields into the default data cache
             */
            virtual int insertIntoDataCacheBind (IHMC_MISC::PreparedStatement *pStmt, Message *pMsg);

            /*
             * Returns true if it has a complete message in the default data
             * cache. False otherwise
             */
            virtual bool hasCompleteDataMessage (const char *pszGroupName, const char *pszSenderNodeId,
                                                 uint32 ui32MsgSeqId, uint8 ui8ChunkSeqId,
                                                 bool bUseConstraintOnChunkId = true);
        protected:
            NOMADSUtil::LoggingMutex _m;

            SQLPropertyStore *_pPropStore;

            IHMC_MISC::DatabasePtr *_pDB;     // DB connection
            IHMC_MISC::PreparedStatement *_pCountChunksPrepStmt;
            IHMC_MISC::PreparedStatement *_pInsertByteCode;
            IHMC_MISC::PreparedStatement *_pHasCompleteMsgPrepStmt;
            IHMC_MISC::PreparedStatement *_pHasCompleteMsgOrAnyChunkPrepStmt;
            IHMC_MISC::PreparedStatement *_pGetMsgInfoPrepStmt;
            IHMC_MISC::PreparedStatement *_pGetCompleteMsgInfoPrepStmt;
            IHMC_MISC::PreparedStatement *_pGetNotSentMsgInfoPrepStmt;
            IHMC_MISC::PreparedStatement *_pGetMatchingFragmentMsgInfoPrepStmt_1;
            IHMC_MISC::PreparedStatement *_pGetMatchingFragmentMsgInfoPrepStmt_2;
            IHMC_MISC::PreparedStatement *_pGetMatchingFragmentMsgInfoPrepStmt_3;
            IHMC_MISC::PreparedStatement *_pGetComplChunkMsgHeadersPrepStmt;
            IHMC_MISC::PreparedStatement *_pGetAnnotationMetadataPrepStmt;
            IHMC_MISC::PreparedStatement *_pGetComplAnnotationsPrepStmt;
            IHMC_MISC::PreparedStatement *_pEliminateFragments;
            IHMC_MISC::PreparedStatement *_pEliminateFragmentIDs;
            IHMC_MISC::PreparedStatement *_pFindAllMessageIDsWithFragments;
            IHMC_MISC::PreparedStatement *_pDeleteRow;
            IHMC_MISC::PreparedStatement *_pGetDSIdsByObjAndInstId;
            IHMC_MISC::PreparedStatement *_pGetDSIdsByObjId;
    };

}

#endif   // #ifndef INCL_SQL_MESSAGE_HEADER_STORAGE_H
