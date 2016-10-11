/*
 * LinkCapacityTrait.h
 *
 * This file is part of the IHMC Util Library
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

#ifndef INCL_ABSTRACT_NETWORK_INTERFACE_H
#define INCL_ABSTRACT_NETWORK_INTERFACE_H

#include "NetworkInterface.h"

#include "ConditionVariable.h"
#include "FIFOQueue.h"
#include "ManageableThread.h"
#include "Mutex.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class NetworkInterface;
    class NetworkMessage;
    class NetworkMessageReceiver;
    class NetworkInterfaceManagerListener;

    class LinkCapacityTrait
    {
        public:
            LinkCapacityTrait (void);
            ~LinkCapacityTrait (void);

            void setLinkCapacity (uint32 ui32Capacity);
            uint32 getLinkCapacity (void);

        private:
            uint32 _ui32LinkCapacity;
    };

    class AsyncTranmissionTrait : public ManageableThread
    {
        public:
            AsyncTranmissionTrait (NetworkInterface *pNetIf, bool bAsyncTransmission);
            ~AsyncTranmissionTrait (void);

            void run (void);

            bool asynchronousTransmission (void);

            uint32 getTransmissionQueueSize (void);
            uint8 getRescaledTransmissionQueueSize (void);

            void setTransmissionQueueMaxSize (uint32 ui32MaxSize);
            uint32 getTransmissionQueueMaxSize (void);

            void setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue = 3000);
            uint32 getAutoResizeQueue (void);

            bool bufferOutgoingMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr,
                                        bool bExpedited, const char *pszHints,
                                        const char *pszBindingInterfaceSpec);

        private:
            /*
             * Adjusts the queue size to the transmit rate limit, so that
             * messages wait maximum _ui32MaxTimeInQueue msecs in queue
             */
            void autoResizeQueue (uint16 ui16MsgSize);

            struct QueuedMessage
            {
                QueuedMessage (void);
                ~QueuedMessage (void);
                NetworkMessage *pMsg;
                uint32 ui32DestinationAddr;
                String hints;
            };

            const bool _bAsyncTransmission;
            bool _bAutoResizeQueue;
            NetworkInterface *_pNetIf;

            uint32 _ui32MaxTimeInQueue;
            uint32 _ui32TransmissionQueueMaxLength;    // the current max queue length
            uint32 _ui32TransmissionQueueMaxLengthCap; // the limit above which _ui32TransmissionQueueMaxLength cannot go
            Mutex _mTransmissionQueue;
            ConditionVariable _cvTransmissionQueue;
            FIFOQueue _transmissionQueue;
            FIFOQueue _expeditedTransmissionQueue;
    };

    class AbstractNetworkInterface : public NetworkInterface
    {
        public:
            explicit AbstractNetworkInterface (bool bAsyncTransmission);
            virtual ~AbstractNetworkInterface (void);

            int init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                      NetworkInterfaceManagerListener *pNMSParent,
                      bool bReceiveOnly, bool bSendOnly,
                      const char *pszPropagationAddr, uint8 ui8McastTTL);

            int start (void);
            int stop (void);

            bool canSend (void);
            bool canReceive (void);

            bool boundToWildcardAddr (void);
            const char * getBindingInterfaceSpec (void) const;

            const char * getBroadcastAddr (void) const;
            const char * getNetmask (void) const;
            uint16 getPort (void);
            const char * getPropagatonAddr (void);
            NetworkMessageReceiver * getReceiver (void);
            uint8 getTTL (void);

            void setLinkCapacity (uint32 ui32Capacity);
            uint32 getLinkCapacity (void);

            uint32 getTransmissionQueueSize (void);
            uint8 getRescaledTransmissionQueueSize (void);

            void setTransmissionQueueMaxSize (uint32 ui32MaxSize);
            uint32 getTransmissionQueueMaxSize (void);

            void setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue = 3000);
            uint32 getAutoResizeQueue (void);

            void setReceiveRateSampleInterval (uint32 ui32IntervalInMS);
            uint32 getReceiveRate (void);

            bool bufferOutgoingMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr,
                                        bool bExpedited, const char *pszHints,
                                        const char *pszBindingInterfaceSpec);

            int sendMessage (const NetworkMessage *pNetMsg, bool bExpedited = false, const char *pszHints = NULL);
            int sendMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, bool bExpedited = false, const char *pszHints = NULL);

            bool operator == (const NetworkInterface &rhsStr) const;

        protected:
            bool _bReceiveOnly;
            bool _bSendOnly;
            uint8 _ui8McastTTL;
            uint16 _ui16Port;
            String _defaultPropagationAddr;
            String _bindingInterfaceSpec;
            String _netmask;
            String _broadcastAddr;
            NetworkInterfaceManagerListener *_pNMSParent;
            NetworkMessageReceiver *_pReceiver;

        private:
            LinkCapacityTrait _linkCapacity;
            AsyncTranmissionTrait _asyncTx;
    };
}


#endif  /* INCL_ABSTRACT_NETWORK_INTERFACE_H */

