/*
 * NetworkHeaders.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2017 IHMC.
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

#include "NetworkHeaders.h"

#if defined (UNIX)
    #include <arpa/inet.h>
#elif defined (WIN32)
    #include <winsock2.h>
#endif

#include <stdio.h>

using namespace NOMADSUtil;

void EtherMACAddr::ntoh (void)
{
    ui16Word1 = ntohs (ui16Word1);
    ui16Word2 = ntohs (ui16Word2);
    ui16Word3 = ntohs (ui16Word3);
}

void EtherMACAddr::hton (void)
{
    ui16Word1 = htons (ui16Word1);
    ui16Word2 = htons (ui16Word2);
    ui16Word3 = htons (ui16Word3);
}

void EtherFrameHeader::ntoh (void)
{
    dest.ntoh();
    src.ntoh();
    ui16EtherType = ntohs (ui16EtherType);
}

void EtherFrameHeader::hton (void)
{
    dest.hton();
    src.hton();
    ui16EtherType = htons (ui16EtherType);
}

void EtherFrameHeader802_1Q::ntoh (void)
{
    dest.ntoh();
    src.ntoh();
    ui16_802_1Q_TPI = ntohs (ui16_802_1Q_TPI);
    ui16_802_1Q_TCI = ntohs (ui16_802_1Q_TCI);
    ui16EtherType = ntohs (ui16EtherType);
}

void EtherFrameHeader802_1Q::hton (void)
{
    dest.hton();
    src.hton();
    ui16_802_1Q_TPI = htons (ui16_802_1Q_TPI);
    ui16_802_1Q_TCI = htons (ui16_802_1Q_TCI);
    ui16EtherType = htons (ui16EtherType);
}

void EtherFrameHeader802_1AD::ntoh (void)
{
    dest.ntoh();
    src.ntoh();
    ui16_802_1AD_TPI = ntohs (ui16_802_1AD_TPI);
    ui16_802_1AD_TCI = ntohs (ui16_802_1AD_TCI);
    ui16_802_1Q_TPI = ntohs (ui16_802_1Q_TPI);
    ui16_802_1Q_TCI = ntohs (ui16_802_1Q_TCI);
    ui16EtherType = ntohs (ui16EtherType);
}

void EtherFrameHeader802_1AD::hton (void)
{
    dest.hton();
    src.hton();
    ui16_802_1AD_TPI = htons (ui16_802_1AD_TPI);
    ui16_802_1AD_TCI = htons (ui16_802_1AD_TCI);
    ui16_802_1Q_TPI = htons (ui16_802_1Q_TPI);
    ui16_802_1Q_TCI = htons (ui16_802_1Q_TCI);
    ui16EtherType = htons (ui16EtherType);
}

void IPv4Addr::ntoh (void)
{
    ui32Addr = ntohl (ui32Addr);
}

void IPv4Addr::hton (void)
{
    ui32Addr = htonl (ui32Addr);
}

void ARPPacket::ntoh (void)
{
    ui16HType = ntohs (ui16HType);
    ui16PType = ntohs (ui16PType);
    ui16Oper = ntohs (ui16Oper);
    sha.ntoh();
    spa.ntoh();
    tha.ntoh();
    tpa.ntoh();
}

void ARPPacket::hton (void)
{
    ui16HType = htons (ui16HType);
    ui16PType = htons (ui16PType);
    ui16Oper = htons (ui16Oper);
    sha.hton();
    spa.hton();
    tha.hton();
    tpa.hton();
}

uint16 IPHeader::computeChecksum (void *pBuf, uint16 ui16BufLen)
{
    // The following code is adapted from the "C" implementation example in RFC 1071 - Computing the Internet Checksum
    uint32 ui32Sum = 0;
    uint8 *pui8ChecksumData = (uint8*) pBuf;
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
    return (uint16) (~ui32Sum);
}

void IPHeader::computeChecksum (IPHeader *pIPHeader)
{
    uint16 ui16Len = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
    pIPHeader->ui16CRC = 0;
    pIPHeader->hton();
    pIPHeader->ui16CRC = computeChecksum (pIPHeader, ui16Len);
    pIPHeader->ntoh();
}

void IPHeader::ntoh (void)
{
    ui16TLen = ntohs (ui16TLen);
    ui16Ident = ntohs (ui16Ident);
    ui16FlagsAndFragOff = ntohs (ui16FlagsAndFragOff);
    ui16CRC = ntohs (ui16CRC);
    srcAddr.ntoh();
    destAddr.ntoh();
}

void IPHeader::hton (void)
{
    ui16TLen = htons (ui16TLen);
    ui16Ident = htons (ui16Ident);
    ui16FlagsAndFragOff = htons (ui16FlagsAndFragOff);
    ui16CRC = htons (ui16CRC);
    srcAddr.hton();
    destAddr.hton();
}











void ICMPHeader::computeChecksum (uint16 ui16Len)
{
    this->ui16Checksum = 0;
    this->hton();
    this->ui16Checksum = IPHeader::computeChecksum ((void*) this, ui16Len);
    this->ntoh();
}



void ICMPHeader::ntoh (void)
{
    ui16Checksum = ntohs (ui16Checksum);
    ui32RoH = ntohl (ui32RoH);
}

void ICMPHeader::hton (void)
{
    ui16Checksum = htons (ui16Checksum);
    ui32RoH = htonl (ui32RoH);
}

void TCPHeader::computeChecksum (uint8 *pIPPacket)
{
    // The following code is adapted from the "C" implementation example in RFC 1071 - Computing the Internet Checksum
    IPHeader *pIPHeader = (IPHeader*) pIPPacket;
    uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
    TCPHeader *pTCPHeader = (TCPHeader*) (pIPPacket + ui16IPHeaderLen);
    uint16 ui16TCPLen = pIPHeader->ui16TLen - ui16IPHeaderLen;
    pTCPHeader->ui16CRC = 0;
    uint32 ui32Sum = 0;
    ui32Sum += (pIPHeader->srcAddr.ui8Byte2 << 8) + (pIPHeader->srcAddr.ui8Byte1 << 0);
    ui32Sum += (pIPHeader->srcAddr.ui8Byte4 << 8) + (pIPHeader->srcAddr.ui8Byte3 << 0);
    ui32Sum += (pIPHeader->destAddr.ui8Byte2 << 8) + (pIPHeader->destAddr.ui8Byte1 << 0);
    ui32Sum += (pIPHeader->destAddr.ui8Byte4 << 8) + (pIPHeader->destAddr.ui8Byte3 << 0);
    ui32Sum += (pIPHeader->ui8Proto << 8);
    ui32Sum += htons (ui16TCPLen);
    pTCPHeader->hton();
    uint8 *pui8ChecksumData = (uint8*) pTCPHeader;
    while (ui16TCPLen > 1) {
        ui32Sum += *((uint16*)pui8ChecksumData);
        pui8ChecksumData += 2;
        ui16TCPLen -= 2;
    }
    if (ui16TCPLen > 0) {
        ui32Sum += *pui8ChecksumData;
    }
    while (ui32Sum >> 16) {
        ui32Sum = (ui32Sum & 0xFFFF) + (ui32Sum >> 16);
    }
    pTCPHeader->ui16CRC = (uint16) (~ui32Sum);
    pTCPHeader->ntoh();
}

void TCPHeader::ntoh (void)
{
    ui16SPort = ntohs (ui16SPort);
    ui16DPort = ntohs (ui16DPort);
    ui32SeqNum = ntohl (ui32SeqNum);
    ui32AckNum = ntohl (ui32AckNum);
    ui16Window = ntohs (ui16Window);
    ui16CRC = ntohs (ui16CRC);
    ui16Urgent = ntohs (ui16Urgent);
}

void TCPHeader::hton (void)
{
    ui16SPort = htons (ui16SPort);
    ui16DPort = htons (ui16DPort);
    ui32SeqNum = htonl (ui32SeqNum);
    ui32AckNum = htonl (ui32AckNum);
    ui16Window = htons (ui16Window);
    ui16CRC = htons (ui16CRC);
    ui16Urgent = htons (ui16Urgent);
}

void UDPHeader::ntoh (void)
{
    ui16SPort = ntohs (ui16SPort);
    ui16DPort = ntohs (ui16DPort);
    ui16Len = ntohs (ui16Len);
    ui16CRC = ntohs (ui16CRC);
}

void UDPHeader::hton (void)
{
    ui16SPort = htons (ui16SPort);
    ui16DPort = htons (ui16DPort);
    ui16Len = htons (ui16Len);
    ui16CRC = htons (ui16CRC);
}
