#ifndef NETSENSOR_NetSensorUtilities__INCLUDED
#define NETSENSOR_NetSensorUtilities__INCLUDED
/*
* NetSensorUtilities.h
* Author: rfronteddu@ihmc.us
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
* General utilities for used by NetSensor
*
*/
#include <cstdio>
#include "google/protobuf/timestamp.pb.h"

#include "FTypes.h"
#include "NLFLib.h"
#include "LList.h"
#include "Logger.h"
#include "NetworkHeaders.h"
#include "StrClass.h"
#include "PacketStructures.h"
#include "NetSensorConstants.h"

namespace IHMC_NETSENSOR
{
inline int64 getTimeInNanos(void)
{
    return NOMADSUtil::getTimeInMilliseconds() * 1000;
}

inline uint32 convertIpToHostOrder (uint32 ui32networkOrderIp)
{
    NOMADSUtil::IPv4Addr ip {ui32networkOrderIp};
    ip.hton();

    return ip.ui32Addr;
};

inline uint32 getIpInHostOrder (NOMADSUtil::String sAddr)
{
	uint32 ui32SrcAddr = NOMADSUtil::InetAddr(sAddr).getIPAddress();
	NOMADSUtil::IPv4Addr ip{ ui32SrcAddr };
	ip.hton();
	return ip.ui32Addr;
};

void tokenizeStr(const char *str, const char separator,
    NOMADSUtil::LList<NOMADSUtil::String>* tokens);

void printStringList(NOMADSUtil::LList<NOMADSUtil::String>* list);

void buildEthernetMACAddressFromString(NOMADSUtil::EtherMACAddr
    &eMACAddrToUpdate, const uint8 * pszMACAddr);

NOMADSUtil::String buildStringFromEthernetMACAddress(NOMADSUtil::EtherMACAddr
    eMACAddrToUpdate);

// Updated by Blake Ordway 5/16/17
// Now works with IHMC_NETSENSOR_NET_UTILS::NS_EtherMACAddr
NOMADSUtil::String buildStringFromEthernetMACAddress(
    IHMC_NETSENSOR_NET_UTILS::NS_EtherMACAddr eMACAddrToUpdate);

inline bool convertIntToString(uint16 i, char* s)
{
    sprintf(s, "%d", i);
    //itoa(i, s, 10);
    return true;
}

bool isInMulticastRange(const uint32 ui32Addr);

void setProtobufTimestamp(google::protobuf::Timestamp & ts);
}
#endif