/*
 * TCPConnTable.cpp
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

#include <algorithm>

#include "Logger.h"

#include "TCPConnTable.h"
#include "ConfigurationManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    Entry * const TCPConnTable::getEntry (uint16 ui16LocalID) const
    {
        if (ui16LocalID == 0) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lg{_mtx};
        if (static_cast<long> (ui16LocalID - 1) <= _entries.getHighestIndex()) {
            Entry *pEntry = &_entries.get (ui16LocalID - 1);
            if (pEntry && (pEntry->ui16ID == ui16LocalID)) {
                // Found the entry
                return pEntry;
            }
        }

        return nullptr;
    }

    Entry * const TCPConnTable::getEntry (uint16 ui16LocalID, uint16 ui16RemoteID) const
    {
        if ((ui16LocalID == 0) || (ui16RemoteID == 0)) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lg{_mtx};
        if (static_cast<long> (ui16LocalID - 1) <= _entries.getHighestIndex()) {
            Entry *pEntry = &_entries.get (ui16LocalID - 1);
            if (pEntry && (pEntry->ui16ID == ui16LocalID) &&
                ((pEntry->ui16RemoteID == ui16RemoteID) || (pEntry->ui16RemoteID == 0))) {
                // Found the entry
                return pEntry;
            }
        }

        return nullptr;
    }

    Entry * const TCPConnTable::getEntry (uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP,
                                          uint16 ui16RemotePort, uint32 uint32AssignedPriority)
    {
        Entry * pEntry = nullptr;

        std::lock_guard<std::mutex> lg{_mtx};
        // See if there is an entry for the specified parameters
        for (long i = 0; i <= _entries.getHighestIndex(); ++i) {
            pEntry = &_entries.get (i);
            if (pEntry && (pEntry->ui32LocalIP == ui32LocalIP) && (pEntry->ui16LocalPort == ui16LocalPort) &&
                (pEntry->ui32RemoteIP == ui32RemoteIP) && (pEntry->ui16RemotePort == ui16RemotePort)) {
                // Found the entry
                return pEntry;
            }
        }

        // No entry exists for the specified parameters - return an unused one
        for (long i = 0; i <= _entries.getHighestIndex(); ++i) {
            pEntry = &_entries.get (i);
            if (!pEntry || (pEntry->localState == TCTLS_LISTEN)) {
                if (!pEntry) {
                    // Allocate new Entry
                    pEntry = &_entries[i];
                }
                else {
                    // Clear the Entry
                    pEntry->clear();
                }
                pEntry->ui16ID = static_cast<uint16> (i + 1);
                pEntry->ui32LocalIP = ui32LocalIP;
                pEntry->ui16LocalPort = ui16LocalPort;
                pEntry->ui32RemoteIP = ui32RemoteIP;
                pEntry->ui16RemotePort = ui16RemotePort;
                pEntry->assignedPriority = uint32AssignedPriority;
                return pEntry;
            }
        }

        // No unused entries - create a new one if we can
        const long lNewIndex = (_entries.getHighestIndex() >= 0) ? (_entries.getHighestIndex() + 1) : 0;
        if (lNewIndex > 65534) {
            // Can't use this because it would cause a problem with the ui16ID field
            checkAndLogMsg ("TCPConnTable::getEntry", NOMADSUtil::Logger::L_MildError,
                            "no slots available in the TCP Connection Table\n");
            return nullptr;
        }
        pEntry = &_entries[lNewIndex];
        pEntry->ui16ID = static_cast<uint16> (lNewIndex + 1);
        pEntry->ui32LocalIP = ui32LocalIP;
        pEntry->ui16LocalPort = ui16LocalPort;
        pEntry->ui32RemoteIP = ui32RemoteIP;
        pEntry->ui16RemotePort = ui16RemotePort;
        pEntry->assignedPriority = uint32AssignedPriority;

        return pEntry;
    }

    void TCPConnTable::resetGet (void)
    {
        ++_ui32FirstIndex %= (_entries.size() > 0) ? _entries.size() : 1;
        _ui32NextIndex = _ui32FirstIndex;
        _bIsCounterReset = true;
    }

    Entry * const TCPConnTable::getNextEntry (void)
    {
        if (_entries.getHighestIndex() < 0) {
            return nullptr;
        }

        Entry * pEntry = nullptr;
        if (_bIsCounterReset) {
            _bIsCounterReset = false;
            pEntry = &(_entries.get (_ui32NextIndex++));
            _ui32NextIndex %= _entries.size();
            if (pEntry && (pEntry->localState != TCTLS_LISTEN)) {
                return pEntry;
            }
        }
        while (_ui32NextIndex != _ui32FirstIndex) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            if (pEntry && (pEntry->localState != TCTLS_LISTEN)) {
                return pEntry;
            }
        }

        return nullptr;
    }

    Entry * const TCPConnTable::getNextActiveLocalEntry (void)
    {
        Entry * pEntry;
        while ((pEntry = getNextEntry()) != nullptr) {
            if ((pEntry->localState != TCTLS_TIME_WAIT) && (pEntry->localState != TCTLS_CLOSED)) {
                return pEntry;
            }
        }

        return nullptr;
    }

    Entry * const TCPConnTable::getNextActiveRemoteEntry (void)
    {
        Entry * pEntry;
        while ((pEntry = getNextEntry()) != nullptr) {
            if ((pEntry->remoteState != TCTRS_Unknown) && (pEntry->remoteState != TCTRS_DisconnRequestSent) &&
                (pEntry->remoteState != TCTRS_Disconnected)) {
                return pEntry;
            }
        }

        return nullptr;
    }

    Entry * const TCPConnTable::getNextClosedEntry (void)
    {
        Entry * pEntry;
        while ((pEntry = getNextEntry()) != nullptr) {
            if ((pEntry->remoteState == TCTRS_Disconnected) &&
                ((pEntry->localState == TCTLS_TIME_WAIT) || (pEntry->localState == TCTLS_CLOSED))) {
                return pEntry;
            }
        }

        return nullptr;
    }

    uint16 TCPConnTable::getActiveLocalConnectionsCount (void) const
    {
        uint16 returnVal = 0;
        const Entry * pEntry;
        for (long i = 0; i <= _entries.getHighestIndex(); ++i) {
            pEntry = &_entries.get (i);
            if (pEntry && (pEntry->localState != TCTLS_LISTEN) &&
                (pEntry->localState != TCTLS_TIME_WAIT) && (pEntry->localState != TCTLS_CLOSED)) {
                ++returnVal;
            }
        }

        return returnVal;
    }

    void TCPConnTable::removeUnusedEntries (void)
    {
        const Entry * pEntry = nullptr;
        long i = NetProxyApplicationParameters::TCP_CONN_TABLE_ENTRIES_POOL_SIZE + 1,
            lastActiveEntry = NetProxyApplicationParameters::TCP_CONN_TABLE_ENTRIES_POOL_SIZE;
        while (i <= _entries.getHighestIndex()) {
            const auto * const pEntry = &_entries.get (i);
            if (pEntry) {
                std::unique_lock<std::mutex> ul{pEntry->getMutexRef(), std::try_to_lock};
                if (ul.owns_lock()) {
                    if ((pEntry->localState == TCTLS_LISTEN) && (pEntry->remoteState == TCTRS_Unknown)) {
                        ul.unlock();
                        _entries.clear (i);
                    }
                    else {
                        lastActiveEntry = i;
                    }
                }
                else {
                    // TryLock() failed --> Entry instance was already locked by another thread (assume it is in an active state)
                    lastActiveEntry = i;
                }
            }
            ++i;
        }

        if (lastActiveEntry < _entries.getHighestIndex()) {
            _entries.trimSize (lastActiveEntry + 1);
        }
    }

}
