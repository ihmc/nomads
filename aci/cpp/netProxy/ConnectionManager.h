#ifndef INCL_CONNECTION_MANAGER_H
#define INCL_CONNECTION_MANAGER_H

/*
 * ConnectionManager.h
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
 * Class that handles all active connections and the mapping between the
 * IP address of remote hosts and that of the corresponding NetProxy.
 * The ConnectionManager class is a singleton and it provides an interface
 * so that other classes in the NetProxy can retrieve all necessary
 * information to send messages to a specific remote host.
 */

#include <mutex>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "InetAddr.h"

#include "TuplesHashFunction.h"
#include "NetworkAddressRange.h"
#include "QueryResult.h"
#include "ConnectivitySolutions.h"
#include "AutoConnectionEntry.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class RemoteProxyInfo;
    class TCPConnTable;
    class Connection;
    class Connector;
    class PacketRouter;


    class ConnectionManager
    {
    public:
        ConnectionManager (void);
        explicit ConnectionManager (const ConnectionManager & connectionManager) = delete;
        ~ConnectionManager (void);

        // Connectors
        Connector * const getConnectorBoundToAddressForType (uint32 ui32LocalIPv4Address, ConnectorType connectorType) const;
        int registerConnector (const std::shared_ptr <Connector> && spConnector);
        int deregisterConnector (uint32 ui32LocalIPv4Address, ConnectorType connectorType);
        unsigned int getNumberOfConnectors (void) const;

        // RemoteProxyInfos
        int addNewRemoteProxyInfo (const RemoteProxyInfo & remoteProxyInfo);
        int addNewRemoteProxyInfo (RemoteProxyInfo && remoteProxyInfo);
        int updateRemoteProxyInfo (Connection * const pConnectionToRemoteProxy, const uint32 ui32RemoteProxyUniqueID,
                                   const std::vector<uint32> & vui32InterfaceIPv4AddressList, uint16 ui16MocketsServerPort,
                                   uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability);
        uint32 findUniqueIDOfRemoteNetProxyWithMainIPv4Address (uint32 ui32IPv4Address) const;
        uint32 findUniqueIDOfRemoteNetProxyWithIPAddress (uint32 ui32InterfaceIPv4Address) const;
        const RemoteProxyInfo * const getRemoteProxyInfoForNetProxyWithID (uint32 ui32RemoteProxyUniqueID) const;
        uint32 findBestMatchingUniqueIDOfRemoteNetProxyWithInterfaceIPs (const std::vector<uint32> & vui32InterfaceIPv4AddressList,
                                                                         bool & bClashingConfiguration) const;

        // Remapping rules
        Connection * const addNewActiveConnectionToRemoteProxy (Connection * const pConnectionToRemoteProxy);
        Connection * const removeActiveConnectionFromRemoteProxyConnectivityTable (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                   uint32 ui32RemoteInterfaceIPv4Address,
                                                                                   const Connection * const pClosedConnection);
        void prepareForDelete (TCPConnTable & rTCPConnTable, Connection * const pConnectionToDelete,
                               Connection * const pConnectionToSubstitute = nullptr);

        int addNewAddressMappingToBook (const std::pair<NetworkAddressRange, NetworkAddressRange> & pardardMappingFilter,
                                        const std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr> & tui32iaiaMappingRule);
        void finalizeMappingRules (void);
        void clearAllConnectionMappings (void);

        bool isFlowPotentiallyMapped (uint32 ui32SourceIP, uint32 ui32DestinationIP) const;
        bool isFlowMapped (uint32 ui32SourceIP, uint16 ui16SourcePort, uint32 ui32DestinationIP, uint16 ui16DestinationPort) const;
        bool isFlowMapped (uint32 ui32SourceIP, uint32 ui32DestinationIP, uint16 ui16SourcePort, uint16 ui16DestinationPort) const = delete;
        const QueryResult queryConnectionToRemoteHostForConnectorType (uint32 ui32SourceIPAddress, uint32 ui32DestinationIPAddress,
                                                                       ConnectorType connectorType, EncryptionType encryptionType) const;
        const QueryResult queryConnectionToRemoteHostForConnectorType (uint32 ui32SourceIPAddress, uint16 ui16SourcePort,
                                                                       uint32 ui32DestinationIPAddress, uint16 ui16DestinationPort,
                                                                       ConnectorType connectorType, EncryptionType encryptionType) const;
        const std::vector<QueryResult> queryAllConnectivitySolutionsToMulticastAddressForConnectorType (uint32 ui32SourceIP, uint16 ui16SourcePort,
                                                                                                        uint32 ui32DestinationIP, uint16 ui16DestinationPort,
                                                                                                        ConnectorType connectorType,
                                                                                                        EncryptionType encryptionType) const;
        const QueryResult queryConnectionToRemoteProxyIDAndInterfaceIPv4AddressForConnectorTypeAndEncryptionType (uint32 ui32RemoteProxyUniqueID,
                                                                                                                  uint32 ui32LocalInterfaceIPv4Address,
                                                                                                                  uint32 ui32RemoteInterfaceIPv4Address,
                                                                                                                  ConnectorType connectorType,
                                                                                                                  EncryptionType encryptionType) const;
        ConnectivitySolutions * const
            findConnectivitySolutionsToNetProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                     uint32 ui32RemoteInterfaceIPv4Address) const;
        std::unordered_set<ConnectivitySolutions *>
            findAllConnectivitySolutionsToNetProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteInterfaceIPv4Address) const;
        std::vector<const ConnectivitySolutions *>
            findConnectivitySolutionsForRemappedMulticastAddress (uint32 ui32SourceIP, uint16 ui16SourcePort,
                                                                  uint32 ui32DestinationIP, uint16 ui16DestinationPort) const;
        std::vector<ConnectivitySolutions *> getAllConnectivitySolutions (void) const;

        // Connections
        bool isConnectionToRemoteProxyOpenedForConnector (uint32 ui32RemoteProxyUniqueID, ConnectorType connectorType,
                                                                EncryptionType encryptionType) const;
        bool isConnectionToRemoteProxyOpeningForConnector (uint32 ui32RemoteProxyUniqueID, ConnectorType connectorType,
                                                                 EncryptionType encryptionType) const;

        bool getReachabilityFromRemoteProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                       uint32 ui32RemoteInterfaceIPv4Address) const;
        const char * const getMocketsConfigFileForProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                             uint32 ui32RemoteInterfaceIPv4Address) const;

        // AutoConnections
        int addOrUpdateAutoConnectionToList (const AutoConnectionEntry & autoConnectionEntry);
        void updateRemoteProxyIDForAutoConnectionWithID (uint32 ui32OldRemoteProxyID, uint32 ui32NewRemoteProxyID);
        void updateAutoConnectionEntries (void);
        std::shared_ptr<AutoConnectionEntry> getAutoConnectionEntryToRemoteProxyWithID (uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteInterfaceIPv4Address,
                                                                                        ConnectorType ct) const;
        const std::unordered_set<std::shared_ptr<AutoConnectionEntry>> getSetOfAutoConnectionEntriesToRemoteProxyWithID (uint32 ui32RemoteProxyUniqueID) const;
        std::unordered_map<uint32, std::unordered_set<std::pair<uint32, uint8>>> getAutoConnectionKeysMap (void) const;
        unsigned int getNumberOfValidAutoConnectionEntries (void) const;
        void clearAutoConnectionTable (void);

        int getNumberOfOpenConnections (void);

        /*
        * This method returns a list containing the statically configured remote NetProxies IPs
        */
        std::unordered_set<uint32> getRemoteProxyAddrList (void) const;

        static std::vector<uint32> getListOfLocalInterfacesIPv4Addresses (void);

    private:
        friend class Connection;


        Connection * const addNewActiveConnectionToRemoteProxy_NoLock (Connection * const pConnectionToRemoteProxy);
        Connection * const addNewActiveConnectionToRemoteProxy_Impl (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                     uint32 ui32RemoteInterfaceIPv4Address, Connection * const pConnectionToRemoteProxy);
        Connection * const addNewActiveConnectionToRemoteProxyIfNone (Connection * const pConnectionToRemoteProxy);
        Connection * const addNewActiveConnectionToRemoteProxyIfNone (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                      uint32 ui32RemoteInterfaceIPv4Address, Connection * const pConnectionToRemoteProxy);
        Connection * const addNewActiveConnectionToRemoteProxyIfNone_Impl (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                           uint32 ui32RemoteInterfaceIPv4Address, Connection * const pConnectionToRemoteProxy);
        Connection * const switchActiveConnectionInRemoteProxyConnectivityTable_NoLock (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                        uint32 ui32RemoteInterfaceIPv4Address, Connection * const pClosedConnection,
                                                                                        Connection * const pNewConnection) const;
        Connection * const removeActiveConnectionFromRemoteProxyConnectivityTable_NoLock (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                          uint32 ui32RemoteInterfaceIPv4Address,
                                                                                          const Connection * const pClosedConnection) const;
        void resetAutoConnectionInstances_NoLock (Connection * const pConnectionToDelete);

        bool isMulticastAddressPotentiallyMapped (uint32 ui32SourceIP, uint32 ui32DestinationIP) const;
        bool isMulticastAddressMapped (uint32 ui32SourceIP, uint16 ui16SourcePort, uint32 ui32DestinationIP, uint16 ui16DestinationPort) const;
        bool isMulticastAddressMapped (uint32 ui32SourceIP, uint32 ui32DestinationIP, uint16 ui16SourcePort, uint16 ui16DestinationPort) const = delete;
        ConnectivitySolutions * const findMappedConnectivitySolutions (uint32 ui32SourceIP, uint32 ui32DestinationIP) const;
        ConnectivitySolutions * const findMappedConnectivitySolutions (uint32 ui32SourceIP, uint16 ui16SourcePort,
                                                                       uint32 ui32DestinationIP, uint16 ui16DestinationPort) const;
        const bool getReachabilityFromRemoteProxyWithIDAndIPv4Address_NoLock (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                              uint32 ui32RemoteInterfaceIPv4Address) const;

        std::shared_ptr<AutoConnectionEntry> getAutoConnectionEntryToRemoteProxyWithID_NoLock (uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteInterfaceIPv4Address,
                                                                                               ConnectorType ct) const;

        static void updateRemoteProxyInfo (std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo, uint32 ui32RemoteProxyUniqueID,
                                           const std::vector<uint32> & vui32InterfaceIPv4AddressList,
                                           uint16 ui16MocketsServerPort, uint16 ui16TCPServerPort,
                                           uint16 ui16UDPServerPort, bool bRemoteProxyReachability);
        static void resetTCPConnectionEntriesWithConnection_NoLock (TCPConnTable & rTCPConnTable, Connection * const pConnectionToDelete);

        // A map of all connectors, keyed with the 32-bit representation of the IPv4 address they are bound to and the 8-bit representation of their ConnectorType
        std::unordered_map<uint32, std::unordered_map<uint8, std::shared_ptr<Connector>>> _umConnectorsTable;
        // Keys are the remote NetProxy UniqueID and the pair <LocalIPv4Address, RemoteIPv4Address>, respectively
        mutable std::unordered_map<uint32, std::unordered_map<std::pair<uint32, uint32>, std::shared_ptr<ConnectivitySolutions>>>
            _umRemoteProxyConnectivityTable;
        // Key is the 32-bits UniqueID of the remote NetProxy
        mutable std::unordered_map<uint32, std::shared_ptr<RemoteProxyInfo>> _umRemoteProxyInfoTable;
        std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::shared_ptr<ConnectivitySolutions>>>
            _addressMappingList;
        // List of all mappings to multi/broad-cast addresses
        std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::shared_ptr<ConnectivitySolutions>>>
            _multiBroadCastAddressMappingList;
        // Map that organizes all AutoConnectionEntry instances
        std::unordered_map<uint32, std::unordered_map<uint32, std::unordered_map<uint8, std::shared_ptr<AutoConnectionEntry>>>>
            _umAutoConnectionTable;

        mutable std::mutex _mtx;
        mutable std::mutex _mtxAutoConnectionTable;
    };


    inline ConnectionManager::ConnectionManager (void) { }

    inline ConnectionManager::~ConnectionManager (void) { }

    inline Connector * const ConnectionManager::getConnectorBoundToAddressForType (uint32 ui32LocalIPv4Address, ConnectorType connectorType) const
    {
        return ((_umConnectorsTable.count (ui32LocalIPv4Address) > 0) &&
            (_umConnectorsTable.at (ui32LocalIPv4Address).count (static_cast<uint8> (connectorType)) > 0)) ?
            _umConnectorsTable.at (ui32LocalIPv4Address).at (static_cast<uint8> (connectorType)).get() : nullptr;
    }

    inline int ConnectionManager::deregisterConnector (uint32 ui32LocalIPv4Address, ConnectorType connectorType)
    {
        std::lock_guard<std::mutex> lg{_mtx};
        if (getConnectorBoundToAddressForType (ui32LocalIPv4Address, connectorType) != nullptr) {
            _umConnectorsTable[ui32LocalIPv4Address].erase (static_cast<uint8> (connectorType));
            return 0;
        }

        return -1;
    }

    inline unsigned int ConnectionManager::getNumberOfConnectors (void) const
    {
        unsigned int uiRes = 0;

        std::lock_guard<std::mutex> lg{_mtx};
        for (const auto & u32mEntry : _umConnectorsTable) {
            std::for_each (u32mEntry.second.cbegin(), u32mEntry.second.cend(),
                           [&uiRes] (const std::pair<uint8, std::shared_ptr<Connector>> & pucsp) {
                uiRes += pucsp.second ? 1 : 0;
            });
        }

        return uiRes;
    }

    inline uint32 ConnectionManager::findUniqueIDOfRemoteNetProxyWithMainIPv4Address (uint32 ui32IPv4Address) const
    {
        for (const auto & pui32sp : _umRemoteProxyInfoTable) {
            if (pui32sp.second->getRemoteProxyMainInetAddr().getIPAddress() == ui32IPv4Address) {
                return pui32sp.first;
            }
        }

        return 0;
    }

    inline const RemoteProxyInfo * const ConnectionManager::getRemoteProxyInfoForNetProxyWithID (uint32 ui32RemoteProxyUniqueID) const
    {
        return _umRemoteProxyInfoTable.count (ui32RemoteProxyUniqueID) > 0 ?
            _umRemoteProxyInfoTable.at (ui32RemoteProxyUniqueID).get() : nullptr;
    }

    inline ConnectivitySolutions * const
        ConnectionManager::findConnectivitySolutionsToNetProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                    uint32 ui32RemoteInterfaceIPv4Address) const
    {
        std::lock_guard<std::mutex> lg (_mtx);
        if ((_umRemoteProxyConnectivityTable.count (ui32RemoteProxyUniqueID) == 0) ||
            _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).count ({ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address}) == 0) {
            return nullptr;
        }

        return _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).at ({ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address}).get();
    }

    inline bool ConnectionManager::getReachabilityFromRemoteProxyWithIDAndIPv4Address (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                       uint32 ui32RemoteInterfaceIPv4Address) const
    {
        std::lock_guard<std::mutex> lg (_mtx);
        return getReachabilityFromRemoteProxyWithIDAndIPv4Address_NoLock (ui32LocalInterfaceIPv4Address, ui32RemoteProxyUniqueID, ui32RemoteInterfaceIPv4Address);
    }

    inline std::shared_ptr<AutoConnectionEntry> ConnectionManager::getAutoConnectionEntryToRemoteProxyWithID (uint32 ui32RemoteProxyUniqueID,
                                                                                                              uint32 ui32RemoteInterfaceIPv4Address,
                                                                                                              ConnectorType ct) const
    {
        std::lock_guard<std::mutex> lg (_mtxAutoConnectionTable);
        return getAutoConnectionEntryToRemoteProxyWithID_NoLock (ui32RemoteProxyUniqueID, ui32RemoteInterfaceIPv4Address, ct);
    }

    inline const std::unordered_set<std::shared_ptr<AutoConnectionEntry>>
        ConnectionManager::getSetOfAutoConnectionEntriesToRemoteProxyWithID (uint32 ui32RemoteProxyUniqueID) const
    {
        std::unordered_set<std::shared_ptr<AutoConnectionEntry>> usRes;
        std::lock_guard<std::mutex> lg (_mtxAutoConnectionTable);
        if (_umAutoConnectionTable.count (ui32RemoteProxyUniqueID) == 0) {
            return usRes;
        }

        for (auto & pui32ctAutoConn : _umAutoConnectionTable.at (ui32RemoteProxyUniqueID)) {
            for (auto & pctAutoConn : pui32ctAutoConn.second) {
                usRes.insert (pctAutoConn.second);
            }
        }
        return usRes;
    }

    inline void ConnectionManager::clearAutoConnectionTable (void)
    {
        std::lock_guard<std::mutex> lg (_mtxAutoConnectionTable);
        _umAutoConnectionTable.clear();
    }

    inline bool ConnectionManager::isMulticastAddressPotentiallyMapped (uint32 ui32SourceIP, uint32 ui32DestinationIP) const
    {
        for (auto & ppADRADRui32 : _multiBroadCastAddressMappingList) {
            if (ppADRADRui32.first.first.matchesAddress (ui32SourceIP) &&
                ppADRADRui32.first.second.matchesAddress (ui32DestinationIP) && ppADRADRui32.second) {
                return true;
            }
        }

        return false;
    }

    inline bool ConnectionManager::isMulticastAddressMapped (uint32 ui32SourceIP, uint16 ui16SourcePort,
                                                             uint32 ui32DestinationIP, uint16 ui16DestinationPort) const
    {
        for (auto & ppADRADRui32 : _multiBroadCastAddressMappingList) {
            if (ppADRADRui32.first.first.matches (ui32SourceIP, ui16SourcePort) &&
                ppADRADRui32.first.second.matches (ui32DestinationIP, ui16DestinationPort) && ppADRADRui32.second) {
                return true;
            }
        }

        return false;
    }

    inline const bool ConnectionManager::getReachabilityFromRemoteProxyWithIDAndIPv4Address_NoLock (uint32 ui32RemoteProxyUniqueID, uint32 ui32LocalInterfaceIPv4Address,
                                                                                                    uint32 ui32RemoteInterfaceIPv4Address) const
    {
        if ((_umRemoteProxyConnectivityTable.count (ui32RemoteProxyUniqueID) == 0) ||
            _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).count ({ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address}) == 0) {
            return NetworkConfigurationSettings::DEFAULT_LOCAL_PROXY_REACHABILITY_FROM_REMOTE;
        }

        auto const pConnectivitySolutions = _umRemoteProxyConnectivityTable.at (ui32RemoteProxyUniqueID).at ({ui32LocalInterfaceIPv4Address,
                                                                                                              ui32RemoteInterfaceIPv4Address}).get();
        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().isLocalProxyReachableFromRemote() :
            NetworkConfigurationSettings::DEFAULT_LOCAL_PROXY_REACHABILITY_FROM_REMOTE;
    }

    inline std::shared_ptr<AutoConnectionEntry> ConnectionManager::getAutoConnectionEntryToRemoteProxyWithID_NoLock (uint32 ui32RemoteProxyUniqueID,
                                                                                                                     uint32 ui32RemoteInterfaceIPv4Address,
                                                                                                                     ConnectorType ct) const
    {
        if ((_umAutoConnectionTable.count (ui32RemoteProxyUniqueID) > 0) &&
            (_umAutoConnectionTable.at (ui32RemoteProxyUniqueID).count (ui32RemoteInterfaceIPv4Address) > 0) &&
            (_umAutoConnectionTable.at (ui32RemoteProxyUniqueID).at (ui32RemoteInterfaceIPv4Address).count (static_cast<uint8> (ct)) > 0)) {
            return _umAutoConnectionTable.at (ui32RemoteProxyUniqueID).at (ui32RemoteInterfaceIPv4Address).at (static_cast<uint8> (ct));
        }

        return nullptr;
    }

}

#endif  // INCL_CONNECTION_MANAGER_H
