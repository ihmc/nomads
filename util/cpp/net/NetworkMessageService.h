/*
 * NetworkMessageService.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 */

#ifndef INCL_NETWORK_MESSAGE_SERVICE_H
#define INCL_NETWORK_MESSAGE_SERVICE_H

#include "NetworkInterface.h"
#include "NetworkMessage.h"

#include "FIFOQueue.h"
#include "FTypes.h"
#include "ManageableThread.h"
#include "OSThread.h"
#include "PtrLList.h"
#include "RollingBoundedBitmap.h"
#include "StrClass.h"
#include "UInt32Hashtable.h"
#include "StringHashtable.h"
#include "StringHashset.h"

#define NMS_BROADCAST_ADDRESS "255.255.255.255"
#define NMS_MULTICAST_ADDRESS "239.0.0.239"

namespace NOMADSUtil
{
    class MessageFactory;
    class NetworkMessageServiceListener;
    class NICInfo;
    class Reassembler;

    class NetworkMessageService : public NOMADSUtil::ManageableThread
    {
        public:
            enum PROPAGATION_MODE {
                BROADCAST = 0x00,
                MULTICAST = 0x01
            };
            NetworkMessageService (PROPAGATION_MODE mode=MULTICAST, bool bAsyncDelivery = false,
                                   bool bAsyncTransmission = false, uint8 ui8MessageVersion = 1,
                                   bool bReplyViaUnicast = false);
            virtual ~NetworkMessageService (void);

            static const int NO_TARGET = 0;
            static const uint8 NMS_CTRL_MSG = 0x00;

            /**
             * if PROPAGATION_MODE == BROADCAST this will set the pszDestAddr to
             * NMS_BROADCAST_ADDRESS
             *
             * if PROPAGATION_MODE == MULTICAST this will set the pszDestAddr to
             * NMS_MULTICAST_ADDRESS
             */
            virtual int init (uint16 ui16Port);
            virtual int init (uint16 ui16Port, const char *pszDestAddr, uint8 ui8McastTTL = DEFAULT_MCAST_TTL);
            virtual int init (uint16 ui16Port, const char **ppszBindingInterfaces, const char **ppszIgnoredInterfaces);
            virtual int init (uint16 ui16Port, const char **ppszBindingInterfaces,
                              const char **ppszIgnoredInterfaces, const char *pszDestAddr,
                              uint8 ui8McastTTL = DEFAULT_MCAST_TTL);

            virtual int init (uint16 ui16Port, const char **ppszBindingInterfaces,
                              const char **ppszIgnoredInterfaces, const char **ppszAddedInterfaces,
                              const char *pszDestAddr,
                              uint8 ui8McastTTL = DEFAULT_MCAST_TTL);

            // Set the retransmit timeout (in milliseconds) for reliable messages
            int setRetransmissionTimeout (uint32 ui32Timeout);
            int setPrimaryInterface (const char *pszInterfaceAddr);

            bool isLocalAddress (uint32 ui32Addr);

            // Override start() method from ManageableThread
            virtual int start (void);
            virtual int stop (void);

            virtual int registerHandlerCallback (uint8 ui8MsgType, NetworkMessageServiceListener *pListener);

            /**
             * Broadcast a message
             * Returns 0 if successful or a negative value in case of error
             *
             * ui8MsgType - an application-defined identifier that is used to demultiplex multiple
             *              applications and message types - same as the key used when registering
             *              the listener and the key that will be given to the handler when a message arrives
             *
             * pOutgoingInterfaces - a list of interfaces that should be used to broadcast the message.
             *                       Can be NULL to indicate that the message propagation service should
             *                       make a decision or use a default.
             *
             * ui32BroadcastAddress - the address that should be used to broadcast the message
             *
             * ui16MsgId - a unique identifier for this message.
             *             Using 0 will cause the message propagation service to generate a new id.
             *
             * ui8HopCount - the hop count for this message - will be incremented before the handler on
             *               the receiving node is invoked.
             *
             * ui8TTL - the Time To Live (in terms of the number of hops) for this message
             *          NOTE: if the ui8HopCount <= ui8TTL, the message will NOT be transmitted
             *
             * ui16DelayTolerance - the maximum length of time, in milliseconds, that the message propagation
             *                      service might wait before sending the message. Used to allow message
             *                      aggregation by the message propagation service.
             *
             * pMsgMetaData - metadata for this message - as required by the component using the MessagePropagationService.
             *                Some potential data includes the path and link characteristics that the message has
             *                traversed to arrive at this node. The metadata may be modified as necessary (appended to,
             *                truncated, etc.) by the listener before forwarding the message on to the next hop.
             *                NOTE: The size of the metadata is limited to 65535 bytes, given that the size is
             *                specified using a 16-bit unsigned number.
             *
             * ui16MsgMetaDataLen - the size of the message metadata in bytes
             *
             * pMsg - the message itself
             *
             * ui16MsgLen - length of the message
             *
             * pszHints - optional hints that might be used to modify the behavior of the broadcast
             *
             */
            virtual int broadcastMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces,
                                          uint32 ui32BroadcastAddress, uint16 ui16MsgId,
                                          uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                          const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                          const void *pMsg, uint16 ui16MsgLen, bool bExpedited,
                                          const char *pszHints = NULL);

