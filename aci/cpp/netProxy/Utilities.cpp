/*
 * Utilities.cpp
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
*/

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
        }

        return -1;
    }

    char connectorTypeToChar (ConnectorType connectorType)
    {
        switch (connectorType) {
            case CT_MOCKETS:
            {
                return 'M';
            }
            case CT_SOCKET:
            {
                return 'T';
            }
            case CT_UDP:
            {
                return 'U';
            }

            case CT_CSR:
            {
                return 'C';
            }
        }

        return -1;
    }

    const char * const connectorTypeToString (ConnectorType connectorType)
    {
        switch (connectorType) {
            case CT_MOCKETS:
            {
                return sMocket;
            }

            case CT_SOCKET:
            {
                return sTCPSocket;
            }

            case CT_UDP:
            {
                return sUDPSocket;
            }

            case CT_CSR:
            {
                return sCSR;
            }
        }

        return sUNDEF;
    }

    ConnectorType connectorTypeFromString (const String &sConnTypeName)
    {
        if ((sConnTypeName ^= sMocket) || (sConnTypeName ^= sMockets)){
            return CT_MOCKETS;
        }
        if (sConnTypeName ^= sTCP) {
            return CT_SOCKET;
        }
        if (sConnTypeName ^= sUDP) {
            return CT_UDP;
        }
        if (sConnTypeName ^= sCSR) {
            return CT_CSR;
        }

        return CT_UNDEF;
    }

    void buildEthernetMACAddressFromString (EtherMACAddr &eMACAddrToUpdate, const uint8 * const pszMACAddr)
    {
        eMACAddrToUpdate.ui8Byte1 = pszMACAddr[0];
        eMACAddrToUpdate.ui8Byte2 = pszMACAddr[1];
        eMACAddrToUpdate.ui8Byte3 = pszMACAddr[2];
        eMACAddrToUpdate.ui8Byte4 = pszMACAddr[3];
        eMACAddrToUpdate.ui8Byte5 = pszMACAddr[4];
        eMACAddrToUpdate.ui8Byte6 = pszMACAddr[5];
    }

    void buildVirtualNetProxyEthernetMACAddress (EtherMACAddr &virtualEtherMACAddr, const uint8 ui8Byte5, const uint8 ui8Byte6)
    {
        virtualEtherMACAddr.ui8Byte1 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE1;
        virtualEtherMACAddr.ui8Byte2 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE2;
        virtualEtherMACAddr.ui8Byte3 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE3;
        virtualEtherMACAddr.ui8Byte4 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE4;
        virtualEtherMACAddr.ui8Byte5 = ui8Byte5;
        virtualEtherMACAddr.ui8Byte6 = ui8Byte6;
    }
}
