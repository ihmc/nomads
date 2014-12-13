#ifndef INCL_CONNECTOR_H
#define INCL_CONNECTOR_H

/*
 * Connector.h
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
 * Abstract class that provides the interface of a Connector,
 * that is a class that is capable of opening new
 * connections with remote NetProxies.
 */

#include "FTypes.h"
#include "UInt64Hashtable.h"

#include "Utilities.h"
#include "ProxyMessages.h"
#include "ProtocolSetting.h"
#include "QueryResult.h"
#include "Entry.h"
#include "ActiveConnection.h"


namespace NOMADSUtil
{
    class InetAddr;
}

namespace ACMNetProxy
{
    class Connection;
    class AutoConnectionEntry;
    class UDPDatagramPacket;
    class GUIStatsManager;
    class TCPManager;
    class PacketRouter;

    class Connector
    {
        public:
            virtual ~Connector (void);

            virtual int init (uint16 ui16AcceptServerPort) = 0;
            virtual void terminateExecution (void) = 0;

            ConnectorType getConnectorType (void) const;
            const char * const getConnectorTypeAsString (void) const;

            Connection * const getAvailableConnectionToRemoteProxy (const NOMADSUtil::InetAddr * const pRemoteProxyAddr);
            Connection * const openNewConnectionToRemoteProxy (const QueryResult & query, bool bBlocking);

            static const uint16 getAcceptServerPortForConnector (ConnectorType connectorType);


        protected:
            friend class AutoConnectionEntry;
            friend class TCPManager;
            friend class PacketRouter;

            Connector (ConnectorType connectorType);

            virtual bool isEnqueueingAllowed (void) const = 0;
            bool isConnectingToRemoteAddr (const NOMADSUtil::InetAddr * const pRemoteProxyAddr) const;
            bool isConnectedToRemoteAddr (const NOMADSUtil::InetAddr * const pRemoteProxyAddr) const;
            bool hasFailedConnectionToRemoteAddr (const NOMADSUtil::InetAddr * const pRemoteProxyAddr) const;

            const ConnectorType _connectorType;
            const char * const _pcConnectorTypeName;

            NOMADSUtil::UInt64Hashtable<Connection> _connectionsTable;
            mutable NOMADSUtil::Mutex _mConnector;
            mutable NOMADSUtil::Mutex _mConnectionsTable;

            static ConnectionManager * const _pConnectionManager;
            static GUIStatsManager * const _pGUIStatsManager;
            static PacketRouter * const _pPacketRouter;


        private:
            friend class Connection;

            explicit Connector (const Connector& rConnector);

            static Connector * const connectorFactoryMethod (ConnectorType connectorType);

            Connection * const removeFromConnectionsTable (const Connection * const pConnectionToRemove);
    };


    inline void Connector::terminateExecution (void)
    {
        _mConnectionsTable.lock();
        _connectionsTable.removeAll();
        _mConnectionsTable.unlock();
    }

    inline ConnectorType Connector::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline const char * const Connector::getConnectorTypeAsString (void) const
    {
        return _pcConnectorTypeName;
    }

    inline Connector::Connector (ConnectorType connectorType) :
        _connectorType (connectorType), _pcConnectorTypeName (connectorTypeToString (connectorType)), _connectionsTable (true) { }

}

#endif  // INCL_CONNECTOR_H
