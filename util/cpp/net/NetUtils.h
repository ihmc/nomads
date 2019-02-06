/*
 * NetUtils.h
 *
 *This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_NET_UTILS_H
#define INCL_NET_UTILS_H

#include "StrClass.h"
#include "InetAddr.h"
#include "FTypes.h"
#include "NICInfo.h"
#include "NetworkHeaders.h"

#if defined (WIN32)
    #define _WINSOCKAPI_ //prevent inclusion of winsock.h
    #include <winsock2.h>
#elif defined (UNIX)
    #include <arpa/inet.h>
    #include <stdlib.h>
#endif

namespace NOMADSUtil
{
    class NetUtils
    {
        public:
            // returns the number of available network interfaces (note this method is inefficient
            // because is internally calling getNICsInfo(). There is no other way of getting the
            // number of NICs.
            static uint16 getNICsNumber (bool bIncludeLoopback = false, bool bUniqueNetworks = false);

            // NOTE: Caller must free memory allocated by the getNICsInfo() calling freeNICsInfo()
            static NICInfo ** getNICsInfo (bool bIncludeLoopback = false, bool bUniqueNetworks = false);

            // returns an array (NULL terminated) of pointers to NICInfo objects containing the NICInfo of
            // network interfaces matching an ip address containing wildcards.
            // NOTE: Caller must free memory allocated by the getNICsInfo() calling freeNICsInfo()
            static NICInfo ** getNICsInfo (const char *pszIPAddrWild);

            // NOTE: caller must deallocate memory used by NICInfo* using the delete() operator.
            static NICInfo * getNICInfo (const char *pszIPAddr);
            static NICInfo * getNICInfo (struct in_addr ipAddr);

            static String getIncomingNIC (const char *pszBindingIPAddr, const char *pszBindingIPAddrNetmask,
                                          const char *pszPacketRemoteAddr, const char *pszPacketDstAddr);

            /* Returns an EtherMACAddr instance that corresponds to the value
             * represented in the string passed in as parameter.
             * NOTE: in case of error, the returned value will be all zeros.
             */
            static EtherMACAddr getEtherMACAddrFromString (const char * const pszEtherMACAddr);

            /* Returns true if the string passed in as parameter is a correct representation
             * of an Ethernet MAC address, expressed in the form AA:BB:CC:DD:EE:FF.
             */
            static bool checkEtherMACAddressFormat (const char * pszEtherMACAddr);

            static bool isLocalAddress (uint32 ulIPAddr, NICInfo **pMyNetIFs = NULL);

            static bool isLocalAddress (struct in_addr ipAddr);

            static bool isLocalAddress (const char *pszIPAddr);

            static String getLocalHostName (void);

            static struct in_addr getLocalIPAddress (void);

            // Lookup a host by name and return the IP address
            // Also handles strings of IP addresses (e.g. "p.q.r.s")
            // Returns 0 in case of error
            static uint32 getHostByName (const char *pszHostName);

            static bool areInSameNetwork (NICInfo *pNICInfoA, NICInfo *pNICInfoB);
            static bool areInSameNetwork (const char *pszAddrA, const char *pszNetmaskA, const char *pszAddrB, const char *pszNetmaskB);
            static bool areInSameNetwork (uint32 ui32AddrA, uint32 ui32NetmaskA, uint32 ui32AddrB, uint32 ui32NetmaskB);

            /*
             * Given the network interfaces of the local node and of a remote node, it determines which IP address of the
             * remote node is reachable from the local node. It returns the first IP address that is reachable.
             *
             * If the local network interfaces are not specified, they are determined automatically.
             */
            static InetAddr determineDestIPAddr (NICInfo **pRemoteNetIFs, NICInfo **pLocalNetIFs = NULL);

            /*
             * Check if an IP address belongs to the broadcast address class.
             */
            static bool isBroadcastAddress (const char *pszIPAddress, const char *pszNetmask);
            static bool isBroadcastAddress (InetAddr addr, InetAddr netmask);

            /*
             * Check if an IP address belongs to the class D, included in the range
             * between 224.0.0.0 and 239.255.255.255.
             */
            static bool isMulticastAddress (const char *pszIPAddr);
            static bool isMulticastAddress (InetAddr addr);
            static bool isMulticastAddress (unsigned long ulAddr);

            // frees the memory allocated by ppNICsInfo
            static void freeNICsInfo (NICInfo **ppNICsInfo);

            // this function wraps around inet_ntoa, therefore, as inet_ntoa, it
            // converts the Internet host address ui32Addr, given in network byte
            // order, to a string  in  IPv4  dotted-decimal  notation.
            // However, the function returns a copy of the statically allocated
            // string returned by inet_ntoa, therefore the caller must deallocate
            // it
            static char * ui32Inetoa (uint32 ui32Addr);
            static char * ui32Inetoa (in_addr& addr);
    };

    inline bool NetUtils::areInSameNetwork (NICInfo *pNICInfoA, NICInfo *pNICInfoB)
    {
        if ((pNICInfoA == NULL) || (pNICInfoB == NULL)) {
            return false;
        }
        return areInSameNetwork (pNICInfoA->ip.s_addr, pNICInfoA->netmask.s_addr, pNICInfoB->ip.s_addr, pNICInfoB->netmask.s_addr);
    }

    inline bool NetUtils::areInSameNetwork (const char *pszAddrA, const char *pszNetmaskA, const char *pszAddrB, const char *pszNetmaskB)
    {
        if ((pszAddrA == NULL) || (pszNetmaskA == NULL) || (pszAddrB == NULL) || (pszNetmaskB == NULL)) {
            return false;
        }
        return areInSameNetwork (inet_addr (pszAddrA), inet_addr (pszNetmaskA), inet_addr (pszAddrB), inet_addr (pszNetmaskB));
    }

    inline bool NetUtils::areInSameNetwork (uint32 ui32AddrA, uint32 ui32NetmaskA, uint32 ui32AddrB, uint32 ui32NetmaskB)
    {
        return ((ui32AddrA & ui32NetmaskA) == (ui32AddrB & ui32NetmaskB));
    }

    inline bool NetUtils::isBroadcastAddress (const char *pszIPAddress, const char *pszNetmask)
    {
        return isBroadcastAddress (InetAddr (pszIPAddress), InetAddr (pszNetmask));
    }

    inline bool NetUtils::isBroadcastAddress (InetAddr addr, InetAddr netmask)
    {
        return (~((uint32)addr.getIPAddress() | (uint32)netmask.getIPAddress()) == 0) && ~((uint32)netmask.getIPAddress());
    }

    inline bool NetUtils::isMulticastAddress (const char *pszIPAddr)
    {
        InetAddr addr (pszIPAddr);
        return isMulticastAddress (addr.getIPAddress());
    }

    inline bool NetUtils::isMulticastAddress (InetAddr addr)
    {
        return isMulticastAddress (addr.getIPAddress());
    }

    inline bool NetUtils::isMulticastAddress (unsigned long ulAddr)
    {
        return (((ulAddr & 255) & 0xF0) == 0xE0);
    }
}

#endif   // #ifdef INCL_NETUTILS_H
