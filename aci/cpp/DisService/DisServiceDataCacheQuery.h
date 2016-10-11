/*
 * DisServiceDataCacheQuery.h
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
 * Author: Giacomo Benincasa (gbenincasa@ihmc.us)
 * Created on January 9, 2009, 11:55 AM
 */

#ifndef INCL_DISSERVICE_DATA_CACHE_QUERY_H
#define	INCL_DISSERVICE_DATA_CACHE_QUERY_H

#include "DArray.h"
#include "FTypes.h"
#include "StrClass.h"
#include "ReceivedMessages.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class ChunkRange;
}

namespace IHMC_ACI
{
    class DisServiceDataCacheQuery
    {
        public:
            DisServiceDataCacheQuery (void);

            /**
             * The number of tags, client IDs and client types that will be set
             * in the query.
             * (This is only for performance purpose, if the correct number is
             * set, the DArray containing tags, client IDs and client types,
             * will not be to reallocate memory)
             */
            virtual ~DisServiceDataCacheQuery (void);

            /**
             * Reset the query (there's no need ti instanciate a new object
             * DisServiceDataCacheQuery)
             */
            void reset (void);

            /**
             * Operations supported by the DisServiceDataCacheQuery class
             */
            enum OperationTypes {
                SELECT_OP = 0,
                DELETE_OP = 1,
            };

            /**
             * Select the operation to perform on the table
             */
            void selectOperation (uint8 ui8OperationType);

            /**
             * Select on which table the query should be performed. Two table
             * types are supported:
             * 1) ui8TableType 0 => DEFAULT_CACHE
             * 2) ui8TableType 1 => CHUNK_CACHE
             * NOTE : This MUST be the first istruction called from anyone wants
             * to use DisServiceDataCacheQuery objetc
             */
            void selectTable (uint8 ui8TableType);

            /**
             * Select the primary key fields of the selected table. This excludes
             * all the other option of selecting different fields
             */
            void selectPrimaryKey (void);

            /**
             * Select all fields of the selected table. This excludes all the
             * other option of selecting different fields
             */
            void selectAll (void);

            /**
             * Select only the Metadata fields of the DEFAULT_CACHE. It prints
             * error message on the checkAndLog file
             * if this methods is called on the CHUNK_CACHE. This excludes all
             * the other option of selecting different fields
             */
            void selectMetadataFields (void);

            /**
             * Select the rowId
             */
            void selectRowId (void);

            /**
             * Specify constraint on the group name.
             */
            void addConstraintOnGroupName (const char *pszGroupName);

            /**
             * Specify constraint on the sender node ID.
             */
            void addConstraintOnSenderNodeId (const char *pszSenderNodeId);

            /**
             * Adds a constraint to get all the messages except the ones specified in the array passed as parameter
             */
            void addConstraintOnMsgIdNotIn (const char **ppszMessageIdFilters);
            
            /**
             * Adds a constraint to get all the messages except the ones specified in the ReceivedMsgsByGrp passed as parameter
             */
            void addConstraintOnGrpPubSeqIdNotIn (NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pMessageIdFilters);

            /**
             * Adds a constraint to getthe message with the specified message ID
             */
            void addConstraintOnMsgId (const char *pszMessageId);

            /**
             * Specify constraint on the message sequence ID.
             */
            void addConstraintOnMsgSeqId (uint32 ui32SeqId);

            /**
             * Specify constraint on the message sequence ID's range.
             */
            void addConstraintOnMsgSeqId (uint32 ui32SeqIdFrom, uint32 ui32SeqIdTo);
            void addConstraintOnMsgSeqId (uint32 ui32SeqIdFrom, uint32 ui32SeqIdTo, bool bOpenInterval);

            /**
             * Specify constraint on the message sequence ID's range.
             * (Open range)
             */
            void addConstraintOnMsgSeqIdGreaterThan (uint32 ui32SeqId);
            void addConstraintOnMsgSeqIdSmallerThan (uint32 ui32SeqId);
            void addConstraintOnMsgSeqIdGreaterThanOrEqual (uint32 ui32SeqId);
            void addConstraintOnMsgSeqIdSmallerThanOrEqual (uint32 ui32SeqId);

