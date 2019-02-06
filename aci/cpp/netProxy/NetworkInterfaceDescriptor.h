#ifndef INCL_NETWORK_INTERFACE_DESCRIPTOR_H
#define INCL_NETWORK_INTERFACE_DESCRIPTOR_H

/*
* NetworkInterfaceDescriptor.h
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
*
* Class that groups several properties related to network interfaces.
*/

#include <string>

#include "FTypes.h"
#include "NetworkHeaders.h"

namespace ACMNetProxy
{
    struct NetworkInterfaceDescriptor
    {
        std::string sInterfaceName{""};                                         // User friendly name of the interface
        uint32 ui32IPv4Address{0};                                              // IP address of the network interface (or the TUN/TAP interface if running in HM)
        uint32 ui32IPv4NetMask{0};                                              // Netmask of the network to which the interface belongs
        uint16 ui16InterfaceMTU{0};                                             // MTU (in bytes) for frames written to or read from the network interface
        NOMADSUtil::EtherMACAddr emaInterfaceMACAddress{};                      // MAC address of the network interface
        uint32 ui32IPv4GatewayAddress{0};                                       // IP address of the network gateway
        NOMADSUtil::EtherMACAddr emaNetworkGatewayMACAddress{};                 // MAC address of the network gateway
        bool bForwardBroadcastPacketsOnlyIfSameNetwork;                         // If true, broadcast packets are forwarded onto this interface only if the destination address matches the network's broadcast address

        std::unordered_set<std::string> usMulticastForwardingInterfacesList;    // Names of all interfaces on which multicast packets received by this interface will be forwarded
        std::unordered_set<std::string> usBroadcastForwardingInterfacesList;    // Names of all interfaces on which broadcast packets received by this interface will be forwarded
    };

    inline bool operator == (const NetworkInterfaceDescriptor & lhs, const NetworkInterfaceDescriptor & rhs)
    {
        return (lhs.sInterfaceName == rhs.sInterfaceName) && (lhs.ui32IPv4Address == rhs.ui32IPv4Address) &&
            (lhs.ui32IPv4NetMask == rhs.ui32IPv4NetMask) && (lhs.ui16InterfaceMTU == rhs.ui16InterfaceMTU) &&
            (lhs.ui32IPv4GatewayAddress == rhs.ui32IPv4GatewayAddress) &&
            (lhs.emaInterfaceMACAddress == rhs.emaInterfaceMACAddress) &&
            (lhs.emaNetworkGatewayMACAddress == rhs.emaNetworkGatewayMACAddress);
    }

    inline bool operator != (const NetworkInterfaceDescriptor & lhs, const NetworkInterfaceDescriptor & rhs)
    {
        return !(lhs == rhs);
    }
}

#endif  // INCL_NETWORK_INTERFACE_DESCRIPTOR_H