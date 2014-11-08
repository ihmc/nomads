#ifndef INCL_CONNECTION_INFO_H
#define INCL_CONNECTION_INFO_H

/*
 * ConnectivitySolutions.h
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
        Connection * const getActiveConnectionForConnectorType (ConnectorType connectorType) const;
        const QueryResult getBestConnectionSolutionForConnectorType (ConnectorType connectorType) const;

        Connection * const setActiveConnection (Connection * const pConnection);
        Connection * const removeActiveConnection (const Connection * const pConnection);

    private:
        friend class ConnectionManager;

        ConnectivitySolutions (void);

        ActiveConnection _activeConnections;
        RemoteProxyInfo _remoteProxyInfo;
    };

    
    inline ConnectivitySolutions::ConnectivitySolutions (void) { }

    inline ConnectivitySolutions::ConnectivitySolutions (const RemoteProxyInfo & remoteProxyInfo) :
        _remoteProxyInfo (remoteProxyInfo) { }

    inline ConnectivitySolutions::ConnectivitySolutions (Connection * const pActiveConnectionToRemoteProxy, const RemoteProxyInfo & remoteProxyInfo) :
        _remoteProxyInfo (remoteProxyInfo)
    {
        _activeConnections.setNewActiveConnection (pActiveConnectionToRemoteProxy);
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

    inline Connection * const ConnectivitySolutions::getActiveConnectionForConnectorType (ConnectorType connectorType) const
    {
        return _activeConnections.getActiveConnection (connectorType);
    }

    inline const QueryResult ConnectivitySolutions::getBestConnectionSolutionForConnectorType (ConnectorType connectorType) const
    {
        // The InetAddr of the remote NetProxy server is included only if the remote NetProxy is reachable from the local host
        return QueryResult (_remoteProxyInfo.getRemoteProxyID(), _remoteProxyInfo.isRemoteProxyReachableFromLocalHost() ? _remoteProxyInfo.getRemoteProxyInetAddr (connectorType) : NULL,
                            getActiveConnectionForConnectorType (connectorType));
    }
    
    inline Connection * const ConnectivitySolutions::setActiveConnection (Connection * const pConnection)
    {
        return _activeConnections.setNewActiveConnection (pConnection);
    }

    inline Connection * const ConnectivitySolutions::removeActiveConnection (const Connection * const pConnection)
    {
        return _activeConnections.removeActiveConnection (pConnection);
    }
}

#endif  // INCL_CONNECTION_INFO_H