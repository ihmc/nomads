/*
 * AutoConnectionEntry.cpp
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

#include "AutoConnectionEntry.h"
#include "Connector.h"
#include "ConnectionManager.h"


using namespace NOMADSUtil;

namespace ACMNetProxy
{
    AutoConnectionEntry::AutoConnectionEntry (const uint32 ui32RemoteProxyID, const ConnectorType connectorType,
                                              const uint32 ui32AutoReconnectTimeInMillis) :
        _ui32RemoteProxyID(ui32RemoteProxyID), _connectorType(connectorType), _ucAutoConnectionEncryptionDescriptor(ET_UNDEF),
        _ui32AutoReconnectTimeInMillis(ui32AutoReconnectTimeInMillis), _ui64LastConnectionAttemptTime{},
        _bSynchronized{false}, _bValid{true} { }

    const AutoConnectionEntry & AutoConnectionEntry::operator = (const AutoConnectionEntry &rhs)
    {
        _ui32RemoteProxyID = rhs._ui32RemoteProxyID;
        _connectorType = rhs._connectorType;
        _ucAutoConnectionEncryptionDescriptor = rhs._ucAutoConnectionEncryptionDescriptor;
        _ui32AutoReconnectTimeInMillis = rhs._ui32AutoReconnectTimeInMillis;
        memcpy (_ui64LastConnectionAttemptTime, rhs._ui64LastConnectionAttemptTime, sizeof(_ui64LastConnectionAttemptTime));
        for (unsigned int i = 0; i < ET_SIZE; ++i) {
            _bSynchronized[i] = rhs._bSynchronized[i];
            _bValid[i] = rhs._bValid[i];
        }

        return *this;
    }

    void AutoConnectionEntry::updateEncryptionDescriptor (void)
    {
        auto * pConnectivitySolutions = P_CONNECTION_MANAGER->findConnectivitySolutionsToProxyWithID (_ui32RemoteProxyID);
        _ucAutoConnectionEncryptionDescriptor |= pConnectivitySolutions ?
            pConnectivitySolutions->getEncryptionDescription (_connectorType) : ET_UNDEF;
    }

    int AutoConnectionEntry::synchronize (EncryptionType encryptionType)
    {
        if ((_ui32RemoteProxyID == 0) || (_connectorType == CT_UNDEF)) {
            return -1;
        }

        auto * pConnectivitySolutions = P_CONNECTION_MANAGER->findConnectivitySolutionsToProxyWithID (_ui32RemoteProxyID);
        if (!pConnectivitySolutions) {
            return -2;
        }

        QueryResult query (pConnectivitySolutions->getBestConnectionSolutionForConnectorType (_connectorType, encryptionType));
        Connection *pActiveConnection = query.getActiveConnectionToRemoteProxy();
        if (!pActiveConnection && !pConnectivitySolutions->getRemoteProxyInfo().isRemoteProxyReachableFromLocalHost()) {
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
            if (!pActiveConnection && pConnector->isConnectingToRemoteAddr (query.getRemoteProxyServerAddress(), encryptionType)) {
                // Connection still establishing!
                return 0;
            }
            query = pConnectivitySolutions->getBestConnectionSolutionForConnectorType (_connectorType, encryptionType);
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

        if (isSynchronized (encryptionType)) {
            // Nothing to do here
            pActiveConnection->unlock();
            return 0;
        }
        bool bReachable = pConnectivitySolutions->getRemoteProxyInfo().isLocalProxyReachableFromRemote();
        if (0 != pActiveConnection->openConnectionWithRemoteProxy (query.getBestConnectionSolution(), bReachable)) {
            pActiveConnection->unlock();
            resetSynch (encryptionType);
            return -7;
        }
        pActiveConnection->unlock();
        synchronized (encryptionType);

        return 0;
    }

    const NOMADSUtil::InetAddr * const AutoConnectionEntry::getRemoteProxyInetAddress (EncryptionType encryptionType) const
    {
        auto * pConnectivitySolutions = P_CONNECTION_MANAGER->findConnectivitySolutionsToProxyWithID (_ui32RemoteProxyID);
        return pConnectivitySolutions ?
            pConnectivitySolutions->getBestConnectionSolutionForConnectorType (_connectorType,
                                                                               encryptionType).getBestConnectionSolution() :
            nullptr;
    }
}
