#ifndef INCL_MOCKET_STATUS_NOTIFIER_H
#define INCL_MOCKET_STATUS_NOTIFIER_H

/*
 * MocketStatusNotifier.h
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
 * 
 * The MocketStatusNotifier class sends out status information about
 * mocket connections on a UDP port, which may be monitored by the
 * MocketStatusMonitor or some other component.
 */

#include "Mocket.h"
#include "MocketStatus.h"

#include "BufferWriter.h"
#include "UDPDatagramSocket.h"
#include "FTypes.h"
#include "InetAddr.h"
#include "StrClass.h"


class MocketStatusNotifier
{
    public:
        MocketStatusNotifier (void);
        ~MocketStatusNotifier (void);

        int init (uint16 ui16StatsPort, bool bASCIIMode);
        int init (const char *pszDestAddr, uint16 ui16StatsPort, bool bASCIIMode);

        int connectionFailed (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int connected (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int connectionReceived (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int connectionRestored (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int setLastContactTime (int64 i64LastContactTime);
        int sendStats (const char *pszLocalIdentifier, MocketStats *pStats);
        void setLocalAddress(const char *pszLocalAddr);
        int disconnected (const char *pszLocalIdentifier);

    private:
        int sendConnectionFailedASCII (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int sendConnectionEstablishedASCII (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int sendConnectionReceivedASCII (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int sendConnectionRestoredASCII (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int sendStatsASCII (const char *pszLocalIdentifier, MocketStats *pStats);
        int sendDisconnectionASCII (const char *pszLocalIdentifier);

        int sendConnectionFailedBinary (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int sendConnectionEstablishedBinary (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int sendConnectionReceivedBinary (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);
        int sendConnectionRestoredBinary (const char *pszLocalIdentifier, NOMADSUtil::InetAddr *pLocalAddr, uint16 ui16LocalPort, NOMADSUtil::InetAddr *pRemoteAddr, uint16 ui16RemotePort);

        int sendStatsBinary (const char *pszLocalIdentifier, MocketStats *pStats);
        int sendDisconnectionBinary (const char *pszLocalIdentifier);

        void writeLocalIdentifierASCII (const char *pszLocalIdentifier);
        void writeLocalIdentifierBinary (const char *pszLocalIdentifier);

        void writeMessageStats (MocketStats::MessageStats *pMsgStats);
        void fillInMessageStatisticsInfo (MessageStatisticsInfo *pMSI, MocketStats::MessageStats *pMsgStats);

        int sendPacket (void);

    private:
        NOMADSUtil::BufferWriter _bufWriter;
        NOMADSUtil::InetAddr _destinationHostAddr;
        uint16 _ui16StatsPort;
        NOMADSUtil::UDPDatagramSocket _statSocket;
        bool _bASCIIMode;
        int64 _i64LastContactTime;
        struct EndPointsInfo _epi;
        NOMADSUtil::String _localEndPointInfo;
        NOMADSUtil::String _remoteEndPointInfo;




};

inline int MocketStatusNotifier::setLastContactTime (int64 i64LastContactTime)
{
    _i64LastContactTime = i64LastContactTime;
    return 0;
}

#endif   // #ifndef INCL_MOCKET_STATUS_NOTIFIER_H
