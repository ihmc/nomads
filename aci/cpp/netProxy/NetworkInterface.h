#ifndef INCL_NETWORK_INTERFACE_H
#define INCL_NETWORK_INTERFACE_H

/*
 * NetworkInterface.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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

#ifndef NULL
#define NULL 0
#endif

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "StrClass.h"


namespace ACMNetProxy
{
    class NetworkInterface
    {
        public:
            enum Type {
                T_Unknown = 0x00,
                T_Tap = 0x01,
                T_PCap = 0x02
            };

            virtual ~NetworkInterface (void);
            virtual void requestTermination (void) = 0;

            //Returns the type of Network Interface
            Type getType (void) const;
            //Returns the name of Network Adapter
            const NOMADSUtil::String & getAdapterName (void) const;
            /* Returns the MAC address for the Tap Interface, if available, or NULL
             * otherwise. The MAC address is returned as a pointer to a 6-byte array. */
            virtual const uint8 * const getMACAddr (void) const;
            // Returns the IPv4 address as a IPv4Addr*
            virtual const NOMADSUtil::IPv4Addr * const getIPv4Addr (void) const;
            // Returns the Netmask as a IPv4Addr*
            virtual const NOMADSUtil::IPv4Addr * const getNetmask (void) const;
            // Returns the Default Gateway address as a IPv4Addr*
            virtual const NOMADSUtil::IPv4Addr * const getDefaultGateway (void) const;
            // Returns the size of the MTU as a uint16
            virtual uint16 getMTUSize (void) const;

            virtual int readPacket (uint8 *pui8Buf, uint16 ui16BufSize) = 0;
            virtual int writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen) = 0;
            
            static NOMADSUtil::String getDeviceNameFromUserFriendlyName (const char *const pszUserFriendlyInterfaceName);

        protected:
            NetworkInterface (Type tType);

            NOMADSUtil::String _sAdapterName;
            const Type _tType;
            uint8 _aui8MACAddr[6];
            NOMADSUtil::IPv4Addr _ipv4Addr;
            NOMADSUtil::IPv4Addr _ipv4Netmask;
            NOMADSUtil::IPv4Addr _ipv4DefaultGateway;
            uint16 _ui16MTU;
            bool _bMACAddrFound;
            bool _bIPAddrFound;
            bool _bNetmaskFound;
            bool _bDefaultGatewayFound;
            bool _bMTUFound;
            bool _bIsTerminationRequested;
            
            static const uint8 * const getMACAddrForDevice (const char * const pszUserFriendlyInterfaceName);
            static NOMADSUtil::IPv4Addr getDefaultGatewayForInterface (const char *const pszUserFriendlyInterfaceName);

        private:
            explicit NetworkInterface (const NetworkInterface& rCopy);
    };


    inline NetworkInterface::~NetworkInterface (void) {}

    inline void NetworkInterface::requestTermination (void)
    {
        _bIsTerminationRequested = true;
    }

    inline const NOMADSUtil::String & NetworkInterface::getAdapterName (void) const
    {
        return _sAdapterName;
    }

    inline const uint8 * const NetworkInterface::getMACAddr (void) const
    {
        if (_bMACAddrFound) {
            return _aui8MACAddr;
        }
        else {
            return NULL;
        }
    }

    inline const NOMADSUtil::IPv4Addr * const NetworkInterface::getIPv4Addr (void) const
    {
        if (_bIPAddrFound) {
            return &_ipv4Addr;
        }

        return NULL;
    }

    inline const NOMADSUtil::IPv4Addr * const NetworkInterface::getNetmask (void) const
    {
        if (_bNetmaskFound) {
            return &_ipv4Netmask;
        }

        return NULL;
    }

    inline const NOMADSUtil::IPv4Addr * const NetworkInterface::getDefaultGateway (void) const
    {
        if (_bDefaultGatewayFound) {
            return &_ipv4DefaultGateway;
        }

        return NULL;
    }

    inline uint16 NetworkInterface::getMTUSize (void) const
    {
        if (_bMTUFound) {
            return _ui16MTU;
        }

        return 0;
    }
}

#endif   // #ifndef INCL_NETWORK_INTERFACE_H
