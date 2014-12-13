/*
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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

#include "GUIUpdateMessage.h"
#include "ConfigurationParameters.h"
#include "TCPConnTable.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    void ConnectorStats::increaseTrafficIn (ProtocolType protocolType, uint32 ui32BytesIn)
    {
        switch (protocolType) {
            case PT_TCP:
            {
                _detailedTCPStats.increaseTrafficIn (ui32BytesIn);
                break;
            }
            case PT_UDP:
            {
                _detailedUDPStats.increaseTrafficIn (ui32BytesIn);
                break;
            }
            case PT_ICMP:
            {
                _detailedICMPStats.increaseTrafficIn (ui32BytesIn);
                break;
            }
            case PT_UNDEF:
            {
                return;
            }
        }

        _i64LastUpdateTime = getTimeInMilliseconds();
    }

    void ConnectorStats::increaseTrafficOut (ProtocolType protocolType, uint32 ui32BytesOut)
    {
        switch (protocolType) {
            case PT_TCP:
            {
                _detailedTCPStats.increaseTrafficOut (ui32BytesOut);
                break;
            }
            case PT_UDP:
            {
                _detailedUDPStats.increaseTrafficOut (ui32BytesOut);
                break;
            }
            case PT_ICMP:
            {
                _detailedICMPStats.increaseTrafficOut (ui32BytesOut);
                break;
            }
            case PT_UNDEF:
            {
                return;
            }
        }

        _i64LastUpdateTime = getTimeInMilliseconds();
    }

    void GUIStatsManager::increaseTrafficIn (ConnectorStats * const pConnectorStats, ProtocolType protocolType, uint32 ui32BytesIn)
    {
        if (!pConnectorStats || !NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED) {
            return;
        }

        pConnectorStats->lock();
        pConnectorStats->increaseTrafficIn (protocolType, ui32BytesIn);
        pConnectorStats->unlock();

        _m.lock();
        increaseSummaryTrafficIn (protocolType, ui32BytesIn);
        _m.unlock();
    }

    void GUIStatsManager::increaseTrafficOut (ConnectorStats * const pConnectorStats, ProtocolType protocolType, uint32 ui32BytesOut)
    {
        if (!pConnectorStats || !NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED) {
            return;
        }

        pConnectorStats->lock();
        pConnectorStats->increaseTrafficOut (protocolType, ui32BytesOut);
        pConnectorStats->unlock();

        _m.lock();
        increaseSummaryTrafficOut (protocolType, ui32BytesOut);
        _m.unlock();
    }

    void GUIStatsManager::packGUIUpdateMessage (void)
    {
        static TCPConnTable * const pTCPConnTable = TCPConnTable::getTCPConnTable();

        pTCPConnTable->lock();
        _m.lock();

        int64 i64CurrentTime = getTimeInMilliseconds();
        int64 i64ElapsedTime = i64CurrentTime - i64LastStatsUpdateComputationTime;
        float fElapsedTimeInSecs = static_cast<float> (i64ElapsedTime / 1000.0);
        _MessagePackBuffer.clear();

        // Compute general statistics
        _mConnectorStatsTable.lock();
        uint16 ui16ConnectorsNum = static_cast<uint16> (_ConnectorStatsTable.getCount());
        UInt64Hashtable<ConnectorStats>::Iterator iterator = _ConnectorStatsTable.getAllElements();
        ConnectorStats *connectorStat = NULL;
        while (connectorStat = iterator.getValue()) {
            connectorStat->lock();
            if ((connectorStat->getConnectorType() == CT_UDP) &&
                ((i64CurrentTime - connectorStat->getLastUpdateTime()) > NetProxyApplicationParameters::DEFAULT_UDP_CONNECTOR_INACTIVITY_TIME)) {
                ui16ConnectorsNum--;
            }
            connectorStat->unlock();
            iterator.nextElement();
        }

        // Packing header, IP address of the external network interface, and the summary statistics
        _MessagePacker.pack (std::string (NetProxyApplicationParameters::DEFAULT_GUI_UPDATE_MESSAGE_HEADER));
        _MessagePacker.pack_uint32 (NetProxyApplicationParameters::NETPROXY_EXTERNAL_IP_ADDR);
        packSummaryStats (fElapsedTimeInSecs);

        // Packing connectors and relative detailed statistics
        _MessagePacker.pack_uint16 (ui16ConnectorsNum);
        iterator = _ConnectorStatsTable.getAllElements();
        while (connectorStat = iterator.getValue()) {
            connectorStat->lock();

            if ((connectorStat->getConnectorType() == CT_UDP) &&
                ((i64CurrentTime - connectorStat->getLastUpdateTime()) > NetProxyApplicationParameters::DEFAULT_UDP_CONNECTOR_INACTIVITY_TIME)) {
                connectorStat->resetRate();
                connectorStat->unlock();
                iterator.nextElement();
                continue;
            }

            _MessagePacker.pack_uint8 (connectorStat->getConnectorTypeAsChar());
            _MessagePacker.pack_uint32 (connectorStat->getRemoteHostIP());
            _MessagePacker.pack_uint32 (connectorStat->getRemoteProxyIP());
            _MessagePacker.pack_uint16 (connectorStat->getRemoteProxyPort());
            _MessagePacker.pack_uint32 ((uint32) ((i64CurrentTime - connectorStat->getCreationTime()) / 1000));

            // Packing detailed statistics for the selected connector
            packDetailedStats (connectorStat, connectorStat->_detailedTCPStats, fElapsedTimeInSecs);
            packDetailedStats (connectorStat, connectorStat->_detailedUDPStats, fElapsedTimeInSecs);
            packDetailedStats (connectorStat, connectorStat->_detailedICMPStats, fElapsedTimeInSecs);

            connectorStat->resetRate();
            connectorStat->unlock();
            iterator.nextElement();
        }
        _mConnectorStatsTable.unlock();

        resetRate();
        updateLastStatsComputationTime();

        _m.unlock();
        pTCPConnTable->unlock();
    }

    void GUIStatsManager::deleteStatistics (ConnectorType connectorType, uint32 ui32RemoteProxyUniqueID)
    {
        if (!NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED ||
            (ui32RemoteProxyUniqueID == 0) || (connectorType == CT_UNDEF)) {
            return;
        }

        if (_mConnectorStatsTable.lock() == Mutex::RC_Ok) {
            for (UInt64Hashtable<ConnectorStats>::Iterator iter = _ConnectorStatsTable.getAllElements(); !iter.end(); iter.nextElement()) {
                if ((iter.getValue()->getRemoteProxyUniqueID() == ui32RemoteProxyUniqueID) &&
                    (iter.getValue()->getConnectorType() == connectorType)) {
                    delete _ConnectorStatsTable.remove (iter.getKey());
                }
            }
            _mConnectorStatsTable.unlock();
        }
    }

    ConnectorStats * const GUIStatsManager::registerStatisticsIfNecessary (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint32 ui32RemoteProxyUniqueID,
                                                                           uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort)
    {
        ConnectorStats *pConnectorStats = NULL;
        if (_mConnectorStatsTable.lock() == Mutex::RC_Ok) {
            if ((pConnectorStats = _ConnectorStatsTable.get (generateUInt64Key (ui32RemoteHostIP, connectorType))) &&
                (pConnectorStats->getRemoteProxyUniqueID() == ui32RemoteProxyUniqueID)) {
                // Connector already registered --> do nothing
                _mConnectorStatsTable.unlock();
                return pConnectorStats;
            }

            pConnectorStats = new ConnectorStats (connectorType, ui32RemoteHostIP, ui32RemoteProxyUniqueID, ui32RemoteProxyIP, ui16RemoteProxyPort);
            delete _ConnectorStatsTable.put (generateUInt64Key (ui32RemoteHostIP, connectorType), pConnectorStats);
            _mConnectorStatsTable.unlock();
            checkAndLogMsg ("GUIStatsManager::registerStatisticsIfNecessary", Logger::L_MediumDetailDebug,
                            "successfully registered new statistics for %s traffic sent to the remote host with IP "
                            "address %s, reachable through the NetProxy with ID %u\n", connectorTypeToString (connectorType),
                            InetAddr (ui32RemoteHostIP).getIPAsString(), ui32RemoteProxyUniqueID);
        }

        return pConnectorStats;
    }

    void GUIStatsManager::increaseSummaryTrafficIn (ProtocolType protocolType, uint32 ui32BytesIn)
    {
        switch (protocolType) {
            case PT_TCP:
            {
                summaryTCPStats.increaseTrafficIn (ui32BytesIn);
                break;
            }
            case PT_UDP:
            {
                summaryUDPStats.increaseTrafficIn (ui32BytesIn);
                break;
            }
            case PT_ICMP:
            {
                summaryICMPStats.increaseTrafficIn (ui32BytesIn);
                break;
            }
        }
    }

    void GUIStatsManager::increaseSummaryTrafficOut (ProtocolType protocolType, uint32 ui32BytesOut)
    {
        switch (protocolType) {
            case PT_TCP:
            {
                summaryTCPStats.increaseTrafficOut (ui32BytesOut);
                break;
            }
            case PT_UDP:
            {
                summaryUDPStats.increaseTrafficOut (ui32BytesOut);
                break;
            }
            case PT_ICMP:
            {
                summaryICMPStats.increaseTrafficOut (ui32BytesOut);
                break;
            }
        }
    }

    void GUIStatsManager::packSummaryStats (float fElapsedTimeInSeconds)
    {
        _MessagePacker.pack_int8 ((uint8) 'T');
        _MessagePacker.pack_uint32 (summaryTCPStats._ui32TrafficInBytes);
        _MessagePacker.pack_float (summaryTCPStats._ui32TrafficInRate / fElapsedTimeInSeconds);
        _MessagePacker.pack_uint32 (summaryTCPStats._ui32TrafficOutBytes);
        _MessagePacker.pack_float (summaryTCPStats._ui32TrafficOutRate / fElapsedTimeInSeconds);
        _MessagePacker.pack_uint16 (TCPConnTable::getTCPConnTable()->getActiveLocalConnectionsCount());

        _MessagePacker.pack_int8 ((uint8) 'U');
        _MessagePacker.pack_uint32 (summaryUDPStats._ui32TrafficInBytes);
        _MessagePacker.pack_float (summaryUDPStats._ui32TrafficInRate / fElapsedTimeInSeconds);
        _MessagePacker.pack_uint32 (summaryUDPStats._ui32TrafficOutBytes);
        _MessagePacker.pack_float (summaryUDPStats._ui32TrafficOutRate / fElapsedTimeInSeconds);

        _MessagePacker.pack_int8 ((uint8) 'I');
        _MessagePacker.pack_uint32 (summaryICMPStats._ui32TrafficInBytes);
        _MessagePacker.pack_float (summaryICMPStats._ui32TrafficInRate / fElapsedTimeInSeconds);
        _MessagePacker.pack_uint32 (summaryICMPStats._ui32TrafficOutBytes);
        _MessagePacker.pack_float (summaryICMPStats._ui32TrafficOutRate / fElapsedTimeInSeconds);
    }

    void GUIStatsManager::packDetailedStats (const ConnectorStats * const connectorStats, const ConnectionStats &connectionStats, float fElapsedTimeInSeconds)
    {
        _MessagePacker.pack_uint8 (protocolTypeToChar (connectionStats.getProtocolType()));
        _MessagePacker.pack_uint32 (connectionStats._ui32TrafficInBytes);
        _MessagePacker.pack_float (connectionStats._ui32TrafficInRate / fElapsedTimeInSeconds);
        _MessagePacker.pack_uint32 (connectionStats._ui32TrafficOutBytes);
        _MessagePacker.pack_float (connectionStats._ui32TrafficOutRate / fElapsedTimeInSeconds);

        if (connectionStats.getProtocolType() == PT_TCP) {
            _MessagePacker.pack_uint16 (TCPConnTable::getTCPConnTable()->getActiveLocalConnectionsCount (connectorStats->getRemoteHostIP(), connectorStats->getConnectorType()));
        }
    }
}