            /*
             * TODO: implement this
             */
            virtual int transmitMessage (uint8 ui8MsgType,
                                         const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                         uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                         uint16 ui16DelayTolerance,
                                         const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                         const void *pMsg, uint16 ui16MsgLen,
                                         const char *pszHints = NULL);

            /*
             * Sends a message reliably, using fragmentation if necessary
             */
            virtual int transmitReliableMessage (uint8 ui8MsgType,
                                                 const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                                 uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                                 uint16 ui16DelayTolerance,
                                                 const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                                 const void *pMsg, uint16 ui16MsgLen,
                                                 const char *pszHints = NULL);

            void run (void);

            /**
             * Get the MTU of the non-receive-only interface with minimum value
             * of MTU
             */
            uint16 getMinMTU (void);

            /**
             * Get the address of the network interfaces being used by the
             * Network Message Service.
             *
             * NOTE: the returned array and its elements MUST be deallocated by
             * the caller.
             */
            char ** getActiveNICsInfoAsString (void);
            char ** getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination);
            char ** getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr);

            /**
             * Return the propagation mode NetworkMessageService is running in.
             */
            PROPAGATION_MODE getPropagationMode (void);

            /**
             * Returns the size of the delivery queue if the asynchronous delivery
             * is enabled, it always returns 0 otherwise.
             */
            uint32 getDeliveryQueueSize (void);

            /**
             * in Bps.
             * Returns a negative number if the interface was not found
             */
            int64 getReceiveRate (const char *pszAddr);

            /**
             * Get the size of the transmission queue for the specified interface
             * Only applies if the interface had asynchronous transmission enabled
             */
            uint32 getTransmissionQueueSize (const char *pszOutgoingInterface);

            /**
             * Get the size of the transmission queue for the specified interface
             * rescaled to fit in the interval [0, 255]
             * Only applies if the interface had asynchronous transmission enabled
             */
            uint8 getRescaledTransmissionQueueSize (const char *pszOutgoingInterface);

            /**
             * get the max size of the queue
             */
            uint32 getTransmissionQueueMaxSize (const char *pszOutgoingInterface);

            uint32 getTransmitRateLimit (const char *pszInterface);

            /**
             * set the max size of the transmission queue
             */
            int setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize);

            /**
             * Set the transmit rate limit for this socket
             * The target address, if specified, sets this limit only when sending data
             *     to the specified target. A value of 0 implies that it should be applied to
             *     all outgoing traffic.
             * The rate limit is specified in bytes per second
             * A value of 0 turns off the transmit rate limit
             * This is a pass-through to the underlying socket - which may or may
             * not enforce the transmit rate limit.
             *
             * Returns 0 if successful or a negative value in case of error.
             *
             * If the interface it is not specified, the rate limit is applied
             * to all the available interfaces used by the Network Message Service
             */
            int setTransmitRateLimit (const char *pszInterface, const char * pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszInterface, uint32 ui32DestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (uint32 ui32RateLimit);

            /*
             * Get the capacity of the link used by the specified interface
             */
            uint32 getLinkCapacity (const char *pszInterface);

            /*
             * Set the capacity of the link used by the specified interface
             */
            void setLinkCapacity (const char *pszInterface, uint32 ui32Capacity);

            uint8 getNeighborQueueLength (const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);

            // Checks whether it is clear to send on the specified interface
            bool clearToSend (const char *pszInterface);

            // Checks whether it is clear to send on all outgoing interfaces
            bool clearToSendOnAllInterfaces (void);

        public:
            static const unsigned int DEFAULT_PORT = 6666;
            static const unsigned int DEFAULT_MTU = 1400;
            static const unsigned int DEFAULT_TIME_OUT = 10000;
            static const unsigned int DEFAULT_MCAST_TTL = 1;

            static const unsigned int EMPTY_RECIPIENT = 0;
            static const uint32 DEFAULT_RETRANSMISSION_TIME = 5000;
            static const uint32 DEFAULT_MAX_NUMBER_OF_RETRANSMISSIONS = 0;  // if DEFAULT_MAX_NUMBER_OF_RETRANSMISSIONS
                                                                            // is set to 0, then NetworMessageService
                                                                            // keeps retransmitting until the message is
                                                                            // acked (potentially forever :-~)
            static const uint8 K = 1;

        private:
            friend class NetworkMessageReceiver;

            // Check whether the address for any interfaces using proxy datagram sockets have been obtained
            // and if so, moves them into the regular set of interfaces
            int resolveProxyDatagramSocketAddresses (void);
 
            int ackArrived (NetworkMessage *pNetMsg, const char *pchIncomingInterface);
            int callListeners (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);
            
            //resets to 0 the queue size neighbors that have the flag
            //_bUpdatedSincelastCheck set to false
            void cleanOldNeighborQueueLengths (void);

            int fragmentAndTransmitMessage (uint8 ui8MsgType, const char **ppszOutgoingInterfaces, uint32 ui32TargetAddress,
                                            uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, uint16 ui16DelayTolerance,
                                            const void *pMsgMetaData, uint16 ui16MsgMetaDataLen, const void *pMsg, uint16 ui16MsgLen,
                                            bool bReliable, bool bExpedited, const char *pszHints = NULL);
            int initInternal (uint16 ui16Port, const char **ppszBindingInterfaces,
                              const char *pszDestAddr, uint8 ui8McastTTL);
            bool isUnicast (uint32 ui32Address, const char *pszIncomingInterface);

            /**
             * NOTE: the NetworkMessageService may need to keep the arrived
             * NetworkMessage, however, for performances reasons, it does NOT
             * make a copy of it.  Therefore the caller should NOT delete the
             * passed NetworkMessageV1.
             * Deletion of the NetworkMessageV1 is up to the NetworkMessageService
             * or its inner data structures.
             */
            int messageArrived (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);

            int notifyListeners (NetworkMessage *pNetMsg, const char *pchIncomingInterface, unsigned long ulSenderRemoteAddr);
            
            int rebroadcastMessage (NetworkMessage *pNetMsg, const char *pchIncomingInterface);
            int resendUnacknowledgedMessages (void);
            int sendNetworkMessage (NetworkMessage *pNetMsg, const char **ppszOutgoingInterfaces,
                                    uint32 ui32Address, uint16 ui16DelayTolerance,
                                    bool bReliable, bool bExpedited=false, bool bDeallocatedNetMsg=true,
                                    const char *pszHints = NULL);
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
            uint16 _ui16BroadcastedMsgCounter;
            bool checkOldMessages (uint32 ui32SourceAddress, uint16 ui16SessionId, uint16 ui16MsgId);
            void updateOldMessagesList (uint32 ui32SourceAddress, uint16 ui16SessionId, uint16 ui16MsgId);

        private:
            struct PeerState
            {
                PeerState (void);
                void resetMessageHistory (void);
                void setAsReceived (uint16 ui16MsgSeqId);
                bool checkIfReceived (uint16 ui16MsgseqId);
                uint16 ui16SessionId;
                uint32 ui32ReceivedMessageHistoryBitmap[4];
            };
            UInt32Hashtable<PeerState> _lastMsgs;

            //------------------------------------------------------------------
            // Reliable transmission: data structures to handle unacked messages
            //------------------------------------------------------------------
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
            struct UI16Wrapper
            {
                uint16 ui16;
            };
            typedef UInt32Hashtable<UI16Wrapper> CumulativeTSNByDestination;
            UnackedSentMessagesByDestination _unackedSentMessagesByDestination;
            CumulativeTSNByDestination _cumulativeTSNByDestination;
            uint32 _ui32RetransmissionTimeout;
            uint8 _ui8DefMaxNumOfRetransmissions;

            // Application listeners
            typedef PtrLList<NetworkMessageServiceListener> NMSListerList;
            UInt32Hashtable<NMSListerList > _listeners;

            // Network interfaces
            StringHashtable<NetworkInterface> _interfaces;
            Mutex _mProxyInterfacesToResolve;
            PtrLList<NetworkInterface> _proxyInterfacesToResolve;    // List of interfaces using the ProxyDatagramSocket that need to be resolved

            // It Keeps track of the primary interface set from the DisService configuration file
            uint32 _ui32PrimaryInterface;
            bool _bPrimaryInterfaceIdSet;

            MessageFactory *_pMessageFactory;
            Reassembler *_pReassembler;

            PROPAGATION_MODE _mode;

            // Variable for asynchronous transmission
            bool _bAsyncTransmission;

            // Variables and methods for asynchronous delivery
            bool _bAsyncDelivery;
            struct QueuedMessage
            {
                QueuedMessage (void);
                NetworkMessage *pMsg;
                String incomingInterface;
                unsigned long ulSenderRemoteAddress;
            };
            OSThread _ostDeliveryThread;
            Mutex _mDeliveryQueue;
            ConditionVariable _cvDeliveryQueue;
            FIFOQueue _deliveryQueue;
            static void deliveryThread (void *pArg);

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
            //the hashtable containing the queue lengths of the neighbours
            StringHashtable<ByInterface> _tQueueLengthByInterface;
            //the max amount of time a message can spend in the transmission queue
            //0 = disabled
            uint32 _ui32MaxMsecsInOutgoingQueue;

            uint16 _ui16MTU;
            const bool _bReplyViaUnicast;
            Mutex _m;
            Mutex _mxMessageArrived;
            Mutex _mxUnackedSentMessagesByDestination;
            Mutex _mQueueLengthsTable;
    };

    inline int NetworkMessageService::setRetransmissionTimeout (uint32 ui32Timeout)
    {
        _ui32RetransmissionTimeout = ui32Timeout;
        return 0;
    }

    inline NetworkMessageService::PeerState::PeerState (void)
    {
        ui16SessionId = 0;
        resetMessageHistory();
    }

    inline void NetworkMessageService::PeerState::resetMessageHistory (void)
    {
        ui32ReceivedMessageHistoryBitmap[0] = 0;
        ui32ReceivedMessageHistoryBitmap[1] = 0;
        ui32ReceivedMessageHistoryBitmap[2] = 0;
        ui32ReceivedMessageHistoryBitmap[3] = 0;
    }

    inline void NetworkMessageService::PeerState::setAsReceived (uint16 ui16MsgSeqId)
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

    inline bool NetworkMessageService::PeerState::checkIfReceived (uint16 ui16MsgSeqId)
    {
        uint16 ui16Bit = ui16MsgSeqId % 128;
        uint16 ui16BitArrayIndex = ui16Bit / 32;
        uint16 ui16BitOffset = ui16MsgSeqId % 32;
        return ((ui32ReceivedMessageHistoryBitmap[ui16BitArrayIndex] & (0x00000001UL << ui16BitOffset)) != 0);
    }

    inline NetworkMessageService::UnackedMessageWrapper::UnackedMessageWrapper (NetworkMessage *pNetMsg, bool bDeleteNetMsg)
    {
        _pNetMsg = pNetMsg;
        _ui32RetransmitCount = 0;
        _bDeleteNetMsg = bDeleteNetMsg;
        _ppszOutgoingInterfaces = NULL;
    }

    inline NetworkMessageService::UnackedMessageWrapper::~UnackedMessageWrapper ()
    {
        if (_ppszOutgoingInterfaces != NULL) {
            char ** ppszOutgoingInterfaces = (char **) _ppszOutgoingInterfaces;
            for (uint8 i = 0; ppszOutgoingInterfaces[i] != NULL; i++) {
                free (ppszOutgoingInterfaces[i]);
            }
            free (ppszOutgoingInterfaces);
            _ppszOutgoingInterfaces = NULL;
        }
        if (_bDeleteNetMsg) {
            delete _pNetMsg;
            _pNetMsg = NULL;
        }
    }

    inline bool NetworkMessageService::UnackedMessageWrapper::operator == (UnackedMessageWrapper &rhsUnackedMessageWrapper)
    {
        return (_pNetMsg->getMsgId() == rhsUnackedMessageWrapper._pNetMsg->getMsgId());
    }

    inline bool NetworkMessageService::UnackedMessageWrapper::operator > (UnackedMessageWrapper &rhsUnackedMessageWrapper)
    {
        return ((_i64SendingTime + _ui32TimeOut) > (rhsUnackedMessageWrapper._i64SendingTime + rhsUnackedMessageWrapper._ui32TimeOut));
    }

    inline bool NetworkMessageService::UnackedMessageWrapper::operator < (UnackedMessageWrapper &rhsUnackedMessageWrapper)
    {
        return ((_i64SendingTime + _ui32TimeOut) < (rhsUnackedMessageWrapper._i64SendingTime + rhsUnackedMessageWrapper._ui32TimeOut));
    }

    inline NetworkMessageService::QueuedMessage::QueuedMessage (void)
    {
        pMsg = NULL;
    }

}

#endif   // #ifndef INCL_NETWORK_MESSAGE_SERVICE_H
