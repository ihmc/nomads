#ifndef INCL_STATISTICS_UPDATE_MANAGER_H
#define INCL_STATISTICS_UPDATE_MANAGER_H

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
 * The StatisticsUpdateManagerThread is responsible for sending messages containing
 * updated statistics information (generally, to a SENSEI NodeMonitor instance)
 */

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <utility>

#include "UDPDatagramSocket.h"
#include "ManageableThread.h"

#include "measure.pb.h"


namespace ACMNetProxy
{
    class StatisticsManager;


    // StatisticsUpdateManagerThread is responsible for sending messages containing updated statistics info
    class StatisticsUpdateManagerThread : public NOMADSUtil::ManageableThread
    {
    public:
        StatisticsUpdateManagerThread (StatisticsManager & rStatisticsManager);
        StatisticsUpdateManagerThread (const StatisticsUpdateManagerThread & rSUT) = delete;

        int init (const std::string & sNotificationAddressList);

        void run (void);
        void notify (void);


    private:
        void setupSocket (void);

        int sendMeasure (const measure::Measure * const pMeasure, char * pcBuffer, size_t stBufferSize, uint64 ui64CurrentTime);


        NOMADSUtil::UDPDatagramSocket _udpSocket;
        std::vector<std::pair<std::string, uint16>> _vpNotificationAddressList;

        StatisticsManager & _rStatisticsManager;

        mutable std::mutex _mtx;
        mutable std::condition_variable _cv;

        static const size_t PROTO_MESSAGE_BUFFER_SIZE = 1500;               // Size in bytes of a protobuf message
        static const int64 I64_SUMT_TIME_BETWEEN_ITERATIONS = 1000;         // Time between each iteration of the SUMT
    };


    inline StatisticsUpdateManagerThread::StatisticsUpdateManagerThread (StatisticsManager & rStatisticsManager) :
        _rStatisticsManager{rStatisticsManager}
    {
        setupSocket();
    }

    inline void StatisticsUpdateManagerThread::notify (void)
    {
        _cv.notify_one();
    }

    inline void StatisticsUpdateManagerThread::setupSocket (void)
    {
        _udpSocket.init();
    }
}

#endif  // INCL_STATISTICS_UPDATE_MANAGER_H
