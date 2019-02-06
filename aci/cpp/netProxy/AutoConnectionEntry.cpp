/*
 * AutoConnectionEntry.cpp
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

#include "Logger.h"

#include "AutoConnectionEntry.h"
#include "Connection.h"
#include "ConnectionManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    int AutoConnectionEntry::synchronize (EncryptionType encryptionType, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                                          TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager)
    {
        if ((_ui32RemoteProxyID == 0) || (_connectorType == CT_UNDEF)) {
            return -1;
        }

        auto * const pConnectivitySolutions =
            rConnectionManager.findConnectivitySolutionsToNetProxyWithIDAndIPv4Address (_ui32RemoteProxyID, _iaLocalInterfaceIPv4Address.getIPAddress(),
                                                                                        _iaRemoteInterfaceIPv4Address.getIPAddress());
        if (!pConnectivitySolutions) {
            return -2;
        }

        auto query{pConnectivitySolutions->getBestConnectionSolutionForConnectorAndEncryptionType (_connectorType, encryptionType)};
        auto * pActiveConnection = query.getActiveConnectionToRemoteProxy();
        if (!pActiveConnection && !pConnectivitySolutions->getRemoteProxyInfo().isRemoteProxyReachableFromLocalHost()) {
            // It is impossible to open a connection to the remote NetProxy from the local host
            setInvalid (encryptionType);
            return -3;
        }

        // Check if the Connection instance exists; if it is nullptr, open a new connection
        pActiveConnection = pActiveConnection ?
            pActiveConnection : Connection::openNewConnectionToRemoteProxy (rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter,
                                                                            rStatisticsManager, query, false);
        // Check if a new Connection instance was returned; if it is nullptr, retrieve any available connection, regardless of the status
        pActiveConnection = pActiveConnection ?
            pActiveConnection : Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                                    query.getRemoteProxyServerAddress(), _connectorType, encryptionType);
        if (!pActiveConnection) {
            // Connection still nullptr --> return an error!
            return -4;
        }
        if (pActiveConnection->isConnecting()) {
            // Connection still being established
            return 0;
        }
        if (!pActiveConnection->isConnected()) {
            // Unexpected case
            resetSynchronization (encryptionType);
            return -5;
        }

        // If we get here, connection was established successfully --> synchronize for this specific EncryptionType
        setAsSynchronized (encryptionType);
        return 0;
    }

    void AutoConnectionEntry::updateEncryptionDescriptor (ConnectionManager & rConnectionManager)
    {
        auto * const pConnectivitySolutions =
            rConnectionManager.findConnectivitySolutionsToNetProxyWithIDAndIPv4Address (_ui32RemoteProxyID, _iaLocalInterfaceIPv4Address.getIPAddress(),
                                                                                        _iaRemoteInterfaceIPv4Address.getIPAddress());
        _ucAutoConnectionEncryptionDescriptor |= pConnectivitySolutions ?
            pConnectivitySolutions->getEncryptionDescription (_connectorType) : ET_UNDEF;
    }

    bool AutoConnectionEntry::areConnectivitySolutionsAvailableWithEncryption (EncryptionType encryptionType, ConnectionManager & rConnectionManager) const
    {
        auto * const pConnectivitySolutions =
            rConnectionManager.findConnectivitySolutionsToNetProxyWithIDAndIPv4Address (_ui32RemoteProxyID, _iaLocalInterfaceIPv4Address.getIPAddress(),
                                                                                        _iaRemoteInterfaceIPv4Address.getIPAddress());
        return pConnectivitySolutions != nullptr;
    }

    void AutoConnectionEntry::resetAutoConnectionEntry (Connection * const pConnectionToReset)
    {
        std::lock_guard<std::mutex> lg{_mtx};
        if ((getConnectorType() == pConnectionToReset->getConnectorType()) &&
            isEncryptionTypeInDescriptor (getConnectionEncryptionDescriptor(), pConnectionToReset->getEncryptionType())) {
            resetSynchronization (pConnectionToReset->getEncryptionType());
            checkAndLogMsg ("AutoConnectionEntry::resetAutoConnectionEntry", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "successfully reset synchronization of the AutoConnectionEntry instance "
                            "for the remote NetProxy with UniqueID %u and address <%s:%hu>\n",
                            pConnectionToReset->getRemoteNetProxyID(),
                            pConnectionToReset->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            pConnectionToReset->getRemoteInterfaceLocalInetAddr()->getPort());
        }
        else {
            checkAndLogMsg ("AutoConnectionEntry::resetAutoConnectionEntry", NOMADSUtil::Logger::L_LowDetailDebug,
                            "the AutoConnectionEntry instance found for the remote NetProxy with UniqueID %u "
                            "and address <%s:%hu> did not match the ConnectorType and/or the EncryptionType\n",
                            pConnectionToReset->getRemoteNetProxyID(),
                            pConnectionToReset->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            pConnectionToReset->getRemoteInterfaceLocalInetAddr()->getPort());
        }
    }
}
