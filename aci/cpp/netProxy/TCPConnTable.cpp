/*
 * TCPConnTable.cpp
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
 */

#include <algorithm>

#include "Logger.h"

#include "TCPConnTable.h"
#include "NetProxyConfigManager.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    Entry * const TCPConnTable::getEntry (uint16 ui16LocalID) const
    {
        if (ui16LocalID == 0) {
            return NULL;
        }

        _m.lock();
        if (static_cast<long> (ui16LocalID - 1) <= _entries.getHighestIndex()) {
            Entry *pEntry = &_entries.get (ui16LocalID - 1);
            if (pEntry && (pEntry->ui16ID == ui16LocalID)) {
                // Found the entry
                _m.unlock();
                return pEntry;
            }
        }
        _m.unlock();

        return NULL;
    }

    Entry * const TCPConnTable::getEntry (uint16 ui16LocalID, uint16 ui16RemoteID) const
    {
        if ((ui16LocalID == 0) || (ui16RemoteID == 0)) {
            return NULL;
        }

        _m.lock();
        if (static_cast<long> (ui16LocalID - 1) <= _entries.getHighestIndex()) {
            Entry *pEntry = &_entries.get (ui16LocalID - 1);
            if (pEntry && (pEntry->ui16ID == ui16LocalID) &&
                ((pEntry->ui16RemoteID == ui16RemoteID) || (pEntry->ui16RemoteID == 0))) {
                // Found the entry
                _m.unlock();
                return pEntry;
            }
        }
        _m.unlock();

        return NULL;
    }

    Entry * const TCPConnTable::getEntry (uint32 ui32LocalIP, uint16 ui16LocalPort, uint32 ui32RemoteIP, uint16 ui16RemotePort, 
		uint32 uint32AssignedPriority)
    {
        Entry *pEntry = NULL;

        _m.lock();
        // See if there is an entry for the specified parameters
        for (long i = 0; i <= _entries.getHighestIndex(); ++i) {
            pEntry = &_entries.get (i);
            if (pEntry && (pEntry->ui32LocalIP == ui32LocalIP) && (pEntry->ui16LocalPort == ui16LocalPort) &&
                (pEntry->ui32RemoteIP == ui32RemoteIP) && (pEntry->ui16RemotePort == ui16RemotePort)) {
                // Found the entry
                _m.unlock();
                return pEntry;
            }
        }

        // No entry exists for the specified parameters - return an unused one
        for (long i = 0; i <= _entries.getHighestIndex(); ++i) {
            pEntry = &_entries.get (i);
            if (!pEntry || (pEntry->localStatus == TCTLS_Unused)) {
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
                pEntry->prepareNewConnection();
                _m.unlock();
                return pEntry;
            }
        }

        // No unused entries - create a new one if we can
        const long lNewIndex = (_entries.getHighestIndex() >= 0) ? (_entries.getHighestIndex() + 1) : 0;
        if (lNewIndex > 65534) {
            // Can't use this because it would cause a problem with the ui16ID field
            checkAndLogMsg ("TCPConnTable::getEntry", Logger::L_MildError,
                            "no slots available in the TCP Connection Table\n");
            _m.unlock();
            return NULL;
        }
        pEntry = &_entries[lNewIndex];
        pEntry->ui16ID = static_cast<uint16> (lNewIndex + 1);
        pEntry->ui32LocalIP = ui32LocalIP;
        pEntry->ui16LocalPort = ui16LocalPort;
        pEntry->ui32RemoteIP = ui32RemoteIP;
        pEntry->ui16RemotePort = ui16RemotePort;
        pEntry->prepareNewConnection();

        _m.unlock();
        return pEntry;
    }

    void TCPConnTable::resetGet (void)
    {
        _m.lock();

        ++_ui32FirstIndex %= (_entries.size() > 0) ? _entries.size() : 1;
        _ui32NextIndex = _ui32FirstIndex;
        _bIsCounterReset = true;

        _m.unlock();
    }

    Entry * const TCPConnTable::getNextEntry (void)
    {
        _m.lock();

        if (_entries.getHighestIndex() < 0) {
            _m.unlock();
            return NULL;
        }

        Entry *pEntry = NULL;
        if (_bIsCounterReset) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            _bIsCounterReset = false;
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) &&
                (pEntry->localStatus != TCTLS_LISTEN)) {
                _m.unlock();
                return pEntry;
            }
        }
        while (_ui32NextIndex != _ui32FirstIndex) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) &&
                (pEntry->localStatus != TCTLS_LISTEN)) {
                _m.unlock();
                return pEntry;
            }
        }

        _m.unlock();
        return NULL;
    }

    Entry * const TCPConnTable::getNextActiveLocalEntry (void)
    {
        _m.lock();

        if (_entries.getHighestIndex() < 0) {
            _m.unlock();
            return NULL;
        }

        Entry *pEntry = NULL;
        if (_bIsCounterReset) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            _bIsCounterReset = false;
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) && (pEntry->localStatus != TCTLS_LISTEN) &&
                (pEntry->localStatus != TCTLS_TIME_WAIT) && (pEntry->localStatus != TCTLS_CLOSED)) {
                _m.unlock();
                return pEntry;
            }
        }
        while (_ui32NextIndex != _ui32FirstIndex) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) && (pEntry->localStatus != TCTLS_LISTEN) &&
                (pEntry->localStatus != TCTLS_TIME_WAIT) && (pEntry->localStatus != TCTLS_CLOSED)) {
                _m.unlock();
                return pEntry;
            }
        }

        _m.unlock();
        return NULL;
    }

    Entry * const TCPConnTable::getNextActiveRemoteEntry (void)
    {
        _m.lock();

        if (_entries.getHighestIndex() < 0) {
            _m.unlock();
            return NULL;
        }

        Entry *pEntry = NULL;
        if (_bIsCounterReset) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            _bIsCounterReset = false;
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) && (pEntry->localStatus != TCTLS_LISTEN) &&
                (pEntry->remoteStatus != TCTRS_Unknown) && (pEntry->remoteStatus != TCTRS_Disconnected)) {
                _m.unlock();
                return pEntry;
            }
        }
        while (_ui32NextIndex != _ui32FirstIndex) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) && (pEntry->localStatus != TCTLS_LISTEN) &&
                (pEntry->remoteStatus != TCTRS_Unknown) && (pEntry->remoteStatus != TCTRS_Disconnected)) {
                _m.unlock();
                return pEntry;
            }
        }

        _m.unlock();
        return NULL;
    }

    Entry * const TCPConnTable::getNextClosedLocalEntry (void)
    {
        _m.lock();

        if (_entries.getHighestIndex() < 0) {
            _m.unlock();
            return NULL;
        }

        Entry *pEntry = NULL;
        if (_bIsCounterReset) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            _bIsCounterReset = false;
            if (pEntry && ((pEntry->localStatus == TCTLS_TIME_WAIT) || (pEntry->localStatus == TCTLS_CLOSED))) {
                _m.unlock();
                return pEntry;
            }
        }
        while (_ui32NextIndex != _ui32FirstIndex) {
            pEntry = &(_entries.get (_ui32NextIndex));
            ++_ui32NextIndex %= _entries.size();
            if (pEntry && ((pEntry->localStatus == TCTLS_TIME_WAIT) || (pEntry->localStatus == TCTLS_CLOSED))) {
                _m.unlock();
                return pEntry;
            }
        }

        _m.unlock();
        return NULL;
    }

    uint16 TCPConnTable::getActiveLocalConnectionsCount (void) const
    {
        _m.lock();

        uint16 returnVal = 0;
        const Entry *pEntry;
        for (long i = 0; i <= _entries.getHighestIndex(); ++i) {
            pEntry = &_entries.get (i);
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) && (pEntry->localStatus != TCTLS_LISTEN) &&
                (pEntry->localStatus != TCTLS_TIME_WAIT) && (pEntry->localStatus != TCTLS_CLOSED)) {
                ++returnVal;
            }
        }

        _m.unlock();
        return returnVal;
    }

    uint16 TCPConnTable::getActiveLocalConnectionsCount (uint32 ui32RemoteIP, ConnectorType connectorType) const
    {
        _m.lock();

        uint16 returnVal = 0;
        const Entry *pEntry;
        for (long i = 0; i <= _entries.getHighestIndex(); ++i) {
            pEntry = &_entries.get (i);
            if (pEntry && (pEntry->localStatus != TCTLS_Unused) && (pEntry->localStatus != TCTLS_LISTEN) &&
                (pEntry->localStatus != TCTLS_TIME_WAIT) && (pEntry->localStatus != TCTLS_CLOSED) &&
                (pEntry->ui32RemoteIP == ui32RemoteIP) && pEntry->_connectors.pConnector &&
                (pEntry->_connectors.pConnector->getConnectorType() == connectorType)) {
                ++returnVal;
            }
        }

        _m.unlock();
        return returnVal;
    }

    void TCPConnTable::cleanMemory (void)
    {
        _m.lock();

        const Entry *pEntry = NULL;
        uint32 ui32ActiveEntries = 0;
        long i = NetProxyApplicationParameters::ENTRIES_POOL_SIZE + 1;
        while (i <= _entries.getHighestIndex()) {
            pEntry = &_entries.get (i);
            if (pEntry) {
                if (pEntry->tryLock() == Mutex::RC_Ok) {
                    if ((pEntry->localStatus == TCTLS_Unused) || (pEntry->localStatus == TCTLS_LISTEN) ||
                        (pEntry->localStatus == TCTLS_CLOSED)) {
                        pEntry->unlock();
                        _entries.clear (i);
                    }
                    else {
                        pEntry->unlock();
                        ++ui32ActiveEntries;
                    }
                }
                else {
                    // TryLock() failed --> Entry instance was already locked by another thread --> increase active entries counter
                    ++ui32ActiveEntries;
                }
            }
            ++i;
        }

        if ((ui32ActiveEntries == 0) && (_entries.getHighestIndex() > NetProxyApplicationParameters::ENTRIES_POOL_SIZE)) {
            _entries.trimSize (NetProxyApplicationParameters::ENTRIES_POOL_SIZE + 1);
        }

        _m.unlock();
    }

}
