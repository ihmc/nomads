/*
 * WorldStateForwardingController.h
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

#ifndef INCL_WORLDSTATE_FORWARDING_CONTROLLER_H
#define INCL_WORLDSTATE_FORWARDING_CONTROLLER_H

#include "ForwardingController.h"

namespace IHMC_ACI
{
    class WorldStateForwardingController : public ForwardingController, public GroupMembershipListener
    {
        public:
            WorldStateForwardingController (DisseminationService *pDisService);
            WorldStateForwardingController (DisseminationService *pDisService, float fForwardProbability);

            virtual ~WorldStateForwardingController (void);

            void newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUUID);
            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);

            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, DisServiceMsg *pDSMsg,
                                     uint32 ui32SourceIPAddress, const char *pszIncomingInterface);

            void newSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);
            void removedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);
            void modifiedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);

        private:
    };
}

#endif // end INCL_WORLDSTATE_FORWARDING_CONTROLLER_H
