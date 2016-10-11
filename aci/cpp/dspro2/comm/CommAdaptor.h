/*
 * CommAdaptor.h
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

#ifndef INCL_COMM_ADAPTOR_H
#define	INCL_COMM_ADAPTOR_H

#include "AdaptorProperties.h"
#include "CommAdaptorListener.h"

#include "StrClass.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class Message;
    class CommAdaptorListenerNotifier;

    class CommAdaptor
    {
        public:
            virtual ~CommAdaptor (void);

            virtual int init (NOMADSUtil::ConfigManager *pCfgMgr) = 0;
            virtual int startAdaptor (void) = 0;
            virtual int stopAdaptor (void) = 0;

            unsigned int getAdaptorId (void) const;
            const AdaptorProperties * getAdaptorProperties (void) const;
            AdaptorType getAdaptorType (void) const;
            const char * getAdaptorAsString (void);
            NOMADSUtil::String getNodeId (void) const;
            NOMADSUtil::String getSessionId (void) const;

            virtual void resetTransmissionCounters (void) = 0;

            bool supportsCaching (void);
            bool supportsDirectConnection (void);
            virtual bool supportsManycast (void) = 0;

            virtual int sendContextUpdateMessage (const void *pBuf, uint32 ui32Len,
                                                  const char **ppszRecipientNodeIds,
                                                  const char **ppszInterfaces) = 0;
            virtual int sendContextVersionMessage (const void *pBuf, uint32 ui32Len,
                                                   const char **ppszRecipientNodeIds,
                                                   const char **ppszInterfaces) = 0;
            virtual int sendDataMessage (Message *pMsg,
                                         const char **ppszRecipientNodeIds,
                                         const char **ppszInterfaces) = 0;
            virtual int sendChunkedMessage (Message *pMsg, const char *pszDataMimeType,
                                            const char **ppszRecipientNodeIds,
                                            const char **ppszInterfaces) = 0;
            virtual int sendMessageRequestMessage (const char *pszMsgId,
                                                   const char *pszPublisherNodeId,
                                                   const char **ppszRecipientNodeIds,
                                                   const char **ppszInterfaces) = 0;
            virtual int sendChunkRequestMessage (const char *pszMsgId,
                                                 NOMADSUtil::DArray<uint8> *pCachedChunks,
                                                 const char *pszPublisherNodeId,
                                                 const char **ppszRecipientNodeIds,
                                                 const char **ppszInterfaces) = 0;
            virtual int sendPositionMessage (const void *pBuf, uint32 ui32Len,
                                             const char **ppszRecipientNodeIds,
                                             const char **ppszInterfaces) = 0;
            virtual int sendSearchMessage (SearchProperties &searchProp,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces) = 0;
            virtual int sendSearchReplyMessage (const char *pszQueryId,
                                                const char **ppszMatchingMsgIds,
                                                const char *pszTarget,
                                                const char *pszMatchingNode,
                                                const char **ppszRecipientNodeIds,
                                                const char **ppszInterfaces) = 0;
            virtual int sendVolatileSearchReplyMessage (const char *pszQueryId,
                                                        const void *pReply, uint16 ui16ReplyLen,
                                                        const char *pszTarget,
                                                        const char *pszMatchingNode,
                                                        const char **ppszRecipientNodeIds,
                                                        const char **ppszInterfaces) = 0;
            virtual int sendTopologyReplyMessage (const void *pBuf, uint32 ui32BufLen,
                                                  const char **ppszRecipientNodeIds,
                                                  const char **ppszInterfaces) = 0;
            virtual int sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len,
                                                    const char **ppszRecipientNodeIds,
                                                    const char **ppszInterfaces) = 0;
            virtual int sendUpdateMessage (const void *pBuf, uint32 ui32Len,
                                           const char *pszPublisherNodeId,
                                           const char **ppszRecipientNodeIds,
                                           const char **ppszInterfaces) = 0;
            virtual int sendVersionMessage (const void *pBuf, uint32 ui32Len,
                                            const char *pszPublisherNodeId,
                                            const char **ppszRecipientNodeIds,
                                            const char **ppszInterfaces) = 0;
            virtual int sendWaypointMessage (const void *pBuf, uint32 ui32Len,
                                             const char *pszPublisherNodeId,
                                             const char **ppszRecipientNodeIds,
                                             const char **ppszInterfaces) = 0;
            virtual int sendWholeMessage (const void *pBuf, uint32 ui32Len,
                                          const char *pszPublisherNodeId,
                                          const char **ppszRecipientNodeIds,
                                          const char **ppszInterfaces) = 0;
 
        protected:
            CommAdaptor (AdaptorId uiId, AdaptorType adaptorType, bool bSupportsCaching,
                         bool bSupportsDirectConnection, const char *pszNodeId, const char *pszSessionId,
                         CommAdaptorListener *pListener);

        protected:
            CommAdaptorListener *_pListener;
            const NOMADSUtil::String _nodeId;
            const NOMADSUtil::String _sessionId;
            const AdaptorProperties _adptorProperties;
    };

    inline unsigned int CommAdaptor::getAdaptorId() const
    {
        return _adptorProperties.uiAdaptorId;
    }

    inline const AdaptorProperties * CommAdaptor::getAdaptorProperties (void) const
    {
        return &_adptorProperties;
    }

    inline AdaptorType CommAdaptor::getAdaptorType() const
    {
        return _adptorProperties.uiAdaptorType;
    }

    inline bool CommAdaptor::supportsCaching (void)
    {
        return _adptorProperties.bSupportsCaching;
    }
}

#endif	/* INCL_COMM_ADAPTOR_H */

