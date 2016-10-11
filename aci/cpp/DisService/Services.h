/*
 * Services.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Created on May 24, 2011, 1:02 PM
 */

#ifndef INCL_SERVICES_H
#define	INCL_SERVICES_H

#include "FTypes.h"
#include "DArray2.h"
#include "PtrLList.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class String;
    template <class T> class DArray2;
    template <class T> class PtrLList;
}

namespace IHMC_ACI
{
    class ControllerToControllerMsg;
    class DataCacheInterface;
    class DisseminationService;
    class DisServiceCtrlMsg;
    class DisServiceDataCacheQuery;
    class DisServiceDataMsg;
    class LocalNodeInfo;
    class MessageHeader;
    class PropertyStoreInterface;
    class PeerState;
    class RemoteNodeInfo;
    class TransmissionHistoryInterface;
    class TransmissionService;

    class ServiceBase
    {
        public:
            ServiceBase (DisseminationService *pDisService);
            virtual ~ServiceBase (void);

            DisseminationService * getDisService (void);

        private:
            DisseminationService *_pDisService;
    };

    class DataCacheService : public ServiceBase
    {
        public:
            DataCacheService (DisseminationService *pDisService);
            virtual ~DataCacheService (void);

            virtual bool isMessageInCache (const char *pszMsgId);
            virtual int deleteMessage (const char *pszMsgId);

            /**
             * Perform a query on the data cache using the specified parameters.
             * Specifying a NULL (for a const char*) or a 0 (for the other types) indicates a wild card.
             * The returned value is a newly allocated linked list of matching MessageInfo.
             *
             * NOTE: The caller must call releaseQueryResults() when done. Until this method has been invoked, the 
             * DataCache is locked to prevent any race conditions, which implies that any operations (e.g. push()) 
             * that needs to change the data cache will be blocked.
             */
            virtual NOMADSUtil::PtrLList<MessageHeader> * lockAndQueryDataCache (const char *pszSQLStatement);
            virtual NOMADSUtil::PtrLList<MessageHeader> * lockAndQueryDataCache (DisServiceDataCacheQuery *pQuery);

            /**
             * Get all the node IDs of all the nodes for which messages that
             * belong to pszGroupName have been received.
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getSenderNodeIds (const char *pszGroupName);

            /**
             * Get the complete MessageHeader object that describes the data
             * message identified by pszId. 
             */
            virtual MessageHeader * getMessageInfo (const char *pszId);

            /**
             * Returns all the messages with expired expiration time 
             */
            virtual NOMADSUtil::DArray2<NOMADSUtil::String> * getExpiredEntries (void);

            virtual void releaseQueryResults (void);
            virtual void releaseQueryResults (MessageHeader *pMI);
            virtual void releaseQueryResults (NOMADSUtil::PtrLList<MessageHeader> *pMessageList);

            DataCacheInterface * getDataCacheInterface (void);

        private:
            DataCacheInterface *_pDataCacheInterface;
            LocalNodeInfo *_pLocalNodeInfo;
            const char *_pszNodeName;
    };

    class GroupMembershipService : public ServiceBase
    {
        public:
            GroupMembershipService (DisseminationService *pDisService);
            virtual ~GroupMembershipService (void);

            /**
             * Returns all the subscriptions currently present on the local
             * nodes. 
             */
            NOMADSUtil::PtrLList<NOMADSUtil::String> * getAllLocalSubscriptions (void);

        private:
            LocalNodeInfo *_pLocalNodeInfo;
            PeerState *_pPeerState;
    };

    class MessagingService : public ServiceBase
    {
        public:
            static const char * DEFAULT_LOG_MSG;

            MessagingService (DisseminationService *pDisService);
            virtual ~MessagingService (void);

            TransmissionHistoryInterface * getTrasmissionHistoryInterface (void);
            TransmissionService * getTrasmissionService (void);

            // Checks whether it is clear to send on the specified interface
            bool clearToSend (const char *pszInterface);

            // Checks whether it is clear to send on all outgoing interfaces
            bool clearToSendOnAllInterfaces (void);

            virtual int broadcastCtrlMessage (DisServiceCtrlMsg *pCtrlMsg,
                                              const char **ppszOutgoingInterfaces,
                                              const char *pszLogMsg = DEFAULT_LOG_MSG,
                                              const char *pszTargetAddr = NULL,
                                              const char *pszHints = NULL);

