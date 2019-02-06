/*
 * LocalTCPTransmitter.cpp
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

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <utility>

#include "StatisticsUpdateManager.h"
#include "StatisticsManager.h"
#include "Utilities.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{

    int StatisticsUpdateManagerThread::init (const std::string & sNotificationAddressList)
    {
        auto vsAddressList = splitStringToVector (sNotificationAddressList, ',');

        // Parse notification address list
        for (auto sIPv4PortPair : vsAddressList) {
            auto vsIPv4PortPair = splitStringToVector (sIPv4PortPair, ':');
            if (vsIPv4PortPair.size() != 2) {
                checkAndLogMsg ("StatisticsUpdateManagerThread::init", NOMADSUtil::Logger::L_Warning,
                                "error parsing the entry <%s> from the list of notification addresses <%s>\n",
                                sIPv4PortPair.c_str(), sNotificationAddressList.c_str());
            }
            std::string sIP{vsIPv4PortPair[0]};
            std::string sPort{vsIPv4PortPair[1]};

            _vpNotificationAddressList.push_back (std::make_pair (sIP, static_cast<uint16> (atoi (sPort.c_str()))));
        }

        return 0;
    }

    void StatisticsUpdateManagerThread::run (void)
    {
        started();

        int rc;
        char pucBuf[PROTO_MESSAGE_BUFFER_SIZE];

        std::unique_lock<std::mutex> ul{_mtx};
        while (!terminationRequested()) {
            const auto i64NextCycleTime = NOMADSUtil::getTimeInMilliseconds() + I64_SUMT_TIME_BETWEEN_ITERATIONS;
            _cv.wait_for (ul, std::chrono::milliseconds{I64_SUMT_TIME_BETWEEN_ITERATIONS}, [this, i64NextCycleTime]
                          {
                            return (NOMADSUtil::getTimeInMilliseconds() >= i64NextCycleTime) || terminationRequested();
                          });
            if (terminationRequested()) {
                break;
            }

            const auto i64CurrentTime = NOMADSUtil::getTimeInMilliseconds();
            // Process measures
            while (const auto upMeasure = _rStatisticsManager.getNextProcessMeasureToSend (i64CurrentTime)) {
                if ((rc = sendMeasure (upMeasure.get(), pucBuf, PROTO_MESSAGE_BUFFER_SIZE, i64CurrentTime)) < 0) {
                    checkAndLogMsg ("StatisticsUpdateManagerThread::run", NOMADSUtil::Logger::L_Warning,
                                    "sendMeasure() of a Process measure failed with rc = %d\n", rc);
                }
            }

            // Configuration measures
            while (const auto upMeasure = _rStatisticsManager.getNextConfigurationMeasureToSend (i64CurrentTime)) {
                if ((rc = sendMeasure (upMeasure.get(), pucBuf, PROTO_MESSAGE_BUFFER_SIZE, i64CurrentTime)) < 0) {
                    checkAndLogMsg ("StatisticsUpdateManagerThread::run", NOMADSUtil::Logger::L_Warning,
                                    "sendMeasure() of a Configuration measure failed with rc = %d\n", rc);
                }
            }

            // Topology measures
            while (const auto upMeasure = _rStatisticsManager.getNextTopologyMeasureToSend (i64CurrentTime)) {
                if ((rc = sendMeasure (upMeasure.get(), pucBuf, PROTO_MESSAGE_BUFFER_SIZE, i64CurrentTime)) < 0) {
                    checkAndLogMsg ("StatisticsUpdateManagerThread::run", NOMADSUtil::Logger::L_Warning,
                                    "sendMeasure() of a Topology measure failed with rc = %d\n", rc);
                }
            }

            // Link Description measures
            while (const auto upMeasure = _rStatisticsManager.getNextLinkDescriptionMeasureToSend (i64CurrentTime)) {
                if ((rc = sendMeasure (upMeasure.get(), pucBuf, PROTO_MESSAGE_BUFFER_SIZE, i64CurrentTime)) < 0) {
                    checkAndLogMsg ("StatisticsUpdateManagerThread::run", NOMADSUtil::Logger::L_Warning,
                                    "sendMeasure() of a Link Description measure failed with rc = %d\n", rc);
                }
            }

            // Link Traffic measures
            while (const auto upMeasure = _rStatisticsManager.getNextLinkTrafficMeasureToSend (i64CurrentTime)) {
                if ((rc = sendMeasure (upMeasure.get(), pucBuf, PROTO_MESSAGE_BUFFER_SIZE, i64CurrentTime)) < 0) {
                    checkAndLogMsg ("StatisticsUpdateManagerThread::run", NOMADSUtil::Logger::L_Warning,
                                    "sendMeasure() of a Link Traffic measure failed with rc = %d\n", rc);
                }
            }
        }

        terminating();
    }

    int StatisticsUpdateManagerThread::sendMeasure (const measure::Measure * const pMeasure, char * pcBuffer, size_t stBufferSize, uint64 ui64CurrentTime)
    {
        int rc;

        size_t st_measureSize = pMeasure->ByteSizeLong();
        if (st_measureSize > stBufferSize) {
            checkAndLogMsg ("StatisticsUpdateManagerThread::sendMeasure", NOMADSUtil::Logger::L_Warning,
                            "could not serialize protobuf message: size too big. Available "
                            "buffer size is %u bytes, required buffer size is %u bytes\n",
                            stBufferSize, st_measureSize);
            return -1;
        }
        pMeasure->SerializeToArray (pcBuffer, st_measureSize);

        // Send to all listeners
        uint16 ui16Port = 0;
        const char *pszIPAddress = nullptr;
        for (auto ipPortPair : _vpNotificationAddressList) {
            pszIPAddress = ipPortPair.first.c_str();
            ui16Port = ipPortPair.second;
            if (st_measureSize != (rc = _udpSocket.sendTo (pszIPAddress, ui16Port, pcBuffer, st_measureSize))) {
                checkAndLogMsg ("StatisticsUpdateManagerThread::sendMeasure", NOMADSUtil::Logger::L_Warning,
                                "sendTo() failed to send a protobuf measure of %hu bytes to address <%s:%hu>; rc = %d\n",
                                st_measureSize, pszIPAddress, ui16Port, rc);
            }
            else {
                checkAndLogMsg ("StatisticsUpdateManagerThread::sendMeasure", NOMADSUtil::Logger::L_HighDetailDebug,
                                "successfully sent a protobuf measure of %hu bytes to %s:%hu\n",
                                st_measureSize, pszIPAddress, ui16Port);
            }
        }

        return 0;
    }

    const int64 StatisticsUpdateManagerThread::I64_SUMT_TIME_BETWEEN_ITERATIONS;
}