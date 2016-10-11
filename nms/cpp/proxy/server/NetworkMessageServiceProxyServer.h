/* 
 * NetworkMessageServiceProxyServer.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Created on February 17, 2015, 2:01 AM
 */

#ifndef INCL_NETWORK_MESSAGE_SERVICE_PROXY_SERVER_H
#define	INCL_NETWORK_MESSAGE_SERVICE_PROXY_SERVER_H

#include "Registry.h"

#include "NetworkMessageService.h"
#include "NetworkMessageServiceCallbackManager.h"
#include "NetworkMessageServiceUnmarshaller.h"

#include "UInt32Hashtable.h"

namespace NOMADSUtil
{
    class NetworkMessageServiceProxyServer : public Registry<NetworkMessageService>
    {
        public:
            NetworkMessageServiceProxyServer (NetworkMessageService *pNMS, bool bDeallocateNMS);
            ~NetworkMessageServiceProxyServer (void);

            bool registerCallback (uint16 ui16ApplicationId, SimpleCommHelper2 *pCommHelper,
                                   SimpleCommHelper2 *pCallbackCommHelper, SimpleCommHelper2::Error &error);
            bool deregisterAllCallbacks (void);
            bool deregisterCallback (uint16 ui16ApplicationId, CallbackExecutor *pCBackExec,
                                     SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error);

        private:
            typedef UInt32Hashtable<NetworkMessageServiceCallbackManager> ByMsgType;

            UInt32Hashtable<ByMsgType> _cbackHandlersByApplicationId;
            NetworkMessageServiceUnmarshaller _unmarshaller;
    };
}

#endif	/* INCL_NETWORK_MESSAGE_SERVICE_PROXY_SERVER_H */

