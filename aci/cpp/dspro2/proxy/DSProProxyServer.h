/**
 * DSProProxyServer.h
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
 */

#ifndef INCL_DISSERVICE_PRO_PROXY_SERVER_H
#define INCL_DISSERVICE_PRO_PROXY_SERVER_H

#define DISPRO_SVC_PROXY_SERVER_PORT_NUMBER 56487   // Also see DisServiceProProxy.cpp

#include "DSProProxyAdaptor.h"

#include "DisseminationServiceProxyServer.h"

#include "ManageableThread.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class SimpleCommHelper2;
    class Logger;
    class TCPSocket;
}

namespace IHMC_ACI
{
    class DSPro;

    class DSProProxyServer : public NOMADSUtil::ManageableThread
    {
        public:
            DSProProxyServer (void);
            virtual ~DSProProxyServer (void);

            DSPro * getDisServiceProRef (void);

            int init (DSPro *pDisSvcPro, const char *pszProxyServerInterface,
                      uint16 ui16PortNum = DISPRO_SVC_PROXY_SERVER_PORT_NUMBER);

            int initDisseminationServiceProxyServer (DisseminationService *pDisService,
                                                     const char *pszProxyServerInterface,
                                                     uint16 ui16PortNum = DISPRO_SVC_PROXY_SERVER_PORT_NUMBER);

            void run (void);

            void requestTermination (void);
            void requestTerminationAndWait (void);

        public:
            static const NOMADSUtil::String SERVICE;
            static const NOMADSUtil::String VERSION;

        private:
            NOMADSUtil::TCPSocket *_pServerSock;            
            DSPro *_pDSPro;

        private:
            friend class DSPProxyServerConnHandler;
            friend class DSProProxyAdaptor;

            DisseminationServiceProxyServer _disServiceProxySrv;
            NOMADSUtil::StringHashtable<DSProProxyAdaptor> _proxies;
    };

    class DSPProxyServerConnHandler : public NOMADSUtil::ManageableThread
    {
        public: 
            DSPProxyServerConnHandler (DSProProxyServer *pDSPPS, NOMADSUtil::SimpleCommHelper2 *pCommHelper);
            ~DSPProxyServerConnHandler (void);

            void run (void);

        private:
            CommHelperError doHandshake (NOMADSUtil::SimpleCommHelper2 *pCommHelper);
            CommHelperError doRegisterProxy (NOMADSUtil::SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);
            CommHelperError doRegisterProxyCallback (NOMADSUtil::SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);

        private:
            NOMADSUtil::SimpleCommHelper2 *_pCommHelper;
            DSProProxyServer *_pDisSvcProProxyServer;
    };

    inline DSPro * DSProProxyServer::getDisServiceProRef (void)
    {
        return _pDSPro;
    }
}

#endif // INCL_DISSERVICE_PRO_PROXY_SERVER_H
