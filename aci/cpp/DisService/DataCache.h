/*
 * DataCache.h
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

#ifndef INCL_DATA_CACHE_H
#define INCL_DATA_CACHE_H

#include "DataCacheInterface.h"

#include "DArray2.h"
#include "FTypes.h"
#include "PtrLList.h"
#include "MessageInfo.h"

namespace NOMADSUtil
{
    class String;
    class Writer;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class DisServiceDataCacheQuery;
    class DataCacheExpirationController;
    class Message;
    class MessageHeader;
    class StorageInterface;
    #if defined (USE_SQLITE)
        class SQLMessageHeaderStorage;
    #endif
}

namespace IHMC_ACI
{
    class DataCache : public DataCacheInterface
    {
        public:
            DataCache (void);
            virtual ~DataCache (void);

            // GET THE MESSAGE INFO
            MessageHeader * getMessageInfo (const char *pszId);
            int release (NOMADSUtil::PtrLList<MessageHeader> *pMessageInfos);

            // GET THE DATA

            /**
             * Returns a copy of the stored data.
             * NOTE: The returned data should be deallocated by the caller.
             */
            const void * getData (const char *pszId);
            const void * getData (const char *pszId, uint32 &ui32Len);
            int release (const char *pszId, void *pData);

            void getData (const char *pszId, Result &result);
            int release (const char *pszId, Result &result);

            // GET THE WHOLE MESSAGE (MessageInfo AND Data)
            NOMADSUtil::PtrLList<Message> * getMatchingFragments (const char *pszGroupName, const char *pszSenderNodeId,
                                                                  uint32 ui32MsgSeqId, uint8 ui8ChunkId,
                                                                  uint32 ui32StartOffset, uint32 ui32EndOffset);
            NOMADSUtil::PtrLList<Message> * getMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                                         uint16 ui16Tag, uint32 ui32StartSeqNo, uint32 ui32EndSeqNo);
            NOMADSUtil::PtrLList<Message> * getCompleteMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                                                 uint32 ui32SeqId);
            int release (NOMADSUtil::PtrLList<Message> *pMessages);
            int release (Message *pMessage);

            // MISC
            NOMADSUtil::DArray2<NOMADSUtil::String> * getSenderNodeIds (const char *pszGroupName);

            /**
             * Return the ui16HistoryLength most recent cached message's publishers ordered by 
             * DESCENDING order.
             * pszGroupName, ui16Tag can be set to limit the history to a certain groupName/Tag
             *
             * The returned array may contain replicate values.
             */
            NOMADSUtil::DArray2<NOMADSUtil::String> * getRecentPublishers (uint16 ui16HistoryLength,
                                                                           const char *pszGroupName,
                                                                           uint16 ui16Tag);

            int release (const char *pszId, MessageHeader *pMI);

        protected:
            /**
             * NOTE: DataCache does not make a copy of the data, therefore, the caller
             *       must not modify or delete the value of data after it has been added
             *       into the cache.
             * NOTE: This data cache implementation stores the MessageHeaders in the data
             * base while the data is kept in a StringHashtable.
             */
            int addDataNoNotifyInternal (MessageHeader *pMessageHeader, const void *pData,
                                         unsigned int uiListenerID);

        private:
            NOMADSUtil::DArray2<NOMADSUtil::String> * getExpiredEntries (void);
            NOMADSUtil::PtrLList<Message> * getCompleteMessagesInternals (const char *pszGroupName,
                                                                          const char *pszSenderNodeId,
                                                                          uint32 ui32SeqId);
            NOMADSUtil::PtrLList<Message> * getMessages (NOMADSUtil::PtrLList<MessageHeader> *pMHs);

            /**
             * Returns a copy of the stored data.
             * NOTE: The returned data should be deallocated by the caller.
             */
            void getDataInternal (const char *pszId, Result &result);
            
            enum EntryType
            {
                DC_Entry = 0x00,
                DC_FileDataEntry = 0x01
            };

            struct EntryHeader
            {
                virtual ~EntryHeader (void);

                virtual uint32 getLength (void)=0;

                EntryType ui8Type;
                uint32 ui32Length;
                
                protected:
                    EntryHeader (EntryType ui8Type);
            };

            struct Entry : public EntryHeader
            {
                Entry (void);
                ~Entry (void);
                uint32 getLength (void);

                void *pData;
            };

            struct FileEntry : public EntryHeader
            {
                FileEntry (void);
                ~FileEntry (void);
                uint32 getLength (void);

                char *pszFilePath;
            };

            int addDataNoNotifyInternal (MessageHeader *pMessageHeader, Entry *pEntry,
                                         unsigned int uiListenerID);

            /*
             * Delete a single data in the cache but not the correspondent MessageInfo in the DB.
             */
            int deleteDataFromCache (const char *pszKey);

            /*
             * Delete a single message of fragment. Both the data in the cache and the MessageInfo are deleted.
             */
            int deleteDataAndMessageInfo (const char *pszKey, bool bIsLatestMessagePushedByNode);

            /* 
             * For both the hashtable the KEY value is the message ID.
             * defaultCache (where the general data is kept) and the
             * _chunkCache (where chunks are kept).
             */            
            NOMADSUtil::StringHashtable<Entry> _dataCache;
        };
}

#endif   // #ifndef INCL_MESSAGE_CACHE_H

