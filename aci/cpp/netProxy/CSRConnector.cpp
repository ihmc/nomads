/*
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

#include "ProxyCommInterface.h"
#include "Logger.h"

#include "CSRConnector.h"
#include "ConnectorAdapter.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "ConfigurationManager.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    CSRConnector::CSRConnector (void) :
        Connector (CT_CSR), _pServerMocket (nullptr) { }

    CSRConnector::~CSRConnector (void)
    {
        requestTermination();

        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pServerMocket) {
                _pServerMocket->close();

                delete _pServerMocket;
                _pServerMocket = nullptr;
            }
            _mConnector.unlock();
        }
    }

    int CSRConnector::init (uint16 ui16MocketPort)
    {
        int rc;

        ProxyCommInterface *pPCI = new ProxyCommInterface (NetProxyApplicationParameters::CSR_PROXY_SERVER_ADDR, NetProxyApplicationParameters::CSR_PROXY_SERVER_PORT);
        _pServerMocket = new ServerMocket (nullptr, pPCI);
        if ((rc = _pServerMocket->listen (ui16MocketPort)) < 0) {
            checkAndLogMsg ("CSRConnector::init", Logger::L_MildError,
                            "listen() on ServerMocket failed - could not initialize to use port %d; rc = %d\n",
                            (int) ui16MocketPort, rc);
            return -1;
        }

        return 0;
    }

    void CSRConnector::terminateExecution (void)
    {
        requestTermination();

        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pServerMocket) {
                _pServerMocket->close();
            }
            _mConnector.unlock();
        }
    }

    void CSRConnector::run (void)
    {
        started();

        while (!terminationRequested()) {
            Mocket *pMocket = _pServerMocket->accept();
            if (!pMocket) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("CSRConnector::run", Logger::L_MildError,
                                    "accept() on ServerMocket failed\n");
                    setTerminatingResultCode (-1);
                }
                break;
            }
            pMocket->setLocalAddr(InetAddr(NetProxyApplicationParameters::EXTERNAL_IP_ADDR).getIPAsString());
            ConnectorAdapter * const pConnectorAdapter = ConnectorAdapter::ConnectorAdapterFactory (pMocket, _connectorType);
            Connection * const pConnection = new Connection (pConnectorAdapter, this);

            lockConnectionTable();
            pConnection->lock();

            Connection * const pOldConnection = _connectionsTable.put (
                generateUInt64Key (pMocket->getRemoteAddress(), pMocket->getRemotePort(), pConnection->getEncryptionType()), pConnection);
            if (pOldConnection) {
                // There was already a connection from this node to the remote node - close that one
                checkAndLogMsg ("CSRConnector::run", Logger::L_Info,
                                "replaced an existing CSRMocketConnection to <%s:%hu> in status %hu with a new instance\n",
                                pConnection->getRemoteProxyInetAddr()->getIPAsString(),
                                pConnection->getRemoteProxyInetAddr()->getPort(),
                                pOldConnection->getStatus());
                delete pOldConnection;
            }
            else {
                checkAndLogMsg ("CSRConnector::run", Logger::L_Info,
                                "accepted a new CSR Mockets connection from <%s:%hu>\n",
                                pConnection->getRemoteProxyInetAddr()->getIPAsString(),
                                pConnection->getRemoteProxyInetAddr()->getPort());
            }
            pConnection->startMessageHandler();

            pConnection->unlock();
            unlockConnectionTable();
        }
        checkAndLogMsg ("CSRConnector::run", Logger::L_Info,
                        "CSRConnector terminated; termination code is %d\n",
                        getTerminatingResultCode());

        Connector::_pConnectionManager->deregisterConnector (_connectorType);

        terminating();
        delete this;
    }

}
