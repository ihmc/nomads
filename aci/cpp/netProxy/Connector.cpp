/*
 * Connector.cpp
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

#include "Connector.h"
#include "Connection.h"
#include "MocketConnector.h"
#include "SocketConnector.h"
#include "UDPConnector.h"
#include "CSRConnector.h"
#include "ConnectionManager.h"
#include "GUIUpdateMessage.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    Connector::~Connector (void)
    {
        Connector::terminateExecution();

        checkAndLogMsg ("Connector::~Connector", Logger::L_LowDetailDebug,
                        "%sConnector terminated\n", connectorTypeToString (_connectorType));
    }

    Connection * const Connector::getAvailableConnectionToRemoteProxy (const InetAddr * const pRemoteProxyAddr, EncryptionType encryptionType)
    {
        if (_connectorType == CT_UDPSOCKET) {
            return UDPConnector::getUDPConnection();
        }

        if (!pRemoteProxyAddr) {
            return nullptr;
        }

        lockConnectionTable();
        Connection * const pConnection = _connectionsTable.get (generateUInt64Key (pRemoteProxyAddr, encryptionType));
        unlockConnectionTable();
        if (!pConnection) {
            checkAndLogMsg ("Connector::getAvailableConnectionToRemoteProxy", Logger::L_MildError,
                            "%sConnection object to connect to remote proxy at address <%s:%hu> not available\n",
                            getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());
        }

        return pConnection;
    }

    Connection * const Connector::openNewConnectionToRemoteProxy (const QueryResult & query, bool bBlocking)
    {
        if ((query.getRemoteProxyUniqueID() == 0) || (_connectorType == CT_UNDEF)) {
            return nullptr;
        }

        if (_connectorType == CT_UDPSOCKET) {
            return UDPConnector::getUDPConnection();
        }

        if (query.getActiveConnectionToRemoteProxy()) {
            return query.getActiveConnectionToRemoteProxy();
        }

        int rc;
        const InetAddr * const pRemoteProxyAddr = query.getRemoteProxyServerAddress();
        const uint64 ui64HashKey = generateUInt64Key (pRemoteProxyAddr, query.getQueryEncryptionType());
        lockConnectionTable();
        Connection *pConnection = _connectionsTable.get (ui64HashKey);
        if (!pConnection) {
            pConnection = new Connection (_connectorType, query.getQueryEncryptionType(), query.getRemoteProxyUniqueID(), pRemoteProxyAddr, this);
			if (!pConnection) {
				checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_MildError,
					            "impossible to instantiate a new %sConnection object for connection to the remote NetProxy with address %s:%hu\n",
					            getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());
				return nullptr;
			}
            _connectionsTable.put (ui64HashKey, pConnection);
            checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_LowDetailDebug,
                            "created a %sConnection object using %s encryption to connect to the remote NetProxy at address %s:%hu\n",
                            pConnection->getConnectorTypeAsString(), encryptionTypeToString (query.getQueryEncryptionType()),
                            pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());
        }
        pConnection->lock();
        if (pConnection->isDeleteRequested()) {
            // Connection marked for delete --> create new Connection and add it to the Connections Table
            pConnection->unlock();  // unlock old Connection
			pConnection = nullptr;
            pConnection = new Connection (_connectorType, query.getQueryEncryptionType(), query.getRemoteProxyUniqueID(), pRemoteProxyAddr, this);
			if (!pConnection) {
				checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_MildError,
					            "impossible to instantiate a new %sConnection object for connection to the remote NetProxy with address %s:%hu\n",
					            getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());
				return nullptr;
			}
            pConnection->lock();    // lock new Connection
            _connectionsTable.put (ui64HashKey, pConnection);
            checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_Warning,
                            "retrieved a %sConnection object for the remote NetProxy with address %s:%hu, but the object was marked for deletion; "
                            "created a new %sConnection object using %s encryption to connect to the remote NetProxy\n", getConnectorTypeAsString(),
                            pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort(), getConnectorTypeAsString(),
                            encryptionTypeToString (query.getQueryEncryptionType()));
        }
        unlockConnectionTable();

        if (pConnection->isConnected()) {
            // Status is CS_Connected and ConnectorAdapter is connected
            checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_Warning,
                            "retrieved an active %sConnection object for the remote NetProxy with address %s:%hu even if the query did "
                            "not return any active Connection; %sConnection is connected to the address %s:%hu using %s encryption\n",
                            getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort(),
                            getConnectorTypeAsString(), pConnection->getRemoteProxyInetAddr()->getIPAsString(),
                            pConnection->getRemoteProxyInetAddr()->getPort(), encryptionTypeToString (query.getQueryEncryptionType()));
            _pConnectionManager->addNewActiveConnectionToRemoteProxy (pConnection);
            pConnection->unlock();
            return pConnection;
        }
        if (pConnection->isConnecting()) {
            checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_LowDetailDebug,
                            "%sConnection object to reach remote NetProxy at address %s:%hu using %s encryption is still connecting\n",
                             getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort(),
                            encryptionTypeToString (query.getQueryEncryptionType()));
            pConnection->unlock();
            return nullptr;
        }
        if (pConnection->getConnectorAdapter()->isConnected()) {
            // Close connection to keep consistency with the Connection status
            pConnection->getConnectorAdapter()->close();
            checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_Warning,
                            "%sConnection object for remote NetProxy at address %s:%hu is in status %d, but the ConnectionAdapter was connected to "
                            "the remote host; the actual connection was closed to keep consistency with the status of the %sConnection instance\n",
                            getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort(),
                            pConnection->getStatus(), getConnectorTypeAsString());
			//TO-DO: check if it is necessary to delete the ConnectorAdapter and create a new one
		}
        _pConnectionManager->removeActiveConnectionFromTable (pConnection);
        pConnection->setStatusAsDisconnected();

        // Do nothing if status of the Connection instance is not CS_NotConnected
        if (bBlocking) {
            if (0 != (rc = pConnection->connectSync())) {
                checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_MildError,
                                "failed to establish a %s connection to remote NetProxy at address %s; rc = %d\n",
                                getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), rc);
                pConnection->unlock();
                return nullptr;
            }
        }
        else {
            checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_LowDetailDebug,
                            "starting a %sConnection background thread to connect asynchronously to the remote NetProxy at address %s:%hu\n",
                            getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort());

            if (0 != (rc = pConnection->connectAsync())) {
                checkAndLogMsg ("Connector::openNewConnectionToRemoteProxy", Logger::L_MildError,
                                "failed to start background %sConnection thread to connect to the remote NetProxy at address %s:%hu; rc = %d\n",
                                getConnectorTypeAsString(), pRemoteProxyAddr->getIPAsString(), pRemoteProxyAddr->getPort(), rc);
                pConnection->unlock();
                return nullptr;
            }
        }
        pConnection->unlock();

        return pConnection->isConnected() ? pConnection : nullptr;
    }

    const uint16 Connector::getAcceptServerPortForConnector (ConnectorType connectorType)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return NetProxyApplicationParameters::TCP_SERVER_PORT;
        case CT_UDPSOCKET:
            return NetProxyApplicationParameters::UDP_SERVER_PORT;
        case CT_MOCKETS:
            return NetProxyApplicationParameters::MOCKET_SERVER_PORT;
        case CT_CSR:
            return NetProxyApplicationParameters::CSR_MOCKET_SERVER_PORT;
        case CT_UNDEF:
            break;
        }

        return 0;
    }

    bool Connector::isConnectingToRemoteAddr (const InetAddr * const pRemoteProxyAddr, EncryptionType encryptionType) const
    {
        if (!pRemoteProxyAddr) {
            return false;
        }

        lockConnectionTable();
        const Connection * const pConnection = _connectionsTable.get (generateUInt64Key (pRemoteProxyAddr, encryptionType));
        unlockConnectionTable();

        return pConnection ? pConnection->isConnecting() : false;
    }

    bool Connector::isConnectedToRemoteAddr (const InetAddr * const pRemoteProxyAddr, EncryptionType encryptionType) const
    {
        if (!pRemoteProxyAddr) {
            return false;
        }

        lockConnectionTable();
        const Connection * const pConnection = _connectionsTable.get (generateUInt64Key (pRemoteProxyAddr, encryptionType));
        unlockConnectionTable();

        return pConnection ? pConnection->isConnected() : false;
    }

    bool Connector::hasFailedConnectionToRemoteAddr (const InetAddr * const pRemoteProxyAddr, EncryptionType encryptionType) const
    {
        if (!pRemoteProxyAddr) {
            return false;
        }

        lockConnectionTable();
        const Connection * const pConnection = _connectionsTable.get (generateUInt64Key (pRemoteProxyAddr, encryptionType));
        unlockConnectionTable();

        return pConnection ? pConnection->hasFailedConnecting() : false;
    }

    Connector * const Connector::connectorFactoryMethod (ConnectorType connectorType)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return Connector::_pConnectionManager->getConnectorForType (connectorType) ?
                Connector::_pConnectionManager->getConnectorForType (connectorType) : new SocketConnector();
        case CT_UDPSOCKET:
            return Connector::_pConnectionManager->getConnectorForType (connectorType) ?
                Connector::_pConnectionManager->getConnectorForType (connectorType) : new UDPConnector();
        case CT_MOCKETS:
            return Connector::_pConnectionManager->getConnectorForType (connectorType) ?
                Connector::_pConnectionManager->getConnectorForType (connectorType) : new MocketConnector();
        case CT_CSR:
            return Connector::_pConnectionManager->getConnectorForType (connectorType) ?
                Connector::_pConnectionManager->getConnectorForType (connectorType) : new CSRConnector();
        case CT_UNDEF:
            break;
        }

        return nullptr;
    }

    // This method assumes that the lock on _mConnectionsTable is acquired by the caller
    Connection * const Connector::removeFromConnectionsTable (const Connection * const pConnectionToRemove)
    {
        uint64 ui64Key = generateUInt64Key (pConnectionToRemove->getRemoteProxyInetAddr(), pConnectionToRemove->getEncryptionType());
        Connection * const pConnection = _connectionsTable.get (ui64Key);
        if (pConnection == pConnectionToRemove) {
            _connectionsTable.remove (ui64Key);
        }

        return pConnection;
    }

}
