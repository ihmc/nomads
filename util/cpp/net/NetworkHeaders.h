/*
 * NetworkHeaders.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#ifndef INCL_NETWORK_HEADERS_H
#define INCL_NETWORK_HEADERS_H

#include "FTypes.h"

#define IP_RESERVED_FLAG_FILTER 0x8000U
#define IP_DF_FLAG_FILTER 0x4000U
#define IP_MF_FLAG_FILTER 0x2000U
#define IP_OFFSET_FILTER 0x1FFFU
#define TCP_DATA_FLAG_FILTER 0x38

namespace NOMADSUtil
{
    #pragma pack (1)

    // Ethernet MAC Address
    struct EtherMACAddr
    {
        union {
            uint16 ui16Word1;
            struct {
                uint8 ui8Byte2;
                uint8 ui8Byte1;
            };
        };
        union {
            uint16 ui16Word2;
            struct {
                uint8 ui8Byte4;
                uint8 ui8Byte3;
            };
        };
        union {
            uint16 ui16Word3;
            struct {
                uint8 ui8Byte6;
                uint8 ui8Byte5;
            };
        };

        void ntoh (void);
        void hton (void);
        
        bool operator == (const EtherMACAddr &rhs) const;
        bool operator != (const EtherMACAddr &rhs) const;
    };

    // Ethernet Frame Header
    struct EtherFrameHeader
    {
        EtherMACAddr dest;
        EtherMACAddr src;
        uint16 ui16EtherType;

        void ntoh (void);
        void hton (void);
    };

    enum EtherType
    {
        ET_IP = 0x0800,
        ET_ARP = 0x0806,
        ET_IP_v6 = 0x86DD
    };

    struct IPv4Addr
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

        void ntoh (void);
        void hton (void);

        bool operator == (const IPv4Addr &rhs) const;
    };

    // ARP packet
    struct ARPPacket
    {
        uint16 ui16HType;   // Hardware Type
        uint16 ui16PType;   // Protocol Type
        uint8 ui8HLen;      // Hardware Length
        uint8 ui8PLen;      // Protocol Length
        uint16 ui16Oper;    // Operation
        EtherMACAddr sha;   // SHA - Sender Hardware Address
        IPv4Addr spa;       // SPA - Sender Protocol Address
        EtherMACAddr tha;   // THA - Target Hardware Address
        IPv4Addr tpa;       // TPA - Target Protocol Address

        void ntoh (void);
        void hton (void);
    };

    enum IP_PROTO_TYPE
    {
        IP_PROTO_ICMP = 0x01,
        IP_PROTO_IP_v4 = 0x04,          // For encapsulation purposes
        IP_PROTO_TCP = 0x06,
        IP_PROTO_UDP = 0x11
    };

    // IP header
    struct IPHeader
    {
        uint8 ui8VerAndHdrLen;        // Version (4 bits) + Internet header length (4 bits)
        uint8 ui8TOS;                 // Type of service 
        uint16 ui16TLen;              // Total length 
        uint16 ui16Ident;             // Identification
        uint16 ui16FlagsAndFragOff;   // Flags (3 bits) + Fragment offset (13 bits)
        uint8 ui8TTL;                 // Time to live
        uint8 ui8Proto;               // Protocol
        uint16 ui16CRC;               // Header checksum
        IPv4Addr srcAddr;             // Source address
        IPv4Addr destAddr;            // Destination address

        // Computes and returns the checksum for the specified data
        // NOTE: The assumption is that if the data has any headers (e.g., IP Header), it is in Network-byte order.
        //       The checksum value returned is also in network byte order
        static uint16 computeChecksum (void *pBuf, uint16 ui16BufSize);

        // Computes the checksum for the specified IP header and stores it in the ui16CRC field
        // of the specified IP Header
        // Member function - modifies the instance
        void computeChecksum (void);

        // Static function - does not modify an object instance
        static void computeChecksum (IPHeader *pIPHeader);

        void ntoh (void);
        void hton (void);
    };
    
    // ICMP header
    struct ICMPHeader
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

        enum Code_Destination_Unreachable
        {
            CDU_Network_Unreachable,
            CDU_Host_Unreachable,
            CDU_Protocol_Unreachable,
            CDU_Port_Unreachable,
            CDU_Fragmentation_needed_and_DF_set,
            CDU_Source_route_failed,
            CDU_Destination_Network_unknown,
            CDU_Destination_Host_unknown,
            CDU_Source_Host_isolated,
            CDU_Communication_with_Destination_Network_Administratively_Prohibited,
            CDU_Communication_with_Destination_Host_Administratively_Prohibited, 
            CDU_Network_Unreachable_for_Type_Of_Service,
            CDU_Host_Unreachable_for_Type_Of_Service,
            CDU_Communication_Administratively_Prohibited_by_Filtering,
            CDU_Host_Precedence_Violation,
            CDU_Precedence_Cutoff_in_Effect
        };


        uint8 ui8Type;                  // ICMP Message Type
        uint8 ui8Code;                  // ICMP Message code: meaning differs depending on Message Type 
        uint16 ui16Checksum;            // Checksum of the whole ICMP packet
        union {
            uint32 ui32RoH;             // Rest of Header
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
        
        
        // Computes the checksum for this instance of the ICMP header and stores it in the ui16Checksum field
        void computeChecksum (uint16 ui16Len);

        void ntoh (void);
        void hton (void);
    };

    // TCP Header
    struct TCPHeader
    {
        enum Flags {
            TCPF_FIN = 0x01,
            TCPF_SYN = 0x02,
            TCPF_RST = 0x04,
            TCPF_PSH = 0x08,
            TCPF_ACK = 0x10,
            TCPF_URG = 0x20,
            TCPF_ECE = 0x40,
            TCPF_CWR = 0x80
        };
        uint16 ui16SPort;   // Source port
        uint16 ui16DPort;   // Destination port
        uint32 ui32SeqNum;  // Sequence Number
        uint32 ui32AckNum;  // Acknowledgement Number
        uint8 ui8Offset;    // Data Offset (4 bits) + Reserved (4 bits)
        uint8 ui8Flags;     // Flags
        uint16 ui16Window;  // Window size
        uint16 ui16CRC;     // Checksum
        uint16 ui16Urgent;  // Urgent data pointer
        #pragma warning (disable:4200)
            uint8 ui8Options[]; // Options, if any, follow
        #pragma warning (default:4200)

        // Computes the TCP Checksum according to RFC 793 and fill in the ui16CRC field
        // This method must be passed a pointer to the complete IP packet, since the
        //     TCP checksum includes the source and destination IP addresses and the
        //     protocol field, which are all in the IP header
        // NOTE: This is a static function - does not modify an object instance
        static void computeChecksum (uint8 *pIPPacket);

        void ntoh (void);
        void hton (void);
    };

    // UDP header
    struct UDPHeader
    {
        uint16 ui16SPort;   // Source port
        uint16 ui16DPort;   // Destination port
        uint16 ui16Len;     // Datagram length
        uint16 ui16CRC;     // Checksum

        void ntoh (void);
        void hton (void);
    };

    #pragma pack ()

    inline bool EtherMACAddr::operator == (const EtherMACAddr &rhs) const
    {
        return (ui16Word1 == rhs.ui16Word1) && (ui16Word2 == rhs.ui16Word2) && (ui16Word3 == rhs.ui16Word3);
    }

    inline bool EtherMACAddr::operator != (const EtherMACAddr &rhs) const
    {
        return (ui16Word1 != rhs.ui16Word1) || (ui16Word2 != rhs.ui16Word2) || (ui16Word3 != rhs.ui16Word3);
    }

    inline void IPHeader::computeChecksum (void)
    {
        IPHeader::computeChecksum (this);
    }

    inline bool IPv4Addr::operator == (const IPv4Addr &rhs) const
    {
        return ui32Addr == rhs.ui32Addr;
    }
}

#endif   // #ifndef INCL_NETWORK_HEADERS_H
