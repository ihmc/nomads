/* 
 * ConnListener.h
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
 * Created on April 14, 2014, 6:19 PM
 */

#ifndef INCLUDE_CONN_LISTENER_H
#define	INCLUDE_CONN_LISTENER_H

#include "ManageableThread.h"

#include "StrClass.h"

namespace IHMC_ACI
{
    class CommAdaptorListener;
    class ConnCommAdaptor;
    class ConnEndPoint;
    class ConnHandler;

    class ConnListener : public NOMADSUtil::ManageableThread
    {
        public:
            ConnListener (const char *pszListenAddr, uint16 ui16Port,
                          const char *pszNodeId, const char *pszSessionId,
                          CommAdaptorListener *pListener,
                          ConnCommAdaptor *pCommAdaptor);
            virtual ~ConnListener (void);

            virtual void run (void);

        protected:
            virtual ConnHandler * getConnHandler (ConnEndPoint *pEndPoint, const NOMADSUtil::String &remotePeer) = 0;
            virtual ConnEndPoint * acceptConnection (void) = 0;

        protected:
            const uint16 _ui16Port;
            const NOMADSUtil::String _listenAddr;
            ConnCommAdaptor *_pCommAdaptor;
            CommAdaptorListener *_pListener;

        private:
            const NOMADSUtil::String _nodeId;
            const NOMADSUtil::String _sessionId;
    };
}

#endif	/* INCLUDE_CONN_LISTENER_H */

