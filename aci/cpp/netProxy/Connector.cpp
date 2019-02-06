/*
 * Connector.cpp
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

#include "Connector.h"
#include "ConfigurationParameters.h"
#include "TCPSocketConnector.h"
#include "UDPConnector.h"
#include "MocketConnector.h"
#include "CSRConnector.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    Connector * const Connector::connectorFactoryMethod (ConnectorType connectorType, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                                                         TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return new TCPSocketConnector{rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager};
        case CT_UDPSOCKET:
            return new UDPConnector{rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager};
        case CT_MOCKETS:
            return new MocketConnector{rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager};
        case CT_CSR:
            return new CSRConnector{rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager};
        case CT_UNDEF:
            break;
        }

        return nullptr;
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

}
