/* 
 * CommAdaptorListenerNotifier.h
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
 * Created on February 5, 2013, 12:24 PM
 */

#ifndef INCL_COMM_ADAPTOR_LISTENER_NOTIFIER_H
#define	INCL_COMM_ADAPTOR_LISTENER_NOTIFIER_H

#include "CommAdaptorListener.h"

#include "DArray2.h"

namespace IHMC_ACI
{
    struct MessageProperties;
    struct SearchProperties;

    class CommAdaptorListenerNotifier : public CommAdaptorListener
    {
        public:
            CommAdaptorListenerNotifier (void);
            ~CommAdaptorListenerNotifier (void);

            int registerCommAdaptorListener (CommAdaptorListener *pListener, unsigned int &uiListenerId);
            int deregisterCommAdaptorListener (unsigned int uiListenerId);

            int contextUpdateMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                             const void *pBuf, uint32 ui32Len);
            int contextVersionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                              const void *pBuf, uint32 ui32Len);
            int dataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                             const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks);
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
                                      SearchProperties *pSearchProp);
            int searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                           const char *pszQueryId , const char **ppszMatchingMsgIds,
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
                          const char *pszPeerRemoteAddress, const char *pszIncomingInterface);
            void deadPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId);
            void newLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID,
                                const char *pszPeerRemoteAddr, const char *pszIncomingInterface);
            void droppedLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID,
                                    const char *pszPeerRemoteAddr);

        private:
            struct ListenerWrapper
            {
                ListenerWrapper (void);
                ~ListenerWrapper (void);

                CommAdaptorListener *pListener;
            };

            NOMADSUtil::DArray2<ListenerWrapper> _listeners;
    };
}

#endif	/* INCL_COMM_ADAPTOR_LISTENER_NOTIFIER_H */

