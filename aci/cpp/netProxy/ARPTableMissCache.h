#ifndef INCL_ARP_TABLE_MISS_CACHE_H
#define INCL_ARP_TABLE_MISS_CACHE_H

/*
* ARPTableMissCache.h
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

#include "net/NetworkHeaders.h"
#include "PtrLList.h"
#include "UInt32Hashtable.h"
#include "Mutex.h"


namespace ACMNetProxy
{
    class NetworkInterface;

    class ARPTableMissPacket
    {
    public:
        ARPTableMissPacket (uint16 ui16PacketLen, int64 i64CacheTime, NetworkInterface * const pNI, const uint8 * const pui8Buf);
        ARPTableMissPacket (const ARPTableMissPacket & atmp) = delete;
        ARPTableMissPacket & operator= (const ARPTableMissPacket & rhs) = delete;
        ~ARPTableMissPacket (void);

        bool operator== (const ARPTableMissPacket & rhs) const;

        uint16 getPacketLen (void) const { return _ui16PacketLen; }
        int64 getCacheTime (void) const { return _i64CacheTime; }
        NetworkInterface * const getNetworkInterface (void) const { return _pNI; }
        uint8 * const getPacket (void) const { return _pui8Packet; }


    private:
        const uint16 _ui16PacketLen;
        const int64 _i64CacheTime;
        NetworkInterface * const _pNI;
        uint8 * const _pui8Packet;
    };


    class ARPTableMissCache
    {
    public:
        ARPTableMissCache (int64 i64ExpirationTimeInMilliseconds) :
            _i64ExpirationTimeInMilliseconds (i64ExpirationTimeInMilliseconds), _empty {true} {}
        ARPTableMissCache (const ARPTableMissCache & atmc) = delete;
        ~ARPTableMissCache (void) {}

        int64 setTimeout (const int64 i64TimeoutInMilliseconds);

        NOMADSUtil::PtrLList<const ARPTableMissPacket> * const lookup (uint32 ui32IPAddr);
        int insert (uint32 ui32DestIPAddr, NetworkInterface * const pNI, uint8 * const pui8Buf, uint16 ui16PacketLen);
        NOMADSUtil::PtrLList<const ARPTableMissPacket> * const remove (uint32 ui32IPAddr);

        bool isEmpty (void) const { return _empty; }


    private:
        int removeExpiredEntries (uint32 ui32IPAddr, int64 i64ExpirationTime);
        void updateEmptyStatus (void);

        int64 _i64ExpirationTimeInMilliseconds;
        bool _empty;
        NOMADSUtil::UInt32Hashtable<NOMADSUtil::PtrLList<const ARPTableMissPacket>> _table;

        mutable NOMADSUtil::Mutex _m;
    };


    inline ARPTableMissPacket::ARPTableMissPacket (uint16 ui16PacketLen, int64 i64CacheTime, NetworkInterface * const pNI, const uint8 * const pui8Buf) :
        _ui16PacketLen (ui16PacketLen), _i64CacheTime (i64CacheTime), _pNI (pNI), _pui8Packet (new uint8[ui16PacketLen])
    {
        memcpy (_pui8Packet, pui8Buf, _ui16PacketLen);
    }

    inline ARPTableMissPacket::~ARPTableMissPacket (void)
    {
        delete[] _pui8Packet;
    }

    inline bool ARPTableMissPacket::operator== (const ARPTableMissPacket & rhs) const
    {
        return this == &rhs;
    }

    inline int64 ARPTableMissCache::setTimeout (const int64 i64TimeoutInMilliseconds)
    {
        if (i64TimeoutInMilliseconds < 0) {
            return -1;
        }

        const int64 i64CurrentTimeout = _i64ExpirationTimeInMilliseconds;
        _i64ExpirationTimeInMilliseconds = i64TimeoutInMilliseconds;

        return i64CurrentTimeout;
    }

    inline NOMADSUtil::PtrLList<const ARPTableMissPacket> * const ARPTableMissCache::lookup (uint32 ui32IPAddr)
    {
        NOMADSUtil::MutexUnlocker mu (&_m);
        removeExpiredEntries (ui32IPAddr, _i64ExpirationTimeInMilliseconds);
        updateEmptyStatus();

        return _table.get (ui32IPAddr);
    }

    inline NOMADSUtil::PtrLList<const ARPTableMissPacket> * const ARPTableMissCache::remove (uint32 ui32IPAddr)
    {
        NOMADSUtil::MutexUnlocker mu (&_m);
        removeExpiredEntries (ui32IPAddr, _i64ExpirationTimeInMilliseconds);
        auto ret = _table.remove (ui32IPAddr);
        updateEmptyStatus();

        return ret;
    }
}

#endif