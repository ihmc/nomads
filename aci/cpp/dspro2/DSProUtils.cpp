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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#include "DSProUtils.h"

#include "DSPro.h"
#include "Defs.h"
#include "MocketsAdaptor.h"
#include "TCPAdaptor.h"
#include "UDPAdaptor.h"

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
            int rc = dspro.addPeer (adaptorType, nullptr, iter.getKey(), ui16Port);
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
        if ((pszPeerAddresses == nullptr) || (strlen (pszPeerAddresses) <= 0)) {
            return;
        }
        StringTokenizer tokenizer (pszPeerAddresses, ';', ';');
        const char *pszRemoteAddress;
        while ((pszRemoteAddress = tokenizer.getNextToken()) != nullptr) {
            addresses.put (pszRemoteAddress);
        }
    }
}

namespace DSPRO_UTILS
{
    int addPeersInternal (DSPro &dspro, ConfigManager &cfgMgr, AdaptorType adaptorType, uint16 ui16DefaultPort, StringHashset &cumulativeFilter)
    {
        const char *pszMethodName = "DSProUtils::addPeersInternal";

        String type (getAdaptorTypeAsString (adaptorType));
        type.convertToLowerCase();
        String base ("aci.dspro.adaptor."); base += type;
        String port (base + ".port");
        String addr (base + ".peer.addr");

        const uint16 ui16Port = static_cast<uint16> (cfgMgr.getValueAsInt (port, ui16DefaultPort));
        StringHashset addresses;
        parsePeers (cfgMgr.getValue (addr), addresses);
        const int iCount = addresses.getCount();
        addresses.removeAll (cumulativeFilter);
        if (iCount > addresses.getCount()) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "trying to connect to the same peer with another adaptor. "
                            "The duplicated connections via %s will be dropped.\n", type.c_str (), type.c_str());
        }
        int rc = addPeer (dspro, adaptorType, addresses, ui16Port);

        StringHashset::Iterator iter = addresses.getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            cumulativeFilter.put (iter.getKey());
        }

        return rc;
    }
}

int DSProUtils::addPeers (DSPro &dspro, ConfigManager &cfgMgr)
{
    StringHashset cumulativeFilter;                 // Avoids that peers reached via Mockets are also reached via TCP
    int mocketsRc = DSPRO_UTILS::addPeersInternal (dspro, cfgMgr, MOCKETS, MocketsAdaptor::DEFAULT_PORT, cumulativeFilter);
    int tcpRc = DSPRO_UTILS::addPeersInternal (dspro, cfgMgr, TCP, TCPAdaptor::DEFAULT_PORT, cumulativeFilter);

    // TODO: at this time, UDP adaptor is only used by the NetLogger, therefore it is not necessary to filter out peers that
    //       are already connected using other adaptors.
    StringHashset emptyFilter;
    int udpRc = DSPRO_UTILS::addPeersInternal (dspro, cfgMgr, UDP, UDPAdaptor::DEFAULT_PORT, emptyFilter);

    return (mocketsRc + tcpRc + udpRc);
}
