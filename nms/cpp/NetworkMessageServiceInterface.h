/*
 * NetworkMessageServiceInterface.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Created on June 22, 2015, 4:24 PM
 */

#ifndef INCL_NETWORK_MESSAGE_SERVICE_INTERFACE_H
#define    INCL_NETWORK_MESSAGE_SERVICE_INTERFACE_H

#include "FTypes.h"
#include <stddef.h>

namespace NOMADSUtil
{
    enum PROPAGATION_MODE
    {
        BROADCAST = 0x00,
        MULTICAST = 0x01,
        NORM = 0x02
    };

    class NetworkMessageServiceListener;

    class NetworkMessageServiceInterface
    {
        public:
            NetworkMessageServiceInterface (void) {}
            virtual ~NetworkMessageServiceInterface (void) {}
            virtual int broadcastMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces,
                                          uint32 ui32BroadcastAddress, uint16 ui16MsgId,
                                          uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                          const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                          const void *pMsg, uint16 ui16MsgLen, bool bExpedited,
                                          const char *pszHints = NULL) = 0;

            virtual int transmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                         uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg,
                                         uint16 ui16MsgLen, const char *pszHints = NULL) = 0;

            virtual int transmitReliableMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                                 uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                                 const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                                 const char *pszHints = NULL) = 0;

            virtual int changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len) = 0;

            virtual int setRetransmissionTimeout (uint32 ui32Timeout) = 0;

            virtual int setPrimaryInterface (const char *pszInterfaceAddr) = 0;

            virtual uint16 getMinMTU (void) = 0;

            virtual char ** getActiveNICsInfoAsString (void) = 0;

            virtual char ** getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination) = 0;

            virtual char ** getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr) = 0;

            virtual PROPAGATION_MODE getPropagationMode (void) = 0;

            virtual uint32 getDeliveryQueueSize (void) = 0;

            virtual int64 getReceiveRate (const char *pszAddr) = 0;

            virtual uint32 getTransmissionQueueSize (const char *pszOutgoingInterface) = 0;

            virtual uint8 getRescaledTransmissionQueueSize (const char *pszOutgoingInterface) = 0;

            virtual uint32 getTransmissionQueueMaxSize (const char *pszOutgoingInterface) = 0;

            virtual uint32 getTransmitRateLimit (const char *pszInterface) = 0;

            virtual int setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize) = 0;

            virtual int setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress, uint32 ui32RateLimit) = 0;

            virtual int setTransmitRateLimit (const char *pszInterface, uint32 ui32DestinationAddress, uint32 ui32RateLimit) = 0;

            virtual int setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit) = 0;

            virtual int setTransmitRateLimit (uint32 ui32RateLimit) = 0;

            virtual uint32 getLinkCapacity (const char *pszInterface) = 0;

            virtual void setLinkCapacity (const char *pszInterface, uint32 ui32Capacity) = 0;

            virtual uint8 getNeighborQueueLength (const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr) = 0;

            virtual bool clearToSend (const char *pszInterface) = 0;

            virtual bool clearToSendOnAllInterfaces (void) = 0;
            // Callback registration
            virtual int registerHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener) = 0;

            virtual int deregisterHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener) = 0;
    };
}

#endif    /* INCL_NETWORK_MESSAGE_SERVICE_INTERFACE_H */

