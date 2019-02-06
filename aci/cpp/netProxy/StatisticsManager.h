#ifndef INCL_GUI_UPDATE_MESSAGE_H
#define INCL_GUI_UPDATE_MESSAGE_H

/*
 * GUIUpdateMessage.h
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
 * This files defines useful structs and classes that
 * the NetProxy uses to build the GUIUpdate Messages.
 */

#include <mutex>
#include <vector>

#include "FTypes.h"
#include "UInt64Hashtable.h"

#include "ConfigurationParameters.h"
#include "measure.pb.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class ProtocolStats
    {
    private:
        friend class ConnectorStats;

        ProtocolStats (const ProtocolType protocolType);

        const ProtocolType getProtocolType (void) const;
        char getProtocolTypeAsChar (void) const;

        void resetRate (void);
        void increaseTrafficIn (uint32 ui32BytesIn);
        void increaseTrafficOut (uint32 ui32BytesOut);

        const ProtocolType _protocolType;
        uint64 _ui64TrafficInBytes;
        uint64 _ui64TrafficInRate;
        uint64 _ui64TrafficOutBytes;
        uint64 _ui64TrafficOutRate;
    };


    class ConnectorStats
    {
    public:
        ConnectorStats (void);
        ConnectorStats (const ConnectorStats & cs);
        ConnectorStats (ConnectorStats && cs);
        ConnectorStats (ConnectorType connType, uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteProxyIP,
                        uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort);

        ConnectorStats & operator= (const ConnectorStats & cs);
        ConnectorStats & operator= (ConnectorStats && cs);

        uint64 getTotalInboundBytes (void) const;
        uint64 getTotalOutboundBytes (void) const;
        uint64 getRateInboundBytes (void) const;
        uint64 getRateOutboundBytes (void) const;

        ConnectorType getConnectorType (void) const;
        char getConnectorTypeAsChar (void) const;
        uint32 getRemoteProxyUniqueID (void) const;
        uint32 getRemoteProxyIP (void) const;
        uint16 getRemoteProxyPort (void) const;
        uint32 getLocalInterfaceIP (void) const;
        uint16 getLocalProxyPort (void) const;
        int64 getCreationTime (void) const;
        int64 getLastUpdateTime (void) const;
        int64 getLastResetTime (void) const;

        void increaseInboundTraffic (const ProtocolType protocolType, uint32 ui32BytesIn);
        void increaseOutboundTraffic (const ProtocolType protocolType, uint32 ui32BytesOut);

        void resetRate (void);


    private:
        ConnectorType _connectorType;
        uint32 _ui32RemoteProxyUniqueID;
        uint32 _ui32RemoteProxyIP;
        uint16 _ui16RemoteProxyPort;
        uint32 _ui32LocalInterfaceIP;
        uint16 _ui16LocalProxyPort;
        int64 _i64CreationTime;
        int64 _i64LastUpdateTime;
        int64 _i64LastResetTime;

        ProtocolStats _detailedTCPStats;
        ProtocolStats _detailedUDPStats;
        ProtocolStats _detailedICMPStats;
    };


    class StatisticsManager
    {
    public:
        StatisticsManager (void);
        ~StatisticsManager (void);

        void init (void);

        void addAddressMappingRule (measure::Measure && mAMR);
        void addProtocolMappingRule (measure::Measure && mPMR);
        void addNode (uint32 ui32NodeUUID);
        void addEdgeAndActivateLink (uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP, uint16 ui16LocalProxyPort,
                                     uint32 ui32RemoteNetProxyUID, const std::string & sRemoteInterfaceIP,
                                     uint16 ui16RemoteProxyPort, ConnectorType connectorType);
        void deactivateEdgeAndLink (const std::string & sEdgeUUID);
        void updateEdgeUUIDAndRemoteNetProxyUIDInMeasures (uint32 ui32CurrentRemoteProxyUID, uint32 ui32UpdatedRemoteProxyUID,
                                                           const std::string & sCurrentEdgeUUID, const std::string & sUpdatedEdgeUUID,
                                                           const std::string & sLocalInterfaceIP, const std::string & sRemoteInterfaceIP,
                                                           uint16 ui16RemoteProxyPort, ConnectorType connectorType);

        void increaseInboundTraffic (ConnectorType connectorType, uint32 ui32RemotePreoxyUniqueID, uint32 ui32RemoteProxyIP,
                                     uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort,
                                     ProtocolType protocolType, uint32 ui32BytesIn);
        void increaseOutboundTraffic (ConnectorType connectorType, uint32 ui32RemotePreoxyUniqueID, uint32 ui32RemoteProxyIP,
                                      uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort,
                                      ProtocolType protocolType, uint32 ui32BytesOut);
        void deleteStats (ConnectorType connectorType, uint32 ui32RemoteProxyUniqueID);

        std::unique_ptr<measure::Measure> getNextProcessMeasureToSend (int64 i64ReferenceTime);
        std::unique_ptr<measure::Measure> getNextConfigurationMeasureToSend (int64 i64ReferenceTime);
        std::unique_ptr<measure::Measure> getNextTopologyMeasureToSend (int64 i64ReferenceTime);
        std::unique_ptr<measure::Measure> getNextLinkDescriptionMeasureToSend (int64 i64ReferenceTime);
        std::unique_ptr<measure::Measure> getNextLinkTrafficMeasureToSend (int64 i64ReferenceTime);

        static std::string buildEdgeUUID (uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP, uint16 ui16LocalPortNum,
                                          uint32 ui32RemoteNetProxyUID, const std::string & sRemoteInterfaceIP,
                                          uint16 ui16RemotePortNum, ConnectorType connectorType);


    private:
        ConnectorStats & getConnectorStatistics (ConnectorType connectorType, uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteProxyIP,
                                                 uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort);

        void addNodeImpl (uint32 ui32NodeUUID);
        void addEdge (const std::string & sEdgeUUID, uint32 ui32NodeUUID1, uint32 ui32NodeUUID2);
        void addEdgeImpl (const std::string & sEdgeUUID, uint32 ui32NodeUUID1, uint32 ui32NodeUUID2);
        void activateLink (const std::string & sEdgeUUID, uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP,
                           const std::string & sRemoteInterfaceIP, uint16 ui16RemoteProxyPort, ConnectorType connectorType);
        void activateLinkImpl (const std::string & sEdgeUUID, uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP,
                               const std::string & sRemoteInterfaceIP, uint16 ui16RemoteProxyPort, ConnectorType connectorType);
        void deactivateEdge (const std::string & sEdgeUUID);
        void deactivateLink (const std::string & sEdgeUUID);
        void updateConfigurationMeasures (void);

        static void buildProcessMeasure (measure::Measure & m);
        static void buildNodeMeasure (measure::Measure & m, uint32 ui32NetProxyUID);
        static void buildEdgeMeasure (measure::Measure & m, const std::string & sEdgeUUID, uint32 ui32NodeUUID1, uint32 ui32NodeUUID2);
        static void buildLinkDescriptionMeasure (measure::Measure & m, const std::string & sLocalInterfaceIP, const std::string & sRemoteInterfaceIP,
                                                 uint16 ui16RemoteNetProxyPort, ConnectorType connectorType, const std::string & sEdgeUUID);
        static void buildLinkTrafficMeasure (measure::Measure & m, const ConnectorStats & connectorStats);

        static int64 getMeasureTimeInMillis (const measure::Measure & m);


        std::map<uint64, std::map<uint64, ConnectorStats>> _mmConnectorStats;

        measure::Measure _mProcess;
        std::vector<measure::Measure> _vmAddressMappingRules;
        std::vector<measure::Measure> _vmProtocolMappingRules;
        std::map<uint32, measure::Measure> _mmNodes;
        std::map<std::string, std::pair<measure::Measure, int64>> _msmEdges;                    // Pair contains the measure instance and the de-/activation time
        std::map<std::string, std::pair<measure::Measure, int64>> _msmLinkDescriptions;         // Pair contains the measure instance and the de-/activation time
        measure::Measure _mLinkTraffic;

        mutable std::mutex _mtxAddressMapping;
        mutable std::mutex _mtxNodesMap;
        mutable std::mutex _mtxEdgesMap;
        mutable std::mutex _mtxLinkDescription;
        mutable std::mutex _mtxConnectorStatsMap;
    };


    inline ProtocolStats::ProtocolStats (const ProtocolType protocolType) :
        _protocolType{protocolType}, _ui64TrafficInBytes{0}, _ui64TrafficInRate{0},
        _ui64TrafficOutBytes{0}, _ui64TrafficOutRate{0}
    { }

    inline const ProtocolType ProtocolStats::getProtocolType (void) const
    {
        return _protocolType;
    }

    inline char ProtocolStats::getProtocolTypeAsChar (void) const
    {
        return protocolTypeToChar (_protocolType);
    }

    inline void ProtocolStats::resetRate (void)
    {
        _ui64TrafficInRate = 0;
        _ui64TrafficOutRate = 0;
    }

    inline void ProtocolStats::increaseTrafficIn (uint32 ui32BytesIn)
    {
        _ui64TrafficInBytes += ui32BytesIn;
        _ui64TrafficInRate += ui32BytesIn;
    }

    inline void ProtocolStats::increaseTrafficOut (uint32 ui32BytesOut)
    {
        _ui64TrafficOutBytes += ui32BytesOut;
        _ui64TrafficOutRate += ui32BytesOut;
    }

    inline ConnectorStats::ConnectorStats (void) :
        _connectorType(CT_UNDEF), _ui32RemoteProxyUniqueID(0), _ui32RemoteProxyIP(0), _ui16RemoteProxyPort(0),
        _ui32LocalInterfaceIP{0}, _ui16LocalProxyPort{0}, _i64CreationTime(0), _i64LastUpdateTime{0},
        _i64LastResetTime(0), _detailedTCPStats(PT_TCP), _detailedUDPStats(PT_UDP), _detailedICMPStats(PT_ICMP) { }

    inline ConnectorStats::ConnectorStats (const ConnectorType connType, uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteProxyIP,
                                           uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort) :
        _connectorType{connType}, _ui32RemoteProxyUniqueID{ui32RemoteProxyUniqueID}, _ui32RemoteProxyIP{ui32RemoteProxyIP},
        _ui16RemoteProxyPort{ui16RemoteProxyPort}, _ui32LocalInterfaceIP{ui32LocalInterfaceIP}, _ui16LocalProxyPort{ui16LocalProxyPort},
        _i64CreationTime{NOMADSUtil::getTimeInMilliseconds()}, _i64LastUpdateTime{_i64CreationTime}, _i64LastResetTime{_i64CreationTime},
        _detailedTCPStats{PT_TCP}, _detailedUDPStats{PT_UDP}, _detailedICMPStats{PT_ICMP} { }

    inline ConnectorStats::ConnectorStats (const ConnectorStats & cs) :
        _connectorType{cs._connectorType}, _ui32RemoteProxyUniqueID{cs._ui32RemoteProxyUniqueID}, _ui32RemoteProxyIP{cs._ui32RemoteProxyIP},
        _ui16RemoteProxyPort{cs._ui16RemoteProxyPort}, _ui32LocalInterfaceIP{cs._ui32LocalInterfaceIP},
        _ui16LocalProxyPort{cs._ui16LocalProxyPort}, _i64CreationTime{cs._i64CreationTime}, _i64LastUpdateTime{cs._i64LastUpdateTime},
        _i64LastResetTime{cs._i64LastResetTime}, _detailedTCPStats{PT_TCP}, _detailedUDPStats{PT_UDP}, _detailedICMPStats{PT_ICMP} { }

    inline ConnectorStats::ConnectorStats (ConnectorStats && cs) :
        _connectorType{cs._connectorType}, _ui32RemoteProxyUniqueID{cs._ui32RemoteProxyUniqueID}, _ui32RemoteProxyIP{cs._ui32RemoteProxyIP},
        _ui16RemoteProxyPort{cs._ui16RemoteProxyPort}, _ui32LocalInterfaceIP{cs._ui32LocalInterfaceIP},
        _ui16LocalProxyPort{cs._ui16LocalProxyPort}, _i64CreationTime{cs._i64CreationTime}, _i64LastUpdateTime{cs._i64LastUpdateTime},
        _i64LastResetTime{cs._i64LastResetTime}, _detailedTCPStats{PT_TCP}, _detailedUDPStats{PT_UDP}, _detailedICMPStats{PT_ICMP}
    {
        cs = ConnectorStats{};
    }

    inline ConnectorStats & ConnectorStats::operator= (const ConnectorStats & cs)
    {
        _connectorType = cs._connectorType;
        _ui32RemoteProxyUniqueID = cs._ui32RemoteProxyUniqueID;
        _ui32RemoteProxyIP = cs._ui32RemoteProxyIP;
        _ui16RemoteProxyPort = cs._ui16RemoteProxyPort;
        _i64CreationTime = cs._i64CreationTime;
        _i64LastUpdateTime = cs._i64LastUpdateTime;
        _i64LastResetTime = cs._i64LastResetTime;
        _detailedTCPStats.resetRate();
        _detailedUDPStats.resetRate();
        _detailedICMPStats.resetRate();

        return *this;
    }

    inline uint64 ConnectorStats::getTotalInboundBytes (void) const
    {
        return _detailedTCPStats._ui64TrafficInBytes + _detailedUDPStats._ui64TrafficInBytes + _detailedICMPStats._ui64TrafficInBytes;
    }

    inline uint64 ConnectorStats::getTotalOutboundBytes (void) const
    {
        return _detailedTCPStats._ui64TrafficOutBytes + _detailedUDPStats._ui64TrafficOutBytes + _detailedICMPStats._ui64TrafficOutBytes;
    }

    inline uint64 ConnectorStats::getRateInboundBytes (void) const
    {
        return _detailedTCPStats._ui64TrafficInRate + _detailedUDPStats._ui64TrafficInRate + _detailedICMPStats._ui64TrafficInRate;
    }

    inline uint64 ConnectorStats::getRateOutboundBytes (void) const
    {
        return _detailedTCPStats._ui64TrafficOutRate + _detailedUDPStats._ui64TrafficOutRate + _detailedICMPStats._ui64TrafficOutRate;
    }

    inline ConnectorType ConnectorStats::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline char ConnectorStats::getConnectorTypeAsChar (void) const
    {
        return connectorTypeToChar (_connectorType);
    }

    inline uint32 ConnectorStats::getRemoteProxyUniqueID (void) const
    {
        return _ui32RemoteProxyUniqueID;
    }

    inline uint32 ConnectorStats::getRemoteProxyIP (void) const
    {
        return _ui32RemoteProxyIP;
    }

    inline uint16 ConnectorStats::getRemoteProxyPort (void) const
    {
        return _ui16RemoteProxyPort;
    }

    inline uint32 ConnectorStats::getLocalInterfaceIP (void) const
    {
        return _ui32LocalInterfaceIP;
    }

    inline uint16 ConnectorStats::getLocalProxyPort (void) const
    {
        return _ui16LocalProxyPort;
    }

    inline int64 ConnectorStats::getCreationTime (void) const
    {
        return _i64CreationTime;
    }

    inline int64 ConnectorStats::getLastUpdateTime (void) const
    {
        return _i64LastUpdateTime;
    }

    inline int64 ConnectorStats::getLastResetTime (void) const
    {
        return _i64LastResetTime;
    }

    inline void ConnectorStats::resetRate (void)
    {
        _detailedTCPStats.resetRate();
        _detailedUDPStats.resetRate();
        _detailedICMPStats.resetRate();

        _i64LastResetTime = NOMADSUtil::getTimeInMilliseconds();
    }

    inline StatisticsManager::StatisticsManager (void) { }

    inline StatisticsManager::~StatisticsManager (void) { }

    inline void StatisticsManager::init (void)
    {
        buildProcessMeasure (_mProcess);
        updateConfigurationMeasures();
    }

    inline void StatisticsManager::addAddressMappingRule (measure::Measure && mAMR)
    {
        _vmAddressMappingRules.push_back (std::move (mAMR));
    }

    inline void StatisticsManager::addProtocolMappingRule (measure::Measure && mPMR)
    {
        _vmProtocolMappingRules.push_back (std::move (mPMR));
    }

    inline void StatisticsManager::addNode (uint32 ui32NodeUUID)
    {
        std::lock_guard<std::mutex> lg{_mtxNodesMap};

        addNodeImpl (ui32NodeUUID);
    }

    inline void StatisticsManager::addEdgeAndActivateLink (uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP, uint16 ui16LocalProxyPort,
                                                           uint32 ui32RemoteNetProxyUID, const std::string & sRemoteInterfaceIP,
                                                           uint16 ui16RemoteProxyPort, ConnectorType connectorType)
    {
        auto sEdgeUUID = buildEdgeUUID (ui32LocalNetProxyUID, sLocalInterfaceIP, ui16LocalProxyPort, ui32RemoteNetProxyUID,
                                        sRemoteInterfaceIP, ui16RemoteProxyPort, connectorType);

        addEdge (sEdgeUUID, ui32LocalNetProxyUID, ui32RemoteNetProxyUID);
        activateLink (sEdgeUUID, ui32LocalNetProxyUID, sLocalInterfaceIP, sRemoteInterfaceIP, ui16RemoteProxyPort, connectorType);
    }

    inline void StatisticsManager::deactivateEdgeAndLink (const std::string & sEdgeUUID)
    {
        if (sEdgeUUID.length() > 0) {
            deactivateEdge (sEdgeUUID);
            deactivateLink (sEdgeUUID);
        }
    }

    inline void StatisticsManager::increaseInboundTraffic (ConnectorType connectorType, uint32 ui32RemotePreoxyUniqueID, uint32 ui32RemoteProxyIP,
                                                           uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort,
                                                           ProtocolType protocolType, uint32 ui32BytesIn)
    {
        if (NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES && (connectorType != CT_UNDEF) &&
            (ui32RemotePreoxyUniqueID != 0) && (ui16RemoteProxyPort != 0)) {
            std::lock_guard<std::mutex> lock (_mtxConnectorStatsMap);
            auto & connectorStats = getConnectorStatistics (connectorType, ui32RemotePreoxyUniqueID, ui32RemoteProxyIP,
                                                            ui16RemoteProxyPort, ui32LocalInterfaceIP, ui16LocalProxyPort);
            connectorStats.increaseInboundTraffic (protocolType, ui32BytesIn);
        }
    }

    inline void StatisticsManager::increaseOutboundTraffic (ConnectorType connectorType, uint32 ui32RemotePreoxyUniqueID, uint32 ui32RemoteProxyIP,
                                                            uint16 ui16RemoteProxyPort, uint32 ui32LocalInterfaceIP, uint16 ui16LocalProxyPort,
                                                            ProtocolType protocolType, uint32 ui32BytesOut)
    {
        if (NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES && (connectorType != CT_UNDEF) &&
            (ui32RemotePreoxyUniqueID != 0) && (ui16RemoteProxyPort != 0)) {
            std::lock_guard<std::mutex> lock (_mtxConnectorStatsMap);
            auto & connectorStats = getConnectorStatistics (connectorType, ui32RemotePreoxyUniqueID, ui32RemoteProxyIP,
                                                            ui16RemoteProxyPort, ui32LocalInterfaceIP, ui16LocalProxyPort);
            connectorStats.increaseOutboundTraffic (protocolType, ui32BytesOut);
        }
    }

    inline void StatisticsManager::deleteStats (ConnectorType connectorType, uint32 ui32RemoteProxyUniqueID)
    {
        if (NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES &&
            (ui32RemoteProxyUniqueID != 0) && (connectorType != CT_UNDEF)) {
            std::lock_guard<std::mutex> lock (_mtxConnectorStatsMap);
            _mmConnectorStats.erase (generateUInt64Key (ui32RemoteProxyUniqueID, connectorType));
        }
    }

    inline std::string StatisticsManager::buildEdgeUUID (uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP, uint16 ui16LocalPortNum,
                                                         uint32 ui32RemoteNetProxyUID, const std::string & sRemoteInterfaceIP,
                                                         uint16 ui16RemotePortNum, ConnectorType connectorType)
    {
        return std::to_string (ui32LocalNetProxyUID) + ":" + sLocalInterfaceIP + ":" + std::to_string (ui16LocalPortNum) + "-" +
            std::to_string (ui32RemoteNetProxyUID) + ":" + sRemoteInterfaceIP + ":" +
            std::to_string (ui16RemotePortNum) + "-" + connectorTypeToString (connectorType);
    }

    inline void StatisticsManager::addNodeImpl (uint32 ui32NodeUUID)
    {
        measure::Measure mNode;
        buildNodeMeasure (mNode, ui32NodeUUID);

        _mmNodes[ui32NodeUUID] = std::move (mNode);
    }

    inline void StatisticsManager::addEdge (const std::string & sEdgeUUID, uint32 ui32NodeUUID1, uint32 ui32NodeUUID2)
    {
        std::lock_guard<std::mutex> lg{_mtxEdgesMap};
        addEdgeImpl (sEdgeUUID, ui32NodeUUID1, ui32NodeUUID2);
    }

    inline void StatisticsManager::addEdgeImpl (const std::string & sEdgeUUID, uint32 ui32NodeUUID1, uint32 ui32NodeUUID2)
    {
        measure::Measure mEdge;
        buildEdgeMeasure (mEdge, sEdgeUUID, ui32NodeUUID1, ui32NodeUUID2);
        _msmEdges[sEdgeUUID] = std::make_pair (std::move (mEdge), NOMADSUtil::getTimeInMilliseconds());
    }

    inline void StatisticsManager::activateLink (const std::string & sEdgeUUID, uint32 ui32LocalNetProxyUID, const std::string & sLocalInterfaceIP,
                                                 const std::string & sRemoteInterfaceIP, uint16 ui16RemoteProxyPort, ConnectorType connectorType)
    {
        std::lock_guard<std::mutex> lg{_mtxLinkDescription};
        activateLinkImpl (sEdgeUUID, ui32LocalNetProxyUID, sLocalInterfaceIP, sRemoteInterfaceIP, ui16RemoteProxyPort, connectorType);
    }

    inline int64 StatisticsManager::getMeasureTimeInMillis (const measure::Measure & m)
    {
        int64 res = m.timestamp().seconds() * 1000;
        return res + (m.timestamp().nanos() / 1000);
    }

}


#endif  // INCL_GUI_UPDATE_MESSAGE_H
