#ifndef MUTEX_UDP_QUEUE_H
#define MUTEX_UDP_QUEUE_H

/*
 * MutexUDPQueue.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
 * The MutexPtrQueue class extends the PtrQueue class by adding
 * a mutex variable to it to grant mutually exclusive access.
 */

#include "Mutex.h"
#include "PtrQueue.h"

#include "ConfigurationParameters.h"
#include "UDPDatagramPacket.h"


namespace ACMNetProxy
{
    class MutexUDPQueue : public NOMADSUtil::PtrQueue<UDPDatagramPacket>
    {
    public:
        enum EnqueuingReturnValue
        {
            ENQUEUING_NULL = -2,
            ENQUEUING_ERROR,
            ENQUEUING_SUCCEDED,
            ENQUEUING_BUFFER_FULL
        };


        enum ReassemblingReturnValue
        {
            REASSEMBLING_NULL = -3,
            REASSEMBLING_IMPOSSIBLE,
            REASSEMBLING_ERROR,
            REASSEMBLING_COMPLETE,
            REASSEMBLING_INCOMPLETE
        };


        MutexUDPQueue (uint32 ui32MaxEnqueuableBytes = NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);

        uint32 getEnqueuedBytesCount (void) const;
        uint32 getSpaceLeftInBuffer (void) const;
        UDPDatagramPacket * findPacketFromIPHeader (const NOMADSUtil::IPHeader * const pIPHeader);

        int enqueue (UDPDatagramPacket * const pUDPDatagramPacket);
        int reassembleUDPDatagram (const NOMADSUtil::IPHeader * const pIPHeader, const NOMADSUtil::UDPHeader * const pUDPHeader);
        UDPDatagramPacket * dequeue (void);
        UDPDatagramPacket * remove (const UDPDatagramPacket * const pUDPDatagramPacket);
        void removeAll (bool removeData = false);

        int lock (void);
        int tryLock (void);
        int unlock (void);


    private:
        uint32 _ui32EnqueuedBytes;
        const uint32 _ui32MaxEnqueuableBytes;

        NOMADSUtil::Mutex _m;
    };


    inline MutexUDPQueue::MutexUDPQueue (uint32 ui32MaxEnqueuableBytes)
        : _ui32MaxEnqueuableBytes (ui32MaxEnqueuableBytes)
    {
        _ui32EnqueuedBytes = 0;
    }

    inline uint32 MutexUDPQueue::getEnqueuedBytesCount (void) const
    {
        return _ui32EnqueuedBytes;
    }

    inline uint32 MutexUDPQueue::getSpaceLeftInBuffer (void) const
    {
        if (_ui32EnqueuedBytes >= _ui32MaxEnqueuableBytes) {
            return 0;
        }

        return _ui32MaxEnqueuableBytes - _ui32EnqueuedBytes;
    }

    inline UDPDatagramPacket * MutexUDPQueue::dequeue (void)
    {
        UDPDatagramPacket * const pUDPDatagramPacket = PtrQueue<UDPDatagramPacket>::dequeue();
        if (pUDPDatagramPacket) {
            _ui32EnqueuedBytes -= pUDPDatagramPacket->getPacketLen();
        }

        return pUDPDatagramPacket;
    }

    inline UDPDatagramPacket * MutexUDPQueue::remove (const UDPDatagramPacket * const pUDPDatagramPacket)
    {
        UDPDatagramPacket * const pRemovedUDPDatagramPacket = PtrQueue<UDPDatagramPacket>::remove (pUDPDatagramPacket);
        if (pRemovedUDPDatagramPacket) {
            _ui32EnqueuedBytes -= pUDPDatagramPacket->getPacketLen();
        }

        return pRemovedUDPDatagramPacket;
    }

    inline void MutexUDPQueue::removeAll (bool removeData)
    {
        PtrQueue<UDPDatagramPacket>::removeAll (removeData);
        _ui32EnqueuedBytes = 0;
    }

    inline int MutexUDPQueue::lock (void)
    {
        return _m.lock();
    }

    inline int MutexUDPQueue::tryLock (void)
    {
        return _m.tryLock();
    }

    inline int MutexUDPQueue::unlock (void)
    {
        return _m.unlock();
    }
}

#endif   // MUTEX_UDP_QUEUE_H
