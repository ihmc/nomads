/*
 * CommAdaptorListener.h
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
 * Created on February 5, 2013, 12:20 PM
 */

#ifndef INCL_COMM_ADAPTOR_LISTENER_H
#define INCL_COMM_ADAPTOR_LISTENER_H

#include "AdaptorProperties.h"

#include "DArray.h"

namespace IHMC_ACI
{
    struct MessageProperties;
    struct SearchProperties;

    class CommAdaptorListener
    {
        public:
            virtual ~CommAdaptorListener (void) {};
            virtual int contextUpdateMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                     const void *pBuf, uint32 ui32Len) = 0;
            virtual int contextVersionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                      const void *pBuf, uint32 ui32Len) = 0;
            virtual int dataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                                     const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks) = 0;
            virtual int metadataArrived (const AdaptorProperties *pAdaptorProperties, const MessageProperties *pProperties,
                                         const void *pBuf, uint32 ui32Len, const char *pszReferredDataId) = 0;

            virtual int messageRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                      const char *pszPublisherNodeId, const char *pszMsgId) = 0;
            virtual int chunkRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                    const char *pszPublisherNodeId, const char *pszMsgId,
                                                    NOMADSUtil::DArray<uint8> *pCachedChunks) = 0;

            virtual int positionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                const void *pBuf, uint32 ui32Len) = 0;
            virtual int searchMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                              SearchProperties *pSearchProperties) = 0;
            virtual int searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                   const char *pszQueryId, const char **ppszMatchingMsgIds,
                                                   const char *pszTarget, const char *pszMatchingNodeId) = 0;
            virtual int searchReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                   const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                                   const char *pszTarget, const char *pszMatchingNodeId) = 0;
            virtual int topologyReplyMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                     const void *pBuf, uint32 ui32Len) = 0;
            virtual int topologyRequestMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                       const void *pBuf, uint32 ui32Len) = 0;
            virtual int updateMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                              const char *pszPublisherNodeId,
                                              const void *pBuf, uint32 ui32Len) = 0;
            virtual int versionMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                               const char *pszPublisherNodeId,
                                               const void *pBuf, uint32 ui32Len) = 0;
            virtual int waypointMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                                const char *pszPublisherNodeId,
                                                const void *pBuf, uint32 ui32Len) = 0;
            virtual int wholeMessageArrived (AdaptorId uiAdaptorId, const char *pszSenderNodeId,
                                             const char *pszPublisherNodeId,
                                             const void *pBuf, uint32 ui32Len) = 0;

            virtual void newPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId,
                                  const char *pszPeerRemoteAddress,
                                  const char *pszIncomingInterface) = 0;
            virtual void deadPeer (const AdaptorProperties *pAdaptorProperties, const char *pszDstPeerId) = 0;
            virtual void newLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID,
                                        const char *pszPeerRemoteAddr, const char *pszIncomingInterface) = 0;
            virtual void droppedLinkToPeer (const AdaptorProperties *pAdaptorProperties, const char *pszNodeUID,
                                            const char *pszPeerRemoteAddr) = 0;
    };
}

#endif    /* INCL_COMM_ADAPTOR_LISTENER_H */
