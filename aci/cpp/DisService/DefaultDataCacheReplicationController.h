/*
 * DefaultDataCacheReplicationController.h
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

#ifndef INCL_DEFAULT_DATE_CACHE_REPLICATION_CONTROLLER_H
#define INCL_DEFAULT_DATE_CACHE_REPLICATION_CONTROLLER_H

#include "DataCacheReplicationController.h"

namespace IHMC_ACI
{
    class DisseminationService;
}

namespace IHMC_ACI
{
    class DefaultDataCacheReplicationController : public DataCacheReplicationController
    {
        public:
            DefaultDataCacheReplicationController (DisseminationService *pDisService,
                                                   bool bRequireAck = DataCacheReplicationController::DEFAULT_REQUIRE_ACK);
            DefaultDataCacheReplicationController (DisseminationServiceProxyServer *pDisServiceProxy);
            ~DefaultDataCacheReplicationController(void);

            void newNeighbor (const char *pszNodeUUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUUID);
            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);
            void dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad);

        protected:
            void disServiceControllerMsgArrived (ControllerToControllerMsg *pCtrlMsg);
            void disServiceControlMsgArrived (DisServiceCtrlMsg *pCtrlMsg);
            void disServiceDataMsgArrived (DisServiceDataMsg *pDataMsg);

    };
}

#endif  // INCL_DEFAULT_DATE_CACHE_REPLICATION_CONTROLLER_H


