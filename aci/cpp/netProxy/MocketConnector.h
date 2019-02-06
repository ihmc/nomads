#ifndef INCL_MOCKET_CONNECTOR_H
#define INCL_MOCKET_CONNECTOR_H

/*
 * MocketConnector.h
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
 * Handles incoming and outgoing Mockets connections with remote NetProxies.
 */

#include <memory>

#include "ManageableThread.h"
#include "ServerMocket.h"

#include "Connector.h"


namespace ACMNetProxy
{
    class MocketConnector : public NOMADSUtil::ManageableThread, public virtual Connector
    {
    public:
        MocketConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                         PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);
        virtual ~MocketConnector (void);

        virtual int init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address);
        virtual void terminateExecution (void);
        void run (void);


    private:
        std::unique_ptr<ServerMocket> _upServerMocket;
    };


    inline MocketConnector::MocketConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                                             PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        Connector{CT_MOCKETS, rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager}, _upServerMocket{nullptr}
    { }

}

#endif   // #ifndef INCL_MOCKET_CONNECTOR_H
