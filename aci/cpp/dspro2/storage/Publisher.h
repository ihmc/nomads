/**
 * Publisher.h
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
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on July 1, 2015, 5:01 PM
 */

#ifndef INCL_DSPRO_PUBLISHER_H
#define	INCL_DSPRO_PUBLISHER_H

#include "ChunkingAdaptor.h"
#include "MessageIdGenerator.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DataStore;
    class InformationStore;
    class MetadataInterface;

    class ThreadSafePublisher
    {
        public:
            ThreadSafePublisher (void);
            virtual ~ThreadSafePublisher (void);

            virtual int addMetadata (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                                     const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                                     const void *pBuf, uint32 ui32Len, int64 i64Expiration) = 0;

            virtual int addData (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                                 const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata,
                                 uint32 ui32AnnotationMetdataLen, const char *pszMimeType,
                                 const char *pszChecksum, const void *pBuf, uint32 ui32Len,
                                 int64 i64Expiration, uint8 ui8NChunks, uint8 ui8TotNChunks) = 0;
    };

    class Publisher : public ThreadSafePublisher
    {
        public:
            Publisher (const char *pszNodeId, const char *pszRootGroupName,
                       InformationStore *pInfoStore, DataStore *pDataStore);
            virtual ~Publisher (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            struct PublicationInfo
            {
                PublicationInfo (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId);
                PublicationInfo (const char *pszGroupName, const char *pszObjectId, const char *pszInstanceId, int64 i64ExpirationTime);
                ~PublicationInfo (void);

                uint32 ui32RefDataSize;
                int64 i64ExpirationTime;
                const char *pszGroupName;
                const char *pszObjectId;
                const char *pszInstanceId;
                const char *pszReferredObjectId;
            };

            /**
             * It adds a metadata message into InformationStore and DataStore
             *
             * NOTE: it modifies pMetadata!
             */
            int setAndAddMetadata (PublicationInfo &pubInfo, MetadataInterface *pMetadata,
                                   NOMADSUtil::String &msgId, bool bStoreInInfoStore);

            /**
             * It chunks the data message and stores it into the DataStore
             */
            int chunkAndAddData (PublicationInfo &pubInfo, const char *pszAnnotatedObjMsgId,
                                 const void *pszAnnotationMetadata, uint32 ui32AnnotationMetdataLen,
                                 const void *pData, uint32 ui32DataLen, const char *pszDataMimeType,
                                 char **ppszId, bool bDoNotChunk);

            /**
             * It stores the metadata message into the DataStore
             * @threadsafe
             */
            int addMetadata (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                             const char *pszMimeType, const char *pszChecksum, const char *pszReferredObjectId,
                             const void *pBuf, uint32 ui32Len, int64 i64Expiration);

            /**
             * It stores the data message into the DataStore
             * @threadsafe
             */
            int addData (const char *pszId, const char *pszObjectId, const char *pszInstanceId,
                         const char *pszAnnotatedObjMsgId, const void *pAnnotationMetadata,
                         uint32 ui32AnnotationMetdataLen, const char *pszMimeType,
                         const char *pszChecksum, const void *pBuf, uint32 ui32Len,
                         int64 i64Expiration, uint8 ui8NChunks = 0, uint8 ui8TotNChunks = 0);

        private:
            const NOMADSUtil::String _nodeId;
            ChunkingConfiguration _chunkingConf;
            MessageIdGenerator _idGen;
            DataStore *_pDataStore;
            InformationStore *_pInfoStore;
    };
}

#endif    // INCL_DSPRO_PUBLISHER_H

