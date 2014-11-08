#ifndef INCL_PACKET_PROCESSOR_H
#define INCL_PACKET_PROCESSOR_H

/*
 * PacketProcessor.h
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "PacketQueue.h"

#include "ConditionVariable.h"
#include "LList.h"
#include "Mutex.h"
#include "Thread.h"

#include <stdarg.h>


class Mocket;
class Packet;
class Receiver;
class SequencedPacketQueue;
class UnsequencedPacketQueue;
class ReceivedTSNRangeHandler;

class PacketProcessor : public NOMADSUtil::Thread
{
    public:
        PacketProcessor (Mocket *pMocket);
        ~PacketProcessor (void);

        // Called by the Mocket after the Receiver and Transmitter have been 
        // constructed also - caches references to the receiver and other data structures
        int init (void);
        
        //Called by the Mocket in the restoreState method
        // Same function as init ()
        int reinitAfterDefrost (void);

        // Called by the Receiver after a new packet has been enqueued into one of the three
        // sequenced packet queues
        void packetArrived (void);

        // Called by the Receiver to process a reliable, sequenced packet
        int processReliableUnsequencedPacket (Packet *pPacket);

        // Called by the Receiver to process an unreliable, sequenced packet
        // without using a receive buffer
        // If buffering is going to be used to try to reorder packets, then the packets
        // are enqueued into the unreliable sequenced packet queue and packetArrived() is invoked
        int processUnreliableSequencedPacketWithoutBuffering (Packet *pPacket);

        // Called by the receiver to process an unreliable unsequenced packet
        int processUnreliableUnsequencedPacket (Packet *pPacket);

        // Returns the size of the next message that is ready to be delivered to the application,
        //     0 in case of the connection being closed, and -1 in case no data is available within the specified timeout
        // If no message is available, the call will block based on the timeout parameter
        // Not specifiying a timeout or a timeout of 0 implies that the default timeout should be used
        //     whereas a timeout of -1 implies wait indefinitely
        int getNextMessageSize (int64 i64Timeout);

        // Returns the cumulative size of all messages that are ready to be delivered to the application
        //     0 in the case of no messages being available
        // NOTE: This method does not provide an indication that the connection has been closed
        uint32 getCumulativeSizeOfAvailableMessages (void);

        // Retrieves the data from next message that is ready to be delivered to the application
        // At most ui32BufSize bytes are copied into the specified buffer
        // A timeout of 0 implies that the default timeout should be used whereas a timeout of -1 implies wait indefinitely
        // NOTE: Any additional data in the packet that will not fit in the buffer is discarded
        // Returns the number of bytes that were copied into the buffer, 0 in case of the connection
        //     being closed, and -1 in case no data is available within the specified timeout
        int receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout);

        // Scatter read version of receive
        int sreceive (int64 i64Timeout, void *pBuf1, uint32 ui32BufSize1, va_list valist);

        void run (void);

    private:
        friend class DataBuffer;
        void dequeuedPacket (Packet *pPacket);

    private:
        friend class Mocket;
        bool tryToProcessFirstPacketFromControlQueue (void);
        bool tryToProcessFirstPacketFromReliableSequencedQueue (void);
        bool tryToProcessFirstPacketFromUnreliableSequencedQueue (void);

        int processPacket (Packet *pPacket);

        // Returns true if the next available unreliable packet can be delivered while skipping over the
        // missing ones because the timeout has expired
        bool checkForUnreliablePacketTimeout (uint32 ui32NextTSN, uint32 ui32PacketTSN, int64 i64TimeReceived);

        // Returns true if the delivery prerequisites (if any) specified for the packet have been satisfied
        bool deliveryPrerequisitesSatisfied (Packet *pPacket);

        int deliverFragments (NOMADSUtil::LList<Packet*> *pFragmentList);

        // Returns a count of the number of fragments deleted
        uint16 deleteFragments (NOMADSUtil::LList<Packet*> *pFragmentList);

        void updateMessageStatistics (bool bReliable, bool bSequenced, uint16 ui16Tag);
        
        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);
        
        int freezeLinkedList (NOMADSUtil::LList<Packet*> * pSequencedFragments, NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrostLinkedList (NOMADSUtil::LList<Packet*> * pSequencedFragments, NOMADSUtil::ObjectDefroster &objectDefroster);

    private:
        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        NOMADSUtil::Mutex _mReceive;

        Mocket *_pMocket;
        Receiver *_pReceiver;
        SequencedPacketQueue *_pControlPacketQueue;
        SequencedPacketQueue *_pReliableSequencedPacketQueue;
        SequencedPacketQueue *_pUnreliableSequencedPacketQueue;
        UnsequencedPacketQueue *_pReliableUnsequencedPacketQueue;
        ReceivedTSNRangeHandler *_pReliableUnsequencedPacketTracker;
        UnsequencedPacketQueue *_pUnreliableUnsequencedPacketQueue;
        PacketQueue _receivedDataQueue;

        uint32 _ui32NextControlPacketTSN;
        uint32 _ui32NextReliableSequencedPacketTSN;
        uint32 _ui32NextUnreliableSequencedPacketTSN;

        NOMADSUtil::LList<Packet*> *_pReliableSequencedFragments;
        NOMADSUtil::LList<Packet*> *_pUnreliableSequencedFragments;
        NOMADSUtil::LList<Packet*> *_pReliableUnsequencedFragments;
        NOMADSUtil::LList<Packet*> *_pUnreliableUnsequencedFragments;
};

#endif   // #ifndef INCL_PACKET_PROCESSOR_H
