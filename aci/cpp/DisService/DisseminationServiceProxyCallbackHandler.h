/*
 * DisseminationServiceProxyCallbackHandler.h
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

#ifndef INCL_DISSEMINATION_SERVICE_PROXY_CALLBACK_HANDLER_H
#define INCL_DISSEMINATION_SERVICE_PROXY_CALLBACK_HANDLER_H

#include "DisseminationServiceProxy.h"

#include "CommHelper2.h"
#include "ManageableThread.h"

namespace IHMC_ACI
{
    class DisseminationServiceProxyCallbackHandler : public NOMADSUtil::ManageableThread
    {
        public:
            DisseminationServiceProxyCallbackHandler (DisseminationServiceProxy *pProxy, NOMADSUtil::CommHelper2 *pCommHelper);
            virtual ~DisseminationServiceProxyCallbackHandler (void);

            void run (void);
            int doDataArrivedCallback (void);
            int doChunkArrivedCallback (void);
            int doMetadataArrivedCallback (void);
            int doDataAvailableCallback (void);
            int doSearchArrivedCallback (void);
            int doNewPeerCallback (void);
            int doDeadPeerCallback (void);

            private:
                DisseminationServiceProxy *_pProxy;
                NOMADSUtil::CommHelper2 *_pCommHelper;
    };
}

#endif   // #ifndef INCL_DISSEMINATION_SERVICE_PROXY_CALLBACK_HANDLER_H
