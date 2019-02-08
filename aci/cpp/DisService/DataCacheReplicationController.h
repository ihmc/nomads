/*
 * DataCacheReplicationController.h
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
 */

#ifndef INCL_DATA_CACHE_REPLICATION_CONTROLLER_H
#define INCL_DATA_CACHE_REPLICATION_CONTROLLER_H

#include "Listener.h"
#include "Services.h"

#include "AckController.h"

#include "LoggingMutex.h"

namespace IHMC_ACI
{
    class DataCacheInterface;
    class DisseminationService;
    class DisseminationServiceProxyServer;
    class DisServiceCtrlMsg;
    class DisServiceDataCacheQuery;
    class DisServiceDataMsg;
    class MessageInfo;
    class MessageHeader;
    class ControllerToControllerMsg;

    class DataCacheReplicationController : public DataCacheService, public MessagingService,
                                           public DataCacheListener, public MessageListener, public PeerStateListener
    {
        public:
            enum Type {
                DCRC_Default = 0x00,
                DCRC_Push = 0x01,
                DCRC_Pull = 0x02,
                DCRC_PushToConvoy = 0x03,
                DCRC_BandwidthSensitive = 0x04,
                DCRC_DSPro = 0x05,
                DCRC_TargetBased = 0x06
            };

            static const bool DEFAULT_REQUIRE_ACK;

            DataCacheReplicationController (Type type, DisseminationService *pDisService,
                                            bool bRequireAck=DEFAULT_REQUIRE_ACK);
            DataCacheReplicationController (Type type, DisseminationServiceProxyServer *pDisServiceProxy,
                                            bool bRequireAck=DEFAULT_REQUIRE_ACK);
            virtual ~DataCacheReplicationController (void);

            // DisService -> Cache Controller

            virtual void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                             DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                             const char *pszIncomingInterface);

            /**
             * - pPayLoad: it may be either a data or a metadata describing
             *             a data published by makeAvailable or a metadata
             *             describing the data contained in the same payload.
             *
             * - bIsTarget: true if the node was NOT the target of the message,
             *              false otherwise.
             */
            virtual void dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad) = 0;

            virtual void capacityReached (void);
            virtual void thresholdCapacityReached (uint32 ui32Length);
            virtual void spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo, void *pIncomingData);
            virtual int cacheCleanCycle (void);

            NOMADSUtil::PtrLList<MessageHeader> * lockAndQueryDataCache (const char *pszSQLStatement);
            NOMADSUtil::PtrLList<MessageHeader> * lockAndQueryDataCache (DisServiceDataCacheQuery *pQuery);

            void releaseQueryResults (void);
            void releaseQueryResults (NOMADSUtil::PtrLList<MessageHeader> *pMessageList);

            void lockDataCache (void);
            void releaseDataCache (void);

            uint8 getType (void);

            // Acknowledgement
            bool getAck (void);

            /**
             * Replicate messages (each with its priority value) on one node.
             */
            int replicateMessages (const char **pszMessageIDs, const char *pReplicateOnNodeId,
                                   int64 i64TimeOut, uint8 *ui8Priority);

            /**
             * Replicate one message on different receivers. The message has different priorities
             * for each receiver.
             */
            int replicateMessage (const char *pszMessageID, const char **pReplicateOnNodeIds,
                                  int64 i64TimeOut, uint8 *ui8Priority);

            /**
             * Replicate the specified message to the specified receiver
             */
            int replicateMessage (const char *pszMessageId, const char *pszReplicationTargetNodeId, int64 i64TimeOut);

            int replicateMessage (MessageHeader *pMI, const char *pszReplicateOnNodeId, int64 i64TimeOut);

        protected:
            virtual void disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg) = 0;
            virtual void disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg) = 0;
            virtual void disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg) = 0;

        protected:
            bool _bRequireAck;
            bool _bReplicateDataCacheQueryMsg;
            Type _type;
            DataCacheInterface *_pDataCacheInterface;

            AckController *_pAck;
            NOMADSUtil::LoggingMutex _mx;
    };

    inline bool DataCacheReplicationController::getAck (void)
    {
        return _bRequireAck;
    }

    inline uint8 DataCacheReplicationController::getType (void)
    {
        return _type;
    }
}

#endif   // #ifndef INCL_DATA_CACHE_REPLICATION_CONTROLLER_H
