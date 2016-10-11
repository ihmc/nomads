/*
 * DataStore.h
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
 */

#ifndef INCL_DATA_STORE_H
#define INCL_DATA_STORE_H

#include "DataCacheInterface.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DataCacheInterface;
    class DataStore;
    class Message;
    class PropertyStoreInterface;

    /**
     * After instantiation, DataStore is thread-safe
     */
    class DataStore
    {
        public:
            static const char * PERSISTENCE_MODE;
            static const char * PERSISTENCE_FILE;
            static const char * PERSISTENCE_AUTOCOMMIT;

            enum Persistence
            {
                IN_MEMORY = 0,
                ON_FILE   = 1
            };

            virtual ~DataStore (void);

            static DataStore * getDataStore (NOMADSUtil::ConfigManager *pCfgMgr, const char *pszSessionId);

            /**
             * Insert a message into the data store
             * @thread-safe
             */
            virtual int insert (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                                const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen, 
                                const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                                const void *pBuf, uint32 ui32Len, bool bIsMetadata, int64 i64ExpirationTimeout, uint8 ui8ChunkId,
                                uint8 ui8TotNChunks);

            /**
             * Returns the IDs of the chunks that are locally cached, for the
             * message identified by pszMsgId.
             */
            NOMADSUtil::DArray<uint8> * getCachedChunkIDs (const char *pszMsgId, uint8 &ui8TotalNumberOfChunks);

            /**
             * NOTE: returns a copy of the message data, Message, MessageHeader
             *       and data must be deallocated by the caller.
             */
            virtual Message * getCompleteMessage (const char *pszMsgId);

            /**
             * NOTE: returns a copy of the message data, Message, MessageHeader
             *       and data must be deallocated by the caller using releaseChunks()
             */
            virtual NOMADSUtil::PtrLList<Message> * getCompleteChunks (const char *pszMsgId);
            int releaseChunks (NOMADSUtil::PtrLList<Message> *pMessages);

            /**
             * NOTE: returns a copy of the data, that should be deallocated by
             *       the caller.
             */
            virtual int getData (const char *pszMsgId, void **ppData, uint32 &ui32DataLength);
            virtual bool hasData (const char *pszMsgId);

            /**
             * Returns the list of DSPro identifiers that correspond to the
             * object id in pszObjectId, and to the instance id in pszInstanceId.
             * If pszInstanceId is set to NULL, then all the DSPro ids that
             * correspond to pszObjectId are returned.
             */
            char ** getDSProIds (const char *pszObjectId, const char *pszInstanceId);

            /**
             * Returns the sequence id that should be used to generate the ID
             * of a message being added to the data cache
             */
            uint32 getNextExpectedSeqId (const char *pszGroupName, const char *pszSenderId);

            bool isMetadataMessageStored (const char *pszMsgId);

            virtual int getNumberOfReceivedChunks (const char *pszId, uint8 &ui8NumberOfChunks,
                                                   uint8 &ui8TotalNumberOfChunks)= 0;
            virtual int getReceivedChunkIds (const char *pszId, NOMADSUtil::DArray<uint8> &receivedChunkIds,
                                             uint8 &ui8TotalNumberOfChunks) = 0;

            NOMADSUtil::PtrLList<MessageHeader> * getMessageInfos (const char *pszSQLStatement);

            PropertyStoreInterface * getPropertyStore (void);

            void lock (void);
            void unlock (void);

        protected:
            explicit DataStore (DataCacheInterface *pDataCache);

        protected:
            DataCacheInterface *_pDataCache; // _pDataCache is thread safe

        private:
            static DataStore *_pDataStore;
    };

    class DisServiceDataStore : public DataStore
    {
        public:
            ~DisServiceDataStore (void);

            int insert (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                        const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                        const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                        const void *pBuf, uint32 ui32Len, bool bIsMetadata, int64 i64ExpirationTimeout,
                        uint8 ui8NChunks, uint8 ui8TotNChunks);

            int getNumberOfReceivedChunks (const char *pszId, uint8 &ui8NumberOfChunks,
                                           uint8 &ui8TotalNumberOfChunks);
            int getReceivedChunkIds (const char *pszId, NOMADSUtil::DArray<uint8> &receivedChunkIds,
                                     uint8 &ui8TotalNumberOfChunks);

        private:
            friend class DataStore;

            explicit DisServiceDataStore (DataCacheInterface *pDataCache);
    };
}


#endif // INCL_DATA_STORE_H