            /**
             * Select the chunkId. This methods should be used only when the
             * query is addressed to the CHUNK_TABLE
             */
            void addConstraintOnChunkId (uint8 ui8ChunkId);
            void addConstraintOnChunkIdSmallerThanOrEqual (uint8 ui8ChunkId);

            /**
             * Specify constraint on the tag.
             */
            void addConstraintOnTag (uint16 ui16Tag);

            /**
             * Specify constraint on the tag.
             * Accepts a set of tags
             */
            void addConstraintOnTag (uint8 ui8NumOfTags, ...);

            /**
             * Specify constraint on the tag. This methods should be used only
             * when the query is addressed to the DEFAULT_TABLE
             */
            void addConstraintOnClientId (uint16 ui16ClientId);

            /**
             * Specify constraint on the tag. This methods should be used only
             * when the query is addressed to the DEFAULT_TABLE
             * Accepts a set of client IDs.
             */
            void addConstraintOnClientId (uint8 ui8NumOfClientIds, ...);

            /**
             * Specify constraint on the tag. This methods should be used only
             * when the query is addressed to the DEFAULT_TABLE
             */
            void addConstraintOnClientType (uint16 ui8ClientType);

            /**
             * Specify constraint on the tag. This methods should be used only
             * when the query is addressed to the DEFAULT_TABLE
             * Accepts a set of client Types.
             */
            void addConstraintOnClientType (uint8 ui8NumOfClientTypes, ...);

            /**
             * Specify constraint on the priority of the message. This methods
             * should be used only when the query is addressed to the
             * DEFAULT_TABLE
             */
            void addConstraintOnPriority (uint8 ui8Priority);

            /**
             * Specify constraint on the priority's range. This methods should be
             * used only when the query is addressed to the DEFAULT_TABLE
             */
            void addConstraintOnPriority (uint8 ui8PriorityFrom, uint8 ui8PriorityTo);

            /**
             * Specify constraint on a specific fragment of the message.
             */
            void addConstraintOnFragment (uint32 ui32FragOffset, uint32 ui32FragLength, bool bFragRangeExactMatch=true);
            
            /**
             * This method will select all the chunks except the ones specified
             * in the range
             */
            void excludeChunks (uint8 ui8ChunkIdStart, uint8 ui8ChunkIdEnd);

            /**
             * This method will select all the chunks except the ones listed in
             * the parameter
             */
            void excludeChunks (NOMADSUtil::DArray<uint8> *pChunkIDs);

            /**
             * Specify constraint on the expiration time. This methods should be
             * used only when the query is addressed to the DEFAULT_TABLE
             */
            void addConstraintOnExpirationTimeGreaterThan (int64 i64ExpirationLowerThreshold);
            void addConstraintOnExpirationTimeLowerThan (int64 i64ExpirationUpperThreshold);

            /**
             * Specify constraint on the expiration time. This methods should be
             * used only when the query is addressed to the DEFAULT_TABLE
             */
            void addConstraintOnArrivalTimeGreaterThan (int64 i64ExpirationLowerThreshold);
            void addConstraintOnArrivalTimeLowerThan (int64 i64ExpirationUpperThreshold);

            /**
             * Specify constraint on the utility. This methods should be used
             * only when the query is addressed to the DEFAULT_TABLE
             * NOTE: the field Utility is not in the cache yet, 
             * don't use the following 2 methods for now!!!
             */
            void addConstraintOnUtilityGreaterThan (int8 i8UtilityLowerThreshold);
            void addConstraintOnUtilityLowerThan (int8 i8UtilityUpperThreshold);

            /**
             * Only complete messages are selected.
             * NOTE : This selects the fields TotalMessageLength,
             * FragmentLength/ChunkFragmentLength and
             * FragmentOffset/ChunkFragmentOffset
             */
            void setNoFragments (bool bNoFrag);

            /**
             * Returns 
             *   0) if it was possible to set the requested order,
             *  -1) otherwise.
             * NOTE: The first is to be preferred to the second as it is more
             *       efficient.
             * NOTE: In order to order by a certain field, that fild must be in
             *       the ones specified in SELECT.
             *       This method does NOT check for it.
             * NOTE :
             */
            int setOrderBy(uint8 ui8Field, bool bDesc);
            int setOrderBy (const char * pszField, bool bDesc=false);

            //void setNoMetaData (bool bNoMeta);
            //void setOnlyMetadata (bool bOnlyMeta);

