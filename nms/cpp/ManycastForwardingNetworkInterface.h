/*
 * ManycastForwardingNetworkInterface.h
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
 * Created on January 28, 2014, 6:43 PM
 */

#ifndef INCL_MANYCAST_FORWARDING_NETWORK_INTERFACE_H
#define	INCL_MANYCAST_FORWARDING_NETWORK_INTERFACE_H

#include "NetworkInterface.h"

#include "Mutex.h"
#include "TimeBoundedStringHashset.h"

namespace NOMADSUtil
{
    class ManycastForwardingNetworkInterface : public NetworkInterface
    {
        public:
            static uint8 MCAST_FWD_IF;

            ManycastForwardingNetworkInterface (NetworkInterface *pNetInt, uint32 ui32StorageDuration = 60000);
            virtual ~ManycastForwardingNetworkInterface (void);

            bool addForwardingIpAddress (uint32 ui32IPv4Addr);
            bool addForwardingIpAddress (const char *pszKey);

            int init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                      NetworkInterfaceManagerListener *pNMSParent,
                      bool bReceiveOnly , bool bSendOnly,
                      const char *pszPropagationAddr, uint8 ui8McastTTL);

            int rebind (void);

            int start (void);
            int stop (void);

            uint8 getMode (void);
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
            bool isAvailable (void);
            bool boundToWildcardAddr (void);


            int getReceiveBufferSize (void);
            int setReceiveBufferSize (int iBufSize);

            virtual uint8 getType (void);

            uint32 getTransmitRateLimit (const char *pszDestinationAddr);
            uint32 getTransmitRateLimit (void);

            int setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit);
            int setTransmitRateLimit (uint32 ui32RateLimit);

            void setReceiveRateSampleInterval (uint32 ui32IntervalInMS);
            uint32 getReceiveRate (void);

            uint32 getTransmissionQueueSize (void);
            uint8 getRescaledTransmissionQueueSize (void);
            void setTransmissionQueueMaxSize (uint32 ui32MaxSize);
            uint32 getTransmissionQueueMaxSize (void);

            void setLinkCapacity (uint32 ui32Capacity);
            uint32 getLinkCapacity (void);

            void setAutoResizeQueue (bool bEnable, uint32 ui32MaxTimeInQueue = 3000);
            uint32 getAutoResizeQueue (void);

            int receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr, InetAddr *pRemoteAddr);

            int sendMessage (const NetworkMessage *pNetMsg, bool bExpedited = false, const char *pszHints = NULL);
            int sendMessage (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, bool bExpedited = false, const char *pszHints = NULL);
            int sendMessageNoBuffering (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints);

            bool canSend (void);
            bool canReceive (void);

            bool clearToSend (void);

            bool operator == (const NetworkInterface &rhsStr) const;

        private:
            Mutex _m;
            TimeBoundedStringHashset _unicastAddresses;
            NetworkInterface *_pNetInt;
    };
}

#endif	// INCL_MANYCAST_FORWARDING_NETWORK_INTERFACE_H

