/*
 * MulticastUDPDatagramSocket.cpp
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

#include "MulticastUDPDatagramSocket.h"

#include "net/NetUtils.h"
#include <stdio.h>
#include <string.h>

#if defined (WIN32)
    #include <Ws2tcpip.h>
#endif

#include "Logger.h"
#include "StringTokenizer.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

#if defined UNIX
    MulticastUDPDatagramSocket::MulticastUDPDatagramSocket (bool bPktInfoEnabled)
        : UDPDatagramSocket (bPktInfoEnabled)
#else
    MulticastUDPDatagramSocket::MulticastUDPDatagramSocket()
#endif
{
    // Default values
    _ui8TTL = 1;
    _bLoopbackMode = false; //disabled

    /*!!*/ // This is temporary code for Agile Bloodhound 2012 - sets up UDP destinations
    FILE *fileRelayers = NULL;
    String relayersFilePath = getPathSepCharAsString();
    relayersFilePath += "ihmc";
    relayersFilePath += getPathSepCharAsString();
    relayersFilePath += "conf";
    relayersFilePath += getPathSepCharAsString();
    relayersFilePath += "udprelays.cfg";
    if (NULL == (fileRelayers = fopen (relayersFilePath, "r"))) {
        checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                        "did not find file <%s> - assuming no UDP relays have been configured\n", (const char*) relayersFilePath);
    }
    else {
        char relayEntryBuf[256];
        while (NULL != fgets (relayEntryBuf, sizeof (relayEntryBuf)-1, fileRelayers)) {
            relayEntryBuf[255] = '\0';
            StringTokenizer st (relayEntryBuf, ':');
            const char *pszRelayIP = st.getNextToken();
            const char *pszRelayPort = st.getNextToken();
            if ((pszRelayIP != NULL) && (pszRelayPort != NULL)) {
                checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                                "adding %s:%d as a unicast UDP relay\n", pszRelayIP, atoi (pszRelayPort));
                InetAddr relayAddr (pszRelayIP);
                relayAddr.setPort (atoi (pszRelayPort));
                addUnicastRelayAddress (relayAddr);
            }
        }
        fclose (fileRelayers);
    }

    /*!!*/ // This is temporary code for Agile Bloodhound - sets up relays for Multicast
    relayersFilePath = getPathSepCharAsString();
    relayersFilePath += "ihmc";
    relayersFilePath += getPathSepCharAsString();
    relayersFilePath += "conf";
    relayersFilePath += getPathSepCharAsString();
    relayersFilePath += "mcastrelays.cfg";
    if (NULL == (fileRelayers = fopen (relayersFilePath, "r"))) {
        checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                        "did not find file <%s>\n", (const char*) relayersFilePath);
        // Try a different directory
        relayersFilePath = getPathSepCharAsString();
        relayersFilePath += "sdcard";
        relayersFilePath += getPathSepCharAsString();
        relayersFilePath += "ihmc";
        relayersFilePath += getPathSepCharAsString();
        relayersFilePath += "conf";
        relayersFilePath += getPathSepCharAsString();
        relayersFilePath += "mcastrelays.cfg";
        if (NULL == (fileRelayers = fopen (relayersFilePath, "r"))) {
            checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                            "did not find file <%s>\n", (const char*) relayersFilePath);
            // Try a different directory
            relayersFilePath = getPathSepCharAsString();
            relayersFilePath += "sdcard";
            relayersFilePath += getPathSepCharAsString();
            relayersFilePath += "external_sd";
            relayersFilePath += getPathSepCharAsString();
            relayersFilePath += "ihmc";
            relayersFilePath += getPathSepCharAsString();
            relayersFilePath += "conf";
            relayersFilePath += getPathSepCharAsString();
            relayersFilePath += "mcastrelays.cfg";
            if (NULL == (fileRelayers = fopen (relayersFilePath, "r"))) {
                checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                                "did not find file <%s>\n", (const char*) relayersFilePath);
                // Try a different directory
                relayersFilePath = getPathSepCharAsString();
                relayersFilePath += "sdcard-ext";
                relayersFilePath += getPathSepCharAsString();
                relayersFilePath += "ihmc";
                relayersFilePath += getPathSepCharAsString();
                relayersFilePath += "conf";
                relayersFilePath += getPathSepCharAsString();
                relayersFilePath += "mcastrelays.cfg";
                if (NULL == (fileRelayers = fopen (relayersFilePath, "r"))) {
                    checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                                    "did not find file <%s>\n", (const char*) relayersFilePath);
                }
            }
        }
    }

    if (fileRelayers != NULL) {
        checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                        "using file <%s> as the multicast relays configuration file\n", (const char *) relayersFilePath);
        char relayEntryBuf[256];
        while (NULL != fgets (relayEntryBuf, sizeof (relayEntryBuf)-1, fileRelayers)) {
            relayEntryBuf[255] = '\0';
            StringTokenizer st (relayEntryBuf, ':');
            const char *pszRelayIP = st.getNextToken();
            const char *pszRelayPort = st.getNextToken();
            if ((pszRelayIP != NULL) && (pszRelayPort != NULL)) {
                checkAndLogMsg ("MulticastUDPDatagramSocket::MulticastUDPDatagramSocket", Logger::L_Info,
                                "adding %s:%d as a multicast relayer\n", pszRelayIP, atoi (pszRelayPort));
                InetAddr relayAddr (pszRelayIP);
                relayAddr.setPort (atoi (pszRelayPort));
                addMulticastRelayAddress (relayAddr);
            }
        }
        fclose (fileRelayers);
    }
}

MulticastUDPDatagramSocket::~MulticastUDPDatagramSocket()
{
}