            /**
             * This methods should be used only when the
             * query is addressed to the DEFAULT_TABLE
             */
            void setIncludeData (bool bIncludeData=false);

            void setLimit (uint16 ui16ResultLimit);
            bool isLimitSet (void);
            uint16 getLimit (void);

            /**
             * Custom constraint adds a constraint (by AND).
             * Custom condition add a condition (by OR).
             */
            void addCustomConstraint (NOMADSUtil::String *pszCondition);
            void addCustomCondition (NOMADSUtil::String *pszCondition);

            void setPersistent (void);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            #if defined (USE_SQLITE)
                const char * getSqlQuery (void);
                const char * getComputedSqlQuery (void);
            #endif

        private:
            friend class DataCache;

            enum FirstFlagsWriteMask {
                WRITE_DELETE_OP = 0x01,
                WRITE_PERSISTENT = 0x02,
                WRITE_PRIMARY_KEY = 0x04,
                WRITE_METADATA_FIELDS = 0x08,
                WRITE_ALL_FIELDS = 0x10,
                WRITE_SELECT_ROW_ID = 0x20,
                WRITE_ADD_CONDITION = 0x40,
                WRITE_SELECT_GROUP_NAME = 0x80
            };

            enum SecondFlagsWriteMask {
                WRITE_SELECT_SENDER_NODE_ID = 0x01,
                WRITE_SEQ_ID_GREATER_THAN = 0x02,
                WRITE_SEQ_ID_SMALLER_THAN = 0x04,
                WRITE_SEQ_ID = 0x08,
                WRITE_CHUNK_ID = 0x10,
                WRITE_SELECT_TAG = 0x20,
                WRITE_SELECT_CLIENT_ID = 0x40,
                WRITE_SELECT_CLIENT_TYPE = 0x80
            };

            enum ThirdFlagsWriteMask {
                WRITE_PRIORITY = 0x01,
                WRITE_EXCLUDE_CHUNK_IDS = 0x02,
                WRITE_EXCLUDE_RANGE = 0x04,
                WRITE_SELECT_FRAGMENT = 0x08,
                WRITE_FRAG_RANGE_EXACT_MATCH = 0x10,
                WRITE_EXPIRATION_LOWER_THRESHOLD = 0x20,
                WRITE_EXPIRATION_UPPER_THRESHOLD = 0x40,
                WRITE_ARRIVAL_LOWER_THRESHOLD = 0x80
            };

            enum ForthFlagsWriteMask {
                WRITE_ARRIVAL_UPPER_THRESHOLD = 0x01,
                WRITE_UTILITY_LOWER_THRESHOLD = 0x02,
                WRITE_UTILITY_UPPER_THRESHOLD = 0x04,
                WRITE_RESULT_LIMIT = 0x08,
                WRITE_INCLUDE_DATA = 0x10,
                WRITE_NO_FRAGMENTS = 0x20,
                WRITE_ORDER_SET = 0x40,
                WRITE_DESC_ORDER = 0x80
            };
            // TODO Add Message Id NOT IN

            enum FirstFlagsReadMask {
                READ_DELETE_OP = 0xFE,
                READ_PERSISTENT = 0xFD,
                READ_PRIMARY_KEY = 0xFB,
                READ_METADATA_FIELDS = 0xF7,
                READ_ALL_FIELDS = 0xEF,
                READ_SELECT_ROW_ID = 0xDF,
                READ_ADD_CONDITION = 0xBF,
                READ_SELECT_GROUP_NAME = 0x7F
            };

            enum SecondFlagsReadMask {
                READ_SELECT_SENDER_NODE_ID = 0xFE,
                READ_SEQ_ID_GREATER_THAN = 0xFD,
                READ_SEQ_ID_SMALLER_THAN = 0xFB,
                READ_SEQ_ID = 0xF7,
                READ_CHUNK_ID = 0xEF,
                READ_SELECT_TAG = 0xDF,
                READ_SELECT_CLIENT_ID = 0xBF,
                READ_SELECT_CLIENT_TYPE = 0x7F
            };

