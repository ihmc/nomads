/*
 * NICInfo.h
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

#ifndef INCL_NIC_INFO_H
#define INCL_NIC_INFO_H

#include "InetAddr.h"
#include "StrClass.h"

#if defined (WIN32)
    #include <Winsock2.h>
#elif defined (UNIX)
    #include <arpa/inet.h>
#endif

namespace NOMADSUtil
{
    /**
     * Class describing a Network Interface Configuration
     *
     * Currently it contains the IP address, netmask and
     * the broadcast address of a network interface.
     */ 
    class NICInfo
    {
        public:
            NICInfo (void);
            NICInfo (const NICInfo &n);
            ~NICInfo (void);

            int operator == (const NICInfo &n);
            InetAddr getIPAddr (void);
            InetAddr getBroadcastAddr (void);
            InetAddr getNetmask (void);
            String getIPAddrAsString (void);
            String getBroadcastAddrAsString (void);
            String getNetmaskAsString (void);

            String toString (void);

        public:
            #ifdef UNIX
                unsigned int uiIndex;
            #endif

            struct in_addr ip;
            struct in_addr broadcast;
            struct in_addr netmask;
    };

    inline NICInfo::NICInfo (void)
    {
    }

    inline NICInfo::NICInfo (const NICInfo &n)
    {
        #ifdef UNIX
            uiIndex = n.uiIndex;
        #endif
        ip.s_addr = n.ip.s_addr;
        broadcast.s_addr = n.broadcast.s_addr;
        netmask.s_addr = n.netmask.s_addr;
    }

    inline NICInfo::~NICInfo()
    {
    }

    inline int NICInfo::operator == (const NICInfo &n)
    {
        #ifdef UNIX
            if (uiIndex != n.uiIndex) {
                return false;
            }
        #endif
        return (ip.s_addr == n.ip.s_addr && 
                broadcast.s_addr == n.broadcast.s_addr && 
                netmask.s_addr == n.netmask.s_addr);
    }

    inline InetAddr NICInfo::getIPAddr (void)
    {
        return InetAddr (inet_ntoa (ip));
    }

    inline InetAddr NICInfo::getBroadcastAddr (void)
    {
        return InetAddr (inet_ntoa (broadcast));
    }

    inline InetAddr NICInfo::getNetmask (void)
    {
        return InetAddr (inet_ntoa (netmask));
    }

    inline String NICInfo::getIPAddrAsString (void)
    {
        return String (inet_ntoa (ip));
    }

    inline String NICInfo::getBroadcastAddrAsString (void)
    {
        return String (inet_ntoa (broadcast));
    }

    inline String NICInfo::getNetmaskAsString (void)
    {
        return String (inet_ntoa (netmask));
    }

    inline String NICInfo::toString (void)
    {
        String ifinfo = inet_ntoa (ip);
        ifinfo += "/";
        ifinfo += inet_ntoa (netmask);
        ifinfo += "/";
        ifinfo += inet_ntoa (broadcast);
        return ifinfo;
    }
}


#endif   // #ifndef INCL_NIC_INFO_H
