/*
 * NetworkInterface.h
 *
 * This file is part of the IHMC Util Library
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
 */

#ifndef INCL_NETWORK_INTERFACE_H
#define INCL_NETWORK_INTERFACE_H

#include "ConditionVariable.h"
#include "FIFOQueue.h"
#include "FTypes.h"
#include "Mutex.h"
#include "OSThread.h"
#include "StrClass.h"

#include <stddef.h>

namespace NOMADSUtil
{
    class DatagramSocket;
    class NetworkMessage;
    class NetworkMessageReceiver;
    class NetworkMessageService;
}

namespace NOMADSUtil
{
    class InetAddr;

    class NetworkInterface
    {
        public:
            static uint8 NET_IF;

            NetworkInterface (bool bAsyncTransmission = false);
            virtual ~NetworkInterface (void);

            /**
             * pszPropagationAddr is the address message will be sent to, unless
             * a different address is specified in send().
             *
             * NOTE: in case the NetworkMessageService is running in MULTICAST
             * mode, the pszPropagationAddr must be the multicast address of  the
             * group to be joined
             */
            int init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                      NetworkMessageService *pNMSParent,
                      bool bReceiveOnly , bool bSendOnly,
                      const char *pszPropagationAddr, uint8 ui8McastTTL);

            /**
             * Tries to re-bind to the interface that was configured int init().
             * If init() was never called, rebind() fails.
             * Returns 0 if the attempt to re-bind was successful, a negative
             * number otherwise.
             */
            int rebind (void);

            /** 
             * Start receiving data (if bSendOnly was not set to true during initialization)
             */
            int start (void);

            int stop (void);

            uint16 getMTU (void);

            const char * getBindingInterfaceSpec (void) const;
            const char * getNetworkAddr (void);
            const char * getBroadcastAddr (void) const;
            const char * getNetmask (void) const;

            uint16 getPort (void);
            const char * getPropagatonAddr (void);
            NetworkMessageReceiver * getReceiver (void);
            uint8 getTTL (void);

            void setDisconnected (void);
            bool isConnected (void);
            bool boundToWildcardAddr (void);

            /**
             *  Get the size of the receive buffer for the underlying socket
             *Returns -1 if the underlying socket does not have the notion of a receive buffer
             */
            int getReceiveBufferSize (void);

            /**
             * Set the size of the receive buffer for the underlying socket
             * Returns -1 if the underlying socket does not have the notion of a receive buffer
             */
            int setReceiveBufferSize (uint16 ui16BufSize);

            uint8 getType (void);

            /**
             * Get the transmit rate limit for this interface for a specified target address
             * This is a pass-through to the underlying socket - which may or may not support transmit rate limits
             * Returns the limit in bytes per second or 0 if there is no limit set
             */
            virtual uint32 getTransmitRateLimit (const char *pszDestinationAddr);
            virtual uint32 getTransmitRateLimit (void);

