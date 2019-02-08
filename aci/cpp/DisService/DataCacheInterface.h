/*
 * DataCacheInterface.h
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
 * Author: Mirko Gilioli mgilioli@ihmc.us
 * Created on October 22, 2009, 2:27 PM
 */

#ifndef INCL_DATA_CACHE_INTERFACE_H
#define	INCL_DATA_CACHE_INTERFACE_H

#include "Controllable.h"
#include "ListenerNotifier.h"

#include "DArray2.h"
#include "FTypes.h"
#include "PtrLList.h"
#include "StorageInterface.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Writer;
}

namespace IHMC_MISC
{
    class ChunkingManager;
}

namespace IHMC_ACI
{
    class DataCacheService;
    class DisseminationService;
    class DataCacheExpirationController;
    class DisServiceDataCacheQuery;
    class MessageHeader;
    class Message;
    class Subscription;

    class DataCacheInterface : public Controllable
    {
        public:
            virtual ~DataCacheInterface (void);

            static const uint32 DEFAULT_MAX_CACHE_SIZE = 0;            // 0 means the cache has no size constraint
            static const uint32 DEFAULT_CACHE_SECURITY_THREASHOLD = 0; // If set to 0 the security
                                                                       // threshold is not used
            static const uint8 DEFAULT_CACHE_CLEAN_CYCLE = 10;

            enum StorageMode {
                MEMORY_MODE     = 0x00,
                PERSISTENT_MODE = 0x01
            };

            /**
             * These enum and struct are used to specify the kind of returned result.
             * If
             * - DATA is set: it means that the data pointed by pData is the actual
             *   data of the message.
             * - FILE is set: it means that the data pointed by pData is a file name.
             *   The file named by that file name contains the actual data of the message.
             */
            enum StorageType {
                MEMORY          = 0x00,
                FILE            = 0x01,
                NOSTORAGETYPE   = 0x02
            };

            /**
             * - ui8Type can assume either the values DATA or FILE (read the previous comment for further
             *   details)
             * - pData is either the data of the message or the filename of the file containing the actual
             *   data of the message, depending of the value of ui8Type.
             * -
             */
            struct Result {
                Result (void);
                virtual ~Result (void);

                StorageType ui8StorageType;
                uint8 ui8NChunks;
                uint8 ui8TotalNChunks;
                uint32 ui32Length;
                NOMADSUtil::String objectId;
                NOMADSUtil::String instanceId;
                void *pData;
            };

            StorageInterface * getStorageInterface (void);

            int deregisterAllDataCacheListeners (void);
            int deregisterDataCacheListener (unsigned int uiIndex);
            int registerDataCacheListener (DataCacheListener *pListener);

            /**
             * Add the data to the cache along with the its MessageHeader
             *
             * NOTE: The cache may not make a copy of the data, therefore, the caller
             *       may not be supposed to modify or delete the value of data
             *       after it has been added into the cache.
             *       Whether the data is copied or not depends on the implementation
             *       of the DataCache that is being used. Check the implementing class's
             *       documentation for further details.
             * NOTE: The message info and the data will be automatically deleted
             *       by the DataCache destructor
             * NOTE: if a complete message is stored, the data cache first delete
             *       all the previously received message's fragments
             * RETURNED VALUES:
             *   0  -   data stored
             *  -1  -   no room enough
             */
            int addData (MessageHeader *pMessageHeader, const void *pData);
            int addData (MessageHeader *pMessageHeader, const char *pszFilePath);

            /**
             * Analogous to addData(), but the listener with ID uiListenerID is
             * not notified upon successful insertion.
             */
            int addDataNoNotify (MessageHeader *pMessageHeader, const void *pData,
                                 unsigned int uiListenerID);

            /**
             * Cleans the cache
             */
            void cacheCleanCycle (void);

            /**
             * Specifies the amount of memory (in bytes) that may be used to
             * cache messages (taking into account the size of the data, not the message info)
             */
            virtual void setCacheLimit (uint32 ui32CacheLimit);

            /**
             * This may be used to set a security range which may be used to run a cache cleaning
             * before it reaches a size too close to the ui32CacheLimit
             */
            virtual void setSecurityRangeSize (uint32 ui32SecRange);

