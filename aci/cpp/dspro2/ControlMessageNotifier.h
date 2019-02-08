/**
 * Instrumentator.h
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
 * Created on September 24, 2010, 11:54 AM
 */

#ifndef INCL_CONTROL_MESSAGE_NOTIFIER_H
#define INCL_CONTROL_MESSAGE_NOTIFIER_H

#include "ControlMessageListener.h"
#include "DArray2.h"
#include "FTypes.h"
#include "LoggingMutex.h"

namespace IHMC_ACI
{
    class ControlMessageNotifier : public ControlMessageListener
    {
        public:
            ControlMessageNotifier (NOMADSUtil::LoggingMutex *pmCallback);
            virtual ~ControlMessageNotifier (void);

            int registerAndEnableControllerMessageListener (uint16 ui16ClientId, ControlMessageListener *pControllerMessageListener, uint16 &ui16AssignedClientId);
            int deregisterAndDisableControllerMessageListener (uint16 ui16ClientId);

            bool isEnabled (void);

            bool contextUpdateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool contextVersionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool messageRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool chunkRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool positionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool searchMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool topologyReplyMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool topologyRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool updateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool versionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool waypointMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);
            bool wholeMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId);

        private:
            struct ControlMessageClientInfoPro
            {
                ControlMessageClientInfoPro (void);
                ~ControlMessageClientInfoPro (void);

                ControlMessageListener *pControllerMessageListener;
            };

            unsigned int _uiNListeners;
            NOMADSUtil::LoggingMutex *_pmCallback;
            NOMADSUtil::DArray2<ControlMessageNotifier::ControlMessageClientInfoPro> _ctrlMsgClients;
    };

    extern ControlMessageNotifier *pCtrlMsgNotifier;

    inline bool ControlMessageNotifier::isEnabled()
    {
        return _uiNListeners > 0;
    }
}

#endif    // INCL_CONTROL_MESSAGE_NOTIFIER_H
