#ifndef INCL_MEMORY_CLEANER_MANAGER_H
#define INCL_MEMORY_CLEANER_MANAGER_H

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
 * The MemoryCleanerManagerThread is responsible for cleaning up memory by
 * deleting unused and expired entries in the TCPConnTable and ARPTableMissCache
 */

#include <mutex>
#include <condition_variable>

#include "ManageableThread.h"


namespace ACMNetProxy
{
    class ARPTableMissCache;
    class TCPConnTable;
    class TCPManager;


    class MemoryCleanerManagerThread : public NOMADSUtil::ManageableThread
    {
    public:
        MemoryCleanerManagerThread (ARPTableMissCache & rARPTableMissCache, TCPConnTable & rTCPConnTable,
                                    TCPManager & rTCPManager);

        void run (void);
        void notify (void);


    private:
        ARPTableMissCache & _rARPTableMissCache;
        TCPConnTable & _rTCPConnTable;
        TCPManager & _rTCPManager;

        mutable std::mutex _mtx;
        mutable std::condition_variable _cv;

        static const int64 I64_MCMT_TIME_BETWEEN_ITERATIONS = 5000;             // Time between each iteration of the MCMT
        static const int64 I64_MCMT_MEMORY_CLEANING_INTERVAL = 10000U;          // Inactivity time before performing a TCPConnTable cleanup
    };


    inline MemoryCleanerManagerThread::MemoryCleanerManagerThread (ARPTableMissCache & rARPTableMissCache, TCPConnTable & rTCPConnTable,
                                                                   TCPManager & rTCPManager) :
        _rARPTableMissCache{rARPTableMissCache}, _rTCPConnTable{rTCPConnTable}, _rTCPManager{rTCPManager}
    { }

    inline void MemoryCleanerManagerThread::notify (void)
    {
        _cv.notify_one();
    }
}

#endif  // INCL_MEMORY_CLEANER_MANAGER_H
