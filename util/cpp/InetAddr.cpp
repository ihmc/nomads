/*
 * InetAddr.cpp
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

#include "InetAddr.h"

#include <NLFLib.h>
#include <StringTokenizer.h>

using namespace NOMADSUtil;

bool InetAddr::isIPv4Addr (const char *pszAddr)
{
    if (pszAddr == NULL) {
        return false;
    }
    if (strlen (pszAddr) > 15) {
        return false;
    }
    char szAddr[16];
    strcpy (szAddr, pszAddr);
    const char *apszOctets[4];
    uint8 ui8NextOctet = 0;
    apszOctets[0] = apszOctets[1] = apszOctets[2] = apszOctets[3] = NULL;
    char *pszTemp = szAddr;
    bool bInOctet = false;
    while (*pszTemp != '\0') {
        if ((*pszTemp >= '0') && (*pszTemp <= '9')) {
            if (bInOctet) {
                pszTemp++;
            }
            else {
                bInOctet = true;
                apszOctets[ui8NextOctet] = pszTemp;
                pszTemp++;
            }
        }
        else if (*pszTemp == '.') {
            bInOctet = false;
            *pszTemp = '\0';
            ui8NextOctet++;
            if (ui8NextOctet > 3) {
                return false;
            }
            pszTemp++;
        }
        else {
            return false;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (apszOctets[i] == NULL) {
            return false;
        }
        int iOctet = atoi (apszOctets[i]);
        if ((iOctet < 0) || (iOctet > 255)) {
            return false;
        }
    }
    return true;
}