            /**
             * Set the transmit rate limit for this socket for a specified target address
             * The rate limit is specified in bytes per second
             * A value of 0 turns off the transmit rate limit
             * This is a pass-through to the underlying socket - which may or may not enforce the transmit rate limit
             * Returns 0 if successful or a negative value in case of error
             */
            virtual int setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit);
            virtual int setTransmitRateLimit (uint32 ui32RateLimit);

            /**
             * Set the interval of time over which the class maintains the average number of bytes
             * received on the interval
             * NOTE: Given that the underlying UDP socket is not in promiscuous mode, this average
             * only applies to data actually received by this interface. However, any broadcast and
             * multicast data would be accounted for as well.
             */
            virtual void setReceiveRateSampleInterval (uint32 ui32IntervalInMS);

            /**
             * Get the average number of bytes received per second on this interface
             * NOTE: See note above for the setReceiveRateSampleInterval() method
             */
            virtual uint32 getReceiveRate (void);

            /**
             * Returns the size of the transmission queue if asynchronous transmission
             * is enabled, otherwise it always returns 0.
             */
            uint32 getTransmissionQueueSize (void);

            /**
             * Returns the size of the transmission queue if asynchronous transmission
             * is enabled, rescaled to fit the range [0, 255].
             * otherwise it always returns 0.
             */
            uint8 getRescaledTransmissionQueueSize (void);

            /*
             * Set the maximum number of elements that can be queued for transmission at any given time
             * set to 0 to disable
             */
            void setTransmissionQueueMaxSize (uint32 ui32MaxSize);
            uint32 getTransmissionQueueMaxSize (void);

            /*
             * Set the maximum capacity of the link the network interface is using
             */
            virtual void setLinkCapacity (uint32 ui32Capacity);

            /*
             * Get the maximum capacity of the link
             */
            virtual uint32 getLinkCapacity();

            /*
             * Activate/deactivate auto resizing of the transmission queue
             */
            void setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue = 3000);
            uint32 getAutoResizeQueue (void);

            int receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr,
                         InetAddr *pRemoteAddr);

            /**
             * Send a message to the address set in pszIPAddr or ui32IPAddr.
             * If no address is set either broadcast (to ANY_ADDRESS) or multicast
             * (to) is used as recipient, depending of the compilation flag set.
             *
             * The value of pszIPAddr and ui32IPAddr can be a broadcast, multicast
             * or unicast address.
             */
            int sendMessage (const NetworkMessage *pNetMsg, bool bExpedited = false, const char *pszHints = NULL);
            int sendMessage (const NetworkMessage *pNetMsg, const char *pszIPAddr, bool bExpedited = false, const char *pszHints = NULL);
            int sendMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, bool bExpedited = false, const char *pszHints = NULL);

            // Checks whether this interface was configured to be able to send data
            bool canSend (void);

            // Checks whether this interface was configured to be able to receive data
            bool canReceive (void);

            // Checks whether this interface is currently able to send data (i.e., flow control)
            bool clearToSend (void);

            bool operator == (const NetworkInterface &rhsStr) const;

        public:
            static const char *IN_ADDR_ANY_STR;

        protected:
            NetworkInterface (uint8 ui8Type, bool bAsyncTransmission);
            virtual int sendMessageInternal (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints);

        private:
            /**
             * Adjusts the queue size to the transmit rate limit, so that
             * messages wait maximum _ui32MaxTimeInQueue msecs in queue
             */
            void autoResizeQueue (uint16 ui16MsgSize);
            void construct (void);
            int bind (void);

            static void transmissionThread (void *pArg);

        private:
            struct QueuedMessage
            {
                QueuedMessage (void);
                NetworkMessage *pMsg;
                uint32 ui32DestinationAddr;
                String hints;
            };

            const uint8 _ui8Type;
            bool _bIsConnected;
            const bool _bAsyncTransmission;
            bool _bAutoResizeQueue;
            bool _bReceiveOnly;
            bool _bSendOnly;
            uint8 _ui8Mode;
            uint8 _ui8MsgVersion;
            uint8 _ui8McastTTL;
            uint16 _ui16Port;
            uint16 _ui16BufSize;
            uint32 _ui32LinkCapacity;
            uint32 _ui32MaxTimeInQueue;
            uint32 _ui32TransmissionQueueMaxLength;    // the current max queue length
            uint32 _ui32TransmissionQueueMaxLengthCap; // the limit above which _ui32TransmissionQueueMaxLength cannot go
            String _bindingInterfaceSpec;
            String _networkAddr;
            String _netmask;
            String _broadcastAddr;
            String _defaultPropagationAddr;
            DatagramSocket *_pDatagramSocket;
            NetworkMessageReceiver *_pReceiver;
            NetworkMessageService *_pNMSParent;
            OSThread _ostTransmissionThread;
            Mutex _mTransmissionQueue;
            ConditionVariable _cvTransmissionQueue;
            Mutex _mQueueNotFull;
            Mutex _mBind;
            FIFOQueue _transmissionQueue;
            FIFOQueue _expeditedTransmissionQueue;
    };

    inline bool NetworkInterface::canSend (void)
    {
        return !_bReceiveOnly;
    }

    inline bool NetworkInterface::canReceive (void)
    {
        return !_bSendOnly;
    }

    inline uint8 NetworkInterface::getType (void)
    {
        return _ui8Type;
    }

    inline bool NetworkInterface::operator == (const NetworkInterface &rhsStr) const
    {
        return ((_networkAddr == rhsStr._networkAddr) == 1);
    }

    inline NetworkInterface::QueuedMessage::QueuedMessage (void)
    {
        pMsg = NULL;
    }
}

#endif   // #ifndef INCL_NETWORK_INTERFACE_H
