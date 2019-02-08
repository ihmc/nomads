/*
 * DataRequestServer.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 5, 2015, 2:38 PM
 */

#ifndef INCL_DATA_REQUEST_SERVER_H
#define	INCL_DATA_REQUEST_SERVER_H

#include "DisServiceMsg.h"
#include "MessageId.h"
#include "ServingRequestProbability.h"

#include "BloomFilter.h"
#include "Mutex.h"
#include "StringHashset.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DataCacheInterface;
    class DisseminationService;
    class NetworkTrafficMemory;
    class TransmissionService;
    class DisServiceStats;

    class DataRequestServer
    {
        public:
            DataRequestServer (DisseminationService *pDisService, bool bTargetFilteringEnabled, bool bOppListeningEnabled);
            virtual ~DataRequestServer (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            void startedPublishingMessage (const char *pszMsgId);
            void endedPublishingMessage (const char *pszMsgId);

            int handleDataRequestMessage (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface, int64 i64Timeout);
            int handleDataRequestMessage (const char *pszMsgId, DisServiceMsg::Range *pRange,bool bIsChunk, const char *pszTarget,
                                          unsigned int uiNumberOfActiveNeighbors, int64 i64RequestArrivalTime, const char **ppszOutgoingInterfaces,
                                          int64 i64Timeout);

        private:
            int handleDataRequest (const char *pszMsgId, NOMADSUtil::PtrLList<DisServiceMsg::Range> *pRequestedRanges,
                                   const char *pszTarget, unsigned int uiNumberOfActiveNeighbors, int64 i64RequestArrivalTime,
                                   const char **ppszOutgoingInterfaces, int64 i64Timeout);
            int filterAndSendMatchingFragment (const MessageId &msgId, Message *pMsg, DisServiceMsg::Range *pRange, const char *pszTarget,
                                               int64 i64RequestArrivalTime, const char **ppszOutgoingInterfaces);
            bool ignoreRequest (const MessageId &msgId, int64 i64Timeout, unsigned int uiNumberOfActiveNeighbors);

        private:
            bool _bOppListeningEnabled;
            bool _bTargetFilteringEnabled;
            const NOMADSUtil::String _nodeId;

            DisseminationService *_pDisService;/*
            DataCacheInterface *_pDataCacheInterface;
            NetworkTrafficMemory *_pNetTrafficMemory;
            TransmissionService *_pTrSvc;
            DisServiceStats *_pStats;*/

            NOMADSUtil::Mutex _m;
            NOMADSUtil::BloomParameters _bfParams;
            NOMADSUtil::BloomFilter _randomlyIgnoredReqs;
            ServingRequestProbability _sevingReqProb;
            NOMADSUtil::StringHashset _msgBeingSent;
    };
}

#endif

