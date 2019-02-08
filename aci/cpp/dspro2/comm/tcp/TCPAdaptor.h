/*
 * TCPAdaptor.h
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.y.
 * Contact IHMC for other types of licenses.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 14, 2014, 12:04 PM
 */

#ifndef INCL_TCP_ADAPTOR_H
#define INCL_TCP_ADAPTOR_H

#include "ConnCommAdaptor.h"

#include "CommAdaptor.h"

namespace IHMC_ACI
{
    class TCPAdaptor : public ConnCommAdaptor
    {
        public:
            static const unsigned short DEFAULT_PORT;

            TCPAdaptor (AdaptorId uiId, const char *pszNodeId,
                        CommAdaptorListener *pListener, uint16 ui16Port);
            virtual ~TCPAdaptor (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);
            void resetTransmissionCounters (void);
            bool supportsManycast (void);

            int sendContextUpdateMessage (const void *pBuf, uint32 ui32Len,
                                          const char **ppszRecipientNodeIds,
                                          const char **ppszInterfaces);
            int sendContextVersionMessage (const void *pBuf, uint32 ui32Len,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces);
            int sendDataMessage (Message *pMsg, const char **ppszRecipientNodeIds,
                                 const char **ppszInterfaces);
            int sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
                                    const char **ppszRecipientNodeIds,
                                    const char **ppszInterfaces);
            int sendMessageRequestMessage (const char *pszMsgId,
                                           const char *pszPublisherNodeId,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces);
            int sendChunkRequestMessage (const char *pszMsgId,
                                         NOMADSUtil::DArray<uint8> *pCachedChunks,
                                         const char *pszPublisherNodeId,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces);
            int sendPositionMessage (const void *pBuf, uint32 ui32Len,
                                     const char **ppszRecipientNodeIds,
                                     const char **ppszInterfaces);
            int sendSearchMessage (SearchProperties &searchProp,
                                   const char **ppszRecipientNodeIds,
                                   const char **ppszInterfaces);
            int sendSearchReplyMessage (const char *pszQueryId,
                                        const char **ppszMatchingMsgIds,
                                        const char *pszTarget,
                                        const char *pszMatchingNode,
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
            int sendUpdateMessage (const void *pBuf, uint32 ui32Len,
                                   const char *pszPublisherNodeId,
                                   const char **ppszRecipientNodeIds,
                                   const char **ppszInterfaces);
            int sendVersionMessage (const void *pBuf, uint32 ui32Len,
                                    const char *pszPublisherNodeId,
                                    const char **ppszRecipientNodeIds,
                                    const char **ppszInterfaces);
            int sendWaypointMessage (const void *pBuf, uint32 ui32Len,
                                     const char *pszPublisherNodeId,
                                     const char **ppszRecipientNodeIds,
                                     const char **ppszInterfaces);
            int sendWholeMessage (const void *pBuf, uint32 ui32Len,
                                  const char *pszPublisherNodeId,
                                  const char **ppszRecipientNodeIds,
                                  const char **ppszInterfaces);
            int notifyEvent (const void *pBuf, uint32 ui32Len,
                             const char *pszPublisherNodeId,
                             const char *pszTopic, const char **ppszInterfaces);

        protected:
            int connectToPeerInternal (const char *pszRemotePeerAddr, uint16 ui16Port);
            ConnListener * getConnListener (const char *pszListenAddr, uint16 ui16Port,
                                            const char *pszNodeId, const char *pszSessionId,
                                            CommAdaptorListener *pListener,
                                            ConnCommAdaptor *pCommAdaptor);

        private:
            const uint8 _ui8DefaultPriority;
    };
}

#endif    /* INCL_TCP_ADAPTOR_H */
