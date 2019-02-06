#ifndef INCL_AUTO_CONNECTIONS_MANAGER_H
#define INCL_AUTO_CONNECTIONS_MANAGER_H

/*
* AutoConnectionsManager.h
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
* This class handles all automatic connections to remote NetProxy instances.
*/

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>

#include "Thread.h"

#include "AutoConnectionEntry.h"

// AutoConnectionManager thread is responsible for establishing and verifying status of any configured autoconnection
namespace ACMNetProxy
{
    class ConnectionManager;
    class TCPConnTable;
    class TCPManager;
    class PacketRouter;
    class StatisticsManager;


    class AutoConnectionManager : public NOMADSUtil::Thread
    {
    public:
        AutoConnectionManager (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                               PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);

        void run (void);

        bool isRunning (void) const;
        bool isTerminationRequested (void) const;
        void requestTermination (void);

        void lock (void) const;
        void unlock (void) const;
        void notify (void) const;


    private:
        std::atomic<bool> _bRunning;
        std::atomic<bool> _bTerminationRequested;

        ConnectionManager & _rConnectionManager;
        TCPConnTable & _rTCPConnTable;
        TCPManager & _rTCPManager;
        PacketRouter & _rPacketRouter;
        StatisticsManager & _rStatisticsManager;

        mutable std::mutex _mtx;
        mutable std::condition_variable _cv;

        static constexpr const int64 ACM_TIME_BETWEEN_ITERATIONS = 10000;           // Time between each iterations for ACM
        static constexpr const int64 ACM_SHORT_TIME_BETWEEN_ITERATIONS = 2000;      // Reduced wait time when checking for a connection being established
    };


    inline AutoConnectionManager::AutoConnectionManager (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                                                         PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        _bRunning{false}, _bTerminationRequested{false}, _rConnectionManager{rConnectionManager}, _rTCPConnTable{rTCPConnTable},
        _rTCPManager{rTCPManager}, _rPacketRouter{rPacketRouter}, _rStatisticsManager{rStatisticsManager}
    { }

    inline bool AutoConnectionManager::isRunning (void) const
    {
        return _bRunning;
    }

    inline bool AutoConnectionManager::isTerminationRequested (void) const
    {
        return _bTerminationRequested;
    }

    inline void AutoConnectionManager::requestTermination (void)
    {
        _bTerminationRequested = true;
    }

    inline void AutoConnectionManager::lock (void) const
    {
        _mtx.lock();
    }

    inline void AutoConnectionManager::unlock (void) const
    {
        _mtx.unlock();
    }

    inline void AutoConnectionManager::notify (void) const
    {
        _cv.notify_one();
    }
}

#endif      // INCL_AUTO_CONNECTIONS_MANAGER_H