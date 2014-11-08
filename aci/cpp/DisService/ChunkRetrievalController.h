/*
 * ChunkRetrievalController.h
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
 *
 * ChunkDiscoveryController and ChunkRetrievalController
 * handle the discovery and subsequent retrieval of chunks.
 *
 * Author: Domenico Castagnaro            (dcastagnaro@ihmc.us)
 *         Giacomo Benincasa              (gbenincasa@ihmc.us)
 */

#ifndef INCL_CHUNK_RETRIEVAL_CONTROLLER_H
#define INCL_CHUNK_RETRIEVAL_CONTROLLER_H

#include "Listener.h"
#include "Services.h"

#include "LoggingMutex.h"
#include "PtrLList.h"
#include "StringHashset.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{   
    class ChunkDiscoveryController;
    class ChunkRetrievalMsgQuery;
    class ChunkRetrievalMsgQueryHits;
    class DisseminationService;
    class DataCacheInterface;
    class MessageReassembler;        

    class ChunkRetrievalController : public MessageListener,
                                     public MessagingService
    {
        public:
            ChunkRetrievalController (DisseminationService *pDisService,
                                      ChunkDiscoveryController *pDiscoveryCtrl,
                                      DataCacheInterface *pDataCacheInterface);
            virtual ~ChunkRetrievalController (void);

            /**
             * Handle retrieval message type received.
             * It checks which type of message is arrived and respond it.
             */                        
            void newIncomingMessage (const void *, uint16, DisServiceMsg *pDisServiceMsg, uint32,
                                     const char *pszIncomingInterface);

        private:
            /**
             * Handle a Query message received.
             */
            int replyToQuery (ChunkRetrievalMsgQuery *pChunkRetrievalMsgQuery,
                              const char *pszIncomingInterface);

            /**
             * Handle a Query Hit message received.
             */
            int replyToQueryHits (ChunkRetrievalMsgQueryHits *pChunkRetrievalMsgQueryHits);
            
            /**
             * Store parameters to use in the Query Hit history.
             * @param pszSenderNodeId
             * @param pMH
             */
            struct Hit
            {
                Hit (const char* pszSenderNodeId, MessageHeader *pMH);
                virtual ~Hit (void);
                
                const char *_pszSenderNodeId;
                MessageHeader *_pMH;
            };
            
            /**
             * Store a list of Hit related to a pszQueryId.
             */
            struct ContainerHitsList
            {
                ContainerHitsList (NOMADSUtil::PtrLList<IHMC_ACI::ChunkRetrievalController::Hit> *pListHits);
                virtual ~ContainerHitsList (void);
                
                NOMADSUtil::PtrLList<IHMC_ACI::ChunkRetrievalController::Hit> *_pListHits;
            };  

            ChunkDiscoveryController *_pDiscoveryCtrl;
            DisseminationService *_pDisService;        
            DataCacheInterface *_pDataCacheInterface;
            MessageReassembler *_pMessageReassembler;                          
            Hit *_pHit;
            NOMADSUtil::PtrLList<IHMC_ACI::ChunkRetrievalController::Hit> *_pListHits;
            ContainerHitsList *_pContainerHitsList;
            NOMADSUtil::StringHashtable<ContainerHitsList> _hitHistoryTable; // It maps a Query Id to Queries Hit received.
    };

    class ChunkDiscoveryController : public MessageListener,
                                     public MessagingService
    {
        public:
            ChunkDiscoveryController (DisseminationService *pDisService,
                                      DataCacheInterface *pDataCacheInterface);
            ~ChunkDiscoveryController (void);

            bool hasReceivedHitForQuery (const char *pszQueryId);
            void newIncomingMessage (const void *, uint16, DisServiceMsg *pDisServiceMsg,
                                     uint32, const char *);

            void retrieve (const char *pszGroupName, const char *pszSenderNodeId,
                           uint32 ui32MsgSeqId, int64 i64Timeout);
            void retrieve (const char *pszId, int64 i64Timeout);

            void sendDiscoveryMessage (void);

        private:
            void retrieveInternal (const char *pszId, int64 i64Timeout, bool bLock);

        private:
            NOMADSUtil::LoggingMutex _m;
            NOMADSUtil::StringHashtable<int64> _discovering;
            DataCacheInterface *_pDataCacheInterface;
            DisseminationService *_pDisService;
    };
}
#endif // INCL_CHUNK_RETRIEVAL_CONTROLLER_H
