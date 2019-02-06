/*
* ARPTableMissCache.cpp
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
* The ARPTableMissCache class implements a table to store packets that
* NetProxy cannot send because the ARP Cache does not have the MAC
* address of the destination.
* The table uses a hash map where the key is the IP address of the
* destination and the value is a list of packets to transmit to that
* host. Each entry of the list also stores a timestamp of when the
* packet was inserted into the table and a pointer to the interface
* from which the packet will be sent.
*/

#include "ARPTableMissCache.h"


namespace ACMNetProxy
{
    bool ARPTableMissCache::hasPacketInCache (uint32 ui32DestIPAddr, uint8 * const pui8Buf, uint16 ui16PacketLen) const
    {
        std::lock_guard<std::mutex> lg{_mtx};

        if (!isEmpty() && (_umPacketsCache.count (ui32DestIPAddr) > 0)) {
            for (auto & atmp : _umPacketsCache.at (ui32DestIPAddr)) {
                if (atmp.getPacketLen() != ui16PacketLen) {
                    continue;
                }

                unsigned int i = 0;
                while ((i < ui16PacketLen) && (atmp.getPacket()[i] == pui8Buf[i])) {
                    ++i;
                }

                if (i == ui16PacketLen) {
                    return true;
                }
            }
        }

        return false;
    }

    unsigned int ARPTableMissCache::removeExpiredEntries (uint32 ui32IPAddr, int64 i64ExpirationTimeInMilliseconds)
    {
        auto & rdeqATMP = _umPacketsCache[ui32IPAddr];

        // Remove old entries
        int counter = 0;
        const int64 i64CurrentTime = NOMADSUtil::getTimeInMilliseconds();
        while ((rdeqATMP.size() > 0) && ((i64CurrentTime - rdeqATMP.front().getCacheTime()) >= i64ExpirationTimeInMilliseconds)) {
            rdeqATMP.pop_front();
            ++counter;
        }

        return counter;
    }

    void ARPTableMissCache::clearTableFromExpiredEntries (void)
    {
        std::lock_guard<std::mutex> lg{_mtx};

        for (auto it = _umPacketsCache.begin(); it != _umPacketsCache.end(); ) {
            if (it->second.size() > 0) {
                // Remove all expired entries
                auto uiRemovedPackets = removeExpiredEntries (it->first, _i64ExpirationTimeInMilliseconds);
                if (uiRemovedPackets > 0) {
                    _uiCachedPackets -= uiRemovedPackets;
                    if (it->second.size() == 0) {
                        // Remove empty deque
                        _umPacketsCache.erase (it++);
                        continue;
                    }
                }
            }
            else {
                // Remove empty deque
                _umPacketsCache.erase (it++);
                continue;
            }

            // Move on to the next deque
            ++it;
        }
    }

}