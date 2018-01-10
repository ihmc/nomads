/*
 * NetUtils.cpp
 *
 * This file is part of the IHMC Util Library
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

#include "NetUtils.h"

#include <cstdlib>
#include "Logger.h"
#include "NLFLib.h"

#if defined (WIN32)
    #define PATH_MAX _MAX_PATH
    #include <ws2tcpip.h>
#elif defined (UNIX)
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <net/if.h>
    #include <string.h>
    #include <netdb.h>
    #include <unistd.h>
    #define inaddrr(x) (*(struct in_addr*) &ifr.x[sizeof sa.sin_port])
    #if defined (OSX)
        #include <ifaddrs.h>
        #ifndef PATH_MAX
            #define PATH_MAX 1024
        #endif
    #endif
#endif

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

struct in_addr NetUtils::getLocalIPAddress (void)
{
    NICInfo **ppNICsInfo = getNICsInfo (false, true);
    in_addr ipAddr;

    if (ppNICsInfo != NULL && ppNICsInfo[0] != NULL) {
        ipAddr = ppNICsInfo[0]->ip;
    }
    else {
       ipAddr.s_addr = 0;
    }

    freeNICsInfo (ppNICsInfo);

    return ipAddr;
}

uint16 NetUtils::getNICsNumber (bool bIncludeLoopback, bool bUniqueNetworks)
{
    NICInfo **ppNICsInfo = getNICsInfo (bIncludeLoopback, bUniqueNetworks);
    if (ppNICsInfo) {
        uint16 ui16NICsNumber;
        for (ui16NICsNumber = 0; ppNICsInfo[ui16NICsNumber]; ui16NICsNumber++) {
		}
		freeNICsInfo (ppNICsInfo);
        return ui16NICsNumber;
    }
    return 0;
}

NICInfo ** NetUtils::getNICsInfo (bool bIncludeLoopback, bool bUniqueNetworks)
{
    const char * const pszMethodName = "NetUtils::getNICInfos";

    #if defined (WIN32)

        WSADATA WinsockData;
        if (WSAStartup (MAKEWORD(2, 2), &WinsockData) != 0) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Failed to initialize Winsock!\n");
            return NULL;
        }

        SOCKET sd = WSASocket (AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
        if (sd == SOCKET_ERROR) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Failed to create a socket.\n");
            return NULL;
        }

        INTERFACE_INFO InterfaceList[256];
        u_long ulSize = 0;
        if (WSAIoctl (sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
			          sizeof (InterfaceList), &ulSize, 0, 0) == SOCKET_ERROR) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Failed calling WSAIoctl\n");
		    closesocket (sd);
            return NULL;
        }
        uint16 ui16IFNum = (uint16) ulSize / sizeof (INTERFACE_INFO);
        closesocket (sd);

        NICInfo **ppNICsInfo = new NICInfo*[ui16IFNum+1];
        int j = 0;
        bool bAddIF = true;
        for (int i = 0; i < ui16IFNum; i++) {
            u_long nFlags = InterfaceList[i].iiFlags;
            bool bLoopback = ((nFlags & IFF_LOOPBACK) != 0);
            if ((nFlags & IFF_UP) && (!bLoopback || (bLoopback && bIncludeLoopback))) {
                sockaddr_in *pAddress;
                NICInfo *pNICInfo = new NICInfo();
                pAddress = (sockaddr_in *) & (InterfaceList[i].iiAddress);
                pNICInfo->ip = pAddress->sin_addr;
                pAddress = (sockaddr_in *) & (InterfaceList[i].iiNetmask);
                pNICInfo->netmask = pAddress->sin_addr;
                pNICInfo->broadcast.S_un.S_addr = pNICInfo->ip.S_un.S_addr | ~pNICInfo->netmask.S_un.S_addr;

                if (bUniqueNetworks) {
                    for (int z=0; z<j; z++) {
                        if (areInSameNetwork (pNICInfo, ppNICsInfo[z])) {
                            bAddIF = false;
                            break;
                        }
                    }
                }
                if (bAddIF) {
                    ppNICsInfo[j++] = pNICInfo;
                }
                else {
                    delete pNICInfo;
                }
            }
        }

        // Terminator
        ppNICsInfo[j] = NULL;

        WSACleanup();
        return ppNICsInfo;

    #elif defined (LINUX)

        struct sockaddr_in sa;
        struct ifreq ifr;
        struct ifreq *pifr;
        struct ifconf ifc;
        char buf[1024];
        int sockfd;

        if ((sockfd = socket (AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Failed to create a socket\n");
            return NULL;
        }

        ifc.ifc_len = sizeof (buf);
        ifc.ifc_buf = buf;
        ioctl (sockfd, SIOCGIFCONF, &ifc);
        pifr = ifc.ifc_req;

        uint16 ui16IFNum = ifc.ifc_len / sizeof (struct ifreq);

        NICInfo **ppNICsInfo = new NICInfo*[ui16IFNum+1];
        int j = 0;
        bool bAddIF = true;
        for (int i = ui16IFNum; --i >= 0; pifr++) {
            strcpy (ifr.ifr_name, pifr->ifr_name);
            if (ioctl (sockfd, SIOCGIFFLAGS, &ifr) == 0) {
                bool bLoopback = (ifr.ifr_flags & IFF_LOOPBACK);
                bool bUp = (ifr.ifr_flags & IFF_UP);
                if (bUp && (!bLoopback || (bLoopback && bIncludeLoopback))) {
                    NICInfo *pNICInfo = new NICInfo();

                    // Index
                    pNICInfo->uiIndex = if_nametoindex (ifr.ifr_name);

                    // IP Address
                    if (ioctl (sockfd, SIOCGIFADDR, &ifr) == 0) {
                        pNICInfo->ip = inaddrr (ifr_addr.sa_data);
                    }
                    // netmask
                    if (ioctl (sockfd, SIOCGIFNETMASK, &ifr) == 0) {
                        pNICInfo->netmask = inaddrr (ifr_addr.sa_data);
                    }
                    // broadcast
                    if (ioctl (sockfd, SIOCGIFBRDADDR, &ifr) == 0) {
                        pNICInfo->broadcast = inaddrr (ifr_addr.sa_data);
                    }

                    if (bUniqueNetworks) {
                        for (int z=0; z<j; z++) {
                            if (areInSameNetwork (pNICInfo, ppNICsInfo[z])) {
                                bAddIF = false;
                                break;
                            }
                        }
                    }

                    if (bAddIF) {
                        ppNICsInfo[j++] = pNICInfo;
                    }
                    else {
                        delete pNICInfo;
                    }
                }
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_MildError, "Error calling ioctl\n");
            }
        }
        close (sockfd);

        // Terminator
        ppNICsInfo[j] = NULL;

        return ppNICsInfo;

    #elif defined (OSX)

        struct ifaddrs *myaddrs, *ifa;

        if (getifaddrs (&myaddrs) != 0) {
            freeifaddrs (myaddrs);
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Failed to get information about NICs\n");
            return NULL;
        }

        uint16 ui16IFNum = 0;
        for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next, ui16IFNum++);

        NICInfo **ppNICsInfo = new NICInfo*[ui16IFNum+1];
        int j = 0;
        for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
        {
            bool bAddIF = true;
            if (ifa->ifa_addr == NULL) {
                continue;
            }

            bool bUp = ifa->ifa_flags & IFF_UP;
            bool bLoopback = ifa->ifa_flags & IFF_LOOPBACK;
            bool bIPv4 = (ifa->ifa_addr->sa_family == AF_INET);

            if (bUp && (!bLoopback || (bLoopback && bIncludeLoopback)) && bIPv4) {
                struct sockaddr_in *ip = (struct sockaddr_in *) (ifa->ifa_addr);
                struct sockaddr_in *netmask = (struct sockaddr_in *) (ifa->ifa_netmask);

                NICInfo *pNICInfo = new NICInfo();
                pNICInfo->uiIndex = if_nametoindex (ifa->ifa_name);
                pNICInfo->ip = ip->sin_addr;
                pNICInfo->netmask = netmask->sin_addr;
                pNICInfo->broadcast.s_addr = ip->sin_addr.s_addr | ~netmask->sin_addr.s_addr;

                if (bUniqueNetworks) {
                    for (int z=0; z<j; z++) {
                        if (areInSameNetwork (pNICInfo, ppNICsInfo[z])) {
                            bAddIF = false;
                        }
                    }
                }

                if (bAddIF) {
                    ppNICsInfo[j++] = pNICInfo;
                }
                else {
                    delete pNICInfo;
                }
            }
        }

        // Terminator
        ppNICsInfo[j] = NULL;

        freeifaddrs (myaddrs);
        return ppNICsInfo;

    #endif
}

NICInfo ** NetUtils::getNICsInfo (const char *pszIPAddrWild)
{
    NICInfo **ppNICsInfo = getNICsInfo (false, false);
    uint8 ui8NICsIFNum;
    for (ui8NICsIFNum = 0; ppNICsInfo[ui8NICsIFNum] != NULL; ui8NICsIFNum++);
    NICInfo **ppMatchingNICsInfo = new NICInfo*[ui8NICsIFNum+1];

    int j = 0;
    for (int i = 0; ppNICsInfo[i] != NULL; i++) {
        if (wildcardStringCompare (inet_ntoa(ppNICsInfo[i]->ip), pszIPAddrWild)) {
            ppMatchingNICsInfo[j++] = new NICInfo (*ppNICsInfo[i]);
        }
    }
    ppMatchingNICsInfo[j] = NULL; // Terminator
    if (j == 0) {
        delete[] ppMatchingNICsInfo;
        ppMatchingNICsInfo = NULL;
    }

    // Delete the memory allocated by ppNICsInfo
    freeNICsInfo (ppNICsInfo);

    return ppMatchingNICsInfo;
}

NICInfo * NetUtils::getNICInfo (const char *pszIPAddr)
{
    NICInfo **ppNICsInfo = getNICsInfo (false, false);
    NICInfo *pNICInfo = NULL;
    for (int i = 0; ppNICsInfo[i]; i++) {
        if (strcmp (inet_ntoa(ppNICsInfo[i]->ip), pszIPAddr) == 0) {
            pNICInfo = new NICInfo (*ppNICsInfo[i]);
            break;
        }
    }

    // Free the memory allocated by ppNICsInfo
    freeNICsInfo (ppNICsInfo);

    return pNICInfo;    // Maybe NULL if no interface was found with the specified address
}

NICInfo * NetUtils::getNICInfo (struct in_addr ipAddr)
{
    NICInfo **ppNICsInfo = getNICsInfo (false, false);
    NICInfo *pNICInfo = NULL;
    for (int i = 0; ppNICsInfo[i]; i++) {
        if (ppNICsInfo[i]->ip.s_addr == ipAddr.s_addr) {
            pNICInfo = new NICInfo (*ppNICsInfo[i]);
            break;
        }
    }

    // Free the memory allocated by ppNICsInfo
    freeNICsInfo (ppNICsInfo);

    return pNICInfo;    // Maybe NULL if no interface was found with the specified address
}

String NetUtils::getIncomingNIC (const char *pszBindingIPAddr, const char *pszBindingIPAddrNetmask,
                                 const char *pszPacketRemoteAddr, const char *pszPacketDstAddr)
{
    // If the binding address is not the wildcard address, we already know what
    // the incoming interface is
    if (pszBindingIPAddr != NULL && (strcmp (pszBindingIPAddr, "0.0.0.0") != 0)) {
        return String (pszBindingIPAddr);
    }

    // Check the destination address for the packet, and see if it matches any
    // of the available interfaces
    if (pszPacketDstAddr != NULL) {
        if (NetUtils::isBroadcastAddress (pszPacketDstAddr, pszBindingIPAddrNetmask)) {
            if (strcmp (pszPacketDstAddr, "255.255.255.255") != 0) {
                NICInfo **ppNICs = NetUtils::getNICsInfo();
                if (ppNICs != NULL) {
                    InetAddr remoteAddr (pszPacketRemoteAddr);
                    // Iterate through all the interfaces and find the first one that matches
                    bool bFoundOne = false;
                    String incomingIface;
                    for (unsigned int i = 0; ppNICs[i] != NULL; i++) {
                        if (ppNICs[i]->getBroadcastAddr().getIPAddress() == remoteAddr.getIPAddress()) {
                            if (!bFoundOne) {
                                incomingIface = ppNICs[i]->getIPAddrAsString();
                                bFoundOne = true;
                            }
                            else {
                                bFoundOne = false;
                                break;
                            }
                        }
                    }
                    NetUtils::freeNICsInfo (ppNICs);
                    if (bFoundOne) {
                        return String (incomingIface);
                    }
                }
            }
        }
    }

    // If no matching interfaces has yet been found, try to use the source address
    // of the packet, and see if it is in the same subnetwork of any of the available
    // interfaces
    if (pszPacketRemoteAddr != NULL) {
        NICInfo **ppNICs = NetUtils::getNICsInfo();
        if (ppNICs != NULL) {
            InetAddr remoteAddr (pszPacketRemoteAddr);
            // Iterate through all the interfaces and find the first one that matches
            bool bFoundOne = false;
            String incomingIface;
            for (unsigned int i = 0; ppNICs[i] != NULL; i++) {
                if (NetUtils::areInSameNetwork (ppNICs[i]->getIPAddr().getIPAddress(), ppNICs[i]->getNetmask().getIPAddress(),
                                                remoteAddr.getIPAddress(), ppNICs[i]->getNetmask().getIPAddress())) {
                    if (!bFoundOne) {
                        incomingIface = ppNICs[i]->getIPAddrAsString();
                        bFoundOne = true;
                    }
                    else {
                        bFoundOne = false;
                        break;
                    }
                }
            }
            NetUtils::freeNICsInfo (ppNICs);
            if (bFoundOne) {
                return String (incomingIface);
            }
        }
    }

    return String();
}

EtherMACAddr NetUtils::getEtherMACAddrFromString (const char * const pszEtherMACAddr)
{
    static const unsigned int MAC_ADDRESS_SIZE = 6;
    static const char AC_SEPARATOR[] = ":";

    EtherMACAddr etherMACaddr;
    const char *pszMACAddressOctets[MAC_ADDRESS_SIZE];
    char *pszTemp;
    memset (reinterpret_cast<void *> (&etherMACaddr), 0, sizeof(EtherMACAddr));

    if (!pszEtherMACAddr || !checkEtherMACAddressFormat (pszEtherMACAddr)) {
        // Return empty MAC
        return etherMACaddr;
    }

    // Parse Ethernet MAC address in the format A:B:C:D:E:F
    unsigned int i = 0;
    if ((pszMACAddressOctets[i++] = strtok_mt (pszEtherMACAddr, AC_SEPARATOR, &pszTemp)) == NULL) {
        return etherMACaddr;
    }
    for (; i < MAC_ADDRESS_SIZE; ++i) {
        pszMACAddressOctets[i] = strtok_mt (NULL, AC_SEPARATOR, &pszTemp);
        if (pszMACAddressOctets[i] == NULL) {
            return etherMACaddr;
        }
    }

    uint8 *pui8EtherMACAddress = reinterpret_cast<uint8*> (&etherMACaddr);
    for (i = 0; i < MAC_ADDRESS_SIZE; i += 2) {
        pui8EtherMACAddress[i] = static_cast<uint8> (std::strtoul (pszMACAddressOctets[i + 1], 0, 16));
        pui8EtherMACAddress[i + 1] = static_cast<uint8> (std::strtoul (pszMACAddressOctets[i], 0, 16));
    }

    return etherMACaddr;
}

bool NetUtils::checkEtherMACAddressFormat(const char * pszEtherMACAddr)
{
    unsigned int uiByteLength = 0, uiMACLength = 0;
    while (*pszEtherMACAddr) {
        if (*pszEtherMACAddr == ':') {
            if (++uiMACLength > 5) {
                return false;
            }
            ++pszEtherMACAddr;
            uiByteLength = 0;
            continue;
        }
        ++uiByteLength;
        if (uiByteLength > 2) {
            return false;
        }
        if (!checkCharRange (*pszEtherMACAddr, '0', '9') && !checkCharRange (*pszEtherMACAddr, 'a', 'f') &&
            !checkCharRange (*pszEtherMACAddr, 'A', 'F')) {
            return false;
        }
        ++pszEtherMACAddr;
    }

    if (uiMACLength != 5) {
        return false;
    }
    return true;
}

bool NetUtils::isLocalAddress (uint32 ulIPAddr, NICInfo **pMyNetIFs)
{
    const char *pszMethodName = "NetUtils::isLocalAddress";

    // If it is a loopback address we already know that it is local
    if ((ulIPAddr & 255) == 127) {
        return true;
    }

    NICInfo **ppNICsInfo;
    if (pMyNetIFs == NULL) {
        // Get network interface info
        if ((ppNICsInfo = getNICsInfo (false)) == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "Error getting network interfaces info\n");
            return false;
        }
    }
    else {
        ppNICsInfo = pMyNetIFs;
    }

    bool bFound = false;
    for (int i = 0; !bFound && ppNICsInfo[i]; i++) {
        if (ppNICsInfo[i]->ip.s_addr == ulIPAddr) {
            bFound = true;
        }
    }

    if (pMyNetIFs == NULL) {
        freeNICsInfo (ppNICsInfo);
    }
    return bFound;
}

bool NetUtils::isLocalAddress (struct in_addr ipAddr)
{
        return isLocalAddress (ipAddr.s_addr);
}

bool NetUtils::isLocalAddress (const char *pszIPAddr)
{
    const char *pszMethodName = "NetUtils::isLocalAddress";
    struct in_addr ia;
    hostent *phe;

    if ((ia.s_addr = inet_addr (pszIPAddr)) == INADDR_NONE) {
        if ((phe = gethostbyname (pszIPAddr)) == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "Unable to resolve host: %s\n", pszIPAddr);
            return false;
        }
        ia = *(in_addr *) phe->h_addr_list[0];
    }

    return isLocalAddress (ia);
}

String NetUtils::getLocalHostName (void)
{
    #ifdef WIN32
        WORD VersionRequested;
        WSADATA WsaData;
        VersionRequested = MAKEWORD (2,2);
        WSAStartup (VersionRequested, &WsaData);
    #endif

    char pszHostname[PATH_MAX];
    memset (pszHostname, 0, sizeof (pszHostname));
    gethostname (pszHostname, sizeof (pszHostname));

    #ifdef WIN32
        WSACleanup();
    #endif

    return String (pszHostname);
}

uint32 NetUtils::getHostByName (const char *pszHostName)
{
    // First check to see if the hostname is really an IP address
    uint32 ui32Addr = inet_addr (pszHostName);
    if (ui32Addr != INADDR_NONE) {
        return ui32Addr;
    }
    // Try to lookup the hostname
    struct hostent *pHostEnt;
    if (NULL == (pHostEnt = gethostbyname (pszHostName))) {
        return 0;
    }
    if (pHostEnt->h_length != 4) {
        return 0;
    }
    memcpy (&ui32Addr, pHostEnt->h_addr_list[0], 4);
    return ui32Addr;
}

InetAddr NetUtils::determineDestIPAddr (NICInfo **pRemoteNetIFs, NICInfo **pLocalNetIFs)
{
    InetAddr iAddr;
    bool bFound = false;
    bool bFreeMem = false;

    if (pLocalNetIFs == NULL) {
        pLocalNetIFs = getNICsInfo (false, true);
        bFreeMem = true;
    }

    for (int i = 0; pLocalNetIFs[i] != NULL && !bFound; i++) {
        for (int j = 0; pRemoteNetIFs[j] != NULL; j++) {
            if (NetUtils::areInSameNetwork (pLocalNetIFs[i], pRemoteNetIFs[j])) {
                iAddr.setIPAddress (pRemoteNetIFs[j]->ip);
                bFound = true;
                break;
            }
        }
    }

    if (!bFound) {
        // NOTE: if we cannot find any address that is directly reachable
        // we assume that the OS will properly route the packet. This is the
        // case in which we have multiple instances of the group manager are
        // concurrently executing in the same node configured with IPs in
        // different networks:
        //
        // e.g. GroupManager A with IP 192.168.0.1/24
        //      GroupManager B with IP 10.1.2.3/8
        iAddr.setIPAddress (pRemoteNetIFs[0]->ip);
    }

    if (bFreeMem == true) {
        freeNICsInfo (pLocalNetIFs);
    }

    return iAddr;
}

void NetUtils::freeNICsInfo (NICInfo **ppNICsInfo)
{
    if (ppNICsInfo) {
        for (int i = 0; ppNICsInfo[i]; i++) {
            delete ppNICsInfo[i];
            ppNICsInfo[i] = NULL;
        }
        delete[] ppNICsInfo;
    }
}

char * NetUtils::ui32Inetoa (uint32 ui32Addr)
{
    return ui32Inetoa (*(struct in_addr *)&ui32Addr);
}

char * NetUtils::ui32Inetoa (in_addr& addr)
{
    return strDup (inet_ntoa (addr));
}

