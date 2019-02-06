#ifndef INCL_ADDRESS_RANGE_DESCRIPTOR_H
#define INCL_ADDRESS_RANGE_DESCRIPTOR_H

/*
 * NetworkAddressRange.h
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
 *
 * The NetworkAddressRange class stores the two endpoints of a range
 * of IP:port pairs and it defines useful methods to check, for instance,
 * if a range is valid or if it contains a specific IP:port pair.
 * In addition, the parse() method can parse a line in the configuration
 * files to initialize the object attributes with the configured values.
 *
 * The NetworkAddressRangeDescriptor class adds string representations
 * of the NetworkAddressRange object it inherits from.
 */

#include <string>
#include <limits>

#include "FTypes.h"
#include "EndianHelper.h"

#include "Range.h"
#include "Utilities.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif


namespace ACMNetProxy
{
    class NetworkAddressRange
    {
        using IPv4AddressRange = URange<uint8>[4];
        using PortNumberRange = URange<uint16>;


    public:
        NetworkAddressRange (void);
        NetworkAddressRange (char const * const pszDescriptor);
        NetworkAddressRange (uint32 ui32IPv4Address, uint16 ui16Port);
        NetworkAddressRange (uint32 ui32IPv4Address, PortNumberRange portRange);

        bool operator== (const NetworkAddressRange & rhs) const;

        Range * operator[] (unsigned int index);
        const Range * operator[] (unsigned int index) const;

        bool isValid (void) const;
        bool isAnIPRange (void) const;
        bool isAPortRange (void) const;
        bool overlaps (const NetworkAddressRange & rhs) const;
        bool contains (const NetworkAddressRange & rhs, unsigned int uIndex = 0) const;
        bool containsIgnorePort (const NetworkAddressRange & rhs, unsigned int uIndex = 0) const;
        bool matches (uint32 ui32IPv4Addr, uint16 ui16Port, unsigned int uIndex = 0) const;
        bool matchesAddress (uint32 ui32IPv4Addr) const;
        bool matchesAddressForAllPorts (uint32 ui32IPv4Addr) const;
        bool matchesPort (uint16 ui16Port) const;
        uint64 getNumberOfAddressesInRange (void) const;

        uint32 getLowestAddress (void) const;
        uint32 getHighestAddress (void) const;


        friend void swap (NetworkAddressRange & lhs, NetworkAddressRange & rhs);


    protected:
        friend bool overlap (const NetworkAddressRange & lhs, const NetworkAddressRange & rhs);

        /* This method parses a range in the form of A-B, where A,B are
         * two byte octets which can assume any value between 0 and 255. */
        int parseOctet (const char *pszOctet, URange<uint8> & ur8Range);
        /* This method parses a descriptor in the form of A.B.C.D:E, where A.B.C.D are
         * the four octets of the IP address and E is the port.
         * Each octet of the IP address may be an individual number between 0 and 255,
         * a range such as 10-15, or '*' (which is equivalent to the range 0-255).
         * The port may also be an individual number between 0 and 65535, a range such as
         * 2000-3000, or '*' (which is equivalent to the range 0-65535).
         * The method returns 0 upon successful parsing of the descriptor,
         * or a negative value in case of error. */
        virtual int parse (char const * const pszDescriptor);

        virtual void reset (void);

        template <typename T> bool matches (T t1, T t2) const = delete;                     // To forbid implicit type conversions
        template <typename T1, typename T2> bool matches (T1 t1, T2 t2) const = delete;     // To forbid implicit type conversions
        template <typename T> bool matchesPort (T t) const = delete;                        // To forbid implicit type conversions


        IPv4AddressRange _ipAddr;
        PortNumberRange _portAddr;
    };


    class NetworkAddressRangeDescriptor : public NetworkAddressRange
    {
    public:
        NetworkAddressRangeDescriptor (void);
        NetworkAddressRangeDescriptor (const NetworkAddressRange & nar);
        NetworkAddressRangeDescriptor (const NetworkAddressRange && nar);
        NetworkAddressRangeDescriptor (const NetworkAddressRangeDescriptor & nard);
        NetworkAddressRangeDescriptor (const NetworkAddressRangeDescriptor && nard);

