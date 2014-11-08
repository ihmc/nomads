/*
 * SubscriptionForwardingController.h
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
 *
 * Author: Mirko Gilioli (mgilioli@ihmc.us)
 * Created on July 31, 2012, 10:28 AM
 */

#ifndef INCL_SUBSCRIPTION_FORWARDING_CONTROLLER_H
#define	INCL_SUBSCRIPTION_FORWARDING_CONTROLLER_H

#include "FTypes.h"
#include "Listener.h"
#include "PtrLList.h"
#include "StrClass.h"
#include "SubscriptionAdvTable.h"

namespace NOMADSUtil
{
    class String;
    template <class T> class PtrLList;
}

namespace IHMC_ACI
{
    class DisServiceMsg;
    class DisseminationService;
    class SubscriptionAdvTable;
        
    class SubscriptionForwardingController : public MessageListener, public PeerStateListener
    {
        public:
            SubscriptionForwardingController (DisseminationService *pDisService);
            ~SubscriptionForwardingController (void);
            
            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                     DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                     const char *pszIncomingInterface);
            
            void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUID);
            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);
            void setAdvertisementThreshold (uint16 ui16AdvThreshold);
            void setAdvertisementPeriod (int64 i64AdvPeriod);
            void advertiseSubscriptions (void);
            void configureTable (int64 i64LifeTimeUpdateThreshold, int64 i64SeqNoLifetimeExpirationThreshold); 
            
        private:
            
            int updateTable (const char *pszSenderNodeId, NOMADSUtil::PtrLList<NOMADSUtil::String> *pSubList,
                             NOMADSUtil::PtrLList<NOMADSUtil::String> *pNodeList);
            
            struct CtrlSeqNo 
            {
                CtrlSeqNo (uint32 ui32SeqNo, int64 i64TimeStamp);
                
                uint32 _ui32SeqNo;
                int64 _i64TimeStamp;
            };
            
            static const uint16 DEFAULT_ADV_THRESHOLD = 3;
            uint16 _ui16AdvThreshold;
            int64 _i64AdvPeriod;
            int64 _i64LastAdvTime;
            int64 _i64SeqNoLifetimeExpirationThreshold;
            uint16 _ui16LiveNeighbors;
            DisseminationService *_pDisService;
            SubscriptionAdvTable _subAdvTable;
            NOMADSUtil::StringHashtable<CtrlSeqNo> _lastSeenCtrlSeqNoHashtable;
    };
}


#endif	/* INCL_SUBSCRIPTION_FORWARDING_CONTROLLER_H */

