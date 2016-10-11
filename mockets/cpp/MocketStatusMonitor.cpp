/*
 * MocketStatusMonitor.cpp
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "MocketStatusMonitor.h"

#include "MocketStatus.h"

#include "UDPDatagramSocket.h"
#include "InetAddr.h"
#include "Logger.h"
#include "StrClass.h"
#include <StringHashtable.h>

//#include <stdio.h>
#include <string.h>

#if defined (WIN32)
    #define snprintf _snprintf
#endif


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

MocketStatusMonitor::MocketStatusMonitor (void)
{
    _pDGSocket = NULL;
    _ui16RelayPort = 0;
    _pDGRelaySocket = NULL;
    _fileLog = NULL;
    _pHandlerFn = NULL;
    _pCallbackArg = NULL;
    _szBuf[0] = '\0';
    _i64delay = 10000;
}

MocketStatusMonitor::~MocketStatusMonitor (void)
{
    delete _pDGSocket;
    delete _pDGRelaySocket;
    _pDGSocket = NULL;
}

void MocketStatusMonitor::setStatsIntervalMillisec (int64 i)
{
    _i64delay = i;
}

int64 MocketStatusMonitor::getStatsIntervalMillisec()
{
    return _i64delay;
}

int MocketStatusMonitor::initReceiveSocket (uint16 ui16StatPort)
{
    int rc;
    _pDGSocket = new UDPDatagramSocket();
    InetAddr localAddr ("127.0.0.1");
    if (0 != (rc = _pDGSocket->init (ui16StatPort, localAddr.getIPAddress()))) {
        checkAndLogMsg ("MocketStatusMonitor::initReceiveSocket", Logger::L_MildError,
                        "failed to initialize UDPDatagramSocket on port %d; rc = %d\n", (int) ui16StatPort, rc);
        return -1;
    }
    return 0;
}

int MocketStatusMonitor::initRelaying (const char *pszRelayIP, uint16 ui16RelayPort)
{
    if (pszRelayIP == NULL) {
        return -1;
    }
    int rc;
    _relayIP = pszRelayIP;
    _ui16RelayPort = ui16RelayPort;
    _pDGRelaySocket = new UDPDatagramSocket();
    if (0 != (rc = _pDGRelaySocket->init())) {
        checkAndLogMsg ("MocketStatusMonitor::initRelaying", Logger::L_MildError,
                        "failed to initialize UDPDatagramSocket; rc = %d\n", rc);
        return -2;
    }
    return 0;
}

int MocketStatusMonitor::initFileOutput (FILE *fileLog)
{
    if (fileLog == NULL) {
        _fileLog = stdout;
    }
    else {
        _fileLog = fileLog;
    }
    return 0;
}


int MocketStatusMonitor::initHandler (MessageHandlerCallbackFnPtr pHandlerFn, void *pCallbackArg)
{
    _pHandlerFn = pHandlerFn;
    _pCallbackArg = pCallbackArg;
    return 0;
}

void MocketStatusMonitor::run (void)
{
    int rc;
    char buf[65535];
    InetAddr senderAddr;
    started();
    _i64StartTime = getTimeInMilliseconds();
    printHeader();
    while (!terminationRequested()) {
        rc = _pDGSocket->receive (buf, sizeof (buf), &senderAddr);
        if (rc <= 0) {
            checkAndLogMsg ("MocketStatusMonitor::go", Logger::L_MildError,
                            "receive on UDPDatagramSocket failed; rc = %d\n", rc);
            setTerminatingResultCode (-1);
            terminating();
            return;
        }
        uint16 ui16MsgLen = (uint16) rc;
        switch (getUpdateType (buf, ui16MsgLen)) {
            case MSNT_ConnectionFailed:
                 handleConnectionFailedUpdate (buf, ui16MsgLen);
                 break;
            case MSNT_ConnectionEstablished:
                 handleConnectionEstablishedUpdate (buf, ui16MsgLen);
                 break;
            case MSNT_ConnectionReceived:
                 handleConnectionReceivedUpdate (buf, ui16MsgLen);
                 break;
            case MSNT_Stats:
                 handleStatsUpdate (buf, ui16MsgLen);
                 break;
            case MSNT_Disconnected:
                 handleDisconnectionUpdate (buf, ui16MsgLen);
                 break;
            default:
                 checkAndLogMsg ("MocketStatusMonitor::go", Logger::L_MildError,
                                 "received an update of an unknown type %d\n", (int) getUpdateType (buf, ui16MsgLen));
                 break;
        }
    }
    terminating();
}

int MocketStatusMonitor::printHeader (void)
{
    snprintf (_szBuf, sizeof (_szBuf)-1,
              "Time, DTime, Component, PID, Identifier, Event, LIP, LPort, RIP, RPort, LCTime, BSent, PSent, Retrans, BRcvd, PRcvd, "
              "Discard1, Discard2, Discard3, RTT, PDS, PQS, RSDS, RSQS, RUDS, RUQS, "
              "MsgType, RSMSent, RUMSent, USMSent, UUMSent, RSMRcvd, RUMRcvd, USMRcvd, UUMRcvd, PCanceled\n");
    return handleMessage();
}

int MocketStatusMonitor::handleConnectionFailedUpdate (const char *pBuf, uint16 ui16BufLen)
{
    uint32 ui32PID = getPID (pBuf, ui16BufLen);
    const char *pszIdentifier = getIdentifier (pBuf, ui16BufLen);
    if (pszIdentifier == NULL) {
        return -1;
    }
    const EndPointsInfo *pEPI = getEndPointsInfo (pBuf, ui16BufLen);
    if (pEPI == NULL) {
        return -2;
    }

    // Handle pszIdentifier and pEPI appropriately
    int64 i64CurrTime = getTimeInMilliseconds();
    InetAddr localAddr (pEPI->ui32LocalAddr), remoteAddr (pEPI->ui32RemoteAddr);
    String localIPAddr = localAddr.getIPAsString();
    String remoteIPAddr = remoteAddr.getIPAsString();
    #if defined (WIN32)
        snprintf (_szBuf, sizeof(_szBuf)-1, "%I64d, %I64d, Mockets, %lu, %s, ConnFailed, %s, %d, %s, %d\n",
    #else
        snprintf (_szBuf, sizeof(_szBuf)-1, "%lld, %lld, Mockets, %u, %s, ConnFailed, %s, %d, %s, %d\n",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            ui32PID,
            (pszIdentifier[0] != '\0' ? pszIdentifier : "<unknown>"),
            (const char*) localIPAddr, (int) pEPI->ui16LocalPort,
            (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);

    if (_pDGRelaySocket) {
        char buf[1024];
        buf[0] = MSNT_ConnectionFailed;
        sprintf (buf + 1, "%s %d %s %d", (const char*) localIPAddr, (int) pEPI->ui16LocalPort, (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);
        _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
    }

    return handleMessage();
}

int MocketStatusMonitor::handleConnectionEstablishedUpdate (const char *pBuf, uint16 ui16BufLen)
{
    uint32 ui32PID = getPID (pBuf, ui16BufLen);
    const char *pszIdentifier = getIdentifier (pBuf, ui16BufLen);
    if (pszIdentifier == NULL) {
        return -1;
    }
    const EndPointsInfo *pEPI = getEndPointsInfo (pBuf, ui16BufLen);
    if (pEPI == NULL) {
        return -2;
    }

    // Handle pszIdentifier and pEPI appropriately
    int64 i64CurrTime = getTimeInMilliseconds();
    InetAddr localAddr (pEPI->ui32LocalAddr), remoteAddr (pEPI->ui32RemoteAddr);
    String localIPAddr = localAddr.getIPAsString();
    String remoteIPAddr = remoteAddr.getIPAsString();
    #if defined (WIN32)
        snprintf (_szBuf, sizeof(_szBuf)-1, "%I64d, %I64d, Mockets, %lu, %s, Conn, %s, %d, %s, %d\n",
    #else
        snprintf (_szBuf, sizeof(_szBuf)-1, "%lld, %lld, Mockets, %u, %s, Conn, %s, %d, %s, %d\n",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            ui32PID,
            (pszIdentifier[0] != '\0' ? pszIdentifier : "<unknown>"),
            (const char*) localIPAddr, (int) pEPI->ui16LocalPort,
            (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);

    if (_pDGRelaySocket) {
        char buf[1024];
        buf[0] = MSNT_ConnectionEstablished;
        sprintf (buf + 1, "%s %d %s %d", (const char*) localIPAddr, (int) pEPI->ui16LocalPort, (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);
        _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
    }

    return handleMessage();
}

int MocketStatusMonitor::handleConnectionReceivedUpdate (const char *pBuf, uint16 ui16BufLen)
{
    uint32 ui32PID = getPID (pBuf, ui16BufLen);
    const char *pszIdentifier = getIdentifier (pBuf, ui16BufLen);
    if (pszIdentifier == NULL) {
        return -1;
    }
    const EndPointsInfo *pEPI = getEndPointsInfo (pBuf, ui16BufLen);
    if (pEPI == NULL) {
        return -2;
    }

    // Handle pszIdentifier and pEPI appropriately
    int64 i64CurrTime = getTimeInMilliseconds();
    InetAddr localAddr (pEPI->ui32LocalAddr), remoteAddr (pEPI->ui32RemoteAddr);
    String localIPAddr = localAddr.getIPAsString();
    String remoteIPAddr = remoteAddr.getIPAsString();
    #if defined (WIN32)
        snprintf (_szBuf, sizeof(_szBuf)-1, "%lld, %lld, Mockets, %lu, %s, ConnRcvd, %s, %d, %s, %d\n",
    #else
        snprintf (_szBuf, sizeof(_szBuf)-1, "%lld, %lld, Mockets, %u, %s, ConnRcvd, %s, %d, %s, %d\n",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            ui32PID,
            (pszIdentifier[0] != '\0' ? pszIdentifier : "<unknown>"),
            (const char*) localIPAddr, (int) pEPI->ui16LocalPort,
            (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);

    if (_pDGRelaySocket) {
        char buf[1024];
        buf[0] = MSNT_ConnectionReceived;
        sprintf (buf + 1, "%s %d %s %d", (const char*) localIPAddr, (int) pEPI->ui16LocalPort, (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);
        _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
    }

    return handleMessage();
}

int MocketStatusMonitor::handleStatsUpdate (const char *pBuf, uint16 ui16BufLen)
{
    uint32 ui32PID = getPID (pBuf, ui16BufLen);
    const char *pszIdentifier = getIdentifier (pBuf, ui16BufLen);
    if (pszIdentifier == NULL) {
        return -1;
    }
    const EndPointsInfo *pEPI = getEndPointsInfo (pBuf, ui16BufLen);
    if (pEPI == NULL) {
        return -2;
    }
    const StatisticsInfo *pSI = getStatistics (pBuf, ui16BufLen);
    if (pSI == NULL) {
        return -3;
    }

    // Handle pszIdentifier, pEPI, and pSI appropriately
    int64 i64CurrTime = getTimeInMilliseconds();
    InetAddr localAddr (pEPI->ui32LocalAddr), remoteAddr (pEPI->ui32RemoteAddr);
    String localIPAddr = localAddr.getIPAsString();
    String remoteIPAddr = remoteAddr.getIPAsString();
    #if defined (WIN32)
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%I64d, %I64d, Mockets, %lu, %s, Stats, %s, %d, %s, %d, %I64d, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %f, %lu, %lu, %lu, %lu, %lu, %lu",
    #else
        snprintf (_szBuf, sizeof (_szBuf) - 1,
                  "%lld, %lld, Mockets, %lu, %s, Stats, %s, %d, %s, %d, %lld, %u, %u, %u, %u, %u, %u, %u, %u, %f, %u, %u, %u, %u, %u, %u",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            ui32PID,
            (pszIdentifier[0] != '\0' ? pszIdentifier : "<unknown>"),
            (const char*) localIPAddr, (int) pEPI->ui16LocalPort,
            (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort,
            (i64CurrTime - pSI->i64LastContactTime),
            pSI->ui32SentBytes, pSI->ui32SentPackets, pSI->ui32Retransmits,
            pSI->ui32ReceivedBytes, pSI->ui32ReceivedPackets,
            pSI->ui32DuplicatedDiscardedPackets, pSI->ui32NoRoomDiscardedPackets, pSI->ui32ReassemblySkippedDiscardedPackets,
            pSI->fEstimatedRTT,
            pSI->ui32PendingDataSize, pSI->ui32PendingPacketQueueSize,
            pSI->ui32ReliableSequencedDataSize, pSI->ui32ReliableSequencedPacketQueueSize,
            pSI->ui32ReliableUnsequencedDataSize, pSI->ui32ReliableUnsequencedPacketQueueSize);
    handleMessage();

    if (_pDGRelaySocket) {
        char buf[1024];
        char buf2[64];
        int64 *newTime;
        buf[0] = MSNT_Stats;
        sprintf (buf + 1, "%s %d %s %d %lld %u %u %u %u %u %u %u %u %f %u %u %u %u %u %u", (const char*) localIPAddr, (int) pEPI->ui16LocalPort, (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort, (i64CurrTime - pSI->i64LastContactTime), pSI->ui32SentBytes, pSI->ui32SentPackets, pSI->ui32Retransmits, pSI->ui32ReceivedBytes, pSI->ui32ReceivedPackets, pSI->ui32DuplicatedDiscardedPackets, pSI->ui32NoRoomDiscardedPackets, pSI->ui32ReassemblySkippedDiscardedPackets, pSI->fEstimatedRTT, pSI->ui32PendingDataSize, pSI->ui32PendingPacketQueueSize,  pSI->ui32ReliableSequencedDataSize, pSI->ui32ReliableSequencedPacketQueueSize, pSI->ui32ReliableUnsequencedDataSize, pSI->ui32ReliableUnsequencedPacketQueueSize);
        sprintf (buf2, "%s %d %s %d", (const char*) localIPAddr, (int) pEPI->ui16LocalPort, (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);
        String *temp = _stats.get (buf2);
        if (temp != NULL) {
            delete temp;
        }
        _stats.put (buf2, new String(buf));
        int64 *oldTime = _timestats.get (buf2);
        if (oldTime == NULL || i64CurrTime - (*oldTime) > _i64delay) {
            _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
            if (oldTime != NULL) {
                delete oldTime;
            }
            newTime = new int64();
            (*newTime) = i64CurrTime;
            _timestats.put (buf2, newTime);
        }
    }

    const MessageStatisticsInfo *pMSI = getMessageStatistics (pBuf, ui16BufLen);
    if (pMSI != NULL) {
        // This is an update from a MessageMocket
        // Handle pMSI appropriately
        snprintf (_szBuf, sizeof(_szBuf)-1, ", %d, %u, %u, %u, %u, %u, %u, %u, %u, %u\n",
                 (int) pMSI->ui16MsgType,
                 pMSI->ui32SentReliableSequencedMsgs, pMSI->ui32SentReliableUnsequencedMsgs,
                 pMSI->ui32SentUnreliableSequencedMsgs, pMSI->ui32SentUnreliableUnsequencedMsgs,
                 pMSI->ui32ReceivedReliableSequencedMsgs, pMSI->ui32ReceivedReliableUnsequencedMsgs,
                 pMSI->ui32ReceivedUnreliableSequencedMsgs, pMSI->ui32ReceivedUnreliableUnsequencedMsgs,
                 pMSI->ui32CancelledPackets);
        handleMessage();
        // Iterate through the per-type MSIs
        const MessageStatisticsInfo *pPerTypeMSI = NULL;
        while (NULL != (pPerTypeMSI = getPerTypeMessageStatistics (pBuf, ui16BufLen, pPerTypeMSI))) {
            // Handle pPerTypeMSI appropriately
            snprintf (_szBuf, sizeof(_szBuf)-1, ", , , , , , , , , , , , , , , , , , , , , , , , %d, %u, %u, %u, %u, %u, %u, %u, %u, %u\n",
                     (int) pPerTypeMSI->ui16MsgType,
                     pPerTypeMSI->ui32SentReliableSequencedMsgs, pPerTypeMSI->ui32SentReliableUnsequencedMsgs,
                     pPerTypeMSI->ui32SentUnreliableSequencedMsgs, pPerTypeMSI->ui32SentUnreliableUnsequencedMsgs,
                     pPerTypeMSI->ui32ReceivedReliableSequencedMsgs, pPerTypeMSI->ui32ReceivedReliableUnsequencedMsgs,
                     pPerTypeMSI->ui32ReceivedUnreliableSequencedMsgs, pPerTypeMSI->ui32ReceivedUnreliableUnsequencedMsgs,
                     pPerTypeMSI->ui32CancelledPackets);
            handleMessage();
        }
    }
    else {
        strcpy (_szBuf, "\n");
        handleMessage();
    }
    return 0;
}

int MocketStatusMonitor::handleDisconnectionUpdate (const char *pBuf, uint16 ui16BufLen)
{
    uint32 ui32PID = getPID (pBuf, ui16BufLen);
    const char *pszIdentifier = getIdentifier (pBuf, ui16BufLen);
    if (pszIdentifier == NULL) {
        return -1;
    }
    const EndPointsInfo *pEPI = getEndPointsInfo (pBuf, ui16BufLen);
    if (pEPI == NULL) {
        return -2;
    }
    // Handle pszIdentifier and pEPI appropriately
    int64 i64CurrTime = getTimeInMilliseconds();
    InetAddr localAddr (pEPI->ui32LocalAddr), remoteAddr (pEPI->ui32RemoteAddr);
    String localIPAddr = localAddr.getIPAsString();
    String remoteIPAddr = remoteAddr.getIPAsString();
    #if defined (WIN32)
        snprintf (_szBuf, sizeof(_szBuf)-1, "%I64d, %I64d, Mockets, %lu, %s, Disconn, %s, %d, %s, %d\n",
    #else
        snprintf (_szBuf, sizeof(_szBuf)-1, "%lld, %lld, Mockets, %u, %s, Disconn, %s, %d, %s, %d\n",
    #endif
            i64CurrTime, (i64CurrTime - _i64StartTime),
            ui32PID,
            (pszIdentifier[0] != '\0' ? pszIdentifier : "<unknown>"),
            (const char*) localIPAddr, (int) pEPI->ui16LocalPort,
            (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);

    if (_pDGRelaySocket) {
        char buf[1024];
        buf[0] = MSNT_Disconnected;
        sprintf (buf + 1, "%s %d %s %d", (const char*) localIPAddr, (int) pEPI->ui16LocalPort, (const char*) remoteIPAddr, (int) pEPI->ui16RemotePort);
        String *stat = _stats.get(buf + 1);
        if (stat != NULL) {
            _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, (char *) (*stat), 1024);
            _stats.remove(buf + 1);
            delete stat;
        }
        int64 * t = _timestats.get(buf + 1);
        if (t != NULL)
            delete t;
        _timestats.remove(buf + 1);
        _pDGRelaySocket->sendTo (_relayIP, _ui16RelayPort, buf, 1024);
    }

    return handleMessage();
}

MocketStatusNoticeType MocketStatusMonitor::getUpdateType (const char *pBuf, uint16 ui16BufLen)
{
    uint8 ui8Type = *((uint8*)pBuf);
    if ((ui8Type != MSNT_ConnectionFailed) && (ui8Type != MSNT_ConnectionEstablished) &&
        (ui8Type != MSNT_ConnectionReceived) && (ui8Type != MSNT_Stats) && (ui8Type != MSNT_Disconnected)) {
        return MSNT_Undefined;
    }
    return (MocketStatusNoticeType) ui8Type;
}

uint32 MocketStatusMonitor::getPID (const char *pBuf, uint16 ui16BufLen)
{
    return *((uint32*)(pBuf+1));
}

const char * MocketStatusMonitor::getIdentifier (const char *pBuf, uint16 ui16BufLen)
{
    // The identifier is null-terminated anyway, so just skip the length and return
    // a pointer to the start of the identifier
    return (pBuf+7);
}

const EndPointsInfo * MocketStatusMonitor::getEndPointsInfo (const char *pBuf, uint16 ui16BufLen)
{
    uint16 ui16IdentifierLen = *((uint16*)(pBuf+5));
    return (EndPointsInfo*) (pBuf + 1 + 4 + 2 + ui16IdentifierLen + 1);
}

const StatisticsInfo * MocketStatusMonitor::getStatistics (const char *pBuf, uint16 ui16BufLen)
{
    if (getUpdateType (pBuf, ui16BufLen) != MSNT_Stats) {
        return NULL;
    }
    uint16 ui16IdentifierLen = *((uint16*)(pBuf+5));
    return (StatisticsInfo*) (pBuf + 1 + 4 + 2 + ui16IdentifierLen + 1 + sizeof (EndPointsInfo));
}

const MessageStatisticsInfo * MocketStatusMonitor::getMessageStatistics (const char *pBuf, uint16 ui16BufLen)
{
    if (getUpdateType (pBuf, ui16BufLen) != MSNT_Stats) {
        return NULL;
    }
    uint16 ui16IdentifierLen = *((uint16*)(pBuf+5));
    uint16 ui16Offset = 1 + 4 + 2 + ui16IdentifierLen + 1 + sizeof (EndPointsInfo) + sizeof (StatisticsInfo);
    if (MSF_OverallMessageStatistics != *((uint8*)(pBuf+ui16Offset))) {
        return NULL;
    }
    return (MessageStatisticsInfo*) (pBuf + ui16Offset + 1);    
}

const MessageStatisticsInfo * MocketStatusMonitor::getPerTypeMessageStatistics (const char *pBuf, uint16 ui16BufLen, const MessageStatisticsInfo *pPrev)
{
    if (getUpdateType (pBuf, ui16BufLen) != MSNT_Stats) {
        return NULL;
    }
    uint16 ui16Offset;
    uint16 ui16IdentifierLen;
    if (pPrev == NULL) {
        ui16IdentifierLen = *((uint16*)(pBuf+5));
        ui16Offset = 1 + 4 + 2 + ui16IdentifierLen + 1 + sizeof (EndPointsInfo) + sizeof (StatisticsInfo);
        if (MSF_OverallMessageStatistics != *((uint8*)(pBuf+ui16Offset))) {
            return NULL;
        }
        ui16Offset += (1 + sizeof (MessageStatisticsInfo));
    }
    else {
        if ((const char*)pPrev <= pBuf) {
            return NULL;
        }
        ui16Offset = (uint16) (((const char*)pPrev) - pBuf);
        ui16Offset += (1 + sizeof (MessageStatisticsInfo));
    }
    if (MSF_PerTypeMessageStatistics != *((uint8*)(pBuf+ui16Offset))) {
        return NULL;
    }
    return (MessageStatisticsInfo*) (pBuf + ui16Offset + 1);
}

int MocketStatusMonitor::parseUpdate (const char *pBuf, uint16 ui16BufLen)
{
    ((char*)pBuf)[ui16BufLen] = '\0';
    printf ("%s\n----\n", pBuf);
    return 0;
}

int MocketStatusMonitor::handleMessage (void)
{
    _szBuf[sizeof(_szBuf)-1] = '\0';   // Just in case
    if (_fileLog) {
        fprintf (_fileLog, _szBuf);
        fflush (_fileLog);
    }
    if (_pHandlerFn) {
        (*_pHandlerFn) (_pCallbackArg, _szBuf);
    }
    return 0;
}
