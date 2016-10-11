/*
 * NetworkMessageService.h
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
 */

#ifndef INCL_NETWORK_MESSAGE_SERVICE_H
#define INCL_NETWORK_MESSAGE_SERVICE_H

#include "Mutex.h"
#include "NetworkMessageServiceInterface.h"

#include <stddef.h>

#define NMS_BROADCAST_ADDRESS "255.255.255.255"
#define NMS_MULTICAST_ADDRESS "239.0.0.239"

namespace NOMADSUtil
{
    class ConfigManager;
    class ManageableDatagramSocketManager;

    class MessageFactory;
    class NetworkMessageServiceListener;
    class NetworkMessageServiceProxyServer;
    class NICInfo;
    class NMSCommandProcessor;
    class Reassembler;

    class NetworkInterfaceManager;
    class NetworkMessageServiceImpl;

    class NetworkMessageService : public NetworkMessageServiceInterface
    {
        public:
            static const unsigned int DEFAULT_PORT = 6666;
            static const unsigned int DEFAULT_MTU = 1400;
            static const unsigned int DEFAULT_TIME_OUT = 10000;
            static const unsigned int DEFAULT_MCAST_TTL = 1;

            static const unsigned int EMPTY_RECIPIENT = 0;
            static const uint32 DEFAULT_RETRANSMISSION_TIME = 5000;
            static const uint32 DEFAULT_MAX_NUMBER_OF_RETRANSMISSIONS = 0;  // if DEFAULT_MAX_NUMBER_OF_RETRANSMISSIONS
                                                                            // is set to 0, then NetworMessageService
                                                                            // keeps retransmitting until the message is
                                                                            // acked (potentially forever :-~)
            static const uint8 K = 1;

        public:
            NetworkMessageService (PROPAGATION_MODE mode=MULTICAST, bool bAsyncDelivery = false,
                                   bool bAsyncTransmission = false, uint8 ui8MessageVersion = 1,
                                   bool bReplyViaUnicast = false);
            virtual ~NetworkMessageService (void);

            static const int NO_TARGET = 0;
            static const uint8 NMS_CTRL_MSG = 0x00;

            static NetworkMessageService * getInstance (ConfigManager *pCfgMgr);
            static NetworkMessageServiceProxyServer * getProxySvrInstance (ConfigManager *pCfgMgr);

            NMSCommandProcessor * getCmdProcessor (void);

            /**
             * if PROPAGATION_MODE == BROADCAST this will set the pszDestAddr to
             * NMS_BROADCAST_ADDRESS
             *
             * if PROPAGATION_MODE == MULTICAST this will set the pszDestAddr to
             * NMS_MULTICAST_ADDRESS
             */

            int init (ConfigManager *pCfgMgr);
            int init (uint16 ui16Port = DEFAULT_PORT, const char **ppszBindingInterfaces = NULL,
                      const char **ppszIgnoredInterfaces = NULL, const char **ppszAddedInterfaces = NULL,
                      const char *pszDestAddr = NULL, uint8 ui8McastTTL = DEFAULT_MCAST_TTL);

            // Set the retransmit timeout (in milliseconds) for reliable messages
            int setRetransmissionTimeout (uint32 ui32Timeout);
            int setPrimaryInterface (const char *pszInterfaceAddr);
            int start (void);
            int stop (void);

