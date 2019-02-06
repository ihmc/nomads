/*
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
 */

#include "Mocket.h"
#include "ServerMocket.h"
#include "ProxyCommInterface.h"
#include "Logger.h"

#include "CSRConnector.h"
#include "ConnectorAdapter.h"
#include "Connection.h"
#include "ConnectionManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    CSRConnector::~CSRConnector (void)
    {
        requestTermination();

        if (_upServerMocket) {
            _upServerMocket->close();
        }
    }

    int CSRConnector::init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address)
    {
        int rc;

        ProxyCommInterface *pPCI = new ProxyCommInterface{NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort};
        _upServerMocket = make_unique<ServerMocket> (nullptr, pPCI);
        if ((rc = _upServerMocket->listen (ui16AcceptServerPort, NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString())) < 0) {
            checkAndLogMsg ("CSRConnector::init", NOMADSUtil::Logger::L_MildError,
                            "listen() on ServerMocket failed - could not bind to address <%s:%hu>; rc = %d\n",
                            NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort, rc);
            return -1;
        }
        Connector::init (ui16AcceptServerPort, ui32LocalIPv4Address);

        checkAndLogMsg ("CSRConnector::init", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully initialized a server CSR-Mocket listening on the address <%s:%hu>\n",
                        NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort);

        return 0;
    }

    void CSRConnector::terminateExecution (void)
    {
        requestTermination();

        if (_upServerMocket) {
            _upServerMocket->close();
        }
        Connector::terminateExecution();
    }

    void CSRConnector::run (void)
    {
        started();

        int rc;
        while (!terminationRequested()) {
            auto * const pMocket = _upServerMocket->accept();
            if (!pMocket) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("CSRConnector::run", NOMADSUtil::Logger::L_MildError,
                                    "accept() on ServerMocket failed\n");
                    setTerminatingResultCode (-1);
                }
                break;
            }

            auto * const pConnectorAdapter =
                ConnectorAdapter::ConnectorAdapterFactory (pMocket, {pMocket->getLocalAddress(), pMocket->getLocalPort()}, getConnectorType());
            auto spConnection =
                std::make_shared<Connection> (pConnectorAdapter->getLocalInetAddr(), pConnectorAdapter->getRemoteInetAddr(), pConnectorAdapter,
                                              _rConnectionManager, _rTCPConnTable, _rTCPManager, _rPacketRouter, _rStatisticsManager);
            if ((rc = Connection::addNewUninitializedConnectionToTable (spConnection)) < 0) {
                checkAndLogMsg ("CSRConnector::run", NOMADSUtil::Logger::L_Warning,
                                "error adding newly accepted (uninitialized) CSR "
                                "connection to the table; rc = %d\n", rc);
            }
        }
        checkAndLogMsg ("CSRConnector::run", NOMADSUtil::Logger::L_Info,
                        "CSRConnector terminated; termination code is %d\n",
                        getTerminatingResultCode());
        terminating();

        // Deregistration might trigger the Connector's delete, and so it must be done after the call to terminating()
        _rConnectionManager.deregisterConnector (getBoundIPv4Address(), _connectorType);
    }

}
