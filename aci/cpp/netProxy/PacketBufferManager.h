#ifndef INCL_PACKET_BUFFER_MANAGER_H
#define INCL_PACKET_BUFFER_MANAGER_H

/*
 * PacketBufferManager.h
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
 * Class that provides the functions to manage an array
 * of memory buffers with mutually exclusive access.
 */

#include "Mutex.h"

#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    class PacketBufferManager
    {
        public:
            static PacketBufferManager * const getPacketBufferManagerInstance (void);

            char * const getAndLockWriteBuf (void);
            int findAndUnlockWriteBuf (const void * const pui8Buf) const;

        private:
            PacketBufferManager (void);
            PacketBufferManager (const PacketBufferManager &rhPBM);

            PacketBufferManager & operator = (const PacketBufferManager &rhPBM);

            char _cTAPBuf[NetProxyApplicationParameters::WRITE_PACKET_BUFFERS][NetProxyApplicationParameters::ETHERNET_MAX_MFS];

            mutable NOMADSUtil::Mutex _mTAPBuf[NetProxyApplicationParameters::WRITE_PACKET_BUFFERS];
    };


    inline PacketBufferManager * const PacketBufferManager::getPacketBufferManagerInstance (void)
    {
        static PacketBufferManager packetBufferManager;

        return &packetBufferManager;
    }
}

#endif   // #ifndef INCL_PACKET_BUFFER_MANAGER_H
