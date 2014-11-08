/*
 * DisServiceStatusNotifier.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "DisServiceStatusNotifier.h"

#include "DisServiceDefs.h"
#include "DisServiceStats.h"
#include "DisServiceStatus.h"

#include "Logger.h"

#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;
using namespace msgpack;

DisServiceStatusNotifier::DisServiceStatusNotifier (const char *pszNodeId, const char *pszNotifyAddr)
    : _packer (_bufWriter), _nodeId (pszNodeId)
{
    _localHostAddr.setIPAddress (pszNotifyAddr);
}

DisServiceStatusNotifier::~DisServiceStatusNotifier (void)
{
}

int DisServiceStatusNotifier::init (uint16 ui16StatsPort)
{
    int rc;
    if (0 != (rc = _statSocket.init())) {
        checkAndLogMsg ("DisServiceStatusNotifier::init", Logger::L_MildError,
                        "failed to initialize DatagramSocket on port %d; rc = %d\n", (int) ui16StatsPort, rc);
        return -1;
    }
    checkAndLogMsg ("DisServiceStatusNotifier::init", Logger::L_Info,
                     "initialized DatagramSocket on port %d\n", (int) ui16StatsPort);
    _ui16StatsPort = ui16StatsPort;
    return 0;
}

int DisServiceStatusNotifier::clientConnected (uint16 ui16ClientId)
{
    uint8 ui8Header = DisServiceStatusHeaderByte;
    uint8 ui8Type = DSSNT_ClientConnected;
    _bufWriter.clear();
    _packer.pack_short (ui8Header);
    _packer.pack_short (ui8Type);
    _packer.pack_int (ui16ClientId);
    return sendPacket();
}

int DisServiceStatusNotifier::clientDisconnected (uint16 ui16ClientId)
{
    uint8 ui8Header = DisServiceStatusHeaderByte;
    uint8 ui8Type = DSSNT_ClientDisconnected;
    _bufWriter.clear();
    _packer.pack_short (ui8Header);
    _packer.pack_short (ui8Type);
    _packer.pack_int (ui16ClientId);
    return sendPacket();
}

int DisServiceStatusNotifier::sendSummaryStats (DisServiceStats *pStats)
{
    _bufWriter.clear();

    uint8 ui8Header = DisServiceStatusHeaderByte;
    _packer.pack_short (ui8Header);

    _packer.pack (std::string (_nodeId.c_str()));

    uint8 ui8Type = DSSNT_SummaryStats;
    _packer.pack_short (ui8Type);

    pStats->lock();

    DisServiceBasicStatisticsInfo dsbsi;
    dsbsi.ui32DataMessagesReceived = pStats->_ui32DataMessagesReceived;
    dsbsi.ui32DataBytesReceived = pStats->_ui32DataBytesReceived;
    dsbsi.ui32DataFragmentsReceived = pStats->_ui32DataFragmentsReceived;
    dsbsi.ui32DataFragmentBytesReceived = pStats->_ui32DataFragmentBytesReceived;
    dsbsi.ui32MissingFragmentRequestMessagesSent = pStats->_ui32MissingFragmentRequestMessagesSent;
    dsbsi.ui32MissingFragmentRequestBytesSent = pStats->_ui32MissingFragmentRequestBytesSent;
    dsbsi.ui32MissingFragmentRequestMessagesReceived = pStats->_ui32MissingFragmentRequestMessagesReceived;
    dsbsi.ui32MissingFragmentRequestBytesReceived = pStats->_ui32MissingFragmentRequestBytesReceived;
    dsbsi.ui32DataCacheQueryMessagesSent = pStats->_ui32DataCacheQueryMessagesSent;
    dsbsi.ui32DataCacheQueryBytesSent = pStats->_ui32DataCacheQueryBytesSent;
    dsbsi.ui32DataCacheQueryMessagesReceived = pStats->_ui32DataCacheQueryMessagesReceived;
    dsbsi.ui32DataCacheQueryBytesReceived = pStats->_ui32DataCacheQueryBytesReceived;
    dsbsi.ui32TopologyStateMessagesSent = pStats->_ui32TopologyStateMessagesSent;
    dsbsi.ui32TopologyStateBytesSent = pStats->_ui32TopologyStateBytesSent;
    dsbsi.ui32TopologyStateMessagesReceived = pStats->_ui32TopologyStateMessagesReceived;
    dsbsi.ui32TopologyStateBytesReceived = pStats->_ui32TopologyStateBytesReceived;
    dsbsi.ui32KeepAliveMessagesSent = pStats->_ui32KeepAliveMessagesSent;
    dsbsi.ui32KeepAliveMessagesReceived = pStats->_ui32KeepAliveMessagesReceived;
    dsbsi.ui32QueryMessageSent = pStats->_ui32QueryMessageSent;
    dsbsi.ui32QueryMessageReceived = pStats->_ui32QueryMessageReceived;
    dsbsi.ui32QueryHitsMessageSent = pStats->_ui32QueryHitsMessageSent;
    dsbsi.ui32QueryHitsMessageReceived = pStats->_ui32QueryHitsMessageReceived;
    dsbsi.write (&_packer);

    uint8 ui8Flags;
    ui8Flags = DSSF_OverallStats;
    _packer.pack_short (ui8Flags);

    DisServiceStatsInfo dssi;
    dssi.ui32ClientMessagesPushed = pStats->_overallStats.ui32ClientMessagesPushed;
    dssi.ui32ClientBytesPushed = pStats->_overallStats.ui32ClientBytesPushed;
    dssi.ui32ClientMessagesMadeAvailable = pStats->_overallStats.ui32ClientMessagesMadeAvailable;
    dssi.ui32ClientBytesMadeAvailable = pStats->_overallStats.ui32ClientBytesMadeAvailable;
    dssi.ui32FragmentsPushed = pStats->_overallStats.ui32FragmentsPushed;
    dssi.ui32FragmentBytesPushed = pStats->_overallStats.ui32FragmentBytesPushed;
    dssi.ui32OnDemandFragmentsSent = pStats->_overallStats.ui32OnDemandFragmentsSent;
    dssi.ui32OnDemandFragmentBytesSent = pStats->_overallStats.ui32OnDemandFragmentBytesSent;
    dssi.write (&_packer);

    ui8Flags = DSSF_DuplicateTrafficInfo;
    _packer.pack_short (ui8Flags);

    DisServiceDuplicateTrafficInfo dsdti;
    dsdti.ui32TargetedDuplicateTraffic = pStats->_overallStats.ui32TargetedDuplicateTraffic;
    dsdti.ui32OverheardDuplicateTraffic = pStats->_overallStats.ui32OverheardDuplicateTraffic;
    dsdti.write (&_packer);

    StringHashtable<DisServiceBasicStatisticsInfoByPeer>::Iterator iter = pStats->_statsByPeer.getAllElements();
    for (; !iter.end();iter.nextElement()) {
        ui8Flags = DSSF_PerClientGroupTagStats;
        _packer.pack_short (ui8Flags);

        const char *pszPeerId = iter.getKey();
        _packer.pack (std::string (pszPeerId));

        DisServiceBasicStatisticsInfoByPeer *pDSBSIByPeer = iter.getValue();
        pDSBSIByPeer->write (&_packer);
    }

    ui8Flags = DSSF_End;
    _packer.pack_short (ui8Flags);

    pStats->unlock();

    return sendPacket();
}

int DisServiceStatusNotifier::sendNeighborList (const char **ppszNeighbors)
{
    uint8 ui8Header = DisServiceStatusHeaderByte;
    uint8 ui8Type = DSSNT_TopologyStatus;
    _bufWriter.clear();
    _packer.pack_short (ui8Header);
    _packer.pack_short (ui8Type);

    uint16 i = 0;
    for (; ppszNeighbors[i] != NULL; i++);
    for (uint16 j = 0; j < i; j++) {
        _packer.pack (std::string (ppszNeighbors[i]));
        i++;
    }
    delete[] ppszNeighbors;
    return sendPacket();
}

int DisServiceStatusNotifier::sendDetailedStats (DisServiceStats *pStats)
{
    return 0;
}

int DisServiceStatusNotifier::sendPacket (void)
{
    int rc = _statSocket.sendTo (_localHostAddr.getIPAddress(), _ui16StatsPort,
                                 _bufWriter.data(), _bufWriter.size());
    if (rc < 0) {
        return rc;
    }
    return 0;
}
