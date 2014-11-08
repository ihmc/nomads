/*
 * ForwardingController.h
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
 */

#ifndef INCL_FORWARDING_CONTROLLER_H
#define INCL_FORWARDING_CONTROLLER_H

#include "Listener.h"
#include "Services.h"

#include "FTypes.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_ACI
{
    class DataCacheService;
    class DisseminationService;
    class MessageHeader;
    class PeerState;

    class ForwardingController : public MessageListener, public MessagingService, public PeerStateListener
    {
        public:
            enum Type {
                FC_Default = 0x00,
                FC_WS = 0x01
            };

            ForwardingController (Type type, DisseminationService *pDisService);
            virtual ~ForwardingController (void);

            int setDefaultForwardingcontroller (ForwardingController *pDefFwdCtrl);

            uint8 getType (void);

            //------------------------------------------------------------------
            // ForwardingController -> DisService
            //------------------------------------------------------------------

            int handleMessageWithDefaults (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                           DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                           const char *pszIncomingInterface);

            PeerState * lockAndGetWorldState (void);
            void releaseWorldState (PeerState * pPeerState);

        protected:
            static const bool REQUIRE_ACKNOWLEDGMENT;
            Type _type;
            DisseminationService *_pDisService;
            DataCacheService *_pDCCtlr;
            NOMADSUtil::ConfigManager *_pConfigManager;

        private:
            ForwardingController *_pDefFwdCtrl;
    };

    inline uint8 ForwardingController::getType (void)
    {
        return _type;
    }
}

#endif   // #ifndef INCL_FORWARDING_CONTROLLER_H
