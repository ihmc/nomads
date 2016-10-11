#ifndef INCL_UTILITIES_H
#define INCL_UTILITIES_H

/*
 * Utilities.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * File that gathers useful functions and definitions.
 */

#include "net/NetworkHeaders.h"
#include "NLFLib.h"
#include "StrClass.h"
#include "InetAddr.h"


namespace ACMNetProxy
{
    static const char * const sMocket = "Mocket";
    static const char * const sMockets = "Mockets";
    static const char * const sTCP = "TCP";
    static const char * const sTCPSocket = "TCPSocket";
    static const char * const sUDP = "UDP";
    static const char * const sUDPSocket = "UDPSocket";
    static const char * const sCSR = "CSR";
    static const char * const sUNDEF = "UNDEFINED";

    enum ProtocolType
    {
        PT_UNDEF, PT_TCP, PT_UDP, PT_ICMP
    };


    enum ConnectorType
    {
        CT_UNDEF, CT_MOCKETS, CT_SOCKET, CT_UDP, CT_CSR
    };


    char protocolTypeToChar (ProtocolType protocolType);
    char connectorTypeToChar (ConnectorType connectorType);
    const char * const connectorTypeToString (ConnectorType connectorType);
    ConnectorType connectorTypeFromString (const NOMADSUtil::String &sConnTypeName);
    void buildEthernetMACAddressFromString (NOMADSUtil::EtherMACAddr &eMACAddrToUpdate, const uint8 * const pszMACAddr);
    void buildVirtualNetProxyEthernetMACAddress (NOMADSUtil::EtherMACAddr &virtualEtherMACAddr, const uint8 ui8Byte5, const uint8 ui8Byte6);
    bool checkEtherMACAddressFormat (const char * const pcEthernetMACAddress);

    inline static bool isConnectorTypeNameCorrect (const NOMADSUtil::String &sConnTypeNameToCheck)
    {
        return (sConnTypeNameToCheck ^= sMocket) || (sConnTypeNameToCheck ^= sMockets) ||
               (sConnTypeNameToCheck ^= sTCP) || (sConnTypeNameToCheck ^= sUDP);
    }

    inline static bool isConnectorTypeNameCorrect (const char * const pcConnTypeNameToCheck)
    {
        return isConnectorTypeNameCorrect (NOMADSUtil::String (pcConnTypeNameToCheck));
    }

    inline static ConnectorType connectorTypeFromString (const char * const pProtocolName)
    {
        return connectorTypeFromString (NOMADSUtil::String (pProtocolName));
    }

    inline static uint64 generateUInt64Key (const uint32 ui32IPAddr, const uint16 ui16PortNumber)
    {
        return (((uint64) ui32IPAddr) << 32) | ((uint64) ui16PortNumber);
    }

    inline static uint64 generateUInt64Key (const NOMADSUtil::InetAddr * const pInetAddr)
    {
        return generateUInt64Key (pInetAddr->getIPAddress(), pInetAddr->getPort());
    }

    inline static uint64 generateUInt64Key (const NOMADSUtil::InetAddr &rInetAddr)
    {
        return generateUInt64Key (rInetAddr.getIPAddress(), rInetAddr.getPort());
    }

    inline bool checkCharRange (const char c, const char cMin, const char cMax)
    {
        return (c >= cMin) && (c <= cMax);
    }
}
#endif  //INCL_UTILITIES_H
