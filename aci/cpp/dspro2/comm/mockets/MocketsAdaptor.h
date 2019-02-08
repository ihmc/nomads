/*
 * MocketsAdaptor.h
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
 * Created on August 19, 2012, 4:14 PM
 */

#ifndef INCL_MOCKETS_ADAPTOR_H
#define INCL_MOCKETS_ADAPTOR_H

#include "ConnCommAdaptor.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class MocketsAdaptor : public ConnCommAdaptor
    {
        public:
            static const unsigned short DEFAULT_PORT;

            MocketsAdaptor (AdaptorId uiId, const char *pszNodeId,
                            CommAdaptorListener *pListener, uint16 ui16Port);
            ~MocketsAdaptor (void);

            int init (NOMADSUtil::ConfigManager *pConfMgr);

            int disconnectFromPeer (const char *pszRemotePeerNodeId, const char *pszRemotePeerAddr);

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
            int sendMessageRequestMessage (const char *pszMsgId, const char *pszPublisherNodeId,
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
            const uint8 _ui16DefaultPriority;
            NOMADSUtil::String _mocketsCfgFile;
            NOMADSUtil::String _mocketsCert;
            NOMADSUtil::String _mocketsKey;
    };

    inline bool MocketsAdaptor::supportsManycast (void)
    {
        return false;
    }
}

#endif // INCL_MOCKETS_ADAPTOR_H
