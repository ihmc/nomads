/*
 * PersistentDataCache.h
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

#ifndef INCL_PERSISTENT_DATA_CACHE_H
#define INCL_PERSISTENT_DATA_CACHE_H

#include "DisServiceDefs.h"

#include "ChunkingAdaptor.h"
#include "DataCacheInterface.h"

#include "DArray2.h"
#include "FTypes.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class String;
    class Writer;
}

namespace IHMC_ACI
{
    class ChunkMsgInfo;
    class DataCacheExpirationController;
    class DisseminationService;
    class DisServiceDataCacheQuery;
    class Message;
    class MessageHeader;
    class MessageInfo;
    class SQLMessageStorage;
}

namespace IHMC_ACI
{
    class PersistentDataCache : public DataCacheInterface
    {
        public:
            PersistentDataCache (bool bUseTransactionTimer);
            PersistentDataCache (const char *pszDBName, bool bUseTransactionTimer);
            virtual ~PersistentDataCache (void);

            // GET THE MESSAGE INFO

            int release (NOMADSUtil::PtrLList<MessageHeader> *pMessageInfos);

            // GET THE DATA
            const void * getData (const char *pszId);
            const void * getData (const char *pszId, uint32 &ui32Len);

            int release (const char *pszId, void *pData);

            // GET THE WHOLE MESSAGE (MessageInfo AND Data)

            NOMADSUtil::PtrLList<Message> * getMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                                         uint16 ui16Tag, uint32 ui32StartSeqNo, uint32 ui32EndSeqNo);
            NOMADSUtil::PtrLList<Message> * getCompleteMessages (const char *pszGroupName, const char *pszSenderNodeId,
                                                                 uint32 ui32SeqId);
            int release (NOMADSUtil::PtrLList<Message> *pMessages);
            int release (Message *pMessage);

            // MISC
            NOMADSUtil::DArray2<NOMADSUtil::String> * getRecentPublishers (uint16 ui16HistoryLength,
                                                                           const char *pszGroupName,
                                                                           uint16 ui16Tag);

            void getData (const char *pszId, Result &result);
            int release (const char *pszId, Result &result);
            int release (const char *pszId, MessageHeader *pMI);

        protected:
            /**
             * NOTE: PersistentDataCache does make a copy of the data, therefore, the caller
             *       can delete/modify the value of data after it has been added into the cache.
             * NOTE: This data cache implementation stores both MessageHeaders and data
             *       in the database.
             */
            int addDataNoNotifyInternal (MessageHeader *pMessageHeader, const void *pData,
                                         unsigned int uiListenerID);

        private:
            int addDataNoNotifyInternal (Message *pMessage);
            NOMADSUtil::DArray2<NOMADSUtil::String> * getExpiredEntries (void);           

            void getDataInternal (const char *pszId, Result &result);

            /*
             * Delete a single message of fragment. Both the data in the cache and the MessageInfo are deleted.
             */
            int deleteDataAndMessageInfo (const char *pszKey, bool bIsLatestMessagePushedByNode);

            SQLMessageStorage *_pPersistentDB;
    };
}

#endif   // #ifndef INCL_PERSISTENT_DATA_CACHE_H
