/*
 * TransmissionServiceListener.h
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

#ifndef INCL_TRANSMISSION_SERVICE_LISTENER_H
#define INCL_TRANSMISSION_SERVICE_LISTENER_H

#include "NetworkMessageServiceListener.h"

#include "ListenerNotifier.h"

#include "LoggingMutex.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class DataCacheInterface;
    class DataMessageInterface;
    class DataRequestHandler;
    class DisseminationService;
    class DisServiceDataMsg;
    class DisServiceMsg;
    class LocalNodeInfo;
    class Message;
    class MessageHeader;
    class MessageReassembler;
    class NetworkTrafficMemory;
    class SubscriptionState;

    class TransmissionServiceListener : public NOMADSUtil::NetworkMessageServiceListener
    {
        public:
            TransmissionServiceListener (DisseminationService *pDisService, DataRequestHandler *pDataReqHandler,
                                         LocalNodeInfo *pLocalNodeInfo, MessageReassembler *pMsgReassembler,
                                         SubscriptionState *pSubState, NetworkTrafficMemory *pTrafficMemory,
                                         bool bOppListeningEnabled, bool bTargetFilteringEnabled);

            virtual ~TransmissionServiceListener (void);

            int deregisterAllMessageListeners (void);
            int deregisterMessageListener (unsigned int uiIndex);
            int registerMessageListener (MessageListener *pListener);

            virtual int messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                        uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                        const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp);

        private:
            void deliverCompleteMessage (Message *pMessage, bool bIsNotTarget);

            void evaluateAcknowledgment (MessageHeader *pMH);

            int messageArrivedInternal (const char *pszIncomingInterface, uint32 ui32SourceIPAddress,
                                        uint8 ui8MsgType, uint16 ui16MsgId, uint8 ui8HopCount,
                                        uint8 ui8TTL, const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp);
            
        private:
            const bool _bOppListeningEnabled;
            const bool _bTargetFilteringEnabled;
            const NOMADSUtil::String _nodeId;

            DataCacheInterface *_pDCI;
            DataRequestHandler *_pDataReqHandler;
            DisseminationService *_pDisService;
            LocalNodeInfo *_pLocalNodeInfo;
            MessageListenerNotifier _notifier;
            MessageReassembler *_pMsgReassembler;
            NetworkTrafficMemory *_pNetTrafficMemory;
            SubscriptionState *_pSubState;
            NOMADSUtil::LoggingMutex _m;
    };
}

#endif   // INCL_TRANSMISSION_SERVICE_LISTENER_H
