#ifndef INCL_ARP_CACHE_H
#define INCL_ARP_CACHE_H

/*
 * ARPCache.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
 * The ARPCache class implements an ARP Cache Table using a hash table.
 * The hash table uses the IP address of a node as its key
 * and stores the MAC address of the network interface of
 * that node as the value for that key.
 */

#include <mutex>
#include <unordered_map>

#include "ConfigurationParameters.h"

#include "net/NetworkHeaders.h"


namespace ACMNetProxy
{
    class ARPCache
    {
    public:
        ARPCache (void);

        void insert (uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr & pMACAddr);
        const NOMADSUtil::EtherMACAddr & lookup (uint32 ui32IPAddr) const;


    private:
        std::unordered_map<uint32, NOMADSUtil::EtherMACAddr> _umARPCache;

        mutable std::mutex _mtx;
    };


    inline ARPCache::ARPCache (void) { }

    inline const NOMADSUtil::EtherMACAddr & ARPCache::lookup (uint32 ui32IPAddr) const
    {
        std::lock_guard<std::mutex> lg{_mtx};
        return (_umARPCache.count (ui32IPAddr) == 1) ?
            _umARPCache.at (ui32IPAddr) : NetProxyApplicationParameters::EMA_INVALID_ADDRESS;
    }

    inline void ARPCache::insert (uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr & ema)
    {
        std::lock_guard<std::mutex> lg{_mtx};
        _umARPCache[ui32IPAddr] = ema;
    }
}

#endif   // #ifndef INCL_ARP_CACHE_H
