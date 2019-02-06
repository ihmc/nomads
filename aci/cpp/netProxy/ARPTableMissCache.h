#ifndef INCL_ARP_TABLE_MISS_CACHE_H
#define INCL_ARP_TABLE_MISS_CACHE_H

/*
* ARPTableMissCache.h
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

#include <cstring>
#include <deque>
#include <unordered_map>
#include <mutex>

#include "FTypes.h"
#include "NLFLib.h"


namespace ACMNetProxy
{
    class NetworkInterface;


    class ARPTableMissPacket
    {
    public:
        ARPTableMissPacket (uint16 ui16PacketLen, int64 i64CacheTime, NetworkInterface * const pNI, const uint8 * const pui8Buf);
        ARPTableMissPacket (const ARPTableMissPacket & atmp) = delete;
        ARPTableMissPacket (ARPTableMissPacket && atmp);
        ~ARPTableMissPacket (void);

        ARPTableMissPacket & operator= (const ARPTableMissPacket & rhs) = delete;
        ARPTableMissPacket & operator= (ARPTableMissPacket && rhs);

        bool operator== (const ARPTableMissPacket & rhs) const;

        uint16 getPacketLen (void) const { return _ui16PacketLen; }
        int64 getCacheTime (void) const { return _i64CacheTime; }
        NetworkInterface * const getNetworkInterface (void) const { return _pNI; }
        uint8 * const getPacket (void) const { return _pui8Packet; }


    private:
        uint16 _ui16PacketLen;
        int64 _i64CacheTime;
        NetworkInterface * _pNI;
        uint8 * _pui8Packet;
    };


    class ARPTableMissCache
    {
    public:
        ARPTableMissCache (int64 i64ExpirationTimeInMilliseconds) :
            _uiCachedPackets{0}, _i64ExpirationTimeInMilliseconds{i64ExpirationTimeInMilliseconds} { }
        ARPTableMissCache (const ARPTableMissCache & atmc) = delete;
        ~ARPTableMissCache (void) { }

        int64 setTimeout (const int64 i64TimeoutInMilliseconds);

        bool hasCachedPacketsWithDestination (uint32 ui32DestIPAddr);
        std::deque<ARPTableMissPacket> & lookup (uint32 ui32DestIPAddr);
        bool hasPacketInCache (uint32 ui32DestIPAddr, uint8 * const pui8Buf, uint16 ui16PacketLen) const;
        void insert (uint32 ui32DestIPAddr, NetworkInterface * const pNI, uint8 * const pui8Buf, uint16 ui16PacketLen);
        void remove (uint32 ui32DestIPAddr);

        void clearTableFromExpiredEntries (void);


    private:
        unsigned int removeExpiredEntries (uint32 ui32IPAddr, int64 i64ExpirationTime);

        bool isEmpty (void) const { return _uiCachedPackets == 0; }
        unsigned int getCachedPacketsNum (void) const { return _uiCachedPackets; }

        unsigned int _uiCachedPackets;
        int64 _i64ExpirationTimeInMilliseconds;
        std::unordered_map<uint32, std::deque<ARPTableMissPacket>> _umPacketsCache;

        mutable std::mutex _mtx;
    };


    inline ARPTableMissPacket::ARPTableMissPacket (uint16 ui16PacketLen, int64 i64CacheTime, NetworkInterface * const pNI, const uint8 * const pui8Buf) :
        _ui16PacketLen{ui16PacketLen}, _i64CacheTime{i64CacheTime}, _pNI{pNI}, _pui8Packet{new uint8[ui16PacketLen]}
    {
        memcpy (_pui8Packet, pui8Buf, _ui16PacketLen);
    }

    inline ARPTableMissPacket::ARPTableMissPacket (ARPTableMissPacket && atmp) :
        _ui16PacketLen{std::move (atmp._ui16PacketLen)}, _i64CacheTime{std::move (atmp._i64CacheTime)},
        _pNI{std::move (atmp._pNI)}, _pui8Packet{std::move (atmp._pui8Packet)}
    {
        atmp._pNI = nullptr;
        atmp._pui8Packet = nullptr;
    }

    inline ARPTableMissPacket::~ARPTableMissPacket (void)
    {
        delete[] _pui8Packet;
    }

    inline ARPTableMissPacket & ARPTableMissPacket::operator= (ARPTableMissPacket && rhs)
    {
        std::swap (_ui16PacketLen, rhs._ui16PacketLen);
        std::swap (_i64CacheTime, rhs._i64CacheTime);

        _pNI = nullptr;
        std::swap (_pNI, rhs._pNI);

        _pui8Packet = nullptr;
        std::swap (_pui8Packet, rhs._pui8Packet);
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

    inline bool ARPTableMissCache::hasCachedPacketsWithDestination (uint32 ui32DestIPAddr)
    {
        std::lock_guard<std::mutex> lg{_mtx};
        return _uiCachedPackets && _umPacketsCache.count (ui32DestIPAddr) > 0;
    }

    inline std::deque<ARPTableMissPacket> & ARPTableMissCache::lookup (uint32 ui32DestIPAddr)
    {
        std::lock_guard<std::mutex> lg{_mtx};
        if (!isEmpty()) {
            _uiCachedPackets -= removeExpiredEntries (ui32DestIPAddr, _i64ExpirationTimeInMilliseconds);
        }

        return _umPacketsCache[ui32DestIPAddr];
    }

    inline void ARPTableMissCache::insert (uint32 ui32DestIPAddr, NetworkInterface * const pNI, uint8 * const pui8Buf, uint16 ui16PacketLen)
    {
        std::lock_guard<std::mutex> lg{_mtx};

        if (!isEmpty() && (_umPacketsCache.count (ui32DestIPAddr) > 0)) {
            _uiCachedPackets -= removeExpiredEntries (ui32DestIPAddr, _i64ExpirationTimeInMilliseconds);
        }
        _umPacketsCache[ui32DestIPAddr].emplace_back (ui16PacketLen, NOMADSUtil::getTimeInMilliseconds(), pNI, pui8Buf);
        ++_uiCachedPackets;
    }

    inline void ARPTableMissCache::remove (uint32 ui32DestIPAddr)
    {
        std::lock_guard<std::mutex> lg{_mtx};

        if (!isEmpty() && (_umPacketsCache.count (ui32DestIPAddr) > 0)) {
            _uiCachedPackets -= _umPacketsCache.at (ui32DestIPAddr).size();
            _umPacketsCache.erase (ui32DestIPAddr);
        }
    }

}

#endif