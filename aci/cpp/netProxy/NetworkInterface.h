#ifndef INCL_NETWORK_INTERFACE_H
#define INCL_NETWORK_INTERFACE_H

/*
 * NetworkInterface.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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

#include <atomic>
#include <string>

#include "FTypes.h"
#include "net/NetworkHeaders.h"

#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    class NetworkInterface
    {
    public:
        enum class Type {
            T_Unknown = 0x00,
            T_Tap = 0x01,
            T_PCap = 0x02
        };

        explicit NetworkInterface (const NetworkInterface & rCopy) = delete;
        virtual ~NetworkInterface (void);
        virtual void requestTermination (void) = 0;

        //Returns the type of the network interface
        Type getType (void) const;
        //Returns the name of the network interface
        const std::string & getInterfaceName (void) const;
        //Returns the user friendly name of the network interface
        const std::string & getUserFriendlyInterfaceName (void) const;
        /* Returns the MAC address for the Tap Interface, if available, or nullptr
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

        virtual int setMACAddress (const uint8 * const pui8MACAddress);
        virtual int setMACAddress (const NOMADSUtil::EtherMACAddr & ema);
        virtual int setIPv4Address (uint32 ui32IPv4Address);
        virtual int setIPv4Netmask (uint32 ui32IPv4NetMask);
        virtual int setIPv4DefaultGatewayAddress (uint32 ui32IPv4DefaultGatewayAddress);
        virtual int setMTU (uint16 ui16MTU);

        virtual int readPacket (const uint8 ** pui8Buf, uint16 & ui16PacketLen) = 0;
        virtual int writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen) = 0;

        static std::string getDeviceNameFromUserFriendlyName (const char * const pcUserFriendlyInterfaceName);


    protected:
        NetworkInterface (Type tType, const std::string & sInterfaceName, const std::string & sUserFriendlyInterfaceName = "");

        static const uint8 * const getMACAddrForDevice (const char * const pszUserFriendlyInterfaceName);
        static NOMADSUtil::IPv4Addr getDefaultGatewayForInterface (const char *const pszUserFriendlyInterfaceName);
        static const uint32 retrieveMTUForInterface (const char *const pszUserFriendlyInterfaceName, int fd = -1);

        std::string _sInterfaceName;
        std::string _sUserFriendlyInterfaceName;
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
        std::atomic<bool> _bIsTerminationRequested;
    };


    inline NetworkInterface::~NetworkInterface (void) { }

    inline void NetworkInterface::requestTermination (void)
    {
        _bIsTerminationRequested = true;
    }

    inline NetworkInterface::Type NetworkInterface::getType (void) const
    {
        return _tType;
    }

    inline const std::string & NetworkInterface::getInterfaceName (void) const
    {
        return _sInterfaceName;
    }

    inline const std::string & NetworkInterface::getUserFriendlyInterfaceName (void) const
    {
        return _sUserFriendlyInterfaceName;
    }

    inline const uint8 * const NetworkInterface::getMACAddr (void) const
    {
        if (_bMACAddrFound) {
            return _aui8MACAddr;
        }
        else {
            return nullptr;
        }
    }

    inline const NOMADSUtil::IPv4Addr * const NetworkInterface::getIPv4Addr (void) const
    {
        if (_bIPAddrFound) {
            return &_ipv4Addr;
        }

        return nullptr;
    }

    inline const NOMADSUtil::IPv4Addr * const NetworkInterface::getNetmask (void) const
    {
        if (_bNetmaskFound) {
            return &_ipv4Netmask;
        }

        return nullptr;
    }

    inline const NOMADSUtil::IPv4Addr * const NetworkInterface::getDefaultGateway (void) const
    {
        if (_bDefaultGatewayFound) {
            return &_ipv4DefaultGateway;
        }

        return nullptr;
    }

    inline uint16 NetworkInterface::getMTUSize (void) const
    {
        if (_bMTUFound) {
            return _ui16MTU;
        }

        return 0;
    }

    inline int NetworkInterface::setMACAddress (const uint8 * const pui8MACAddress)
    {
        if (!_bMACAddrFound) {
            memcpy (_aui8MACAddr, pui8MACAddress, sizeof(_aui8MACAddr));
            _bMACAddrFound = true;

            return 1;
        }

        return 0;
    }

    inline int NetworkInterface::setMACAddress (const NOMADSUtil::EtherMACAddr & ema)
    {
        if (!_bMACAddrFound && (ema != NetProxyApplicationParameters::EMA_INVALID_ADDRESS)) {
            _aui8MACAddr[0] = ema.ui8Byte1;
            _aui8MACAddr[1] = ema.ui8Byte2;
            _aui8MACAddr[2] = ema.ui8Byte3;
            _aui8MACAddr[3] = ema.ui8Byte4;
            _aui8MACAddr[4] = ema.ui8Byte5;
            _aui8MACAddr[5] = ema.ui8Byte6;
            _bMACAddrFound = true;

            return 1;
        }

        return 0;
    }

    inline int NetworkInterface::setIPv4Address (uint32 ui32IPv4Address)
    {
        if (!_bIPAddrFound) {
            _ipv4Addr.ui32Addr = ui32IPv4Address;
            _bIPAddrFound = true;

            return 1;
        }

        return 0;
    }

    inline int NetworkInterface::setIPv4Netmask (uint32 ui32IPv4NetMask)
    {
        if (!_bNetmaskFound) {
            _ipv4Netmask.ui32Addr = ui32IPv4NetMask;
            _bNetmaskFound = true;

            return 1;
        }

        return 0;
    }

    inline int NetworkInterface::setIPv4DefaultGatewayAddress (uint32 ui32IPv4DefaultGatewayAddress)
    {
        if (!_bDefaultGatewayFound) {
            _ipv4DefaultGateway.ui32Addr = ui32IPv4DefaultGatewayAddress;
            _bDefaultGatewayFound = true;

            return 1;
        }

        return 0;
    }

    inline int NetworkInterface::setMTU (uint16 ui16MTU)
    {
        if (!_bMTUFound) {
            _ui16MTU = ui16MTU;
            _bMTUFound = true;

            return 1;
        }

        return 0;
    }
}

#endif   // #ifndef INCL_NETWORK_INTERFACE_H
