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

#include <chrono>
#include <mutex>
#include <condition_variable>

#include "Logger.h"

#include "MemoryCleanerManager.h"
#include "ARPTableMissCache.h"
#include "Entry.h"
#include "Connection.h"
#include "TCPConnTable.h"
#include "TCPManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    void MemoryCleanerManagerThread::run (void)
    {
        started();

        int rc;
        int64 i64RefTime = NOMADSUtil::getTimeInMilliseconds();
        std::unique_lock<std::mutex> ul{_mtx};
        while (!terminationRequested()) {
            const auto nextCycleTimeInMS = std::chrono::system_clock::now() + std::chrono::milliseconds{I64_MCMT_TIME_BETWEEN_ITERATIONS};
            _cv.wait_until (ul, nextCycleTimeInMS, [this, nextCycleTimeInMS]
                          {
                            return (std::chrono::system_clock::now() >= nextCycleTimeInMS) || terminationRequested();
                          });
            if (terminationRequested()) {
                break;
            }

            // Clear ARPTableMissCache
            _rARPTableMissCache.clearTableFromExpiredEntries();

            // Clear TCPConnTable entries
            bool bWorkDone = false;
            Entry * pEntry;
            std::lock_guard<std::mutex> lgTCPConnTable{_rTCPConnTable.getMutexRef()};
            _rTCPConnTable.resetGet();
            const auto i64CurrTime = NOMADSUtil::getTimeInMilliseconds();
            while ((pEntry = _rTCPConnTable.getNextClosedEntry()) != nullptr) {
                std::unique_lock<std::mutex> ul{pEntry->getMutexRef(), std::try_to_lock};
                if (ul.owns_lock()) {
                    // Check the entry to see if it needs to be cleaned up
                    if ((pEntry->localState == TCTLS_TIME_WAIT) &&
                        ((i64CurrTime - pEntry->i64LocalActionTime) >= Entry::STANDARD_MSL) &&
                        !pEntry->areThereUntransmittedPacketsInUDPTransmissionQueue()) {
                        checkAndLogMsg ("MemoryCleanerManagerThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: cleaning communication entry in statue TIME_WAIT\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        pEntry->clear();
                        bWorkDone = true;
                    }
                    else if ((pEntry->localState == TCTLS_CLOSED) &&
                        ((i64CurrTime - pEntry->i64LocalActionTime) >= I64_MCMT_TIME_BETWEEN_ITERATIONS) &&
                        !pEntry->areThereUntransmittedPacketsInUDPTransmissionQueue()) {
                        checkAndLogMsg ("MemoryCleanerManagerThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: cleaning communication entry in state CLOSED\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        pEntry->clear();
                        bWorkDone = true;
                    }
                }
            }

            if (bWorkDone) {
                i64RefTime = NOMADSUtil::getTimeInMilliseconds();
            }
            else if ((NOMADSUtil::getTimeInMilliseconds() - i64RefTime) >= I64_MCMT_MEMORY_CLEANING_INTERVAL) {
                // No cleaning done for I64_MCMT_MEMORY_CLEANING_INTERVAL ms --> check if it is possible to shrink the TCPConnTable
                const auto ui32EntriesNum = _rTCPConnTable.getEntriesNum();
                _rTCPConnTable.removeUnusedEntries();
                if (ui32EntriesNum != _rTCPConnTable.getEntriesNum()) {
                    checkAndLogMsg ("MemoryCleanerManagerThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                    "TCP connections table shrinked from %u to %u entries\n",
                                    ui32EntriesNum, _rTCPConnTable.getEntriesNum());
                }
                i64RefTime = NOMADSUtil::getTimeInMilliseconds();
            }
        }

        terminating();
    }

    const int64 MemoryCleanerManagerThread::I64_MCMT_TIME_BETWEEN_ITERATIONS;
    const int64 MemoryCleanerManagerThread::I64_MCMT_MEMORY_CLEANING_INTERVAL;
}