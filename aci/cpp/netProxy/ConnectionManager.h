#ifndef INCL_CONNECTION_MANAGER_H
#define INCL_CONNECTION_MANAGER_H

/*
 * ConnectionManager.h
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
 *
 * Class that handles all active connections and the mapping between the
 * IP address of remote hosts and that of the corresponding NetProxy.
 * The ConnectionManager class is a singleton and it provides an interface
 * so that other classes in the NetProxy can retrieve all necessary
 * information to send messages to a specific remote host.
 */

#include <utility>

#include "UInt32Hashtable.h"
#include "UInt64Hashtable.h"
#include "NPDArray2.h"
#include "StrClass.h"
#include "InetAddr.h"

#include "Utilities.h"
#include "AddressRangeDescriptor.h"
#include "QueryResult.h"
#include "ConnectivitySolutions.h"
#include "AutoConnectionEntry.h"
#include "Connection.h"
#include "Connector.h"


namespace ACMNetProxy
{
    class ConnectionManager
    {
    public:
        static ConnectionManager * const getConnectionManagerInstance (void);

        Connector * const getConnectorForType (ConnectorType connectorType) const;
        int registerConnector (Connector * const pConnector);
        Connector * const deregisterConnector (ConnectorType connectorType);

        void clearAllConnectionMappings (void);

        int addNewRemoteProxyInfo (const RemoteProxyInfo &remoteProxyInfo);
        int updateAddressMappingBook (const AddressRangeDescriptor & addressRange, uint32 ui32RemoteProxyID);
        int updateRemoteProxyUniqueID (uint32 ui32RemoteProxyOldID, uint32 ui32RemoteProxyNewID);
        int updateRemoteProxyInfo (uint32 ui32RemoteProxyID, const NOMADSUtil::InetAddr * const pRemoteProxyAddress, uint16 ui16MocketsServerPort,
                                   uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability);
        Connection * const updateRemoteProxyInfo (Connection * const pConnectionToRemoteProxy, uint32 ui32RemoteProxyID, uint16 ui16MocketsServerPort,
                                                  uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability);

        Connection * const addNewActiveConnectionToRemoteProxy (Connection * const pConnectionToRemoteProxy);
        Connection * const addNewActiveConnectionToRemoteProxy (Connection * const pConnectionToRemoteProxy, uint32 ui32RemoteProxyID);
        Connection * const removeActiveConnectionFromTable (const Connection * const pClosedConnection);

