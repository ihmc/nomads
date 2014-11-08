#ifndef INCL_GUI_UPDATE_MESSAGE_H
#define INCL_GUI_UPDATE_MESSAGE_H

/*
 * GUIUpdateMessage.h
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
 *
 * This files defines useful structs and classes that
 * the NetProxy uses to build the GUIUpdate Messages.
 */

#include "FTypes.h"
#include "UInt64Hashtable.h"
#include "Mutex.h"
#include "msgpack.hpp"

#include "Utilities.h"
#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    class GUIStatsManager;

    class ConnectionStats
    {
    private:
        friend class ConnectorStats;
        friend class GUIStatsManager;

        ConnectionStats (const ProtocolType protocolType);
        const ProtocolType getProtocolType (void) const;
        char getProtocolTypeAsChar (void) const;

        void resetRate (void);
        void increaseTrafficIn (uint32 ui32BytesIn);
        void increaseTrafficOut (uint32 ui32BytesOut);

        const ProtocolType _protocolType;
        uint32 _ui32TrafficInBytes;
        uint32 _ui32TrafficInRate;
        uint32 _ui32TrafficOutBytes;
        uint32 _ui32TrafficOutRate;
    };


    class ConnectorStats
    {
    public:
        ConnectorStats (ConnectorType connType, uint32 ui32RemoteHostIP, uint32 ui32RemoteProxyUniqueID, uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort);

        ConnectorType getConnectorType (void) const;
        char getConnectorTypeAsChar (void) const;
        uint32 getRemoteHostIP (void) const;
        uint32 getRemoteProxyUniqueID (void) const;
        uint32 getRemoteProxyIP (void) const;
        uint16 getRemoteProxyPort (void) const;
        int64 getCreationTime (void) const;
        int64 getLastUpdateTime (void) const;

        void resetRate (void);


    private:
        friend class GUIStatsManager;

        void increaseTrafficIn (ProtocolType protocolType, uint32 ui32BytesIn);
        void increaseTrafficOut (ProtocolType protocolType, uint32 ui32BytesOut);

        int lock (void);
        int tryLock (void);
        int unlock (void);

        const ConnectorType _connectorType;
        const uint32 _ui32RemoteHostIP;
        const uint32 _ui32RemoteProxyUniqueID;
        const uint32 _ui32RemoteProxyIP;
        const uint16 _ui16RemoteProxyPort;
        const int64 _i64CreationTime;
        int64 _i64LastUpdateTime;

        ConnectionStats _detailedTCPStats;
        ConnectionStats _detailedUDPStats;
        ConnectionStats _detailedICMPStats;

        NOMADSUtil::Mutex _m;
    };


    class GUIStatsManager
    {
    public:
        ~GUIStatsManager (void);
        static GUIStatsManager * const getGUIStatsManager (void);

        void increaseTrafficIn (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint32 ui32RemotePreoxyUniqueID, uint32 ui32RemoteProxyIP,
                                uint16 ui16RemoteProxyPort, ProtocolType protocolType, uint32 ui32BytesIn);
        void increaseTrafficOut (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint32 ui32RemotePreoxyUniqueID, uint32 ui32RemoteProxyIP,
                                 uint16 ui16RemoteProxyPort, ProtocolType protocolType, uint32 ui32BytesOut);

        void packGUIUpdateMessage (void);
        void getGUIUpdateMessage (const char *&cGUIUpdateMessage, uint16 &ui16BufferSize) const;

        void deleteStatistics (ConnectorType connectorType, uint32 ui32RemoteProxyUniqueID);


    private:
        GUIStatsManager (void);

        ConnectorStats * const registerStatisticsIfNecessary (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint32 ui32RemoteProxyUniqueID,
                                                              uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort);

        void increaseTrafficIn (ConnectorStats * const pConnectorStats, ProtocolType protocolType, uint32 ui32BytesIn);
        void increaseTrafficOut (ConnectorStats * const pConnectorStats, ProtocolType protocolType, uint32 ui32BytesOut);
        void increaseSummaryTrafficIn (ProtocolType protocolType, uint32 ui32BytesIn);
        void increaseSummaryTrafficOut (ProtocolType protocolType, uint32 ui32BytesOut);

        void resetRate (void);

        void packSummaryStats (float fElapsedTimeInSeconds);
        void packDetailedStats (const ConnectorStats * const connectorStats, const ConnectionStats &connectionStats, float fElapsedTimeInSeconds);
        void updateLastStatsComputationTime (void);

        ConnectionStats summaryTCPStats;
        ConnectionStats summaryUDPStats;
        ConnectionStats summaryICMPStats;
        int64 i64LastStatsUpdateComputationTime;

        msgpack::sbuffer _MessagePackBuffer;
        msgpack::packer<msgpack::sbuffer> _MessagePacker;

        NOMADSUtil::Mutex _m;
        NOMADSUtil::Mutex _mConnectorStatsTable;
        NOMADSUtil::UInt64Hashtable<ConnectorStats> _ConnectorStatsTable;
    };


    inline ConnectionStats::ConnectionStats (const ProtocolType protocolType)
        : _protocolType (protocolType), _ui32TrafficInBytes (0), _ui32TrafficInRate (0), _ui32TrafficOutBytes (0), _ui32TrafficOutRate (0) { }

    inline const ProtocolType ConnectionStats::getProtocolType (void) const
    {
        return _protocolType;
    }

    inline char ConnectionStats::getProtocolTypeAsChar (void) const
    {
        return protocolTypeToChar (_protocolType);
    }

    inline void ConnectionStats::resetRate (void)
    {
        _ui32TrafficInRate = 0;
        _ui32TrafficOutRate = 0;
    }

    inline void ConnectionStats::increaseTrafficIn (uint32 ui32BytesIn)
    {
        _ui32TrafficInBytes += ui32BytesIn;
        _ui32TrafficInRate += ui32BytesIn;
    }

    inline void ConnectionStats::increaseTrafficOut (uint32 ui32BytesOut)
    {
        _ui32TrafficOutBytes += ui32BytesOut;
        _ui32TrafficOutRate += ui32BytesOut;
    }

    inline ConnectorStats::ConnectorStats (const ConnectorType connType, uint32 ui32RemoteHostIP, uint32 ui32RemoteProxyUniqueID,
                                           uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort) :
        _connectorType (connType), _ui32RemoteHostIP (ui32RemoteHostIP), _ui32RemoteProxyUniqueID (ui32RemoteProxyUniqueID), _ui32RemoteProxyIP (ui32RemoteProxyIP),
        _ui16RemoteProxyPort (ui16RemoteProxyPort), _i64CreationTime (NOMADSUtil::getTimeInMilliseconds()), _i64LastUpdateTime (NOMADSUtil::getTimeInMilliseconds()),
        _detailedTCPStats (PT_TCP), _detailedUDPStats (PT_UDP), _detailedICMPStats (PT_ICMP) { }

    inline ConnectorType ConnectorStats::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline char ConnectorStats::getConnectorTypeAsChar (void) const
    {
        return connectorTypeToChar (_connectorType);
    }

    inline uint32 ConnectorStats::getRemoteHostIP (void) const
    {
        return _ui32RemoteHostIP;
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

    inline int64 ConnectorStats::getCreationTime (void) const
    {
        return _i64CreationTime;
    }

    inline int64 ConnectorStats::getLastUpdateTime (void) const
    {
        return _i64LastUpdateTime;
    }

    inline void ConnectorStats::resetRate (void)
    {
        _detailedTCPStats.resetRate();
        _detailedUDPStats.resetRate();
        _detailedICMPStats.resetRate();
    }

    inline int ConnectorStats::lock (void)
    {
        return _m.lock();
    }

    inline int ConnectorStats::tryLock (void)
    {
        return _m.tryLock();
    }

    inline int ConnectorStats::unlock (void)
    {
        return _m.unlock();
    }

    inline GUIStatsManager::GUIStatsManager (void)
        : summaryTCPStats (PT_TCP), summaryUDPStats (PT_UDP), summaryICMPStats (PT_ICMP), _MessagePacker (&_MessagePackBuffer)
    {
        updateLastStatsComputationTime();
    }

    inline GUIStatsManager::~GUIStatsManager (void) {}

    inline GUIStatsManager * const GUIStatsManager::getGUIStatsManager (void)
    {
        static GUIStatsManager guiStatsManager;

        return &guiStatsManager;
    }

    inline void GUIStatsManager::increaseTrafficIn (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint32 ui32RemotePreoxyUniqueID,
                                                    uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort, ProtocolType protocolType, uint32 ui32BytesIn)
    {
        if (NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED && (connectorType != CT_UNDEF) &&
            (ui32RemoteHostIP != 0) && (ui32RemotePreoxyUniqueID != 0)) {
            ConnectorStats * const pConnectorStats = registerStatisticsIfNecessary (connectorType, ui32RemoteHostIP, ui32RemotePreoxyUniqueID,
                                                                                    ui32RemoteProxyIP, ui16RemoteProxyPort);
            increaseTrafficIn (pConnectorStats, protocolType, ui32BytesIn);
        }
    }

    inline void GUIStatsManager::increaseTrafficOut (ConnectorType connectorType, uint32 ui32RemoteHostIP, uint32 ui32RemotePreoxyUniqueID,
                                                     uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort, ProtocolType protocolType, uint32 ui32BytesOut)
    {
        if (NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED && (connectorType != CT_UNDEF) &&
            (ui32RemoteHostIP != 0) && (ui32RemotePreoxyUniqueID != 0)) {
            ConnectorStats * const pConnectorStats = registerStatisticsIfNecessary (connectorType, ui32RemoteHostIP, ui32RemotePreoxyUniqueID,
                                                                                    ui32RemoteProxyIP, ui16RemoteProxyPort);
            increaseTrafficOut (pConnectorStats, protocolType, ui32BytesOut);
        }
    }

    inline void GUIStatsManager::getGUIUpdateMessage (const char *&cGUIUpdateMessage, uint16 &ui16BufferSize) const
    {
        cGUIUpdateMessage = _MessagePackBuffer.data();
        ui16BufferSize = _MessagePackBuffer.size();
    }

    inline void GUIStatsManager::updateLastStatsComputationTime (void)
    {
        i64LastStatsUpdateComputationTime = NOMADSUtil::getTimeInMilliseconds();
    }

    inline void GUIStatsManager::resetRate (void)
    {
        summaryTCPStats.resetRate();
        summaryUDPStats.resetRate();
        summaryICMPStats.resetRate();
    }

}


#endif  // INCL_GUI_UPDATE_MESSAGE_H
