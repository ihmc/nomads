/*
 * DisseminationServiceProxyServer.h
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

#ifndef INCL_DISSEMINATION_SERVICE_PROXY_SERVER_H
#define INCL_DISSEMINATION_SERVICE_PROXY_SERVER_H

#define DIS_SVC_PROXY_SERVER_PORT_NUMBER 56487   // Also see DisseminationServiceProxy.cpp

#include "DisseminationServiceProxyAdaptor.h"

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
    class DisseminationService;

    class DisseminationServiceProxyServer : public NOMADSUtil::ManageableThread
    {
        public:
            DisseminationServiceProxyServer (void);
            ~DisseminationServiceProxyServer (void);

            DisseminationService *getDisseminationServiceRef (void);

            int init (DisseminationService *pDissSvc, uint16 ui16PortNum = DIS_SVC_PROXY_SERVER_PORT_NUMBER,
                      const char *pszProxyServerInterface = NULL);

            void run (void);

            void requestTermination (void);
            void requestTerminationAndWait (void);

    private:
            NOMADSUtil::TCPSocket *_pServerSock;
            DisseminationService *_pDissSvc;

        private:
            friend class DSProxyServerConnHandler;
            friend class DisseminationServiceProxyAdaptor;

            NOMADSUtil::StringHashtable<DisseminationServiceProxyAdaptor> _proxies;
    };

    class DSProxyServerConnHandler : public NOMADSUtil::ManageableThread
    {
        public:
            DSProxyServerConnHandler (DisseminationServiceProxyServer *pDSPS, NOMADSUtil::SimpleCommHelper2 *pCommHelper);
            ~DSProxyServerConnHandler (void);

            void run (void);

        private:
            void doRegisterProxy (NOMADSUtil::SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);
            void doRegisterProxyCallback (NOMADSUtil::SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);

        private:
            NOMADSUtil::SimpleCommHelper2 *_pCommHelper;
            DisseminationServiceProxyServer *_pDissSvcProxyServer;
    };

    inline DisseminationService* DisseminationServiceProxyServer::getDisseminationServiceRef(void)
    {
        return _pDissSvc;
    }
}

#endif   // #ifndef INCL_DISSEMINATION_SERVICE_PROXY_SERVER_H
