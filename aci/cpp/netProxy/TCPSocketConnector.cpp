/*
 * TCPSocketConnector.cpp
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

#include "Logger.h"
#include "TCPSocket.h"

#include "TCPSocketConnector.h"
#include "ConnectorAdapter.h"
#include "Connection.h"
#include "ConnectionManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    TCPSocketConnector::~TCPSocketConnector (void)
    {
        requestTermination();

        if (_upServerSocket) {
            _upServerSocket->shutdown (true, false);
            _upServerSocket->disableReceive();
        }
    }

    int TCPSocketConnector::init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address)
    {
        int rc;

        _upServerSocket = make_unique<NOMADSUtil::TCPSocket>();
        if ((rc = _upServerSocket->setupToReceive (ui16AcceptServerPort, 5, ui32LocalIPv4Address)) < 0) {
            checkAndLogMsg ("TCPSocketConnector::init", NOMADSUtil::Logger::L_MildError,
                            "listen() on ServerSocket failed - could not bind to address <%s:%hu>; rc = %d\n",
                            NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort, rc);
            return -1;
        }
        Connector::init (ui16AcceptServerPort, ui32LocalIPv4Address);

        checkAndLogMsg ("TCPSocketConnector::init", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully initialized a TCP server listening on the address <%s:%hu>\n",
                        NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort);

        return 0;
    }

    void TCPSocketConnector::terminateExecution (void)
    {
        requestTermination();

        if (_upServerSocket) {
            _upServerSocket->shutdown (true, false);
            _upServerSocket->disableReceive();
        }
        Connector::terminateExecution();
    }

    void TCPSocketConnector::run (void)
    {
        started();

        int rc;
        const NOMADSUtil::InetAddr iaBoundAddress{getBoundIPv4Address(), _ui16BoundPort};
        while (!terminationRequested()) {
            auto * const pSocket = dynamic_cast<NOMADSUtil::TCPSocket * const> (_upServerSocket->accept());
            if (!pSocket) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("TCPSocketConnector::run", NOMADSUtil::Logger::L_MildError,
                                    "accept() on ServerSocket failed with error %d\n",
                                    _upServerSocket->error());
                    setTerminatingResultCode (-1);
                }
                break;
            }
            pSocket->bufferingMode (0);

            auto * const pConnectorAdapter = ConnectorAdapter::ConnectorAdapterFactory (pSocket, iaBoundAddress, getConnectorType());
            auto spConnection =
                std::make_shared<Connection> (iaBoundAddress, pConnectorAdapter->getRemoteInetAddr(), pConnectorAdapter, _rConnectionManager,
                                              _rTCPConnTable, _rTCPManager, _rPacketRouter, _rStatisticsManager);
            if ((rc = Connection::addNewUninitializedConnectionToTable (spConnection)) < 0) {
                checkAndLogMsg ("TCPSocketConnector::run", NOMADSUtil::Logger::L_Warning,
                                "error adding newly accepted (uninitialized) TCP "
                                "connection to the table; rc = %d\n", rc);
            }
        }
        checkAndLogMsg ("TCPSocketConnector::run", NOMADSUtil::Logger::L_Info,
                        "TCPSocketConnector terminated; termination code is %d\n",
                        getTerminatingResultCode());

        setTerminatingResultCode (0);
        terminating();

        // Deregistration might trigger the Connector's delete, and so it must be done after the call to terminating()
        _rConnectionManager.deregisterConnector (getBoundIPv4Address(), _connectorType);
    }
}
