#ifndef INCL_ADDRESS_RANGE_DESCRIPTOR_H
#define INCL_ADDRESS_RANGE_DESCRIPTOR_H

/*
 * AddressRangeDescriptor.h
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
 * 
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * The AddressRangeDescriptor class stores the two endpoints of a range
 * of IP:port pairs and it defines useful methods to check, for instance,
 * if a range is valid or if it contains a specific IP:port pair.
 * In addition, the parse() method can parse a line in the configuration
 * files to initialize the object attributes with the configured values.
 */

#include "FTypes.h"
#include "StrClass.h"


namespace ACMNetProxy
{
    class AddressRangeDescriptor
    {
    public:
        AddressRangeDescriptor (void);
        AddressRangeDescriptor (char const * const pszDescriptor);
        explicit AddressRangeDescriptor (uint32 ui32IPv4Address, uint16 ui16Port = 0);
        ~AddressRangeDescriptor (void);

        AddressRangeDescriptor & operator= (const AddressRangeDescriptor & rhs);

        bool operator== (const AddressRangeDescriptor & rhs) const;
        bool operator== (uint32 ui32IPv4Address) const;

        operator const char * const (void) const;

        bool isValid (void) const;
        bool isAnIPRange (void) const;
        bool isAPortRange (void) const;
        bool isSubsetOf (const AddressRangeDescriptor & rhs) const;
        bool overlaps (const AddressRangeDescriptor & rhs) const;
        bool matches (uint32 ui32IPAddr, uint16 ui16Port) const;
        bool matchesPort (uint16 ui16Port) const;

        uint32 getLowestAddress (void) const;
        uint32 getHighestAddress (void) const;
        const NOMADSUtil::String & getLowestAddressAsString (void) const;
        const NOMADSUtil::String & getHighestAddressAsString (void) const;
        const NOMADSUtil::String & getAddressRangeStringDescription (void) const;

    private:
        /* This method parses a range in the form of A-B, where A,B are
         * two byte octets which can assume any value between 0 and 255. */
        int parseOctet (const char *pszOctet, uint8 *pui8OctetLow, uint8 *pui8OctetHigh);
        /* This method parses a descriptor in the form of A.B.C.D:E, where A.B.C.D are
         * the four octets of the IP address and E is the port.
         * Each octet of the IP address may be an individual number between 0 and 255,
         * a range such as 10-15, or '*' (which is equivalent to the range 0-255).
         * The port may also be an individual number between 0 and 65535, a range such as
         * 2000-3000, or '*' (which is equivalent to the range 0-65535).
         * The method returns 0 upon successful parsing of the descriptor,
         * or a negative value in case of error. */
        int parse (char const * const pszDescriptor);

        void reset (void);
        
        template <typename T> bool matches (T t1, T t2) const;                      // To forbid implicit type conversions
        template <typename T1, typename T2> bool matches (T1 t1, T2 t2) const;      // To forbid implicit type conversions
        template <typename T> bool matchesPort (T t) const;                         // To forbid implicit type conversions

        uint8 ui8IPAddrOctet1Low;       // Octet 1 is X.-.-.-
        uint8 ui8IPAddrOctet1High;
        uint8 ui8IPAddrOctet2Low;       // Octet 2 is -.X.-.-
        uint8 ui8IPAddrOctet2High;
        uint8 ui8IPAddrOctet3Low;       // Octet 3 is -.-.X.-
        uint8 ui8IPAddrOctet3High;
        uint8 ui8IPAddrOctet4Low;       // Octet 4 is -.-.-.X
        uint8 ui8IPAddrOctet4High;
        uint16 ui16PortLow;
        uint16 ui16PortHigh;

        NOMADSUtil::String _sLowestEndpointAddr;
        NOMADSUtil::String _sHighestEndpointAddr;
        NOMADSUtil::String _sAddressRangeDescriptor;
    };


