#ifndef INCL_CONNECTION_INFO_H
#define INCL_CONNECTION_INFO_H

/*
 * ConnectivitySolutions.h
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
 * Class that gathers all possible connectivity solutions to reach a remote NetProxy.
 * In its current version, the class uses composition to manage all active
 * connections to a given NetProxy (object instance of ActiveConnection),
 * as well as all the necessary information to open new connections
 * to it (object instance of RemoteProxyInfo).
 */

#include <memory>

#include "QueryResult.h"
#include "RemoteProxyInfo.h"
#include "ActiveConnection.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class Connection;


    class ConnectivitySolutions
    {
        /* Class that contains all possible connectivity
         * solutions to reach a remote NetProxy.
         * So far, possible solutions comprehend:
         *      - an ActiveConnection instance, with currently
         *        opened connections to the remote NetProxy;
         *      - a RemoteProxyInfo instance, with the necessary
         *        information to open a new connection to the
         *        remote NetProxy.
         */
    public:
        ConnectivitySolutions (uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                               const std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo);
        ConnectivitySolutions (uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                               Connection * const pActiveConnectionToRemoteProxy,
                               const std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo);

        bool operator== (const ConnectivitySolutions & rhs) const;
        bool operator!= (const ConnectivitySolutions & rhs) const;

        static const ConnectivitySolutions & getInvalidConnectivitySolutions (void);

        const uint32 getRemoteNetProxyID (void) const;
        const uint32 getLocalInterfaceIPv4Address (void) const;
        const uint32 getRemoteInterfaceIPv4Address (void) const;
        const ActiveConnection & getActiveConnections (void) const;
        const RemoteProxyInfo & getRemoteProxyInfo (void) const;
        const unsigned char getEncryptionDescription (ConnectorType connectorType) const;
        Connection * const getActiveConnectionForConnectorAndEncryptionType (ConnectorType connectorType, EncryptionType encryptionType) const;
        const QueryResult getBestConnectionSolutionForConnectorAndEncryptionType (ConnectorType connectorType, EncryptionType encryptionType) const;

        bool hasSolutionToConnectToIPv4Address (uint32 ui32InterfaceIPv4Address) const;

        Connection * const setActiveConnection (Connection * const pConnection);
        Connection * const removeActiveConnection (const Connection * const pConnection);
        void setEncryptionDescription (ConnectorType connectorType, EncryptionType encryptionType);
        void setRemoteProxyInfo (std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo);


    private:
        void addInterfaceIPv4AddressToRemoteProxyInfo (const NOMADSUtil::InetAddr & iaInterfaceIPv4Address);

        uint8 _connEncryptionDescriptor[CT_SIZE];
        const uint32 _ui32LocalInterfaceIPv4Address;
        const uint32 _ui32RemoteInterfaceIPv4Address;
        ActiveConnection _activeConnections;
        std::shared_ptr<RemoteProxyInfo> _spRemoteProxyInfo;
    };


    inline ConnectivitySolutions::ConnectivitySolutions (uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                                                         const std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo) :
        _connEncryptionDescriptor{}, _ui32LocalInterfaceIPv4Address{ui32LocalInterfaceIPv4Address},
        _ui32RemoteInterfaceIPv4Address{ui32RemoteInterfaceIPv4Address}, _spRemoteProxyInfo{spRemoteProxyInfo}
    { }

    inline ConnectivitySolutions::ConnectivitySolutions (uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                                                         Connection * const pActiveConnectionToRemoteProxy,
                                                         const std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo) :
        _connEncryptionDescriptor{}, _ui32LocalInterfaceIPv4Address{ui32LocalInterfaceIPv4Address},
        _ui32RemoteInterfaceIPv4Address{ui32RemoteInterfaceIPv4Address}, _activeConnections{pActiveConnectionToRemoteProxy},
        _spRemoteProxyInfo{spRemoteProxyInfo}
    { }

    inline bool ConnectivitySolutions::operator== (const ConnectivitySolutions & rhs) const
    {
        return (getRemoteNetProxyID() == rhs.getRemoteNetProxyID()) &&
            (getRemoteInterfaceIPv4Address() == rhs.getRemoteInterfaceIPv4Address());
    }

    inline bool ConnectivitySolutions::operator!= (const ConnectivitySolutions & rhs) const
    {
        return !(*this == rhs);
    }

    inline const ConnectivitySolutions & ConnectivitySolutions::getInvalidConnectivitySolutions (void)
    {
        static ConnectivitySolutions INVALID_CONNECTIVITY_SOLUTIONS{0, 0, nullptr};

        return INVALID_CONNECTIVITY_SOLUTIONS;
    }

    inline const uint32 ConnectivitySolutions::getRemoteNetProxyID (void) const
    {
        return _spRemoteProxyInfo->getRemoteNetProxyID();
    }

    inline const uint32 ConnectivitySolutions::getLocalInterfaceIPv4Address (void) const
    {
        return _ui32LocalInterfaceIPv4Address;
    }

    inline const uint32 ConnectivitySolutions::getRemoteInterfaceIPv4Address (void) const
    {
        return _ui32RemoteInterfaceIPv4Address;
    }

    inline const ActiveConnection & ConnectivitySolutions::getActiveConnections (void) const
    {
        return _activeConnections;
    }

    inline const RemoteProxyInfo & ConnectivitySolutions::getRemoteProxyInfo (void) const
    {
        return *_spRemoteProxyInfo;
    }

    inline const unsigned char ConnectivitySolutions::getEncryptionDescription (ConnectorType connectorType) const
    {
        return _connEncryptionDescriptor[connectorType];
    }

    inline Connection * const ConnectivitySolutions::getActiveConnectionForConnectorAndEncryptionType (ConnectorType connectorType, EncryptionType encryptionType) const
    {
        return _activeConnections.getActiveConnection (connectorType, encryptionType);
    }

    inline const QueryResult ConnectivitySolutions::getBestConnectionSolutionForConnectorAndEncryptionType (ConnectorType connectorType, EncryptionType encryptionType) const
    {
        // The InetAddr of the remote NetProxy server is included only if the remote NetProxy is reachable from the local host
        const auto & iaRemoteNetProxy = _spRemoteProxyInfo->isRemoteProxyReachableFromLocalHost() ?
            _spRemoteProxyInfo->getRemoteServerInetAddrForIPv4AddressAndConnectorType (_ui32RemoteInterfaceIPv4Address, connectorType) :
            NetProxyApplicationParameters::IA_INVALID_ADDR;
        return QueryResult{_spRemoteProxyInfo->getRemoteNetProxyID(), getLocalInterfaceIPv4Address(), iaRemoteNetProxy, connectorType,
                           encryptionType, getActiveConnectionForConnectorAndEncryptionType (connectorType, encryptionType)};
    }

    inline bool ConnectivitySolutions::hasSolutionToConnectToIPv4Address (uint32 ui32InterfaceIPv4Address) const
    {
        return _spRemoteProxyInfo->hasInterfaceIPv4Address (ui32InterfaceIPv4Address);
    }

    inline Connection * const ConnectivitySolutions::setActiveConnection (Connection * const pConnection)
    {
        return _activeConnections.setNewActiveConnection (pConnection);
    }

    inline Connection * const ConnectivitySolutions::removeActiveConnection (const Connection * const pConnection)
    {
        return _activeConnections.removeActiveConnection (pConnection);
    }

    inline void ConnectivitySolutions::setEncryptionDescription (ConnectorType connectorType, EncryptionType encryptionType)
    {
        _connEncryptionDescriptor[connectorType] |= encryptionType;
    }

    inline void ConnectivitySolutions::setRemoteProxyInfo (std::shared_ptr<RemoteProxyInfo> & spRemoteProxyInfo)
    {
        _spRemoteProxyInfo = spRemoteProxyInfo;
    }

    inline void ConnectivitySolutions::addInterfaceIPv4AddressToRemoteProxyInfo (const NOMADSUtil::InetAddr & iaInterfaceIPv4Address)
    {
        _spRemoteProxyInfo->addRemoteServerAddr (iaInterfaceIPv4Address);
    }

}

#endif  // INCL_CONNECTION_INFO_H
