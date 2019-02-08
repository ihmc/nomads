/*
 * NetworkInterface.h
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

#ifndef INCL_NETWORK_INTERFACE_H
#define INCL_NETWORK_INTERFACE_H

#include "ConditionVariable.h"
#include "FIFOQueue.h"
#include "FTypes.h"
#include "Mutex.h"
#include "OSThread.h"
#include "StrClass.h"

#include "NetworkMessageService.h"

#include <stddef.h>

namespace NOMADSUtil
{
    class DatagramSocket;
    class NetworkMessage;
    class NetworkMessageReceiver;
    class NetworkInterfaceManagerListener;
}

namespace NOMADSUtil
{
    class InetAddr;
    class ManageableDatagramSocketManager;

    class NetworkInterface
    {
        public:
            static uint8 NET_IF;
            static const char *IN_ADDR_ANY_STR;

            NetworkInterface (void);
            virtual ~NetworkInterface (void);

            /**
             * pszPropagationAddr is the address message will be sent to, unless
             * a different address is specified in send().
             *
             * NOTE: in case the NetworkMessageService is running in MULTICAST
             * mode, the pszPropagationAddr must be the multicast address of  the
             * group to be joined
             */
            virtual int init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                              NetworkInterfaceManagerListener *pNMSParent,
                              bool bReceiveOnly , bool bSendOnly,
                              const char *pszPropagationAddr, uint8 ui8McastTTL) = 0;
            void manage (ManageableDatagramSocketManager *pMgblSockMgr);

            /**
             * Tries to re-bind to the interface that was configured int init().
             * If init() was never called, rebind() fails.
             * Returns 0 if the attempt to re-bind was successful, a negative
             * number otherwise.
             */
            virtual int rebind (void) = 0;

            /**
             * Start receiving data (if bSendOnly was not set to true during initialization)
             */
            virtual int start (void) = 0;

            virtual int stop (void) = 0;

            virtual uint8 getMode (void) = 0;
            virtual uint16 getMTU (void) = 0;

            virtual const char * getBindingInterfaceSpec (void) const = 0;
            virtual const char * getNetworkAddr (void) = 0;
            virtual const char * getBroadcastAddr (void) const = 0;
            virtual const char * getNetmask (void) const = 0;

            virtual uint16 getPort (void) = 0;
            virtual const char * getPropagatonAddr (void) = 0;
            virtual NetworkMessageReceiver * getReceiver (void) = 0;
            virtual uint8 getTTL (void) = 0;

            virtual void setDisconnected (void) = 0;
            virtual bool isAvailable (void) = 0;
            virtual bool boundToWildcardAddr (void) = 0;

            /**
             *  Get the size of the receive buffer for the underlying socket
             *Returns -1 if the underlying socket does not have the notion of a receive buffer
             */
            virtual int getReceiveBufferSize (void) = 0;

            /**
             * Set the size of the receive buffer for the underlying socket
             * Returns -1 if the underlying socket does not have the notion of a receive buffer
             */
            virtual int setReceiveBufferSize (int iBufSize) = 0;

            virtual uint8 getType (void) = 0;

            /**
             * Get the transmit rate limit for this interface for a specified target address
             * This is a pass-through to the underlying socket - which may or may not support transmit rate limits
             * Returns the limit in bytes per second or 0 if there is no limit set
             */
            virtual uint32 getTransmitRateLimit (const char *pszDestinationAddr) = 0;
            virtual uint32 getTransmitRateLimit (void) = 0;

            /**
             * Set the transmit rate limit for this socket for a specified target address
             * The rate limit is specified in bytes per second
             * A value of 0 turns off the transmit rate limit
             * This is a pass-through to the underlying socket - which may or may not enforce the transmit rate limit
             * Returns 0 if successful or a negative value in case of error
             */
            virtual int setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit) = 0;
            virtual int setTransmitRateLimit (uint32 ui32RateLimit) = 0;

            /**
             * Set the interval of time over which the class maintains the average number of bytes
             * received on the interval
             * NOTE: Given that the underlying UDP socket is not in promiscuous mode, this average
             * only applies to data actually received by this interface. However, any broadcast and
             * multicast data would be accounted for as well.
             */
            virtual void setReceiveRateSampleInterval (uint32 ui32IntervalInMS) = 0;

            /**
             * Get the average number of bytes received per second on this interface
             * NOTE: See note above for the setReceiveRateSampleInterval() method
             */
            virtual uint32 getReceiveRate (void) = 0;

            /**
             * Returns the size of the transmission queue if asynchronous transmission
             * is enabled, otherwise it always returns 0.
             */
            virtual uint32 getTransmissionQueueSize (void) = 0;

            /**
             * Returns the size of the transmission queue if asynchronous transmission
             * is enabled, rescaled to fit the range [0, 255].
             * otherwise it always returns 0.
             */
            virtual uint8 getRescaledTransmissionQueueSize (void) = 0;

            /*
             * Set the maximum number of elements that can be queued for transmission at any given time
             * set to 0 to disable
             */
            virtual void setTransmissionQueueMaxSize (uint32 ui32MaxSize) = 0;
            virtual uint32 getTransmissionQueueMaxSize (void) = 0;

            /*
             * Set the maximum capacity of the link the network interface is using
             */
            virtual void setLinkCapacity (uint32 ui32Capacity) = 0;

            /*
             * Get the maximum capacity of the link
             */
            virtual uint32 getLinkCapacity (void) = 0;

            /*
             * Activate/deactivate auto resizing of the transmission queue
             */
            virtual void setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue = 3000) = 0;
            virtual uint32 getAutoResizeQueue (void) = 0;

            virtual int receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr, InetAddr *pRemoteAddr) = 0;

            /**
             * Send a message to the address set in pszIPAddr or ui32IPAddr.
             * If no address is set either broadcast (to ANY_ADDRESS) or multicast
             * (to) is used as recipient, depending of the compilation flag set.
             *
             * The value of pszIPAddr and ui32IPAddr can be a broadcast, multicast
             * or unicast address.
             */
            virtual int sendMessage (const NetworkMessage *pNetMsg, bool bExpedited = false, const char *pszHints = NULL) = 0;
            virtual int sendMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, bool bExpedited = false, const char *pszHints = NULL) = 0;
            virtual int sendMessageNoBuffering (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints) = 0;

            // Checks whether this interface was configured to be able to send data
            virtual bool canSend (void) = 0;

            // Checks whether this interface was configured to be able to receive data
            virtual bool canReceive (void) = 0;

            // Checks whether this interface is currently able to send data (i.e., flow control)
            virtual bool clearToSend (void) = 0;

            virtual bool operator == (const NetworkInterface &rhsStr) const = 0;
    };
}

#endif   // #ifndef INCL_NETWORK_INTERFACE_H
