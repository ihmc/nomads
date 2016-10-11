/*
 * IncomingMessageForwardingPolicy.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on June 20, 2013, 2:40 PM
 */

#ifndef INCL_MESSAGE_FORWARDING_POLICY_H
#define	INCL_MESSAGE_FORWARDING_POLICY_H

#include "PeerNodeContext.h"
#include "PtrLList.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class MetaData;
    class Targets;
    class Topology;

    class MessageForwardingPolicy
    {
        public:
            MessageForwardingPolicy (const char *pszNodeId);
            virtual ~MessageForwardingPolicy (void);

            NOMADSUtil::PtrLList<PeerNodeContext> * getNodesToMatch (PeerNodeContextList *pPeerNodeContextList,
                                                                     const char *pszMsgSource, Topology *_pTopology, const char *pszMsgId,
                                                                     MetaData *pMetadata);

        private:
            bool isForwardingTarget (Targets **ppTargets, const char *pszNodeId);
            bool isActiveForwardingTarget (PeerNodeContextList *pActiveForwardingPeers, const char *pszNodeId);

        private:
            const NOMADSUtil::String _nodeId;
    };
}

#endif	// INCL_MESSAGE_FORWARDING_POLICY_H

