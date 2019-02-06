#ifndef INCL_CONNECTOR_H
#define INCL_CONNECTOR_H

/*
 * Connector.h
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
 * Abstract class that provides the interface of a Connector,
 * that is a class that is capable of opening new
 * connections with remote NetProxies.
 */

#include "FTypes.h"
#include "Logger.h"

#include "Utilities.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    class ConnectionManager;
    class TCPConnTable;
    class TCPManager;
    class PacketRouter;
    class StatisticsManager;


    class Connector
    {
    public:
        static Connector * const connectorFactoryMethod (ConnectorType connectorType, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                                                         TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);

        explicit Connector (const Connector & rConnector) = delete;
        virtual ~Connector (void);

        virtual int init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address = 0) = 0;
        template <typename INVALID1, typename INVALID2> int init (INVALID1, INVALID2) = delete;     // Do not allow any implicit type conversion

        virtual void terminateExecution (void) = 0;

        ConnectorType getConnectorType (void) const;
        const char * const getConnectorTypeAsString (void) const;
        uint32 getBoundIPv4Address (void) const;
        uint16 getBoundPort (void) const;

        static const uint16 getAcceptServerPortForConnector (ConnectorType connectorType);


    protected:
        Connector (ConnectorType connectorType, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                   TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);


        const ConnectorType _connectorType;
        const char * const _pcConnectorTypeName;
        uint32 _ui32BoundIPv4Address;
        uint16 _ui16BoundPort;

        ConnectionManager & _rConnectionManager;
        TCPConnTable & _rTCPConnTable;
        TCPManager & _rTCPManager;
        PacketRouter & _rPacketRouter;
        StatisticsManager & _rStatisticsManager;
    };


    inline Connector::~Connector (void)
    {
        Connector::terminateExecution();

        checkAndLogMsg ("Connector::~Connector", NOMADSUtil::Logger::L_LowDetailDebug,
                        "%sConnector terminated\n", connectorTypeToString (_connectorType));
    }

    inline void Connector::terminateExecution (void) { }

    inline int Connector::init (uint16 ui16AcceptServerPort, uint32 ui32BindIPv4Address)
    {
        _ui32BoundIPv4Address = ui32BindIPv4Address;
        _ui16BoundPort = ui16AcceptServerPort;

        return 0;
    }

    inline ConnectorType Connector::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline const char * const Connector::getConnectorTypeAsString (void) const
    {
        return _pcConnectorTypeName;
    }

    inline uint32 Connector::getBoundIPv4Address (void) const
    {
        return _ui32BoundIPv4Address;
    }

    inline uint16 Connector::getBoundPort (void) const
    {
        return _ui16BoundPort;
    }

    inline Connector::Connector (ConnectorType connectorType, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                                 TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        _connectorType{connectorType}, _pcConnectorTypeName{connectorTypeToString (connectorType)}, _ui32BoundIPv4Address{0},
        _ui16BoundPort{0}, _rConnectionManager{rConnectionManager}, _rTCPConnTable{rTCPConnTable}, _rTCPManager{rTCPManager},
        _rPacketRouter{rPacketRouter}, _rStatisticsManager{rStatisticsManager}
    { }
}

#endif  // INCL_CONNECTOR_H
