/* 
 * NetworkMessageServiceProxy.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Created on February 27, 2015, 5:00 PM
 */

#ifndef INCL_NETWORK_MESSAGE_SERVICE_PROXY_H
#define	INCL_NETWORK_MESSAGE_SERVICE_PROXY_H

#include "Stub.h"

#include "NetworkMessageServiceInterface.h"
#include "NetworkMessageServiceListener.h"

namespace NOMADSUtil
{
    class NetworkMessageServiceProxy : public Stub, public NetworkMessageServiceInterface
    {
        public:
            explicit NetworkMessageServiceProxy (uint16 ui16DesiredApplicationId, bool bUseBackgroundReconnect = false);
            virtual ~NetworkMessageServiceProxy (void);

            // Methods
            int broadcastMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces,
                                  uint32 ui32BroadcastAddress, uint16 ui16MsgId,
                                  uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                  const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                  const void *pMsg, uint16 ui16MsgLen, bool bExpedited,
                                  const char *pszHints = NULL);

            int transmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                 uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                 const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg,
                                 uint16 ui16MsgLen, const char *pszHints = NULL);

            int transmitReliableMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                         uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                         const char *pszHints = NULL);

            int setRetransmissionTimeout (uint32 ui32Timeout);
            int setPrimaryInterface (const char *pszInterfaceAddr);
            int startSvc (void);
            int stopSvc (void);

            uint16 getMinMTU (void);
            char ** getActiveNICsInfoAsString (void);
            char ** getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination);
            char ** getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr);
            PROPAGATION_MODE getPropagationMode (void);
            uint32 getDeliveryQueueSize (void);

            int64 getReceiveRate (const char *pszAddr);
            uint32 getTransmissionQueueSize (const char *pszOutgoingInterface);
            uint8 getRescaledTransmissionQueueSize (const char *pszOutgoingInterface);
            uint32 getTransmissionQueueMaxSize (const char *pszOutgoingInterface);
            uint32 getTransmitRateLimit (const char *pszInterface);
            int setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize);
            int setTransmitRateLimit (const char *pszInterface, const char * pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszInterface, uint32 ui32DestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (uint32 ui32RateLimit);
            uint32 getLinkCapacity (const char *pszInterface);
            void setLinkCapacity (const char *pszInterface, uint32 ui32Capacity);
            uint8 getNeighborQueueLength (const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);
            bool clearToSend (const char *pszInterface);
            bool clearToSendOnAllInterfaces (void);

            int ping (void);

            // Callbacks
            int messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp);
            int pong (void);

            // Callback registration
            int registerHandlerCallback(uint8 ui8MsgType, NetworkMessageServiceListener *pListener);
            int deregisterHandlerCallback(uint8 ui8MsgType, NetworkMessageServiceListener *pListener);

        protected:
            int reregisterListeners (void);

        private:
            NetworkMessageServiceListener *_pListener;
    };
}

#endif	/* INCL_NETWORK_MESSAGE_SERVICE_PROXY_H */

