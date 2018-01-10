#ifndef INCL_ARP_CACHE_H
#define INCL_ARP_CACHE_H

/*
 * ARPCache.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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

#include "UInt32Hashtable.h"

#include "net/NetworkHeaders.h"


namespace ACMNetProxy
{
    class ARPCache
    {
        public:
            ARPCache (void);

            int insert (uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr &pMACAddr);
            const NOMADSUtil::EtherMACAddr * const lookup (uint32 ui32IPAddr) const;

        private:
            NOMADSUtil::UInt32Hashtable<NOMADSUtil::EtherMACAddr> _arpCache;
    };


    inline ARPCache::ARPCache (void) : _arpCache (true) {}

    inline const NOMADSUtil::EtherMACAddr * const ARPCache::lookup (uint32 ui32IPAddr) const
    {
        return _arpCache.get (ui32IPAddr);
    }

    inline int ARPCache::insert (uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr &pMACAddr)
    {
        delete _arpCache.put (ui32IPAddr, new NOMADSUtil::EtherMACAddr (pMACAddr));
        return 0;
    }
}

#endif   // #ifndef INCL_ARP_CACHE_H
