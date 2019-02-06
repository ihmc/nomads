/*
 * Registry.h
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
 * Template class that implements a generic TCP server that accepts connections
 * from a remote client and assigns it to a local skeleton that interfaces between
 * the remote application and a generic local service.
 * The template specialization needs to pass a reference to the service, and a
 * reference to a proper specialization of the Unmarshaller template.
 *
 * NOTE: the instance of the service may be shared among multiple threads, and
 *       it is assumed to be thread-safe.
 *
 * authors : Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on February 16, 2015, 8:52 PM
 */

#ifndef INCL_REGISTRY_H
#define INCL_REGISTRY_H

#include "ConnHandler.h"
#include "InetAddr.h"
#include "Mutex.h"
#include "UInt32Hashtable.h"
#include "TCPSocket.h"

namespace NOMADSUtil
{
    template<class S>
    class Registry : public ManageableThread, public RegistryInterface<S>
    {
        public:
            Registry (S *pSvc, UnmarshalFnPtr pUnmarshaller, const char *pszService,
                      const char *pszVersion, bool bDeallocateSvc);
            virtual ~Registry (void);

            int init (uint16 ui16PortNum, const char *pszProxyServerInterface = NULL);

            Skeleton<S> * getSkeleton (uint16 ui16ApplicationId);
            S * getSvcInstace (void);

            uint16 registerSkeleton (uint16 ui16ApplicationId, Skeleton<S> *pAdapor);
            void removedSkeleton (uint16 ui16ApplicationId);

            void run (void);

            // Overrides of ManageableThread methods
            void requestTermination (void);
            void requestTerminationAndWait (void);

        protected:
            S *_pSvc;

        private:
            typedef Skeleton<S> Skel;
            const bool _bDeallocateSvc;
            UnmarshalFnPtr _pUnmarshaller;
            Mutex _m;
            TCPSocket _serverSock;
            UInt32Hashtable<Skel> _proxies;
            const String _service;
            const String _version;
    };

    template<class S>
    Registry<S>::Registry (S *pSvc, UnmarshalFnPtr pUnmarshaller, const char *pszService,
                           const char *pszVersion, bool bDeallocateSvc)
        : _pSvc (pSvc),
          _bDeallocateSvc (bDeallocateSvc),
          _pUnmarshaller (pUnmarshaller),
          _service (pszService),
          _version (pszVersion)
    {
    }

    template<class S>
    Registry<S>::~Registry (void)
    {
        _serverSock.disableReceive();
        if (isRunning()) {
            requestTerminationAndWait();
        }
        if (_bDeallocateSvc && (_pSvc != NULL)) {
            delete _pSvc;
        }
    }

    template<class S>
    int Registry<S>::init (uint16 ui16PortNum, const char *pszProxyServerInterface)
    {
        unsigned long ulIPAddr = INADDR_ANY;
        if (pszProxyServerInterface != NULL) {
            ulIPAddr = InetAddr (pszProxyServerInterface).getIPAddress();
        }
        if (_serverSock.setupToReceive (ui16PortNum, 5, ulIPAddr) < 0) {
            return -1;
        }
        return 0;
    }

    template<class S>
    uint16 Registry<S>::registerSkeleton (uint16 ui16ApplicationId, Skeleton<S> *pSkeleton)
    {
        _m.lock();
        uint16 ui16AppId = ui16ApplicationId;
        for (; _proxies.contains (ui16AppId); ++ui16AppId);
        _proxies.put (ui16AppId, pSkeleton);
        _m.unlock();
        return ui16AppId;
    }

    template<class S>
    void Registry<S>::removedSkeleton (uint16 ui16ApplicationId)
    {
        _m.lock();
        _proxies.remove (ui16ApplicationId);
        _m.unlock();
    }

    template<class S>
    Skeleton<S> * Registry<S>::getSkeleton (uint16 ui16ApplicationId)
    {
        _m.lock();
        Skeleton<S> *pSkeleton = _proxies.get (ui16ApplicationId);
        _m.unlock();
        return pSkeleton;
    }

    template<class S>
    S * Registry<S>::getSvcInstace (void)
    {
        // It assumes that _pSvc is thread-safe!
        return _pSvc;
    }

    template<class S>
    void Registry<S>::run (void)
    {
        const char * const pszMethodName = "Registry::run";
        setName (pszMethodName);
        started();
        if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Info, "Registry started\n.");
        while (!terminationRequested()) {
            // Get next connection, or abort if it fails
            if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Info, "waiting for connection on port: %u\n.", _serverSock.getLocalPort());
            TCPSocket *pClientSock = static_cast<TCPSocket*>(_serverSock.accept());
            if (NULL != pClientSock) {
                if (pClientSock->setLingerOptions (1, 30)) { // Enable SO_LINGER and set the timeout to 30 seconds
                    if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Warning,
                        "failed to set the linger option for the socket\n");
                }
                pClientSock->bufferingMode (false);

                SimpleCommHelper2 *pCommHelper = new SimpleCommHelper2();
                int rc = pCommHelper->init (pClientSock);
                if (rc != 0) {
                    if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_MildError,
                        "could not initialize CommHelper; rc = %d\n", rc);
                    delete pCommHelper;
                    pCommHelper = NULL;
                    terminating();
                    return;
                }

                SkeletonConnHandler<S> *pConnHandler = new SkeletonConnHandler<S> (this, _pUnmarshaller, pCommHelper, _service, _version);
                if (pConnHandler != NULL) {
                    pLogger->logMsg (pszMethodName, Logger::L_Info, "accepted connection from %s:%d.\n",
                                     pClientSock->getRemoteHostAddr(), pClientSock->getRemotePort());
                    pConnHandler->start();
                }
            }
        }
        terminating();
    }

    template<class S>
    void Registry<S>::requestTermination (void)
    {
        _serverSock.disableReceive();
        ManageableThread::requestTermination();
    }

    template<class S>
    void Registry<S>::requestTerminationAndWait (void)
    {
        _serverSock.disableReceive();
        ManageableThread::requestTerminationAndWait();
    }
}

#endif    /* INCL_REGISTRY_H */
