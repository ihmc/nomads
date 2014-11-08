/*
 * ManycastForwardingNetworkInterface.cpp
 *
 *  This file is part of the IHMC Util Library
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on January 28, 2014, 6:43 PM
 */

#include "ManycastForwardingNetworkInterface.h"

#include "Logger.h"
#include "NetUtils.h"
#include "NetworkMessage.h"

using namespace NOMADSUtil;

uint8 ManycastForwardingNetworkInterface::MCAST_FWD_IF = 0x01;

ManycastForwardingNetworkInterface::ManycastForwardingNetworkInterface (bool bAsyncTransmission, uint32 ui32StorageDuration)
    : NetworkInterface (MCAST_FWD_IF, bAsyncTransmission),
      _unicastAddresses (ui32StorageDuration)
{
}

ManycastForwardingNetworkInterface::~ManycastForwardingNetworkInterface()
{
}

bool ManycastForwardingNetworkInterface::addForwardingIpAddress (uint32 ui32IPv4Addr)
{
    char *pszIpAddr = NetUtils::ui32Inetoa (ui32IPv4Addr);
    bool rc = false;
    if (pszIpAddr != NULL) {
        rc = addForwardingIpAddress (pszIpAddr);
        free (pszIpAddr);
    }
    return rc;
}

bool ManycastForwardingNetworkInterface::addForwardingIpAddress (const char *pszIpAddr)
{
    if (pszIpAddr == NULL) {
        return false;
    }
    if (NetUtils::isMulticastAddress(pszIpAddr)) {
        return false;
    }
    _m.lock();
    bool rc = _unicastAddresses.put (pszIpAddr);
    _m.unlock();
    return rc;
}

int ManycastForwardingNetworkInterface::sendMessageInternal (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints)
{
    int rc = NetworkInterface::sendMessageInternal (pNetMsg, ui32IPAddr, pszHints);

    if ((!NetUtils::isMulticastAddress (pNetMsg->getTargetAddress()))) {
        return rc;
    }

    _m.lock();
    if (_unicastAddresses.getCount() > 0) {
        TimeBoundedStringHashset::Iterator iter = _unicastAddresses.getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            const char *pszIPAddr = iter.getKey();
            if (pszIPAddr != NULL) {
                if (NetworkInterface::sendMessageInternal (pNetMsg, inet_addr (pszIPAddr), pszHints) != 0) {
                    if (pLogger != NULL) pLogger->logMsg ("ManycastForwardingNetworkInterface::sendMessageInternal",
                            Logger::L_Warning, "could not unicast message to %s\n", pszIPAddr);
                    rc = -2;
                }
            }
        }
    }
    _m.unlock();
    return rc;
}

