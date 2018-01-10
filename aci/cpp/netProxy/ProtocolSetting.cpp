/*
 * ProtocolSetting.cpp
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
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "ProtocolSetting.h"


namespace ACMNetProxy
{
    const char * const ProtocolSetting::getProxyMessageProtocolAsString (ProxyMessage::Protocol protocol)
    {
        switch (protocol) {
            case ProxyMessage::PMP_UNDEF_MOCKETS:
                return "Mockets";
            case ProxyMessage::PMP_MocketsRS:
                return "MocketsRS";
            case ProxyMessage::PMP_MocketsUS:
                return "MocketsUS";
            case ProxyMessage::PMP_MocketsRU:
                return "MocketsRU";
            case ProxyMessage::PMP_MocketsUU:
                return "MocketsUU";
            case ProxyMessage::PMP_TCP:
                return "TCP";
            case ProxyMessage::PMP_UDP:
                return "UDP";
            case ProxyMessage::PMP_UNDEF_CSR:
                return "CSR";
            case ProxyMessage::PMP_CSRRS:
                return "CSRRS";
            case ProxyMessage::PMP_CSRUS:
                return "CSRUS";
            case ProxyMessage::PMP_CSRRU:
                return "CSRRU";
            case ProxyMessage::PMP_CSRUU:
                return "CSRUU";
        }

        return (char *) 0;
    }

    bool ProtocolSetting::isProtocolNameCorrect (const char * const pcProtocolToCheck)
    {
        NOMADSUtil::String sProtocolToCheck (pcProtocolToCheck);
        sProtocolToCheck.convertToLowerCase();

        return (sProtocolToCheck == "mockets") || (sProtocolToCheck == "mocketsrs") || (sProtocolToCheck == "mocketsuu") ||
               (sProtocolToCheck == "mocketsus") || (sProtocolToCheck == "mocketsru") || (sProtocolToCheck == "tcp") ||
               (sProtocolToCheck == "udp") || (sProtocolToCheck == "csr") || (sProtocolToCheck == "csrrs") ||
               (sProtocolToCheck == "csruu") || (sProtocolToCheck == "csrus") || (sProtocolToCheck == "csrru");
    }

    ProxyMessage::Protocol ProtocolSetting::getProtocolFlagFromProtocolString (const char * const pProtocolName)
    {
        NOMADSUtil::String sProtocolString (pProtocolName);
        sProtocolString.convertToLowerCase();

        if (sProtocolString == "mockets") {
            return ProxyMessage::PMP_UNDEF_MOCKETS;
        }
        if (sProtocolString == "mocketsrs") {
            return ProxyMessage::PMP_MocketsRS;
        }
        if (sProtocolString == "mocketsuu") {
            return ProxyMessage::PMP_MocketsUU;
        }
        if (sProtocolString == "mocketsus") {
            return ProxyMessage::PMP_MocketsUS;
        }
        if (sProtocolString == "mocketsru") {
            return ProxyMessage::PMP_MocketsRU;
        }
        if (sProtocolString == "tcp") {
            return ProxyMessage::PMP_TCP;
        }
        if (sProtocolString == "udp") {
            return ProxyMessage::PMP_UDP;
        }
        if (sProtocolString == "csr") {
            return ProxyMessage::PMP_UNDEF_CSR;
        }
        if (sProtocolString == "csrrs") {
            return ProxyMessage::PMP_CSRRS;
        }
        if (sProtocolString == "csruu") {
            return ProxyMessage::PMP_CSRUU;
        }
        if (sProtocolString == "csrus") {
            return ProxyMessage::PMP_CSRUS;
        }
        if (sProtocolString == "csrru") {
            return ProxyMessage::PMP_CSRRU;
        }

        return ProxyMessage::PMP_UNDEF_PROTOCOL;
    }

    ConnectorType ProtocolSetting::protocolToConnectorType (const ProxyMessage::Protocol protocol)
    {
        if (isTCPProtocol (protocol)) {
            return CT_TCPSOCKET;
        }
        if (isUDPProtocol (protocol)) {
            return CT_UDPSOCKET;
        }
        if (isMocketsProtocol (protocol)) {
            return CT_MOCKETS;
        }
        if (isCSRProtocol (protocol)) {
            return CT_CSR;
        }

        return CT_UNDEF;
    }

    ProxyMessage::Protocol ProtocolSetting::connectorTypeToProtocol (const ConnectorType connectorType)
    {
        switch (connectorType) {
            case CT_TCPSOCKET:
                return ProxyMessage::PMP_TCP;
            case CT_UDPSOCKET:
                return ProxyMessage::PMP_UDP;
            case CT_MOCKETS:
                return ProxyMessage::PMP_UNDEF_MOCKETS;
            case CT_CSR:
                return ProxyMessage::PMP_UNDEF_CSR;
            case CT_UNDEF:
                return ProxyMessage::PMP_UNDEF_PROTOCOL;
        }

        return ProxyMessage::PMP_UNDEF_PROTOCOL;
    }
    
}
