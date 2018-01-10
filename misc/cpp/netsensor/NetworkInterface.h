#ifndef NETSENSOR_NetworkInterface__INCLUDED
#define NETSENSOR_NetworkInterface__INCLUDED
/*
* NetworkInterface.h
* Author: rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
* Network interface wrapper
*
*/

#include <cstring>

#include "FTypes.h"
#include "StrClass.h"
#include "NetworkHeaders.h"


namespace IHMC_NETSENSOR
{
class NetworkInterface
{
public:
    NetworkInterface();
    NetworkInterface(uint32 ui32IPAddr, uint32 ui32Netmask, uint32 ui32GwIPAddr,
                     NOMADSUtil::EtherMACAddr emacInterfaceMAC);
    virtual ~NetworkInterface(void);
    virtual void requestTermination(void);
    virtual bool terminationRequested(void) const;

    static NOMADSUtil::IPv4Addr getDefaultGatewayForInterface(
        const char *const pszUserFriendlyInterfaceName);
    virtual const NOMADSUtil::IPv4Addr * const getIPv4Addr(void) const;
    // Returns the Default Gateway address as a IPv4Addr*
    virtual const NOMADSUtil::IPv4Addr * const getDefaultGateway(void) const;
    static NOMADSUtil::String getDeviceNameFromUserFriendlyName(
        const char * const pszUserFriendlyInterfaceName);
    static const uint8 * const getMACAddrForDevice(
        const char * const pszUserFriendlyInterfaceName);
    virtual const uint8 * const getMACAddr(void) const;
    virtual const NOMADSUtil::IPv4Addr * const getNetmask(void) const;
    virtual int readPacket(uint8 *pui8Buf, uint16 ui16BufSize, int64 *tus) = 0;

//<--------------------------------------------------------------------------->
protected:
    NOMADSUtil::String _sAdapterName;

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
    bool _bTerminationRequested;
};


inline NetworkInterface::NetworkInterface() :
    _ipv4Addr{0}, _ipv4Netmask{0}, _ipv4DefaultGateway{0},
    _ui16MTU(0), _bMACAddrFound(false), _bIPAddrFound(false),
    _bNetmaskFound(false), _bDefaultGatewayFound(false),
    _bMTUFound(false), _bTerminationRequested(false)
{
    memset(_aui8MACAddr, 0, 6);
}

inline NetworkInterface::NetworkInterface(uint32 ui32IPAddr, uint32 ui32Netmask,
                                          uint32 ui32GwIPAddr,
                                          NOMADSUtil::EtherMACAddr emacInterfaceMAC) :
    _ipv4Addr{ui32IPAddr}, _ipv4Netmask{ui32Netmask},
    _ipv4DefaultGateway{ui32GwIPAddr}, _ui16MTU(1500),
    _bMACAddrFound(true), _bIPAddrFound(true),
    _bNetmaskFound(true), _bDefaultGatewayFound(true),
    _bMTUFound(true), _bTerminationRequested(false)
{
    memcpy(static_cast<void *>(_aui8MACAddr), static_cast<void *> (&emacInterfaceMAC), 6);
}

inline NetworkInterface::~NetworkInterface(void) { }

inline void NetworkInterface::requestTermination(void)
{
    _bTerminationRequested = true;
}

inline bool NetworkInterface::terminationRequested(void) const
{
    return _bTerminationRequested;
}

inline const NOMADSUtil::IPv4Addr * const
    NetworkInterface::getNetmask(void) const
{
    return _bNetmaskFound ? &_ipv4Netmask : NULL;
}

inline const NOMADSUtil::IPv4Addr * const
    NetworkInterface::getIPv4Addr(void) const
{
    return _bIPAddrFound ? &_ipv4Addr : NULL;
}

inline const uint8 * const NetworkInterface::getMACAddr(void) const
{
    return _bMACAddrFound ? _aui8MACAddr : NULL;
}

inline const NOMADSUtil::IPv4Addr * const
    NetworkInterface::getDefaultGateway(void) const
{
    return _bDefaultGatewayFound ? &_ipv4DefaultGateway : NULL;
}

}
#endif