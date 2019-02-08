/*
 * DisServiceStats.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#include "DisServiceStats.h"

#include "DisServiceDefs.h"
#include "DisServiceMsg.h"

#include "Logger.h"
#include "DisServiceStatus.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DisServiceStats::DisServiceStats (void)
    : _m (22)
{
    _ui32DataMessagesReceived = 0;
    _ui32DataBytesReceived = 0;
    _ui32DataFragmentsReceived = 0;
    _ui32DataFragmentBytesReceived = 0;

    _ui32AcknowledgmentMessageSent = 0;
    _ui32AcknowledgmentMessageBytesSent = 0;
    _ui32AcknowledgmentMessageReceived = 0;
    _ui32AcknowledgmentMessageBytesReceived = 0;

    _ui32CtrlToCtrlMessageSent = 0;
    _ui32CtrlToCtrlMessageBytesSent = 0;
    _ui32CtrlToCtrlMessageReceived = 0;
    _ui32CtrlToCtrlMessageBytesReceived = 0;

    _ui32MissingFragmentRequestMessagesSent = 0;
    _ui32MissingFragmentRequestBytesSent = 0;
    _ui32MissingFragmentRequestMessagesReceived = 0;
    _ui32MissingFragmentRequestBytesReceived = 0;

    _ui32DataCacheQueryMessagesSent = 0;
    _ui32DataCacheQueryBytesSent = 0;
    _ui32DataCacheQueryMessagesReceived = 0;
    _ui32DataCacheQueryBytesReceived = 0;
    _ui32DataCacheQueryMessagesReplySent = 0;
    _ui32DataCacheQueryReplyBytesSent = 0;
    _ui32DataCacheQueryMessagesReplyReceived = 0;
    _ui32DataCacheQueryReplyBytesReceived = 0;

    _ui32TopologyStateMessagesSent = 0;
    _ui32TopologyStateBytesSent = 0;
    _ui32TopologyStateMessagesReceived = 0;
    _ui32TopologyStateBytesReceived = 0;

    _ui32KeepAliveMessagesSent = 0;
    _ui32KeepAliveMessagesReceived = 0;

    _ui32TotDataFragRcvd = 0;
    _ui32DataFragFrwded = 0;

    _ui32QueryMessageSent = 0;
    _ui32QueryMessageBytesSent = 0;
    _ui32QueryMessageReceived = 0;
    _ui32QueryMessageBytesReceived = 0;

    _ui32QueryHitsMessageSent = 0;
    _ui32QueryHitsMessageBytesSent = 0;
    _ui32QueryHitsMessageReceived = 0;
    _ui32QueryHitsMessageBytesReceived = 0;
}

DisServiceStats::~DisServiceStats (void)
{
}

void DisServiceStats::lock (void)
{
    _m.lock (158);
}

void DisServiceStats::unlock (void)
{
    _m.unlock (158);
}

void DisServiceStats::newIncomingMessage (const void *, uint16, DisServiceMsg *pDisServiceMsg,
                                          uint32, const char *)
{
    const char *pszRemoteNodeId = pDisServiceMsg->getSenderNodeId();
    uint16 ui16Size = pDisServiceMsg->getSize();

    switch (pDisServiceMsg->getType()) {
        case DisServiceMsg::DSMT_Data:
            dataMessageReceived (pszRemoteNodeId);
            break;

        case DisServiceMsg::DSMT_DataReq:
            missingFragmentRequestReceived (pszRemoteNodeId, ui16Size);
            break;

        case DisServiceMsg::DSMT_WorldStateSeqId:
            keepAliveMessageReceived (pszRemoteNodeId);
            break;

        case DisServiceMsg::DSMT_SubStateMessage:
            break;

        case DisServiceMsg::DSMT_SubStateReq:
            break;

        case DisServiceMsg::DSMT_DataCacheQuery:
            dataCacheQueryMessageReceived (pszRemoteNodeId, ui16Size);
            break;

        case DisServiceMsg::DSMT_DataCacheQueryReply:
            dataCacheQueryMessageReplyReceived (pszRemoteNodeId, ui16Size);
            break;

        case DisServiceMsg::DSMT_DataCacheMessagesRequest:
            break;

        case DisServiceMsg::DSMT_AcknowledgmentMessage:
            acknowledgmentMessageReceived (pszRemoteNodeId, ui16Size);
            break;

        case DisServiceMsg::DSMT_CompleteMessageReq:
            break;

        case DisServiceMsg::DSMT_CacheEmpty:
            break;

        case DisServiceMsg::DSMT_CtrlToCtrlMessage:
            ctrlToCtrlMessageReceived (pszRemoteNodeId, ui16Size);
            break;

        case DisServiceMsg::DSMT_ChunkReq:
            break;

        case DisServiceMsg::DSMT_HistoryReq:
            break;

        case DisServiceMsg::DSMT_HistoryReqReply:
            break;

        case DisServiceMsg::CRMT_QueryHits:
            break;

        case DisServiceMsg::CRMT_Query:
            queryMessageReceived (pszRemoteNodeId, ui16Size);
            break;

        case DisServiceMsg::DSMT_SubAdvMessage:
            break;

        case DisServiceMsg::DSMT_ImprovedSubStateMessage:
            break;

        case DisServiceMsg::DSMT_ProbabilitiesMsg:
            break;

        default:
            checkAndLogMsg ("DisServiceStats::newIncomingMessage", Logger::L_MildError,
                            "DisServiceStats does not handle message of type %d\n",
                             pDisServiceMsg->getType());
            break;
    }
}

void DisServiceStats::messagePushedByClient (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint32 ui32Size)
{
    _overallStats.ui32ClientMessagesPushed++;
    _overallStats.ui32ClientBytesPushed += ui32Size;
    Stats *pStats = getStatsForClientGroupTag (ui16ClientId, pszGroupName, ui16Tag);
    if (pStats != NULL) {
        pStats->ui32ClientMessagesPushed++;
        pStats->ui32ClientBytesPushed += ui32Size;
    }
}

void DisServiceStats::messageMadeAvailableByClient (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint32 ui32Size)
{
    _overallStats.ui32ClientMessagesMadeAvailable++;
    _overallStats.ui32ClientBytesMadeAvailable += ui32Size;
    Stats *pStats = getStatsForClientGroupTag (ui16ClientId, pszGroupName, ui16Tag);
    if (pStats != NULL) {
        pStats->ui32ClientMessagesMadeAvailable++;
        pStats->ui32ClientBytesMadeAvailable += ui32Size;
    }
}

void DisServiceStats::fragmentPushed (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint16 ui16Size)
{
    _overallStats.ui32FragmentsPushed++;
    _overallStats.ui32FragmentBytesPushed += ui16Size;
    Stats *pStats = getStatsForClientGroupTag (ui16ClientId, pszGroupName, ui16Tag);
    if (pStats != NULL) {
        pStats->ui32FragmentsPushed++;
        pStats->ui32FragmentBytesPushed += ui16Size;
    }
}

void DisServiceStats::onDemandFragmentPushed (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag, uint16 ui16Size)
{
    _overallStats.ui32OnDemandFragmentsSent++;
    _overallStats.ui32OnDemandFragmentBytesSent += ui16Size;
    Stats *pStats = getStatsForClientGroupTag (ui16ClientId, pszGroupName, ui16Tag);
    if (pStats != NULL) {
        pStats->ui32OnDemandFragmentsSent++;
        pStats->ui32OnDemandFragmentBytesSent += ui16Size;
    }
}

void DisServiceStats::dataFragmentReceived (const char *pszRemoteNodeId, const char *pszGroupName, uint16 ui16Tag, uint16 ui16Size)
{
    _ui32DataFragmentsReceived++;
    _ui32DataFragmentBytesReceived += ui16Size;
    DisServiceBasicStatisticsInfoByPeer *pDBSIByPeer = _statsByPeer.get (pszRemoteNodeId);
    if (pDBSIByPeer == NULL) {
        pDBSIByPeer = new DisServiceBasicStatisticsInfoByPeer;
        _statsByPeer.put (pszRemoteNodeId, pDBSIByPeer);
    }
    pDBSIByPeer->ui32DataFragmentsReceived++;
    pDBSIByPeer->ui32DataFragmentBytesReceived += ui16Size;
}

void DisServiceStats::dataMessageReceived (const char *pszRemoteNodeId, const char *pszGroupName, uint16 ui16Tag, uint32 ui32Size)
{
    _ui32DataMessagesReceived++;
    _ui32DataBytesReceived += ui32Size;
    DisServiceBasicStatisticsInfoByPeer *pDBSIByPeer = _statsByPeer.get (pszRemoteNodeId);
    if (pDBSIByPeer == NULL) {
        pDBSIByPeer = new DisServiceBasicStatisticsInfoByPeer;
        _statsByPeer.put (pszRemoteNodeId, pDBSIByPeer);
    }
    pDBSIByPeer->ui32DataMessagesReceived++;
    pDBSIByPeer->ui32DataBytesReceived += ui32Size;
}

void DisServiceStats::acknowledgmentMessageSent (uint16 ui16Size)
{
    _ui32AcknowledgmentMessageSent++;
    _ui32AcknowledgmentMessageBytesSent += ui16Size;
}

void DisServiceStats::acknowledgmentMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32AcknowledgmentMessageReceived++;
    _ui32MissingFragmentRequestBytesReceived += ui16Size;
}

void DisServiceStats::ctrlToCtrlMessageSent (uint16 ui16Size)
{
    _ui32CtrlToCtrlMessageSent++;
    _ui32CtrlToCtrlMessageBytesSent += ui16Size;
}

void DisServiceStats::ctrlToCtrlMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32CtrlToCtrlMessageReceived++;
    _ui32CtrlToCtrlMessageBytesReceived += ui16Size;
}

void DisServiceStats::missingFragmentRequestSent (uint16 ui16Size)
{
    _ui32MissingFragmentRequestMessagesSent++;
    _ui32AcknowledgmentMessageBytesReceived += ui16Size;
}

void DisServiceStats::missingFragmentRequestReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32MissingFragmentRequestMessagesReceived++;
    _ui32MissingFragmentRequestBytesReceived += ui16Size;
    DisServiceBasicStatisticsInfoByPeer *pDBSIByPeer = _statsByPeer.get (pszRemoteNodeId);
    if (pDBSIByPeer == NULL) {
        pDBSIByPeer = new DisServiceBasicStatisticsInfoByPeer;
        _statsByPeer.put (pszRemoteNodeId, pDBSIByPeer);
    }
    pDBSIByPeer->ui32MissingFragmentRequestMessagesReceived++;
    pDBSIByPeer->ui32MissingFragmentRequestBytesReceived += ui16Size;
}

void DisServiceStats::dataCacheQueryMessageSent (const char *pszTargetNodeId, uint16 ui16Size)
{
    _ui32DataCacheQueryMessagesSent++;
    _ui32DataCacheQueryBytesSent += ui16Size;
}

void DisServiceStats::dataCacheQueryMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32DataCacheQueryMessagesReceived++;
    _ui32DataCacheQueryBytesReceived += ui16Size;
}

void DisServiceStats::dataCacheQueryMessageReplySent (uint16 ui16Size)
{
    _ui32DataCacheQueryMessagesReplySent++;
    _ui32DataCacheQueryReplyBytesSent += ui16Size;
}

void DisServiceStats::dataCacheQueryMessageReplyReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32DataCacheQueryMessagesReplyReceived++;
    _ui32DataCacheQueryReplyBytesReceived += ui16Size;
}

void DisServiceStats::topologyStateMessageSent (uint16 ui16Size)
{
    _ui32TopologyStateMessagesSent++;
    _ui32TopologyStateBytesSent += ui16Size;
}

void DisServiceStats::topologyStateMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32TopologyStateMessagesReceived++;
    _ui32TopologyStateBytesReceived += ui16Size;
}

void DisServiceStats::keepAliveMessageSent (void)
{
    _ui32KeepAliveMessagesSent++;
}

void DisServiceStats::keepAliveMessageReceived (const char *pszRemoteNodeId)
{
    _ui32KeepAliveMessagesReceived++;
    DisServiceBasicStatisticsInfoByPeer *pDBSIByPeer = _statsByPeer.get (pszRemoteNodeId);
    if (pDBSIByPeer == NULL) {
        pDBSIByPeer = new DisServiceBasicStatisticsInfoByPeer;
        _statsByPeer.put (pszRemoteNodeId, pDBSIByPeer);
    }
    pDBSIByPeer->ui32KeepAliveMessagesReceived++;
}

void DisServiceStats::dataMessageReceived (const char *pszRemoteNodeId)
{
    _ui32TotDataFragRcvd++;
    DisServiceBasicStatisticsInfoByPeer *pDBSIByPeer = _statsByPeer.get (pszRemoteNodeId);
    if (pDBSIByPeer == NULL) {
        pDBSIByPeer = new DisServiceBasicStatisticsInfoByPeer;
        _statsByPeer.put (pszRemoteNodeId, pDBSIByPeer);
    }
    pDBSIByPeer->ui32DataMessagesReceived++;
}

void DisServiceStats::dataMessageForwarded (void)
{
    _ui32DataFragFrwded++;
}

void DisServiceStats::queryMessageSent (uint16 ui16Size)
{
    _ui32QueryMessageSent++;
    _ui32QueryMessageBytesSent += ui16Size;
}

void DisServiceStats::queryMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32QueryMessageReceived++;
    _ui32QueryMessageBytesReceived += ui16Size;
}

void DisServiceStats::queryHitsMessageSent (uint16 ui16Size)
{
    _ui32QueryHitsMessageSent++;
    _ui32QueryHitsMessageBytesSent += ui16Size;
}

void DisServiceStats::queryHitsMessageReceived (const char *pszRemoteNodeId, uint16 ui16Size)
{
    _ui32QueryHitsMessageReceived++;
    _ui32QueryHitsMessageBytesReceived += ui16Size;
}

DisServiceStats::Stats * DisServiceStats::getStatsForClientGroupTag (uint16 ui16ClientId, const char *pszGroupName, uint16 ui16Tag)
{
    if (pszGroupName == NULL) {
        return NULL;
    }
    _m.lock (184);
    TagStats *pTagStats = _clientGroupTagStats[ui16ClientId].get (pszGroupName);
    if (pTagStats == NULL) {
        pTagStats = new TagStats();
        _clientGroupTagStats[ui16ClientId].put (pszGroupName, pTagStats);
    }
    DisServiceStats::Stats *pStats = &((*pTagStats)[ui16Tag]);
    _m.unlock (184);
    return pStats;
}

DisServiceStats::Stats::Stats (void)
{
    ui32ClientMessagesPushed = 0;
    ui32ClientBytesPushed = 0;
    ui32ClientMessagesMadeAvailable = 0;
    ui32ClientBytesMadeAvailable = 0;
    ui32FragmentsPushed = 0;
    ui32FragmentBytesPushed = 0;
    ui32OnDemandFragmentsSent = 0;
    ui32OnDemandFragmentBytesSent = 0;
    ui32TargetedDuplicateTraffic = 0;
    ui32OverheardDuplicateTraffic = 0;
}

