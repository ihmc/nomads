/*
 * NetworkAddressRange.cpp
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

#include <cstring>
#include <algorithm>

#include "NLFLib.h"
#include "EndianHelper.h"

#include "NetworkAddressRange.h"


namespace ACMNetProxy
{
    bool NetworkAddressRange::operator== (const NetworkAddressRange & rhs) const
    {
        for (int i = 0; i < 5; ++i) {
            if ((operator[](i)->getLowestEnd() != rhs.operator[] (i)->getLowestEnd()) ||
                (operator[](i)->getHighestEnd() != rhs.operator[] (i)->getHighestEnd())) {
                return false;
            }
        }

        return true;
    }

    int NetworkAddressRange::parseOctet (const char *pszOctet, URange<uint8> & ur8Range)
    {
        if (pszOctet[0] == '*') {
            ur8Range = {0,255};
        }
        else if (nullptr == strchr (pszOctet, '-')) {
            ur8Range = {static_cast<uint8> (atoi (pszOctet))};
        }
        else {
            char *pszTemp, *pszDupOctet = strdup (pszOctet);
            const char *pszToken;
            pszToken = NOMADSUtil::strtok_mt (pszDupOctet, "-", &pszTemp);
            if (pszToken == nullptr) {
                free (pszDupOctet);
                return -1;
            }
            uint8 ui8OctetLow = (uint8) atoi (pszToken);
            pszToken = NOMADSUtil::strtok_mt (nullptr, "", &pszTemp);
            if (pszToken == nullptr) {
                free (pszDupOctet);
                return -2;
            }
            uint8 ui8OctetHigh = (uint8) atoi (pszToken);
            free (pszDupOctet);

            ur8Range = {ui8OctetLow, ui8OctetHigh};
        }

        return 0;
    }

    int NetworkAddressRange::parse (char const * const pszDescriptor)
    {
        if (pszDescriptor == nullptr) {
            return -1;
        }

        auto sDescriptor = std::string{pszDescriptor}, sPort = std::string{""};
        if (std::count (sDescriptor.begin(), sDescriptor.end(), ':') > 1) {
            return -2;
        }

        // Parse IPv4 in format <A[-B].C[-D].E[-F].G[-H]>
        char *pszTemp, *pszDupDescriptor = strdup (pszDescriptor);
        const char *pszOctet[4], *pszPort;
        pszOctet[0] = NOMADSUtil::strtok_mt (pszDupDescriptor, ".", &pszTemp);
        pszOctet[1] = NOMADSUtil::strtok_mt (nullptr, ".", &pszTemp);
        pszOctet[2] = NOMADSUtil::strtok_mt (nullptr, ".", &pszTemp);
        pszOctet[3] = NOMADSUtil::strtok_mt (nullptr, ".:", &pszTemp);
        pszPort = NOMADSUtil::strtok_mt (nullptr, "", &pszTemp);
        for (int i = 0; i < 4; ++i) {
            if ((pszOctet[i] == nullptr) || (parseOctet (pszOctet[i], _ipAddr[i]))) {
                free (pszDupDescriptor);
                return -2 - i;
            }
        }

        // Parse PORT in format <P[-Q]>
        if (!pszPort || (pszPort[0] == '*')) {
            _portAddr = {0, 65535};
        }
        else if (nullptr == strchr (pszPort, '-')) {
            _portAddr = {static_cast<uint16> (atoi (pszPort))};
        }
        else {
            char *pszTemp2, *pszDupPort = strdup (pszPort);;
            const char *pszToken2;
            pszToken2 = NOMADSUtil::strtok_mt (pszDupPort, "-", &pszTemp2);
            if (pszToken2 == nullptr) {
                free (pszDupDescriptor);
                free (pszDupPort);
                return -7;
            }
            uint16 ui16PortLow = (uint16) atoi (pszToken2);
            pszToken2 = NOMADSUtil::strtok_mt (nullptr, "-", &pszTemp2);
            if (pszToken2 == nullptr) {
                free (pszDupDescriptor);
                free (pszDupPort);
                return -8;
            }
            uint16 ui16PortHigh = (uint16) atoi (pszToken2);
            free (pszDupPort);

            _portAddr = {ui16PortLow, ui16PortHigh};
        }
        free (pszDupDescriptor);

        return 0;
    }

    int NetworkAddressRangeDescriptor::parse (char const * const pszDescriptor)
    {
        int rc = NetworkAddressRange::parse (pszDescriptor);
        if (rc < 0) {
            reset();
        }
        else {
            updateStringRepresentations();
        }

        return rc;
    }

    void NetworkAddressRangeDescriptor::updateStringRepresentations (void)
    {
        std::ostringstream oss;
        oss << _ipAddr[0].getLowestEnd() << '.' << _ipAddr[1].getLowestEnd() << '.' << _ipAddr[2].getLowestEnd() <<
            '.' << _ipAddr[3].getLowestEnd() << ':' << _portAddr.getLowestEnd();
        _sLowestEndpointAddr = oss.str();

        oss.str ({});
        oss << _ipAddr[0].getHighestEnd() << '.' << _ipAddr[1].getHighestEnd() << '.' << _ipAddr[2].getHighestEnd() <<
            '.' << _ipAddr[3].getHighestEnd() << ':' << _portAddr.getHighestEnd();
        _sHighestEndpointAddr = oss.str();

        oss.str ({});
        oss << std::string {_ipAddr[0]} << '.' << std::string {_ipAddr[1]} << '.' << std::string {_ipAddr[2]} <<
            '.' << std::string {_ipAddr[3]} << ':' << std::string {_portAddr};
        _sNetworkAddressRangeRepresentation = oss.str();
    }

}