            /**
             * pszId is the identifier of a Message or of a fragment (thus a fragment)!!!
             *
             * The method returns true only if the cache has the complete message
             * for the id specified in specified in pszId
             *
             * NOTE: the fact that this method returns false does NOT mean that
             * the fragment identified by pszId is not cached.
             *
             * NOTE: access to this method is not mutually exclusive. If mutually
             * exclusive access is needed the extending class will have to
             * implement it.
             */
            virtual bool hasCompleteMessage (const char *pszId);
            virtual bool hasCompleteMessage (const char *pszGroupName, const char *pszPublisherNodeId,
                                             uint32 ui32MsgSeqId, uint8 ui8ChunkId);
            virtual bool hasCompleteMessage (MessageHeader *pMH);

            virtual bool hasCompleteMessageOrAnyCompleteChunk (const char *pszId);
            virtual bool hasCompleteMessageOrAnyCompleteChunk (const char *pszGroupName, const char *pszPublisherNodeId,
                                                               uint32 ui32MsgSeqId);

            /**
             * Returns true if the cache contains a message with exactly the same
             * msgId of the MessageInfo passed.
             *
             * TODO: make this return true also when the message (fragment) is
             * contained in the cache, split on more fragments.
             *
             * NOTE: access to this method is not mutually exclusive. If mutually
             * exclusive access is needed the extending class will have to
             * implement it.
             */
            virtual bool containsMessage (MessageHeader *pMH);

            int countChunks (const char *pszGroupName, const char *pszPublisherNodeId,
                             uint32 ui32MsgSeqId, unsigned int &uiTotNChunks);

            // GET MESSAGE INFO(s)
            virtual MessageHeader * getMessageInfo (const char *pszId);
            virtual int release (const char *pszId, MessageHeader *pMH) = 0;

            /**
             * NOTE: if the query matches chunks of a large object, only one
             *       of them is returned in the PtrLList. If the caller needs
             *       the large object, it can use the obtained ChunkMessageInfo
             *       to retrieve the other chunks.
             *
             * NOTE: in the second version, the one that accept pQuery as a
             *       parameter, the caller MUST select the PRIMARY KEY of the
             *       table with the method selectPrimaryKey()
             */
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMessageInfos (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                           uint8 ui8ClientType, uint16 ui6Tag,
                                                                           uint32 ui32From, uint32 ui32To);
            NOMADSUtil::PtrLList<MessageHeader> * getMessageInfos (DisServiceDataCacheQuery *pQuery);
            virtual NOMADSUtil::PtrLList<MessageHeader> * getMessageInfos (const char *pszSQLStatement);
            NOMADSUtil::PtrLList<MessageId> * getNotReplicatedMsgList (const char *pszTargetPeer, unsigned int uiLimit,
                                                                       NOMADSUtil::StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pFilters);
            NOMADSUtil::PtrLList<MessageHeader> * getCompleteChunkMessageInfos (const char *pszSQLStatement);
            NOMADSUtil::PtrLList<MessageHeader> * getCompleteChunkMessageInfos (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                                uint32 ui32MsgSeqId);
            virtual NOMADSUtil::PtrLList<Message> * getCompleteMessages (const char *pszGroupName, const char *pszPublisherNodeId, uint32 ui32SeqId) = 0;
            virtual int release (NOMADSUtil::PtrLList<MessageHeader> *pMessageInfos)=0;

            // GET DATA

            /**
             * Returns the data (or metadata) given the Message Id
             * In case the message has been published by
             * - push: result.pData contains published data.
             * - makeAvailable: result.pData contains the metadata which describes
             *   the actual data published.
             */
            virtual const void * getData (const char *pszId)=0;
            virtual const void * getData (const char *pszId, uint32 &ui32Len)=0;

            virtual int release (const char *pszId, void *pData)=0;

            virtual void getData (const char *pszId, Result &result) = 0;
            virtual int release (const char *pszId, Result &result)=0;

            char ** getDisseminationServiceIds (const char *pszObjectId, const char *pszInstanceId);

            // MISC

            /**
             * Get all the senderIDs that the node knows to have published at least
             * one message in the group pszGroupName.
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getSenderNodeIds (const char *pszGroupName);

            /**
             * Retrieve what subscriptions are in the network by analyzing the data
             * in the cache.
             *
             * NOTE: this method only provides the subscription's name and tag. Information
             * about reliability/sequentiality are not provided
             */
            virtual NOMADSUtil::PtrLList<StorageInterface::RetrievedSubscription> * getSubscriptions (const char *pszPublisherNodeId);

