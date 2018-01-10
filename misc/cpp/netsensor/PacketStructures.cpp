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
*/

#include"PacketStructures.h"
#if defined (UNIX)
    #include <arpa/inet.h>
#elif defined (WIN32)
    #include <winsock2.h>
#endif
namespace IHMC_NETSENSOR_NET_UTILS
{
    void NS_EtherMACAddr::ntoh(void)
    {
        ui16Word1 = ntohs(ui16Word1);
        ui16Word2 = ntohs(ui16Word2);
        ui16Word3 = ntohs(ui16Word3);
    }

    void NS_EtherMACAddr::hton(void)
    {
        ui16Word1 = htons(ui16Word1);
        ui16Word2 = htons(ui16Word2);
        ui16Word3 = htons(ui16Word3);
    }

    void NS_EtherFrameHeader::ntoh(void)
    {
        dest.ntoh();
        src.ntoh();
        ui16EtherType = ntohs(ui16EtherType);
    }

    void NS_EtherFrameHeader::hton(void)
    {
        dest.hton();
        src.hton();
        ui16EtherType = htons(ui16EtherType);
    }

    void NS_IPv4Addr::ntoh(void)
    {
        ui32Addr = ntohl(ui32Addr);
    }

    void NS_IPv4Addr::hton(void)
    {
        ui32Addr = htonl(ui32Addr);
    }

    void NS_IPHeader::computeChecksum(void)
    {
        NS_IPHeader::computeChecksum(this);
    }

    void NS_IPHeader::computeChecksum(NS_IPHeader *pIPHeader)
    {
        uint16 ui16Len = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        pIPHeader->ui16CRC = 0;
        pIPHeader->hton();
        pIPHeader->ui16CRC = computeChecksum(pIPHeader, ui16Len);
        pIPHeader->ntoh();
    }

    uint16 NS_IPHeader::computeChecksum(void *pBuf, uint16 ui16BufLen)
    {
        // The following code is adapted from the "C" implementation example in RFC 1071 - Computing the Internet Checksum
        uint32 ui32Sum = 0;
        uint8 *pui8ChecksumData = (uint8*)pBuf;
        while (ui16BufLen > 1) {
            ui32Sum += *((uint16*)pui8ChecksumData);
            pui8ChecksumData += 2;
            ui16BufLen -= 2;
        }
        if (ui16BufLen > 0) {
            ui32Sum += *pui8ChecksumData;
        }
        while (ui32Sum >> 16) {
            ui32Sum = (ui32Sum & 0xFFFF) + (ui32Sum >> 16);
        }
        return (uint16)(~ui32Sum);
    }

    void NS_IPHeader::ntoh()
    {
        ui16TLen = ntohs(ui16TLen);
        ui16Ident = ntohs(ui16Ident);
        ui16FlagsAndFragOff = ntohs(ui16FlagsAndFragOff);
        ui16CRC = ntohs(ui16CRC);
        srcAddr.ntoh();
        destAddr.ntoh();
    }

    void NS_IPHeader::hton()
    {
        ui16TLen = htons(ui16TLen);
        ui16Ident = htons(ui16Ident);
        ui16FlagsAndFragOff = htons(ui16FlagsAndFragOff);
        ui16CRC = htons(ui16CRC);
        srcAddr.hton();
        destAddr.hton();
    }


    void NS_TCPHeader8Byte::hton()
    {
        ui16SPort = ntohs(ui16SPort);
        ui16DPort = ntohs(ui16DPort);
        ui32SeqNum = ntohl(ui32SeqNum);
    }

    void NS_TCPHeader8Byte::ntoh(void)
    {
        ui16SPort = ntohs(ui16SPort);
        ui16DPort = ntohs(ui16DPort);
        ui32SeqNum = ntohl(ui32SeqNum);
    }

    void NS_UDPHeader8Byte::ntoh(void)
    {
        ui16SPort = ntohs(ui16SPort);
        ui16DPort = ntohs(ui16DPort);
        ui16Len = ntohs(ui16Len);
        ui16CRC = ntohs(ui16CRC);
    }

    void NS_UDPHeader8Byte::hton(void)
    {
        ui16SPort = htons(ui16SPort);
        ui16DPort = htons(ui16DPort);
        ui16Len = htons(ui16Len);
        ui16CRC = htons(ui16CRC);
    }
}