        NetworkAddressRangeDescriptor & operator= (const NetworkAddressRange rhs);
        NetworkAddressRangeDescriptor & operator= (const NetworkAddressRangeDescriptor rhs);

        const std::string & getLowestAddressAsString (void) const;
        const std::string & getHighestAddressAsString (void) const;
        const std::string & getAddressRangeStringDescription (void) const;


        friend void swap (NetworkAddressRangeDescriptor & lhs, NetworkAddressRangeDescriptor & rhs);

    private:
        virtual int parse (char const * const pszDescriptor);
        virtual void reset (void);

        void updateStringRepresentations (void);

        std::string _sLowestEndpointAddr;
        std::string _sHighestEndpointAddr;
        std::string _sNetworkAddressRangeRepresentation;
    };


    inline NetworkAddressRange::NetworkAddressRange (void) :
        _ipAddr{0}, _portAddr{0}
    { }

    // If the string descriptor does not specify the port range, '*' (range 0-65535) will be assumed
    inline NetworkAddressRange::NetworkAddressRange (char const * const pszDescriptor) :
        _ipAddr{0}, _portAddr{0}
    {
        if (parse (pszDescriptor) < 0) {
            reset();
        }
    }

    inline NetworkAddressRange::NetworkAddressRange (uint32 ui32IPv4Address, uint16 ui16Port) :
        _ipAddr{{ui32IPv4Address & 0x000000FFUL}, {(ui32IPv4Address >> 8) & 0x000000FFUL},
        {(ui32IPv4Address >> 16) & 0x000000FFUL}, {(ui32IPv4Address >> 24) & 0x000000FFUL}},
        _portAddr{ui16Port} { }

    inline NetworkAddressRange::NetworkAddressRange (uint32 ui32IPv4Address, PortNumberRange portRange) :
        _ipAddr{{ui32IPv4Address & 0x000000FFUL}, {(ui32IPv4Address >> 8) & 0x000000FFUL},
        {(ui32IPv4Address >> 16) & 0x000000FFUL}, {(ui32IPv4Address >> 24) & 0x000000FFUL}},
        _portAddr{portRange} { }

    inline Range * NetworkAddressRange::operator[] (unsigned int uiIndex)
    {
        // Make sure the index is not out of range
        uiIndex %= 5;

        if (uiIndex < 4) {
            return &(_ipAddr[uiIndex]);
        }
        return &_portAddr;
    }

    inline const Range * NetworkAddressRange::operator[] (unsigned int uiIndex) const
    {
        // Make sure the index is not out of range
        uiIndex %= 5;

        if (uiIndex < 4) {
            return &(_ipAddr[uiIndex]);
        }
        return &_portAddr;
    }

    inline bool NetworkAddressRange::isValid (void) const
    {
        return (((_ipAddr[0].getRangeWidth() > 1) || (_ipAddr[0].getLowestEnd() > 0)) ||
            ((_ipAddr[1].getRangeWidth() > 1) || (_ipAddr[0].getLowestEnd() > 0)) ||
            ((_ipAddr[2].getRangeWidth() > 1) || (_ipAddr[0].getLowestEnd() > 0)) ||
            ((_ipAddr[3].getRangeWidth() > 1) || (_ipAddr[0].getLowestEnd() > 0)) ||
            ((_portAddr.getRangeWidth() > 1) || (_portAddr.getLowestEnd() > 0)));
    }

    inline bool NetworkAddressRange::isAnIPRange (void) const
    {
        return ((_ipAddr[0].getRangeWidth() > 1) || (_ipAddr[1].getRangeWidth() > 1) ||
            (_ipAddr[2].getRangeWidth() > 1) || (_ipAddr[3].getRangeWidth() > 1));
    }

    inline bool NetworkAddressRange::isAPortRange (void) const
    {
        return _portAddr.getRangeWidth() > 1;
    }

