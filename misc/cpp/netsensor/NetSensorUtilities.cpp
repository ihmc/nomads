/*
* NetSensorUtilities.cpp
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
*/

#include "NetSensorUtilities.h"
#ifdef LINUX
    #include <sys/time.h>
#endif

using namespace NOMADSUtil;
#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{

void tokenizeStr (const char *str, const char separator, LList<String>* tokens)
{
    String tmp;
    tmp = str;

    int start = 0;
    int end = 0;

    do
    {
        while (*str != separator && *str)
        {
            str++;
            end++;
        }
        tokens->add(tmp.substring(start, end));
        end++;
        start = end;
    } while (0 != *str++);
}

void printStringList (LList<String> *list)
{
    list->resetGet();
    String supportString;
    while (list->getNext(supportString))
    {
        checkAndLogMsg ("printStringList()", Logger::L_Info, "%s\n", supportString.c_str());
    }
}

void buildEthernetMACAddressFromString(
	NOMADSUtil::EtherMACAddr &eMACAddrToUpdate, const uint8 * pszMACAddr)
{
    eMACAddrToUpdate.ui8Byte1 = pszMACAddr[0];
    eMACAddrToUpdate.ui8Byte2 = pszMACAddr[1];
    eMACAddrToUpdate.ui8Byte3 = pszMACAddr[2];
    eMACAddrToUpdate.ui8Byte4 = pszMACAddr[3];
    eMACAddrToUpdate.ui8Byte5 = pszMACAddr[4];
    eMACAddrToUpdate.ui8Byte6 = pszMACAddr[5];

    checkAndLogMsg("buildEthernetMACAddressFromString",
		Logger::L_Info, "Device mac is is %02X:%02X:%02X:%02X:%02X:%02X\n",
        eMACAddrToUpdate.ui8Byte1,
        eMACAddrToUpdate.ui8Byte2,
        eMACAddrToUpdate.ui8Byte3,
        eMACAddrToUpdate.ui8Byte4,
        eMACAddrToUpdate.ui8Byte5,
        eMACAddrToUpdate.ui8Byte6);
}

bool isInMulticastRange(const uint32 ui32Addr)
{
    IPv4Addr timpIP;
    timpIP.ui32Addr = ui32Addr;
    timpIP.ntoh();
    if ((C_MULTICAST_MIN_ADDRESS <= timpIP.ui32Addr) &&
		(timpIP.ui32Addr <= C_MULTICAST_MAX_ADDRESS)) {
		return true;
	}
    else {
		return false;
	}
}

NOMADSUtil::String buildStringFromEthernetMACAddress(
	NOMADSUtil::EtherMACAddr eMACAddrToUpdate)
{
    eMACAddrToUpdate.ntoh();
    char tmpS[30];
    sprintf(tmpS, "%02x:%02x:%02x:%02x:%02x:%02x",
		eMACAddrToUpdate.ui8Byte1,
        eMACAddrToUpdate.ui8Byte2,
        eMACAddrToUpdate.ui8Byte3,
		eMACAddrToUpdate.ui8Byte4,
        eMACAddrToUpdate.ui8Byte5,
        eMACAddrToUpdate.ui8Byte6);

    String sSupportMAC;
    sSupportMAC = tmpS;
    return sSupportMAC;
}

//Added by Blake Ordway on 5/16
// Now works with NIHMC_NETSENSOR_NET_UTILS::NS_EtherMACAddr
NOMADSUtil::String buildStringFromEthernetMACAddress(
    IHMC_NETSENSOR_NET_UTILS::NS_EtherMACAddr eMACAddrToUpdate)
{
    eMACAddrToUpdate.ntoh();
    char tmpS[30];
    sprintf(tmpS, "%02x:%02x:%02x:%02x:%02x:%02x",
        eMACAddrToUpdate.ui8Byte1,
        eMACAddrToUpdate.ui8Byte2,
        eMACAddrToUpdate.ui8Byte3,
        eMACAddrToUpdate.ui8Byte4,
        eMACAddrToUpdate.ui8Byte5,
        eMACAddrToUpdate.ui8Byte6);

    String sSupportMAC;
    sSupportMAC = tmpS;
    return sSupportMAC;
}

void setProtobufTimestamp(google::protobuf::Timestamp & ts)
{
#if defined (WIN32)
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    UINT64 ticks = (((UINT64)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;

    ts.set_seconds((INT64)((ticks / 10000000) - 11644473600LL));
    ts.set_nanos((INT32)((ticks % 10000000) * 100));
#endif

#if defined (LINUX)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts.set_seconds(tv.tv_sec);
    ts.set_nanos(tv.tv_usec * 1000);
#endif
}
}
