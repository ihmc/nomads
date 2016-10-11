/* 
 * NetworkMessageServiceImpl.h
 * This file is part of the IHMC Network Message Service Library
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 15, 2015, 1:59 PM
 */

#ifndef INCL_NETWORK_MESSAGE_SERVICE_IMPLEMENTATION_H
#define	INCL_NETWORK_MESSAGE_SERVICE_IMPLEMENTATION_H

#include "NetworkMessageService.h"

#include "FIFOQueue.h"
#include "ManageableThread.h"
#include "OSThread.h"
#include "PtrLList.h"
#include "RollingBoundedBitmap.h"
#include "StrClass.h"
#include "StringHashtable.h"
#include "StringHashset.h"
#include "UInt32Hashtable.h"

#include "MessageFactory.h"
#include "NetworkInterfaceManager.h"
#include "Reassembler.h"

namespace IHMC_NMS
{
    class Instrumentation;
}

namespace NOMADSUtil
{
    class ConfigManager;

    class NetworkMessageServiceImpl : public NOMADSUtil::ManageableThread, public NetworkInterfaceManagerListener
    {
        public:
            NetworkMessageServiceImpl (PROPAGATION_MODE mode, bool bAsyncDelivery,
                                       uint8 ui8MessageVersion, NetworkInterfaceManager *pNetIntMgr);
            virtual ~NetworkMessageServiceImpl (void);

            int init (ConfigManager *pCfgMgr);
            
            int transmit (TransmissionInfo &trInfo, MessageInfo &msgInfo);

            int broadcastMessage (TransmissionInfo &trInfo, MessageInfo &msgInfo);
            int transmitMessage (TransmissionInfo &trInfo, MessageInfo &msgInfo);
            int transmitReliableMessage (TransmissionInfo &trInfo, MessageInfo &msgInfo);

            // Set the retransmit timeout (in milliseconds) for reliable messages
            int setRetransmissionTimeout (uint32 ui32Timeout);
            int setPrimaryInterface (const char *pszInterfaceAddr);

