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

#include <iostream>

#include "NLFLib.h"
#include "StrClass.h"
#include "InetAddr.h"
#include "net/NetworkHeaders.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    static const char * const sMocket = "Mocket";
    static const char * const sMockets = "Mockets";
    static const char * const sTCP = "TCP";
    static const char * const sTCPSocket = "TCPSocket";
    static const char * const sUDP = "UDP";
    static const char * const sUDPSocket = "UDPSocket";
    static const char * const sCSR = "CSR";
    static const char * const sUNDEF = "UNDEFINED_CONNECTOR";
    static const char * const sET_PLAIN = "PLAIN";
    static const char * const sET_DTLS = "DTLS";
    static const char * const sET_UNDEF = "UNDEFINED_ENCRYPTION";

    enum ProtocolType
    {
        PT_TCP, PT_UDP, PT_ICMP, PT_UNDEF
    };

    static const ProtocolType PT_AVAILABLE[] = {PT_TCP, PT_UDP, PT_ICMP};
    static const uint32 PT_SIZE = sizeof(PT_AVAILABLE) / sizeof(ProtocolType);


    enum ConnectorType
    {
        CT_TCPSOCKET, CT_UDPSOCKET, CT_MOCKETS, CT_CSR, CT_UNDEF
    };

    static const ConnectorType CT_AVAILABLE[] = {CT_TCPSOCKET, CT_UDPSOCKET, CT_MOCKETS, CT_CSR};
    static const uint32 CT_SIZE = sizeof(CT_AVAILABLE) / sizeof(ConnectorType);


    // EncryptionType values should be powers of 2 (0, 1, 2, 4, 8, ...)
    enum EncryptionType
    {
        ET_UNDEF = 0x00, ET_PLAIN = 0x01, ET_DTLS = 0x02
    };

    static const EncryptionType ET_AVAILABLE[] = {ET_PLAIN, ET_DTLS};
    static const uint32 ET_SIZE = sizeof(ET_AVAILABLE) / sizeof(EncryptionType);


    char protocolTypeToChar (ProtocolType protocolType);
    char connectorTypeToChar (ConnectorType connectorType);
    const char * const connectorTypeToString (ConnectorType connectorType);
    ConnectorType connectorTypeFromString (const NOMADSUtil::String &sConnTypeName);
    const char * const encryptionTypeToString (EncryptionType encryptionType);

    const EtherMACAddr buildEthernetMACAddressFromString (const uint8 * const pszMACAddr);
    const EtherMACAddr buildVirtualNetProxyEthernetMACAddress (const uint8 ui8Byte5, const uint8 ui8Byte6);
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

    inline static const bool isEncryptionTypeInDescriptor (unsigned char ucDescriptor, EncryptionType encryptionType)
    {
        return ucDescriptor & static_cast<unsigned char> (encryptionType);
    }

    inline static uint64 generateUInt64Key (const uint32 ui32IPAddr, ConnectorType connectorType)
    {
        return (static_cast<uint64> (ui32IPAddr) << 32) | static_cast<uint64> (connectorType);
    }

    inline static uint64 generateUInt64Key (const uint32 ui32IPAddr, const uint16 ui16PortNumber, EncryptionType encryptionType)
    {
        return (static_cast<uint64> (ui32IPAddr) << 32) | (static_cast<uint64> (ui16PortNumber) << 16) | static_cast<uint64> (encryptionType);
    }

    inline static uint64 generateUInt64Key (const NOMADSUtil::InetAddr * const pInetAddr, EncryptionType encryptionType)
    {
        return generateUInt64Key (pInetAddr->getIPAddress(), pInetAddr->getPort(), encryptionType);
    }

    inline static uint64 generateUInt64Key (const NOMADSUtil::InetAddr &rInetAddr, EncryptionType encryptionType)
    {
        return generateUInt64Key (rInetAddr.getIPAddress(), rInetAddr.getPort(), encryptionType);
    }

    inline bool checkCharRange (const char c, const char cMin, const char cMax)
    {
        return (c >= cMin) && (c <= cMax);
    }

    void printBytes (std::ostream& out, const unsigned char *data, size_t dataLen, bool format = true);
}
#endif  //INCL_UTILITIES_H
