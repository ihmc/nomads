#ifndef INCL_LOCAL_UDP_DATAGRAMS_MANAGER_H
#define INCL_LOCAL_UDP_DATAGRAMS_MANAGER_H

/*
 * LocalUDPDatagramsManager.h
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
 * The LocalUDPDatagramsManager thread is responsible for caching UDP datagrams
 * received from the local applications for sending to remote NetProxies.
 */

#include <mutex>
#include <condition_variable>

#include "ManageableThread.h"
#include "UInt32Hashtable.h"

#include "MutexUDPQueue.h"


namespace NOMADSUtil
{
    struct IPHeader;
    struct UDPHeader;
}


namespace ACMNetProxy
{
    class Connection;


    class LocalUDPDatagramsManagerThread : public NOMADSUtil::ManageableThread
    {
    public:
        LocalUDPDatagramsManagerThread (void);
        ~LocalUDPDatagramsManagerThread (void);

        void run (void);
        void notify (void) const;

        int addDatagramToOutgoingQueue (Connection * const pConnection, const CompressionSettings & compressionSettings,
                                        const Protocol protocol, const NOMADSUtil::IPHeader * const pIPHeader,
                                        const NOMADSUtil::UDPHeader * const pUDPHeader);


    private:
        int sendEnqueuedDatagramsToRemoteProxy (MutexUDPQueue * const pUDPDatagramsQueue) const;

        MutexUDPQueue _muqUDPReassembledDatagramsQueue;
        NOMADSUtil::UInt32Hashtable<MutexUDPQueue> _ui32UDPDatagramsQueueHashTable;

        std::atomic<bool> _bNotified;

        mutable std::mutex _mtx;
        mutable std::mutex _mtxUDPDatagramsQueueHashTable;
        mutable std::condition_variable _cv;

        static const int64 I64_LUDMT_TIME_BETWEEN_ITERATIONS = 500;             // Time between each iteration of the LUDMT
    };


    inline LocalUDPDatagramsManagerThread::LocalUDPDatagramsManagerThread (void) :
        _muqUDPReassembledDatagramsQueue{}, _bNotified{false} { }

    inline LocalUDPDatagramsManagerThread::~LocalUDPDatagramsManagerThread (void) { }

    inline void LocalUDPDatagramsManagerThread::notify (void) const
    {
        _cv.notify_one();
    }
}

#endif  // INCL_LOCAL_UDP_DATAGRAMS_MANAGER_H