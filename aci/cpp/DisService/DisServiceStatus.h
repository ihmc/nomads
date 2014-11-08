/*
 * DisServiceStatus.h
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

#ifndef INCL_DIS_SERVICE_STATUS_H
#define INCL_DIS_SERVICE_STATUS_H

#include "FTypes.h"

#include "msgpack.hpp"

namespace IHMC_ACI
{
    const uint8 DisServiceStatusHeaderByte = 0xD1;   // This will be the first byte of every DisService status packet
    const uint16 DEFAULT_DIS_SERVICE_STATUS_PORT = 1401;

    enum DisServiceStatusNoticeType
    {
        DSSNT_Undefined = 0x00,
        DSSNT_ClientConnected = 0x01,
        DSSNT_ClientDisconnected = 0x02,
        DSSNT_SummaryStats = 0x03,
        DSSNT_DetailedStats = 0x04,
        DSSNT_TopologyStatus = 0x05
    };

    enum DisServiceStatusFlags
    {
        DSSF_Undefined = 0x00,
        DSSF_End = 0x01,
        DSSF_OverallStats = 0x02,
        DSSF_PerClientGroupTagStats = 0x03,
        DSSF_DuplicateTrafficInfo = 0x04
    };

    struct DisServiceBasicStatisticsInfo
    {
        void write (msgpack::packer<msgpack::sbuffer> *pPacker);

        uint32 ui32DataMessagesReceived;
        uint32 ui32DataBytesReceived;
        uint32 ui32DataFragmentsReceived;
        uint32 ui32DataFragmentBytesReceived;
        uint32 ui32MissingFragmentRequestMessagesSent;
        uint32 ui32MissingFragmentRequestBytesSent;
        uint32 ui32MissingFragmentRequestMessagesReceived;
        uint32 ui32MissingFragmentRequestBytesReceived;
        uint32 ui32DataCacheQueryMessagesSent;
        uint32 ui32DataCacheQueryBytesSent;
        uint32 ui32DataCacheQueryMessagesReceived;
        uint32 ui32DataCacheQueryBytesReceived;
        uint32 ui32TopologyStateMessagesSent;
        uint32 ui32TopologyStateBytesSent;
        uint32 ui32TopologyStateMessagesReceived;
        uint32 ui32TopologyStateBytesReceived;
        uint32 ui32KeepAliveMessagesSent;
        uint32 ui32KeepAliveMessagesReceived;
        uint32 ui32QueryMessageSent;
        uint32 ui32QueryMessageReceived;
        uint32 ui32QueryHitsMessageSent;
        uint32 ui32QueryHitsMessageReceived;
    };

    struct DisServiceBasicStatisticsInfoByPeer
    {
        DisServiceBasicStatisticsInfoByPeer (void);
        ~DisServiceBasicStatisticsInfoByPeer (void);

        void write (msgpack::packer<msgpack::sbuffer> *pPacker);

        uint32 ui32DataMessagesReceived;
        uint32 ui32DataBytesReceived;
        uint32 ui32DataFragmentsReceived;
        uint32 ui32DataFragmentBytesReceived;
        uint32 ui32MissingFragmentRequestMessagesReceived;
        uint32 ui32MissingFragmentRequestBytesReceived;
        uint32 ui32KeepAliveMessagesReceived;
    };

    // The following is utilized for both overall stats as well as stats per client, group, and tag
    struct DisServiceStatsInfo
    {
        void write (msgpack::packer<msgpack::sbuffer> *pPacker);

        uint32 ui32ClientMessagesPushed;
        uint32 ui32ClientBytesPushed;
        uint32 ui32ClientMessagesMadeAvailable;
        uint32 ui32ClientBytesMadeAvailable;
        uint32 ui32FragmentsPushed;
        uint32 ui32FragmentBytesPushed;
        uint32 ui32OnDemandFragmentsSent;
        uint32 ui32OnDemandFragmentBytesSent;
    };

    struct DisServiceDuplicateTrafficInfo
    {
        void write (msgpack::packer<msgpack::sbuffer> *pPacker);

        uint32 ui32TargetedDuplicateTraffic;
        uint32 ui32OverheardDuplicateTraffic;
    };

    struct DisServiceClientGroupTagStatsInfoHeader
    {
        void write (msgpack::packer<msgpack::sbuffer> *pPacker);

        uint16 ui16ClientId;
        uint16 ui16Tag;
        uint16 ui16GroupNameLength;        // Bytes for the group name will follow the struct
    };

    inline void DisServiceBasicStatisticsInfo::write (msgpack::packer<msgpack::sbuffer> *pPacker)
    {
        if (pPacker == NULL) return;
        pPacker->pack_uint32 (ui32DataMessagesReceived);
        pPacker->pack_uint32 (ui32DataBytesReceived);
        pPacker->pack_uint32 (ui32DataFragmentsReceived);
        pPacker->pack_uint32 (ui32DataFragmentBytesReceived);
        pPacker->pack_uint32 (ui32MissingFragmentRequestMessagesSent);
        pPacker->pack_uint32 (ui32MissingFragmentRequestBytesSent);
        pPacker->pack_uint32 (ui32MissingFragmentRequestMessagesReceived);
        pPacker->pack_uint32 (ui32MissingFragmentRequestBytesReceived);
        pPacker->pack_uint32 (ui32DataCacheQueryMessagesSent);
        pPacker->pack_uint32 (ui32DataCacheQueryBytesSent);
        pPacker->pack_uint32 (ui32DataCacheQueryMessagesReceived);
        pPacker->pack_uint32 (ui32DataCacheQueryBytesReceived);
        pPacker->pack_uint32 (ui32TopologyStateMessagesSent);
        pPacker->pack_uint32 (ui32TopologyStateBytesSent);
        pPacker->pack_uint32 (ui32TopologyStateMessagesReceived);
        pPacker->pack_uint32 (ui32TopologyStateBytesReceived);
        pPacker->pack_uint32 (ui32KeepAliveMessagesSent);
        pPacker->pack_uint32 (ui32KeepAliveMessagesReceived);
        pPacker->pack_uint32 (ui32QueryMessageSent);
        pPacker->pack_uint32 (ui32QueryMessageReceived);
        pPacker->pack_uint32 (ui32QueryHitsMessageSent);
        pPacker->pack_uint32 (ui32QueryHitsMessageReceived);
    }

    inline DisServiceBasicStatisticsInfoByPeer::DisServiceBasicStatisticsInfoByPeer (void)
    {
        ui32DataMessagesReceived = 0;
        ui32DataBytesReceived = 0;
        ui32DataFragmentsReceived = 0;
        ui32DataFragmentBytesReceived = 0;
        ui32MissingFragmentRequestMessagesReceived = 0;
        ui32MissingFragmentRequestBytesReceived = 0;
        ui32KeepAliveMessagesReceived= 0;
    }

    inline DisServiceBasicStatisticsInfoByPeer::~DisServiceBasicStatisticsInfoByPeer (void)
    {
    }

    inline void DisServiceBasicStatisticsInfoByPeer::write (msgpack::packer<msgpack::sbuffer> *pPacker)
    {
        if (pPacker == NULL) return;
        pPacker->pack_uint32 (ui32DataMessagesReceived);
        pPacker->pack_uint32 (ui32DataBytesReceived);
        pPacker->pack_uint32 (ui32DataFragmentsReceived);
        pPacker->pack_uint32 (ui32DataFragmentBytesReceived);
        pPacker->pack_uint32 (ui32MissingFragmentRequestMessagesReceived);
        pPacker->pack_uint32 (ui32MissingFragmentRequestBytesReceived);
        pPacker->pack_uint32 (ui32KeepAliveMessagesReceived);
    }

    inline void DisServiceStatsInfo::write (msgpack::packer<msgpack::sbuffer> *pPacker)
    {
        if (pPacker == NULL) return;
        pPacker->pack_uint32 (ui32ClientMessagesPushed);
        pPacker->pack_uint32 (ui32ClientBytesPushed);
        pPacker->pack_uint32 (ui32ClientMessagesMadeAvailable);
        pPacker->pack_uint32 (ui32ClientBytesMadeAvailable);
        pPacker->pack_uint32 (ui32FragmentsPushed);
        pPacker->pack_uint32 (ui32FragmentBytesPushed);
        pPacker->pack_uint32 (ui32OnDemandFragmentsSent);
        pPacker->pack_uint32 (ui32OnDemandFragmentBytesSent);
    }

    inline void DisServiceClientGroupTagStatsInfoHeader::write (msgpack::packer<msgpack::sbuffer> *pPacker)
    {
        if (pPacker == NULL) return;
        pPacker->pack_uint16 (ui16ClientId);
        pPacker->pack_uint16 (ui16Tag);
        pPacker->pack_uint16 (ui16GroupNameLength);
    }

    inline void DisServiceDuplicateTrafficInfo::write (msgpack::packer<msgpack::sbuffer> *pPacker)
    {
        if (pPacker == NULL) return;
        pPacker->pack_uint32 (ui32TargetedDuplicateTraffic);
        pPacker->pack_uint32 (ui32OverheardDuplicateTraffic);
    }
}

#endif   // #ifndef INCL_DIS_SERVICE_STATUS_H
