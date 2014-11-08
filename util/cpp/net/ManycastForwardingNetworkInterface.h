/*
 * ManycastForwardingNetworkInterface.h
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

#ifndef INCL_MANYCAST_FORWARDING_NETWORK_INTERFACE_H
#define	INCL_MANYCAST_FORWARDING_NETWORK_INTERFACE_H

#include "NetworkInterface.h"

#include "Mutex.h"
#include "TimeBoundedStringHashset.h"

namespace NOMADSUtil
{
    class ManycastForwardingNetworkInterface : public NetworkInterface
    {
        public:
            static uint8 MCAST_FWD_IF;

            ManycastForwardingNetworkInterface (bool bAsyncTransmission = false, uint32 ui32StorageDuration = 60000);
            virtual ~ManycastForwardingNetworkInterface (void);

            bool addForwardingIpAddress (uint32 ui32IPv4Addr);
            bool addForwardingIpAddress (const char *pszKey);

        protected:
            int sendMessageInternal (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints);

        private:
            Mutex _m;
            TimeBoundedStringHashset _unicastAddresses;
    };
}

#endif	// INCL_MANYCAST_FORWARDING_NETWORK_INTERFACE_H

