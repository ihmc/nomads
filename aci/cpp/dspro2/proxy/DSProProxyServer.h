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
            DSProProxyServer (bool bStrictHandshake);
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
            const bool _bStrictHandshake;
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
            DSPProxyServerConnHandler (DSProProxyServer *pDSPPS, NOMADSUtil::SimpleCommHelper2 *pCommHelper, bool bStrictHandshake);
            ~DSPProxyServerConnHandler (void);

            void run (void);

        private:
            CommHelperError doHandshake (NOMADSUtil::SimpleCommHelper2 *pCommHelper, bool bStrict);
            CommHelperError doRegisterProxy (NOMADSUtil::SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);
            CommHelperError doRegisterProxyCallback (NOMADSUtil::SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);

        private:
            const bool _bStrictHandshake;
            NOMADSUtil::SimpleCommHelper2 *_pCommHelper;
            DSProProxyServer *_pDisSvcProProxyServer;
    };


    inline DSProProxyServer::DSProProxyServer (bool bStrictHandshake) :
        _bStrictHandshake (bStrictHandshake), _pServerSock (nullptr), _pDSPro (nullptr),
        _proxies (true,   // bCaseSensitiveKeys
                  true,   // bCloneKeys
                  true,   // bDeleteKeys
                  true)   // bDeleteValues
    { }

    inline DSPro * DSProProxyServer::getDisServiceProRef (void)
    {
        return _pDSPro;
    }

    inline DSPProxyServerConnHandler::DSPProxyServerConnHandler (DSProProxyServer *pDisSvcPPS,
                                                                 NOMADSUtil::SimpleCommHelper2 *pCommHelper,
                                                                 bool bStrictHandshake) :
        _bStrictHandshake (bStrictHandshake), _pCommHelper (pCommHelper), _pDisSvcProProxyServer (pDisSvcPPS)
    { }

    inline DSPProxyServerConnHandler::~DSPProxyServerConnHandler()
    {
        _pCommHelper->setDeleteUnderlyingSocket (true);
        delete _pCommHelper;
    }
}

#endif // INCL_DISSERVICE_PRO_PROXY_SERVER_H
