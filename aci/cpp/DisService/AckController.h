/*
 * AckController.h
 *
 *  This file is part of the IHMC DisService Library/Component
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
 * Controller that handles the periodic retransmission
 * of unacknowledged messages.
 */

#ifndef INCL_ACK_CONTROLLER_H
#define INCL_ACK_CONTROLLER_H

#include "Listener.h"
#include "MessageInfo.h"
#include "Services.h"

#include "ManageableThread.h"
#include "PtrLList.h"
#include "PtrQueue.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    template <class T> class PtrLList;
    template <class T> class StringHashtable;
    template <class T> class PtrQueue;
    class String;
}

namespace IHMC_ACI
{
    class DataCacheReplicationController;
    class DisseminationService;
    class MessageHeader;

    class AckController : public NOMADSUtil::ManageableThread, public PeerStateListener,
                          public MessagingService, public MessageListener
    {
        public:
            static const uint32 DEFAULT_RETRANSMISSION_TIMEOUT;
            static const uint32 DEFAULT_RETRANSMISSION_CYCLE_PERIOD;
            static const uint8 DEFAULT_TRANSMISSION_WINDOW;

            /**
             * i64TimeOut = 0 means that the Controller must wait indefinitely for the ack
             */
            AckController (DisseminationService *pDisService, int64 i64TimeOut=DEFAULT_RETRANSMISSION_TIMEOUT,
                           uint8 ui8TransmissionWindow=DEFAULT_TRANSMISSION_WINDOW);
            virtual ~AckController (void);

            void run (void);

            /**
             * Add pszNodeUUID to the AckController. AckController will take care
             * of re-sending the message until it is acknowledged
             */
            void ackRequest (const char *pszNodeUUID, MessageHeader *pMH);

            // Listeners
            void newIncomingMessage (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                     DisServiceMsg *pDisServiceMsg, uint32 ui32SourceIPAddress,
                                     const char *pszIncomingInterface);
            void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                              const char *pszIncomingInterface);
            void deadNeighbor (const char *pszNodeUID);
            void newLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                    const char *pszIncomingInterface);
            void droppedLinkToNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr);
            void stateUpdateForPeer (const char *pszNodeUID, PeerStateUpdateInterface *pUpdate);

        private:
            static const bool REQUIRE_ACKNOWLEDGMENT;

            struct MessageState
            {
                MessageState (int64 i64SendingTime, int64 ui64TimeOut);
                virtual ~MessageState (void);

                bool hasTimedOut (int64 i64Now);

                int64 i64SendingTime;
                int64 i64TimeOut;
                bool bReplicationEnabled;
                uint8 ui8Counter;
            };

            struct MessageByTarget
            {
                MessageByTarget (void);
                virtual ~MessageByTarget (void);

                bool isAlive (void);
                bool hasUnsentMessages (void);
                bool hasUnacknowledgedMessages (void);
                void setAlive (void);
                void setDead (void);

                NOMADSUtil::Mutex mIsAlive;
                NOMADSUtil::PtrLList<MessageHeader> unsentMessages;
                NOMADSUtil::StringHashtable<MessageState> unacknowledgedMessages; // Key is a messageId
                int64 i64LastReplicationTime;

                private:
                    bool bIsAlive;
            };

            void messageRequested (const char *pszMessageAcknowledgedId, const char *pszSenderNodeId);
            void messageAcknowledged (const char *pszMessageAcknowledgedId, const char *pszSenderNodeId);

            /**
             * Returns true if there are unsent or unacknowledged messages for
             * the target.
             * Returns false otherwise.
             */
            bool hasEnquequedMessages (const char *pszSenderNodeId);
            bool hasEnquequedMessages (MessageByTarget *pMBT);

            void replicateTo (MessageByTarget *pMBT, const char *pszTarget);
            void sendUnacknowledgedMessagesTo (MessageByTarget *pMBT, const char *pszTarget);
            void sendUnsentMessagesTo (MessageByTarget *pMBT, const char *pszTarget);

        private:
            NOMADSUtil::StringHashtable<MessageByTarget> _messagesByTarget;
            int64 _i64TimeOut;
            int64 _i64AckCyclePeriod;

            NOMADSUtil::PtrQueue<NOMADSUtil::String> _clientsToServe;

            NOMADSUtil::ConditionVariable _cv;
            NOMADSUtil::Mutex _m;

            uint8 _ui8TransmissionWindow;
            DisseminationService *_pDisService;

            NOMADSUtil::Mutex _mTopologyQueues;
            NOMADSUtil::PtrQueue<NOMADSUtil::String> _newNeighbors;
            NOMADSUtil::PtrQueue<NOMADSUtil::String> _deadNeighbors;
    };

    inline AckController::MessageState::MessageState (int64 i64SendingTime, int64 i64TimeOut)
    {
        this->i64SendingTime = i64SendingTime;
        this->i64TimeOut = i64TimeOut;
        bReplicationEnabled = true;
        ui8Counter = 0;
    }

    inline AckController::MessageState::~MessageState()
    {
    }

    inline bool AckController::MessageState::hasTimedOut (int64 i64Now)
    {
        return (i64SendingTime + i64TimeOut < i64Now);
    }

    inline AckController::MessageByTarget::MessageByTarget()
        : unsentMessages (true),
          unacknowledgedMessages (true, true, true, true)
    {
        i64LastReplicationTime = 0;
        bIsAlive = true;
    }

    inline AckController::MessageByTarget::~MessageByTarget()
    {
        MessageHeader *pMH, *pTmpMH;
        pTmpMH = unsentMessages.getFirst();
        while ((pMH = pTmpMH) != NULL) {
            pTmpMH = unsentMessages.getNext();
            delete pMH;
        }
    }

    inline bool AckController::MessageByTarget::isAlive()
    {
        mIsAlive.lock();
        bool bRet = bIsAlive;
        mIsAlive.unlock();
        return bRet;
    }

    inline void AckController::MessageByTarget::setAlive()
    {
        mIsAlive.lock();
        bIsAlive = true;
        mIsAlive.unlock();
    }

    inline void AckController::MessageByTarget::setDead()
    {
        mIsAlive.lock();
        bIsAlive = false;
        mIsAlive.unlock();
    }
}

#endif // INCL_ACK_CONTROLLER_H
