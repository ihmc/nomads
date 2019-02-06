/*
 * PacketBufferManager.cpp
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
 */

#include "Logger.h"

#include "PacketBufferManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    char * const PacketBufferManager::getAndLockWriteBuf (void)
    {
        for (int i = 0; i < NetProxyApplicationParameters::WRITE_PACKET_BUFFERS; i++) {
            if (_amtxTAPBuf[i].try_lock()) {
                return _cTAPBuf[i];
            }
        }

        checkAndLogMsg ("PacketBufferManager::getWriteBuf", NOMADSUtil::Logger::L_Warning,
                        "could not find a free buffer; waiting for one to be freed\n");

        _amtxTAPBuf[0].lock();
        return _cTAPBuf[0];
    }

    int PacketBufferManager::findAndUnlockWriteBuf (const void * const pui8Buf) const
    {
        for (int i = 0; i < NetProxyApplicationParameters::WRITE_PACKET_BUFFERS; i++) {
            if ((const void * const) _cTAPBuf[i] == pui8Buf) {
                _amtxTAPBuf[i].unlock();
                return 0;
            }
        }

        return -1;
    }

}
