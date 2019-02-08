/*
 * NetworkInterfaceManager.h
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
 * Created on May 16, 2015, 7:30 AM
 */

#ifndef INCL_NETWORK_INTERFACE_MANAGER_H
#define    INCL_NETWORK_INTERFACE_MANAGER_H

#include "ManageableThread.h"
#include "Mutex.h"
#include "PtrLList.h"
#include "StringHashset.h"
#include "StringHashtable.h"

#include "NetworkInterface.h"

namespace NOMADSUtil
{
    class NetworkMessage;
    typedef StringHashtable<NetworkInterface> Interfaces;

    class NetworkInterfaceManagerListener
    {
        public:
            virtual int messageArrived (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr) = 0;
            virtual int messageSent (const NetworkMessage *pNetMsg, const char *pchOutgoingInterface) = 0;
    };

    class NetworkInterfaceManager : public ManageableThread
    {
        public:
            NetworkInterfaceManager (PROPAGATION_MODE mode, bool bReplyViaUnicast, bool bAsyncTransmission);
            ~NetworkInterfaceManager (void);

            void run (void);

            int init (ConfigManager *pCfgMgr);
            int init (const char **ppszBindingInterfaces, const char **ppszIgnoredInterfaces,
                      const char **ppszAddedInterfaces, const char *pszDestinationAddr,
                      uint16 ui16Port, uint8 ui8TTL);

            void addFwdingAddrToManycastIface (uint32 ui32SourceAddr, const char *pszIncomingInterface);
            bool clearToSend (const char *pszInterface);
            bool clearToSendOnAllInterfaces (void);

            char ** getActiveNICsInfoAsString (void);
            char ** getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination);
            char ** getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr);

            uint32 getLinkCapacity (const char *pszInterface);
            uint16 getMinMTU (void);
            uint16 getMTU (void) const;
            String getOutgoingInterfaceForAddr (unsigned long int ulRemoteAddr);

            bool isPrimaryIfaceSet (void) const;

            // returns empty string if it's not possible to guess
            String tryToGuessIncomingIface (void);
            uint32 getPrimaryInterface (void) const;
            int64 getReceiveRate (const char *pszAddr);
            uint8 getRescaledTransmissionQueueSize (const char *pszOutgoingInterface);
            uint32 getTransmissionQueueMaxSize (const char *pszOutgoingInterface);
            uint32 getTransmissionQueueSize (const char *pszOutgoingInterface);
            uint32 getTransmitRateLimit (const char *pszInterface);

            bool isUnicast (uint32 ui32Address, const char *pszIncomingInterface);
            bool isSupportedManycastAddr (uint32 ui32Address);

            int registerListener (NetworkInterfaceManagerListener *pListener);
            void reset (void);

            // Check whether the address for any interfaces using proxy datagram sockets have been obtained
            // and if so, moves them into the regular set of interfaces
            int resolveProxyDatagramSocketAddresses (void);

            // Returns true if the message was sent on at lest one interface
            bool send (NetworkMessage *pNetMsg, const char **ppszOutgoingInterfaces,
                       uint32 ui32Address, bool bExpedited, const char *pszHints);

            void setLinkCapacity (const char *pszInterface, uint32 ui32Capacity);
            int setPrimaryInterface (const char *pszInterfaceAddr);
            int setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize);
            int setTransmitRateLimit (uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress, uint32 ui32RateLimit);

            void start (void);
            void stop (void);

        private:
            void bindInterfaces (StringHashset &ifaces);
            void bindInterface (const String &iface);

            void setSampleRate (NetworkInterface *pIface);
            void setSampleRate (Interfaces &ifaces);

        private:
            const PROPAGATION_MODE _mode;
            bool _bPrimaryInterfaceIdSet;
            const bool _bReplyViaUnicast;
            bool _bAsyncTransmission;
            bool _bRejoinMcastGrp;
            uint8 _ui8MTTL;
            uint16 _ui16MTU;
            uint16 _ui16Port;
            uint32 _ui32PrimaryInterface;
            uint32 _ui32SampleInterval;
            uint32 _ui32MaxMsecsInOutgoingQueue;
            String _dstAddr;
            NetworkInterfaceManagerListener *_pListener;
            Mutex _m;
            Mutex _mProxyInterfacesToResolve;
            StringHashset _requieredInterfaces;  // not all of them may immediately be available
            StringHashset _optionalInterfaces;   // not all of them may immediately be available
            StringHashset _forbiddenInterfaces;
            PtrLList<NetworkInterface> _proxyInterfacesToResolve;
            Interfaces _interfaces;
    };
}

#endif    /* INCL_NETWORK_INTERFACE_MANAGER_H */

