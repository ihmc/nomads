/*
 * MocketStatusNotifier.cpp
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

#include "MocketStatusNotifier.h"

#include "MocketStatus.h"

#include "Logger.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

MocketStatusNotifier::MocketStatusNotifier (void)
{
    // Stats are currently sent to localhost
    _destinationHostAddr.setIPAddress ("127.0.0.1");
    _ui16StatsPort = 0;
    _i64LastContactTime = 0;
    _bASCIIMode = false;
    _epi.ui32LocalAddr = 0;
    _epi.ui16LocalPort = 0;
    _epi.ui32RemoteAddr = 0;
    _epi.ui16RemotePort = 0;
}

MocketStatusNotifier::~MocketStatusNotifier (void)
{
}

int MocketStatusNotifier::init (uint16 ui16StatsPort, bool bASCIIMode)
{
    int rc;
    if (0 != (rc = _statSocket.init())) {
        checkAndLogMsg ("MocketStatusNotifier::init", Logger::L_MildError,
                        "failed to initialize UDPDatagramSocket on port %d; rc = %d\n", (int) ui16StatsPort, rc);
        return -1;
    }
    _ui16StatsPort = ui16StatsPort;
    _bASCIIMode = bASCIIMode;
    return 0;
}

int MocketStatusNotifier::init (const char *pszDestAddr, uint16 ui16StatsPort, bool bASCIIMode)
{
    int rc;
    if (0 != (rc = _statSocket.init())) {
        checkAndLogMsg ("MocketStatusNotifier::init", Logger::L_MildError,
                        "failed to initialize UDPDatagramSocket on port %d; rc = %d\n", (int) ui16StatsPort, rc);
        return -1;
    }
    _destinationHostAddr.setIPAddress (pszDestAddr);
    _ui16StatsPort = ui16StatsPort;
    _bASCIIMode = bASCIIMode;
    return 0;
}

int MocketStatusNotifier::connectionFailed (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    if (_bASCIIMode) {
        return sendConnectionFailedASCII (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
    else {
        return sendConnectionFailedBinary (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
}

int MocketStatusNotifier::connected (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    if (_bASCIIMode) {
        return sendConnectionEstablishedASCII (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
    else {
        return sendConnectionEstablishedBinary (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
}

int MocketStatusNotifier::connectionReceived (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    if (_bASCIIMode) {
        return sendConnectionReceivedASCII (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
    else {
        return sendConnectionReceivedBinary (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
}

int MocketStatusNotifier::connectionRestored (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    if (_bASCIIMode) {
        return sendConnectionRestoredASCII (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
    else {
        return sendConnectionRestoredBinary (pszLocalIdentifier, pLocalAddr, ui16LocalPort, pRemoteAddr, ui16RemotePort);
    }
}

int MocketStatusNotifier::sendStats (const char *pszLocalIdentifier, MocketStats *pStats)
{
    if (_bASCIIMode) {
        return sendStatsASCII (pszLocalIdentifier, pStats);
    }
    else {
        return sendStatsBinary (pszLocalIdentifier, pStats);
    }
}

int MocketStatusNotifier::disconnected (const char *pszLocalIdentifier)
{
    if (_bASCIIMode) {
        return sendDisconnectionASCII (pszLocalIdentifier);
    }
    else {
        return sendDisconnectionBinary (pszLocalIdentifier);
    }
}

int MocketStatusNotifier::sendConnectionFailedASCII (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    char szBuf[80];
    _bufWriter.reset();
    _bufWriter.writeBytes ("Connection Failed\r\n", 19);
    writeLocalIdentifierASCII (pszLocalIdentifier);
    sprintf (szBuf, "LocalIP=%s\r\n", pLocalAddr->getIPAsString());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "LocalPort=%d\r\n", (int) ui16LocalPort);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "RemoteIP=%s\r\n", pRemoteAddr->getIPAsString());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "RemotePort=%d\r\n", (int) ui16RemotePort);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    _bufWriter.writeBytes ("END Connection Failed\r\n", 23);
    return sendPacket();
}

int MocketStatusNotifier::sendConnectionEstablishedASCII (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    char szBuf[80];
    sprintf (szBuf, "LocalIP=%s\r\n", pLocalAddr->getIPAsString());
    _localEndPointInfo = szBuf;
    sprintf (szBuf, "LocalPort=%d\r\n", (int) ui16LocalPort);
    _localEndPointInfo += szBuf;
    sprintf (szBuf, "RemoteIP=%s\r\n", pRemoteAddr->getIPAsString());
    _remoteEndPointInfo = szBuf;
    sprintf (szBuf, "RemotePort=%d\r\n", (int) ui16RemotePort);
    _remoteEndPointInfo += szBuf;
    _bufWriter.reset();
    _bufWriter.writeBytes ("Connection Established\r\n", 24);
    writeLocalIdentifierASCII (pszLocalIdentifier);
    _bufWriter.writeBytes ((const char*)_localEndPointInfo, _localEndPointInfo.length());
    _bufWriter.writeBytes ((const char*)_remoteEndPointInfo, _remoteEndPointInfo.length());
    _bufWriter.writeBytes ("END Connection Established\r\n", 28);
    return sendPacket();
}

int MocketStatusNotifier::sendConnectionReceivedASCII (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    char szBuf[80];
    sprintf (szBuf, "LocalIP=%s\r\n", pLocalAddr->getIPAsString());
    _localEndPointInfo = szBuf;
    sprintf (szBuf, "LocalPort=%d\r\n", (int) ui16LocalPort);
    _localEndPointInfo += szBuf;
    sprintf (szBuf, "RemoteIP=%s\r\n", pRemoteAddr->getIPAsString());
    _remoteEndPointInfo = szBuf;
    sprintf (szBuf, "RemotePort=%d\r\n", (int) ui16RemotePort);
    _remoteEndPointInfo += szBuf;
    _bufWriter.reset();
    _bufWriter.writeBytes ("Connection Received\r\n", 21);
    writeLocalIdentifierASCII (pszLocalIdentifier);
    _bufWriter.writeBytes ((const char*)_localEndPointInfo, _localEndPointInfo.length());
    _bufWriter.writeBytes ((const char*)_remoteEndPointInfo, _remoteEndPointInfo.length());
    _bufWriter.writeBytes ("END Connection Received\r\n", 25);
    return sendPacket();
}

int MocketStatusNotifier::sendConnectionRestoredASCII (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    char szBuf[80];
    sprintf (szBuf, "LocalIP=%s\r\n", pLocalAddr->getIPAsString());
    _localEndPointInfo = szBuf;
    sprintf (szBuf, "LocalPort=%d\r\n", (int) ui16LocalPort);
    _localEndPointInfo += szBuf;
    sprintf (szBuf, "RemoteIP=%s\r\n", pRemoteAddr->getIPAsString());
    _remoteEndPointInfo = szBuf;
    sprintf (szBuf, "RemotePort=%d\r\n", (int) ui16RemotePort);
    _remoteEndPointInfo += szBuf;
    _bufWriter.reset();
    _bufWriter.writeBytes ("Connection Restored\r\n", 21);
    writeLocalIdentifierASCII (pszLocalIdentifier);
    _bufWriter.writeBytes ((const char*)_localEndPointInfo, _localEndPointInfo.length());
    _bufWriter.writeBytes ((const char*)_remoteEndPointInfo, _remoteEndPointInfo.length());
    _bufWriter.writeBytes ("END Connection Restored\r\n", 25);
    return sendPacket();
}

void MocketStatusNotifier::setLocalAddress(const char *pszLocalAddr)
{
    char szBuf[80];
    sprintf(szBuf, "LocalIP=%s\r\n", pszLocalAddr);
    _localEndPointInfo = szBuf;
    _epi.ui32LocalAddr = InetAddr(pszLocalAddr).getIPAddress();
}

int MocketStatusNotifier::sendStatsASCII (const char *pszLocalIdentifier, MocketStats *pStats)
{
    char szBuf[80];
    _bufWriter.reset();
    _bufWriter.writeBytes ("ConnectionStats\r\n", 17);
    writeLocalIdentifierASCII (pszLocalIdentifier);
    _bufWriter.writeBytes ((const char*)_localEndPointInfo, _localEndPointInfo.length());
    _bufWriter.writeBytes ((const char*)_remoteEndPointInfo, _remoteEndPointInfo.length());

    #if defined (WIN32)
        sprintf (szBuf, "LastContactTime=%I64d\r\n", _i64LastContactTime);
    #else
        sprintf (szBuf, "LastContactTime=%lld\r\n", _i64LastContactTime);
    #endif
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));

    sprintf (szBuf, "BytesSent=%u\r\n", pStats->getSentByteCount());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "PacketsSent=%u\r\n", pStats->getSentPacketCount());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "PacketsRetransmitted=%u\r\n", pStats->getRetransmitCount());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "BytesReceived=%u\r\n", pStats->getReceivedByteCount());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "PacketsReceived=%u\r\n", pStats->getReceivedPacketCount());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "PacketsDiscarded-Duplicate=%u\r\n", pStats->getDuplicatedDiscardedPacketCount());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "PacketsDiscarded-NoRoom=%u\r\n", pStats->getNoRoomDiscardedPacketCount());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "EstimatedRTT=%f\r\n", pStats->getEstimatedRTT());
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));

    _bufWriter.writeBytes ("Overall Message Statistics\r\n", 28);
    writeMessageStats (pStats->getOverallMessageStatistics());
    for (uint16 ui16 = 1; ui16 <= pStats->getHighestTag(); ui16++) {
        if (pStats->isTagUsed (ui16)) {
            sprintf (szBuf, "Message Statistics for Message Type %d\n", (int) ui16);
            _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
            writeMessageStats (pStats->getMessageStatisticsForType (ui16));
        }
    }

    _bufWriter.writeBytes ("END ConnectionStats\r\n", 21);

    return sendPacket();
}

int MocketStatusNotifier::sendDisconnectionASCII (const char *pszLocalIdentifier)
{
    _bufWriter.reset();
    _bufWriter.writeBytes ("Disconnected\r\n", 14);
    writeLocalIdentifierASCII (pszLocalIdentifier);
    _bufWriter.writeBytes ((const char*)_localEndPointInfo, _localEndPointInfo.length());
    _bufWriter.writeBytes ((const char*)_remoteEndPointInfo, _remoteEndPointInfo.length());
    _bufWriter.writeBytes ("END Disconnected\r\n", 18);
    return sendPacket();
}

int MocketStatusNotifier::sendConnectionFailedBinary (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    uint8 ui8Type = MSNT_ConnectionFailed;
    uint32 ui32PID = getPID();
    _bufWriter.reset();
    _bufWriter.write8 (&ui8Type);
    _bufWriter.writeBytes (&ui32PID, sizeof (ui32PID));    // Don't use write32 since it will do Endian conversion
    writeLocalIdentifierBinary (pszLocalIdentifier);
    EndPointsInfo epi;
    epi.ui32LocalAddr = pLocalAddr->getIPAddress();
    epi.ui16LocalPort = ui16LocalPort;
    epi.ui32RemoteAddr = pRemoteAddr->getIPAddress();
    epi.ui16RemotePort = ui16RemotePort;
    _bufWriter.writeBytes (&epi, sizeof (epi));
    return sendPacket();
}

int MocketStatusNotifier::sendConnectionEstablishedBinary (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    uint8 ui8Type = MSNT_ConnectionEstablished;
    uint32 ui32PID = getPID();
    _bufWriter.reset();
    _bufWriter.write8 (&ui8Type);
    _bufWriter.writeBytes (&ui32PID, sizeof (ui32PID));    // Don't use write32 since it will do Endian conversion
    writeLocalIdentifierBinary (pszLocalIdentifier);
    _epi.ui32LocalAddr = pLocalAddr->getIPAddress();
    _epi.ui16LocalPort = ui16LocalPort;
    _epi.ui32RemoteAddr = pRemoteAddr->getIPAddress();
    _epi.ui16RemotePort = ui16RemotePort;
    _bufWriter.writeBytes (&_epi, sizeof (_epi));
    return sendPacket();
}

int MocketStatusNotifier::sendConnectionReceivedBinary (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    uint8 ui8Type = MSNT_ConnectionReceived;
    uint32 ui32PID = getPID();
    _bufWriter.reset();
    _bufWriter.write8 (&ui8Type);
    _bufWriter.writeBytes (&ui32PID, sizeof (ui32PID));
    writeLocalIdentifierBinary (pszLocalIdentifier);
    _epi.ui32LocalAddr = pLocalAddr->getIPAddress();
    _epi.ui16LocalPort = ui16LocalPort;
    _epi.ui32RemoteAddr = pRemoteAddr->getIPAddress();
    _epi.ui16RemotePort = ui16RemotePort;
    _bufWriter.writeBytes (&_epi, sizeof (_epi));
    return sendPacket();
}

int MocketStatusNotifier::sendConnectionRestoredBinary (const char *pszLocalIdentifier, InetAddr *pLocalAddr, uint16 ui16LocalPort, InetAddr *pRemoteAddr, uint16 ui16RemotePort)
{
    uint8 ui8Type = MSNT_ConnectionRestored;
    uint32 ui32PID = getPID();
    _bufWriter.reset();
    _bufWriter.write8 (&ui8Type);
    _bufWriter.writeBytes (&ui32PID, sizeof (ui32PID));
    writeLocalIdentifierBinary (pszLocalIdentifier);
    _epi.ui32LocalAddr = pLocalAddr->getIPAddress();
    _epi.ui16LocalPort = ui16LocalPort;
    _epi.ui32RemoteAddr = pRemoteAddr->getIPAddress();
    _epi.ui16RemotePort = ui16RemotePort;
    _bufWriter.writeBytes (&_epi, sizeof (_epi));
    return sendPacket();
}

int MocketStatusNotifier::sendStatsBinary (const char *pszLocalIdentifier, MocketStats *pStats)
{
    uint8 ui8Type = MSNT_Stats;
    uint32 ui32PID = getPID();
    _bufWriter.reset();
    _bufWriter.write8 (&ui8Type);
    _bufWriter.writeBytes (&ui32PID, sizeof (ui32PID));    // Don't use write32 since it will do Endian conversion
    writeLocalIdentifierBinary (pszLocalIdentifier);
    _bufWriter.writeBytes (&_epi, sizeof (_epi));
    StatisticsInfo si;
    
    si.i64LastContactTime = _i64LastContactTime;
    si.ui32SentBytes = pStats->getSentByteCount();
    si.ui32SentPackets = pStats->getSentPacketCount();
    si.ui32Retransmits = pStats->getRetransmitCount();
    si.ui32ReceivedBytes = pStats->getReceivedByteCount();
    si.ui32ReceivedPackets = pStats->getReceivedPacketCount();
    si.ui32DuplicatedDiscardedPackets = pStats->getDuplicatedDiscardedPacketCount();
    si.ui32NoRoomDiscardedPackets = pStats->getNoRoomDiscardedPacketCount();
    si.ui32ReassemblySkippedDiscardedPackets = pStats->getReassemblySkippedDiscardedPacketCount();
    si.fEstimatedRTT = pStats->getEstimatedRTT();
    si.ui32UnacknowledgedDataSize = 0;
    si.ui32PendingDataSize = pStats->getPendingDataSize();
    si.ui32PendingPacketQueueSize = pStats->getPendingPacketQueueSize();
    si.ui32ReliableSequencedDataSize = pStats->getReliableSequencedDataSize();
    si.ui32ReliableSequencedPacketQueueSize = pStats->getReliableSequencedPacketQueueSize();
    si.ui32ReliableUnsequencedDataSize = pStats->getReliableUnsequencedDataSize();
    si.ui32ReliableUnsequencedPacketQueueSize = pStats->getReliableUnsequencedPacketQueueSize();
    _bufWriter.writeBytes (&si, sizeof (si));

    uint8 ui8Flags;
    ui8Flags = MSF_OverallMessageStatistics;
    _bufWriter.write8 (&ui8Flags);
    MessageStatisticsInfo msi;
    msi.ui16MsgType = 0;
    fillInMessageStatisticsInfo (&msi, pStats->getOverallMessageStatistics());
    _bufWriter.writeBytes (&msi, sizeof (msi));
    for (uint16 ui16 = 1; ui16 <= pStats->getHighestTag(); ui16++) {
        if (pStats->isTagUsed (ui16)) {
            ui8Flags = MSF_PerTypeMessageStatistics;
            _bufWriter.write8 (&ui8Flags);
            msi.ui16MsgType = ui16;
            fillInMessageStatisticsInfo (&msi, pStats->getMessageStatisticsForType (ui16));
            _bufWriter.writeBytes (&msi, sizeof (msi));
        }
    }

    ui8Flags = MSF_End;
    _bufWriter.write8 (&ui8Flags);

    return sendPacket();
}

int MocketStatusNotifier::sendDisconnectionBinary (const char *pszLocalIdentifier)
{
    uint8 ui8Type = MSNT_Disconnected;
    uint32 ui32PID = getPID();
    _bufWriter.reset();
    _bufWriter.write8 (&ui8Type);
    _bufWriter.writeBytes (&ui32PID, sizeof (ui32PID));
    writeLocalIdentifierBinary (pszLocalIdentifier);
    _bufWriter.writeBytes (&_epi, sizeof (_epi));
    return sendPacket();
}

void MocketStatusNotifier::writeLocalIdentifierASCII (const char *pszLocalIdentifier)
{
    _bufWriter.writeBytes ("LocalIdentifier=", 16);
    if (pszLocalIdentifier) {
        _bufWriter.writeBytes (pszLocalIdentifier, (unsigned long) strlen (pszLocalIdentifier));
        _bufWriter.writeBytes ("\r\n", 2);
    }
    else {
        _bufWriter.writeBytes ("<none>\r\n", 8);
    }
}

void MocketStatusNotifier::writeLocalIdentifierBinary (const char *pszLocalIdentifier)
{
    if (pszLocalIdentifier) {
        uint16 ui16Length = (uint16) strlen (pszLocalIdentifier);
        _bufWriter.writeBytes (&ui16Length, 2);       // Don't use write16 since it will do Endian conversion
        _bufWriter.writeBytes (pszLocalIdentifier, ui16Length);
    }
    else {
        uint16 ui16Length = 0;
        _bufWriter.writeBytes (&ui16Length, 2);       // Don't use write16 since it will do Endian conversion
    }
    _bufWriter.write8 ((void*)"\0");
}

void MocketStatusNotifier::writeMessageStats (MocketStats::MessageStats *pMsgStats)
{
    char szBuf[80];
    sprintf (szBuf, "ReliableSequencedMessagesSent=%u\r\n", pMsgStats->ui32SentReliableSequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "ReliableUnsequencedMessagesSent=%u\r\n", pMsgStats->ui32SentReliableUnsequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "UnreliableSequencedMessagesSent=%u\r\n", pMsgStats->ui32SentUnreliableSequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "UnreliableUnsequencedMessagesSent=%u\r\n", pMsgStats->ui32SentUnreliableUnsequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "ReliableSequencedMessagesReceived=%u\r\n", pMsgStats->ui32ReceivedReliableSequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "ReliableUnsequencedMessagesReceived=%u\r\n", pMsgStats->ui32ReceivedReliableUnsequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "UnreliableSequencedMessagesReceived=%u\r\n", pMsgStats->ui32ReceivedUnreliableSequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "UnreliableUnsequencedMessagesReceived=%u\r\n", pMsgStats->ui32ReceivedUnreliableUnsequencedMsgs);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
    sprintf (szBuf, "CancelledPackets=%u\r\n", pMsgStats->ui32CancelledPackets);
    _bufWriter.writeBytes (szBuf, (unsigned long) strlen (szBuf));
}

void MocketStatusNotifier::fillInMessageStatisticsInfo (MessageStatisticsInfo *pMSI, MocketStats::MessageStats *pMsgStats)
{
    pMSI->ui32SentReliableSequencedMsgs = pMsgStats->ui32SentReliableSequencedMsgs;
    pMSI->ui32SentReliableUnsequencedMsgs = pMsgStats->ui32SentReliableUnsequencedMsgs;
    pMSI->ui32SentUnreliableSequencedMsgs = pMsgStats->ui32SentUnreliableSequencedMsgs;
    pMSI->ui32SentUnreliableUnsequencedMsgs = pMsgStats->ui32SentUnreliableUnsequencedMsgs;
    pMSI->ui32ReceivedReliableSequencedMsgs = pMsgStats->ui32ReceivedReliableSequencedMsgs;
    pMSI->ui32ReceivedReliableUnsequencedMsgs = pMsgStats->ui32ReceivedReliableUnsequencedMsgs;
    pMSI->ui32ReceivedUnreliableSequencedMsgs = pMsgStats->ui32ReceivedUnreliableSequencedMsgs;
    pMSI->ui32ReceivedUnreliableUnsequencedMsgs = pMsgStats->ui32ReceivedUnreliableUnsequencedMsgs;
    pMSI->ui32CancelledPackets = pMsgStats->ui32CancelledPackets;
}

int MocketStatusNotifier::sendPacket (void)
{
    if (0 != _statSocket.sendTo (_destinationHostAddr.getIPAddress(), _ui16StatsPort, _bufWriter.getBuffer(), _bufWriter.getBufferLength())) {
        return -1;
    }
    return 0;
}
