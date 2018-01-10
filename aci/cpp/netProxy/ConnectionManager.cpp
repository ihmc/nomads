/*
 * ConnectionManager.cpp
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

#include "Logger.h"

#include "ConnectionManager.h"
#include "RemoteProxyInfo.h"
#include "AutoConnectionEntry.h"
#include "Connection.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    void ConnectionManager::clearAllConnectionMappings (void)
    {
        std::lock_guard<std::mutex> lg (_m);

        _connectorsTable.trimSize (0);
        _remoteHostAddressMappingCache.removeAll();
        _remoteHostAddressMappingBook.trimSize (0);
        _remoteProxyConnectivityTable.removeAll();
    }

    int ConnectionManager::addNewRemoteProxyInfo (const RemoteProxyInfo &remoteProxyInfo)
    {
        int res = 0;
        std::lock_guard<std::mutex> lg (_m);

        ConnectivitySolutions *pConnectivitySolutions = _remoteProxyConnectivityTable.get (remoteProxyInfo.getRemoteProxyID());
        if (pConnectivitySolutions) {
            // Update relative information
            pConnectivitySolutions->_remoteProxyInfo = remoteProxyInfo;
        }
        else {
            // add new RemoteProxyInfo
            pConnectivitySolutions = new ConnectivitySolutions (remoteProxyInfo);
            _remoteProxyConnectivityTable.put (pConnectivitySolutions->getRemoteProxyID(), pConnectivitySolutions);
            res = 1;
        }

        return res;
    }

    /* This method assumes that the remote NetProxy UniqueID matches the one in the mapping address table.
     * This method returns:
     *      0 - in case no changes are made;
     *      1 - if an old entry was substituted with the new one;
     *      2 - if the new entry was added to the book;
     *      a negative value - in case of error.
     */
    int ConnectionManager::updateAddressMappingBook (const AddressRangeDescriptor & addressRange, uint32 ui32RemoteProxyID)
    {
        if (!addressRange.isValid() || !ui32RemoteProxyID) {
            return -1;
        }

        if (!addressRange.isAnIPRange()) {
            const std::pair<AddressRangeDescriptor, ConnectivitySolutions *> * const pPair = _remoteHostAddressMappingCache.get (addressRange.getLowestAddress());
            if (pPair) {
                return 0;
            }
        }

        ConnectivitySolutions * const pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (!pConnectivitySolutions) {
            checkAndLogMsg ("ConnectionManager::updateAddressMappingBook", Logger::L_Warning,
                            "remote NetProxy with UniqueID %u could not be found in the Remote NetProxy Connectivity Table\n",
                            ui32RemoteProxyID);
            return -2;
        }

        int res = -3;
        std::lock_guard<std::mutex> lg (_m);

        // Add the new address mapping entry in the connection Info Table (removing the old entry, if any)
        std::pair<AddressRangeDescriptor, ConnectivitySolutions *> *pPair = nullptr;
        for (int i = 0; (i <= _remoteHostAddressMappingBook.getHighestIndex()) && (res < 0); ++i) {
            pPair = &(_remoteHostAddressMappingBook.get (i));
            if (addressRange.isSubsetOf (pPair->first)) {
                // Check if it is necessary to cache the IP address
                if (!addressRange.isAnIPRange()) {
                    checkAndLogMsg ("ConnectionManager::updateAddressMappingBook", Logger::L_HighDetailDebug,
                                    "adding entry <%s - %u> in the Address Mapping Cache; remote NetProxy UniqueID is %u\n",
                                    addressRange.operator const char *const(), pPair->second->getRemoteProxyID(), ui32RemoteProxyID);
                    _remoteHostAddressMappingCache.put (addressRange.getLowestAddress(), pPair);
                }
                return 0;
            }
            else if (pPair->first.isSubsetOf (addressRange)) {
                checkAndLogMsg ("ConnectionManager::updateAddressMappingBook", Logger::L_Info,
                                "remote IP address %s found to be a superset of the address range %s in the Address Mapping Book - "
                                "substituting the old address range with the new one; remote NetProxy UniqueID is %u and the mapped "
                                "remote NetProxy has UniqueID %u\n", addressRange.operator const char *const(),
                                pPair->first.operator const char *const(), ui32RemoteProxyID, pPair->second->getRemoteProxyID());
                _remoteHostAddressMappingBook[i] = std::make_pair (addressRange, pConnectivitySolutions);
                pPair = &_remoteHostAddressMappingBook.get (i);
                res = 1;
            }
            else if (pPair->first.overlaps (addressRange)) {
                // For now, do the same as above
                checkAndLogMsg ("ConnectionManager::updateAddressMappingBook", Logger::L_Warning,
                                "remote IP address %s found to overlap the address range %s in the Address Mapping Book - substituting "
                                "the old address range with the new one; remote NetProxy UniqueID is %u and the mapped remote NetProxy "
                                "has UniqueID %u; WARNING: this might cause some problems!\n", addressRange.operator const char *const(),
                                pPair->first.operator const char *const(), ui32RemoteProxyID, pPair->second->getRemoteProxyID());
                _remoteHostAddressMappingBook[i] = std::make_pair (addressRange, pConnectivitySolutions);
                pPair = &_remoteHostAddressMappingBook.get (i);
                res = 1;
            }
        }

        if (res < 0) {
            // If we reach this point, we need to add a new mapping in the book
            checkAndLogMsg ("ConnectionManager::updateAddressMappingBook", Logger::L_Info,
                            "no match was found for the remote IP address %s in the Address Mapping Book - adding it as a new "
                            "entry in the Address Mapping Book; the mapped remote NetProxy has UniqueID %u\n",
                            addressRange.operator const char *const(), ui32RemoteProxyID);
            if (!addressRange.isAnIPRange() && !addressRange.isAPortRange()) {
                // Do not add a new mapping only for a specific port
                const AddressRangeDescriptor addressRangeDescriptor = AddressRangeDescriptor (EndianHelper::htonl (addressRange.getLowestAddress()));
                checkAndLogMsg ("ConnectionManager::updateAddressMappingBook", Logger::L_HighDetailDebug,
                                "adding new entry in the Address Mapping Book for the remote IP address %s\n",
                                addressRangeDescriptor.operator const char *const(), ui32RemoteProxyID);
                _remoteHostAddressMappingBook[_remoteHostAddressMappingBook.getHighestIndex() + 1] = std::make_pair (addressRangeDescriptor, pConnectivitySolutions);
            }
            else {
                _remoteHostAddressMappingBook[_remoteHostAddressMappingBook.getHighestIndex() + 1] = std::make_pair (addressRange, pConnectivitySolutions);
            }
            pPair = &_remoteHostAddressMappingBook.get (_remoteHostAddressMappingBook.getHighestIndex());
            res = 2;
        }
        if (!addressRange.isAnIPRange() && pPair) {
            checkAndLogMsg ("ConnectionManager::updateAddressMappingBook", Logger::L_HighDetailDebug,
                            "adding new entry <%s - %u> in the Address Mapping Cache; remote NetProxy UniqueID is %u\n",
                            addressRange.operator const char *const(), pPair->second->getRemoteProxyID(), ui32RemoteProxyID);
            _remoteHostAddressMappingCache.put (addressRange.getLowestAddress(), pPair);
        }

        return res;
    }

    int ConnectionManager::updateRemoteProxyUniqueID (uint32 ui32RemoteProxyOldID, uint32 ui32RemoteProxyNewID)
    {
        if ((ui32RemoteProxyOldID == ui32RemoteProxyNewID) || (ui32RemoteProxyOldID == 0)) {
            // Nothing to do
            return 0;
        }

        std::lock_guard<std::mutex> lg (_m);
        // It is necessary to update the remote NetProxy UniqueID and the key of the corresponding entry in the Remote Proxy Connectivity Table
        ConnectivitySolutions * const pConnectivitySolutions = _remoteProxyConnectivityTable.remove (ui32RemoteProxyOldID);
        if (pConnectivitySolutions) {
            checkAndLogMsg ("ConnectionManager::updateRemoteProxyUniqueID", Logger::L_Info,
                            "The UniqueID notified from the remote NetProxy with address %s differs from the one previously "
                            "set for that NetProxy (old ID: %u - new ID: %u) - updating the remote NetProxy UniqueID\n",
                            pConnectivitySolutions->getRemoteProxyInfo().getRemoteProxyIPAddressAsString(),
                            ui32RemoteProxyOldID, ui32RemoteProxyNewID);
            if (ConnectivitySolutions * const pConnectivityToNewID = _remoteProxyConnectivityTable.get (ui32RemoteProxyNewID)) {
                /* There is already an entry in the Remote Proxy Connectivity Table for the new UniqueID --> update all entries
                    * in the Address Mapping Book to point to the new ConnectivitySolutions object and then delete the old one */
                for (int i = 0; i <= _remoteHostAddressMappingBook.getHighestIndex(); ++i) {
                    if (_remoteHostAddressMappingBook.get (i).second == pConnectivitySolutions) {
                        _remoteHostAddressMappingBook.get (i).second = pConnectivityToNewID;
                    }
                }
                delete pConnectivitySolutions;
            }
            else {
                // Update remote NetProxy UniqueID and add the ConnectivitySolutions object to the table with the new key
                pConnectivitySolutions->_remoteProxyInfo.setRemoteProxyID (ui32RemoteProxyNewID);
                _remoteProxyConnectivityTable.put (ui32RemoteProxyNewID, pConnectivitySolutions);
            }
        }

        return 1;
    }

    int ConnectionManager::updateRemoteProxyInfo (uint32 ui32RemoteProxyID, const NOMADSUtil::InetAddr * const pRemoteProxyAddress, uint16 ui16MocketsServerPort,
                                                  uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability)
    {
        if (ui32RemoteProxyID == 0) {
            return -1;
        }

        int res = 0;
        std::lock_guard<std::mutex> lg (_m);
        ConnectivitySolutions *pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (pConnectivitySolutions) {
            // ConnectivitySolutions to reach the remote proxy already in the table; updating it will update all related entry in the Remote Host Mapping Table
            ConnectionManager::updateRemoteProxyInfo (pConnectivitySolutions->_remoteProxyInfo, pRemoteProxyAddress, ui16MocketsServerPort,
                                                      ui16TCPServerPort, ui16UDPServerPort, bRemoteProxyReachability);
        }
        else {
            // New info needs to be added to the Remote Proxy Info Table; the default Mockets configuration file will be used
            pConnectivitySolutions = new ConnectivitySolutions (RemoteProxyInfo (ui32RemoteProxyID, pRemoteProxyAddress->getIPAddress(),
                                                                                 ui16MocketsServerPort, ui16TCPServerPort, ui16UDPServerPort,
                                                                                 NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE));
            pConnectivitySolutions->_remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (bRemoteProxyReachability);
            _remoteProxyConnectivityTable.put (ui32RemoteProxyID, pConnectivitySolutions);
            res = 1;
        }

        return res;
    }

    Connection * const ConnectionManager::updateRemoteProxyInfo (Connection * const pConnectionToRemoteProxy, uint32 ui32RemoteProxyID, uint16 ui16MocketsServerPort,
                                                                 uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability)
    {
        if (!pConnectionToRemoteProxy || (ui32RemoteProxyID == 0)) {
            return nullptr;
        }

        Connection *pOldConnection = nullptr;
        ConnectivitySolutions *pConnectivitySolutions = nullptr;
        std::lock_guard<std::mutex> lg (_m);
        if ((pConnectionToRemoteProxy->getRemoteProxyID() != ui32RemoteProxyID) && (pConnectionToRemoteProxy->getRemoteProxyID() != 0)) {
            checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", Logger::L_Info,
                            "The UniqueID notified from the remote NetProxy with address %s:%hu differs from the one previously "
                            "set for this connection instance (old ID: %u - new ID: %u) - updating the remote NetProxy UniqueID\n",
                            pConnectionToRemoteProxy->getRemoteProxyInetAddr()->getIPAsString(),
                            pConnectionToRemoteProxy->getRemoteProxyInetAddr()->getPort(),
                            pConnectionToRemoteProxy->getRemoteProxyID(), ui32RemoteProxyID);

            // It is necessary to update the remote NetProxy UniqueID and the key of the corresponding entry in the Remote Proxy Connectivity Table
            pConnectivitySolutions = _remoteProxyConnectivityTable.remove (pConnectionToRemoteProxy->getRemoteProxyID());
            if (ConnectivitySolutions * const pConnectivityToNewID = _remoteProxyConnectivityTable.get (ui32RemoteProxyID)) {
                /* There is already an entry in the Remote Proxy Connectivity Table for the new UniqueID --> update all entries
                    * in the Address Mapping Book to point to the new ConnectivitySolutions object and then delete the old one */
                for (int i = 0; i <= _remoteHostAddressMappingBook.getHighestIndex(); ++i) {
                    if (_remoteHostAddressMappingBook.get (i).second == pConnectivitySolutions) {
                        _remoteHostAddressMappingBook.get (i).second = pConnectivityToNewID;
                    }
                }
                delete pConnectivitySolutions;
                pConnectivitySolutions = pConnectivityToNewID;      // To avoid another lookup below
            }
            else {
                // Update remote NetProxy UniqueID and add the ConnectivitySolutions object to the table with the new key
                pConnectivitySolutions->_remoteProxyInfo.setRemoteProxyID (ui32RemoteProxyID);
                _remoteProxyConnectivityTable.put (ui32RemoteProxyID, pConnectivitySolutions);
            }
        }

        // Look for the entry in the table only if necessary (namely, only if the execution flow did not enter the previous "if" brench)
        pConnectivitySolutions = pConnectivitySolutions ? pConnectivitySolutions : _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (pConnectivitySolutions) {
            /* ConnectivitySolutions to reach the remote proxy already in the table.
                * Updating this object will in turn update all the entries in the Remote Host Mapping Table that point to it. */
            pOldConnection = pConnectivitySolutions->setActiveConnection (pConnectionToRemoteProxy);
            ConnectionManager::updateRemoteProxyInfo (pConnectivitySolutions->_remoteProxyInfo, pConnectionToRemoteProxy->getRemoteProxyInetAddr(),
                                                        ui16MocketsServerPort, ui16TCPServerPort, ui16UDPServerPort, bRemoteProxyReachability);
        }
        else {
            // New info needs to be added to the Remote Proxy Info Table; the default Mockets configuration file will be used
            pConnectivitySolutions = new ConnectivitySolutions (pConnectionToRemoteProxy,
                                                                RemoteProxyInfo (ui32RemoteProxyID, pConnectionToRemoteProxy->getRemoteProxyInetAddr()->getIPAddress(),
                                                                                    ui16MocketsServerPort, ui16TCPServerPort, ui16UDPServerPort,
                                                                                    NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE));
            pConnectivitySolutions->_remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (bRemoteProxyReachability);
            _remoteProxyConnectivityTable.put (ui32RemoteProxyID, pConnectivitySolutions);
        }

        return pOldConnection;
    }

    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxy (Connection * const pConnectionToRemoteProxy, uint32 ui32RemoteProxyID)
    {
        if (!pConnectionToRemoteProxy || (ui32RemoteProxyID == 0)) {
            return nullptr;
        }

        Connection *pOldConnection = nullptr;
        std::lock_guard<std::mutex> lg (_m);
        ConnectivitySolutions *pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (pConnectivitySolutions) {
            /* ConnectivitySolutions to reach the remote proxy already in the table.
                * Updating this object will in turn update all the entries in the Remote Host Mapping Table that point to it. */
            pOldConnection = pConnectivitySolutions->setActiveConnection (pConnectionToRemoteProxy);
        }
        else {
            pConnectivitySolutions = new ConnectivitySolutions (pConnectionToRemoteProxy, RemoteProxyInfo (ui32RemoteProxyID,
                                                                static_cast<uint32> (pConnectionToRemoteProxy->getRemoteProxyInetAddr()->getIPAddress())));
            _remoteProxyConnectivityTable.put (ui32RemoteProxyID, pConnectivitySolutions);
        }

        return pOldConnection;
    }

    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone (Connection * const pConnectionToRemoteProxy, uint32 ui32RemoteProxyID)
    {
        if (!pConnectionToRemoteProxy || (ui32RemoteProxyID == 0)) {
            return nullptr;
        }

        Connection *pConnectionToReturn = nullptr;
        std::lock_guard<std::mutex> lg (_m);
        ConnectivitySolutions *pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (pConnectivitySolutions) {
            /* ConnectivitySolutions to reach the remote proxy already in the table.
            * Updating this object will in turn update all the entries in the Remote Host Mapping Table that point to it. */
            Connection * pOldConnection =
                pConnectivitySolutions->getActiveConnectionForConnectorType (pConnectionToRemoteProxy->getConnectorType(), pConnectionToRemoteProxy->getEncryptionType());
            if (pOldConnection && pOldConnection != pConnectionToRemoteProxy) {
                pConnectionToReturn = pConnectionToRemoteProxy;
            }
            else if (!pOldConnection) {
                pConnectivitySolutions->setActiveConnection (pConnectionToRemoteProxy);
            }
            // if pOldConnection == pConnectionToRemoteProxy it is weird, but it is OK
        }
        else {
            pConnectivitySolutions = new ConnectivitySolutions (pConnectionToRemoteProxy,
                RemoteProxyInfo (ui32RemoteProxyID, static_cast<uint32> (pConnectionToRemoteProxy->getRemoteProxyInetAddr()->getIPAddress())));
            _remoteProxyConnectivityTable.put (ui32RemoteProxyID, pConnectivitySolutions);
        }

        return pConnectionToReturn;
    }

    bool ConnectionManager::isRemoteHostIPMapped (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const
    {
        bool res = false;
        std::lock_guard<std::mutex> lg (_m);

        std::pair<AddressRangeDescriptor, ConnectivitySolutions *> *pPair = _remoteHostAddressMappingCache.get (ui32RemoteHostIP);
        if (pPair) {
            res = pPair->first.matchesPort (ui16RemoteHostPort);
        }
        if (!res) {
            /* This second lookup makes sure to find address mappings when
                * defined on multiple lines in the proxyAddrMapping.cfg file. */
            res = isAddressAMatchInTheMappingBook (ui32RemoteHostIP, ui16RemoteHostPort);
        }

        return res;
    }

    const QueryResult ConnectionManager::queryConnectionToRemoteHostForConnectorType (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort,
                                                                                      ConnectorType connectorType, EncryptionType encryptionType) const
    {
        std::lock_guard<std::mutex> lg (_m);

        ConnectivitySolutions *pConnectivitySolutions = findConnectivitySolutionsToRemoteHost (ui32RemoteHostIP, ui16RemoteHostPort);
        const QueryResult queryRes (pConnectivitySolutions ?
                                    pConnectivitySolutions->getBestConnectionSolutionForConnectorType (connectorType, encryptionType) :
                                    QueryResult::getInvalidQueryResult());

        return queryRes;
    }

    // Method invoked when forwarding packets to multiple hosts (behind one or more NetProxy), such as in case of remapped multicast/broadcast traffic
    const NPDArray2<QueryResult> ConnectionManager::queryAllConnectionsToRemoteHostForConnectorType (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort,
                                                                                                     ConnectorType connectorType, EncryptionType encryptionType) const
    {
        NPDArray2<QueryResult> queryResList;

        std::lock_guard<std::mutex> lg (_m);
        const DArray<const ConnectivitySolutions *> da2ConnectivitySolutions (findAllConnectivitySolutionsToRemoteHost (ui32RemoteHostIP, ui16RemoteHostPort));
        for (int i = 0; i <= da2ConnectivitySolutions.getHighestIndex(); ++i) {
            queryResList.add (da2ConnectivitySolutions.get (i) ?
                              da2ConnectivitySolutions.get (i)->getBestConnectionSolutionForConnectorType (connectorType, encryptionType) : QueryResult::getInvalidQueryResult());
        }

        return queryResList;
    }

    const QueryResult ConnectionManager::queryConnectionToRemoteProxyIDForConnectorTypeAndEncryptionType (uint32 ui32RemoteProxyID, ConnectorType connectorType, EncryptionType encryptionType) const
    {
        std::lock_guard<std::mutex> lg (_m);
        const ConnectivitySolutions * const pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        const QueryResult queryRes (pConnectivitySolutions ?
                                    pConnectivitySolutions->getBestConnectionSolutionForConnectorType (connectorType, encryptionType) :
                                    QueryResult::getInvalidQueryResult());

        return queryRes;
    }

    const bool ConnectionManager::isConnectionToRemoteProxyOpenedForConnector (uint32 ui32RemoteProxyID, ConnectorType connectorType, EncryptionType encryptionType) const
    {
        bool res = false;

        std::lock_guard<std::mutex> lg (_m);
        const ConnectivitySolutions * const pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (pConnectivitySolutions) {
            const Connection * const pConnection = pConnectivitySolutions->getActiveConnectionForConnectorType (connectorType, encryptionType);
            res = pConnection ? pConnection->isConnected() : false;
        }

        return res;
    }

    const bool ConnectionManager::isConnectionToRemoteProxyOpeningForConnector (uint32 ui32RemoteProxyID, ConnectorType connectorType, EncryptionType encryptionType) const
    {
        bool res = false;

        std::lock_guard<std::mutex> lg (_m);
        const ConnectivitySolutions * const pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (pConnectivitySolutions) {
            const Connection * const pConnection = pConnectivitySolutions->getActiveConnectionForConnectorType (connectorType, encryptionType);
            res = pConnection ? pConnection->isConnecting() : false;
        }

        return res;
    }

    Connection * const ConnectionManager::removeActiveConnectionFromTable (const Connection * const pClosedConnection)
    {
        if (!pClosedConnection || (pClosedConnection->getRemoteProxyID() == 0)) {
            return nullptr;
        }

        Connection *pOldConnection = nullptr;
        std::lock_guard<std::mutex> lg (_m);

        ConnectivitySolutions * const pConnectivitySolutions = _remoteProxyConnectivityTable.get (pClosedConnection->getRemoteProxyID());
        if (pConnectivitySolutions) {
            pOldConnection = pConnectivitySolutions->removeActiveConnection (pClosedConnection);
        }

        return pOldConnection;
    }

    AutoConnectionEntry * const ConnectionManager::getAutoConnectionEntryToRemoteProxyID (uint32 ui32RemoteProxyID) const
    {
        for (int i = 0; i <= _autoConnectionList.getHighestIndex(); ++i) {
            AutoConnectionEntry &autoConnectionEntry = _autoConnectionList.get (i);
            if (autoConnectionEntry.getRemoteProxyID() == ui32RemoteProxyID) {
                return &autoConnectionEntry;
            }
        }

        return nullptr;
    }

    int ConnectionManager::getNumberOfOpenConnections (void)
    {
        int res = 0;
        const ConnectivitySolutions * pConnectivitySolution = nullptr;

        std::lock_guard<std::mutex> lg (_m);
        auto connectivitySolutionsIter = _remoteProxyConnectivityTable.getAllElements();
        while (pConnectivitySolution = connectivitySolutionsIter.getValue()) {
            auto & activeConnections = pConnectivitySolution->getActiveConnections();
            if (activeConnections.isAnyConnectorActive()) {
                ++res;
            }
            connectivitySolutionsIter.nextElement();
        }

        return res;
    }

	NOMADSUtil::LList<uint32> ConnectionManager::getRemoteProxyAddrList (void) const
	{
		while (true) {
            std::lock_guard<std::mutex> lg (_m);

			NOMADSUtil::LList<uint32> uint32remoteProxyAddrList;
			for (int count = 0; count < _remoteHostAddressMappingBook.size(); count++) {
                uint32 tmpAddr = (((_remoteHostAddressMappingBook.get (count)).second)->getRemoteProxyInfo()).getRemoteProxyIPAddress();
                if (uint32remoteProxyAddrList.search (tmpAddr) == 0) {
                    uint32remoteProxyAddrList.add (tmpAddr);
                }
			}

			return uint32remoteProxyAddrList;
		}
	}

    /* This method should be invoked by the NetProxyConfigManager parser only, and so it does not need to lock() */
    int ConnectionManager::addNewAddressMappingToBook (const AddressRangeDescriptor & addressRange, uint32 ui32RemoteProxyID)
    {
        ConnectivitySolutions *pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
        if (pConnectivitySolutions) {
            // Add the new address mapping entry in the connection Info Table (deleting the old entry, if any)
            _remoteHostAddressMappingBook.add (std::make_pair (addressRange, pConnectivitySolutions));
        }
        else {
            return -1;
        }

        return 0;
    }

    /* This method should be invoked by the NetProxyConfigManager parser only, and so it does not need to lock() */
    int ConnectionManager::addOrUpdateAutoConnectionToList (const AutoConnectionEntry & autoConnectionEntry) {
        if (autoConnectionEntry.getRemoteProxyID() == 0) {
            // Invalid remote NetProxy UniqueID
            return -1;
        }

        for (int i = 0; i <= _autoConnectionList.getHighestIndex(); i++) {
            AutoConnectionEntry &rAutoConnectionEntry = _autoConnectionList.get (i);
            if (rAutoConnectionEntry.getRemoteProxyID() == autoConnectionEntry.getRemoteProxyID()) {
                // Update old AutoConnection entry with the new one
                _autoConnectionList[i] = autoConnectionEntry;
                return 0;
            }
        }

        if (autoConnectionEntry.getConnectorType() != CT_UNDEF) {
            // Add new entry
            _autoConnectionList.add (autoConnectionEntry);
            return 1;
        }

        // Nothing could be done
        return -2;
    }

    ConnectivitySolutions * const ConnectionManager::findConnectivitySolutionsToRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const
    {
        std::pair<AddressRangeDescriptor, ConnectivitySolutions *> *pPair  = nullptr;
        {
            std::lock_guard<std::mutex> lg (_m);
            if ((pPair = _remoteHostAddressMappingCache.get (ui32RemoteHostIP)) == nullptr) {
                // Connectivity solutions not cached --> search in the mapping book for a match for the specified host (do not consider the port now)
                pPair = findConnectivitySolutionsInTheMappingBook (ui32RemoteHostIP, 0);
            }
        }

        return pPair ? (pPair->first.matchesPort (ui16RemoteHostPort) ? pPair->second : nullptr) : nullptr;
    }
    /*
    ConnectivitySolutions * const ConnectionManager::findConnectivitySolutionsFromTableWithConnection (const Connection * const pConnection) const
    {
        if (!pConnection) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lg (_m);

        auto connectivityTableIter = _remoteProxyConnectivityTable.getAllElements();
        ConnectivitySolutions * pConnectivitySolutions = nullptr;
        while (pConnectivitySolutions = connectivityTableIter.getValue()) {
            if (pConnection ==
                pConnectivitySolutions->getActiveConnectionForConnectorType (pConnection->getConnectorType(), pConnection->getEncryptionType())) {
                return pConnectivitySolutions;
            }
            connectivityTableIter.nextElement();
        }

        return nullptr;
    }*/

    // Method invoked when forwarding packets to multiple hosts (behind one or more NetProxy), such as in case of remapped multicast/broadcast traffic
    const DArray<const ConnectivitySolutions *> ConnectionManager::findAllConnectivitySolutionsToRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const
    {
        unsigned int solutionsFound = 0;
        DArray<const ConnectivitySolutions *> daConnectivitySolutions;

        std::lock_guard<std::mutex> lg (_m);
        for (int i = 0; i <= _remoteHostAddressMappingBook.getHighestIndex(); ++i) {
            if (_remoteHostAddressMappingBook.get (i).first.matches (ui32RemoteHostIP, ui16RemoteHostPort)) {
                daConnectivitySolutions[solutionsFound++] = _remoteHostAddressMappingBook.get (i).second;
            }
        }

        return daConnectivitySolutions;
    }

    std::pair<AddressRangeDescriptor, ConnectivitySolutions *> * const ConnectionManager::findConnectivitySolutionsInTheMappingBook (uint32 ui32RemoteHostIP,
                                                                                                                                     uint16 ui16RemoteHostPort) const
    {
        for (int i = 0; i <= _remoteHostAddressMappingBook.getHighestIndex(); ++i) {
            if (_remoteHostAddressMappingBook.get (i).first.matches (ui32RemoteHostIP, ui16RemoteHostPort)) {
                _remoteHostAddressMappingCache.put (ui32RemoteHostIP, &_remoteHostAddressMappingBook.get (i));
                return &_remoteHostAddressMappingBook.get (i);
            }
        }

        return nullptr;
    }

    AutoConnectionEntry * const ConnectionManager::getAutoConnectionEntryForProxyWithID (uint32 ui32RemoteProxyID) const
    {
        for (int i = 0; i <= _autoConnectionList.getHighestIndex(); i++) {
            AutoConnectionEntry &rAutoConnectionEntry = _autoConnectionList.get (i);
            if (rAutoConnectionEntry.getRemoteProxyID() == ui32RemoteProxyID) {
                // Update old AutoConnection entry with the new one
                return &rAutoConnectionEntry;
            }
        }

        return nullptr;
    }

    void ConnectionManager::updateRemoteProxyInfo (RemoteProxyInfo & currentRemoteProxyInfo, const RemoteProxyInfo & updatedRemoteProxyInfo)
    {
        // Update all attributes
        currentRemoteProxyInfo.setRemoteIPAddr (updatedRemoteProxyInfo.getRemoteProxyInetAddr (CT_MOCKETS)->getIPAddress());
        currentRemoteProxyInfo.setRemoteServerPort (CT_MOCKETS, updatedRemoteProxyInfo.getRemoteProxyInetAddr (CT_MOCKETS)->getPort());
        currentRemoteProxyInfo.setRemoteServerPort (CT_TCPSOCKET, updatedRemoteProxyInfo.getRemoteProxyInetAddr (CT_TCPSOCKET)->getPort());
        currentRemoteProxyInfo.setRemoteServerPort (CT_UDPSOCKET, updatedRemoteProxyInfo.getRemoteProxyInetAddr (CT_UDPSOCKET)->getPort());
        currentRemoteProxyInfo.setRemoteServerPort (CT_CSR, updatedRemoteProxyInfo.getRemoteProxyInetAddr (CT_CSR)->getPort());
        currentRemoteProxyInfo.setMocketsConfigFileName (updatedRemoteProxyInfo.getMocketsConfFileName());
        currentRemoteProxyInfo.setLocalProxyReachabilityFromRemote (updatedRemoteProxyInfo.isLocalProxyReachableFromRemote());
        currentRemoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (updatedRemoteProxyInfo.isRemoteProxyReachableFromLocalHost());
    }

    void ConnectionManager::updateRemoteProxyInfo (RemoteProxyInfo & currentRemoteProxyInfo, const NOMADSUtil::InetAddr * const pInetAddr,
                                                   uint16 ui16MocketsServerPort, uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability)
    {
        // Update all attributes
        currentRemoteProxyInfo.setRemoteIPAddr (pInetAddr->getIPAddress());
        currentRemoteProxyInfo.setRemoteServerPort (CT_MOCKETS, ui16MocketsServerPort);
        currentRemoteProxyInfo.setRemoteServerPort (CT_TCPSOCKET, ui16TCPServerPort);
        currentRemoteProxyInfo.setRemoteServerPort (CT_UDPSOCKET, ui16UDPServerPort);
        currentRemoteProxyInfo.setRemoteServerPort (CT_CSR, ui16MocketsServerPort);
        currentRemoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (bRemoteProxyReachability);
    }
}
