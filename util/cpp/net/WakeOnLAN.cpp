/* 
 * WakeOnLAN.cpp
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

#include "WakeOnLAN.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Logger.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

#if defined (UNIX)
    #define stricmp strcasecmp
#endif

WakeOnLAN::WakeOnLAN (void)
{
}

WakeOnLAN::~WakeOnLAN (void)
{
}

int WakeOnLAN::init (void)
{
    int rc;
    if (0 != (rc = _dgSocket.init())) {
        checkAndLogMsg ("WakeOnLAN::init", Logger::L_MildError,
                        "failed to initialize DatagramSocket; rc = %d\n", rc);
        return -1;
    }
    return 0;
}

int WakeOnLAN::wakeUp (const char *pszMACAddr)
{
    int rc;
    uint32 aui32MACAddr [MAC_ADDR_LEN];
    uint8 aui8MACAddr [MAC_ADDR_LEN];
    if (MAC_ADDR_LEN != (rc = sscanf (pszMACAddr, "%2x:%2x:%2x:%2x:%2x:%2x",   /*!!*/ // NOTE: There is a dependency between this format specifier and MAC_ADDR_LEN
                                      &aui32MACAddr[0], &aui32MACAddr[1], &aui32MACAddr[2], &aui32MACAddr[3], &aui32MACAddr[4], &aui32MACAddr[5]))) {
        checkAndLogMsg ("WakeOnLAN::wakeUp1", Logger::L_MildError,
                        "failed to parse MAC address - expecting %d octets; got %d octets\n", MAC_ADDR_LEN, rc);
        return -1;
    }
    for (int i = 0; i < 6; i++) {
        aui8MACAddr[i] = (uint8) aui32MACAddr[i];
    }
    if (0 != (rc = wakeUp (aui8MACAddr))) {
        checkAndLogMsg ("WakeOnLAN::wakeUp1", Logger::L_MildError,
                        "wakeUp2 failed with rc = %d\n", rc);
        return -2;
    }
    return 0;
}

int WakeOnLAN::wakeUp (uint8 *pui8MACAddr)
{
    int rc;
    uint8 aui8WOLPacket [6 + 16 * MAC_ADDR_LEN];
    for (int i = 0; i < 6; i++) {
        aui8WOLPacket[i] = 0xFF;
    }
    for (int i = 0; i < 16 * MAC_ADDR_LEN; i++) {
        aui8WOLPacket[i+6] = pui8MACAddr [i % MAC_ADDR_LEN];
    }
    if ((rc = _dgSocket.sendTo ("255.255.255.255", 7, aui8WOLPacket, sizeof (aui8WOLPacket))) < 0) {
        checkAndLogMsg ("WakeOnLAN::wakeUp2", Logger::L_MildError,
                        "sendTo() on UDPDatagramSocket failed with rc = %d; errno = %d (%s)\n",
                        rc, _dgSocket.getLastError(), strerror (_dgSocket.getLastError()));
        return -1;
    }
    return 0;
}
