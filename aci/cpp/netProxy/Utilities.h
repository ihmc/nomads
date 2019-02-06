#ifndef INCL_UTILITIES_H
#define INCL_UTILITIES_H

/*
 * Utilities.h
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
 *
 * File that gathers useful functions and definitions.
 */

#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <functional>

#include "NLFLib.h"
#include "net/NetworkHeaders.h"


inline constexpr int8 operator "" _c (unsigned long long value)
{
    return static_cast<int8> (value);
}

inline constexpr uint8 operator "" _uc (unsigned long long value)
{
    return static_cast<uint8> (value);
}

inline constexpr int16 operator "" _s (unsigned long long value)
{
    return static_cast<int16> (value);
}

inline constexpr uint16 operator "" _us (unsigned long long value)
{
    return static_cast<uint16> (value);
}

template <typename T, typename... Args> std::unique_ptr<T> make_unique (Args && ... args) {
    return std::unique_ptr<T>{new T{std::forward<Args> (args)...}};
}

template <typename M, typename V, typename FilterFunction>
void MapToVec_if (const M & map, V & vector, const FilterFunction & f_filter) {
    for (const auto & pair : map) {
        if (f_filter (pair)) {
            vector.push_back (pair.second);
        }
    }
}

std::vector<std::string> splitStringToVector (const std::string & s, char cDelim, const std::function<bool(const std::string &)> & fValidator =
                                                [](const std::string & s) { return true; }, bool bTrimWhitespaces = true);

std::vector<std::string> splitStringToVector (const std::string & s, char cDelim, const unsigned int uiMaxElements,
                                              const std::function<bool(const std::string &)> & fValidator =
                                                [](const std::string & s) { return true; },
                                              bool bTrimWhitespaces = true);

template <typename T>
std::string vectorToString (const std::vector<T> & vTs, const std::function<std::string(const T &)> & fConverter = [](const T & rT) { return rT; },
                            const std::string & sBeginDelimiter = "[", const std::string & sEndDelimiter = "]", const std::string & sSeparator = ", ")
{
    std::ostringstream oss;

    oss << sBeginDelimiter;
    if (vTs.size() > 0) {
        oss << fConverter (vTs[0]);
    }
    for (unsigned int i = 1; i < vTs.size(); ++i) {
        oss << sSeparator << fConverter (vTs[i]);
    }
    oss << sEndDelimiter;

    return oss.str();
}

struct ci_char_traits : public std::char_traits<char> {
    static bool eq (char c1, char c2) { return toupper(c1) == toupper(c2); }
    static bool ne (char c1, char c2) { return toupper(c1) != toupper(c2); }
    static bool lt (char c1, char c2) { return toupper(c1) <  toupper(c2); }
    static int compare (const char* s1, const char* s2, size_t n) {
        while (n-- != 0) {
            if (toupper (*s1) < toupper (*s2)) return -1;
            if (toupper (*s1) > toupper (*s2)) return 1;
            ++s1; ++s2;
        }
        return 0;
    }
    static const char* find (const char* s, int n, char a) {
        while ((n-- > 0) && (toupper(*s) != toupper(a))) {
            ++s;
        }
        return s;
    }
};

typedef std::basic_string<char, ci_char_traits> ci_string;

namespace ACMNetProxy
{
    constexpr const char * const sMocket = "Mocket";
    constexpr const char * const sMockets = "Mockets";
    constexpr const char * const sTCP = "TCP";
    constexpr const char * const sUDP = "UDP";
    constexpr const char * const sCSR = "CSR";
    constexpr const char * const sUNDEF = "UNDEFINED_CONNECTOR";
    constexpr const char * const sET_PLAIN = "PLAIN";
    constexpr const char * const sET_DTLS = "DTLS";
    constexpr const char * const sET_UNDEF = "UNDEFINED_ENCRYPTION";

    enum ProtocolType
    {
        PT_TCP, PT_UDP, PT_ICMP, PT_UNDEF
    };

    static const ProtocolType PT_AVAILABLE[] = {PT_TCP, PT_UDP, PT_ICMP};
    static const uint32 PT_SIZE = sizeof(PT_AVAILABLE) / sizeof(ProtocolType);


    enum ConnectorType : uint8
    {
        CT_TCPSOCKET, CT_UDPSOCKET, CT_MOCKETS, CT_CSR, CT_UNDEF
    };

    static const ConnectorType CT_AVAILABLE[] = {CT_TCPSOCKET, CT_UDPSOCKET, CT_MOCKETS, CT_CSR};
    static const uint32 CT_SIZE = sizeof(CT_AVAILABLE) / sizeof(ConnectorType);