    inline AddressRangeDescriptor::AddressRangeDescriptor (void) :
        ui8IPAddrOctet1Low (0), ui8IPAddrOctet2Low (0), ui8IPAddrOctet3Low (0), ui8IPAddrOctet4Low (0), ui8IPAddrOctet1High (0),
        ui8IPAddrOctet2High (0), ui8IPAddrOctet3High (0), ui8IPAddrOctet4High (0), ui16PortLow (0), ui16PortHigh (0) { }

    inline AddressRangeDescriptor::AddressRangeDescriptor (char const * const pszDescriptor) :
        ui8IPAddrOctet1Low (0), ui8IPAddrOctet2Low (0), ui8IPAddrOctet3Low (0), ui8IPAddrOctet4Low (0), ui8IPAddrOctet1High (0),
        ui8IPAddrOctet2High (0), ui8IPAddrOctet3High (0), ui8IPAddrOctet4High (0), ui16PortLow (0), ui16PortHigh (0)
    {
        if (parse (pszDescriptor) < 0) {
            reset();
        }
    }
    
    inline AddressRangeDescriptor::~AddressRangeDescriptor (void) { }
    
    inline bool AddressRangeDescriptor::operator== (uint32 ui32IPv4Address) const
    {
        return (*this) == AddressRangeDescriptor (ui32IPv4Address);
    }
    
    inline AddressRangeDescriptor::operator const char * const (void) const
    {
        return _sAddressRangeDescriptor.c_str();
    }

    inline bool AddressRangeDescriptor::isValid (void) const
    {
        return (ui8IPAddrOctet1Low || ui8IPAddrOctet2Low || ui8IPAddrOctet3Low || ui8IPAddrOctet4Low ||
               ui8IPAddrOctet1High || ui8IPAddrOctet2High || ui8IPAddrOctet3High || ui8IPAddrOctet4High) &&
               (ui8IPAddrOctet1Low <= ui8IPAddrOctet1High) && (ui8IPAddrOctet2Low <= ui8IPAddrOctet2High) &&
               (ui8IPAddrOctet3Low <= ui8IPAddrOctet3High) && (ui8IPAddrOctet4Low <= ui8IPAddrOctet4High);
    }

    inline bool AddressRangeDescriptor::isAnIPRange (void) const
    {
        return (ui8IPAddrOctet1Low < ui8IPAddrOctet1High) || (ui8IPAddrOctet2Low < ui8IPAddrOctet2High) ||
               (ui8IPAddrOctet3Low < ui8IPAddrOctet3High) || (ui8IPAddrOctet4Low < ui8IPAddrOctet4High);
    }

    inline bool AddressRangeDescriptor::isAPortRange (void) const
    {
        return ui16PortLow != ui16PortHigh;
    }

    inline bool AddressRangeDescriptor::matchesPort (uint16 ui16Port) const
    {
        return (ui16Port == 0) || ((ui16PortLow <= ui16Port) && (ui16Port <= ui16PortHigh));
    }

    inline uint32 AddressRangeDescriptor::getLowestAddress (void) const
    {
        return (((uint32) ui8IPAddrOctet1Low) << 24) | (((uint32) ui8IPAddrOctet2Low) << 16) |
               (((uint32) ui8IPAddrOctet3Low) << 8) | ((uint32) ui8IPAddrOctet4Low);
    }

    inline uint32 AddressRangeDescriptor::getHighestAddress (void) const
    {
        return (((uint32) ui8IPAddrOctet1High) << 24) | (((uint32) ui8IPAddrOctet2High) << 16) |
               (((uint32) ui8IPAddrOctet3High) << 8) | ((uint32) ui8IPAddrOctet4High);
    }

    inline const NOMADSUtil::String & AddressRangeDescriptor::getLowestAddressAsString (void) const
    {
        return _sLowestEndpointAddr;
    }

    inline const NOMADSUtil::String & AddressRangeDescriptor::getHighestAddressAsString (void) const
    {
        return _sHighestEndpointAddr;
    }

    inline const NOMADSUtil::String & AddressRangeDescriptor::getAddressRangeStringDescription (void) const
    {
        return _sAddressRangeDescriptor;
    }
}

#endif  // INCL_ADDRESS_RANGE_DESCRIPTOR_H