        bool isRemoteHostIPMapped (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort = 0) const;
        const QueryResult queryConnectionToRemoteHostForConnectorType (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;
        const NPDArray2<QueryResult> queryAllConnectionsToRemoteHostForConnectorType (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;
        const QueryResult queryConnectionToRemoteProxyIDForConnectorType (ConnectorType connectorType, uint32 ui32RemoteProxyID) const;
        const bool isConnectionToRemoteProxyOpenedForConnector (ConnectorType connectorType, uint32 ui32RemoteProxyID) const;
        const bool isConnectionToRemoteProxyOpeningForConnector (ConnectorType connectorType, uint32 ui32RemoteProxyID) const;

        const uint32 getIPAddressForRemoteProxyWithID (uint32 ui32RemoteProxyID) const;
        const char * const getIPAddressAsStringForProxyWithID (uint32 ui32RemoteProxyID) const;
        const NOMADSUtil::InetAddr * getInetAddressForProtocolAndProxyWithID (ConnectorType connectorType, uint32 ui32RemoteProxyID) const;
        const bool getReachabilityFromRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;
        const bool getReachabilityFromRemoteProxyWithID (uint32 ui32RemoteProxyID) const;
        const char * const getMocketsConfigFileForProxyWithID (uint32 ui32RemoteProxyID) const;
        const char * const getMocketsConfigFileForConnectionsToRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;

        const NPDArray2<AutoConnectionEntry> * const getAutoConnectionTable (void) const;
        AutoConnectionEntry * const getAutoConnectionEntryToRemoteProxyID (uint32 ui32RemoteProxyID) const;
        void clearAutoConnectionTable (void);
		NOMADSUtil::LList<uint32> *getRemoteProxyAddrList();

    private:
        friend class NetProxyConfigManager;
        friend class AutoConnectionEntry;

        ConnectionManager (void);
        explicit ConnectionManager (const ConnectionManager & connectionManager);
        ~ConnectionManager (void);

        int addNewAddressMappingToBook (const AddressRangeDescriptor & addressRange, uint32 ui32RemoteProxyID);
        int addOrUpdateAutoConnectionToList (const AutoConnectionEntry & autoConnectionEntry);

        bool isAddressAMatchInTheMappingBook (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;
        ConnectivitySolutions * const findConnectivitySolutionsToProxyWithID (uint32 ui32RemoteProxyID) const;
        ConnectivitySolutions * const findConnectivitySolutionsToRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;
        const NOMADSUtil::DArray<const ConnectivitySolutions *> findAllConnectivitySolutionsToRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;
        std::pair<AddressRangeDescriptor, ConnectivitySolutions *> * const findConnectivitySolutionsInTheMappingBook (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const;

        RemoteProxyInfo * const getRemoteProxyInfoForProxyWithID (uint32 ui32RemoteProxyID) const;
        AutoConnectionEntry * const getAutoConnectionEntryForProxyWithID (uint32 ui32RemoteProxyID) const;

        static void updateRemoteProxyInfo (RemoteProxyInfo & currentRemoteProxyInfo, const RemoteProxyInfo & updatedRemoteProxyInfo);
        static void updateRemoteProxyInfo (RemoteProxyInfo & currentRemoteProxyInfo, const NOMADSUtil::InetAddr * const pInetAddr, uint16 ui16MocketsServerPort,
                                           uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability);

        /* A list of all enabled connectors; their index in the list is the integer representation of their ConnectorType */
        NOMADSUtil::DArray<Connector *> _connectorsTable;
        NOMADSUtil::UInt32Hashtable<ConnectivitySolutions> _remoteProxyConnectivityTable;                       // The Key is the 32-bits GUID of the remote NetProxy
        NPDArray2<std::pair<AddressRangeDescriptor, ConnectivitySolutions *> > _remoteHostAddressMappingBook;   // A list of all mapped remote host addresses
        mutable NOMADSUtil::UInt32Hashtable<std::pair<AddressRangeDescriptor, ConnectivitySolutions *> >
            _remoteHostAddressMappingCache;                                                                     // The Key is the 32-bits encoding of the IP address of remote hosts
        NPDArray2<AutoConnectionEntry> _autoConnectionList;                                                     // A list of all AutoConnection instances

        mutable NOMADSUtil::Mutex _m;
    };


    inline ConnectionManager * const ConnectionManager::getConnectionManagerInstance (void)
    {
        static ConnectionManager connectionManagerSingleton;

        return &connectionManagerSingleton;
    }

    inline Connector * const ConnectionManager::getConnectorForType (ConnectorType connectorType) const
    {
        return (static_cast<int> (connectorType) <= _connectorsTable.getHighestIndex()) ? _connectorsTable.get (static_cast<int> (connectorType)) : NULL;
    }

    inline int ConnectionManager::registerConnector (Connector * const pConnector)
    {
        if (!pConnector) {
            return -1;
        }

        _connectorsTable[static_cast<int> (pConnector->getConnectorType())] = pConnector;
        return 0;
    }

    inline Connector * const ConnectionManager::deregisterConnector (ConnectorType connectorType)
    {
        Connector * const pConnector = getConnectorForType (connectorType);
        if (pConnector) {
            _connectorsTable[static_cast<int> (connectorType)] = NULL;
        }

        return pConnector;
    }

    inline Connection * const ConnectionManager::addNewActiveConnectionToRemoteProxy (Connection * const pConnectionToRemoteProxy)
    {
        return addNewActiveConnectionToRemoteProxy (pConnectionToRemoteProxy, pConnectionToRemoteProxy->getRemoteProxyID());
    }

    inline const NOMADSUtil::InetAddr * ConnectionManager::getInetAddressForProtocolAndProxyWithID (ConnectorType connectorType, uint32 ui32RemoteProxyID) const
    {
        const ConnectivitySolutions *pConnectivitySolutions = NULL;
        if (_m.lock() == NOMADSUtil::Mutex::RC_Ok) {
            pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
            _m.unlock();
        }

        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().getRemoteProxyInetAddr (connectorType) : NULL;
    }

    inline const uint32 ConnectionManager::getIPAddressForRemoteProxyWithID (uint32 ui32RemoteProxyID) const
    {
        const ConnectivitySolutions *pConnectivitySolutions = NULL;
        if (_m.lock() == NOMADSUtil::Mutex::RC_Ok) {
            pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
            _m.unlock();
        }

        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().getRemoteProxyInetAddr (CT_MOCKETS)->getIPAddress() : 0;
    }

    inline const char * const ConnectionManager::getIPAddressAsStringForProxyWithID (uint32 ui32RemoteProxyID) const
    {
        const ConnectivitySolutions *pConnectivitySolutions = NULL;
        if (_m.lock() == NOMADSUtil::Mutex::RC_Ok) {
            pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
            _m.unlock();
        }

        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().getRemoteProxyInetAddr (CT_MOCKETS)->getIPAsString() : "";
    }

    inline const bool ConnectionManager::getReachabilityFromRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const
    {
        const ConnectivitySolutions * const pConnectivitySolutions = findConnectivitySolutionsToRemoteHost (ui32RemoteHostIP, ui16RemoteHostPort);

        return pConnectivitySolutions ? pConnectivitySolutions->_remoteProxyInfo.isLocalProxyReachableFromRemote() :
            NetProxyApplicationParameters::DEFAULT_LOCAL_PROXY_REACHABILITY_FROM_REMOTE;
    }

    inline const bool ConnectionManager::getReachabilityFromRemoteProxyWithID (uint32 ui32RemoteProxyID) const
    {
        const ConnectivitySolutions *pConnectivitySolutions = NULL;
        if (_m.lock() == NOMADSUtil::Mutex::RC_Ok) {
            pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
            _m.unlock();
        }

        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().isLocalProxyReachableFromRemote() :
            NetProxyApplicationParameters::DEFAULT_LOCAL_PROXY_REACHABILITY_FROM_REMOTE;
    }

    inline const char * const ConnectionManager::getMocketsConfigFileForProxyWithID (uint32 ui32RemoteProxyID) const
    {
        const ConnectivitySolutions *pConnectivitySolutions = NULL;
        if (_m.lock() == NOMADSUtil::Mutex::RC_Ok) {
            pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
            _m.unlock();
        }

        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().getMocketsConfFileName() : "";
    }

    inline const char * const ConnectionManager::getMocketsConfigFileForConnectionsToRemoteHost (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const
    {
        const ConnectivitySolutions * const pConnectivitySolutions = findConnectivitySolutionsToRemoteHost (ui32RemoteHostIP, ui16RemoteHostPort);

        return pConnectivitySolutions ? pConnectivitySolutions->getRemoteProxyInfo().getMocketsConfFileName() : "";
    }

    inline const NPDArray2<AutoConnectionEntry> * const ConnectionManager::getAutoConnectionTable (void) const
	{
        return &_autoConnectionList;
    }

    inline void ConnectionManager::clearAutoConnectionTable (void)
    {
        _autoConnectionList.trimSize (0);
    }

    inline ConnectionManager::ConnectionManager (void) : _remoteProxyConnectivityTable (true) { }

    inline ConnectionManager::~ConnectionManager (void)
    {
        clearAllConnectionMappings();
    }

    inline bool ConnectionManager::isAddressAMatchInTheMappingBook (uint32 ui32RemoteHostIP, uint16 ui16RemoteHostPort) const
    {
        return findConnectivitySolutionsInTheMappingBook (ui32RemoteHostIP, ui16RemoteHostPort) != NULL;
    }

    inline ConnectivitySolutions * const ConnectionManager::findConnectivitySolutionsToProxyWithID (uint32 ui32RemoteProxyID) const
    {
        ConnectivitySolutions *pConnectivitySolutions = NULL;
        if (_m.lock() == NOMADSUtil::Mutex::RC_Ok) {
            pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
            _m.unlock();
        }

        return pConnectivitySolutions;
    }

    inline RemoteProxyInfo * const ConnectionManager::getRemoteProxyInfoForProxyWithID (uint32 ui32RemoteProxyID) const
    {
        ConnectivitySolutions *pConnectivitySolutions = NULL;
        if (_m.lock() == NOMADSUtil::Mutex::RC_Ok) {
            pConnectivitySolutions = _remoteProxyConnectivityTable.get (ui32RemoteProxyID);
            _m.unlock();
        }

        return pConnectivitySolutions ? &(pConnectivitySolutions->_remoteProxyInfo) : NULL;
    }

}

#endif  // INCL_CONNECTION_MANAGER_H
