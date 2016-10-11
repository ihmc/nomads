/*
 * DSProUtils.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Author: Giacomo Benincasa	(gbenincasa@ihmc.us)
 */

#include "DSProUtils.h"

#include "DSPro.h"
#include "Defs.h"
#include "MocketsAdaptor.h"
#include "TCPAdaptor.h"

#include "Logger.h"
#include "StringTokenizer.h"
#include "StringHashset.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    int addPeer (DSPro &dspro, AdaptorType adaptorType, StringHashset &addresses, uint16 ui16Port)
    {
        int allRc = 0;
        const char *pszMethodName = "DSProUtils::addPeer";
        StringHashset::Iterator iter = addresses.getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            int rc = dspro.addPeer (adaptorType, NULL, iter.getKey(), ui16Port);
            if (rc < 0) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not connect "
                    "to %s:%u via %s. Returned %d.\n", iter.getKey(), ui16Port,
                    getAdaptorTypeAsString (adaptorType), rc);
                allRc = -1;
            }
        }
        return allRc;
    }

    void parsePeers (const char *pszPeerAddresses, StringHashset &addresses)
    {
        StringTokenizer tokenizer (pszPeerAddresses, ';', ';');
        const char *pszRemoteAddress;
        while ((pszRemoteAddress = tokenizer.getNextToken()) != NULL) {
            addresses.put (pszRemoteAddress);
        }
    }
}

int DSProUtils::addPeers (DSPro &dspro, ConfigManager &cfgMgr)
{
    const char *pszMethodName = "DSProUtils::addPeers";
    const uint16 ui16MocketsPort = static_cast<uint16>(cfgMgr.getValueAsInt ("aci.dspro.adaptor.mockets.port", MocketsAdaptor::DEFAULT_PORT));
    StringHashset mocketsAddresses;
    parsePeers (cfgMgr.getValue ("aci.dspro.adaptor.mockets.peer.addr"), mocketsAddresses);
    const int mocketsRc = addPeer (dspro, MOCKETS, mocketsAddresses, ui16MocketsPort);

    const uint16 ui16TCPPort = static_cast<uint16>(cfgMgr.getValueAsInt ("aci.dspro.adaptor.tcp.port", TCPAdaptor::DEFAULT_PORT));
    StringHashset tcpAddresses;
    parsePeers (cfgMgr.getValue ("aci.dspro.adaptor.tcp.peer.addr"), tcpAddresses);
    const int iCount = tcpAddresses.getCount();
    tcpAddresses.removeAll (mocketsAddresses);
    if (iCount > tcpAddresses.getCount()) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "trying to connect to the same peer with both Mockets and TCP. "
                        "The duplicated connections via TCP will be dropped.\n");
    }
    const int tcpRc = addPeer (dspro, TCP, tcpAddresses, ui16TCPPort);

    return (mocketsRc + tcpRc);
}

