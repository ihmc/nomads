/**
 * Services.h
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
 * Contact IHMC for other types of licenses.
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on February 14, 2013, 7:47 PM
 */

#ifndef INCL_DSPRO_SERVICES_H
#define INCL_DSPRO_SERVICES_H

#include "SearchProperties.h"

namespace IHMC_ACI
{
    class CommAdaptorManager;
    class DSProImpl;
    class Targets;
    class Topology;

    class TopologySvc
    {
        public:
            explicit TopologySvc (DSProImpl *pDSPro);
            virtual ~TopologySvc (void);

        protected:
            Topology * getTopology (void);

        private:
            DSProImpl *_pDSPro;
    };

    class MessagingSvc
    {
        public:
            explicit MessagingSvc (DSProImpl *pDSPro);
            virtual ~MessagingSvc (void);

        protected:
            int addRequestedMessageToUserRequests (const char *pszId, const char *pszQueryId=nullptr);
            int sendAsynchronousRequestMessage (const char *pszId);
            int removeAsynchronousRequestMessage (const char *pszId);
            int sendSearchMessage (SearchProperties &searchProp, Targets **ppTargets);
            int sendSearchReplyMessage (const char *pszQueryId, const char **ppszMatchingMsgIds,
                                        const char *pszTarget, const char *pszMatchingNode,
                                        Targets **ppTargets);
            int sendSearchReplyMessage (const char *pszQueryId, const void *pReply, uint16 ui16ReplyLen,
                                        const char *pszTarget, const char *pszMatchingNode,
                                        Targets **ppTargets);

        private:
            DSProImpl *_pDSPro;
    };

    class ApplicationNotificationSvc
    {
        public:
            explicit ApplicationNotificationSvc (DSProImpl *pDSPro);
            virtual ~ApplicationNotificationSvc (void);

        protected:
            void asynchronouslyNotifyMatchingMetadata (const char *pszQueryId, const char **ppszMatchingMsgIds);
            void asynchronouslyNotifyMatchingSearch (const char *pszQueryId, const void* pReply, uint16 ui16ReplyLen);

        private:
            DSProImpl *_pDSPro;
    };
}

#endif // INCL_DSPRO_SERVICES_H
