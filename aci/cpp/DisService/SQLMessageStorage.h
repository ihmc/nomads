/*
 * SQLMessageStorage.h
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
 * Implementation of StorageInterface to store
 * both the MessageHeader part and the data part
 * of a message in a database.
 */

#ifndef INCL_SQL_MESSAGE_STORAGE_H
#define INCL_SQL_MESSAGE_STORAGE_H

#include "SQLMessageHeaderStorage.h"

#include "FTypes.h"
#include "PtrLList.h"
#include "ManageableThread.h"

namespace IHMC_ACI
{
    class DisServiceDataCacheQuery;

    class SQLMessageStorage : public SQLMessageHeaderStorage
    {
        public:
            SQLMessageStorage (const char *pszDBName = NULL, bool bUseTransactionTimer = false);
            virtual ~SQLMessageStorage (void);

            int init (void);

            /**
             * Returns the data matching the key from the table specified by the
             * parameter
             */
            void * getData (const char *pszKey);

            /**
             * Returns the Message matching the key from the table specified by
             * the parameter
             * NOTE: The key may be <GroupName>:<OriginatorNodeId>:<MsgSeqId>:<ChunkId> of
             *       <Groupname>:<originatorNodeId>:<MsgSeqId>:<ChunkId>:<FragmentOffset>:<FragmentLength>
             * In the first case, the assumption is that the complete message for the specified chunk is desired
             * and the query is the same as the FragmentOffset being set to 0 and the FragmentLength being set to the TotalMsgLength
             */
            Message * getMessage (const char *pszKey);

            /**
             * Returns a list of Message objects that match the Query contained
             * in the DisServiceDataCacheQuery
             * NOTE : To use the method the Primary Key MUST be selected with
             *        the appropriated method in DisServiceDataCacheQuery.
             */
            NOMADSUtil::PtrLList<Message> * getMessages (DisServiceDataCacheQuery *pQuery);
            NOMADSUtil::PtrLList<Message> * getCompleteChunks (const char *pszGroupName, const char *pszSender,
                                                               uint32 ui32MsgSeqId);
            NOMADSUtil::PtrLList<Message> * getCompleteAnnotations (const char *pszAnnotatedObjMsgId);

        protected:
            friend class DataCache;
            friend class PersistentDataCache;
            friend class DataCacheReplicationController;

            NOMADSUtil::String getCreateTableSQLStatement (void);
            NOMADSUtil::String getInsertIntoTableSQLStatement (void);

            /*
             * Insert the MessageInfo fields into the default data cache
             */
            int insertIntoDataCacheBind (IHMC_MISC::PreparedStatement *pStmt, Message *pMsgHeader);

        private:
            NOMADSUtil::PtrLList<Message> * getMessages (IHMC_MISC::PreparedStatement *pStmt, uint16 ui16LimitElements = 0);

            class CommitThread: public NOMADSUtil::ManageableThread
            {
                public:
                    CommitThread (SQLMessageStorage *parent);
                    virtual ~CommitThread (void);

                    virtual void run (void);

                    SQLMessageStorage *parent;
            };

            CommitThread *_pCommitThread;
            bool _bUseTransactionTimer;

            DisServiceDataCacheQuery *_pDSDCQuery;
            IHMC_MISC::PreparedStatement *_pGetData;
            IHMC_MISC::PreparedStatement *_pGetFullyQualifiedMsg;    // Used to retrieve <GroupName>:<OriginatorNodeId>:<MsgSeqId>:<ChunkId>:<FragmentOffset>:<FragmentLength>
            IHMC_MISC::PreparedStatement *_pGetMsg;                  // Used to retrieve <GroupName>:<OriginatorNodeId>:<MsgSeqId>:<ChunkId>
            IHMC_MISC::PreparedStatement *_pGetComplChunksPrepStmt;
            IHMC_MISC::PreparedStatement *_pGetComplAnnotationsPrepStmt;
    };
}

#endif   // #ifndef INCL_SQL_MESSAGE_STORAGE_H
