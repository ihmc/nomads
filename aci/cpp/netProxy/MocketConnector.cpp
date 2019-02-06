/*
 * MocketConnector.cpp
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
#include "Logger.h"

#include "MocketConnector.h"
#include "ConnectorAdapter.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "Utilities.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    MocketConnector::~MocketConnector (void)
    {
        requestTermination();

        if (_upServerMocket) {
            _upServerMocket->close();
        }
    }

    int MocketConnector::init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address)
    {
        int rc;

        _upServerMocket = make_unique<ServerMocket> (nullptr, nullptr, false, NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED);
        if ((rc = _upServerMocket->listen (ui16AcceptServerPort, NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString())) < 0) {
            checkAndLogMsg ("MocketConnector::init", NOMADSUtil::Logger::L_MildError,
                            "listen() on ServerMocket failed - could not bind to address <%s:%hu>; rc = %d\n",
                            NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort, rc);
            return -1;
        }
        Connector::init (ui16AcceptServerPort, ui32LocalIPv4Address);

        checkAndLogMsg ("MocketConnector::init", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully initialized a server Mocket listening on the address <%s:%hu> and using %s encryption\n",
                        NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort,
                        NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED ? "DTLS" : "PLAIN");

        return 0;
    }

    void MocketConnector::terminateExecution (void)
    {
        requestTermination();

        if (_upServerMocket) {
            _upServerMocket->close();
        }
        Connector::terminateExecution();
    }

    void MocketConnector::run (void)
    {
        started();

        int rc;
        while (!terminationRequested()) {
            auto * const pMocket = _upServerMocket->accept();
            if (!pMocket) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("MocketConnector::run", NOMADSUtil::Logger::L_MildError,
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
                checkAndLogMsg ("MocketConnector::run", NOMADSUtil::Logger::L_Warning,
                                "error adding newly accepted (uninitialized) Mocket "
                                "connection to the table; rc = %d\n", rc);
            }
        }
        checkAndLogMsg ("MocketConnector::run", NOMADSUtil::Logger::L_Info,
                        "MocketConnector terminated; termination code is %d\n",
                        getTerminatingResultCode());

        setTerminatingResultCode (0);
        terminating();

        // Deregistration might trigger the Connector's delete, and so it must be done after the call to terminating()
        _rConnectionManager.deregisterConnector (getBoundIPv4Address(), _connectorType);
    }

}
