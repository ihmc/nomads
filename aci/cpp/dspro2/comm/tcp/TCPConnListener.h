/*
 * TCPConnListener.h
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
 * Created on April 15, 2014, 3:09 PM
 */

#ifndef INCL_TCP_CONN_LISTENER_H
#define	INCL_TCP_CONN_LISTENER_H

#include "ConnListener.h"
#include "TCPSocket.h"

namespace IHMC_ACI
{
    class CommAdaptorListener;
    class ConnCommAdaptor;
    class ConnHandler;
    class ConnEndPoint;

    class TCPConnListener : public ConnListener
    {
        public:
            TCPConnListener (const char *pszListenAddr, uint16 ui16Port,
                             const char *pszNodeId, const char *pszSessionId,
                             CommAdaptorListener *pListener, ConnCommAdaptor *pConnCommAdaptor);
            virtual ~TCPConnListener (void);

            void requestTermination (void);
            void requestTerminationAndWait (void);

            void run (void);

        protected:
            ConnHandler * getConnHandler (ConnEndPoint *pEndPoint, const NOMADSUtil::String &remotePeer);
            ConnEndPoint * acceptConnection (void);

        private:
            NOMADSUtil::TCPSocket _servSocket;
    };
}

#endif	/* INCL_TCP_CONN_LISTENER_H */

