/*
* ARPTableMissCache.cpp
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

#include "NLFLib.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    int ARPTableMissCache::insert (uint32 ui32DestIPAddr, NetworkInterface * const pNI, uint8 * const pui8Buf, uint16 ui16PacketLen)
    {
        MutexUnlocker mu (&_m);

        const ARPTableMissPacket * const pATMP = new ARPTableMissPacket (ui16PacketLen, getTimeInMilliseconds(), pNI, pui8Buf);
        if (!_table.contains (ui32DestIPAddr)) {
            _table.put (ui32DestIPAddr, new PtrLList<const ARPTableMissPacket> (pATMP, false));
        }
        else {
            removeExpiredEntries (ui32DestIPAddr, _i64ExpirationTimeInMilliseconds);
            _table.get (ui32DestIPAddr)->append (pATMP);
        }

        _empty = false;

        return 0;
    }

    int ARPTableMissCache::removeExpiredEntries (uint32 ui32IPAddr, int64 i64ExpirationTimeInMilliseconds)
    {
        auto pPtrLList = _table.get (ui32IPAddr);
        if (!pPtrLList) {
            return -1;
        }

        // Remove old entries
        int counter = 0;
        const int64 i64CurrentTime = getTimeInMilliseconds();
        const ARPTableMissPacket * pATMP = nullptr;
        pPtrLList->resetGet();
        while ((pATMP = pPtrLList->getNext()) &&
            ((i64CurrentTime - pATMP->getCacheTime()) >= i64ExpirationTimeInMilliseconds)) {
            delete pPtrLList->remove (pATMP);
            ++counter;
        }

        return counter;
    }

    void ARPTableMissCache::updateEmptyStatus (void)
    {
        auto iterator = _table.getAllElements();
        do {
            auto pPtrLList = iterator.getValue();
            if (pPtrLList && (pPtrLList->getCount() > 0)) {
                // Not empty
                _empty = false;
                return;
            }
        } while (iterator.nextElement());

        // If we get to this point, the table is empty
        _empty = true;
    }

}