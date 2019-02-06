/*
 * ProtocolSetting.cpp
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

#include "ProtocolSetting.h"


namespace ACMNetProxy
{
    const char * const ProtocolSetting::getProxyMessageProtocolAsString (Protocol protocol)
    {
        switch (protocol) {
        case Protocol::PMP_UNDEF_MOCKETS:
            return "Mockets";
        case Protocol::PMP_MocketsRS:
            return "MocketsRS";
        case Protocol::PMP_MocketsUS:
            return "MocketsUS";
        case Protocol::PMP_MocketsRU:
            return "MocketsRU";
        case Protocol::PMP_MocketsUU:
            return "MocketsUU";
        case Protocol::PMP_TCP:
            return "TCP";
        case Protocol::PMP_UDP:
            return "UDP";
        case Protocol::PMP_UNDEF_CSR:
            return "CSR";
        case Protocol::PMP_CSRRS:
            return "CSRRS";
        case Protocol::PMP_CSRUS:
            return "CSRUS";
        case Protocol::PMP_CSRRU:
            return "CSRRU";
        case Protocol::PMP_CSRUU:
            return "CSRUU";
        }

        return "";
    }

    bool ProtocolSetting::isProtocolNameCorrect (const ci_string & sProtocolToCheck)
    {
        return (sProtocolToCheck == "mockets") || (sProtocolToCheck == "mocketsrs") || (sProtocolToCheck == "mocketsuu") ||
               (sProtocolToCheck == "mocketsus") || (sProtocolToCheck == "mocketsru") || (sProtocolToCheck == "tcp") ||
               (sProtocolToCheck == "udp") || (sProtocolToCheck == "csr") || (sProtocolToCheck == "csrrs") ||
               (sProtocolToCheck == "csruu") || (sProtocolToCheck == "csrus") || (sProtocolToCheck == "csrru");
    }

    Protocol ProtocolSetting::getProtocolFlagFromProtocolString (const ci_string & sProtocolName)
    {
        if (sProtocolName == "mockets") {
            return Protocol::PMP_UNDEF_MOCKETS;
        }
        if (sProtocolName == "mocketsrs") {
            return Protocol::PMP_MocketsRS;
        }
        if (sProtocolName == "mocketsuu") {
            return Protocol::PMP_MocketsUU;
        }
        if (sProtocolName == "mocketsus") {
            return Protocol::PMP_MocketsUS;
        }
        if (sProtocolName == "mocketsru") {
            return Protocol::PMP_MocketsRU;
        }
        if (sProtocolName == "tcp") {
            return Protocol::PMP_TCP;
        }
        if (sProtocolName == "udp") {
            return Protocol::PMP_UDP;
        }
        if (sProtocolName == "csr") {
            return Protocol::PMP_UNDEF_CSR;
        }
        if (sProtocolName == "csrrs") {
            return Protocol::PMP_CSRRS;
        }
        if (sProtocolName == "csruu") {
            return Protocol::PMP_CSRUU;
        }
        if (sProtocolName == "csrus") {
            return Protocol::PMP_CSRUS;
        }
        if (sProtocolName == "csrru") {
            return Protocol::PMP_CSRRU;
        }

        return Protocol::PMP_UNDEF_PROTOCOL;
    }

    ConnectorType ProtocolSetting::protocolToConnectorType (const Protocol protocol)
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

    Protocol ProtocolSetting::connectorTypeToProtocol (const ConnectorType connectorType)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return Protocol::PMP_TCP;
        case CT_UDPSOCKET:
            return Protocol::PMP_UDP;
        case CT_MOCKETS:
            return Protocol::PMP_UNDEF_MOCKETS;
        case CT_CSR:
            return Protocol::PMP_UNDEF_CSR;
        case CT_UNDEF:
            return Protocol::PMP_UNDEF_PROTOCOL;
        }

        return Protocol::PMP_UNDEF_PROTOCOL;
    }


    const ProtocolSetting ProtocolSetting::INVALID_PROTOCOL_SETTING{Protocol::PMP_UNDEF_PROTOCOL};
    const ProtocolSetting ProtocolSetting::DEFAULT_ICMP_PROTOCOL_SETTING{Protocol::PMP_UDP};
    const ProtocolSetting ProtocolSetting::DEFAULT_TCP_PROTOCOL_SETTING{Protocol::PMP_MocketsRS};
    const ProtocolSetting ProtocolSetting::DEFAULT_UDP_PROTOCOL_SETTING{Protocol::PMP_MocketsUU};
}
