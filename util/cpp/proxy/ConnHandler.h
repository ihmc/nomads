/*
 * ConnHandler.h
 * 
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Template class that implements a generic handler that handles a clints's (stub's)
 * request to connect to a service registry by performing the handshake with the
 * remote client (stub).
 * If the handshake is successful, a skeleton is created and added to the
 * registry.  The skeleton will handle the established connection.
 *
 * authors : Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on February 16, 2015, 11:38 PM
 */

#ifndef INCL_SKELETON_CONN_HANDLER_H
#define	INCL_SKELETON_CONN_HANDLER_H

#include "Skeleton.h"
#include "Protocol.h"

namespace NOMADSUtil
{
    template<class S>
    class SkeletonConnHandler : public NOMADSUtil::ManageableThread
    {
        public:
            SkeletonConnHandler (RegistryInterface<S> *pRegistry, UnmarshalFnPtr pUnmarshaller, SimpleCommHelper2 *pCommHelper,
                                 const char *pszService, const char *pszVersion);
            ~SkeletonConnHandler (void);

            void run (void);

        private:
            SimpleCommHelper2::Error doRegisterProxy (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);
            SimpleCommHelper2::Error doRegisterProxyCallback (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationID);

        private:
            RegistryInterface<S> *_pRegistry;
            UnmarshalFnPtr _pUnmarshaller;
            SimpleCommHelper2 *_pCommHelper;
            const String _service;
            const String _version;
    };

    template<class S>
    SkeletonConnHandler<S>::SkeletonConnHandler (RegistryInterface<S> *pRegistry, UnmarshalFnPtr pUnmarshaller,
                                                 SimpleCommHelper2 *pCommHelper, const char *pszService, const char *pszVersion)
        : _pRegistry (pRegistry),
          _pUnmarshaller (pUnmarshaller),
          _pCommHelper (pCommHelper),
          _service (pszService),
          _version (pszVersion)
    {
    }

    template<class S>
    SkeletonConnHandler<S>::~SkeletonConnHandler (void)
    {
        if (isRunning()) {
            requestTerminationAndWait();
        }
    }

    template<class S>
    void SkeletonConnHandler<S>::run()
    {
        const char * const pszMethodName = "SkeletonConnHandler::run";

        // Initial handshake        
        SimpleCommHelper2::Error err = Protocol::doHandshake (_pCommHelper, _service, _version);
        if (SimpleCommHelper2::None != err) {
            _pCommHelper->closeConnection (err);
            delete _pCommHelper;
            return;
        }

        const char **ppszBuf = _pCommHelper->receiveParsedSpecific ("1 1", err);
        if (ppszBuf == NULL) {
            err = SimpleCommHelper2::ProtocolError;
            _pCommHelper->closeConnection (err);
            delete _pCommHelper;
        }
        String cmd (ppszBuf[0]);
        cmd.trim();
        if (err == SimpleCommHelper2::None) {
            if (cmd == Protocol::REGISTER_PROXY) {
                err = doRegisterProxy (_pCommHelper, atoi (ppszBuf[1]));
            }
            else if (cmd == Protocol::REGISTER_PROXY_CALLBACK) {
                err = doRegisterProxyCallback (_pCommHelper, atoi (ppszBuf[1]));
            }
            else {
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_MildError,
                                "unknown command: <%s>.\n", cmd.c_str());
                err = SimpleCommHelper2::ProtocolError;
            }
        }

        switch (err) {
            case SimpleCommHelper2::None:
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Info,
                                "%s successful.\n", cmd.c_str());
                break;

            case SimpleCommHelper2::CommError:
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_MildError,
                                "communication error during initial handshake.\n");
                _pCommHelper->closeConnection (err);
                delete _pCommHelper;
                break;

            case SimpleCommHelper2::ProtocolError:
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_MildError,
                                "protocol error during initial handshake.\n");
                _pCommHelper->closeConnection (err);
                delete _pCommHelper;
                break;

            default:
                assert (false);
        }

        delete this;
    }

    template<class S>
    SimpleCommHelper2::Error SkeletonConnHandler<S>::doRegisterProxy (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationId)
    {
        const char *pszMethodName = "SkeletonConnHandler::doRegisterProxy";
        SimpleCommHelper2::Error err = SimpleCommHelper2::None;
        Skeleton<S> *pSkeleton = new Skeleton<S> (_pRegistry, _pUnmarshaller);
        const uint16 ui16AppId = _pRegistry->registerSkeleton (ui16ApplicationId, pSkeleton);
        if (pSkeleton->init (pCommHelper, ui16AppId) != 0) {
            if (pLogger != NULL) pLogger->logMsg (pszMethodName,
                              Logger::L_SevereError, "adaptor init failed\n");
            _pRegistry->removedSkeleton (ui16AppId);
            delete pSkeleton;
        }
        else {
            pCommHelper->sendLine (err, "OK %d", ui16AppId);
            if (err != SimpleCommHelper2::None) {
                if (pLogger != NULL) pLogger->logMsg (pszMethodName,
                              Logger::L_SevereError, "sendLine failed\n");
                _pRegistry->removedSkeleton (ui16AppId);
                delete pSkeleton;
            }
            else {
                pSkeleton->start();
            }
        }
        return err;
    }

    template<class S>
    SimpleCommHelper2::Error SkeletonConnHandler<S>::doRegisterProxyCallback (SimpleCommHelper2 *pCommHelper, uint16 ui16ApplicationId)
    {
        const char *pszMethodName = "SkeletonConnHandler::doRegisterProxyCallback";
        SimpleCommHelper2::Error err = SimpleCommHelper2::None;
        Skeleton<S> *pSkeleton = _pRegistry->getSkeleton (ui16ApplicationId);
        if (pSkeleton == NULL) {
            if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_MildError,
                    "did not find proxy with id %d to register a callback handler\n",
                    (int) ui16ApplicationId);
            SimpleCommHelper2::Error err = SimpleCommHelper2::None;
            pCommHelper->sendLine (err, "ERROR: proxy with id %d not found", ui16ApplicationId);
            err = SimpleCommHelper2::ProtocolError; // register callback arrived before registering the proxy
        }
        else {
            pSkeleton->setCallbackCommHelper (pCommHelper);
            SimpleCommHelper2::Error err = SimpleCommHelper2::None;
            pCommHelper->sendLine (err, "OK");
            if (err != SimpleCommHelper2::None) {
                if (pLogger != NULL) pLogger->logMsg (pszMethodName,
                              Logger::L_SevereError, "sendLine failed\n");
            }
        }
        return err;
    }
}


#endif	/* INCL_SKELETON_CONN_HANDLER_H */

