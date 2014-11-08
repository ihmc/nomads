#ifndef INCL_MOCKET_STATUS_MONITOR_H
#define INCL_MOCKET_STATUS_MONITOR_H

/*
 * MocketStatusMonitor.h
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

#include "MocketStatus.h"

#include "FTypes.h"
#include "ManageableThread.h"
#include "StringHashtable.h"
#include "StrClass.h"
#include "UDPDatagramSocket.h"

//#include <stdio.h>


namespace NOMADSUtil
{
    class UDPDatagramSocket;
}

typedef int (*MessageHandlerCallbackFnPtr) (void *pCallbackArg, const char *pszMsg);

class MocketStatusMonitor : public NOMADSUtil::ManageableThread
{
    public:
        MocketStatusMonitor (void);
        virtual ~MocketStatusMonitor (void);

        int initReceiveSocket (uint16 ui16StatPort);

        // Setter and Getter of the time interval between the relay of two consequent statistics
        void setStatsIntervalMillisec (int64 i);
        int64 getStatsIntervalMillisec();

        // Initialize the MocketStatusMonitor to send messages to a file
        // If the log file is null, the output is written to STDOUT
        int initFileOutput (FILE *fileLog = NULL);

        int initRelaying (const char *pszRelayIP, uint16 ui16RelayPort);

        // Initialize the MocketStatusMonitor to pass messages to a handler function
        int initHandler (MessageHandlerCallbackFnPtr pHandlerFn, void *pCallbackArg);

        void run (void);

    protected:
        int printHeader (void);

        int handleConnectionFailedUpdate (const char *pBuf, uint16 ui16BufLen);
        int handleConnectionEstablishedUpdate (const char *pBuf, uint16 ui16BufLen);
        int handleConnectionReceivedUpdate (const char *pBuf, uint16 ui16BufLen);
        int handleStatsUpdate (const char *pBuf, uint16 ui16BufLen);
        int handleDisconnectionUpdate (const char *pBuf, uint16 ui16BufLen);

        MocketStatusNoticeType getUpdateType (const char *pBuf, uint16 ui16BufLen);
        uint32 getPID (const char *pBuf, uint16 ui16BufLen);
        const char * getIdentifier (const char *pBuf, uint16 ui16BufLen);
        const EndPointsInfo * getEndPointsInfo (const char *pBuf, uint16 ui16BufLen);
        const StatisticsInfo * getStatistics (const char *pBuf, uint16 ui16BufLen);
        const MessageStatisticsInfo * getMessageStatistics (const char *pBuf, uint16 ui16BufLen);
        const MessageStatisticsInfo * getPerTypeMessageStatistics (const char *pBuf, uint16 ui16BufLen, const MessageStatisticsInfo *pPrev);
        virtual int parseUpdate (const char *pBuf, uint16 ui16BufLen);

        int handleMessage (void);

    private:
        NOMADSUtil::StringHashtable<NOMADSUtil::String> _stats;
        NOMADSUtil::StringHashtable<int64> _timestats;
        FILE *_fileLog;
        MessageHandlerCallbackFnPtr _pHandlerFn;
        void *_pCallbackArg;
        NOMADSUtil::UDPDatagramSocket *_pDGSocket;
        NOMADSUtil::String _relayIP;
        uint16 _ui16RelayPort;
        NOMADSUtil::UDPDatagramSocket *_pDGRelaySocket;
        int64 _i64StartTime;
        int64 _i64delay;
        char _szBuf[1024];    // NOTE: This is the maximum size of an individual message that can be supported
};

#endif   // #ifndef INCL_MOCKET_STATUS_MONITOR_H
