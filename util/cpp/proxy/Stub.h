/* 
 * Stub.h
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
 * This class handles the connection with a remote server and instantiate a
 * StubCallbackHandler to handle the reception and the unmarshaling of remote
 * callback methods.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 27, 2015, 1:09 PM
 */

#ifndef INCL_STUB_H
#define	INCL_STUB_H

#include "Mutex.h"
#include "StubCallbackHandler.h"

namespace NOMADSUtil
{
    class Semaphore;
    class SimpleCommHelper2;

    class StubListener
    {
        public:
            virtual void serverConnected (void) {};
            virtual void serverDisconnected (void) {};
    };

    class Stub : public ManageableThread
    {
        public:
            // pUnmarshaller is a function that is in charge of unmarshaling the callbacks
            Stub (uint16 ui16DesiredApplicationId, StubUnmarshalFnPtr pUnmarshaller,
                  const char *pszService, const char *pszVersion, bool bUseBackgroundReconnect = false);
            virtual ~Stub (void);

            // Initialize the proxy by connecting it to the DisseminationService Proxy Server
            // By default, connects to the proxy on localhost (127.0.0.1)
            // Returns 0 if successful or a negative value in case of error
            int init (const char *pszHost = NULL, uint16 ui16Port = 0);

            uint16 getApplicationId (void);

            // Returns true if we are currently connected to the DisService proxy server, false otherwise. 
            bool isConnected (void);

            // TODO:
            // Need some approach to discard (or not retrieve) queued up data (which might have been published while this node was disconnected)
            // Need some approach to examine the set of available data which can be retrieved
            // Handles background reconnects to the DisService server
            void run (void);

            virtual void requestTermination (void);
            virtual void requestTerminationAndWait (void);

        protected:
            int reregisterListeners (void);

        private:
            friend class StubCallbackHandler;

            SimpleCommHelper2 * connectToServer (const char *pszHost, uint16 ui16Port);
            int registerProxy (SimpleCommHelper2 *pch, SimpleCommHelper2 *pchCallback,
                               uint16 ui16DesiredApplicationId);
            int tryConnect (void);
            bool startReconnect (void);
            bool checkConnection (void);

        protected:
            SimpleCommHelper2 *_pCommHelper;
            Mutex _stubMutex;

        private:
            const bool _bUsingBackgroundReconnect;
            bool _bReconnectStarted;
            uint16 _ui16ApplicationId;
            uint16 _ui16Port;
            String _sHost;
            StubListener *_pListener;
            StubUnmarshalFnPtr _pUnmarshaller;
            StubCallbackHandler *_pHandler;
            Semaphore *_pReconnectSemaphore;
            Mutex _mutexReconnect;
            const String _service;
            const String _version;
    };
}

#endif	/* INCL_STUB_H */

