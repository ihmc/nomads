#ifndef INCL_SOCKET_CONNECTOR_H
#define INCL_SOCKET_CONNECTOR_H

/*
 * TCPSocketConnector.h
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
 *
 * Handles TCP connections with remote NetProxies.
 */

#include <memory>

#include "ManageableThread.h"
#include "TCPSocket.h"

#include "Connector.h"


namespace NOMADSUtil
{
    class TCPSocket;
}

namespace ACMNetProxy
{
    class TCPSocketConnector : public NOMADSUtil::ManageableThread, public virtual Connector
    {
    public:
        TCPSocketConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                            PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);
        virtual ~TCPSocketConnector (void);

        virtual int init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address);
        virtual void terminateExecution (void);
        void run (void);


    private:
        std::unique_ptr<NOMADSUtil::TCPSocket> _upServerSocket;
    };


    inline TCPSocketConnector::TCPSocketConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                                                   PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        Connector{CT_TCPSOCKET, rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager}, _upServerSocket{nullptr}
    { }
}

#endif   // #ifndef INCL_SOCKET_CONNECTOR_H
