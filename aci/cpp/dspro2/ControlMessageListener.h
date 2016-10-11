/* 
 * ControlMessageListener.h
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
 * Created on November 30, 2012, 5:37 PM
 */

#ifndef INCL_CONTROL_MESSAGE_LISTENER_H
#define	INCL_CONTROL_MESSAGE_LISTENER_H

namespace IHMC_ACI
{
    class ControlMessageListener
    {
        public:
            ControlMessageListener (void);
            virtual ~ControlMessageListener (void);

            virtual bool contextUpdateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool contextVersionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool messageRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool chunkRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool positionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool searchMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool topologyReplyMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool topologyRequestMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool updateMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool versionMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool waypointMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
            virtual bool wholeMessageArrived (const char *pszSenderNodeId, const char *pszPublisherNodeId) = 0;
    };

    inline ControlMessageListener::ControlMessageListener (void)
    {
    }

    inline ControlMessageListener::~ControlMessageListener (void)
    {
    }
};

#endif	/* INCL_CONTROL_MESSAGE_LISTENER_H */

