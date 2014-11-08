/*
 * ListenerNotifier.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * Created on May 24, 2011, 2:20 AM
 */

#ifndef INCL_LISTENER_NOTIFIER_H
#define	INCL_LISTENER_NOTIFIER_H

#include "Listener.h"

#include "DArray2.h"
#include "LoggingMutex.h"

namespace NOMADSUtil
{
    template <class T> class DArray2;
}

namespace IHMC_ACI
{
    class DisServiceMsg;
    class MessageHeader;
    class Subscription;

    class Notifier
    {
        public:
            static const unsigned int ID_UNSET = 0xFFFFFFFFUL;
    };

    template <class T>
    class ListenerNotifier : public Notifier
    {
        public:
            virtual ~ListenerNotifier (void);

            int deregisterAllListeners (void);

            /**
             * - iIndex: the index returned by registerListener()
             */
            int deregisterListener (unsigned int uiIndex);

            /**
             * Returns the number of listeners that are currently registered
             */
            unsigned int getListenerCount (void);

            /**
             * Returns a unique index for the listener in case of success.
             * Returns a negative number otherwise.
             */
            int registerListener (T *pListener);

        protected:
            ListenerNotifier (void);

            struct ListenerWrapper {
                ListenerWrapper (void);
                virtual ~ListenerWrapper (void);

                T *pListener;
            };
            NOMADSUtil::DArray2<ListenerWrapper> _listeners;
            NOMADSUtil::LoggingMutex _m;

        private:
            unsigned int _uiCount;
    };

    class DataCacheListenerNotifier : public ListenerNotifier<DataCacheListener>, public DataCacheListener
    {
        public:
            DataCacheListenerNotifier (void);
            virtual ~DataCacheListenerNotifier (void);

            void dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad);
            // Does not notify uiListenerID
            void dataCacheUpdatedNoNotify (MessageHeader *pMH, const void *pPayLoad,
                                           unsigned int uiListenerID);
            void capacityReached (void);
            void thresholdCapacityReached (uint32 ui32Length);
            void spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo, void *pIncomingData);
            int cacheCleanCycle (void);
    };

    class MessageListenerNotifier : public ListenerNotifier<MessageListener>, public MessageListener
    {
        public:
            MessageListenerNotifier (void);
            virtual ~MessageListenerNotifier (void);

            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                     DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                     const char *pszIncomingInterface);
    };

    class GroupMembershipListenerNotifier : public ListenerNotifier<GroupMembershipListener>, public GroupMembershipListener
    {
        public:
            GroupMembershipListenerNotifier (void);
            virtual ~GroupMembershipListenerNotifier (void);

            void newSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);
            void removedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);
            void modifiedSubscriptionForPeer (const char *pszPeerNodeId, Subscription *pSubscription);
    };

    class NetworkStateListenerNotifier : public ListenerNotifier<NetworkStateListener>, public NetworkStateListener
    {
        public:
            NetworkStateListenerNotifier (void);
            virtual ~NetworkStateListenerNotifier (void);

            void networkQuiescent (const char **ppszInterfaces);
    };

    class PeerStateListenerNotifier : public ListenerNotifier<PeerStateListener>, public PeerStateListener
    {
        public:
            PeerStateListenerNotifier (void);
            virtual ~PeerStateListenerNotifier (void);

            void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUID);
            
            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);

            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);
    };

    class SearchNotifier : public ListenerNotifier<SearchListener>, public SearchListener
    {
        public:
            SearchNotifier (void);
            virtual ~SearchNotifier (void);

            void searchArrived (const char *pszQueryId, const char *pszGroupName,
                                const char *pszQuerier, const char *pszQueryType,
                                const char *pszQueryQualifiers,
                                const void *pszQuery, unsigned int uiQueryLen);
            void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds,
                                     const char *pszMatchingNodeId);
    };

    template <class T> ListenerNotifier<T>::ListenerNotifier()
        : _m (19)
    {
        _uiCount = 0;
    }

    template <class T> ListenerNotifier<T>::~ListenerNotifier()
    {
    }

    template <class T> int ListenerNotifier<T>::deregisterAllListeners()
    {
        for (unsigned int i = 0; i < _listeners.size(); i++) {
            if (0 != _listeners.used (i)) {
                _listeners.clear (i);
            }
        }
        return 0;
    }

    template <class T> int ListenerNotifier<T>::deregisterListener (unsigned int uiIndex)
    {
        if (uiIndex >= ID_UNSET) {
            return -1;
        }
        if (uiIndex >= _listeners.size()) {
            return -2;
        }
        if (!_listeners.used (uiIndex)) {
            return 0;
        }
        _uiCount--;
        return _listeners.clear (uiIndex);
    }

    template <class T> unsigned int ListenerNotifier<T>::getListenerCount()
    {
        return _uiCount;
    }

    template <class T> int ListenerNotifier<T>::registerListener (T *pListener)
    {
        int i = _listeners.firstFree();
        if (i < 0) {
            return -1;
        }
        if (((unsigned int)i) == ID_UNSET) {
            return -2;
        }

        _listeners[i].pListener = pListener;
        _uiCount++;
        return i;
    }

    template <class T> ListenerNotifier<T>::ListenerWrapper::ListenerWrapper()
    {
        pListener = NULL;
    }

    template <class T> ListenerNotifier<T>::ListenerWrapper::~ListenerWrapper()
    {
        pListener = NULL;
    }
}

#endif	// INCL_LISTENER_NOTIFIER_H

