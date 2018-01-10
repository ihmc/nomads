#ifndef NETSENSOR_PacketStructures__INCLUDED
#define NETSENSOR_PacketStructures__INCLUDED
/*
* PacketStructures.h
* Author: bordway@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
*/
#include "FTypes.h"
#include<cstddef>
namespace IHMC_NETSENSOR_NET_UTILS
{
#pragma pack (1)
    struct NS_EtherMACAddr
    {
        union {
            uint16 ui16Word1;
            struct {
                uint8 ui8Byte1;
                uint8 ui8Byte2;
            };
        };
        union {
            uint16 ui16Word2;
            struct {
                uint8 ui8Byte3;
                uint8 ui8Byte4;
            };
        };
        union {
            uint16 ui16Word3;
            struct {
                uint8 ui8Byte5;
                uint8 ui8Byte6;
            };
        };

        void ntoh(void);
        void hton(void);

        bool operator == (const NS_EtherMACAddr &rhs) const;
        bool operator != (const NS_EtherMACAddr &rhs) const;
    };

    struct NS_EtherFrameHeader
    {
        NS_EtherMACAddr dest;
        NS_EtherMACAddr src;
        uint16 ui16EtherType;

        void ntoh(void);
        void hton(void);
    };

    struct NS_IPv4Addr
    {
        union {
            uint32 ui32Addr;
            struct {
                uint8 ui8Byte4;
                uint8 ui8Byte3;
                uint8 ui8Byte2;
                uint8 ui8Byte1;
            };
        };
        void ntoh(void);
        void hton(void);

        bool operator == (const NS_IPv4Addr &rhs) const;
    };

    struct NS_IPHeader
    {
        uint8 ui8VerAndHdrLen;        // Version (4 bits) + Internet header length (4 bits)
        uint8 ui8TOS;                 // Type of service 
        uint16 ui16TLen;              // Total length 
        uint16 ui16Ident;             // Identification
        uint16 ui16FlagsAndFragOff;   // Flags (3 bits) + Fragment offset (13 bits)
        uint8 ui8TTL;                 // Time to live
        uint8 ui8Proto;               // Protocol
        uint16 ui16CRC;               // Header checksum
        NS_IPv4Addr srcAddr;             // Source address
        NS_IPv4Addr destAddr;            // Destination address

        // Computes and returns the checksum for the specified data
        // NOTE: The assumption is that if the data has any headers (e.g., IP Header), it is in Network-byte order.
        //       The checksum value returned is also in network byte order
        static uint16 computeChecksum(void *pBuf, uint16 ui16BufSize);

        // Computes the checksum for the specified IP header and stores it in the ui16CRC field
        // of the specified IP Header
        // Member function - modifies the instance
        void computeChecksum(void);

        // Static function - does not modify an object instance
        static void computeChecksum(NS_IPHeader *pIPHeader);


        void ntoh(void);
        void hton(void);
    };

    struct NS_TCPHeader8Byte
    {
        uint16 ui16SPort;   // Source port
        uint16 ui16DPort;   // Destination port
        uint32 ui32SeqNum;  // Sequence Number

        void ntoh(void);
        void hton(void);
    };

    struct NS_UDPHeader8Byte
    {
        uint16 ui16SPort;   // Source port
        uint16 ui16DPort;   // Destination port
        uint16 ui16Len;     // Datagram length
        uint16 ui16CRC;     // Checksum

        void ntoh(void);
        void hton(void);
    };

    struct NS_ICMP
    {
        enum Type
        {
            T_Echo_Reply = 0,
            T_Destination_Unreachable = 3,
            T_Source_Quench = 4,
            T_Redirect_Message = 5,
            T_Echo_Request = 8,
            T_Time_Exceeded = 11,
            T_Parameter_Problem = 12,
            T_Timestamp_Request = 13,
            T_Timestamp_Reply = 14,
            T_Information_Request = 15,
            T_Information_Reply = 16,
            T_Address_Mask_Request = 17,
            T_Address_Mask_Reply = 18
        };

        uint8 ui8Type;
        uint8 ui8Code;
        uint16 ui16Checksum;
        union {
            uint32 ui32RoH; // Rest of Header
            union
            {
                uint16 ui16RoHWord1;
                struct {
                    uint8 ui8RoHByte2;
                    uint8 ui8RoHByte1;
                };
            };
            union
            {
                uint16 ui16RoHWord2;
                struct {
                    uint8 ui8RoHByte4;
                    uint8 ui8RoHByte3;
                };
            };
        };
        union
        {
            NS_IPHeader origIpv4Header; // IPv4Header, 20 bytes
            struct // Timestamp/addr mask
            {
                // 12 bytes
                union
                {
                    struct
                    {
                        union {
                            uint32 addrMask;
                            struct {
                                uint8 ui8AddrMaskByte4;
                                uint8 ui8AddrMaskByte3;
                                uint8 ui8AddrMaskByte2;
                                uint8 ui8AddrMaskByte1;
                            };
                        };
                        uint64 unusedSpace1;
                    };
                    struct
                    {
                        uint32 originateTimestamp;
                        uint32 receiveTimestamp;
                        uint32 transmitTimestamp;
                    };
                };
            };
        };
        union // 8 bytes of original datagram
        {
            NS_TCPHeader8Byte tcpHeader;
            NS_UDPHeader8Byte udpHeader;
        };
    };

    struct NS_ICMPPacket
    {
        NS_EtherFrameHeader *pMacHeader;
        NS_IPHeader         *pIpHeader;
        NS_ICMP             *pIcmpData;
        uint64               i64RcvTimeStamp;
    };


#pragma pack()

    inline bool NS_EtherMACAddr::operator == (const NS_EtherMACAddr &rhs) const
    {
        return (ui16Word1 == rhs.ui16Word1) && (ui16Word2 == rhs.ui16Word2) && (ui16Word3 == rhs.ui16Word3);
    }

    inline bool NS_EtherMACAddr::operator != (const NS_EtherMACAddr &rhs) const
    {
        return (ui16Word1 != rhs.ui16Word1) || (ui16Word2 != rhs.ui16Word2) || (ui16Word3 != rhs.ui16Word3);
    }

    //inline void NS_IPHeader::computeChecksum(void)
    //{
    //    IPHeader::computeChecksum(this);
    //}

    inline bool NS_IPv4Addr::operator == (const NS_IPv4Addr &rhs) const
    {
        return ui32Addr == rhs.ui32Addr;
    }
}
#endif // NETSENSOR_PacketStructures__INCLUDED
