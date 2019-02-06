#ifndef INCL_PACKET_BUFFER_MANAGER_H
#define INCL_PACKET_BUFFER_MANAGER_H

/*
 * PacketBufferManager.h
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
 * Class that provides the functions to manage an array
 * of memory buffers with mutually exclusive access.
 */

#include <array>
#include <mutex>

#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    class PacketBufferManager
    {
    public:
        PacketBufferManager (void);
        PacketBufferManager (const PacketBufferManager & rhPBM) = delete;
        PacketBufferManager & operator = (const PacketBufferManager & rhPBM) = delete;

        char * const getAndLockWriteBuf (void);
        int findAndUnlockWriteBuf (const void * const pui8Buf) const;


    private:
        char _cTAPBuf[NetProxyApplicationParameters::WRITE_PACKET_BUFFERS][NetProxyApplicationParameters::ETHERNET_MAX_MFS];

        mutable std::array<std::mutex, NetProxyApplicationParameters::WRITE_PACKET_BUFFERS> _amtxTAPBuf;
    };


    inline PacketBufferManager::PacketBufferManager (void) :
        _cTAPBuf{}, _amtxTAPBuf{}
    { }
}

#endif   // #ifndef INCL_PACKET_BUFFER_MANAGER_H
