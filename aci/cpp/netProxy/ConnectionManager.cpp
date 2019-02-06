/*
 * ConnectionManager.cpp
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

#include "uint128.h"
#include "InetAddr.h"
#include "Logger.h"

#include "ConnectionManager.h"
#include "RemoteProxyInfo.h"
#include "AutoConnectionEntry.h"
#include "TCPConnTable.h"
#include "Connection.h"
#include "Connector.h"
#include "Utilities.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    int ConnectionManager::registerConnector (const std::shared_ptr <Connector> && spConnector)
    {
        if (!spConnector) {
            return -1;
        }

        _umConnectorsTable[spConnector->getBoundIPv4Address()][static_cast<uint8> (spConnector->getConnectorType())] = std::move (spConnector);
        return 0;
    }

    int ConnectionManager::addNewRemoteProxyInfo (const RemoteProxyInfo &remoteProxyInfo)
    {
        // No need to lock any mutex here, as this method is only invoked by the ConfigurationManager
        int res = _umRemoteProxyInfoTable.count (remoteProxyInfo.getRemoteNetProxyID());
        _umRemoteProxyInfoTable[remoteProxyInfo.getRemoteNetProxyID()] = std::make_shared<RemoteProxyInfo> (remoteProxyInfo);

        return res;
    }

    int ConnectionManager::addNewRemoteProxyInfo (RemoteProxyInfo && remoteProxyInfo)
    {
        // No need to lock any mutex here, as this method is only invoked by the ConfigurationManager
        int res = _umRemoteProxyInfoTable.count (remoteProxyInfo.getRemoteNetProxyID());
        _umRemoteProxyInfoTable[remoteProxyInfo.getRemoteNetProxyID()] = std::make_shared<RemoteProxyInfo> (std::move (remoteProxyInfo));

        return res;
    }

    int ConnectionManager::updateRemoteProxyInfo (Connection * const pConnectionToRemoteProxy, const uint32 ui32RemoteProxyUniqueID,
                                                  const std::vector<uint32> & vui32InterfaceIPv4AddressList, uint16 ui16MocketsServerPort,
                                                  uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability)
    {
        if (!pConnectionToRemoteProxy || (ui32RemoteProxyUniqueID == 0)) {
            return -1;
        }

        Connection * pOldConnection = nullptr;
        const auto ui32OldRemoteNetProxyID = pConnectionToRemoteProxy->getRemoteNetProxyID();

        std::lock_guard<std::mutex> lg{_mtx};
        if ((ui32OldRemoteNetProxyID != ui32RemoteProxyUniqueID) && (_umRemoteProxyInfoTable.count (ui32RemoteProxyUniqueID) == 0)) {
            // UniqueID of the remote NetProxy reachable via pConnectionToRemoteProxy needs to be updated
            checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_LowDetailDebug,
                            "remoteProxyInfoTable does not contain an entry for the remote NetProxy with UniqueID %u "
                            "and interface IPv4 address %s; the NetProxy will look into the table to see if it can "
                            "find an entry that matches the main IPv4 address %s\n", ui32RemoteProxyUniqueID,
                            pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            NOMADSUtil::InetAddr{vui32InterfaceIPv4AddressList.at (0)}.getIPAsString());

            if (ui32OldRemoteNetProxyID != 0) {
                // This happens only for connections for which this NetProxy was the client
                checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_Info,
                                "the UniqueID notified from the remote NetProxy with address <%s:%hu> differs from the one previously "
                                "set for this connection instance (old ID: %u - new ID: %u) - updating the remote NetProxy UniqueID\n",
                                pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getPort(),
                                ui32OldRemoteNetProxyID, ui32RemoteProxyUniqueID);

                // Update the old entry and then update the remoteProxyInfoTable with the new remote NetProxy UniqueID
                ConnectionManager::updateRemoteProxyInfo (_umRemoteProxyInfoTable[ui32OldRemoteNetProxyID], ui32RemoteProxyUniqueID, vui32InterfaceIPv4AddressList,
                                                          ui16MocketsServerPort, ui16TCPServerPort, ui16UDPServerPort, bRemoteProxyReachability);
                auto it = _umRemoteProxyInfoTable.find (ui32OldRemoteNetProxyID);
                std::swap (_umRemoteProxyInfoTable[ui32RemoteProxyUniqueID], it->second);
                _umRemoteProxyInfoTable.erase (it);

                // Update keys in the remoteProxyConnectivityTable
                if (_umRemoteProxyConnectivityTable.count (ui32OldRemoteNetProxyID) > 0) {
                    auto it = _umRemoteProxyConnectivityTable.find (ui32OldRemoteNetProxyID);
                    std::swap (_umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID], it->second);
                    _umRemoteProxyConnectivityTable.erase (it);
                }
            }
            else {
                // This happens only for connections for which this NetProxy was the server --> look for a matching entry in the remoteProxyInfoTable
                auto ui32LocallyConfiguredRemoteNetProxyUniqueID =
                    findUniqueIDOfRemoteNetProxyWithMainIPv4Address (vui32InterfaceIPv4AddressList.at (0));
                if (ui32LocallyConfiguredRemoteNetProxyUniqueID == 0) {
                    // Check that all interfaces configured in the local NetProxy config file are assigned to the same remote NetProxy
                    for (const auto ui32IPv4Addr : vui32InterfaceIPv4AddressList) {
                        if (ui32LocallyConfiguredRemoteNetProxyUniqueID == 0) {
                            ui32LocallyConfiguredRemoteNetProxyUniqueID = findUniqueIDOfRemoteNetProxyWithIPAddress (ui32IPv4Addr);
                        }
                        else {
                            auto ui32RetrievedRemoteNetProxyUniqueID = findUniqueIDOfRemoteNetProxyWithIPAddress (ui32IPv4Addr);
                            if ((ui32RetrievedRemoteNetProxyUniqueID != 0) &&
                                (ui32LocallyConfiguredRemoteNetProxyUniqueID != ui32RetrievedRemoteNetProxyUniqueID)) {
                                // The IP addresses notified by the remote NetProxy are assigned to different NetProxies in the local configuration
                                checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_MildError,
                                                "the remote NetProxy with UniqueID %u has the following IPv4 addresses assigned to its "
                                                "interfaces: %s; however, in the local configuration, at least two of those IP addresses "
                                                "are assigned to different NetProxies, including the ones with UniqueID %u and %u; check "
                                                "the configuration and restart the local NetProxy\n", ui32RemoteProxyUniqueID,
                                                vectorToString<uint32> (vui32InterfaceIPv4AddressList, [](const uint32 & ui32IPv4Address)
                                                    {
                                                        return std::string{NOMADSUtil::InetAddr{ui32IPv4Address}.getIPAsString()};
                                                    }).c_str(),
                                                ui32LocallyConfiguredRemoteNetProxyUniqueID, ui32RetrievedRemoteNetProxyUniqueID);
                                return -2;
                            }
                        }
                    }

                    if (ui32LocallyConfiguredRemoteNetProxyUniqueID == 0) {
                        // Match not found
                        checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_Info,
                                        "the remote NetProxy with UniqueID %u has the following IPv4 addresses assigned to its interfaces: %s; "
                                        "no entry in the local NetProxy configuration is a match for the remote NetProxy; creating a new entry\n",
                                        ui32RemoteProxyUniqueID, vectorToString<uint32> (vui32InterfaceIPv4AddressList,
                                            [](const uint32 & ui32IPv4Address) {
                                                return std::string{NOMADSUtil::InetAddr{ui32IPv4Address}.getIPAsString()};
                                            }).c_str());
                        std::vector<NOMADSUtil::InetAddr> viaRemoteInterfaces;
                        std::transform (vui32InterfaceIPv4AddressList.cbegin(), vui32InterfaceIPv4AddressList.cend(), std::back_inserter (viaRemoteInterfaces),
                            [](const uint32 ui32IPv4Address) { return NOMADSUtil::InetAddr{ui32IPv4Address}; });
                        _umRemoteProxyInfoTable[ui32RemoteProxyUniqueID] =
                            std::make_shared<RemoteProxyInfo>(ui32RemoteProxyUniqueID, viaRemoteInterfaces,
                                                              NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE.c_str());
                    }
                    else {
                        // Found a matching remote NetProxy in the configuration file
                        checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_Info,
                                        "the remote NetProxy with UniqueID %u has the following IPv4 addresses assigned to its interfaces: "
                                        "%s; the remote NetProxy currently configured to have the UniqueID %u is the best match\n",
                                        ui32RemoteProxyUniqueID, vectorToString<uint32> (vui32InterfaceIPv4AddressList,
                                            [](const uint32 & ui32IPv4Address) {
                                                return std::string{NOMADSUtil::InetAddr{ui32IPv4Address}.getIPAsString()};
                                            }).c_str(),
                                        ui32LocallyConfiguredRemoteNetProxyUniqueID);
                    }
                }

                if (ui32LocallyConfiguredRemoteNetProxyUniqueID != 0) {
                    // Update the UniqueID associated to the entry with the same main interface IPv4 address
                    auto it = _umRemoteProxyInfoTable.find (ui32LocallyConfiguredRemoteNetProxyUniqueID);
                    ConnectionManager::updateRemoteProxyInfo (it->second, ui32RemoteProxyUniqueID, vui32InterfaceIPv4AddressList, ui16MocketsServerPort,
                                                              ui16TCPServerPort, ui16UDPServerPort, bRemoteProxyReachability);
                    std::swap (_umRemoteProxyInfoTable[ui32RemoteProxyUniqueID], it->second);
                    _umRemoteProxyInfoTable.erase (it);
                    checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_Info,
                                    "NetProxy updated the remote NetProxy UniqueID of the entry in the remoteProxyInfoTable that "
                                    "has the same main interface IPv4 address %s; the old UniqueID was %u, the new one is %u\n",
                                    pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                    ui32LocallyConfiguredRemoteNetProxyUniqueID, ui32RemoteProxyUniqueID);

                    // Update keys in the remoteProxyConnectivityTable
                    if (_umRemoteProxyConnectivityTable.count (ui32LocallyConfiguredRemoteNetProxyUniqueID) > 0) {
                        auto it = _umRemoteProxyConnectivityTable.find (ui32LocallyConfiguredRemoteNetProxyUniqueID);
                        std::swap (_umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID], it->second);
                        _umRemoteProxyConnectivityTable.erase (it);
                    }
                }
            }
        }

        // Remove all ConnectivitySolutions instances that refer to remote interface IPv4 addresses not in the list provided by the remote NetProxy
        auto & mui32ConnSol = _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID];
        for (auto it = mui32ConnSol.cbegin(); it != mui32ConnSol.cend(); ) {
            if (std::find (vui32InterfaceIPv4AddressList.cbegin(), vui32InterfaceIPv4AddressList.cend(), it->first.second) == vui32InterfaceIPv4AddressList.cend()) {
                mui32ConnSol.erase (it++);
            }
            else {
                ++it;
            }
        }

        // Populate all missing entries with new ConnectivitySolutions instances that refer to the updated remoteProxyInfo object
        for (const auto ui32IPv4Address : vui32InterfaceIPv4AddressList) {
            unsigned int uiCounter = 0;
            for (auto it = mui32ConnSol.cbegin(); it != mui32ConnSol.cend(); ++it) {
                if (it->first.second == ui32IPv4Address) {
                    // Make sure they point to the same RemoteProxyInfo object (it should be pointless to do so)
                    it->second->setRemoteProxyInfo (_umRemoteProxyInfoTable.at (ui32RemoteProxyUniqueID));
                    ++uiCounter;
                }
            }
            if (uiCounter == 0) {
                // Add a new ConnectivitySolutions instance from any local interface (0) to the remote interface
                /* TODO: add a new ConnectivitySolutions only if there is a route via a managed external interfaces to reach ui32IPv4Address. */
                mui32ConnSol[{0, ui32IPv4Address}] =
                    std::make_shared<ConnectivitySolutions> (0, ui32IPv4Address, _umRemoteProxyInfoTable.at (ui32RemoteProxyUniqueID));
            }
        }

        // Update the ConnectivitySolutions to reach the remote NetProxy from any local interface, if it exists
        if (_umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID].count (
            {0, pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAddress()}) > 0) {
            auto & spGeneralConnectivitySolutions =
                _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID][{0, pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAddress()}];
            if (spGeneralConnectivitySolutions) {
                auto pActiveConnection =
                    spGeneralConnectivitySolutions->getActiveConnectionForConnectorAndEncryptionType (pConnectionToRemoteProxy->getConnectorType(),
                                                                                                      pConnectionToRemoteProxy->getEncryptionType());
                if (!pActiveConnection) {
                    spGeneralConnectivitySolutions->setActiveConnection (pConnectionToRemoteProxy);
                    checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "added the newly accepted connection as an ActiveConnection to the ConnectivitySolutions to reach "
                                    "the remote NetProxy with UniqueID %u and interface IPv4 address %s from any local interface\n",
                                    ui32RemoteProxyUniqueID, pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAsString());
                }
            }
        }

        // Update ConnectivitySolutions to reach the remote NetProxy from the local interface that received the new connection
        auto & spConnectivitySolutions =
            _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID][{pConnectionToRemoteProxy->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                      pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAddress()}];
        if (!spConnectivitySolutions) {
            // ConnectivitySolutions to reach the remote NetProxy is absent --> create one
            checkAndLogMsg ("ConnectionManager::updateRemoteProxyInfo", NOMADSUtil::Logger::L_Warning,
                            "the remoteProxyConnectivityTable does not contain a ConnectivitySolutions instance to "
                            "reach the remote NetProxy with UniqueID %u and interface IPv4 address %s from the local "
                            "interface with IPv4 address %s; the NetProxy will create one now\n", ui32RemoteProxyUniqueID,
                            pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            pConnectionToRemoteProxy->getLocalInterfaceInetAddr()->getIPAsString());
            spConnectivitySolutions = std::make_shared<ConnectivitySolutions> (pConnectionToRemoteProxy->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                               pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                                               _umRemoteProxyInfoTable.at (ui32RemoteProxyUniqueID));
        }
        // Updating this instance of the ConnectivitySolutions is enough, as entries of the addressMappingList refer to it
        spConnectivitySolutions->setActiveConnection (pConnectionToRemoteProxy);

        return 0;
    }

    uint32 ConnectionManager::findUniqueIDOfRemoteNetProxyWithIPAddress (uint32 ui32InterfaceIPv4Address) const
    {
        for (const auto & ui32ConnSolPair : _umRemoteProxyInfoTable) {
            if ((ui32ConnSolPair.second != nullptr) &&
                ui32ConnSolPair.second->hasInterfaceIPv4Address (ui32InterfaceIPv4Address)) {
                return ui32ConnSolPair.first;
            }
        }

        return 0;
    }

    uint32 ConnectionManager::findBestMatchingUniqueIDOfRemoteNetProxyWithInterfaceIPs (const std::vector<uint32> & vui32InterfaceIPv4AddressList,
                                                                                        bool & bClashingConfiguration) const
    {
        uint32 ui32LocallyConfiguredRemoteNetProxyUniqueID =
            findUniqueIDOfRemoteNetProxyWithMainIPv4Address (vui32InterfaceIPv4AddressList.at (0));
        if (ui32LocallyConfiguredRemoteNetProxyUniqueID == 0) {
            // Check that all interfaces configured in the local NetProxy config file are assigned to the same remote NetProxy
            for (const auto ui32IPv4Addr : vui32InterfaceIPv4AddressList) {
                if (ui32LocallyConfiguredRemoteNetProxyUniqueID == 0) {
                    ui32LocallyConfiguredRemoteNetProxyUniqueID = findUniqueIDOfRemoteNetProxyWithIPAddress (ui32IPv4Addr);
                }
                else {
                    auto ui32RetrievedRemoteNetProxyUniqueID = findUniqueIDOfRemoteNetProxyWithIPAddress (ui32IPv4Addr);
                    if ((ui32RetrievedRemoteNetProxyUniqueID != 0) &&
                        (ui32LocallyConfiguredRemoteNetProxyUniqueID != ui32RetrievedRemoteNetProxyUniqueID)) {
                        // The IP addresses notified by the remote NetProxy are assigned to different NetProxies in the local configuration
                        checkAndLogMsg ("ConnectionManager::findBestMatchingUniqueIDOfRemoteNetProxyWithInterfaceIPs", NOMADSUtil::Logger::L_Warning,
                                        "the remote NetProxy has the following IPv4 addresses assigned to its interfaces: %s; however, "
                                        "in the local configuration, at least two of those IP addresses are assigned to different "
                                        "remote NetProxies, including, for instance, those with UniqueID %u and %u\n",
                                        vectorToString<uint32> (vui32InterfaceIPv4AddressList, [](const uint32 & ui32IPv4Address)
                                            {
                                                return std::string{NOMADSUtil::InetAddr{ui32IPv4Address}.getIPAsString()};
                                            }).c_str(),
                                        ui32LocallyConfiguredRemoteNetProxyUniqueID, ui32RetrievedRemoteNetProxyUniqueID);
                        bClashingConfiguration = true;
                        return 0;
                    }
                }
            }
        }

        bClashingConfiguration = false;
        return ui32LocallyConfiguredRemoteNetProxyUniqueID;
    }

    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxy (Connection * const pConnectionToRemoteProxy)
    {
        if (!pConnectionToRemoteProxy || (pConnectionToRemoteProxy->getRemoteNetProxyID() == 0)) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lg{_mtx};
        return addNewActiveConnectionToRemoteProxy_NoLock (pConnectionToRemoteProxy);
    }

    Connection * const ConnectionManager::removeActiveConnectionFromRemoteProxyConnectivityTable (uint32 ui32RemoteProxyUniqueID,
                                                                                                  uint32 ui32LocalInterfaceIPv4Address,
                                                                                                  uint32 ui32RemoteInterfaceIPv4Address,
                                                                                                  const Connection * const pClosedConnection)
    {
        if (!pClosedConnection || (ui32RemoteProxyUniqueID == 0) || (ui32RemoteInterfaceIPv4Address == 0)) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lg{_mtx};
        return removeActiveConnectionFromRemoteProxyConnectivityTable_NoLock (ui32RemoteProxyUniqueID, ui32LocalInterfaceIPv4Address,
                                                                              ui32RemoteInterfaceIPv4Address, pClosedConnection);
    }

    // This method assumes that all necessary locks, including the one on pConnectionToDelete, have been acquired by the caller
    void ConnectionManager::prepareForDelete (TCPConnTable & rTCPConnTable, Connection * const pConnectionToDelete,
                                              Connection * const pConnectionToSubstitute)
    {
        if (pConnectionToDelete == nullptr) {
            // Nothing to do
            return;
        }

        // Update entries in the TCPConnTable to point to the new Connection instance
        resetTCPConnectionEntriesWithConnection_NoLock (rTCPConnTable, pConnectionToDelete);

        // Substitute pConnectionToDelete with pConnectionToSubstitute in the RemoteProxyConnectivityTable, or remove it
        auto * const pOldActiveConnection = pConnectionToSubstitute ?
            switchActiveConnectionInRemoteProxyConnectivityTable_NoLock (pConnectionToDelete->getRemoteNetProxyID(),
                                                                         pConnectionToDelete->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                         pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                                         pConnectionToDelete, pConnectionToSubstitute) :
            removeActiveConnectionFromRemoteProxyConnectivityTable_NoLock (pConnectionToDelete->getRemoteNetProxyID(),
                                                                           pConnectionToDelete->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                           pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                                           pConnectionToDelete);
        if (pOldActiveConnection && (pOldActiveConnection != pConnectionToDelete)) {
            checkAndLogMsg ("Connector::prepareForDelete", NOMADSUtil::Logger::L_SevereError,
                            "found a different active %sConnection to the remote NetProxy with UniqueID %u and "
                            "address <%s:%hu> in the RemoteProxyConnectivityTable; this should NEVER happen\n",
                            pConnectionToDelete->getConnectorTypeAsString(), pConnectionToDelete->getRemoteNetProxyID(),
                            pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getPort());
        }

        // Remove pConnectionToDelete from the ConnectionsTable
        auto * const pOldConnectionInTable = Connection::removeConnectionFromTable_NoLock (pConnectionToDelete);
        if (pOldConnectionInTable && (pOldConnectionInTable != pConnectionToDelete)) {
            checkAndLogMsg ("Connector::prepareForDelete", NOMADSUtil::Logger::L_SevereError,
                            "found a different %s Connection instance to the remote NetProxy with UniqueID %u "
                            "and address <%s:%hu> in the ConnectionsTable; this should NEVER happen\n",
                            pConnectionToDelete->getConnectorTypeAsString(), pConnectionToDelete->getRemoteNetProxyID(),
                            pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getPort());
        }

        // Update AutoConnection instances
        resetAutoConnectionInstances_NoLock (pConnectionToDelete);
    }

    // This method should be invoked by the NetProxyConfigManager parser only, so it does not need to lock() the mutex
    int ConnectionManager::addNewAddressMappingToBook (const std::pair<NetworkAddressRange, NetworkAddressRange> & pardardMappingFilter,
                                                       const std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr> & tui32iaiaMappingRule)
    {
        auto & spRemoteProxyInfo = _umRemoteProxyInfoTable[std::get<0> (tui32iaiaMappingRule)];
        if (spRemoteProxyInfo == nullptr) {
            checkAndLogMsg ("ConnectionManager::addNewAddressMappingToBook", NOMADSUtil::Logger::L_Warning,
                            "no information found in the remoteProxyInfoTable for the remote NetProxy "
                            "with UniqueID %u; the NetProxy will ignore this mapping rule\n",
                            std::get<0> (tui32iaiaMappingRule));
            return -1;
        }
        if (std::get<2> (tui32iaiaMappingRule).getIPAddress() == 0) {
            checkAndLogMsg ("ConnectionManager::addNewAddressMappingToBook", NOMADSUtil::Logger::L_Warning,
                            "specified an invalid remote IPv4 address (%s) to remap traffic to the remote NetProxy with UniqueID %u\n",
                            std::get<2> (tui32iaiaMappingRule).getIPAsString(), std::get<0> (tui32iaiaMappingRule));
            return -2;
        }

        // Retrieve the relative ConnectivitySolutions object, or create one if it does not exist
        const auto ui32LocalIPv4Address = std::get<1> (tui32iaiaMappingRule).getIPAddress();
        const auto ui32RemoteIPv4Address = std::get<2> (tui32iaiaMappingRule).getIPAddress();
        auto & spConnSol = _umRemoteProxyConnectivityTable[std::get<0>(tui32iaiaMappingRule)][{ui32LocalIPv4Address, ui32RemoteIPv4Address}];
        spConnSol = spConnSol ?
            spConnSol : std::make_shared<ConnectivitySolutions> (ui32LocalIPv4Address, ui32RemoteIPv4Address, spRemoteProxyInfo);

        if (!spRemoteProxyInfo->hasInterfaceIPv4Address (ui32RemoteIPv4Address)) {
            // Add the specified interface to the remoteProxyInfo
            spRemoteProxyInfo->addRemoteServerAddr (std::get<2> (tui32iaiaMappingRule));
            checkAndLogMsg ("ConnectionManager::addNewAddressMappingToBook", NOMADSUtil::Logger::L_Warning,
                            "no information found in the remoteProxyInfo for the remote NetProxy with UniqueID %u "
                            "about a server address with IPv4 %s; NetProxy added it with the default port numbers\n",
                            std::get<0> (tui32iaiaMappingRule), std::get<2> (tui32iaiaMappingRule).getIPAsString());
        }

        if (isMulticastAddressRange (pardardMappingFilter.second)) {
            // Add the new multicast address mapping entry to the multicast/broadcast address mapping list
            _multiBroadCastAddressMappingList.emplace_back (pardardMappingFilter, spConnSol);
            return 0;
        }
        // Add the new address mapping entry to the address mapping list
        _addressMappingList.emplace_back (pardardMappingFilter, spConnSol);

        return 0;
    }

    void ConnectionManager::finalizeMappingRules (void)
    {
        std::sort (_addressMappingList.begin(), _addressMappingList.end(),
                   [] (const std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::shared_ptr<ConnectivitySolutions>> & lhs,
                       const std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::shared_ptr<ConnectivitySolutions>> & rhs)
                   {
                        return (NOMADSUtil::uint128 {lhs.first.first.getNumberOfAddressesInRange()} * lhs.first.second.getNumberOfAddressesInRange()) <
                            (NOMADSUtil::uint128 {rhs.first.first.getNumberOfAddressesInRange()} * rhs.first.second.getNumberOfAddressesInRange());
                   }
        );
    }

    void ConnectionManager::clearAllConnectionMappings (void)
    {
        std::lock_guard<std::mutex> lg{_mtx};

        _addressMappingList.clear();
        _multiBroadCastAddressMappingList.clear();
        _umRemoteProxyConnectivityTable.clear();
        _umRemoteProxyInfoTable.clear();
    }

    // This method only tries to match source and destination IP addresses against the remapping rules
    bool ConnectionManager::isFlowPotentiallyMapped (uint32 ui32SourceIP, uint32 ui32DestinationIP) const
    {
        std::lock_guard<std::mutex> lg{_mtx};
        if (isMulticastIPv4Address (ui32DestinationIP)) {
            return isMulticastAddressPotentiallyMapped (ui32SourceIP, ui32DestinationIP);
        }

        for (auto & ppARDARDConnSol : _addressMappingList) {
            if (ppARDARDConnSol.first.first.matchesAddress (ui32SourceIP) &&
                ppARDARDConnSol.first.second.matchesAddress (ui32DestinationIP) && ppARDARDConnSol.second) {
                return true;
            }
        }

        return false;
    }

    // This method tries to match <Source_IP:Source_Port>:<Destination_IP:Destination_Port> pairs against the remapping rules
    bool ConnectionManager::isFlowMapped (uint32 ui32SourceIP, uint16 ui16SourcePort, uint32 ui32DestinationIP, uint16 ui16DestinationPort) const
    {
        std::lock_guard<std::mutex> lg{_mtx};
        if (isMulticastIPv4Address (ui32DestinationIP)) {
            return isMulticastAddressMapped (ui32SourceIP, ui16SourcePort, ui32DestinationIP, ui16DestinationPort);
        }

        for (auto & ppARDARDConnSol : _addressMappingList) {
            if (ppARDARDConnSol.first.first.matches (ui32SourceIP, ui16SourcePort) &&
                ppARDARDConnSol.first.second.matches (ui32DestinationIP, ui16DestinationPort) && ppARDARDConnSol.second) {
                return true;
            }
        }

        return false;
    }

    // Find the ConnectivitySolutions instance from the remapping rules that matches the query for the specified IP addresses and that remap all ports
    const QueryResult ConnectionManager::queryConnectionToRemoteHostForConnectorType (uint32 ui32SourceIPAddress, uint32 ui32DestinationIPAddress,
                                                                                      ConnectorType connectorType, EncryptionType encryptionType) const
    {
        std::lock_guard<std::mutex> lg{_mtx};
        auto * const pConnectivitySolutions = findMappedConnectivitySolutions (ui32SourceIPAddress, ui32DestinationIPAddress);

        return QueryResult {pConnectivitySolutions ?
                                pConnectivitySolutions->getBestConnectionSolutionForConnectorAndEncryptionType (connectorType, encryptionType) :
                                QueryResult::getInvalidQueryResult()};
    }

    /* Find the ConnectivitySolutions instance from the remapping rules that matches the query for the specified IP addresses and ports.
     * Using a port number equal to 0 matches any port range specified in the remapping rules. */
    const QueryResult ConnectionManager::queryConnectionToRemoteHostForConnectorType (uint32 ui32SourceIPAddress, uint16 ui16SourcePort,
                                                                                      uint32 ui32DestinationIPAddress, uint16 ui16DestinationPort,
                                                                                      ConnectorType connectorType, EncryptionType encryptionType) const
    {
        std::lock_guard<std::mutex> lg{_mtx};
        auto * const pConnectivitySolutions = findMappedConnectivitySolutions (ui32SourceIPAddress, ui16SourcePort,
                                                                               ui32DestinationIPAddress, ui16DestinationPort);

        return QueryResult {pConnectivitySolutions ?
                                pConnectivitySolutions->getBestConnectionSolutionForConnectorAndEncryptionType (connectorType, encryptionType) :
                                QueryResult::getInvalidQueryResult()};
    }

    // Method invoked when forwarding packets to multiple hosts (behind one or more NetProxy), such as in case of remapped multicast/broadcast traffic
    const std::vector<QueryResult> ConnectionManager::queryAllConnectivitySolutionsToMulticastAddressForConnectorType (uint32 ui32SourceIP, uint16 ui16SourcePort,
                                                                                                                       uint32 ui32DestinationIP, uint16 ui16DestinationPort,
                                                                                                                       ConnectorType connectorType,
                                                                                                                       EncryptionType encryptionType) const
    {
        std::vector<QueryResult> queryResList;

        std::lock_guard<std::mutex> lg{_mtx};
        std::vector<const ConnectivitySolutions *> vConnectivitySolutions
            {findConnectivitySolutionsForRemappedMulticastAddress (ui32SourceIP, ui16SourcePort, ui32DestinationIP, ui16DestinationPort)};
        for (auto it = vConnectivitySolutions.cbegin(); it != vConnectivitySolutions.cend(); ++it) {
            queryResList.push_back (*it ? (*it)->getBestConnectionSolutionForConnectorAndEncryptionType (connectorType, encryptionType) :
                                          QueryResult::getInvalidQueryResult());
        }

        return queryResList;
    }

    const QueryResult ConnectionManager::queryConnectionToRemoteProxyIDAndInterfaceIPv4AddressForConnectorTypeAndEncryptionType (uint32 ui32RemoteProxyUniqueID,
                                                                                                                                 uint32 ui32LocalInterfaceIPv4Address,
                                                                                                                                 uint32 ui32RemoteInterfaceIPv4Address,
                                                                                                                                 ConnectorType connectorType,
                                                                                                                                 EncryptionType encryptionType) const
    {
        std::lock_guard<std::mutex> lg{_mtx};
        const auto & spConnectivitySolutions = _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID][{ui32LocalInterfaceIPv4Address,
                                                                                                         ui32RemoteInterfaceIPv4Address}];
        const QueryResult queryRes (spConnectivitySolutions ?
                                    spConnectivitySolutions->getBestConnectionSolutionForConnectorAndEncryptionType (connectorType, encryptionType) :
                                    QueryResult::getInvalidQueryResult());

        return queryRes;
    }

    std::unordered_set<ConnectivitySolutions *>
        ConnectionManager::findAllConnectivitySolutionsToNetProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteInterfaceIPv4Address) const
    {
        std::unordered_set<ConnectivitySolutions *> usRes;

        std::lock_guard<std::mutex> lg (_mtx);
        if (_umRemoteProxyConnectivityTable.count (ui32RemoteProxyUniqueID) == 0) {
            return usRes;
        }

        for (auto & umpUi32Ui32spCS : _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID)) {
            if (umpUi32Ui32spCS.first.second == ui32RemoteInterfaceIPv4Address) {
                usRes.insert (umpUi32Ui32spCS.second.get());
            }
        }

        return usRes;
    }

    ConnectivitySolutions * const ConnectionManager::findMappedConnectivitySolutions (uint32 ui32SourceIP, uint32 ui32DestinationIP) const
    {
        if (isMulticastIPv4Address (ui32DestinationIP)) {
            return nullptr;
        }

        for (auto & ppARDARDConnSol : _addressMappingList) {
            if (ppARDARDConnSol.first.first.matchesAddressForAllPorts (ui32SourceIP) &&
                ppARDARDConnSol.first.second.matchesAddressForAllPorts (ui32DestinationIP) && ppARDARDConnSol.second) {
                return ppARDARDConnSol.second.get();
            }
        }

        return nullptr;
    }

    ConnectivitySolutions * const ConnectionManager::findMappedConnectivitySolutions (uint32 ui32SourceIP, uint16 ui16SourcePort,
                                                                                      uint32 ui32DestinationIP, uint16 ui16DestinationPort) const
    {
        if (isMulticastIPv4Address (ui32DestinationIP)) {
            return nullptr;
        }

        for (auto & ppARDARDConnSol : _addressMappingList) {
            if (ppARDARDConnSol.first.first.matches (ui32SourceIP, ui16SourcePort) &&
                ppARDARDConnSol.first.second.matches (ui32DestinationIP, ui16DestinationPort) && ppARDARDConnSol.second) {
                return ppARDARDConnSol.second.get();
            }
        }

        return nullptr;
    }

    // Method invoked when forwarding packets to multiple hosts (behind one or more NetProxy), such as in case of remapped multicast/broadcast traffic
    std::vector<const ConnectivitySolutions *>
        ConnectionManager::findConnectivitySolutionsForRemappedMulticastAddress (uint32 ui32SourceIP, uint16 ui16SourcePort, uint32 ui32DestinationIP,
                                                                                 uint16 ui16DestinationPort) const
    {
        std::vector<const ConnectivitySolutions *> vConnectivitySolutions;

        for (auto & ardardConnSolPair : _multiBroadCastAddressMappingList) {
            if (ardardConnSolPair.first.first.matches (ui32SourceIP, ui16SourcePort) &&
                ardardConnSolPair.first.second.matches (ui32DestinationIP, ui16DestinationPort)) {
                vConnectivitySolutions.push_back (ardardConnSolPair.second.get());
            }
        }

        return vConnectivitySolutions;
    }

    std::vector<ConnectivitySolutions *> ConnectionManager::getAllConnectivitySolutions (void) const
    {
        std::vector<ConnectivitySolutions *> vConnSol;

        std::lock_guard<std::mutex> lg{_mtx};
        for (auto it = _umRemoteProxyConnectivityTable.cbegin(); it != _umRemoteProxyConnectivityTable.cend(); ++it) {
            for (const auto & pui32ConnSol : it->second) {
                if (pui32ConnSol.second != nullptr) {
                    vConnSol.emplace_back (pui32ConnSol.second.get());
                }
            }
        }

        return vConnSol;
    }

    bool ConnectionManager::isConnectionToRemoteProxyOpenedForConnector (uint32 ui32RemoteProxyUniqueID, ConnectorType connectorType,
                                                                         EncryptionType encryptionType) const
    {
        bool res = false;

        std::lock_guard<std::mutex> lg{_mtx};
        for (const auto & pui32ConnSol : _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID]) {
            if (pui32ConnSol.second != nullptr) {
                const auto * const pConnection = pui32ConnSol.second->getActiveConnectionForConnectorAndEncryptionType (connectorType, encryptionType);
                res = pConnection ? pConnection->isConnected() : false;
            }
        }

        return res;
    }

    bool ConnectionManager::isConnectionToRemoteProxyOpeningForConnector (uint32 ui32RemoteProxyUniqueID, ConnectorType connectorType,
                                                                          EncryptionType encryptionType) const
    {
        bool res = false;

        std::lock_guard<std::mutex> lg{_mtx};
        for (const auto & pui32ConnSol : _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID]) {
            if (pui32ConnSol.second != nullptr) {
                const auto * const pConnection = pui32ConnSol.second->getActiveConnectionForConnectorAndEncryptionType (connectorType, encryptionType);
                res = pConnection ? pConnection->isConnecting() : false;
            }
        }

        return res;
    }

    const char * const ConnectionManager::getMocketsConfigFileForProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                            uint32 ui32RemoteInterfaceIPv4Address) const
    {
        if ((_umRemoteProxyConnectivityTable.count (ui32RemoteProxyUniqueID) == 0) ||
            _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).count ({ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address}) == 0) {
            return "";
        }

        const ConnectivitySolutions * pConnectivitySolutions = nullptr;
        {
            std::lock_guard<std::mutex> lg (_mtx);
            pConnectivitySolutions = _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).at ({ui32LocalInterfaceIPv4Address,
                                                                                                       ui32RemoteInterfaceIPv4Address}).get();
        }

        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().getMocketsConfFileName() : "";
    }

    /* This method should be invoked by the ConfigurationManager parser only, and so it does not need to lock() */
    int ConnectionManager::addOrUpdateAutoConnectionToList (const AutoConnectionEntry & autoConnectionEntry) {
        if (autoConnectionEntry.getRemoteNetProxyID() == 0) {
            // Invalid remote NetProxy UniqueID
            return -1;
        }
        if (autoConnectionEntry.getRemoteProxyInetAddress() == NetProxyApplicationParameters::IA_INVALID_ADDR) {
            return -2;
        }
        if (autoConnectionEntry.getConnectorType() == CT_UNDEF) {
            return -3;
        }

        int res = 0;
        const auto ui32RemoteNetProxyUID = autoConnectionEntry.getRemoteNetProxyID();
        const auto ui32IPv4Address = autoConnectionEntry.getRemoteProxyInetAddress().getIPAddress();
        if ((_umAutoConnectionTable.count (ui32RemoteNetProxyUID) == 0) ||
            (_umAutoConnectionTable.at (ui32RemoteNetProxyUID).count (ui32IPv4Address) == 0)) {
            // Entry not already present --> return 1
            res = 1;
        }
        _umAutoConnectionTable[ui32RemoteNetProxyUID][ui32IPv4Address][static_cast<uint8> (autoConnectionEntry.getConnectorType())] =
            std::make_shared<AutoConnectionEntry> (autoConnectionEntry);

        return res;
    }

    void ConnectionManager::updateRemoteProxyIDForAutoConnectionWithID (uint32 ui32OldRemoteProxyID, uint32 ui32NewRemoteProxyID)
    {
        if ((ui32OldRemoteProxyID == ui32NewRemoteProxyID) || (ui32OldRemoteProxyID == 0) || (ui32NewRemoteProxyID == 0)) {
            return;
        }

        std::lock_guard<std::mutex> lg{_mtxAutoConnectionTable};
        const auto it = _umAutoConnectionTable.find (ui32OldRemoteProxyID);
        if (it != _umAutoConnectionTable.end()) {
            for (auto & pui32ctAutoConn : it->second) {
                for (auto & pctAutoConn : pui32ctAutoConn.second) {
                    pctAutoConn.second->updateRemoteProxyID (ui32NewRemoteProxyID);
                }
            }
            std::swap (_umAutoConnectionTable[ui32NewRemoteProxyID], it->second);
            _umAutoConnectionTable.erase (it);
        }
    }

    /* This method should be invoked by the ConfigurationManager parser only, and so it does not need to lock() */
    void ConnectionManager::updateAutoConnectionEntries (void)
    {
        for (auto & pui32ui32ctAutoConn : _umAutoConnectionTable) {
            for (auto & pui32ctAutoConn : pui32ui32ctAutoConn.second) {
                for (auto & pctAutoConn : pui32ctAutoConn.second) {
                    pctAutoConn.second->updateEncryptionDescriptor (*this);
                }
            }
        }
    }

    std::unordered_map<uint32, std::unordered_set<std::pair<uint32, uint8>>> ConnectionManager::getAutoConnectionKeysMap (void) const
    {
        std::unordered_map<uint32, std::unordered_set<std::pair<uint32, uint8>>> umAutoConnectionKeys;

        std::lock_guard<std::mutex> lg{_mtxAutoConnectionTable};
        for (auto & pui32ui32ctAutoConn : _umAutoConnectionTable) {
            for (auto & pui32ctAutoConn : pui32ui32ctAutoConn.second) {
                for (auto & pctAutoConn : pui32ctAutoConn.second) {
                    umAutoConnectionKeys[pui32ui32ctAutoConn.first].insert (
                        std::make_pair (pui32ctAutoConn.first, pctAutoConn.first));
                }
            }
        }

        return umAutoConnectionKeys;
    }

    unsigned int  ConnectionManager::getNumberOfValidAutoConnectionEntries (void) const
    {
        unsigned int numOfValidEntries = 0U;

        std::lock_guard<std::mutex> lg{_mtxAutoConnectionTable};
        for (auto & pui32ui32ctAutoConn : _umAutoConnectionTable) {
            for (auto & pui32ctAutoConn : pui32ui32ctAutoConn.second) {
                for (auto & pctAutoConn : pui32ctAutoConn.second) {
                    if (pctAutoConn.second->isAnyValid()) {
                        ++numOfValidEntries;
                    }
                }
            }
        }

        return numOfValidEntries;
    }

    int ConnectionManager::getNumberOfOpenConnections (void)
    {
        int res = 0;
        const ConnectivitySolutions * pConnectivitySolution = nullptr;

        std::lock_guard<std::mutex> lg{_mtx};
        for (auto it = _umRemoteProxyConnectivityTable.cbegin(); it != _umRemoteProxyConnectivityTable.cend(); ++it) {
            for (auto innerIt = it->second.cbegin(); innerIt != it->second.cend(); ++innerIt) {
                if ((innerIt->second != nullptr) && innerIt->second->getActiveConnections().isAnyConnectionActive()) {
                    ++res;
                }
            }
        }

        return res;
    }

    std::unordered_set<uint32> ConnectionManager::getRemoteProxyAddrList (void) const
    {
        std::unordered_set<uint32> usRemoteProxyAddrList;

        std::lock_guard<std::mutex> lg{_mtx};
        for (auto it = _umRemoteProxyInfoTable.cbegin(); it != _umRemoteProxyInfoTable.cend(); ++it) {
            if ((*it).second) {
                auto vIPAddresses = (*it).second->getRemoteProxyIPAddressList();
                usRemoteProxyAddrList.insert (vIPAddresses.cbegin(), vIPAddresses.cend());
            }
        }

        return usRemoteProxyAddrList;
    }

    std::vector<uint32> ConnectionManager::getListOfLocalInterfacesIPv4Addresses (void)
    {
        std::vector<uint32> vRes;
        vRes.reserve (NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.size());
        std::transform (NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cbegin(),
                        NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cend(),
                        std::back_inserter (vRes), [] (const NetworkInterfaceDescriptor & nid)
        {
            return nid.ui32IPv4Address;
        });

        return vRes;
    }

    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxy_NoLock (Connection * const pConnectionToRemoteProxy)
    {
        const uint32 ui32RemoteProxyUniqueID = pConnectionToRemoteProxy->getRemoteNetProxyID(),
            ui32LocalInterfaceIPv4Address = pConnectionToRemoteProxy->getLocalInterfaceInetAddr()->getIPAddress(),
            ui32RemoteInterfaceIPv4Address = pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAddress();

        // Add the new active Connection to the table for the specified local and remote IPv4 addresses
        auto * const pConnectionToReturn = addNewActiveConnectionToRemoteProxy_Impl (ui32RemoteProxyUniqueID, ui32LocalInterfaceIPv4Address,
                                                                                     ui32RemoteInterfaceIPv4Address, pConnectionToRemoteProxy);
        if (pConnectionToReturn && (pConnectionToReturn != pConnectionToRemoteProxy)) {
            // Another active Connection was found when trying to add a newly opened Connection; this should NEVER happen
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone", NOMADSUtil::Logger::L_Warning,
                            "the remoteProxyConnectivityTable contains a ConnectivitySolutions instance that already had an "
                            "active Connection to the remote NetProxy with UniqueID %u, local IPv4 address %s, and remote "
                            "IPv4 address %s; the NetProxy replaced it with the newly opened Connection\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
        }

        // Add the new active Connection to the table if there are remapping rules that do not require a specific source IPv4 address
        auto * const pOldConnection = addNewActiveConnectionToRemoteProxyIfNone_Impl (ui32RemoteProxyUniqueID, 0,
                                                                                      ui32RemoteInterfaceIPv4Address,
                                                                                      pConnectionToRemoteProxy);
        if (pOldConnection && (pOldConnection != pConnectionToRemoteProxy)) {
            // Another active Connection was found when trying to add a newly opened Connection; this should NEVER happen
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone", NOMADSUtil::Logger::L_Warning,
                            "the remoteProxyConnectivityTable already contains a ConnectivitySolutions instance with an "
                            "active Connection to the remote NetProxy with UniqueID %u and remote IPv4 address %s; the "
                            "NetProxy did not substitute it with the newly opened Connection, which will still be used "
                            "for remapping rules that require traffic to go out from the local IPv4 address %s\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString(),
                            pConnectionToRemoteProxy->getLocalInterfaceInetAddr()->getIPAsString());
        }

        return pConnectionToReturn;
    }

    // This method does not check that the RemoteProxyConnectivityTable already contains the entry for the passed in arguments
    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxy_Impl (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                    uint32 ui32RemoteInterfaceIPv4Address, Connection * const pConnectionToRemoteProxy)
    {
        if (_umRemoteProxyInfoTable.count (ui32RemoteProxyUniqueID) == 0) {
            // The remote NetProxy UniqueID was updated while the connection was being established --> return nullptr
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxy_Impl", NOMADSUtil::Logger::L_Warning,
                            "impossible to find a valid RemoteProxyInfo instance associated to the NetProxy with UniqueID %u "
                            "and IPv4 address %s; was the remote NetProxy UID updated by an InitializeConnection ProxyMessage?\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
            return nullptr;
        }

        const auto & spRPI = _umRemoteProxyInfoTable.at (ui32RemoteProxyUniqueID);
        if (!spRPI) {
            // The remote NetProxy UniqueID was updated while the connection was being established --> return nullptr
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxy_Impl", NOMADSUtil::Logger::L_Warning,
                            "found a shared_ptr referencing a null RemoteProxyInfo associated to the NetProxy with UniqueID %u "
                            "and IPv4 address %s; was the remote NetProxy UID updated by an InitializeConnection ProxyMessage?\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
            return nullptr;
        }

        auto & spConnectivitySolutions = _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID][{ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address}];
        if (!spConnectivitySolutions) {
            // Create a new ConnectivitySolutions to hold the Connection
            spConnectivitySolutions =
                std::make_shared<ConnectivitySolutions> (ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address, pConnectionToRemoteProxy, spRPI);
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxy_Impl", NOMADSUtil::Logger::L_Info,
                            "added a new ConnectivitySolutions to the RemoteProxyConnectivityTable to hold connection solutions "
                            "to the remote NetProxy with UniqueID %u, local IPv4 address %s, and remote IPv4 address %s\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
        }

        // Updating this object will in turn update all the entries in the addressMappingList that point to it
        checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxy_Impl", NOMADSUtil::Logger::L_Info,
                        "adding a new %s ActiveConnection to the ConnectivitySolutions that holds solutions to connect "
                        "to the remote NetProxy with UniqueID %u, local IPv4 address %s, and remote IPv4 address %s\n",
                        pConnectionToRemoteProxy->getConnectorTypeAsString(), ui32RemoteProxyUniqueID,
                        NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                        NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
        return spConnectivitySolutions->setActiveConnection (pConnectionToRemoteProxy);
    }

    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone (Connection * const pConnectionToRemoteProxy)
    {
        if (!pConnectionToRemoteProxy || (pConnectionToRemoteProxy->getRemoteNetProxyID() == 0)) {
            return nullptr;
        }

        return addNewActiveConnectionToRemoteProxyIfNone (pConnectionToRemoteProxy->getRemoteNetProxyID(),
                                                          pConnectionToRemoteProxy->getLocalInterfaceInetAddr()->getIPAddress(),
                                                          pConnectionToRemoteProxy->getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                          pConnectionToRemoteProxy);
    }

    // This method assumes that the lock() on _mtx is held by the caller
    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                     uint32 ui32RemoteInterfaceIPv4Address,
                                                                                     Connection * const pConnectionToRemoteProxy)
    {
        // Add new active Connection to the table for remapping rules that require the local IPv4 address used by the new Connection
        auto pConnectionToReturn = addNewActiveConnectionToRemoteProxyIfNone_Impl (ui32RemoteProxyUniqueID, ui32LocalInterfaceIPv4Address,
                                                                                   ui32RemoteInterfaceIPv4Address, pConnectionToRemoteProxy);
        if (pConnectionToReturn && (pConnectionToReturn != pConnectionToRemoteProxy)) {
            // Another active Connection was found when trying to add a newly opened Connection; this should NEVER happen
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone", NOMADSUtil::Logger::L_SevereError,
                            "the remoteProxyConnectivityTable already contains a ConnectivitySolutions instance with an active "
                            "%s Connection to the remote NetProxy with UniqueID %u, local IPv4 address %s, and remote IPv4 "
                            "address %s; the NetProxy did not replace it with the newly opened Connection\n",
                            pConnectionToRemoteProxy->getConnectorTypeAsString(), ui32RemoteProxyUniqueID,
                            NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
            return pConnectionToReturn;
        }

        // Add the new active Connection to the table if there are remapping rules that do not require a specific IPv4 address
        auto * const pOldConnection = addNewActiveConnectionToRemoteProxyIfNone_Impl (ui32RemoteProxyUniqueID, 0,
                                                                                      ui32RemoteInterfaceIPv4Address,
                                                                                      pConnectionToRemoteProxy);
        if (pOldConnection && (pOldConnection != pConnectionToRemoteProxy)) {
            // Another active Connection was found when trying to add a newly opened Connection; this should NEVER happen
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone", NOMADSUtil::Logger::L_LowDetailDebug,
                            "the remoteProxyConnectivityTable already contains a ConnectivitySolutions instance with "
                            "an active Connection to the remote NetProxy with UniqueID %u and remote IPv4 address %s; "
                            "the NetProxy did not substitute it with the newly opened Connection, which will still be "
                            "used for remapping rules that require traffic to go out from the local IPv4 address %s\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString(),
                            pConnectionToRemoteProxy->getLocalInterfaceInetAddr()->getIPAsString());
        }
        else {
            // Check if we need to update
            pConnectionToReturn = pConnectionToReturn ? pConnectionToReturn : pOldConnection;
        }

        if (!pConnectionToReturn) {
            // This method could not do anything!
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone", NOMADSUtil::Logger::L_Warning,
                            "the NetProxy could not add the new active Connection to the remote NetProxy UniqueID %u, "
                            "local IPv4 address %s, and remote IPv4 address %s neither to the ConnectivitySolutions "
                            "specific for the local IPv4 address to which the new connection is bound nor to the general "
                            "ConnectivitySolutions to the specified remote IPv4 address\n", ui32RemoteProxyUniqueID,
                            NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
        }

        return pConnectionToReturn;
    }

    // This method does not check if an entry in the table exists for the specified arguments
    Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone_Impl (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                          uint32 ui32RemoteInterfaceIPv4Address,
                                                                                          Connection * const pConnectionToRemoteProxy)
    {
        if (_umRemoteProxyInfoTable.count (ui32RemoteProxyUniqueID) == 0) {
            // The remote NetProxy UniqueID was updated while the connection was being established --> return nullptr
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone_Impl", NOMADSUtil::Logger::L_Warning,
                            "impossible to find a valid RemoteProxyInfo instance associated to the NetProxy with UniqueID %u "
                            "and IPv4 address %s; was the remote NetProxy UID updated by an InitializeConnection ProxyMessage?\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
            return nullptr;
        }

        const auto & spRPI = _umRemoteProxyInfoTable.at (ui32RemoteProxyUniqueID);
        if (!spRPI) {
            // The remote NetProxy UniqueID was updated while the connection was being established --> return nullptr
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone_Impl", NOMADSUtil::Logger::L_Warning,
                            "found a shared_ptr referencing a null RemoteProxyInfo associated to the NetProxy with UniqueID %u "
                            "and IPv4 address %s; was the remote NetProxy UID updated by an InitializeConnection ProxyMessage?\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
            return nullptr;
        }

        // Retrieve the ConnectivitySolutions instance associated to the new Connection, creating it if missing
        auto & spConnectivitySolutions = _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID][{ui32LocalInterfaceIPv4Address,
                                                                                                   ui32RemoteInterfaceIPv4Address}];
        if (!spConnectivitySolutions) {
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone_Impl", NOMADSUtil::Logger::L_Info,
                            "added a new ConnectivitySolutions to the RemoteProxyConnectivityTable to hold connection solutions "
                            "to the remote NetProxy with UniqueID %u, local IPv4 address %s, and remote IPv4 address %s\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString());
            spConnectivitySolutions =
                std::make_shared<ConnectivitySolutions> (ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address, spRPI);
        }

        /* ConnectivitySolutions to reach the remote NetProxy already in the table.
         * Updating this object will in turn update all the entries in the RemoteHostMappingTable that point to it. */
        auto * const pOldConnection =
            spConnectivitySolutions->getActiveConnectionForConnectorAndEncryptionType (pConnectionToRemoteProxy->getConnectorType(),
                                                                                       pConnectionToRemoteProxy->getEncryptionType());
        if (pOldConnection && (pOldConnection != pConnectionToRemoteProxy)) {
            // Connection active Connection already in the ConnectivitySolutions
            const auto * const pLocalInetAddr = pOldConnection->getLocalInterfaceInetAddr();
            const auto * const pRemoteInetAddr = pOldConnection->getRemoteInterfaceLocalInetAddr();
            checkAndLogMsg ("ConnectionManager::addNewActiveConnectionToRemoteProxyIfNone_Impl", NOMADSUtil::Logger::L_LowDetailDebug,
                            "skipping adding a new ActiveConnection to the ConnectivitySolutions that hold connection solutions to "
                            "reach the remote NetProxy with UniqueID %u, local IPv4 address %s, and remote IPv4 address %s as "
                            "another ActiveConnection is already present (local endpoint <%s:%hu> - remote endpoint <%s:%hu>)\n",
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32LocalInterfaceIPv4Address}.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString(), pLocalInetAddr->getIPAsString(),
                            pLocalInetAddr->getPort(), pRemoteInetAddr->getIPAsString(), pRemoteInetAddr->getPort());
            return pOldConnection;
        }

        if (!pOldConnection) {
            // Add new Connection to the ConnectivitySolutions
            spConnectivitySolutions->setActiveConnection (pConnectionToRemoteProxy);
        }

        return pConnectionToRemoteProxy;
    }

    Connection * const ConnectionManager::switchActiveConnectionInRemoteProxyConnectivityTable_NoLock (uint32 ui32RemoteProxyUniqueID,
                                                                                                       uint32 ui32LocalInterfaceIPv4Address,
                                                                                                       uint32 ui32RemoteInterfaceIPv4Address,
                                                                                                       Connection * const pClosedConnection,
                                                                                                       Connection * const pNewConnection) const
    {
        auto & spConnectivitySolutions = _umRemoteProxyConnectivityTable[ui32RemoteProxyUniqueID][{ui32LocalInterfaceIPv4Address,
                                                                                                   ui32RemoteInterfaceIPv4Address}];
        if (spConnectivitySolutions) {
            const auto pActiveConnectionInTable =
                spConnectivitySolutions->getActiveConnectionForConnectorAndEncryptionType (pClosedConnection->getConnectorType(),
                                                                                           pClosedConnection->getEncryptionType());
            if (!pActiveConnectionInTable || (pActiveConnectionInTable == pClosedConnection)) {
                return spConnectivitySolutions->setActiveConnection (pNewConnection);
            }
            else {
                checkAndLogMsg ("ConnectionManager::switchActiveConnectionInRemoteProxyConnectivityTable", NOMADSUtil::Logger::L_MildError,
                                "the spConnectivitySolutions instance in the RemoteProxyConnectivityTable accessed by the remote NetProxy UniqueID %u "
                                "and remote interface IPv4 address %s contains a pointer to an unexpected Connection with remote NetProxy UniqueID %u, "
                                "address <%s:%hu>, connector type %s, and encryption type %s; the local NetProxy could not switch that Connection "
                                "instance with the new Connection to the remote NetProxy with UniqueID %u and interface IPv4 address %s\n",
                                ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32RemoteInterfaceIPv4Address}.getIPAsString(),
                                pActiveConnectionInTable->getRemoteNetProxyID(), pActiveConnectionInTable->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                pActiveConnectionInTable->getRemoteInterfaceLocalInetAddr()->getPort(), pActiveConnectionInTable->getConnectorTypeAsString(),
                                pActiveConnectionInTable->getEncryptionTypeAsString(), pNewConnection->getRemoteNetProxyID(),
                                pNewConnection->getRemoteInterfaceLocalInetAddr()->getIPAsString());

                return pActiveConnectionInTable;
            }
        }

        return nullptr;
    }

    Connection * const ConnectionManager::removeActiveConnectionFromRemoteProxyConnectivityTable_NoLock (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                                         uint32 ui32RemoteInterfaceIPv4Address,
                                                                                                         const Connection * const pClosedConnection) const
    {
        Connection * pConnectionToReturn = nullptr;

        // Remove Connection from the ConnectivitySolutions with matching local and remote IPv4 addresses
        if ((_umRemoteProxyConnectivityTable.count (ui32RemoteProxyUniqueID) > 0) &&
            _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).count ({ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address}) > 0) {
            auto & spConnectivitySolutions = _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).at ({ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address});
            pConnectionToReturn = spConnectivitySolutions ? spConnectivitySolutions->removeActiveConnection (pClosedConnection) : pConnectionToReturn;
        }

        // Remove Connection from the ConnectivitySolutions with matching remote IPv4 address
        if ((_umRemoteProxyConnectivityTable.count (ui32RemoteProxyUniqueID) > 0) &&
            _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).count ({0, ui32RemoteInterfaceIPv4Address}) > 0) {
            auto & spConnectivitySolutions = _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).at ({0, ui32RemoteInterfaceIPv4Address});
            auto * const pGeneralConnection = spConnectivitySolutions ? spConnectivitySolutions->removeActiveConnection (pClosedConnection) : nullptr;
            pConnectionToReturn = pConnectionToReturn ? pConnectionToReturn : pGeneralConnection;
        }

        return pConnectionToReturn;
    }

    void ConnectionManager::resetAutoConnectionInstances_NoLock (Connection * const pConnectionToDelete)
    {
        if (!pConnectionToDelete || (pConnectionToDelete->getConnectorType() == CT_UDPSOCKET) ||
            (pConnectionToDelete->getConnectorType() == CT_UNDEF)) {
            // Nothing to do
            return;
        }

        auto spAutoConnectionEntryToReset =
            getAutoConnectionEntryToRemoteProxyWithID_NoLock (pConnectionToDelete->getRemoteNetProxyID(),
                                                              pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                              pConnectionToDelete->getConnectorType());

        if (spAutoConnectionEntryToReset) {
            // Reset the autoConnectionEntry linked to the Connection to delete
            spAutoConnectionEntryToReset->resetAutoConnectionEntry (pConnectionToDelete);
        }
    }

    void ConnectionManager::updateRemoteProxyInfo (std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo, uint32 ui32RemoteProxyUniqueID,
                                                   const std::vector<uint32> & vui32InterfaceIPv4AddressList,
                                                   uint16 ui16MocketsServerPort, uint16 ui16TCPServerPort,
                                                   uint16 ui16UDPServerPort, bool bRemoteProxyReachability)
    {
        // Update all attributes
        if (!spRemoteProxyInfo) {
            spRemoteProxyInfo.reset (new RemoteProxyInfo{});
        }
        spRemoteProxyInfo->setRemoteProxyID (ui32RemoteProxyUniqueID);
        spRemoteProxyInfo->setRemoteServerAddr (vui32InterfaceIPv4AddressList, ui16MocketsServerPort, ui16TCPServerPort, ui16UDPServerPort);
        spRemoteProxyInfo->setRemoteProxyReachabilityFromLocalHost (bRemoteProxyReachability);
    }

    /* Reset all entries whose cached Connection points to pConnectionToDelete.
     * This method assumes that the caller has acquired the lock on the TCPConnTable instance. */
    void ConnectionManager::resetTCPConnectionEntriesWithConnection_NoLock (TCPConnTable & rTCPConnTable, Connection * const pConnectionToDelete)
    {
        Entry * pEntry = nullptr;
        rTCPConnTable.resetGet();
        while (pEntry = rTCPConnTable.getNextEntry()) {
            std::lock_guard<std::mutex> lg{pEntry->getMutexRef()};
            if (pEntry->getConnection() == pConnectionToDelete) {
                pEntry->reset();
                checkAndLogMsg ("Connector::prepareForDelete", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "removed the Connection instance with remote NetProxy at address <%s:%hu> "
                                "and UniqueID %u from the entry L%hu-R%hu in the TCPConnTable\n",
                                pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                pConnectionToDelete->getRemoteInterfaceLocalInetAddr()->getPort(),
                                pConnectionToDelete->getRemoteNetProxyID(), pEntry->ui16ID, pEntry->ui16RemoteID);
            }
        }
    }
}
