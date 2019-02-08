/*
 * MocketConnListener.h
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
 * Created on October 12, 2012, 5:18 PM
 */

#ifndef INCL_MOCKET_CONN_LISTENER_H
#define INCL_MOCKET_CONN_LISTENER_H

#include "ConnListener.h"
#include "ServerMocket.h"

namespace IHMC_ACI
{
    class CommAdaptorListener;
    class ConnCommAdaptor;
    class ConnHandler;
    class ConnEndPoint;

    class MocketConnListener : public ConnListener
    {
        public:
            MocketConnListener (const char *pszListenAddr, uint16 ui16Port,
                                const char *pszNodeId, const char *pszSessionId,
                                const char *pszMocketConfigFile,
                                CommAdaptorListener *pListener,
                                ConnCommAdaptor *pConnCommAdaptor,
                                const char *pszCertFile, const char *pszKeyFile);
            virtual ~MocketConnListener (void);

            void requestTermination (void);
            void requestTerminationAndWait (void);

            void run (void);

        protected:
            ConnHandler * getConnHandler (ConnEndPoint *pEndPoint, const NOMADSUtil::String &remotePeer);
            ConnEndPoint * acceptConnection (void);

        private:
            ServerMocket _servMocket;
    };
}

#endif    /* INCL_MOCKET_CONN_LISTENER_H */
