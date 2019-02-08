/*
 * ManycastNetworkMessageReceiver.cpp
 *
 *  This file is part of the IHMC Network Message Service Library
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 14, 2011, 1:34 PM
 */

#include "ManycastNetworkMessageReceiver.h"

#include "NetUtils.h"

using namespace NOMADSUtil;

//
// ManycastNetworkMessageReceiver
//

ManycastNetworkMessageReceiver::ManycastNetworkMessageReceiver (NetworkInterfaceManagerListener *pNMSParent, NetworkInterface *pNetIface,
                                                                bool bReceive, NICInfo **ppNICInfos)
    : NetworkMessageReceiver (pNMSParent, pNetIface, bReceive, new ManycastRateEstimator (ppNICInfos), NetworkMessageReceiver::MCAST_NET_RCV)
{
}

ManycastNetworkMessageReceiver::~ManycastNetworkMessageReceiver()
{
}

//
// ManycastRateEstimator
//

ManycastNetworkMessageReceiver::ManycastRateEstimator::ManycastRateEstimator (NICInfo **ppNICInfos)
    : _rateEstimatorsByNetwork (US_INITSIZE, true)
{
    if (ppNICInfos == NULL) {
        exit (-1);
    }
    _ppNICInfos = ppNICInfos;
    for (int i = 0; _ppNICInfos[i] != NULL; i++) {
        uint32 ui32IPAddr = _ppNICInfos[i]->broadcast.s_addr;
        if (ui32IPAddr != INADDR_NONE && ui32IPAddr != INADDR_ANY) {
            _rateEstimatorsByNetwork.put (ui32IPAddr, new NetworkMessageReceiver::RateEstimator());
        }
    }
}

ManycastNetworkMessageReceiver::ManycastRateEstimator::~ManycastRateEstimator (void)
{
    NetUtils::freeNICsInfo (_ppNICInfos);
    _ppNICInfos = NULL;
}

uint32 ManycastNetworkMessageReceiver::getRateByBcastAddr (const char *pszBcastAddr)
{
    InetAddr addr (pszBcastAddr);
    return getRateByBcastAddr (addr.getIPAddress());
}

