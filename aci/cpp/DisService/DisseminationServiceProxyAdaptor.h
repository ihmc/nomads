/*
 * DisseminationServiceProxyAdaptor.h
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
 */

#ifndef DISSEMINATION_SERVICE_PROXY_ADAPTOR_H
#define DISSEMINATION_SERVICE_PROXY_ADAPTOR_H

#include "DisseminationServiceListener.h"
#include "PeerStatusListener.h"

#include "InetAddr.h"
#include "LoggingMutex.h"
#include "ManageableThread.h"
#include "SimpleCommHelper2.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class Logger;
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class DisseminationService;
    class DisseminationServiceProxyServer;

    typedef NOMADSUtil::SimpleCommHelper2::Error CommHelperError;

    class DisseminationServiceProxyProtocolHelper
    {
        public:
            
            DisseminationServiceProxyProtocolHelper (void);
            ~DisseminationServiceProxyProtocolHelper (void);

            void setCommHelper (NOMADSUtil::SimpleCommHelper2 *pCommHelper);
            void setCbackCommHelper (NOMADSUtil::SimpleCommHelper2 *pCommHelper);

            CommHelperError readUI32Blob (void *&pBuf, uint32 &uiBufLen);
            CommHelperError readUI32String (NOMADSUtil::String &str);
            CommHelperError readIds (NOMADSUtil::String &objectId, NOMADSUtil::String &instanceId);
            CommHelperError readGroupIdsMIMEType (NOMADSUtil::String &grpName, NOMADSUtil::String &objectId,
                                                  NOMADSUtil::String &instanceId, NOMADSUtil::String &mimeType,
                                                  bool bReadMIMEType = true);
            CommHelperError readSubscription (NOMADSUtil::String &grpName, uint8 &ui8Priority);
            CommHelperError readSubscriptionParameters (bool &bGroupReliable, bool &bMsgReliable, bool &bSequenced);

            CommHelperError writeUI32Blob (const void *pBuf, uint32 uiBufLen);

            static const uint32 BUF_LEN = 1024;
            char _buf[BUF_LEN];
        private:
            NOMADSUtil::SimpleCommHelper2 *_pCommHelper;
            NOMADSUtil::SimpleCommHelper2 *_pCallbackCommHelper;
    };

    class DisseminationServiceProxyAdaptor : public NOMADSUtil::ManageableThread, public DisseminationServiceListener,
                                             public PeerStatusListener, public SearchListener
    {
        public:
            DisseminationServiceProxyAdaptor(DisseminationServiceProxyServer *pDSProxyServer);
            virtual ~DisseminationServiceProxyAdaptor (void);

            int init (NOMADSUtil::SimpleCommHelper2 *pCommHelper, uint16 ui16ClientID);

            void packetReceived (const char* pData, uint16 dataLen, NOMADSUtil::InetAddr fromAddr,
                                 NOMADSUtil::InetAddr toAddr);

            void setCallbackCommHelper (NOMADSUtil::SimpleCommHelper2 *pCommHelper);
            void run (void);

            uint16 getClientID (void);

            bool dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                              const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType, const void *pData,
                              uint32 ui32Length, uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority,
                              const char *pszQueryId);
            bool chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                               const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType, const void *pChunk,
                               uint32 ui32Length, uint8 ui8NChunks, uint8 ui8TotNChunks, const char *pszChunkedMsgId,
                               uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);
            bool metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                  const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType, const void *pMetadata,
                                  uint32 ui32MetadataLength, bool bDataChunked, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);
            bool dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType, const char *pszId,
                                const void *pMetadata, uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

            bool newPeer (const char *pszPeerId);
            bool deadPeer (const char *pszPeerId);

            void searchArrived (const char *pszQueryId, const char *pszGroupName,
                                const char *pszQuerier, const char *pszQueryType,
                                const char *pszQueryQualifiers,
                                const void *pszQuery, unsigned int uiQueryLen);
            void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds, const char *pszMatchingNodeId);

        protected:
            bool doGetNodeId (CommHelperError &err);
            bool doGetPeerList (CommHelperError &err);
            bool doGetDisServiceIds (CommHelperError &err);
            bool doPushById (CommHelperError &err);

            enum PublishOption {
                PUSH,
                STORE,
                MAKE_AVAILABLE
            };
            bool doPushOrStore (CommHelperError &err, PublishOption pubOpt);
            bool doMakeAvailableFile (CommHelperError &err);
            bool doCancel_int (CommHelperError &err);
            bool doCancel_psz (CommHelperError &err);
            bool doSubscribe (CommHelperError &err);
            bool doSubscribe_tag (CommHelperError &err);
            bool doSubscribe_predicate (CommHelperError &err);
            bool doUnsubscribe (CommHelperError &err);
            bool doUnsubscribe_tag (CommHelperError &err);

            bool doAddFilter (CommHelperError &err);
            bool doRemoveFilter (CommHelperError &err);

            bool doRetrieve (CommHelperError &err);
            bool doRetrieve_file (CommHelperError &err);

            bool doHistoryRequest (CommHelperError &err);
            bool doRequestMoreChunks (CommHelperError &err);
            bool doRequestMoreChunksByID (CommHelperError &err);

            bool doRegisterDataArrivedCallback (CommHelperError &err);
            bool doRegisterChunkArrivedCallback (CommHelperError &err);
            bool doRegisterMetadataArrivedCallback (CommHelperError &err);
            bool doRegisterDataAvailableCallback (CommHelperError &err);
            bool doRegisterPeerStatusCallback (CommHelperError &err);
            bool doRegisterSearchListener (CommHelperError &err);
            bool doResetTransmissionHistory (CommHelperError &err);

            bool doGetNextPushId (CommHelperError &err);
            bool doSearch (CommHelperError &err);
            bool doSearchReply (CommHelperError &err);

            CommHelperError messageArrivedCallback (const char *pszCallbackId, const char *pszOriginator,
                                                    const char *pszGroupName, uint32 ui32SeqId,
                                                    const char *pszObjectId, const char *pszInstanceId, const char *pszMimeType);

        protected:
            uint16 _ui16ClientID;
            NOMADSUtil::SimpleCommHelper2 *_pCommHelper;
            NOMADSUtil::SimpleCommHelper2 *_pCallbackCommHelper;
            NOMADSUtil::LoggingMutex _mutex;

        private:
            bool _bListenerRegistered;
            bool _bPeerStatusListenerRegistered;
            bool _bSearchListenerRegistered;
            unsigned int _uiSearchListenerIndex;
            DisseminationServiceProxyServer *_pDissSvcProxyServer;
            DisseminationService *_pDissSvc;
            DisseminationServiceProxyProtocolHelper _adptProtHelper;
    };
}

#endif //DISSEMINATION_SERVICE_PROXY_ADAPTOR_H