    inline bool NetworkAddressRange::overlaps (const NetworkAddressRange & rhs) const
    {
        return overlap (*this, rhs);
    }

    // This method ignores the port number of rhs if it is set to 0
    inline bool NetworkAddressRange::contains (const NetworkAddressRange & rhs, unsigned int uiIndex) const
    {
        uiIndex %= 5;

        for (; uiIndex < 5; ++uiIndex) {
            if (!operator[] (uiIndex)->contains (rhs[uiIndex])) {
                if ((uiIndex < 4) || rhs.isAPortRange() || (rhs.operator[] (4)->getLowestEnd() > 0)) {
                    return false;
                }
            }
        }

        return true;
    }

    inline bool NetworkAddressRange::containsIgnorePort (const NetworkAddressRange & rhs, unsigned int uiIndex) const
    {
        uiIndex %= 5;

        for (; uiIndex < 4; ++uiIndex) {
            if (!operator[] (uiIndex)->contains (rhs[uiIndex])) {
                return false;
            }
        }

        return true;
    }

    // ui32IPv4Addr is in network byte order (big endian)
    inline bool NetworkAddressRange::matches (uint32 ui32IPv4Addr, uint16 ui16Port, unsigned int uIndex) const
    {
        return contains (NetworkAddressRange{ui32IPv4Addr, ui16Port}, uIndex);
    }

    inline bool NetworkAddressRange::matchesAddress (uint32 ui32IPv4Addr) const
    {
        return containsIgnorePort (NetworkAddressRange{ui32IPv4Addr, 0});
    }

    inline bool NetworkAddressRange::matchesAddressForAllPorts (uint32 ui32IPv4Addr) const
    {
        return contains (NetworkAddressRange{ui32IPv4Addr, PortNumberRange{std::numeric_limits<PortNumberRange::type>::min(),
                                                                           std::numeric_limits<PortNumberRange::type>::max()}});
    }

    inline bool NetworkAddressRange::matchesPort (uint16 ui16Port) const
    {
        URange<uint16> urPort{ui16Port};
        return (ui16Port == 0) || (_portAddr.contains (&urPort));
    }

    inline uint64 NetworkAddressRange::getNumberOfAddressesInRange (void) const
    {
        return _ipAddr[0].getRangeWidth() * _ipAddr[1].getRangeWidth() * _ipAddr[2].getRangeWidth() *
            _ipAddr[3].getRangeWidth() * _portAddr.getRangeWidth();
    }

    inline uint32 NetworkAddressRange::getLowestAddress (void) const
    {
        return (static_cast<uint32> (_ipAddr[0].getLowestEnd()) << 24) |
            (static_cast<uint32> (_ipAddr[1].getLowestEnd()) << 16) |
            (static_cast<uint32> (_ipAddr[2].getLowestEnd()) << 8) |
            (static_cast<uint32> (_ipAddr[3].getLowestEnd()));
    }

    inline uint32 NetworkAddressRange::getHighestAddress (void) const
    {
        return (static_cast<uint32> (_ipAddr[0].getHighestEnd()) << 24) |
            (static_cast<uint32> (_ipAddr[1].getHighestEnd()) << 16) |
            (static_cast<uint32> (_ipAddr[2].getHighestEnd()) << 8) |
            (static_cast<uint32> (_ipAddr[3].getHighestEnd()));
    }

    // Two address ranges overlap if each pair of octet ranges overlap
    inline bool overlap (const NetworkAddressRange & lhs, const NetworkAddressRange & rhs)
    {
        return lhs.operator[] (0)->overlaps (rhs.operator[] (0)) && lhs.operator[] (1)->overlaps (rhs.operator[] (1)) &&
            lhs.operator[] (2)->overlaps (rhs.operator[] (2)) && lhs.operator[] (3)->overlaps (rhs.operator[] (3)) &&
            (lhs.operator[] (4)->overlaps (rhs.operator[] (4)) || (!rhs.isAPortRange() && (rhs.operator[] (4)->getLowestEnd() == 0)));
    }

