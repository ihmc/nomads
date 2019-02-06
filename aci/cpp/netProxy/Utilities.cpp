/*
 * Utilities.cpp
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
*/

#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>

#if defined (WIN32)
#include <Ws2tcpip.h>
#elif defined (UNIX)
#include <arpa/inet.h>
#endif

#include "Utilities.h"
#include "ConfigurationParameters.h"


std::string trim (const std::string & str, const std::string & whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of (whitespace);
    if (strBegin == std::string::npos) {
        // Whitespaces only
        return "";
    }

    const auto strEnd = str.find_last_not_of (whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr (strBegin, strRange);
}

template<typename Out>
void splitStringToOut (const std::string &s, char cDelim, Out result,
                       std::function<bool(const std::string &)> fValidator,
                       const unsigned int uiMaxElements, bool bTrimWhitespaces)
{
    std::stringstream ss{s};
    std::string item;
    long long iElementsLeft = uiMaxElements;
    while ((--iElementsLeft != 0) && std::getline (ss, item, cDelim)) {
        item = bTrimWhitespaces ? trim (item) : item;
        if (fValidator (item)) {
            *(result++) = item;
        }
    }

    if (uiMaxElements > 0) {
        std::getline (ss, item);
        item = bTrimWhitespaces ? trim (item) : item;
        if (fValidator (item)) {
            *(result++) = item;
        }
    }
}

std::vector<std::string> splitStringToVector (const std::string & s, char cDelim,
                                              const std::function<bool(const std::string &)> & fValidator,
                                              bool bTrimWhitespaces)
{
    std::vector<std::string> elems;
    splitStringToOut (s, cDelim, std::back_inserter (elems), fValidator, 0, bTrimWhitespaces);

    return elems;
}

std::vector<std::string> splitStringToVector (const std::string & s, char cDelim, const unsigned int uiMaxElements,
                                              const std::function<bool(const std::string &)> & fValidator, bool bTrimWhitespaces)
{
    std::vector<std::string> elems;
    splitStringToOut (s, cDelim, std::back_inserter (elems), fValidator, uiMaxElements, bTrimWhitespaces);

    return elems;
}

namespace ACMNetProxy
{
    uint64 etherMACAddrTouint64 (const NOMADSUtil::EtherMACAddr & etherMACAddr)
    {
        uint64 ui64MacAddr = 0;
        const unsigned char * const ucMACAddr = reinterpret_cast<const unsigned char *> (&etherMACAddr);
        unsigned char * const uc64BitsMACAddr = reinterpret_cast<unsigned char *> (&ui64MacAddr);

        for (int i = 0; i < sizeof(NOMADSUtil::EtherMACAddr); ++i) {
            uc64BitsMACAddr[i] = ucMACAddr[i];
        }

        return ui64MacAddr;
    }

    std::string etherMACAddrToString (const NOMADSUtil::EtherMACAddr & etherMACAddr)
    {
        std::ostringstream oss;
        oss.fill ('0');

        oss << std::hex << std::setw (2) << static_cast<int> (etherMACAddr.ui8Byte1);
        oss << ':' << std::hex << std::setw (2) << static_cast<int> (etherMACAddr.ui8Byte2);
        oss << ':' << std::hex << std::setw (2) << static_cast<int> (etherMACAddr.ui8Byte3);
        oss << ':' << std::hex << std::setw (2) << static_cast<int> (etherMACAddr.ui8Byte4);
        oss << ':' << std::hex << std::setw (2) << static_cast<int> (etherMACAddr.ui8Byte5);
        oss << ':' << std::hex << std::setw (2) << static_cast<int> (etherMACAddr.ui8Byte6);

        return oss.str();
    }

    std::string etherMACAddrToString (const uint8 * const pui8EtherMACAddr)
    {
        if (pui8EtherMACAddr == nullptr) {
            return "EE:RR:RR:OO:RR:!!";
        }

        std::ostringstream oss;
        oss.fill ('0');
        oss << std::hex << std::setw (2) << static_cast<int> (pui8EtherMACAddr[0]);
        for (unsigned int i = 1; i < 6; ++i) {
            oss << ':' << std::hex << std::setw (2) << static_cast<int> (pui8EtherMACAddr[i]);
        }

        return oss.str();
    }

    bool hasEnding (std::string const & fullString, std::string const & ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        }

        return false;
    }

    char protocolTypeToChar (ProtocolType protocolType)
    {
        switch (protocolType) {
        case PT_TCP:
            return 'T';
        case PT_UDP:
            return 'U';
        case PT_ICMP:
            return 'I';
        case PT_UNDEF:
            break;
        }

        return -1;
    }

    char connectorTypeToChar (ConnectorType connectorType)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return 'T';
        case CT_UDPSOCKET:
            return 'U';
        case CT_MOCKETS:
            return 'M';
        case CT_CSR:
            return 'C';
        case CT_UNDEF:
            break;
        }

        return -1;
    }

    const char * const connectorTypeToString (ConnectorType connectorType)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return sTCP;
        case CT_UDPSOCKET:
            return sUDP;
        case CT_MOCKETS:
            return sMocket;
        case CT_CSR:
            return sCSR;
        case CT_UNDEF:
            break;
        }

        return sUNDEF;
    }

    const char * const encryptionTypeToString (EncryptionType encryptionType)
    {
        switch (encryptionType) {
        case ET_PLAIN:
            return sET_PLAIN;
        case ET_DTLS:
            return sET_DTLS;
        default:
            return sET_UNDEF;
        }
    }

    ConnectorType connectorTypeFromString (const std::string & sConnTypeName)
    {
        ci_string comparableString{sConnTypeName.c_str()};
        if (comparableString == sTCP) {
            return CT_TCPSOCKET;
        }
        if (comparableString == sUDP) {
            return CT_UDPSOCKET;
        }
        if ((comparableString == sMocket) || (comparableString == sMockets)) {
            return CT_MOCKETS;
        }
        if (comparableString == sCSR) {
            return CT_CSR;
        }

        return CT_UNDEF;
    }

    const NOMADSUtil::EtherMACAddr buildVirtualNetProxyEthernetMACAddress (const uint8 ui8Byte5, const uint8 ui8Byte6)
    {
        NOMADSUtil::EtherMACAddr virtualEtherMACAddr;
        virtualEtherMACAddr.ui8Byte1 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE1;
        virtualEtherMACAddr.ui8Byte2 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE2;
        virtualEtherMACAddr.ui8Byte3 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE3;
        virtualEtherMACAddr.ui8Byte4 = NetProxyApplicationParameters::VIRT_MAC_ADDR_BYTE4;
        virtualEtherMACAddr.ui8Byte5 = ui8Byte5;
        virtualEtherMACAddr.ui8Byte6 = ui8Byte6;

        return virtualEtherMACAddr;
    }

    const NOMADSUtil::EtherMACAddr buildEthernetMACAddressFromArray (const uint8 * const pszMACAddr)
    {
        if (pszMACAddr == nullptr) {
            return NetProxyApplicationParameters::EMA_INVALID_ADDRESS;
        }

        NOMADSUtil::EtherMACAddr etherMACAddr;
        etherMACAddr.ui8Byte1 = pszMACAddr[0];
        etherMACAddr.ui8Byte2 = pszMACAddr[1];
        etherMACAddr.ui8Byte3 = pszMACAddr[2];
        etherMACAddr.ui8Byte4 = pszMACAddr[3];
        etherMACAddr.ui8Byte5 = pszMACAddr[4];
        etherMACAddr.ui8Byte6 = pszMACAddr[5];

        return etherMACAddr;
    }

    const NOMADSUtil::EtherMACAddr buildEthernetMACAddressFromString (const char * const pcEtherMACAddressString)
    {
        static const unsigned int MAC_ADDRESS_SIZE = 6;
        static const char AC_SEPARATOR[] = ":";
        const char *pszMACAddressOctets[MAC_ADDRESS_SIZE];
        char *pszTemp;
        auto res = NetProxyApplicationParameters::EMA_INVALID_ADDRESS;

        if (!checkEtherMACAddressFormat (pcEtherMACAddressString)) {
            return res;
        }
        // Parse Ethernet MAC address in the format A:B:C:D:E:F
        unsigned int i = 0;
        if ((pszMACAddressOctets[i++] = NOMADSUtil::strtok_mt (pcEtherMACAddressString, AC_SEPARATOR, &pszTemp)) == nullptr) {
            return res;
        }
        for (; i < MAC_ADDRESS_SIZE; ++i) {
            pszMACAddressOctets[i] = NOMADSUtil::strtok_mt (nullptr, AC_SEPARATOR, &pszTemp);
            if (pszMACAddressOctets[i] == nullptr) {
                return res;
            }
        }

        auto * const pui8EtherMACAddress = reinterpret_cast<uint8*> (&res);
        for (i = 0; i < MAC_ADDRESS_SIZE; i += 2) {
            pui8EtherMACAddress[i] = static_cast<uint8> (std::strtoul (pszMACAddressOctets[i + 1], 0, 16));
            pui8EtherMACAddress[i + 1] = static_cast<uint8> (std::strtoul (pszMACAddressOctets[i], 0, 16));
        }

        return res;
    }

    bool checkEtherMACAddressFormat (const char * pcEthernetMACAddress)
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

    bool checkIPv4AddressFormat (const std::string & sIPv4Address)
    {
        char cIPv4AddressRepresentation[4];

        return inet_pton(AF_INET, sIPv4Address.c_str(), cIPv4AddressRepresentation) == 1;
    }

    std::pair<std::string, std::string> splitIPAndPort (const std::string & ipPortAddr)
    {
        auto res = std::make_pair (std::string{}, std::string{});
        if (std::count (ipPortAddr.begin(), ipPortAddr.end(), ':') != 1) {
            return res;
        }

        auto vec (splitStringToVector (ipPortAddr, ':'));
        res.first = vec[0];
        res.second = vec[1];

        return res;
    }

    void printBytes (std::ostream & out, const unsigned char * const data, size_t dataLen, bool format)
    {
        out << std::setfill ('0');
        for(size_t i = 0; i < dataLen; ++i) {
            out << std::hex << std::setw(2) << static_cast<int> (data[i]);
            if (format) {
                out << (((i + 1) % 25 == 0) ? "\n" : " ");
            }
        }
        out << std::endl;
    }

    std::string stringSetToString (std::unordered_set<std::string> usStrings, const std::string & sBeginDelimiter,
                                   const std::string & sEndDelimiter, const std::string & sSeparator)
    {
        std::ostringstream oss;

        oss << sBeginDelimiter;
        for (const auto & sString : usStrings) {
            oss << sString << sSeparator;
        }
        oss.seekp (-sSeparator.length(), std::ios_base::end);
        oss << sEndDelimiter;

        return oss.str();
    }

}
