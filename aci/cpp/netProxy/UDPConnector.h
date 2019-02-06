#ifndef INCL_UDP_CONNECTOR_H
#define INCL_UDP_CONNECTOR_H

/*
 * UDPConnector.h
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Handles incoming and outgoing UDP datagrams to/from remote NetProxies.
 */

#include <unordered_map>
#include <mutex>
#include <memory>
#if defined (LINUX)
    #include <algorithm>
#endif

#include "ManageableThread.h"

#include "ConfigurationParameters.h"
#include "UDPSocketAdapter.h"
#include "Connector.h"


namespace ACMNetProxy
{
    class Connection;


    class UDPConnector : public NOMADSUtil::ManageableThread, public virtual Connector
    {
    public:
        UDPConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                      PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);
        virtual ~UDPConnector (void);

        virtual int init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address);
        virtual void terminateExecution (void);
        void run (void);

        UDPSocketAdapter * const getUDPSocketAdapter (void) const;


    private:
        static uint64 generateUDPConnectionsTableKey (const NOMADSUtil::InetAddr & iaRemoteNetProxy);


        unsigned char _pucInBuf[NetworkConfigurationSettings::PROXY_MESSAGE_MTU];
        int32 _i32BytesInBuffer;

        std::unordered_map<uint64, std::shared_ptr<Connection>> _umUDPConnections;

        std::unique_ptr<UDPSocketAdapter> _upUDPSocketAdapter;
    };


    inline UDPConnector::UDPConnector (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                                       PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        Connector{CT_UDPSOCKET, rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager},
        _pucInBuf{0, }, _i32BytesInBuffer{0}, _upUDPSocketAdapter{nullptr}
    { }

    inline UDPSocketAdapter * const UDPConnector::getUDPSocketAdapter (void) const
    {
        return _upUDPSocketAdapter.get();
    }

    inline uint64 UDPConnector::generateUDPConnectionsTableKey (const NOMADSUtil::InetAddr & iaRemoteNetProxy)
    {
        return generateUInt64Key (iaRemoteNetProxy.getIPAddress(), iaRemoteNetProxy.getPort(), CT_UDPSOCKET, ET_PLAIN);
    }
}

#endif   // #ifndef INCL_UDP_CONNECTOR_H