    inline bool isMulticastAddressRange (const NetworkAddressRange & ard)
    {
        return isMulticastIPv4Address (NOMADSUtil::EndianHelper::htonl (ard.getLowestAddress())) &&
            isMulticastIPv4Address (NOMADSUtil::EndianHelper::htonl (ard.getHighestAddress()));
    }

    inline void NetworkAddressRange::reset (void)
    {
        for (int i = 0; i < 4; ++i) {
            _ipAddr[i] = {0};
        }
        _portAddr = {0};
    }

    inline NetworkAddressRangeDescriptor::NetworkAddressRangeDescriptor (void) { }

    inline NetworkAddressRangeDescriptor::NetworkAddressRangeDescriptor (const NetworkAddressRange & nar) :
        NetworkAddressRange{nar}
    {
        updateStringRepresentations();
    }

    inline NetworkAddressRangeDescriptor::NetworkAddressRangeDescriptor (const NetworkAddressRange && nar) :
        NetworkAddressRange{std::move (nar)}
    {
        updateStringRepresentations();
    }

    inline NetworkAddressRangeDescriptor::NetworkAddressRangeDescriptor (const NetworkAddressRangeDescriptor & nard) :
        NetworkAddressRange{nard}, _sLowestEndpointAddr{nard._sLowestEndpointAddr}, _sHighestEndpointAddr{nard._sHighestEndpointAddr},
        _sNetworkAddressRangeRepresentation{nard._sNetworkAddressRangeRepresentation} { }

    inline NetworkAddressRangeDescriptor::NetworkAddressRangeDescriptor (const NetworkAddressRangeDescriptor && nard) :
        NetworkAddressRange{std::move (nard)}, _sLowestEndpointAddr{std::move (nard._sLowestEndpointAddr)},
        _sHighestEndpointAddr{std::move (nard._sHighestEndpointAddr)},
        _sNetworkAddressRangeRepresentation{std::move (nard._sNetworkAddressRangeRepresentation)} { }

    inline NetworkAddressRangeDescriptor & NetworkAddressRangeDescriptor::operator= (NetworkAddressRange rhs)
    {
        swap (static_cast<NetworkAddressRange &> (*this), rhs);
        updateStringRepresentations();

        return *this;
    }

    inline NetworkAddressRangeDescriptor & NetworkAddressRangeDescriptor::operator= (NetworkAddressRangeDescriptor rhs)
    {
        swap (*this, rhs);

        return *this;
    }

    inline const std::string & NetworkAddressRangeDescriptor::getLowestAddressAsString (void) const
    {
        return _sLowestEndpointAddr;
    }

    inline const std::string & NetworkAddressRangeDescriptor::getHighestAddressAsString (void) const
    {
        return _sHighestEndpointAddr;
    }

    inline const std::string & NetworkAddressRangeDescriptor::getAddressRangeStringDescription (void) const
    {
        return _sNetworkAddressRangeRepresentation;
    }

    inline void NetworkAddressRangeDescriptor::reset (void)
    {
        NetworkAddressRange::reset();

        _sLowestEndpointAddr.clear();
        _sHighestEndpointAddr.clear();
        _sNetworkAddressRangeRepresentation.clear();
    }

    inline void swap (NetworkAddressRange & lhs, NetworkAddressRange & rhs)
    {
        using std::swap;

        swap (lhs._ipAddr, rhs._ipAddr);
        swap (lhs._portAddr, rhs._portAddr);
    }

    inline void swap (NetworkAddressRangeDescriptor & lhs, NetworkAddressRangeDescriptor & rhs)
    {
        using std::swap;

        swap (static_cast<NetworkAddressRange &> (lhs), static_cast<NetworkAddressRange &> (rhs));
        swap (lhs._sLowestEndpointAddr, rhs._sLowestEndpointAddr);
        swap (lhs._sHighestEndpointAddr, rhs._sHighestEndpointAddr);
        swap (lhs._sNetworkAddressRangeRepresentation, rhs._sNetworkAddressRangeRepresentation);
    }
}

#endif  // INCL_ADDRESS_RANGE_DESCRIPTOR_H