    // EncryptionType values should be powers of 2 (0, 1, 2, 4, 8, ...)
    enum EncryptionType : uint8
    {
        ET_UNDEF = 0x00, ET_PLAIN = 0x01, ET_DTLS = 0x02
    };

    static const EncryptionType ET_AVAILABLE[] = {ET_PLAIN, ET_DTLS};
    static const uint32 ET_SIZE = sizeof(ET_AVAILABLE) / sizeof(EncryptionType);

    bool hasEnding (std::string const & fullString, std::string const & ending);

    inline const char * nullprtToEmptyString (const char * const s)
    {
        return (s == nullptr) ? "" : s;
    }

    uint64 etherMACAddrTouint64 (const NOMADSUtil::EtherMACAddr & etherMACAddr);
    std::string etherMACAddrToString (const uint8 * const pui8EtherMACAddr);
    std::string etherMACAddrToString (const NOMADSUtil::EtherMACAddr & etherMACAddr);
    char protocolTypeToChar (ProtocolType protocolType);
    char connectorTypeToChar (ConnectorType connectorType);
    const char * const connectorTypeToString (ConnectorType connectorType);
    ConnectorType connectorTypeFromString (const std::string & sConnTypeName);
    const char * const encryptionTypeToString (EncryptionType encryptionType);

    const NOMADSUtil::EtherMACAddr buildVirtualNetProxyEthernetMACAddress (const uint8 ui8Byte5, const uint8 ui8Byte6);
    const NOMADSUtil::EtherMACAddr buildEthernetMACAddressFromArray (const uint8 * const pszMACAddr);
    const NOMADSUtil::EtherMACAddr buildEthernetMACAddressFromString (const char * const pcEtherMACAddressString);
    bool checkEtherMACAddressFormat (const char * const pcEthernetMACAddress);
    bool checkIPv4AddressFormat (const std::string & sIPv4Address);

    std::pair<std::string, std::string> splitIPAndPort (const std::string & ipPortAddr);

    inline bool isConnectorTypeNameCorrect (const std::string & sConnTypeNameToCheck)
    {
        ci_string compareableString{sConnTypeNameToCheck.c_str()};
        return (compareableString == sMocket) || (compareableString == sMockets) ||
               (compareableString == sTCP) || (compareableString == sUDP);
    }

    inline bool isConnectorTypeNameCorrect (const char * const pcConnTypeNameToCheck)
    {
        return isConnectorTypeNameCorrect (std::string {nullprtToEmptyString (pcConnTypeNameToCheck)});
    }

    inline ConnectorType connectorTypeFromString (const char * const pProtocolName)
    {
        return connectorTypeFromString (std::string {nullprtToEmptyString (pProtocolName)});
    }

    inline const bool isEncryptionTypeInDescriptor (unsigned char ucDescriptor, EncryptionType encryptionType)
    {
        return ucDescriptor & static_cast<unsigned char> (encryptionType);
    }

    inline bool isMulticastIPv4Address (uint32 ui32IPAddr)
    {
        return (ui32IPAddr & 0b11100000) == 0b11100000;
    }

    inline uint64 generateUInt64Key (const uint32 ui32NetProxyUniqueID, uint32 ui32IPv4Address)
    {
        return (static_cast<uint64> (ui32NetProxyUniqueID) << 32) | static_cast<uint64> (ui32NetProxyUniqueID);
    }

    inline uint64 generateUInt64Key (const uint32 ui32IPAddr, ConnectorType connectorType)
    {
        return (static_cast<uint64> (ui32IPAddr) << 32) | static_cast<uint64> (connectorType);
    }

    inline uint64 generateUInt64Key (const uint32 ui32IPAddr, const uint16 ui16PortNumber,
                                     const ConnectorType connectorType, EncryptionType encryptionType)
    {
        return (static_cast<uint64> (ui32IPAddr) << 32) | (static_cast<uint64> (ui16PortNumber) << 16) |
            (static_cast<uint64> (connectorType) << 8) | static_cast<uint64> (encryptionType);
    }

    inline bool checkCharRange (const char c, const char cMin, const char cMax)
    {
        return (c >= cMin) && (c <= cMax);
    }

    void printBytes (std::ostream & out, const unsigned char * const data, size_t dataLen, bool format = true);
    std::string stringSetToString (std::unordered_set<std::string> usStrings, const std::string & sBeginDelimiter = "{",
                                   const std::string & sEndDelimiter = "}", const std::string & sSeparator = ", ");
}
#endif  //INCL_UTILITIES_H