            int registerHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener);
            int deregisterHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener);

            /**
             * Broadcast a message
             * Returns 0 if successful or a negative value in case of error
             *
             * ui8MsgType - an application-defined identifier that is used to demultiplex multiple
             *              applications and message types - same as the key used when registering
             *              the listener and the key that will be given to the handler when a message arrives
             *
             * pOutgoingInterfaces - a list of interfaces that should be used to broadcast the message.
             *                       Can be NULL to indicate that the message propagation service should
             *                       make a decision or use a default.
             *
             * ui32BroadcastAddress - the address that should be used to broadcast the message
             *
             * ui16MsgId - a unique identifier for this message.
             *             Using 0 will cause the message propagation service to generate a new id.
             *
             * ui8HopCount - the hop count for this message - will be incremented before the handler on
             *               the receiving node is invoked.
             *
             * ui8TTL - the Time To Live (in terms of the number of hops) for this message
             *          NOTE: if the ui8HopCount <= ui8TTL, the message will NOT be transmitted
             *
             * ui16DelayTolerance - the maximum length of time, in milliseconds, that the message propagation
             *                      service might wait before sending the message. Used to allow message
             *                      aggregation by the message propagation service.
             *
             * pMsgMetaData - metadata for this message - as required by the component using the MessagePropagationService.
             *                Some potential data includes the path and link characteristics that the message has
             *                traversed to arrive at this node. The metadata may be modified as necessary (appended to,
             *                truncated, etc.) by the listener before forwarding the message on to the next hop.
             *                NOTE: The size of the metadata is limited to 65535 bytes, given that the size is
             *                specified using a 16-bit unsigned number.
             *
             * ui16MsgMetaDataLen - the size of the message metadata in bytes
             *
             * pMsg - the message itself
             *
             * ui16MsgLen - length of the message
             *
             * pszHints - optional hints that might be used to modify the behavior of the broadcast
             *
             */
            int broadcastMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces,
                                  uint32 ui32BroadcastAddress, uint16 ui16MsgId,
                                  uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                  const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                  const void *pMsg, uint16 ui16MsgLen, bool bExpedited,
                                  const char *pszHints = NULL);

            /*
             * TODO: implement this
             */
            int transmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                 uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                 const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg,
                                 uint16 ui16MsgLen, const char *pszHints = NULL);

            /*
             * Sends a message reliably, using fragmentation if necessary
             */
            int transmitReliableMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                         uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                         const char *pszHints = NULL);

            /**
             * Get the MTU of the non-receive-only interface with minimum value
             * of MTU
             */
            uint16 getMinMTU (void);

            /**
             * Get the address of the network interfaces being used by the
             * Network Message Service.
             *
             * NOTE: the returned array and its elements MUST be deallocated by
             * the caller.
             */
            char ** getActiveNICsInfoAsString (void);
            char ** getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination);
            char ** getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr);

            /**
             * Return the propagation mode NetworkMessageService is running in.
             */
            PROPAGATION_MODE getPropagationMode (void);

            /**
             * Returns the size of the delivery queue if the asynchronous delivery
             * is enabled, it always returns 0 otherwise.
             */
            uint32 getDeliveryQueueSize (void);

            /**
             * in Bps.
             * Returns a negative number if the interface was not found
             */
            int64 getReceiveRate (const char *pszAddr);

            /**
             * Get the size of the transmission queue for the specified interface
             * Only applies if the interface had asynchronous transmission enabled
             */
            uint32 getTransmissionQueueSize (const char *pszOutgoingInterface);

            /**
             * Get the size of the transmission queue for the specified interface
             * rescaled to fit in the interval [0, 255]
             * Only applies if the interface had asynchronous transmission enabled
             */
            uint8 getRescaledTransmissionQueueSize (const char *pszOutgoingInterface);

            /**
             * get the max size of the queue
             */
            uint32 getTransmissionQueueMaxSize (const char *pszOutgoingInterface);

            uint32 getTransmitRateLimit (const char *pszInterface);

            /**
             * set the max size of the transmission queue
             */
            int setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize);
            
            /**
             * Set the transmit rate limit for this socket
             * The target address, if specified, sets this limit only when sending data
             *     to the specified target. A value of 0 implies that it should be applied to
             *     all outgoing traffic.
             * The rate limit is specified in bytes per second
             * A value of 0 turns off the transmit rate limit
             * This is a pass-through to the underlying socket - which may or may
             * not enforce the transmit rate limit.
             *
             * Returns 0 if successful or a negative value in case of error.
             *
             * If the interface it is not specified, the rate limit is applied
             * to all the available interfaces used by the Network Message Service
             */
            int setTransmitRateLimit (const char *pszInterface, const char * pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszInterface, uint32 ui32DestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (uint32 ui32RateLimit);

            /*
             * Get the capacity of the link used by the specified interface
             */
            uint32 getLinkCapacity (const char *pszInterface);

            /*
             * Set the capacity of the link used by the specified interface
             */
            void setLinkCapacity (const char *pszInterface, uint32 ui32Capacity);

            uint8 getNeighborQueueLength (const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);

            // Checks whether it is clear to send on the specified interface
            bool clearToSend (const char *pszInterface);

            // Checks whether it is clear to send on all outgoing interfaces
            bool clearToSendOnAllInterfaces (void);

        private:
            Mutex _m;
            NetworkInterfaceManager *_pNetIntMgr;
            NetworkMessageServiceImpl *_Impl;
            NMSCommandProcessor *_pCmdProc;
            ManageableDatagramSocketManager *_pMgblSockMgr;
    };
}

#endif   // #ifndef INCL_NETWORK_MESSAGE_SERVICE_H
