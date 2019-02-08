/*
 * DisServiceStatusMonitor.h
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

#ifndef INCL_DIS_SERVICE_STATUS_MONITOR_H
#define INCL_DIS_SERVICE_STATUS_MONITOR_H

#include "DisServiceStatus.h"

#include "FTypes.h"
#include "ManageableThread.h"
#include "StrClass.h"

#include <stdio.h>

namespace NOMADSUtil
{
    class UDPDatagramSocket;
}

namespace IHMC_ACI
{
    struct DisServiceBasicStatisticsInfo;
    struct DisServiceStatsInfo;

    class DisServiceStatusMonitorListener
    {
        public:
            virtual ~DisServiceStatusMonitorListener (void);
            virtual int clientConnected (const char *pszSenderIPAddr, uint16 ui16ClientId);
            virtual int clientDisconnected (const char *pszSenderIPAddr, uint16 ui16ClientId);
            virtual int summaryStats (const char *pszSenderIPAddr, DisServiceBasicStatisticsInfo *pDSBSI, DisServiceStatsInfo *pOverallDSSI);

            // A string-formatted version of the message
            // NOTE: The first call to this will be for the header with the column labels
            // NOTE: pszSenderIPAddr may be NULL
            virtual int message (const char *pszSenderIPAddr, const char *pszMessage);
    };

    class DisServiceStatusMonitor : public NOMADSUtil::ManageableThread
    {
        public:
            DisServiceStatusMonitor (void);
            virtual ~DisServiceStatusMonitor (void);

            int initReceiveSocket (uint16 ui16StatPort);

            int initRelaying (const char *pszRelayIP, uint16 ui16RelayPort);
            void setMinRelayInterval (uint32 ui32MilliSec);
            uint32 getMinRelayInterval (void);

            int initFileOutput (FILE *fileLog = NULL);

            int initListener (DisServiceStatusMonitorListener *pListener);

            void run (void);

        protected:
            int printHeader (void);

            DisServiceStatusNoticeType getMessageType (const char *pBuf, uint16 ui16BufLen);

            int handleClientConnected (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen);
            int handleClientDisconnected (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen);
            int handleSummaryStats (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen);
            int handleDetailedStats (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen);
            int handleTopologyStatus (const char *pszSenderIPAddr, const char *pBuf, uint16 ui16BufLen);

            int handleStringInBuffer (const char *pszSenderIPAddr);

        private:
            FILE *_pFileLog;
            DisServiceStatusMonitorListener *_pListener;
            NOMADSUtil::UDPDatagramSocket *_pDGSocket;
            NOMADSUtil::String _relayIP;
            uint16 _ui16RelayPort;
            NOMADSUtil::UDPDatagramSocket *_pDGRelaySocket;
            int64 _i64StartTime;
            uint32 _ui32RelayInterval;
            int64 _i64LastSummaryStatsRelayTime;
            int64 _i64LastDetailedStatsRelayTime;
            char _szBuf[2048];    // NOTE: This is the maximum size of an individual message that can be supported
    };
}

#endif   // #ifndef INCL_DIS_SERVICE_STATUS_MONITOR_H
