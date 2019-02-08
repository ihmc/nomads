/*
 * CommCommAdaptor.h
 *
 * CommCommAdaptor is a generic adaptor that
 * factorizes common operations for "connection-based"
 * transports such as sockets, or mockets.
 *
 * It handles the creation of connection listeners that
 * accept connections on specified ports, it handles
 * the creation to connection handlers that connect to
 * remote peers, and it handles the re-connections to
 * remote peers.
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
 * Created on April 14, 2014, 7:14 PM
 */

#ifndef INCL_COMM_COMM_ADAPTOR_H
#define INCL_COMM_COMM_ADAPTOR_H

#include "CommAdaptor.h"
#include "ConnHandler.h"
#include "ConnListener.h"

#include "LList.h"
#include "ManageableThread.h"
#include "Queue.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class ConnEndPoint;

    class ConnCommAdaptor : public CommAdaptor, public NOMADSUtil::ManageableThread
    {
        public:
            virtual ~ConnCommAdaptor (void);

            void addHandler (ConnHandler *pHandler);
            int connectToPeer (const char *pszRemotePeerAddr, uint16 ui16Port);

            void run (void);

            int changeEncryptionKey (unsigned char *pchKey, uint32 ui32Len);
            int subscribe (Subscription &sub);

            int startAdaptor (void);
            int stopAdaptor (void);

        protected:
            ConnCommAdaptor (AdaptorId uiId, AdaptorType adaptorType, bool bSupportsCaching,
                             const char *pszNodeId, CommAdaptorListener *pListener, uint16 ui16Port);

            virtual int connectToPeerInternal (const char *pszRemotePeerAddr, uint16 ui16Port) = 0;

            /**
             * This method should be called within a mutually exclusive block
             * that is protected by _mHandlers.
             */
            void cleanHandlers (void);

            virtual ConnListener * getConnListener (const char *pszListenAddr, uint16 ui16Port,
                                                    const char *pszNodeId, const char *pszSessionId,
                                                    CommAdaptorListener *pListener,
                                                    ConnCommAdaptor *pCommAdaptor) = 0;

            virtual int init (char **ppIfaces);

            int sendMessage (MessageHeaders::MsgType type,
                             const void *pBuf, uint32 ui32Len,
                             const char *pszPublisherNodeId,
                             const char **ppszRecipientNodeIds,
                             const char **ppszInterfaces,
                             uint8 ui8Priority);

        protected:
            struct DisconnectedPeer
            {
                DisconnectedPeer (void);
                DisconnectedPeer (const char *pszRemotePeerAddr, uint16 ui16Port);
                ~DisconnectedPeer (void);

                DisconnectedPeer & operator = (const DisconnectedPeer &rhsDisconnectedPeer);
                bool operator == (const DisconnectedPeer &rhsDisconnectedPeer);

                uint16 ui16PeerPort;
                NOMADSUtil::String peerAddr;
            };

            typedef NOMADSUtil::StringHashtable<ConnHandler> ConnHandlers;
            typedef NOMADSUtil::StringHashtable<ConnListener> ConnListeners;

        protected:
            const uint16 _ui16Port;
            ConnHandlers _handlersByPeerId;
            ConnListeners _listenersByInterfaceIP;

            NOMADSUtil::LList<DisconnectedPeer> _disconnectedHandlers;
            NOMADSUtil::Mutex _mHandlers;

        private:
            NOMADSUtil::LList<DisconnectedPeer> _handlersToReconnectToByIPAddr;
            NOMADSUtil::Queue<DisconnectedPeer> _disconnectedPeers;
            NOMADSUtil::Mutex _mDisconnectedPeers;

    };
}

#endif    /* INCL_COMM_COMM_ADAPTOR_H */
