/* 
 * Controller.h
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
 * Created on June 29, 2012, 12:10 PM
 */

#ifndef INCL_CONTROLLER_H
#define	INCL_CONTROLLER_H

#include "MatchmakingHelper.h"
#include "MessageForwardingController.h"
#include "MessageForwardingPolicy.h"
#include "MetadataRankerConfiguration.h"

#include "LoggingMutex.h"
#include "StringHashset.h"

namespace NOMADSUtil
{
    class BufferWriter;
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DSProImpl;
    class InformationPull;
    class InformationPush;
    class InformationPushPolicy;
    class MetaData;
    class Scheduler;
    class Targets;
    class TransmissionHistoryInterface;

    class Controller : public CommAdaptorListener
    {
        public:
            Controller (DSProImpl *pDSPro, LocalNodeContext *pLocalNodeCtxt,
                        Scheduler *pScheduler, InformationStore *pInfoStore,
                        Topology *pTopology, TransmissionHistoryInterface *pTrHistory);
            virtual ~Controller (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr, InformationPushPolicy *pInfoPushPolicy,
                      Scheduler *pScheduler);

            int contextUpdateMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                             const void *pBuf, uint32 ui32Len);
            int contextVersionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                              const void *pBuf, uint32 ui32Len);

            // New data from the network
            int dataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                             const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks);

            // New metadata from the network
            int metadataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                                 const void *pBuf, uint32 ui32Len, const char *pszReferredDataId);

            int messageRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                              const char *pszPublisherNodeId, const char *pszMsgId);
            int chunkRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                            const char *pszPublisherNodeId, const char *pszMsgId,
                                            NOMADSUtil::DArray<uint8> *pCachedChunks);

            int positionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                        const void *pBuf, uint32 ui32Len);
            int searchMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                      SearchProperties *pSearchProperties);
            int searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                           const char *pszQueryId, const char **ppszMatchingMsgIds,
                                           const char *pszTarget, const char *pszMatchingNodeId);
            int searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                           const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                           const char *pszTarget, const char *pszMatchingNodeId);
            int topologyReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                             const void *pBuf, uint32 ui32Len);
            int topologyRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                               const void *pBuf, uint32 ui32Len);
            int updateMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                      const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len);
            int versionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                       const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len);
            int waypointMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                        const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len);
            int wholeMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                     const char *pszPublisherNodeId, const void *pBuf, uint32 ui32Len);

            void newPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId,
                          const char *pszPeerRemoteAddress,
                          const char *pszIncomingInterface);
            void deadPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId);

            void newLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                const char *pszIncomingInterface);
            void droppedLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID, const char *pszPeerRemoteAddr);

            // New metadata from the client app
            int metadataPush (const char *pszId, MetaData *pMetadata,
                              const char *pszPreviusHop=NULL);

        private:
            int dataPull (const char *pszMetadataId, const NOMADSUtil::String &referredObjId, MetaData *pMetadata);
            int previousMetadataPull (MetaData *pMetadata);

        private:
            bool _bPreStagingEnabled;
            bool _bContextForwardingEnabled;

            DSProImpl *_pDSPro;
            InformationPull *_pInfoPull;
            InformationPush *_pInfoPush;
            Scheduler *_pScheduler;
            TransmissionHistoryInterface *_pTrHistory;
            NOMADSUtil::LoggingMutex _m;
            const NOMADSUtil::String _nodeId;
            MessageForwardingController _fwdCtrl;
            NOMADSUtil::StringHashset _deliveredMsgs;
            MessageForwardingPolicy _msgFwdPolicy;
            MetadataRankerLocalConfiguration _rankerLocalConf;
            MatchmakingHelper _matchmakingHelper;
    };
}

#endif	/* INCL_CONTROLLER_H */