int MulticastUDPDatagramSocket::joinGroup (uint32 ui32MulticastGroup, uint32 ui32ListenAddr)
{
    if (NetUtils::isMulticastAddress (InetAddr (ui32MulticastGroup))) {
        struct ip_mreq imr;
        memset (&imr, 0, sizeof (struct ip_mreq));
        imr.imr_multiaddr.s_addr = ui32MulticastGroup; 
        imr.imr_interface.s_addr = ui32ListenAddr;
        if (setsockopt (sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &imr, sizeof (struct ip_mreq)) < 0) {
            return -1;
        }
    }
    else {
        // Invalid multicast address
        return -2;
    }

    return 0;
}

int MulticastUDPDatagramSocket::leaveGroup (uint32 ui32MulticastGroup, uint32 ui32ListenAddr)
{
    if (NetUtils::isMulticastAddress (InetAddr (ui32MulticastGroup))) {
        struct ip_mreq imr;
        memset (&imr, 0, sizeof (struct ip_mreq));
        imr.imr_multiaddr.s_addr = ui32MulticastGroup; 
        imr.imr_interface.s_addr = ui32ListenAddr;
        if (setsockopt (sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &imr, sizeof (struct ip_mreq)) < 0) {
            return -1;
        }
    }
    else {
        // Invalid multicast address
        return -2;
    }

    return 0;
}

int MulticastUDPDatagramSocket::sendTo (uint32 ui32IPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
    int rc;
    rc = UDPDatagramSocket::sendTo (ui32IPAddr, ui16Port, pBuf, iBufSize);
    if (rc <= 0) {
        return rc;
    }

    if (!NetUtils::isMulticastAddress (ui32IPAddr)) {
        return rc;
    }

    // Now send to any relay destinations
    if (_ucastRelayDestinations.getHighestIndex() >= 0) {
        // There is at least one Unicast relay configured
        for (int i = 0; i <= _ucastRelayDestinations.getHighestIndex(); i++) {
            int rc2;
            rc2 = UDPDatagramSocket::sendTo (_ucastRelayDestinations[i]->getIPAddress(), _ucastRelayDestinations[i]->getPort(), pBuf, iBufSize, pszHints);
            if (rc2 <= 0) {
                checkAndLogMsg ("MulticastUDPDatagramSocket::sendTo", Logger::L_MildError,
                                "unicast relay failed - sendTo failed for relay %s:%d; rc = %d\n",
                                _ucastRelayDestinations[i]->getIPAsString(), (int) _ucastRelayDestinations[i]->getPort(), rc2);
            }
        }
    }

    if (_mcastRelayDestinations.getHighestIndex() >= 0) {
        // There is at least one Multicast relay configured
        uint8 ui8RelayPacketBuf [1500];
        if (sizeof (ui8RelayPacketBuf) < (iBufSize + 6)) {
            checkAndLogMsg ("MulticastUDPDatagramSocket::sendTo", Logger::L_MildError,
                            "mcast relay failed - message size is too large - can be at the most 1494 bytes\n");
            return rc;    // For now, just return the result code from the original sendTo
        }
        // Put the IP Address in the relay packet
        memcpy (ui8RelayPacketBuf+0, &ui32IPAddr, 4);
        // Put the port in the relay packet
        memcpy (ui8RelayPacketBuf+4, &ui16Port, 2);
        // Copy the data into the relay packet
        memcpy (ui8RelayPacketBuf+6, pBuf, iBufSize);
        for (int i = 0; i <= _mcastRelayDestinations.getHighestIndex(); i++) {
            int rc2;
            rc2 = UDPDatagramSocket::sendTo (_mcastRelayDestinations[i]->getIPAddress(), _mcastRelayDestinations[i]->getPort(), ui8RelayPacketBuf, iBufSize + 6);
            if (rc2 <= 0) {
                checkAndLogMsg ("MulticastUDPDatagramSocket::sendTo", Logger::L_MildError,
                                "mcast relay failed - sendTo failed for relay %s:%d; rc = %d\n",
                                _mcastRelayDestinations[i]->getIPAsString(), (int) _mcastRelayDestinations[i]->getPort(), rc2);
            }
        }
    }
    // Return the original rc so that the caller's observed behavior is the same
    return rc;
}

int MulticastUDPDatagramSocket::setTTL (uint8 ui8TTL)
{
    if (setsockopt (sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (char*) &ui8TTL, sizeof (ui8TTL)) == 0) {
        _ui8TTL = ui8TTL;
        return 0;
    }
    return -1;
}

uint8 MulticastUDPDatagramSocket::getTTL()
{
    return _ui8TTL;
}

int MulticastUDPDatagramSocket::setLoopbackMode (bool bLoopbackMode)
{
    u_char enabled = bLoopbackMode;

    if (setsockopt (sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*) &enabled, sizeof (enabled)) == 0) {
        _bLoopbackMode = bLoopbackMode;
        return 0;
    }
    return -1;
}

bool MulticastUDPDatagramSocket::getLoopbackMode (void)
{
    return _bLoopbackMode;
}

uint16 MulticastUDPDatagramSocket::getMTU()
{
    return UDPDatagramSocket::getMTU();
}

int MulticastUDPDatagramSocket::addUnicastRelayAddress (InetAddr relayAddr)
{
    _ucastRelayDestinations[_ucastRelayDestinations.getHighestIndex()+1] = new InetAddr (relayAddr);
    return 0;
}

int MulticastUDPDatagramSocket::addMulticastRelayAddress (InetAddr relayAddr)
{
    _mcastRelayDestinations[_mcastRelayDestinations.getHighestIndex()+1] = new InetAddr (relayAddr);
    return 0;
}
