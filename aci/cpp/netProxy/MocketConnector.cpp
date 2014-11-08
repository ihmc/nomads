/*
 * MocketConnector.cpp
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
 */

#include "Logger.h"

#include "MocketConnector.h"
#include "ConnectorAdapter.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "NetProxyConfigManager.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    MocketConnector::MocketConnector (void) : Connector (CT_MOCKETS), _pServerMocket (NULL) { }

    MocketConnector::~MocketConnector (void)
    {
        requestTermination();
        
        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pServerMocket) {
                _pServerMocket->close();

                delete _pServerMocket;
                _pServerMocket = NULL;
            }
            _mConnector.unlock();
        }
    }

    int MocketConnector::init (uint16 ui16MocketPort)
    {
        int rc;

        _pServerMocket = new ServerMocket();
        if ((rc = _pServerMocket->listen (ui16MocketPort)) < 0) {
            checkAndLogMsg ("MocketConnector::init", Logger::L_MildError,
                            "listen() on ServerMocket failed - could not initialize to use port %d; rc = %d\n",
                            (int) ui16MocketPort, rc);
            return -1;
        }

        return 0;
    }

    void MocketConnector::terminateExecution (void)
    {
        requestTermination();

        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pServerMocket) {
                _pServerMocket->close();
            }
            Connector::terminateExecution();
            _mConnector.unlock();
        }
    }

    void MocketConnector::run (void)
    {
        started();

        while (!terminationRequested()) {
            Mocket *pMocket = _pServerMocket->accept();
            if (!pMocket) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("MocketConnector::run", Logger::L_MildError,
                                    "accept() on ServerMocket failed\n");
                    setTerminatingResultCode (-1);
                }
                break;
            }

            ConnectorAdapter * const pConnectorAdapter = ConnectorAdapter::ConnectorAdapterFactory (pMocket, CT_MOCKETS);
            Connection * const pConnection = new Connection (pConnectorAdapter, this);

            _mConnectionsTable.lock();
            pConnection->lock();
            Connection *pOldConnection = _connectionsTable.put (generateUInt64Key (pMocket->getRemoteAddress(), pMocket->getRemotePort()), pConnection);
            _mConnectionsTable.unlock();

            if (pOldConnection) {
                // There was already a connection from this node to the remote node - close that one
                checkAndLogMsg ("MocketConnector::run", Logger::L_Info,
                                "replaced an old MocketConnection to <%s:%hu> in status %hu with a new one\n",
                                pConnection->getRemoteProxyInetAddr()->getIPAsString(), pConnection->getRemoteProxyInetAddr()->getPort(),
                                pOldConnection->getStatus());
                delete pOldConnection;
            }
            else {
                checkAndLogMsg ("MocketConnector::run", Logger::L_Info,
                                "accepted a new MocketConnection from <%s:%hu>\n",
                                pConnection->getRemoteProxyInetAddr()->getIPAsString(),
                                pConnection->getRemoteProxyInetAddr()->getPort());
            }

            pConnection->startMessageHandler();
            pConnection->unlock();
        }
        checkAndLogMsg ("MocketConnector::run", Logger::L_Info,
                        "MocketConnector terminated; termination code is %d\n", getTerminatingResultCode());

        Connector::_pConnectionManager->deregisterConnector (_connectorType);

        terminating();
        delete this;
    }

}
