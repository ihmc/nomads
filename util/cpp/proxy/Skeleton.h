/*
 * Skeleton.h
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
 * Template class that implements a skeleton that receives and parses a remote
 * method invocation request and returns the result of the unmarshaller.
 *
 * authors : Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on February 16, 2015, 11:36 PM
 */

#ifndef INCL_SKELETON_H
#define INCL_SKELETON_H

#include "Callback.h"
#include "RegistryInterface.h"
#include "Unmarshaller.h"

#include "Logger.h"
#include "ManageableThread.h"
#include "StringHashset.h"

namespace NOMADSUtil
{
    template<class S>
    class Skeleton : public NOMADSUtil::ManageableThread, public CallbackExecutor
    {
        public:
            Skeleton (RegistryInterface<S> *pProxyListener, UnmarshalFnPtr pUnmarshaller);
            ~Skeleton (void);

            int init (SimpleCommHelper2 *pCommHelper, uint16 ui16ClientId);

            void setCallbackCommHelper (SimpleCommHelper2 *pCommHelper);
            int doCallback (Callback *pCBack);

            uint16 getClientId (void);

            void run (void);

        private:
            bool checkAndDispatchRemoteInvokation (const String &methodName, SimpleCommHelper2::Error &error);

        private:
            uint16 _ui16ClientId;
            RegistryInterface<S> *_pRegistry;
            SimpleCommHelper2 *_pCommHelper;
            SimpleCommHelper2 *_pCallbackCommHelper;
            UnmarshalFnPtr _pUnmarshaller;
            StringHashset _registeredFunctions;
    };

    template<class S>
    Skeleton<S>::Skeleton (RegistryInterface<S> *pProxyListener, UnmarshalFnPtr pUnmarshaller)
        : _ui16ClientId (0),
          _pRegistry (pProxyListener),
          _pCommHelper (NULL),
          _pCallbackCommHelper (NULL),
          _pUnmarshaller (pUnmarshaller)
    {
    }

    template<class S>
    Skeleton<S>::~Skeleton (void)
    {
        StringHashset::Iterator iter = _registeredFunctions.getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            iter.getKey();
            _pRegistry->deregisterAllCallbacks (_ui16ClientId);
        }
    }

    template<class S>
    int Skeleton<S>::init (SimpleCommHelper2 *pCommHelper, uint16 ui16ClientId)
    {
        _ui16ClientId = ui16ClientId;
        _pCommHelper = pCommHelper;
        return 0;
    }

    template<class S>
    uint16 Skeleton<S>::getClientId (void)
    {
        return _ui16ClientId;
    }

    template<class S>
    void Skeleton<S>::setCallbackCommHelper (SimpleCommHelper2 *pCommHelper)
    {
        if (_pCallbackCommHelper != NULL) {
            delete _pCallbackCommHelper;
        }
        _pCallbackCommHelper = pCommHelper;
    }

    template<class S>
    int Skeleton<S>::doCallback (Callback *pCBack)
    {
        if (pCBack == NULL) {
            return -1;
        }
        if (!_registeredFunctions.containsKey (pCBack->_cbackName)) {
            return 1;
        }
        return pCBack->marshal (_pCallbackCommHelper);
    }

    template<class S>
    void Skeleton<S>::run (void)
    {
        const char *pszMethodName = "Skeleton::run";
        setName (pszMethodName);

        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        char buf[128];
        while (!terminationRequested()) {

            const int size = _pCommHelper->receiveLine (buf, sizeof (buf), error);
            if (error != SimpleCommHelper2::None) {
                break;
            }
            if (size <= 0) {
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Warning,
                                "received line of size %d.\n", size);
                continue;
            }

            String methodName (buf);
            methodName.trim();
            const int iIdx = methodName.indexOf (' ');
            if (iIdx > 0) {
                methodName = methodName.substring (0, methodName.length());
            }

            const bool bSuccess = checkAndDispatchRemoteInvokation (methodName, error);
            if (error != SimpleCommHelper2::None) {
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_SevereError,
                                "Command <%s> failed.  Communication error.\n", buf);
                break;
            }
            if (bSuccess) {
                _pCommHelper->sendLine (error, "OK");
                if (error != SimpleCommHelper2::None) {
                    if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_SevereError,
                                    "Communication error when notifying command success.\n");
                }
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Info, "Command <%s> worked.\n", buf);
            }
            else {
                _pCommHelper->sendLine (error, "ERROR");
                if (error != SimpleCommHelper2::None) {
                    if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_SevereError,
                                    "Communication error when notifying command error.\n");
                }
                if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Warning,
                        "Command <%s> failed.\n", buf);
            }

            if (error != SimpleCommHelper2::None) {
                break;
            }
        }

        if (error != SimpleCommHelper2::None) {
            buf[127] = '\0';
            if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_MildError,
                    "exception in handling client request %s.\n", buf);
        }

        terminating();

        // the destructor will take care of closing all the connections
        // and de-registering this Skeleton from the Registry.
        if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_MildError,
                    "%s thread terminating.\n", pszMethodName);
        delete this;
    }

    template<class S>
    bool Skeleton<S>::checkAndDispatchRemoteInvokation (const String &methodName, SimpleCommHelper2::Error &error)
    {
        const char *pszMethodName = "Skeleton::checkAndDispatchRemoteInvokation";

        if (_pUnmarshaller == NULL) {
            if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_SevereError,
                            "!!!! UNKNOWN OPERATION!!!! [%s]\n", (const char *) methodName);
            return false;
        }
        if (methodName.startsWith ("register") || methodName.startsWith ("Register")) {
            if (_pRegistry->registerCallback (_ui16ClientId, _pCommHelper, _pCallbackCommHelper, error)) {
                _registeredFunctions.put (methodName);
                return true;
            }
            return false;
        }
        if (methodName.startsWith ("deregister")) {
            if (_pRegistry->deregisterCallback (_ui16ClientId, this, _pCommHelper, error)) {
                _registeredFunctions.remove (methodName);
                return true;
            }
            return false;
        }
        return _pUnmarshaller (_ui16ClientId, methodName, _pRegistry->getSvcInstace(), _pCommHelper, error);
    }
}

#endif    /* INCL_SKELETON_H */
