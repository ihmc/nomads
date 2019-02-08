/*
 * DisServiceStatusMonitor.cpp
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

#include "DisServiceStatusMonitor.h"

#include "DisServiceDefs.h"

#include "BufferWriter.h"
#include "UDPDatagramSocket.h"
#include "Logger.h"
#include "StringTokenizer.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DisServiceStatusMonitor::DisServiceStatusMonitor (void)
{
    _pFileLog = NULL;
    _pListener = NULL;
    _pDGSocket = NULL;
    _ui16RelayPort = 0;
    _pDGRelaySocket = NULL;
    _i64StartTime = 0;
    _ui32RelayInterval = 10000;
    _i64LastSummaryStatsRelayTime = 0;
    _i64LastDetailedStatsRelayTime = 0;
}

DisServiceStatusMonitor::~DisServiceStatusMonitor (void)
{
    delete _pDGSocket;
    _pDGSocket = NULL;
    delete _pDGRelaySocket;
    _pDGRelaySocket = NULL;
}

int DisServiceStatusMonitor::initReceiveSocket (uint16 ui16StatPort)
{
    int rc;
    _pDGSocket = new UDPDatagramSocket();
    InetAddr localAddr ("127.0.0.1");
    if (0 != (rc = _pDGSocket->init (ui16StatPort, localAddr.getIPAddress()))) {
        checkAndLogMsg ("DisServiceStatusMonitor::initReceiveSocket", Logger::L_MildError,
                        "failed to initialize DatagramSocket on port %d; rc = %d\n", (int) ui16StatPort, rc);
        return -1;
    }
    return 0;
}

int DisServiceStatusMonitor::initRelaying (const char *pszRelayIP, uint16 ui16RelayPort)
{
    if (pszRelayIP == NULL) {
        return -1;
    }
    int rc;
    _relayIP = pszRelayIP;
    _ui16RelayPort = ui16RelayPort;
    _pDGRelaySocket = new UDPDatagramSocket();
    if (0 != (rc = _pDGRelaySocket->init())) {
        checkAndLogMsg ("DisServiceStatusMonitor::initRelaying", Logger::L_MildError,
                        "failed to initialize DatagramSocket; rc = %d\n", rc);
        return -2;
    }
    return 0;
}

void DisServiceStatusMonitor::setMinRelayInterval (uint32 ui32MilliSec)
{
    _ui32RelayInterval = ui32MilliSec;
}

uint32 DisServiceStatusMonitor::getMinRelayInterval (void)
{
    return _ui32RelayInterval;
}

int DisServiceStatusMonitor::initFileOutput (FILE *pFileLog)
{
    if (pFileLog == NULL) {
        _pFileLog = stdout;
    }
    else {
        _pFileLog = pFileLog;
    }
    return 0;
}

int DisServiceStatusMonitor::initListener (DisServiceStatusMonitorListener *pListener)
{
    _pListener = pListener;
    return 0;
}

void DisServiceStatusMonitor::run (void)
{
    int rc;
    char buf[65535];
    InetAddr senderAddr;
    setName ("DisServiceStatusMonitor::run");
    started();
    _i64StartTime = getTimeInMilliseconds();
    printHeader();
    while (!terminationRequested()) {
        rc = _pDGSocket->receive (buf, sizeof (buf), &senderAddr);
        if (rc <= 0) {
            checkAndLogMsg ("DisServiceStatusMonitor::run", Logger::L_MildError,
                            "receive on DatagramSocket failed; rc = %d\n", rc);
            setTerminatingResultCode (-1);
            terminating();
            return;
        }
        uint16 ui16MsgLen = (uint16) rc;
        switch (getMessageType (buf, ui16MsgLen)) {
            case DSSNT_ClientConnected:
                 handleClientConnected (senderAddr.getIPAsString(), buf, ui16MsgLen);
                 break;
            case DSSNT_ClientDisconnected:
                 handleClientDisconnected (senderAddr.getIPAsString(), buf, ui16MsgLen);
                 break;
            case DSSNT_SummaryStats:
                 handleSummaryStats (senderAddr.getIPAsString(), buf, ui16MsgLen);
                 break;
            case DSSNT_DetailedStats:
                 handleDetailedStats (senderAddr.getIPAsString(), buf, ui16MsgLen);
                 break;
            case DSSNT_TopologyStatus:
                 handleTopologyStatus (senderAddr.getIPAsString(), buf, ui16MsgLen);
                 break;
            default:
                 checkAndLogMsg ("DisServiceStatusMonitor::run", Logger::L_MildError,
                                 "received an unknown message of size %d with header byte %d and type byte %d\n", rc, (int) buf[0], (int) buf[1]);
                 break;
        }
    }
    terminating();
}

int DisServiceStatusMonitor::printHeader (void)
{
    snprintf (_szBuf, sizeof (_szBuf)-1,
              "Time, DTime, Component, Event, MsgP, BytesP, FragP, FragBytesP, ODFragS, ODFragBytesS, MFRMsgS, MFRBytesS, MFRMsgR, MFRBytesR, "
              "DCQMsgS, DCQBytesS, DCQMsgR, DCQBytesR, WSMsgS, WSBytesS, WSMsgR, WSBytesR, KAMsgS, KAMsgR\n");
    return handleStringInBuffer (NULL);
}

DisServiceStatusNoticeType DisServiceStatusMonitor::getMessageType (const char *pBuf, uint16 ui16BufLen)
{
    uint8 ui8Header = *((uint8*)pBuf);
    uint8 ui8Type = *(((uint8*)pBuf)+1);
    if (ui8Header != DisServiceStatusHeaderByte) {
        return DSSNT_Undefined;
    }
    if ((ui8Type != DSSNT_ClientConnected) && (ui8Type != DSSNT_ClientDisconnected) &&
        (ui8Type != DSSNT_SummaryStats) && (ui8Type != DSSNT_DetailedStats) &&
        (ui8Type != DSSNT_TopologyStatus)) {
        return DSSNT_Undefined;
    }
    return (DisServiceStatusNoticeType) ui8Type;
}

int DisServiceStatusMonitor::handleClientConnected (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen)
{
    if (getMessageType (pBuf, ui16BufLen) != DSSNT_ClientConnected) {
        return -1;
    }
    int64 i64CurrTime = getTimeInMilliseconds();
    uint16 ui16ClientId = (uint16) *((uint16*)(pBuf+2));
    if (_pListener) {
        _pListener->clientConnected (pszSenderIPAddr, ui16ClientId);
    }
    #if defined (WIN32)
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%I64d, %I64d, DisService, ClientConnected, %d\n",
    #else
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%lld, %lld, DisService, ClientConnected, %d\n",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            (int) ui16ClientId);

    handleStringInBuffer (pszSenderIPAddr);

    if (_pDGRelaySocket) {
        char buf[1024];
        buf[0] = DSSNT_ClientConnected;

        // TODO: complete relaying handling here

        _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
    }

    return 0;
}

int DisServiceStatusMonitor::handleClientDisconnected (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen)
{
    if (getMessageType (pBuf, ui16BufLen) != DSSNT_ClientDisconnected) {
        return -1;
    }
    int64 i64CurrTime = getTimeInMilliseconds();
    uint16 ui16ClientId = (uint16) *((uint16*)(pBuf+2));
    if (_pListener) {
        _pListener->clientDisconnected (pszSenderIPAddr, ui16ClientId);
    }
    #if defined (WIN32)
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%I64d, %I64d, DisService, ClientDisconnected, %d\n",
    #else
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%lld, %lld, DisService, ClientDisconnected, %d\n",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            (int) ui16ClientId);

    handleStringInBuffer (pszSenderIPAddr);

    if (_pDGRelaySocket) {
        char buf[1024];
        buf[0] = DSSNT_ClientDisconnected;

        // TODO: complete relaying handling here

        _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
    }

    return 0;
}

int DisServiceStatusMonitor::handleSummaryStats (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen)
{
    if (getMessageType (pBuf, ui16BufLen) != DSSNT_SummaryStats) {
        return -1;
    }
    DisServiceBasicStatisticsInfo *pDSBSI = (DisServiceBasicStatisticsInfo*) (pBuf+2);
    if (*(pBuf + 2 + sizeof (DisServiceBasicStatisticsInfo)) != DSSF_OverallStats) {
        return -2;
    }
    int64 i64CurrTime = getTimeInMilliseconds();
    DisServiceStatsInfo *pOverallDSSI = (DisServiceStatsInfo*) (pBuf + 2 + sizeof (DisServiceBasicStatisticsInfo) + 1);
    if (_pListener) {
        _pListener->summaryStats (pszSenderIPAddr, pDSBSI, pOverallDSSI);
    }

    #if defined (WIN32)
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%I64d, %I64d, DisService, SummaryStats, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u\n",
    #else
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%lld, %lld, DisService, SummaryStats, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u\n",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            pOverallDSSI->ui32ClientMessagesPushed, pOverallDSSI->ui32ClientBytesPushed,
            pOverallDSSI->ui32FragmentsPushed, pOverallDSSI->ui32FragmentBytesPushed,
            pOverallDSSI->ui32OnDemandFragmentsSent, pOverallDSSI->ui32OnDemandFragmentBytesSent,
            pDSBSI->ui32MissingFragmentRequestMessagesSent, pDSBSI->ui32MissingFragmentRequestBytesSent,
            pDSBSI->ui32MissingFragmentRequestMessagesReceived, pDSBSI->ui32MissingFragmentRequestBytesReceived,
            pDSBSI->ui32DataCacheQueryMessagesSent, pDSBSI->ui32DataCacheQueryBytesSent,
            pDSBSI->ui32DataCacheQueryMessagesReceived, pDSBSI->ui32DataCacheQueryBytesReceived,
            pDSBSI->ui32TopologyStateMessagesSent, pDSBSI->ui32TopologyStateBytesSent,
            pDSBSI->ui32TopologyStateMessagesReceived, pDSBSI->ui32TopologyStateBytesReceived,
            pDSBSI->ui32KeepAliveMessagesSent, pDSBSI->ui32KeepAliveMessagesReceived);

    handleStringInBuffer (pszSenderIPAddr);

    // TODO: double check it
    if (_pDGRelaySocket) {
        char buf[1024];
        buf[0] = DSSNT_SummaryStats;
        snprintf (buf + 1, sizeof (buf) -2, "%s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
            (const char*) pszSenderIPAddr,
            pOverallDSSI->ui32ClientMessagesPushed, pOverallDSSI->ui32ClientBytesPushed,
            pOverallDSSI->ui32FragmentsPushed, pOverallDSSI->ui32FragmentBytesPushed,
            pOverallDSSI->ui32OnDemandFragmentsSent, pOverallDSSI->ui32OnDemandFragmentBytesSent,
            pDSBSI->ui32MissingFragmentRequestMessagesSent, pDSBSI->ui32MissingFragmentRequestBytesSent,
            pDSBSI->ui32MissingFragmentRequestMessagesReceived, pDSBSI->ui32MissingFragmentRequestBytesReceived,
            pDSBSI->ui32DataCacheQueryMessagesSent, pDSBSI->ui32DataCacheQueryBytesSent,
            pDSBSI->ui32DataCacheQueryMessagesReceived, pDSBSI->ui32DataCacheQueryBytesReceived,
            pDSBSI->ui32TopologyStateMessagesSent, pDSBSI->ui32TopologyStateBytesSent,
            pDSBSI->ui32TopologyStateMessagesReceived, pDSBSI->ui32TopologyStateBytesReceived,
            pDSBSI->ui32KeepAliveMessagesSent, pDSBSI->ui32KeepAliveMessagesReceived);
            _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
    }

    return 0;
}

int DisServiceStatusMonitor::handleTopologyStatus (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen)
{
    if (getMessageType (pBuf, ui16BufLen) != DSSNT_TopologyStatus) {
        return -1;
    }

    // TODO: pass it to the listener

    // TODO: double check it
    if (_pDGRelaySocket) {
        uint16 ui16MaxBufLength = 1024;
        BufferWriter bw (ui16MaxBufLength, 0);
        uint8 ui8 = DSSNT_TopologyStatus;
        bw.write8 (&ui8);
        bw.writeBytes (pszSenderIPAddr, (unsigned long) strlen (pszSenderIPAddr));
        char endString = '\0';
        bw.writeBytes (&endString, 1);
        char * pMsg;
        uint16 ui16BufToRelyLen = (uint16) bw.getBufferLength ();
        if ((ui16MaxBufLength - ui16BufToRelyLen) >= (ui16BufLen-2)) {
            pMsg = bw.relinquishBuffer();
            memcpy (pMsg+ui16BufToRelyLen, pBuf+2, ui16BufLen-2);
        }
        else {
            return -1;
        }

        _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, pMsg, ui16MaxBufLength);
    }

    return 0;
}

int DisServiceStatusMonitor::handleDetailedStats (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen)
{
    return 0;
}

int DisServiceStatusMonitor::handleStringInBuffer (const char *pszSenderIPAddr)
{
    _szBuf[sizeof(_szBuf)-1] = '\0';   // Just in case
    if (_pFileLog) {
        fprintf (_pFileLog, "%s", _szBuf);
        fflush (_pFileLog);
    }
    if (_pListener) {
        _pListener->message (pszSenderIPAddr, _szBuf);
    }
    return 0;
}

//==============================================================================
// DisServiceStatusMonitorListener
//==============================================================================

int DisServiceStatusMonitorListener::clientConnected (const char *pszSenderIPAddr, uint16 ui16ClientId)
{
    return 0;
}

int DisServiceStatusMonitorListener::clientDisconnected (const char *pszSenderIPAddr, uint16 ui16ClientId)
{
    return 0;
}

int DisServiceStatusMonitorListener::summaryStats (const char *pszSenderIPAddr, DisServiceBasicStatisticsInfo *pDSBSI, DisServiceStatsInfo *pOverallDSSI)
{
    return 0;
}

int DisServiceStatusMonitorListener::message (const char *pszSenderIPAddr, const char *pszMessage)
{
    return 0;
}
