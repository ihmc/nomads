#ifndef INCL_CONNECTION_INFO_H
#define INCL_CONNECTION_INFO_H

/*
 * ConnectivitySolutions.h
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
 * Class that gathers all possible connectivity solutions to reach a remote NetProxy.
 * In its current version, the class uses composition to manage all active
 * connections to a given NetProxy (object instance of ActiveConnection),
 * as well as all the necessary information to open new connections
 * to it (object instance of RemoteProxyInfo).
 */

#include "Utilities.h"
#include "QueryResult.h"
#include "RemoteProxyInfo.h"
#include "ActiveConnection.h"
#include "UDPConnector.h"


namespace ACMNetProxy
{
    class Connection;

    class ConnectivitySolutions
    {
        /* Class that contains all possible connectivity
         * solutions to reach a remote NetProxy.
         * So far, possible solutions comprehends:
         *      - an ActiveConnection instance, with currently
         *        opened connections to the remote NetProxy;
         *      - a RemoteProxyInfo instance, with the necessary
         *        information to open a new connection to the
         *        remote NetProxy.
         */
    public:
        ConnectivitySolutions (const RemoteProxyInfo & remoteProxyInfo);
        ConnectivitySolutions (Connection * const pActiveConnectionToRemoteProxy, const RemoteProxyInfo & remoteProxyInfo);

        bool operator== (const ConnectivitySolutions & rhs) const;
        bool operator!= (const ConnectivitySolutions & rhs) const;

        static const ConnectivitySolutions & getInvalidConnectivitySolutions (void);

        const uint32 getRemoteProxyID (void) const;
        const ActiveConnection & getActiveConnections (void) const;
        const RemoteProxyInfo & getRemoteProxyInfo (void) const;
        const unsigned char getEncryptionDescription (ConnectorType connectorType) const;
        Connection * const getActiveConnectionForConnectorType (ConnectorType connectorType, EncryptionType encryptionType) const;
        const QueryResult getBestConnectionSolutionForConnectorType (ConnectorType connectorType, EncryptionType encryptionType) const;

        Connection * const setActiveConnection (Connection * const pConnection);
        Connection * const removeActiveConnection (const Connection * const pConnection);
        void setEncryptionDescription (ConnectorType connectorType, EncryptionType encryptionType);

    private:
        friend class ConnectionManager;

        ConnectivitySolutions (void);

        ActiveConnection _activeConnections{UDPConnector::getUDPConnection()};
        RemoteProxyInfo _remoteProxyInfo;
        unsigned char _connEncryptionDescriptor[CT_SIZE];
    };


    inline ConnectivitySolutions::ConnectivitySolutions (void)
    {
        memset (_connEncryptionDescriptor, 0, CT_SIZE);
    }

    inline ConnectivitySolutions::ConnectivitySolutions (const RemoteProxyInfo & remoteProxyInfo) : _remoteProxyInfo(remoteProxyInfo)
    {
        memset (_connEncryptionDescriptor, 0, CT_SIZE);
    }

    inline ConnectivitySolutions::ConnectivitySolutions (Connection * const pActiveConnectionToRemoteProxy, const RemoteProxyInfo & remoteProxyInfo) :
        _remoteProxyInfo(remoteProxyInfo)
    {
        _activeConnections.setNewActiveConnection (pActiveConnectionToRemoteProxy);
        memset (_connEncryptionDescriptor, 0, CT_SIZE);
    }

    inline bool ConnectivitySolutions::operator== (const ConnectivitySolutions & rhs) const
    {
        return this->getRemoteProxyID() == rhs.getRemoteProxyID();
    }

    inline bool ConnectivitySolutions::operator!= (const ConnectivitySolutions & rhs) const
    {
        return this->getRemoteProxyID() != rhs.getRemoteProxyID();
    }

    inline const ConnectivitySolutions & ConnectivitySolutions::getInvalidConnectivitySolutions (void)
    {
        static ConnectivitySolutions INVALID_CONNECTIVITY_SOLUTIONS;

        return INVALID_CONNECTIVITY_SOLUTIONS;
    }

    inline const uint32 ConnectivitySolutions::getRemoteProxyID (void) const
    {
        return _remoteProxyInfo.getRemoteProxyID();
    }

    inline const ActiveConnection & ConnectivitySolutions::getActiveConnections (void) const
    {
        return _activeConnections;
    }

    inline const RemoteProxyInfo & ConnectivitySolutions::getRemoteProxyInfo (void) const
    {
        return _remoteProxyInfo;
    }

    inline const unsigned char ConnectivitySolutions::getEncryptionDescription (ConnectorType connectorType) const
    {
        return _connEncryptionDescriptor[connectorType];
    }

    inline Connection * const ConnectivitySolutions::getActiveConnectionForConnectorType (ConnectorType connectorType, EncryptionType encryptionType) const
    {
        return _activeConnections.getActiveConnection (connectorType, encryptionType);
    }

    inline const QueryResult ConnectivitySolutions::getBestConnectionSolutionForConnectorType (ConnectorType connectorType, EncryptionType encryptionType) const
    {
        // The InetAddr of the remote NetProxy server is included only if the remote NetProxy is reachable from the local host
        return QueryResult (_remoteProxyInfo.getRemoteProxyID(),
                            _remoteProxyInfo.isRemoteProxyReachableFromLocalHost() ? _remoteProxyInfo.getRemoteProxyInetAddr (connectorType) : nullptr,
                            encryptionType, getActiveConnectionForConnectorType (connectorType, encryptionType));
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

}

#endif  // INCL_CONNECTION_INFO_H
