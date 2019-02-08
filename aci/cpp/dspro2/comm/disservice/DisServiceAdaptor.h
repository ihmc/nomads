/*
 * DisServiceAdaptor.h
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on June 26, 2012, 10:42 PM
 */

#ifndef INCL_DISSERVICE_ADAPTOR_H
#define INCL_DISSERVICE_ADAPTOR_H

#include "CommAdaptor.h"

#include "MessageHeaders.h"

#include "DisseminationServiceListener.h"
#include "Listener.h"
#include "Services.h"

#include "UInt32Hashset.h"
#include "StringHashset.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class ChunkRetrievalMsgQuery;
    class ControllerToControllerMsg;
    class DisseminationService;
    class DisServiceMsg;
    class Message;
    class PropertyStoreInterface;
    class SearchMsg;
    class SearchReplyMsg;
    class VolatileSearchReplyMsg;

    class DisServiceAdaptor : public CommAdaptor,
                              private DataCacheService,
                              private DisseminationServiceListener,
                              private MessageListener,
                              private PeerStateListener,
                              private MessagingService,
                              private NetworkStateListener
    {
        public:
            static const char * DSPRO_GROUP_NAME;
            static const char * DSPRO_CTRL_TO_CTRL_GROUP_NAME;

            ~DisServiceAdaptor (void);

            static bool checkGroupName (const char *pszIncomingGroupName, const char *pszExpectedGroupName);
            static DisServiceAdaptor * getDisServiceAdaptor (unsigned int uiId, const char *pszNodeId,
                                                             CommAdaptorListener *pListener,
                                                             NOMADSUtil::ConfigManager *pCfgMgr);

            DisseminationService * getDisseminationService (void);

            int init (NOMADSUtil::ConfigManager *pConfMgr);

            int changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len);

            int startAdaptor (void);
            int stopAdaptor (void);

            bool supportsManycast (void);

            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                     DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                     const char *pszIncomingInterface);

            void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUID);
            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);

            bool dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                              uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                              const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                              const void *pData, uint32 ui32Length, uint32 ui32MetadataLength,
                              uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                               uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                               const char *pszMimeType, const void *pChunk, uint32 ui32Length,
                               uint8 ui8NChunks, uint8 ui8TotNChunks, const char *pszChunkedMsgId,
                               uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                  uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                  const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength,
                                  bool bDataChunked, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                const char *pszMimeType, const char *pszRefObjId, const void *pMetadata,
                                uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            void resetTransmissionCounters (void);

            int sendContextUpdateMessage (const void *pBuf, uint32 ui32BufLen,
                                          const char **ppszRecipientNodeIds,
                                          const char **ppszInterfaces);
            int sendContextVersionMessage (const void *pBuf, uint32 ui32BufLen,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces);
            int sendDataMessage (Message *pMsg, const char **ppszRecipientNodeIds,
                                 const char **ppszInterfaces);
            int sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
                                    const char **ppszRecipientNodeIds,
                                    const char **ppszInterfaces);
            int sendMessageRequestMessage (const char *pszMsgId, const char *pszPublisherNodeId,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces);
            int sendChunkRequestMessage (const char *pszMsgId,
                                         NOMADSUtil::DArray<uint8> *pCachedChunks,
                                         const char *pszPublisherNodeId,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces);
            int sendPositionMessage (const void *pBuf, uint32 ui32BufLen,
                                     const char **ppszRecipientNodeIds,
                                     const char **ppszInterfaces);
            int sendSearchMessage (SearchProperties &searchProp,
                                   const char **ppszRecipientNodeIds,
                                   const char **ppszInterfaces);
            int sendSearchReplyMessage (const char *pszQueryId,
                                        const char **ppszMatchingMsgIds,
                                        const char *pszTarget,
                                        const char *ppszMatchingNode,
                                        const char **ppszRecipientNodeIds,
                                        const char **ppszInterfaces);
            int sendVolatileSearchReplyMessage (const char *pszQueryId,
                                                const void *pReply, uint16 ui16ReplyLen,
                                                const char *pszTarget,
                                                const char *pszMatchingNode,
                                                const char **ppszRecipientNodeIds,
                                                const char **ppszInterfaces);
            int sendTopologyReplyMessage (const void *pBuf, uint32 ui32BufLen,
                                          const char **ppszRecipientNodeIds,
                                          const char **ppszInterfaces);
            int sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len,
                                            const char **ppszRecipientNodeIds,
                                            const char **ppszInterfaces);
            int sendUpdateMessage (const void *pBuf, uint32 ui32BufLen,
                                   const char *pszPublisherNodeId,
                                   const char **ppszRecipientNodeIds,
                                   const char **ppszInterfaces);
            int sendVersionMessage (const void *pBuf, uint32 ui32BufLen,
                                    const char *pszPublisherNodeId,
                                    const char **ppszRecipientNodeIds,
                                    const char **ppszInterfaces);
            int sendWaypointMessage (const void *pBuf, uint32 ui32Len,
                                     const char *pszPublisherNodeId,
                                     const char **ppszRecipientNodeIds,
                                     const char **ppszInterfaces);
            int sendWholeMessage (const void *pBuf, uint32 ui32BufLen,
                                  const char *pszPublisherNodeId,
                                  const char **ppszRecipientNodeIds,
                                  const char **ppszInterfaces);
            int notifyEvent (const void *pBuf, uint32 ui32Len,
                             const char *pszPublisherNodeId,
                             const char *pszTopic, const char **ppszInterfaces);

            int subscribe (Subscription &sub);

            void networkQuiescent (const char **pszInterfaces);
            void messageCountUpdate (const char *pszPeerNodeId, const char *pszIncomingInterface, const char *pszPeerIp,
                                     uint64 ui64GroumMsgCount, uint64 ui64UnicastMsgCount);

        protected:
            DisServiceAdaptor (unsigned int uiId, CommAdaptorListener *pListener,
                               const char *pszNodeId, DisseminationService *pDisService);

        private:
            void handleDataMsg (DisServiceDataMsg* pDisServiceMsg, const char *pszIncomingInterface);
            void handleChunkRetrievalMsg (ChunkRetrievalMsgQuery* pChunkRetrievalQueryMsg,
                                          const char *pszIncomingInterface);
            void handleCtrlToCtrlMsg (ControllerToControllerMsg *pCtrlMsg,
                                      const char *pszIncomingInterface);
            void handleSearchMsg (SearchMsg *pSearchMsg, const char *pszIncomingInterface);
            void handleSearchReplyMsg (SearchReplyMsg *pSearchReplyMsg, const char *pszIncomingInterface);
            void handleSearchReplyMsg (VolatileSearchReplyMsg *pSearchReplyMsg, const char *pszIncomingInterface);

            int sendAndLogCtrMsg (const void *pBuf, uint32 ui32BufLen,
                                  const char *pszPublisherNodeId,
                                  const char **ppszRecipientNodeIds,
                                  const char **ppszInterfaces,
                                  MessageHeaders::MsgType type);

        private:
            static const uint16 DSPRO_CLIENT_ID;
            static const uint8 DSPRO_SUBSCRIPTION_PRIORITY;

            DisseminationService *_pDisService;
            PropertyStoreInterface *_pPropertyStore;
            static DisServiceAdaptor *_pDisServiceAdaptor;
            NOMADSUtil::Mutex _mSubscribedGrps;
            NOMADSUtil::UInt32Hashset _periodicCtrlMessages;
            NOMADSUtil::StringHashset _subscribedGroups;
    };

    inline bool DisServiceAdaptor::supportsManycast (void)
    {
        return true;
    }
}

#endif // INCL_DISSERVICE_ADAPTOR_H
