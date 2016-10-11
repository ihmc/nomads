/**
 * CommAdaptorManager.h
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
 */

#ifndef INCL_COMM_ADAPTOR_MANAGER_H
#define INCL_COMM_ADAPTOR_MANAGER_H

#include "CommAdaptor.h"
#include "CommAdaptorListenerNotifier.h"

#include "LoggingMutex.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class Controller;
    class MetaData;
    class Targets;
    class PropertyStoreInterface;

    class CommAdaptorManager
    {
        public:
            explicit CommAdaptorManager (const char *pszNodeId);
            virtual ~CommAdaptorManager (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr, const char *pszSessionId, Controller *pController, PropertyStoreInterface *pPropertyStore);

            CommAdaptor * getAdaptorByType (AdaptorType type);
            NOMADSUtil::String getSessionId (void) const;

            /**
             * Returns 0 if the listener was correctly registered, a negative
             * number otherwise.
             */
            int registerCommAdaptorListener (CommAdaptorListener *pListner, unsigned int &uiListenerId);
            int deregisterCommAdaptorListener (unsigned int uiListenerId);

            int startAdaptors (void);
            int stopAdaptors (void);

            AdaptorId addAdptor (void);
            int connectToPeer (AdaptorType type, const char *pszRemoteAddr, uint16 ui16Port);

            void resetTransmissionCounters (void);

            int sendContextUpdateMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets);
            int sendContextVersionMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets);
            int sendDataMessage (Message *pMsg, Targets **ppTargets);
            int sendChunkedMessage (Message *pMsg, const char *pszDataMimeType, Targets **ppTargets);
            int sendMessageRequestMessage (const char *pszMsgId, const char *pszPublisherNodeId, Targets **ppTargets);
            int sendChunkRequestMessage (const char *pszMsgId, NOMADSUtil::DArray<uint8> *pCachedChunks,
                                         const char *pszPublisherNodeId, Targets **ppTargets);
            int sendPositionMessage(const void *pBuf, uint32 ui32Len, Targets **ppTargets);
            int sendSearchMessage (SearchProperties &searchProp, Targets **ppTargets);
            int sendSearchReplyMessage (const char *pszQueryId, const char **ppszMatchingMsgIds,
                                        const char *pszTarget,  const char *pszMatchingNode,
                                        Targets **ppTargets);
            int sendSearchReplyMessage (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                        const char *pszTarget, const char *pszMatchingNode,
                                        Targets **ppTargets);
            int sendTopologyReplyMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets);
            int sendTopologyRequestMessage (const void *pBuf, uint32 ui32Len, Targets **ppTargets);
            int sendUpdateMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets);
            int sendVersionMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets);
            int sendWaypointMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets);
            int sendWholeMessage (const void *pBuf, uint32 ui32Len, const char *pszPublisherNodeId, Targets **ppTargets);

            bool adaptorSupportsCaching (AdaptorId adaptorId);
            AdaptorType getAdaptorType (AdaptorId adaptorId);
            int getDisServiceAdaptorId (AdaptorId &adaptorId);

            bool hasAdaptor (AdaptorId uiId);

        private:
            static const AdaptorId DISSERVICE_ADAPTOR_ID;

            struct AdaptorWrapper
            {
                unsigned int uiId;
                CommAdaptor *pAdaptor;
            };

            mutable NOMADSUtil::Mutex _mSessionId;
            mutable NOMADSUtil::LoggingMutex _m;
            const NOMADSUtil::String _nodeId;
            NOMADSUtil::String _sessionId;
            NOMADSUtil::DArray2<AdaptorWrapper> _adaptors;
            CommAdaptorListenerNotifier _commListenerNotifier;
            PropertyStoreInterface *_pPropertyStore;
    };

    inline bool CommAdaptorManager::hasAdaptor (AdaptorId adaptorId)
    {
        long lHighestIndex = _adaptors.getHighestIndex();
        if (lHighestIndex < 0) {
            return false;
        }
        if (adaptorId > static_cast<unsigned int>(lHighestIndex)) {
            return false;
        }
        return _adaptors.used (adaptorId) == 1;
    }
}

#endif  // INCL_COMM_ADAPTOR_MANAGER_H
