/*
 * TopologyForwardingController.h
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

#ifndef INCL_TOPOLOGY_FORWARDING_CONTROLLER_H
#define INCL_TOPOLOGY_FORWARDING_CONTROLLER_H

#include "ForwardingController.h"

#include "TimeBoundedStringHashset.h"

namespace IHMC_ACI
{
    class DisServiceMsg;
    class MessageHeader;

    class TopologyForwardingController : public ForwardingController
    {
        public:
            TopologyForwardingController (DisseminationService *pDisService);

            virtual ~TopologyForwardingController (void);

            void newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUUID);

            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);

            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);

            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, DisServiceMsg *pDSMsg,
                                     uint32 ui32SourceIPAddress, const char *pszIncomingInterface);

        public:
            static const uint32 DEFAULT_MESSAGE_HISTORY_DURATION = 10000; // 10 Seconds

        private:
            void doTopologyForwarding (DisServiceMsg *pDSMsg);
            bool isMsgTargetNode(const char *pszNodeId, const char *pszTargetNodeId);
            void doStatefulForwarding (DisServiceMsg *pDSMsg);
            void doFloodingForwarding (DisServiceMsg *pDSMsg);

            void init (void);

        private:
            bool _bForwardDataMsgs;
            bool _bForwardDataRequestMsgs;
            bool _bForwardChunkQueryMsgs;
            bool _bForwardChunkQueryHitsMsgs;
            bool _bForwardTargetedMsgs;
            bool _bForwardSearchMsgs;
            bool _bForwardSearchReplyMsgs;

            NOMADSUtil::TimeBoundedStringHashset _msgHistory;

            uint16 _ui16NumberOfActiveNeighbors;
    };
}

#endif // end INCL_TOPOLOGY_FORWARDING_CONTROLLER_H