            /**
             * Get the next expected sequence ID of the message published by pszPublisherNodeId in the
             * group pszGroupName, with tag ui16Tag with highest sequence ID.
             */
            virtual uint32 getNextExpectedSeqId (const char *pszGroupName, const char *pszPublisherNodeId, uint16 ui16Tag);

            virtual NOMADSUtil::PtrLList<Message> * getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                          uint32 ui32MsgSeqId);
            virtual NOMADSUtil::PtrLList<Message> * getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                          uint32 ui32MsgSeqId, uint8 ui8ChunkId);
            virtual NOMADSUtil::PtrLList<Message> * getMatchingFragments (const char *pszGroupName, const char *pszPublisherNodeId,
                                                                          uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                                                          uint32 ui32StartOffset, uint32 ui32EndOffset);

            /**
             * Get a PtrLList containing all the messages matching the specified
             * parameters.
             * ui32StartSeqNo and ui32EndSeqNo are the limit of an open interval
             * (example: if ui32StartSeqNo = 3 and ui32EndSeqNo = 5, message with
             * sequence id in {3, 4, 5} will be returned.
             *
             * NOTE: if the query matches chunks of a large object, only one
             *       of them is returned in the PtrLList. If the caller needs
             *       the large object, it can use the obtained ChunkMessageInfo
             *       to retrieve the other chunks.
             */
            virtual NOMADSUtil::PtrLList<Message> * getMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                                                 uint16 ui16Tag, uint32 ui32StartSeqNo, uint32 ui32EndSeqNo)=0;
            virtual int release (NOMADSUtil::PtrLList<Message> *pMessages)=0;
            virtual int release (Message *pMessage)=0;

            /**
             * Get all the message IDs of the messages that match against the
             * passed pszSQLStatement
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getMessageIDs (const char *pszSQLStatement);

            /**
             * Return the ui16HistoryLength most recent cached message's publishers ordered by
             * DESCENDING order.
             * pszGroupName, ui16Tag can be set to limit the history to a certain groupName/Tag
             *
             * The returned array may contain replicate values.
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getRecentPublishers (uint16 ui16HistoryLength, const char *pszGroupName,
                                                                                   uint16 ui16Tag)=0;
            void * getAnnotationMetadata (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId, uint32 &ui32Len);

            IHMC_MISC::ChunkingManager * getChunkingMgr (void);

            virtual void clear (void) = 0;

        protected:
            static const bool DEFAULT_IS_NOT_TARGET;
            friend class DataCacheService;

            DataCacheInterface (void);
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getExpiredEntries (void)=0;
            virtual bool cleanCache (uint32 ui32Length, MessageHeader *pMH, void *pData);

            virtual int addDataNoNotifyInternal (MessageHeader *pMessageHeader, const void *pData,
                                                 unsigned int uiListenerID) = 0;

            /*
             * Delete a single message of fragment. Both the data in the cache
             * and the MessageInfo are deleted.
             */
            virtual int deleteDataAndMessageInfo (const char *pszKey,
                                                  bool bIsLatestMessagePushedByNode)=0;

        protected:
            uint32 _ui32CacheLimit;
            uint32 _secRange;
            uint32 _ui32CurrentCacheSize;
            IHMC_MISC::ChunkingManager *_pChunkingMgr;
            StorageInterface *_pDB;
            DataCacheListenerNotifier _notifier;

        private:
            NOMADSUtil::PtrLList<Message> * getMatchingFragments (NOMADSUtil::PtrLList<MessageHeader> *pMIs);
    };

    inline StorageInterface * DataCacheInterface::getStorageInterface (void)
    {
        return _pDB;
    }

    class DataCacheFactory
    {
        public:
            static DataCacheInterface * getDataCache (NOMADSUtil::ConfigManager *pCfgMgr);
            static DataCacheInterface * getDataCache (DataCacheInterface::StorageMode mode,
                                                      const NOMADSUtil::String &storageDBName, const NOMADSUtil::String &sessionId,
                                                      bool bUseTransactionTimer=false);

            static NOMADSUtil::String generateDatabaseName (DataCacheInterface::StorageMode mode,
                                                            const NOMADSUtil::String &storageDBName, const NOMADSUtil::String &sessionId);

        private:
            static DataCacheInterface *_pDataCache;
    };
}

#endif  // INCL_DATA_CACHE_INTERFACE_H
