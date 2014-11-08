#ifndef _UNSEQUENCEDPACKETQUEUE_H
#define	_UNSEQUENCEDPACKETQUEUE_H

/*
 * UnsequencedPacketQueue.h
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

#include <stddef.h>

#include "Packet.h"
#include "PacketWrapper.h"

#include "ConditionVariable.h"
#include "FTypes.h"
#include "Logger.h"
#include "Mutex.h"
#include "NLFLib.h"

#include <string.h>


class UnsequencedPacketQueue
{
    public:
        UnsequencedPacketQueue (bool isReliable);
        
        ~UnsequencedPacketQueue (void);
        
        // Obtains a lock on the queue
        int lock (void);

        // Releases the lock on the queue
        int unlock (void);

        // Returns a count of the number of packets in the queue
        uint32 getPacketCount (void);

        // Inserts a new unsequenced fragment in the queue. While inserting the new fragment
        // it also tries to build a list (pFragments) of fragments that form a complete message.
        // If this fragment completes a new message the list is returned otherwise null is returned.
        NOMADSUtil::LList<Packet*> * insert (Packet *pPacket);
        
        PacketWrapper * peek (void);
        
        // Removes the specified packet from the queue
        // NOTE: The function only compares the pointer values, so the object must be the
        // exact same one that is contained in the queue
        // NOTE: The memory for the packet is not deallocated by this method
        // NOTE: Even though the remove is typically called for the first packet that was
        //       retrieved using peek(), this method is still written to check the whole
        //       list; the method still checks the first packet so it is still efficient
        //       if the packet happens to be the first one
        int remove (Packet *pPacket);
        
        // Removes expired fragments from the queue:
        // A fragments expires if the queue is made of unreliable packets
        // and the packet has resided in the queue longer than DEFAULT_TIMEOUT_TO_EXPIRE_PACKETS
        int expireOldPackets (void);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

        // If an unreliable fragments reside in the queue more than the timeout time we
        // can remove it assuming the missing fragments has been lost and won't be
        // retransmitted from the sender
        static const int64 DEFAULT_TIMEOUT_TO_EXPIRE_PACKETS = 3000;
        
        private:
        struct Node
        {
            Node (void);
            ~Node (void);
            Node *pPrev;
            Node *pNext;
            Packet *pData;
            int64 i64Timestamp;
        };

        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        Node *_pFirstNode;
        Node *_pLastNode;
        uint32 _ui32PacketsInQueue;
        bool _bIsRealiablePacketQueue;
        
};

inline UnsequencedPacketQueue::UnsequencedPacketQueue (bool isReliable)
    : _cv (&_m)
{
    _pFirstNode = _pLastNode = NULL;
    _ui32PacketsInQueue = 0;
    _bIsRealiablePacketQueue = isReliable;
}

inline UnsequencedPacketQueue::~UnsequencedPacketQueue (void)
{
    while (_pFirstNode != NULL) {
        Node *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode;
    }
    _pFirstNode = _pLastNode = NULL;
    _ui32PacketsInQueue = 0;
}


inline int UnsequencedPacketQueue::lock (void)
{
    return _m.lock();
}

inline int UnsequencedPacketQueue::unlock (void)
{
    return _m.unlock();
}

inline uint32 UnsequencedPacketQueue::getPacketCount (void)
{
    return _ui32PacketsInQueue;
}

inline NOMADSUtil::LList<Packet*> * UnsequencedPacketQueue::insert (Packet *pPacket)
{
    //printf ("UnsequencedPacketQueue::insert packet with sequence number %d\n", pPacket->getSequenceNum());
    _m.lock();
    // If the queue is made of reliable fragments check if this is a duplicate packet or a new one
    if (_bIsRealiablePacketQueue) {
        // Check if this is a duplicate packet
        bool duplicatePacket = false;
        //TODO: use some sort of SACK approach
        
        if (duplicatePacket) {
            _m.unlock();
            //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
            //            "Received duplicate packet with sequence number: %d\n", pPacket->getSequenceNum());
            return NULL;
        }
    }

    //Check if the list is empty to insert the first element
    if (_pFirstNode == NULL) {
        // There are no other elements - just insert at the beginning (which is also the end)
        //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
        //                "There are no other elements - just insert at the beginning (which is also the end) seqNum %d\n", pPacket->getSequenceNum());
        Node *pNewNode = new Node;
        pNewNode->pData = pPacket;
        pNewNode->i64Timestamp = NOMADSUtil::getTimeInMilliseconds();
        _pFirstNode = _pLastNode = pNewNode;
        _ui32PacketsInQueue++;
        _m.unlock();
        return NULL;
    }

    // While inserting the new fragment in the queue we also try to build a list (pFragments)
    // of fragments that create a complete message. This list will be returned by this method if complete.
    // NOTE: fragments of the same packet share the same ui32MessageTSN but this info is in the PacketWrapper so only at the sender side
    NOMADSUtil::LList<Packet*> *pFragments;
    // Find the right spot to insert the new node
    Node *pTempNode = _pFirstNode;
    bool bAlreadyInserted = false;
    int32 i32LastFragSeqNum = -1;
    while (pTempNode != NULL) {
        // Try to complete a packet with this new fragment, keep trying while pTempNode has a
        // sequence num < of the new fragment to be inserted, or the new fragment has been inserted
        // and possibly the rest of the fragments of the packet are already in the queue
        if ((pTempNode->pData->getSequenceNum() < pPacket->getSequenceNum()) || (bAlreadyInserted)) {
            if (pTempNode->pData->isFirstFragment()) {
                //printf ("First fragment seqNum=%d\n", pTempNode->pData->getSequenceNum());
                // First fragment start to build a possible complete packet
                //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
                //        "First fragment start to build a possible complete packet seqNum=%d\n", pTempNode->pData->getSequenceNum());
                i32LastFragSeqNum = pTempNode->pData->getSequenceNum();
                pFragments = new NOMADSUtil::LList<Packet*>;
                pFragments->add(pTempNode->pData);
            }
            else if (i32LastFragSeqNum+1 == pTempNode->pData->getSequenceNum()) {
                // intermediate or last packet: 
                // if the sequence number follows the last saved in the fragments list add this fragment to the list
                if (pTempNode->pData->isIntermediateFragment()) {
                    //printf ("Intermediate fragment seqNum=%d\n", pTempNode->pData->getSequenceNum());
                    //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
                    //    "Intermediate fragment of a possible complete packet seqNum=%d\n", pTempNode->pData->getSequenceNum());
                    i32LastFragSeqNum++;
                    pFragments->add(pTempNode->pData);
                }
                // We have completed a packet
                else if (pTempNode->pData->isLastFragment()) {
                    //printf ("Last fragment seqNum=%d\n", pTempNode->pData->getSequenceNum());
                    //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
                    //    "Last fragment seqNum=%d. We have completed a packet\n", pTempNode->pData->getSequenceNum());
                    pFragments->add(pTempNode->pData);
                    
                    // Remove the packet in pFragments from the queue because they are going to be delivered
                    Packet *pRemovePacket;
                    pFragments->resetGet();
                    while (pFragments->getNext (pRemovePacket)) {
                        this->remove(pRemovePacket);
                    }
                    //TODO: this shoud be done only for unreliable packets, find a smarter way for reliable packets
                    if (!_bIsRealiablePacketQueue) {
                        // Try to expire old packets
                        expireOldPackets();
                    }
                    // Reset
                    i32LastFragSeqNum = -1;
                    _m.unlock();
                    return pFragments;
                }
            }
            // This fragment is not part of the packet we are building 
            else {
                // Reset
                i32LastFragSeqNum = -1;
                pFragments = NULL;
            }
            pTempNode = pTempNode->pNext;
        }
        else {
            //First fragment with sequence number >=  new packet seqNum
            if (pTempNode->pData->getSequenceNum() == pPacket->getSequenceNum()) {
                // duplicate packet
                _m.unlock();
                //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
                //        "This packet seqNum %d is already in the queue. Duplicate: does not insert\n", pPacket->getSequenceNum());
                return NULL; // Signal to the PacketProcessor that this fragment is duplicated
            }
            else {
                // Need to insert before pTempNode
                //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
                //        "Insert packet seqNum %d before packet seqNum %d\n", pPacket->getSequenceNum(), pTempNode->pData->getSequenceNum());
                Node *pNewNode = new Node;
                pNewNode->pData = pPacket;
                pNewNode->i64Timestamp = NOMADSUtil::getTimeInMilliseconds();
                pNewNode->pNext = pTempNode;
                if (pTempNode == _pFirstNode) {
                    _pFirstNode = pNewNode;
                    pTempNode->pPrev = pNewNode;
                }
                else {
                    pNewNode->pPrev = pTempNode->pPrev;
                    pTempNode->pPrev->pNext = pNewNode;
                    pTempNode->pPrev = pNewNode;
                }
                _ui32PacketsInQueue++;
                bAlreadyInserted = true;
                
                // Keep scanning to see if this fragment completes a packet
                pTempNode = pNewNode;
            }
        }
//        pTempNode = pTempNode->pNext;
        if ((pTempNode == NULL) && (!bAlreadyInserted)) {
            // We reached the end of the queue without inserting so the right spot is as last node
            //checkAndLogMsg ("UnsequencedPacketQueue::insert", NOMADSUtil::Logger::L_LowDetailDebug,
            //            "We reached the end of the queue without inserting so the right spot is as last node. SeqNum %d\n", pPacket->getSequenceNum());
            Node *pNewNode = new Node;
            pNewNode->pData = pPacket;
            pNewNode->i64Timestamp = NOMADSUtil::getTimeInMilliseconds();
            pNewNode->pPrev = _pLastNode;
            _pLastNode->pNext = pNewNode;
            _pLastNode = pNewNode;
            _ui32PacketsInQueue++;
            bAlreadyInserted = true;
            // Keep scanning to see if this fragment completes a packet
            pTempNode = _pLastNode;
            //printf ("insert at the end of the queue and keep scanning\n");
        }
    }
    _m.unlock();
    //printf ("***** packets in queue: %d\n", _ui32PacketsInQueue);
    //printf ("insert return null: reached the end of the queue\n");
    return NULL;
}

inline int UnsequencedPacketQueue::remove (Packet *pPacket)
{
    _m.lock();
    if (_pFirstNode == NULL) {
        // List is empty
        _m.unlock();
        return -1;
    }
    if (_pFirstNode->pData == pPacket) {
        // The node to remove is the first node in the list
        _pFirstNode = _pFirstNode->pNext;
        if (_pFirstNode == NULL) {
            // The list is now empty
            _pLastNode = NULL;
        }
        else {
            _pFirstNode->pPrev = NULL;
        }
        _ui32PacketsInQueue--;
        _m.unlock();
        return 0;
    }
    else {
        Node *pTempNode = _pFirstNode->pNext;
        while (pTempNode != NULL) {
            if (pTempNode->pData == pPacket) {
                // Found the node to delete
                pTempNode->pPrev->pNext = pTempNode->pNext;
                if (pTempNode->pNext == NULL) {
                    // Removed the last node in the list
                    _pLastNode = pTempNode->pPrev;
                }
                else {
                    pTempNode->pNext->pPrev = pTempNode->pPrev;
                }
                _ui32PacketsInQueue --;
                _m.unlock();
                return 0;
            }
            pTempNode= pTempNode->pNext;
        }
        // The specified packet was not found
        _m.unlock();
        return -2;
    }
}

inline int UnsequencedPacketQueue::expireOldPackets (void)
{
    _m.lock();
    int removedPackets = 0;
    if (_pFirstNode == NULL) {
        // List is empty
        _m.unlock();
        return removedPackets;
    }
    int64 i64Now = NOMADSUtil::getTimeInMilliseconds();

    Node *pTempNode = _pFirstNode;
    while (pTempNode != NULL) {
        if ((i64Now - pTempNode->i64Timestamp) > DEFAULT_TIMEOUT_TO_EXPIRE_PACKETS) {
            // Remove this fragment from the list because the timeout is expired
            // we can assume we won't receive the missing fragments for this packet
            // given that the flow in unreliable (no fragments are retransmitted)
            //printf ("UnsequencedPacketQueue::expireOldPackets removing expired fragment with sequence number: %d\n", pTempNode->pData->getSequenceNum());
            Node *pNodeToDelete = pTempNode;
            if (pTempNode->pPrev == NULL) {
                // Removing the first node in the list
                _pFirstNode = pTempNode->pNext;
                if (_pFirstNode == NULL) {
                    // The list is now empty
                    _pLastNode = NULL;
                }
                else {
                    _pFirstNode->pPrev = NULL;
                }
            }
            else {
                pTempNode->pPrev->pNext = pTempNode->pNext;
                if (pTempNode->pNext == NULL) {
                    // Removing the last node in the list
                    //printf ("UnsequencedPacketQueue::expireOldPackets removing last node in the queue\n");
                    _pLastNode = pTempNode->pPrev;
                }
                else {
                    pTempNode->pNext->pPrev = pTempNode->pPrev;
                }
            }
            pTempNode = pTempNode->pNext;
            delete pNodeToDelete;
            _ui32PacketsInQueue --;
            removedPackets ++;
        }
        else {
            //printf ("UnsequencedPacketQueue::expireOldPackets fetch next node\n");
            pTempNode = pTempNode->pNext;
        }
    }
    _m.unlock();
    return removedPackets;
}

inline int UnsequencedPacketQueue::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    objectFreezer.putBool (_bIsRealiablePacketQueue);
    objectFreezer.putUInt32 (_ui32PacketsInQueue);

/*    printf ("UnsequencedPacketQueue\n");
    printf ("_bIsRealiablePacketQueue %lu\n", _bIsRealiablePacketQueue);
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/

    // Go through the whole list of nodes
    Node *pCurrNode = _pFirstNode;
    for (uint32 i=0; i<_ui32PacketsInQueue; i++) {
        //printf ("*** %d\n", i);
        pCurrNode->pData->freeze (objectFreezer);
        objectFreezer.putInt64 (pCurrNode->i64Timestamp);
        pCurrNode = pCurrNode->pNext;
    }
    return 0;
}

inline int UnsequencedPacketQueue::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    objectDefroster >> _bIsRealiablePacketQueue;
    objectDefroster >> _ui32PacketsInQueue;

/*    printf ("UnsequencedPacketQueue\n");
    printf ("_bIsRealiablePacketQueue %lu\n", _bIsRealiablePacketQueue);
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/

    // Insert the nodes
    for (uint32 i=0; i<_ui32PacketsInQueue; i++){
        //printf ("*** %d\n", i);
        Node *pNewNode = new Node;
        Packet *pPacket = new Packet (objectDefroster);
        pNewNode->pData = pPacket;
        objectDefroster >> pNewNode->i64Timestamp;
        if (_pFirstNode == NULL) {
            _pFirstNode = _pLastNode = pNewNode;
        }
        else {
            pNewNode->pPrev = _pLastNode;
            _pLastNode->pNext = pNewNode;
            _pLastNode = pNewNode;
        }
    }
    return 0;
}

inline UnsequencedPacketQueue::Node::Node (void)
{
    pPrev = pNext = NULL;
    pData = NULL;
}

inline UnsequencedPacketQueue::Node::~Node (void)
{
    if (pData) {
        delete pData;
        pData = NULL;
    }
}

#endif	/* _UNSEQUENCEDPACKETQUEUE_H */

