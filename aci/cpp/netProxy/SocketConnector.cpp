/*
 * SocketConnector.cpp
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

#include "SocketConnector.h"
#include "ConnectorAdapter.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "NetProxyConfigManager.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    SocketConnector::SocketConnector (void) : Connector (CT_SOCKET), _pServerSocket (NULL) { }

    SocketConnector::~SocketConnector (void)
    {
        requestTermination();

        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pServerSocket) {
                _pServerSocket->shutdown (true, false);
                _pServerSocket->disableReceive();

                delete _pServerSocket;
                _pServerSocket = NULL;
            }
            _mConnector.unlock();
        }
    }

    int SocketConnector::init (uint16 ui16SocketPort)
    {
        int rc;

        _pServerSocket = new TCPSocket();
        if ((rc = _pServerSocket->setupToReceive (ui16SocketPort)) < 0) {
            checkAndLogMsg ("SocketConnector::init", Logger::L_MildError,
                            "listen() on ServerSocket failed - could not initialize to use port %d; rc = %d\n",
                            (int) ui16SocketPort, rc);
            return -1;
        }

        return 0;
    }

    void SocketConnector::terminateExecution (void)
    {
        requestTermination();

        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pServerSocket) {
                _pServerSocket->shutdown (true, false);
                _pServerSocket->disableReceive();
            }
            Connector::terminateExecution();
            _mConnector.unlock();
        }
    }

    void SocketConnector::run (void)
    {
        started();

        while (!terminationRequested()) {
            TCPSocket * const pSocket = dynamic_cast<TCPSocket * const> (_pServerSocket->accept());
            if (!pSocket) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("SocketConnector::run", Logger::L_MildError,
                                    "accept() on ServerSocket failed with error %d\n",
                                    _pServerSocket->error());
                    setTerminatingResultCode (-1);
                }
                break;
            }
            pSocket->bufferingMode (0);

            ConnectorAdapter * const pConnectorAdapter = ConnectorAdapter::ConnectorAdapterFactory (pSocket);
            Connection * const pConnection = new Connection (pConnectorAdapter, this);

            _mConnectionsTable.lock();
            pConnection->lock();
            Connection * const pOldConnection = _connectionsTable.put (generateUInt64Key (InetAddr (pSocket->getRemoteHostAddr(), pSocket->getRemotePort())), pConnection);
            _mConnectionsTable.unlock();
            if (pOldConnection) {
                // There was already a connection from this node to the remote node - close that one
                checkAndLogMsg ("SocketConnector::run", Logger::L_Info,
                                "replaced an old SocketConnection to <%s:%hu> in status %hu with a new one\n",
                                pConnection->getRemoteProxyInetAddr()->getIPAsString(), pConnection->getRemoteProxyInetAddr()->getPort(),
                                pOldConnection->getStatus());
                delete pOldConnection;
            }
            else {
                checkAndLogMsg ("SocketConnector::run", Logger::L_Info,
                                "accepted a new SocketConnection from <%s:%hu>\n",
                                pConnection->getRemoteProxyInetAddr()->getIPAsString(),
                                pConnection->getRemoteProxyInetAddr()->getPort());
            }

            pConnection->startMessageHandler();
            pConnection->unlock();
        }
        checkAndLogMsg ("SocketConnector::run", Logger::L_Info,
                        "SocketConnector terminated; termination code is %d\n", getTerminatingResultCode());

        Connector::_pConnectionManager->deregisterConnector (_connectorType);

        terminating();
        delete this;
    }
}
