/*
 * Listener.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 24, 2011, 1:22 AM
 */

#ifndef INCL_LISTENER_H
#define	INCL_LISTENER_H

#include "FTypes.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class DisServiceMsg;
    class MessageHeader;
    class Subscription;

    class Listener
    {
    };

    //--------------------------------------------------------------------------
    // DataCacheListener
    //--------------------------------------------------------------------------

    class DataCacheListener : public Listener
    {
        public:
            virtual ~DataCacheListener (void);

            /**
             * Notifies that the data message (or fragment) in pPayLoad and
             * described by pMH has been cached.
             */
            virtual void dataCacheUpdated (MessageHeader *pMH, const void *pPayLoad) = 0;

            /**
             * Notifies that the data cache limit was hit
             */
            virtual void capacityReached (void) = 0;

            /**
             * Notifies that the safety capacity threshold of the data cache was
             * hit, on the other hand, it is not yet exhausted.
             */
            virtual void thresholdCapacityReached (uint32 ui32Length) = 0;

            /**
             * Notifies the application that in order to store the incoming
             * message described by pIncomingMgsInfo, it is necessary to free
             * the amount of bytes in ui32bytesNeeded.
             */
            virtual void spaceNeeded (uint32 ui32bytesNeeded, MessageHeader *pIncomingMgsInfo,
                                      void *pIncomingData) = 0;

            /**
             * This is periodically invoked to clean the data cache.
             */
            virtual int cacheCleanCycle (void) = 0;

        protected:
            DataCacheListener (void);
    };

    //--------------------------------------------------------------------------
    // MessageListener
    //--------------------------------------------------------------------------

    class MessageListener : public Listener
    {
        public:
            virtual ~MessageListener (void);

            /**
             * Notifies that a new message pDisServiceMsg has been received by
             * the transmission service listener (and it has not yet been
             * handled by DisService).
             * The message was received from the interface in pszIncomingInterface
             * from the node at the address in ui32SourceIPAddress.
             */
            virtual void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                             DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
					                         const char *pszIncomingInterface) = 0;

        protected:
            MessageListener (void);
    };

    //--------------------------------------------------------------------------
    // GroupMembershipListener
    //--------------------------------------------------------------------------

    class GroupMembershipListener : public Listener
    {
        public:
            virtual ~GroupMembershipListener (void);

            /**
             * Notifies that the peer identified by pszPeerNodeId subscribed to
             * a new group. The details of the subscriptions are stored in
             * pSubscription.
             */
            virtual void newSubscriptionForPeer (const char *pszPeerNodeId,
                                                 Subscription *pSubscription) = 0;

            /**
             * Notifies that the peer identified by pszPeerNodeId canceled the
             * subscription to a formerly subscribed group.
             * The details of the subscriptions are stored in pSubscription.
             */
            virtual void removedSubscriptionForPeer (const char *pszPeerNodeId,
                                                     Subscription *pSubscription) = 0;

            /**
             * Notifies that the peer identified by pszPeerNodeId modified the
             * details of the subscription for a previously subscribed group.
             * The new details for the subscriptions are stored in pSubscription.
             */
            virtual void modifiedSubscriptionForPeer (const char *pszPeerNodeId,
                                                      Subscription *pSubscription) = 0;

        protected:
            GroupMembershipListener (void);
    };

    //--------------------------------------------------------------------------
    // NetworkStateListener
    //--------------------------------------------------------------------------

    class NetworkStateListener : public Listener
    {
        public:
            virtual ~NetworkStateListener (void);

            /**
             * Returns the list of quiescent network interfaces.  A network
             * interface is considered quiescent if the traffic seen on that
             * interface is under a certain threshold. The value of the
             * threshold is a property of DisseminationService.
             *
             * NOTE: networkQuiescent() should not deallocate pszInterfaces, since
             *       it may be used by other observers, however the notifier may
             *       deallocate pszInterfaces, thus the listener implementation
             *       must make a copy of them if needed after networkQuiescent()
             *       has returned.
             */
            virtual void networkQuiescent (const char **pszInterfaces) = 0;
            virtual void messageCountUpdate (const char *pszPeerNodeId, const char *pszIncomingInterface,
                                             const char *pszPeerIp,
                                             uint64 ui64GroumMsgCount, uint64 ui64UnicastMsgCount) = 0;

        protected:
            NetworkStateListener (void);
    };

    //--------------------------------------------------------------------------
    // PeerStateListener
    //--------------------------------------------------------------------------

    class PeerStateListener : public Listener
    {
        public:
            virtual ~PeerStateListener (void);

            enum StateUpdate {
                SUBSCRIPTION_STATE,
                DATA_CACHE,
                TOPOLOGY
            };

            struct PeerStateUpdateInterface
            {
                StateUpdate _type;
            };

            template <typename T>
            struct PeerStateUpdate : public PeerStateUpdateInterface
            {
                PeerStateUpdate (StateUpdate type, T oldState, T newState);
                virtual ~PeerStateUpdate (void);

                T _oldState;
                T _newState;
            };

            struct UInt16PeerStateUpdate : public PeerStateUpdate<uint16>
            {
                UInt16PeerStateUpdate (StateUpdate type, uint16 ui16OldState, uint16 ui16NewState);
                virtual ~UInt16PeerStateUpdate (void);
            };

            struct UInt32PeerStateUpdate : public PeerStateUpdate<uint32>
            {
                UInt32PeerStateUpdate (StateUpdate type, uint32 ui32OldState, uint32 ui32NewState);
                virtual ~UInt32PeerStateUpdate (void);
            };

            /**
             * A new neighbor is reachable. pszNodeUID is the identifier that
             * uniquely identifies the node.
             */
            virtual void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                      const char *pszIncomingInterface) = 0;

            /**
             * The neighbor uniquely identified by pszNodeUID is no longer reachable.
             */
            virtual void deadNeighbor (const char *pszNodeUID) = 0;

            virtual void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                            const char *pszIncomingInterface) = 0;
            virtual void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr) = 0;

            virtual void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate) = 0;

        protected:
            PeerStateListener (void);
    };

    //--------------------------------------------------------------------------
    // SearchListener
    //--------------------------------------------------------------------------

    class SearchListener : public Listener
    {
        public:
            virtual ~SearchListener (void);

            virtual void searchArrived (const char *pszQueryId, const char *pszGroupName,
                                        const char *pszQuerier, const char *pszQueryType,
                                        const char *pszQueryQualifiers,
                                        const void *pszQuery, unsigned int uiQueryLen) = 0;

            virtual void searchReplyArrived (const char *pszQueryId, const char **ppszMatchingMessageIds,
                                             const char *pszMatchingNodeId) = 0;
            virtual void volatileSearchReplyArrived (const char *pszQueryId, const void *pReply,
                                                     uint16 ui162ReplyLen, const char *pszMatchingNodeId) = 0;

        protected:
            explicit SearchListener (const char *pszDescription);

        public:
            const NOMADSUtil::String _description;
    };

    template <typename T> PeerStateListener::PeerStateUpdate<T>::PeerStateUpdate (PeerStateListener::StateUpdate type,
                                                                                  T oldState, T newState)
    {
        _oldState = oldState;
        _newState = newState;
        _type = type;
    }

    template <typename T> PeerStateListener::PeerStateUpdate<T>::~PeerStateUpdate()
    {
    }
}

#endif	// INCL_LISTENER_H