            enum ThirdFlagsReadMask {
                READ_PRIORITY = 0xFE,
                READ_EXCLUDE_CHUNK_IDS = 0xFD,
                READ_EXCLUDE_RANGE = 0xFB,
                READ_SELECT_FRAGMENT = 0xF7,
                READ_FRAG_RANGE_EXACT_MATCH = 0xEF,
                READ_EXPIRATION_LOWER_THRESHOLD = 0xDF,
                READ_EXPIRATION_UPPER_THRESHOLD = 0xBF,
                READ_ARRIVAL_LOWER_THRESHOLD = 0x7F
            };

            enum ForthFlagsReadMask {
                READ_ARRIVAL_UPPER_THRESHOLD = 0xFE,
                READ_UTILITY_LOWER_THRESHOLD = 0xFD,
                READ_UTILITY_UPPER_THRESHOLD = 0xFB,
                READ_RESULT_LIMIT = 0xF7,
                READ_INCLUDE_DATA = 0xEF,
                READ_NO_FRAGMENTS = 0xDF,
                READ_ORDER_SET = 0xBF,
                READ_DESC_ORDER = 0x7F
            };
            // TODO Add Message Id NOT IN

            NOMADSUtil::String _sqlQuery;

            uint8 _ui8OperationType;
            bool _bDeleteOperation;

            uint8 _ui8TableType;

            bool _bPersistent;

            bool _bPrimaryKey;
            bool _bMetadataFields;
            bool _bAllFields;

            bool _bSelectRowId;

            bool _bAddCustomConstraint;
            NOMADSUtil::String *_customConstraint;

            bool _bConstrGroupName;
            NOMADSUtil::String _constrGroupName;

            bool _bConstrSenderNodeId;
            NOMADSUtil::String _constrSenderNodeId;
            bool _bSelectGroupName;
            NOMADSUtil::String _selectGroupName;

            bool _bSelectSenderNodeId;
            NOMADSUtil::String _selectSenderNodeId;

            bool _bMsgIdNotIn;
            const char **_constrMessageIdFilters;

            bool _bGrpPubSeqIdNotIn;
            NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *_pConstrGrpPubSeqIdFilters;

            bool _bMsgId;
            const char *_constrMessageId;

            bool _bSeqIdGreaterThan;
            bool _bSeqIdSmallerThan;
            bool _bSeqIdGreaterThanOrEqual;
            bool _bSeqIdSmallerThanOrEqual;
            bool _bSeqIdIs;
            uint32 _ui32SeqIdFrom;
            uint32 _ui32SeqIdTo;

            bool _bChunkId;
            uint8 _ui8ChunkId;

            bool _bChunkIdSmallerThanOrEqual;
            uint8 _ui8ChunkIdTo;

            bool _bConstrTag;
            uint8 _ui8NumOfTags;
            NOMADSUtil::DArray<uint16> _tags;

            bool _bConstrClientId;
            uint8 _ui8NumOfClientIds;
            NOMADSUtil::DArray<uint16> _clientIds;

            bool _bConstrClientType;
            uint8 _ui8NumOfClientType;
            NOMADSUtil::DArray<uint8> _clientTypes;

            bool _bConstrPriority;
            uint8 _ui8PriorityFrom;
            uint8 _ui8PriorityTo;

            bool _bExcludeChunkIDs;
            bool _bExcludeRange;
            uint8 _ui8ChunkIdStart;
            uint8 _ui8ChunkIdEnd;
            NOMADSUtil::DArray<uint8> *_pChunkIDs;

            bool _bConstrFragment;
            bool _bFragRangeExactMatch;
            uint32 _ui32FragOffset;
            uint32 _ui32FragLength;

            bool _bExpirationLowerThreshold;
            int64 _i64ExpirationLowerThreshold;

            bool _bExpirationUpperThreshold;
            int64 _i64ExpirationUpperThreshold;

            bool _bArrivalLowerThreshold;
            int64 _i64ArrivalLowerThreshold;

            bool _bArrivalUpperThreshold;
            int64 _i64ArrivalUpperThreshold;

            bool _bUtilityLowerThreshold;
            int8 _i8UtilityLowerThreshold;

            bool _bUtilityUpperThreshold;
            int8 _i8UtilityUpperThreshold;

            bool _bResultLimit;
            uint16 _ui16ResultLimit;

            bool _bIncludeData;

            bool _bNoFragments;

            bool _isOrderSet;
            NOMADSUtil::String _orderBy;
            bool _bDescOrder;
    };
}

#endif	// INCL_DISSERVICE_DATA_CACHE_QUERY_H
