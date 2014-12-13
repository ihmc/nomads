/*
 * AutoConnectionEntry.cpp
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
 */

#include "AutoConnectionEntry.h"
#include "Connector.h"
#include "ConnectionManager.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    AutoConnectionEntry::AutoConnectionEntry (const uint32 ui32RemoteProxyID, const ConnectorType connectorType, const uint32 ui32AutoReconnectTimeInMillis) :
        _ui32RemoteProxyID (ui32RemoteProxyID), _connectorType (connectorType),
        _pConnectivitySolutions (P_CONNECTION_MANAGER->findConnectivitySolutionsToProxyWithID (ui32RemoteProxyID)),
        _ui32AutoReconnectTimeInMillis (ui32AutoReconnectTimeInMillis), _ui64LastConnectionAttemptTime (0), _bSynchronized (false) { }

    const AutoConnectionEntry & AutoConnectionEntry::operator = (const AutoConnectionEntry &rhs)
    {
        _ui32RemoteProxyID = rhs._ui32RemoteProxyID;
        _connectorType = rhs._connectorType;
        _pConnectivitySolutions = P_CONNECTION_MANAGER->findConnectivitySolutionsToProxyWithID (_ui32RemoteProxyID);
        _ui32AutoReconnectTimeInMillis = rhs._ui32AutoReconnectTimeInMillis;
        _ui64LastConnectionAttemptTime = rhs._ui64LastConnectionAttemptTime;
        _bSynchronized = rhs._bSynchronized;

        return *this;
    }

    int AutoConnectionEntry::synchronize (void)
    {
        if ((_ui32RemoteProxyID == 0) || (_connectorType == CT_UNDEF)) {
            return -1;
        }

        _pConnectivitySolutions = _pConnectivitySolutions ? _pConnectivitySolutions : P_CONNECTION_MANAGER->findConnectivitySolutionsToProxyWithID (_ui32RemoteProxyID);
        if (!_pConnectivitySolutions) {
            return -2;
        }

        QueryResult query (_pConnectivitySolutions->getBestConnectionSolutionForConnectorType (_connectorType));
        Connection *pActiveConnection = query.getActiveConnectionToRemoteProxy();
        if (!pActiveConnection && !_pConnectivitySolutions->getRemoteProxyInfo().isRemoteProxyReachableFromLocalHost()) {
            // It is impossible to open a connection to the remote NetProxy from the local host
            return -3;
        }
        if (!pActiveConnection) {
            Connector * const pConnector = P_CONNECTION_MANAGER->getConnectorForType (_connectorType);
            if (!pConnector) {
                // Critical error!
                return -4;
            }

            pActiveConnection = pConnector->openNewConnectionToRemoteProxy (query, false);
            if (!pActiveConnection && pConnector->isConnectingToRemoteAddr(query.getRemoteProxyServerAddress())) {
                // Connection still establishing!
                return 0;
            }
            query = _pConnectivitySolutions->getBestConnectionSolutionForConnectorType (_connectorType);
            if (!(pActiveConnection = query.getActiveConnectionToRemoteProxy())) {
                // Connection failed
                return -5;
            }
        }

        pActiveConnection->lock();
        if (!pActiveConnection->isConnected()) {
            // Unexpected case
            pActiveConnection->unlock();
            return -6;
        }

        if (_bSynchronized) {
            // Nothing to do here
            pActiveConnection->unlock();
            return 0;
        }
        bool bReachable = _pConnectivitySolutions->getRemoteProxyInfo().isLocalProxyReachableFromRemote();
        if (0 != pActiveConnection->openConnectionWithRemoteProxy (query.getBestConnectionSolution(), bReachable)) {
            pActiveConnection->unlock();
            _bSynchronized = false;
            return -7;
        }
        pActiveConnection->unlock();
        _bSynchronized = true;

        return 0;
    }
}
