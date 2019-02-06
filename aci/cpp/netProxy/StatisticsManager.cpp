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

#include <sstream>
#include <string>

#include "Logger.h"

#include "StatisticsManager.h"
#include "Utilities.h"
#include "ConfigurationParameters.h"
#include "subject.pb.h"
#include "TCPConnTable.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    ConnectorStats & ConnectorStats::operator= (ConnectorStats && cs)
    {
        _connectorType = std::move (cs._connectorType);
        _ui32RemoteProxyUniqueID = std::move (cs._ui32RemoteProxyUniqueID);
        _ui32RemoteProxyIP = std::move (cs._ui32RemoteProxyIP);
        _ui16RemoteProxyPort = std::move (cs._ui16RemoteProxyPort);
        _i64CreationTime = std::move (cs._i64CreationTime);
        _i64LastUpdateTime = std::move (cs._i64LastUpdateTime);
        _i64LastResetTime = std::move (cs._i64LastResetTime);

        return *this;
    }
    void ConnectorStats::increaseInboundTraffic (ProtocolType protocolType, uint32 ui32BytesIn)
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

        _i64LastUpdateTime = NOMADSUtil::getTimeInMilliseconds();
    }

    void ConnectorStats::increaseOutboundTraffic (ProtocolType protocolType, uint32 ui32BytesOut)
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

        _i64LastUpdateTime = NOMADSUtil::getTimeInMilliseconds();
    }

    void StatisticsManager::updateEdgeUUIDAndRemoteNetProxyUIDInMeasures (uint32 ui32CurrentRemoteProxyUID, uint32 ui32UpdatedRemoteProxyUID,
                                                                          const std::string & sCurrentEdgeUUID, const std::string & sUpdatedEdgeUUID,
                                                                          const std::string & sLocalInterfaceIP, const std::string & sRemoteInterfaceIP,
                                                                          uint16 ui16RemoteProxyPort, ConnectorType connectorType)
    {
        if ((ui32CurrentRemoteProxyUID == ui32UpdatedRemoteProxyUID) && (sCurrentEdgeUUID == sUpdatedEdgeUUID)) {
            // Nothing to do
            return;
        }

        {
            std::lock_guard<std::mutex> lg{_mtxNodesMap};
            if (_mmNodes.count (ui32CurrentRemoteProxyUID) == 1) {
                // Update is necessary
                auto & mNode = _mmNodes.at (ui32CurrentRemoteProxyUID);
                (*mNode.mutable_strings())["nodeUUID"] = "NP:" + std::to_string (ui32UpdatedRemoteProxyUID);
                std::swap (_mmNodes[ui32UpdatedRemoteProxyUID], _mmNodes[ui32CurrentRemoteProxyUID]);
                _mmNodes.erase (ui32CurrentRemoteProxyUID);
            }
            else if (_mmNodes.count (ui32UpdatedRemoteProxyUID) == 0) {
                // Need to add the new Node
                addNodeImpl (ui32UpdatedRemoteProxyUID);
            }
        }

        {
            std::lock_guard<std::mutex> lock{_mtxEdgesMap};
            if (_msmEdges.count (sCurrentEdgeUUID) == 1) {
                auto pEdgeMeasureStrings = _msmEdges.at (sCurrentEdgeUUID).first.mutable_strings();
                (*pEdgeMeasureStrings)["edge_uuid"] = sUpdatedEdgeUUID;
                (*pEdgeMeasureStrings)["endpoint2"] = "NP:" + std::to_string (ui32UpdatedRemoteProxyUID);
                (*pEdgeMeasureStrings)["status"] = "CONNECTED";
                std::swap (_msmEdges[sUpdatedEdgeUUID], _msmEdges[sCurrentEdgeUUID]);
                _msmEdges.erase (sCurrentEdgeUUID);
            }
            else if (_msmEdges.count (sUpdatedEdgeUUID) == 0) {
                // Need to create and add a new Edge
                addEdgeImpl (sUpdatedEdgeUUID, NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, ui32UpdatedRemoteProxyUID);
            }
        }

        {
            std::lock_guard<std::mutex> lg{_mtxLinkDescription};
            if (_msmLinkDescriptions.count (sCurrentEdgeUUID) == 1) {
                // Move LinkDescription to the updated position in the _msmLinkDescriptions using swap()
                (*_msmLinkDescriptions.at (sCurrentEdgeUUID).first.mutable_strings())["topology_edge_uuid"] = sUpdatedEdgeUUID;
                std::swap (_msmLinkDescriptions[sCurrentEdgeUUID], _msmLinkDescriptions[sUpdatedEdgeUUID]);
                _msmLinkDescriptions.erase (sCurrentEdgeUUID);
            }
            // Create the LinkDescription measure if it does not exist and make sure it is marked as active (connected = 1)
            activateLinkImpl (sUpdatedEdgeUUID, NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, sLocalInterfaceIP,
                              sRemoteInterfaceIP, ui16RemoteProxyPort, connectorType);
        }


        {
            std::lock_guard<std::mutex> lg{_mtxAddressMapping};
            for (auto & mAddressMappingRule : _vmAddressMappingRules) {
                // Update all entries in the AddressMappingRules map
                if (mAddressMappingRule.strings().at ("remote_np_uid") == std::to_string (ui32CurrentRemoteProxyUID)) {
                    (*mAddressMappingRule.mutable_strings())["remote_np_uid"] = std::to_string (ui32UpdatedRemoteProxyUID);
                }
            }
        }
    }

    std::unique_ptr<measure::Measure> StatisticsManager::getNextProcessMeasureToSend (int64 i64ReferenceTime)
    {
        // Process measure
        if ((i64ReferenceTime - getMeasureTimeInMillis (_mProcess)) >
            NetProxyApplicationParameters::I64_PROCESS_MEASURE_INTERVAL_IN_MS) {
            _mProcess.mutable_timestamp()->set_seconds (i64ReferenceTime / 1000);
            _mProcess.mutable_timestamp()->set_nanos ((i64ReferenceTime % 1000) * 1000);
            return std::unique_ptr<measure::Measure>{new measure::Measure{_mProcess}};
        }

        return std::unique_ptr<measure::Measure>{nullptr};
    }

    std::unique_ptr<measure::Measure> StatisticsManager::getNextConfigurationMeasureToSend (int64 i64ReferenceTime)
    {
        {
            // Address measure
            std::lock_guard<std::mutex> lg{_mtxAddressMapping};
            for (auto & mAddressRule : _vmAddressMappingRules) {
                if ((i64ReferenceTime - getMeasureTimeInMillis (mAddressRule)) >
                    NetProxyApplicationParameters::I64_ADDRESS_MEASURE_INTERVAL_IN_MS) {
                    mAddressRule.mutable_timestamp()->set_seconds (i64ReferenceTime / 1000);
                    mAddressRule.mutable_timestamp()->set_nanos ((i64ReferenceTime % 1000) * 1000);
                    return std::unique_ptr<measure::Measure>{new measure::Measure{mAddressRule}};
                }
            }
        }

        // Protocol measure
        for (auto & mProtocolRule : _vmProtocolMappingRules) {
            if ((i64ReferenceTime - getMeasureTimeInMillis (mProtocolRule)) >
                NetProxyApplicationParameters::I64_PROTOCOL_MEASURE_INTERVAL_IN_MS) {
                mProtocolRule.mutable_timestamp()->set_seconds (i64ReferenceTime / 1000);
                mProtocolRule.mutable_timestamp()->set_nanos ((i64ReferenceTime % 1000) * 1000);
                return std::unique_ptr<measure::Measure>{new measure::Measure{mProtocolRule}};
            }
        }

        return std::unique_ptr<measure::Measure>{nullptr};
    }

    std::unique_ptr<measure::Measure> StatisticsManager::getNextTopologyMeasureToSend (int64 i64ReferenceTime)
    {
        {
            // Topology Node measure
            std::lock_guard<std::mutex> lg{_mtxNodesMap};
            for (auto & ui32mpNode : _mmNodes) {
                if ((i64ReferenceTime - getMeasureTimeInMillis (ui32mpNode.second)) >
                    NetProxyApplicationParameters::I64_ADDRESS_MEASURE_INTERVAL_IN_MS) {
                    ui32mpNode.second.mutable_timestamp()->set_seconds (i64ReferenceTime / 1000);
                    ui32mpNode.second.mutable_timestamp()->set_nanos ((i64ReferenceTime % 1000) * 1000);
                    return std::unique_ptr<measure::Measure>{new measure::Measure{ui32mpNode.second}};
                }
            }
        }

        {
            // Topology Edge measure
            std::lock_guard<std::mutex> lg{_mtxEdgesMap};
            for (auto it = _msmEdges.begin(); it != _msmEdges.end(); ) {
                if ((it->second.first.strings().at ("status") == "DISCONNECTED") &&
                    ((i64ReferenceTime - it->second.second) >
                     NetProxyApplicationParameters::I64_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS)) {
                    _msmEdges.erase (it++);
                    continue;
                }
                if ((i64ReferenceTime - getMeasureTimeInMillis (it->second.first)) >
                    NetProxyApplicationParameters::I64_PROTOCOL_MEASURE_INTERVAL_IN_MS) {
                    it->second.first.mutable_timestamp()->set_seconds (i64ReferenceTime / 1000);
                    it->second.first.mutable_timestamp()->set_nanos ((i64ReferenceTime % 1000) * 1000);
                    return std::unique_ptr<measure::Measure>{new measure::Measure{it->second.first}};
                }
                ++it;
            }
        }

        return std::unique_ptr<measure::Measure>{nullptr};
    }

    std::unique_ptr<measure::Measure> StatisticsManager::getNextLinkDescriptionMeasureToSend (int64 i64ReferenceTime)
    {
        // Link description
        std::lock_guard<std::mutex> lg{_mtxLinkDescription};
        for (auto it = _msmLinkDescriptions.begin(); it != _msmLinkDescriptions.end(); ) {
            if ((it->second.first.integers().at ("connected") == 0) &&
                ((i64ReferenceTime - it->second.second) >
                 NetProxyApplicationParameters::I64_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS)) {
                _msmLinkDescriptions.erase (it++);
                continue;
            }
            if ((i64ReferenceTime - getMeasureTimeInMillis (it->second.first)) >
                NetProxyApplicationParameters::I64_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS) {
                it->second.first.mutable_timestamp()->set_seconds (i64ReferenceTime / 1000);
                it->second.first.mutable_timestamp()->set_nanos ((i64ReferenceTime % 1000) * 1000);
                return std::unique_ptr<measure::Measure>{new measure::Measure{it->second.first}};
            }
            ++it;
        }

        return std::unique_ptr<measure::Measure>{nullptr};
    }

    std::unique_ptr<measure::Measure> StatisticsManager::getNextLinkTrafficMeasureToSend (int64 i64ReferenceTime)
    {
        // Link traffic
        std::lock_guard<std::mutex> lg{_mtxConnectorStatsMap};
        for (auto & pmKeyConnectorStats : _mmConnectorStats) {
            for (auto & pKeyConnectorStats : pmKeyConnectorStats.second) {
                if ((i64ReferenceTime - pKeyConnectorStats.second.getLastResetTime()) >
                    NetProxyApplicationParameters::I64_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS) {
                    buildLinkTrafficMeasure (_mLinkTraffic, pKeyConnectorStats.second);
                    pKeyConnectorStats.second.resetRate();
                    return std::unique_ptr<measure::Measure>{new measure::Measure{_mLinkTraffic}};
                }
            }
        }

        return nullptr;
    }

    // Assumes _mtxConnectorStatsMap lock is acquired
    ConnectorStats & StatisticsManager::getConnectorStatistics (ConnectorType connectorType, uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteProxyIP,
                                                                uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort)
    {
        const auto ui64FirstKey = generateUInt64Key (ui32LocalInterfaceIP, connectorType);
        const auto ui64SecondKey = generateUInt64Key (ui32RemoteProxyUniqueID, ui32RemoteProxyIP);
        if ((_mmConnectorStats.count (ui64FirstKey) == 0) || (_mmConnectorStats.at (ui64FirstKey).count (ui64SecondKey) == 0)) {
            _mmConnectorStats[ui64FirstKey][ui64SecondKey] = ConnectorStats{connectorType, ui32RemoteProxyUniqueID, ui32RemoteProxyIP,
                                                                            ui16RemoteProxyPort, ui32LocalInterfaceIP, ui16LocalProxyPort};
            checkAndLogMsg ("StatisticsManager::registerStatisticsIfNecessary", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "successfully registered new statistics for %s traffic sent to the remote NetProxy with UniqueID %u, "
                            "reachable at address <%s:%hu>, from the local address <%s:%hu>\n", connectorTypeToString (connectorType),
                            ui32RemoteProxyUniqueID, NOMADSUtil::InetAddr{ui32RemoteProxyIP}.getIPAsString(), ui16RemoteProxyPort,
                            NOMADSUtil::InetAddr{ui32LocalInterfaceIP}.getIPAsString(), ui16LocalProxyPort);
        }

        return _mmConnectorStats[ui64FirstKey][ui64SecondKey];
    }

    void StatisticsManager::activateLinkImpl (const std::string & sEdgeUUID, uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP,
                                              const std::string & sRemoteInterfaceIP, uint16 ui16RemoteProxyPort, ConnectorType connectorType)
    {
        if (_msmLinkDescriptions.count (sEdgeUUID) == 0) {
            // Link not in the map
            measure::Measure mLinkDescription;
            buildLinkDescriptionMeasure (mLinkDescription, sLocalInterfaceIP, sRemoteInterfaceIP, ui16RemoteProxyPort, connectorType, sEdgeUUID);
            _msmLinkDescriptions[sEdgeUUID] = std::make_pair (std::move (mLinkDescription), NOMADSUtil::getTimeInMilliseconds());
        }

        auto & mIntegersMap = *(_msmLinkDescriptions[sEdgeUUID].first.mutable_integers());
        if (mIntegersMap["connected"] == 0) {
            // (Re-)activate link
            google::protobuf::uint64 linkActivationTime = NOMADSUtil::getTimeInMilliseconds();
            mIntegersMap["connected"] = 1;
            mIntegersMap["started_seconds"] = linkActivationTime / 1000;
            mIntegersMap["started_nanos"] = (linkActivationTime % 1000) * 1000;
        }
    }

    void StatisticsManager::deactivateEdge (const std::string & sEdgeUUID)
    {
        std::lock_guard<std::mutex> lg{_mtxEdgesMap};

        if (_msmEdges.count (sEdgeUUID) == 1) {
            auto & edgeMeasure = _msmEdges.at (sEdgeUUID);
            edgeMeasure.second = NOMADSUtil::getTimeInMilliseconds();
            (*edgeMeasure.first.mutable_strings())["status"] = "DISCONNECTED";
        }
    }

    void StatisticsManager::deactivateLink (const std::string & sEdgeUUID)
    {
        std::lock_guard<std::mutex> lg{_mtxLinkDescription};
        if (_msmLinkDescriptions.count (sEdgeUUID) == 1) {
            _msmLinkDescriptions.at (sEdgeUUID).second = NOMADSUtil::getTimeInMilliseconds();
            auto & mIntegersMap = *_msmLinkDescriptions.at (sEdgeUUID).first.mutable_integers();
            mIntegersMap["connected"] = 0;
            mIntegersMap["started_seconds"] = -1;
            mIntegersMap["started_nanos"] = -1;
        }
    }

    // This method does not need to lock as it is called only at startup, after parsing the configuration files
    void StatisticsManager::updateConfigurationMeasures (void)
    {
        // Address measure
        for (auto & mAddressRule : _vmAddressMappingRules) {
            auto & string_map = *mAddressRule.mutable_strings();
            string_map["sensor_ip"] =
                NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
            string_map["np_uid"] = std::to_string (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        }

        // Protocol measure
        for (auto & mProtocolRule : _vmProtocolMappingRules) {
            auto & string_map = *mProtocolRule.mutable_strings();
            string_map["sensor_ip"] =
                NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
            string_map["np_uid"] = std::to_string (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        }
    }

    void StatisticsManager::buildProcessMeasure (measure::Measure & m)
    {
        m.set_subject (measure::Subject::netproxy_process);

        m.mutable_timestamp()->set_seconds (0);
        m.mutable_timestamp()->set_nanos (0);

        auto & string_map = *m.mutable_strings();
        string_map["sensor_ip"] =
            NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
        string_map["process_host_ip"] =
            NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
        string_map["process_name"] = "NetProxy";
        string_map["uid"] = std::to_string (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        string_map["status"] = "STARTED";

        std::ostringstream desc_oss;
        desc_oss << (NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE ? "Tunneling" : "Proxying") << " traffic in " <<
            (NetProxyApplicationParameters::GATEWAY_MODE ? "Gateway" : "Host") << " Mode.";
        string_map["Description"] = desc_oss.str();

        google::protobuf::uint64 startUpTime = NetProxyApplicationParameters::UI64_STARTUP_TIME_IN_MILLISECONDS;
        auto & integer_map = *m.mutable_integers();
        integer_map["started_seconds"] = startUpTime / 1000;
        integer_map["started_nanos"] = (startUpTime % 1000) * 1000;
    }

    void StatisticsManager::buildNodeMeasure (measure::Measure & m, uint32 ui32NodeUUID)
    {
        m.set_subject (measure::Subject::topology_node);

        m.mutable_timestamp()->set_seconds (0);
        m.mutable_timestamp()->set_nanos (0);

        auto & string_map = *m.mutable_strings();
        string_map["sensor_ip"] =
            NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
        string_map["topology_id"] = "NetProxy";
        string_map["nodeUUID"] = "NP:" + std::to_string (ui32NodeUUID);
    }

    void StatisticsManager::buildEdgeMeasure (measure::Measure & m, const std::string & sEdgeUUID, uint32 ui32NodeUUID1, uint32 ui32NodeUUID2)
    {
        m.set_subject (measure::Subject::topology_edge);

        m.mutable_timestamp()->set_seconds (0);
        m.mutable_timestamp()->set_nanos (0);

        auto & string_map = *m.mutable_strings();
        string_map["sensor_ip"] =
            NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
        string_map["topology_id"] = "NetProxy";
        string_map["edge_uuid"] = sEdgeUUID;
        string_map["endpoint1"] = "NP:" + std::to_string (ui32NodeUUID1);
        string_map["endpoint2"] = "NP:" + std::to_string (ui32NodeUUID2);
        string_map["status"] = "CONNECTED";
    }

    void StatisticsManager::buildLinkDescriptionMeasure (measure::Measure & m, const std::string & sLocalInterfaceIP, const std::string & sRemoteInterfaceIP,
                                                         uint16 ui16RemoteNetProxyPort, ConnectorType connectorType, const std::string & sEdgeUUID)
    {
        m.set_subject (measure::Subject::netproxy_link_desc);

        m.mutable_timestamp()->set_seconds (0);
        m.mutable_timestamp()->set_nanos (0);

        auto & string_map = *m.mutable_strings();
        string_map["sensor_ip"] =
            NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
        string_map["np_uid"] = std::to_string (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        string_map["endpoint_local_ip"] = sLocalInterfaceIP;
        string_map["endpoint_remote_ip"] = sRemoteInterfaceIP;
        string_map["endpoint_remote_port"] = std::to_string (ui16RemoteNetProxyPort);
        string_map["connector_name"] = connectorTypeToString (connectorType);
        string_map["topology_edge_uuid"] = sEdgeUUID;

        google::protobuf::uint64 linkActivationTime = NOMADSUtil::getTimeInMilliseconds();
        auto & integer_map = *m.mutable_integers();
        integer_map["connected"] = 1;
        integer_map["started_seconds"] = linkActivationTime / 1000;
        integer_map["started_nanos"] = (linkActivationTime % 1000) * 1000;
    }

    void StatisticsManager::buildLinkTrafficMeasure (measure::Measure & m, const ConnectorStats & connectorStats)
    {
        m.set_subject (measure::Subject::netproxy_link_traffic);

        google::protobuf::uint64 linkActivationTime = NOMADSUtil::getTimeInMilliseconds();
        m.mutable_timestamp()->set_seconds (linkActivationTime / 1000);
        m.mutable_timestamp()->set_nanos ((linkActivationTime % 1000) * 1000);

        auto & string_map = *m.mutable_strings();
        string_map["sensor_ip"] =
            NOMADSUtil::InetAddr{NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address}.getIPAsString();
        string_map["np_uid"] = std::to_string (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        string_map["endpoint_local_ip"] = NOMADSUtil::InetAddr{connectorStats.getLocalInterfaceIP()}.getIPAsString();
        string_map["endpoint_remote_ip"] = NOMADSUtil::InetAddr{connectorStats.getRemoteProxyIP()}.getIPAsString();
        string_map["endpoint_remote_port"] = std::to_string (connectorStats.getRemoteProxyPort());
        string_map["connector_name"] = connectorTypeToString (connectorStats.getConnectorType());

        auto & integer_map = *m.mutable_integers();
        integer_map["total_bytes_snt"] = connectorStats.getTotalOutboundBytes();
        integer_map["total_bytes_rcvd"] = connectorStats.getTotalInboundBytes();
        integer_map["start_seconds"] = connectorStats.getCreationTime() / 1000;
        integer_map["start_nanos"] = (connectorStats.getCreationTime() % 1000) * 1000;
        integer_map["interval_bytes_snt"] = connectorStats.getRateOutboundBytes();
        integer_map["interval_bytes_rcvd"] = connectorStats.getRateInboundBytes();
        integer_map["interval_start_seconds"] = connectorStats.getLastResetTime() / 1000;
        integer_map["interval_start_nanos"] = (connectorStats.getLastResetTime() % 1000) * 1000;
        integer_map["interval_end_seconds"] = connectorStats.getLastUpdateTime() / 1000;
        integer_map["interval_end_nanos"] = (connectorStats.getLastUpdateTime() % 1000) * 1000;
    }

}
