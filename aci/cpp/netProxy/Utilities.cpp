/*
 * Utilities.cpp
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
*/

#include <iostream>
#include <iomanip>

#include "Utilities.h"
#include "ConfigurationParameters.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    char protocolTypeToChar (ProtocolType protocolType)
    {
        switch (protocolType) {
            case PT_TCP:
            {
                return 'T';
            }
            case PT_UDP:
            {
                return 'U';
            }
            case PT_ICMP:
            {
                return 'I';
            }
            case PT_UNDEF:
                break;
        }

        return -1;
    }

    char connectorTypeToChar (ConnectorType connectorType)
    {
        switch (connectorType) {
            case CT_TCPSOCKET:
            {
                return 'T';
            }
            case CT_UDPSOCKET:
            {
                return 'U';
            }
            case CT_MOCKETS:
            {
                return 'M';
            }
            case CT_CSR:
            {
                return 'C';
            }
            case CT_UNDEF:
                break;
        }

        return -1;
    }

    const char * const connectorTypeToString (ConnectorType connectorType)
    {
        switch (connectorType) {
            case CT_TCPSOCKET:
            {
                return sTCPSocket;
            }
            case CT_UDPSOCKET:
            {
                return sUDPSocket;
            }
            case CT_MOCKETS:
            {
                return sMocket;
            }
            case CT_CSR:
            {
                return sCSR;
            }
            case CT_UNDEF:
                break;
        }

        return sUNDEF;
    }

    const char * const encryptionTypeToString (EncryptionType encryptionType)
    {
        switch (encryptionType) {
            case ET_PLAIN:
            {
                return sET_PLAIN;
            }
            case ET_DTLS:
            {
                return sET_DTLS;
            }
            case ET_UNDEF:
            {
                return sET_UNDEF;
            }
        }
    }

    ConnectorType connectorTypeFromString (const String &sConnTypeName)
    {
        if (sConnTypeName ^= sTCP) {
            return CT_TCPSOCKET;
        }
        if (sConnTypeName ^= sUDP) {
            return CT_UDPSOCKET;
        }
        if ((sConnTypeName ^= sMocket) || (sConnTypeName ^= sMockets)) {
            return CT_MOCKETS;
        }
        if (sConnTypeName ^= sCSR) {
            return CT_CSR;
        }

        return CT_UNDEF;
    }

    const EtherMACAddr buildEthernetMACAddressFromString (const uint8 * const pszMACAddr)
    {
        EtherMACAddr etherMACAddr;
        etherMACAddr.ui8Byte1 = pszMACAddr[0];
        etherMACAddr.ui8Byte2 = pszMACAddr[1];
        etherMACAddr.ui8Byte3 = pszMACAddr[2];
        etherMACAddr.ui8Byte4 = pszMACAddr[3];
        etherMACAddr.ui8Byte5 = pszMACAddr[4];
        etherMACAddr.ui8Byte6 = pszMACAddr[5];

        return etherMACAddr;
    }

    const EtherMACAddr buildVirtualNetProxyEthernetMACAddress (const uint8 ui8Byte5, const uint8 ui8Byte6)
    {
        EtherMACAddr virtualEtherMACAddr;
        virtualEtherMACAddr.ui8Byte1 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE1;
        virtualEtherMACAddr.ui8Byte2 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE2;
        virtualEtherMACAddr.ui8Byte3 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE3;
        virtualEtherMACAddr.ui8Byte4 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE4;
        virtualEtherMACAddr.ui8Byte5 = ui8Byte5;
        virtualEtherMACAddr.ui8Byte6 = ui8Byte6;

        return virtualEtherMACAddr;
    }

    bool checkEtherMACAddressFormat (const char *pcEthernetMACAddress)
    {
        unsigned int uiByteLength = 0, uiMACLength = 0;
        while (*pcEthernetMACAddress) {
            if (*pcEthernetMACAddress == ':') {
                if (++uiMACLength > 5) {
                    return false;
                }
                ++pcEthernetMACAddress;
                uiByteLength = 0;
                continue;
            }
            ++uiByteLength;
            if (uiByteLength > 2) {
                return false;
            }
            if (!checkCharRange (*pcEthernetMACAddress, '0', '9') && !checkCharRange (*pcEthernetMACAddress, 'a', 'f') &&
                !checkCharRange (*pcEthernetMACAddress, 'A', 'F')) {
                return false;
            }
            ++pcEthernetMACAddress;
        }

        if (uiMACLength != 5) {
            return false;
        }
        return true;
    }

    void printBytes (std::ostream& out, const unsigned char *data, size_t dataLen, bool format)
    {
        out << std::setfill('0');
        for(size_t i = 0; i < dataLen; ++i) {
            out << std::hex << std::setw(2) << static_cast<int> (data[i]);
            if (format) {
                out << (((i + 1) % 25 == 0) ? "\n" : " ");
            }
        }
        out << std::endl;
    }

}