            int registerHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener);
            int deregisterHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener);

            void run (void);

            PROPAGATION_MODE getPropagationMode (void);
            uint32 getDeliveryQueueSize (void);
            uint8 getNeighborQueueLength (const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);

        private:
            friend class NetworkMessageReceiver;
 
            int ackArrived (NetworkMessage *pNetMsg, const char *pchIncomingInterface);
            int callListeners (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr, int64 i64Timestamp);
            
            //resets to 0 the queue size neighbors that have the flag
            //_bUpdatedSincelastCheck set to false
            void cleanOldNeighborQueueLengths (void);

            int fragmentAndTransmitMessage (TransmissionInfo &trInfo, MessageInfo &msgInfo);
            int initInternal (uint16 ui16Port, StringHashset &ifaces,
                              const char *pszDestAddr, uint8 ui8McastTTL);

            /**
             * NOTE: the NetworkMessageService may need to keep the arrived
             * NetworkMessage, however, for performances reasons, it does NOT
             * make a copy of it.  Therefore the caller should NOT delete the
             * passed NetworkMessageV1.
             * Deletion of the NetworkMessageV1 is up to the NetworkMessageService
             * or its inner data structures.
             */
            int messageArrived (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);
            int messageSent (const NetworkMessage *pNetMsg, const char *pchOutgoingInterface);

            int notifyListeners (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);
            
            int rebroadcastMessage (NetworkMessage *pNetMsg, const char *pchIncomingInterface);
            int resendUnacknowledgedMessages (void);
            int sendNetworkMessage (NetworkMessage *pNetMsg, TransmissionInfo &trInfo, bool bDeallocatedNetMsg=true);
            int sendSAckMessagesNetworkMessage (void);

            /*
             * set the queue length for the neighbor with the specified ip
             * the interface will be guessed using ulSenderRemoteAddr
             * to avoid considering the 0.0.0.0 interface when receiving
             * a broadcast on linux
             */
            void setNeighborQueueLength (unsigned long ulSenderRemoteAddr, uint8 ui8QueueLength);

            //------------------------------------------------------------------
            // Use only in case the received message has been either multicast
            // or broadcast
            //------------------------------------------------------------------
            bool checkOldMessages (uint32 ui32SourceAddress, uint16 ui16SessionId, uint16 ui16MsgId);
            void updateOldMessagesList (uint32 ui32SourceAddress, uint16 ui16SessionId, uint16 ui16MsgId);

            static void deliveryThread (void *pArg);

        private:
            struct UI16Wrapper
            {
                uint16 ui16;
            };

            struct QueuedMessage
            {
                QueuedMessage (void);
                ~QueuedMessage (void);
                NetworkMessage *pMsg;
                String incomingInterface;
                unsigned long ulSenderRemoteAddress;
                int64 i64Timestamp;
            };

            /*
             * the table of the queue lengths of the neighbors
             * is updated every time a message is received.
             * periodically, the NMS will delete the entries
             * that haven't been updated in a while, assuming
             * the corresponding neighbor is dead (or that it's
             * alive but with nothing to send. the result is the
             * same)
             */

            struct ByNeighbor
            {
                uint8 _ui8QueueLength;
                bool _bUpdatedSinceLastCheck;
            };

            struct ByInterface
            {
                UInt32Hashtable<ByNeighbor> _tByNeighbor;
            };

            struct PeerState
            {
                PeerState (void);
                void resetMessageHistory (void);
                void setAsReceived (uint16 ui16MsgSeqId);
                bool checkIfReceived (uint16 ui16MsgseqId);
                uint16 ui16SessionId;
                uint32 ui32ReceivedMessageHistoryBitmap[4];
            };

            struct UnackedMessageWrapper
            {
                UnackedMessageWrapper (NetworkMessage *pNetMsg, bool bDeleteNetMsg=true);
                virtual ~UnackedMessageWrapper (void);

                NetworkMessage *_pNetMsg;
                int64 _i64SendingTime;
                uint32 _ui32RetransmitCount;
                uint32 _ui32TimeOut;
                const char ** _ppszOutgoingInterfaces;
                uint16 _ui16DelayTolerance;
                bool _bDeleteNetMsg;

                bool operator == (UnackedMessageWrapper &rhsUnackedMessageWrapper);
                bool operator > (UnackedMessageWrapper &rhsUnackedMessageWrapper);
                bool operator < (UnackedMessageWrapper &rhsUnackedMessageWrapper);
            };

            typedef PtrLList<UnackedMessageWrapper> UnackedSentMessagesByMsgId;
            typedef UInt32Hashtable<UnackedSentMessagesByMsgId> UnackedSentMessagesByDestination;
            typedef UInt32Hashtable<UI16Wrapper> CumulativeTSNByDestination;
 
            typedef PtrLList<NetworkMessageServiceListener> NMSListerList;

            // Class Variables
            const PROPAGATION_MODE _mode;
            const bool _bAsyncDelivery;
            uint8 _ui8DefMaxNumOfRetransmissions;
            uint32 _ui32RetransmissionTimeout;
            uint16 _ui16BroadcastedMsgCounter;
            uint32 _ui32MaxMsecsInOutgoingQueue;

            MessageFactory _msgFactory;
            Reassembler _reassembler;
            NetworkInterfaceManager *_pNetIntMgr;
            IHMC_NMS::Instrumentation *_pInstr;

            UInt32Hashtable<PeerState> _lastMsgs;

            // Reliable transmission: data structures to handle unacked messages
            UnackedSentMessagesByDestination _unackedSentMessagesByDestination;
            CumulativeTSNByDestination _cumulativeTSNByDestination;

            StringHashtable<ByInterface> _tQueueLengthByInterface;  // contains the queue
                                                                    // lengths of the neighbors

            // Application listeners
            UInt32Hashtable<NMSListerList > _listeners;

            Mutex _m;
            Mutex _mxMessageArrived;
            Mutex _mxUnackedSentMessagesByDestination;
            Mutex _mQueueLengthsTable;

            // Variables and methods for asynchronous delivery
            Mutex _mDeliveryQueue;
            ConditionVariable _cvDeliveryQueue;
            OSThread _ostDeliveryThread;
            FIFOQueue _deliveryQueue;
    };

    inline void NetworkMessageServiceImpl::PeerState::setAsReceived (uint16 ui16MsgSeqId)
    {
        uint16 ui16Bit = ui16MsgSeqId % 128;
        uint16 ui16BitArrayIndex = ui16Bit / 32;
        uint16 ui16BitOffset = ui16MsgSeqId % 32;
        // Reset the uint32 bitmap that is farthest away
        if (ui16BitArrayIndex == 0) {
            ui32ReceivedMessageHistoryBitmap[2] = 0;
        }
        else if (ui16BitArrayIndex == 1) {
            ui32ReceivedMessageHistoryBitmap[3] = 0;
        }
        else if (ui16BitArrayIndex == 2) {
            ui32ReceivedMessageHistoryBitmap[0] = 0;
        }
        else {
            ui32ReceivedMessageHistoryBitmap[1] = 0;
        }
        ui32ReceivedMessageHistoryBitmap[ui16BitArrayIndex] |= (0x00000001UL << ui16BitOffset);
    }

    inline void NetworkMessageServiceImpl::PeerState::resetMessageHistory (void)
    {
        ui32ReceivedMessageHistoryBitmap[0] = 0;
        ui32ReceivedMessageHistoryBitmap[1] = 0;
        ui32ReceivedMessageHistoryBitmap[2] = 0;
        ui32ReceivedMessageHistoryBitmap[3] = 0;
    }

    inline bool NetworkMessageServiceImpl::PeerState::checkIfReceived (uint16 ui16MsgSeqId)
    {
        uint16 ui16Bit = ui16MsgSeqId % 128;
        uint16 ui16BitArrayIndex = ui16Bit / 32;
        uint16 ui16BitOffset = ui16MsgSeqId % 32;
        return ((ui32ReceivedMessageHistoryBitmap[ui16BitArrayIndex] & (0x00000001UL << ui16BitOffset)) != 0);
    }

    inline bool NetworkMessageServiceImpl::UnackedMessageWrapper::operator == (UnackedMessageWrapper &rhsUnackedMessageWrapper)
    {
        return (_pNetMsg->getMsgId() == rhsUnackedMessageWrapper._pNetMsg->getMsgId());
    }

    inline bool NetworkMessageServiceImpl::UnackedMessageWrapper::operator > (UnackedMessageWrapper &rhsUnackedMessageWrapper)
    {
        return ((_i64SendingTime + _ui32TimeOut) > (rhsUnackedMessageWrapper._i64SendingTime + rhsUnackedMessageWrapper._ui32TimeOut));
    }

    inline bool NetworkMessageServiceImpl::UnackedMessageWrapper::operator < (UnackedMessageWrapper &rhsUnackedMessageWrapper)
    {
        return ((_i64SendingTime + _ui32TimeOut) < (rhsUnackedMessageWrapper._i64SendingTime + rhsUnackedMessageWrapper._ui32TimeOut));
    }

    inline NetworkMessageServiceImpl::QueuedMessage::QueuedMessage (void)
    {
        pMsg = NULL;
    }

    inline NetworkMessageServiceImpl::QueuedMessage::~QueuedMessage (void)
    {
    }
}

#endif	/* INCL_NETWORK_MESSAGE_SERVICE_IMPLEMENTATION_H */

