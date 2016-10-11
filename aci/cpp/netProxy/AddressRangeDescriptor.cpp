/*
 * AddressRangeDescriptor.cpp
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

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "NLFLib.h"

#include "AddressRangeDescriptor.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    AddressRangeDescriptor::AddressRangeDescriptor (uint32 ui32IPv4Address, uint16 ui16Port) :
        ui8IPAddrOctet1Low (ui32IPv4Address & 0x000000FFUL), ui8IPAddrOctet1High (ui8IPAddrOctet1Low),
        ui8IPAddrOctet2Low ((ui32IPv4Address >> 8) & 0x000000FFUL), ui8IPAddrOctet2High (ui8IPAddrOctet2Low),
        ui8IPAddrOctet3Low ((ui32IPv4Address >> 16) & 0x000000FFUL), ui8IPAddrOctet3High (ui8IPAddrOctet3Low),
        ui8IPAddrOctet4Low ((ui32IPv4Address >> 24) & 0x000000FFUL), ui8IPAddrOctet4High (ui8IPAddrOctet4Low),
        ui16PortLow (ui16Port), ui16PortHigh (ui16Port ? ui16Port : 65535)
    {
        char szBuffer[50];
        sprintf (szBuffer, "%hhu.%hhu.%hhu.%hhu:%hu", ui8IPAddrOctet1Low, ui8IPAddrOctet2Low, ui8IPAddrOctet3Low, ui8IPAddrOctet4Low, ui16PortLow);
        _sLowestEndpointAddr = szBuffer;
        sprintf (szBuffer, "%hhu.%hhu.%hhu.%hhu:%hu", ui8IPAddrOctet1High, ui8IPAddrOctet2High, ui8IPAddrOctet3High, ui8IPAddrOctet4High, ui16PortHigh);
        _sHighestEndpointAddr = szBuffer;
        _sAddressRangeDescriptor = szBuffer;
    }

    AddressRangeDescriptor & AddressRangeDescriptor::operator= (const AddressRangeDescriptor & rhs)
    {
        ui8IPAddrOctet1Low = rhs.ui8IPAddrOctet1Low;
        ui8IPAddrOctet1High = rhs.ui8IPAddrOctet1High;
        ui8IPAddrOctet2Low = rhs.ui8IPAddrOctet2Low;
        ui8IPAddrOctet2High = rhs.ui8IPAddrOctet2High;
        ui8IPAddrOctet3Low = rhs.ui8IPAddrOctet3Low;
        ui8IPAddrOctet3High = rhs.ui8IPAddrOctet3High;
        ui8IPAddrOctet4Low = rhs.ui8IPAddrOctet4Low;
        ui8IPAddrOctet4High = rhs.ui8IPAddrOctet4High;
        ui16PortLow = rhs.ui16PortLow;
        ui16PortHigh = rhs.ui16PortHigh;

        _sLowestEndpointAddr = rhs._sLowestEndpointAddr;
        _sHighestEndpointAddr = rhs._sHighestEndpointAddr;
        _sAddressRangeDescriptor = rhs._sAddressRangeDescriptor;

        return *this;
    }

    bool AddressRangeDescriptor::operator== (const AddressRangeDescriptor & rhs) const
    {
        return (ui8IPAddrOctet1Low == rhs.ui8IPAddrOctet1Low) && (ui8IPAddrOctet1High == rhs.ui8IPAddrOctet1High) &&
            (ui8IPAddrOctet2Low == rhs.ui8IPAddrOctet2Low) && (ui8IPAddrOctet2High == rhs.ui8IPAddrOctet2High) &&
            (ui8IPAddrOctet3Low == rhs.ui8IPAddrOctet3Low) && (ui8IPAddrOctet3High == rhs.ui8IPAddrOctet3High) &&
            (ui8IPAddrOctet4Low == rhs.ui8IPAddrOctet4Low) && (ui8IPAddrOctet4High == rhs.ui8IPAddrOctet4High) &&
            (ui16PortLow == rhs.ui16PortLow) && (ui16PortHigh == rhs.ui16PortHigh);
    }

    bool AddressRangeDescriptor::isSubsetOf (const AddressRangeDescriptor & rhs) const
    {
        return (ui8IPAddrOctet1Low >= rhs.ui8IPAddrOctet1Low) && (ui8IPAddrOctet1High <= rhs.ui8IPAddrOctet1High) &&
            (ui8IPAddrOctet2Low >= rhs.ui8IPAddrOctet2Low) && (ui8IPAddrOctet2High <= rhs.ui8IPAddrOctet2High) &&
            (ui8IPAddrOctet3Low >= rhs.ui8IPAddrOctet3Low) && (ui8IPAddrOctet3High <= rhs.ui8IPAddrOctet3High) &&
            (ui8IPAddrOctet4Low >= rhs.ui8IPAddrOctet4Low) && (ui8IPAddrOctet4High <= rhs.ui8IPAddrOctet4High) &&
            (ui16PortLow >= rhs.ui16PortLow) && (ui16PortHigh <= rhs.ui16PortHigh);
    }

    bool AddressRangeDescriptor::overlaps (const AddressRangeDescriptor & rhs) const
    {
        // Two address range overlaps if both all their octet ranges and their port range overlap
        return (((ui8IPAddrOctet1Low <= rhs.ui8IPAddrOctet1Low) && (ui8IPAddrOctet1High >= rhs.ui8IPAddrOctet1Low)) ||
            ((ui8IPAddrOctet1Low <= rhs.ui8IPAddrOctet1High) && (ui8IPAddrOctet1High >= rhs.ui8IPAddrOctet1High))) &&
            (((ui8IPAddrOctet2Low <= rhs.ui8IPAddrOctet2Low) && (ui8IPAddrOctet2High >= rhs.ui8IPAddrOctet2Low)) ||
            ((ui8IPAddrOctet2Low <= rhs.ui8IPAddrOctet2High) && (ui8IPAddrOctet2High >= rhs.ui8IPAddrOctet2High))) &&
            (((ui8IPAddrOctet3Low <= rhs.ui8IPAddrOctet3Low) && (ui8IPAddrOctet3High >= rhs.ui8IPAddrOctet3Low)) ||
            ((ui8IPAddrOctet3Low <= rhs.ui8IPAddrOctet3High) && (ui8IPAddrOctet3High >= rhs.ui8IPAddrOctet3High))) &&
            (((ui8IPAddrOctet4Low <= rhs.ui8IPAddrOctet4Low) && (ui8IPAddrOctet4High >= rhs.ui8IPAddrOctet4Low)) ||
            ((ui8IPAddrOctet4Low <= rhs.ui8IPAddrOctet4High) && (ui8IPAddrOctet4High >= rhs.ui8IPAddrOctet4High))) &&
            (((ui16PortLow <= rhs.ui16PortLow) && (ui16PortHigh >= rhs.ui16PortLow)) ||
            ((ui16PortLow <= rhs.ui16PortHigh) && (ui16PortHigh >= rhs.ui16PortHigh)));
    }

    bool AddressRangeDescriptor::matches (uint32 ui32IPAddr, uint16 ui16Port) const
    {
        // ui32IPAddr is in network byte order (big endian)
        uint8 ui8Octet1 = (uint8) (ui32IPAddr & 0x000000FFUL);
        uint8 ui8Octet2 = (uint8) ((ui32IPAddr >> 8) & 0x000000FFUL);
        uint8 ui8Octet3 = (uint8) ((ui32IPAddr >> 16) & 0x000000FFUL);
        uint8 ui8Octet4 = (uint8) ((ui32IPAddr >> 24) & 0x000000FFUL);

        return ((ui8IPAddrOctet1Low <= ui8Octet1) && (ui8Octet1 <= ui8IPAddrOctet1High) &&
                (ui8IPAddrOctet2Low <= ui8Octet2) && (ui8Octet2 <= ui8IPAddrOctet2High) &&
                (ui8IPAddrOctet3Low <= ui8Octet3) && (ui8Octet3 <= ui8IPAddrOctet3High) &&
                (ui8IPAddrOctet4Low <= ui8Octet4) && (ui8Octet4 <= ui8IPAddrOctet4High) &&
                matchesPort (ui16Port));
    }

    int AddressRangeDescriptor::parseOctet (const char *pszOctet, uint8 *pui8OctetLow, uint8 *pui8OctetHigh)
    {
        if (pszOctet[0] == '*') {
            *pui8OctetLow = 0;
            *pui8OctetHigh = 255;
        }
        else if (NULL == strchr (pszOctet, '-')) {
            *pui8OctetLow = *pui8OctetHigh = (uint8) atoi (pszOctet);
        }
        else {
            char *pszTemp, *pszDupOctet = strdup (pszOctet);
            const char *pszToken;
            pszToken = strtok_mt (pszDupOctet, "-", &pszTemp);
            if (pszToken == NULL) {
                free (pszDupOctet);
                return -1;
            }
            *pui8OctetLow = (uint8) atoi (pszToken);
            pszToken = strtok_mt (NULL, "", &pszTemp);
            if (pszToken == NULL) {
                free (pszDupOctet);
                return -2;
            }
            *pui8OctetHigh = (uint8) atoi (pszToken);
            free (pszDupOctet);
        }

        return 0;
    }

    int AddressRangeDescriptor::parse (char const * const pszDescriptor)
    {
        if (pszDescriptor == NULL) {
            return -1;
        }

        // Parse IPv4 in format <A[-B].C[-D].E[-F].G[-H]>
        char *pszTemp, *pszDupDescriptor = strdup (pszDescriptor);
        const char *pszOctet1, *pszOctet2, *pszOctet3, *pszOctet4, *pszPort;
        pszOctet1 = strtok_mt (pszDupDescriptor, ".", &pszTemp);
        pszOctet2 = strtok_mt (NULL, ".", &pszTemp);
        pszOctet3 = strtok_mt (NULL, ".", &pszTemp);
        pszOctet4 = strtok_mt (NULL, ".:", &pszTemp);
        pszPort = strtok_mt (NULL, "", &pszTemp);
        if ((pszOctet1 == NULL) || (parseOctet (pszOctet1, &ui8IPAddrOctet1Low, &ui8IPAddrOctet1High))) {
            free (pszDupDescriptor);
            return -2;
        }
        if ((pszOctet2 == NULL) || (parseOctet (pszOctet2, &ui8IPAddrOctet2Low, &ui8IPAddrOctet2High))) {
            free (pszDupDescriptor);
            return -3;
        }
        if ((pszOctet3 == NULL) || (parseOctet (pszOctet3, &ui8IPAddrOctet3Low, &ui8IPAddrOctet3High))) {
            free (pszDupDescriptor);
            return -4;
        }
        if ((pszOctet4 == NULL) || (parseOctet (pszOctet4, &ui8IPAddrOctet4Low, &ui8IPAddrOctet4High))) {
            free (pszDupDescriptor);
            return -5;
        }

        // Parse PORT in format <P[-Q]>
        if (!pszPort || (pszPort[0] == '*')) {
            ui16PortLow = 0;
            ui16PortHigh = 65535;
        }
        else if (NULL == strchr (pszPort, '-')) {
            ui16PortLow = ui16PortHigh = (uint16) atoi (pszPort);
        }
        else {
            char *pszTemp2, *pszDupPort = strdup (pszPort);;
            const char *pszToken2;
            pszToken2 = strtok_mt (pszDupPort, "-", &pszTemp2);
            if (pszToken2 == NULL) {
                free (pszDupDescriptor);
                free (pszDupPort);
                return -7;
            }
            ui16PortLow = (uint16) atoi (pszToken2);
            pszToken2 = strtok_mt (NULL, "-", &pszTemp2);
            if (pszToken2 == NULL) {
                free (pszDupDescriptor);
                free (pszDupPort);
                return -8;
            }
            ui16PortHigh = (uint16) atoi (pszToken2);
            free (pszDupPort);
        }
        free (pszDupDescriptor);

        char szBuffer[50];
        sprintf (szBuffer, "%hhu.%hhu.%hhu.%hhu:%hu", ui8IPAddrOctet1Low, ui8IPAddrOctet2Low, ui8IPAddrOctet3Low, ui8IPAddrOctet4Low, ui16PortLow);
        _sLowestEndpointAddr = szBuffer;
        sprintf (szBuffer, "%hhu.%hhu.%hhu.%hhu:%hu", ui8IPAddrOctet1High, ui8IPAddrOctet2High, ui8IPAddrOctet3High, ui8IPAddrOctet4High, ui16PortHigh);
        _sHighestEndpointAddr = szBuffer;
        _sAddressRangeDescriptor = pszDescriptor;

        return 0;
    }

    void AddressRangeDescriptor::reset (void)
    {
        ui8IPAddrOctet1Low = 0;
        ui8IPAddrOctet1High = 0;
        ui8IPAddrOctet2Low = 0;
        ui8IPAddrOctet2High = 0;
        ui8IPAddrOctet3Low = 0;
        ui8IPAddrOctet3High = 0;
        ui8IPAddrOctet4Low = 0;
        ui8IPAddrOctet4High = 0;
        ui16PortLow = 0;
        ui16PortHigh = 0;

        _sLowestEndpointAddr.setSize (0);
        _sHighestEndpointAddr.setSize (0);
        _sAddressRangeDescriptor.setSize (0);
    }

}