            virtual int broadcastCtrlMessage (DisServiceCtrlMsg *pCtrlMsg,
                                              const char *pszLogMsg = DEFAULT_LOG_MSG,
                                              const char *pszTargetAddr = NULL,
                                              const char *pszHints = NULL);

            // 1 Message to multiple targets
            virtual int broadcastDataMessage (const char *pszMessageID, const char **ppszTargetNodeIds,
                                              int64 i64TimeOut, uint8 *pUi8Priorities, bool bRequireAck,
                                              const char *pszLogMsg = DEFAULT_LOG_MSG,
                                              const char *pszHints = NULL);

            // Multiple messages to 1 target
            virtual int broadcastDataMessage (const char **ppszMessageIDs, const char *pszTargetNodeId,
                                              int64 i64TimeOut, uint8 *pUi8Priorities, bool bRequireAck,
                                              const char *pszLogMsg = DEFAULT_LOG_MSG,
                                              const char *pszHints = NULL);

            // 1 Message to 1 target
            virtual int broadcastDataMessage (const char *pszMessageID, const char *pszTargetNodeIds,
                                              int64 i64TimeOut, uint8 ui8Priority, bool bRequireAck,
                                              const char *pszLogMsg = DEFAULT_LOG_MSG,
                                              const char *pszHints = NULL);

            virtual int broadcastDataMessage (MessageHeader *pMI, const char *pszTargetNodeId,
                                              int64 i64TimeOut, const char *pszLogMsg = DEFAULT_LOG_MSG,
                                              const char *pszHints = NULL);

            virtual int broadcastDataMessage (DisServiceDataMsg *pDSMsg, const char *pszLogMsg = DEFAULT_LOG_MSG,
                                              const char **pszOutgoingInterfaces = NULL, const char *pszTargetAddr = NULL,
                                              const char *pszHints = NULL);

            // Unicast control message
            virtual int transmitCtrlMessage (DisServiceCtrlMsg *pCtrlMsg,
                                             const char *pszLogMsg = DEFAULT_LOG_MSG);

            // Unicast controller to controller message
            virtual int transmitCtrlToCtrlMessage (ControllerToControllerMsg *pCtrlMsg,
                                                   const char *pszLogMsg = DEFAULT_LOG_MSG);

            virtual int transmitUnreliableCtrlToCtrlMessage (ControllerToControllerMsg *pCtrlMsg,
                                                             const char *pszLogMsg = DEFAULT_LOG_MSG);

        private:
            DataCacheInterface *_pDataCacheInterface;
    };

    class TopologyService : public ServiceBase
    {
        public:
            TopologyService (DisseminationService *pDisService);
            virtual ~TopologyService (void);

            RemoteNodeInfo * getNeighborNodeInfo (const char *pszRemoteNodeId);

        private:
            PeerState *_pPeerState;
    };

    class ChunkService : public ServiceBase
    {
        public:
            ChunkService (DisseminationService *pDisService);
            virtual ~ChunkService (void);
    };

    class SearchService : public ServiceBase
    {
        public:
            SearchService (DisseminationService *pDisService);
            virtual ~SearchService (void);

            static char * getSearchId (const char *pszGroupName, const char *pszPublisherNodeId,
                                       PropertyStoreInterface *pPropertyStore);
            bool messageNotified (const char *pszMessageId, uint16 ui16ClientId, const char *pszQueryId);
            void notifyClients (const char *pszMessageId, const char *pszSearchId, uint16 ui16ClientId);
            void setNotifiedClients (const char *pszMessageId, const char *pszSearchId, uint16 ui16ClientId);

        private:
            class NotifiedSearches
            {
                public:
                    NotifiedSearches (const char *pszNodeId, PropertyStoreInterface *pPropertyStore);
                    ~NotifiedSearches (void);

                    void put (const char *pszQueryId, uint16 ui16ClientId, const char *pszMessageId);
                    bool messageNotified (const char *pszQueryId, uint16 ui16ClientId, const char *pszMessageId);

                private:
                    NOMADSUtil::String getAttribute (const char *pszQueryId, uint16 ui16ClientId, const char *pszMessageId);

                private:
                    static const char * PROPERTY_NAME;
                    static const char * NOTIFIED;
                    const NOMADSUtil::String _nodeId;
                    PropertyStoreInterface *_pPropertyStore;

            };

            NotifiedSearches _notifiedSearches;
    };

    inline DisseminationService * ServiceBase::getDisService (void)
    {
        return _pDisService;
    }

}

#endif	// INCL_SERVICES_H

