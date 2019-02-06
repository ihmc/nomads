#ifndef INCL_CSR_CONNECTOR_H
#define INCL_CSR_CONNECTOR_H

/*
 * CSRConnector.h
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
 * Handles incoming and outgoing CSR connections with remote NetProxies.
 */

#include <memory>

#include "ManageableThread.h"

#include "Connector.h"


class ServerMocket;


namespace ACMNetProxy
{
    class CSRConnector : public NOMADSUtil::ManageableThread, public virtual Connector
    {
    public:
        CSRConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                      PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);
        virtual ~CSRConnector (void);

        int init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address);
        virtual void terminateExecution (void);
        void run (void);


    private:
        std::unique_ptr<ServerMocket> _upServerMocket;
    };


    inline CSRConnector::CSRConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                                       PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        Connector{CT_CSR, rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager}, _upServerMocket{nullptr}
    { }
}

#endif   // #ifndef INCL_CSR_CONNECTOR_H
