#ifndef INCL_PENDING_PACKET_QUEUE_H
#define INCL_PENDING_PACKET_QUEUE_H

/*
 * PendingPacketQueue.h
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

#include "Packet.h"
#include "PacketWrapper.h"

#include "ConditionVariable.h"
#include "FTypes.h"
#include "Mutex.h"
#include "NLFLib.h"


class PendingPacketQueue
{
    public:
        PendingPacketQueue (uint32 ui32MaximumSize, bool bEnableCrossSequencing);

        // Deletes any enqueued wrappers and packets
        ~PendingPacketQueue (void);

        // Obtains a lock on the queue
        int lock (void);

        // Releases the lock on the queue
        int unlock (void);

        bool isEmpty (void);

        uint32 getBytesInQueue (void);

        uint32 getPacketsInQueue (void);

        // Returns the number of bytes that can be enqueued without blocking
        uint32 getSpaceAvailable (void);

        // Inserts the specified wrapper into the queue if there is room available
        // If there is no room available, the method blocks until there is room or the specified timeout expires
        // A zero or negative timeout value implies wait indefinitely
        bool insert (PacketWrapper *pWrapper, int64 i64Timeout);

        PacketWrapper * peek (void);

        // Removes the specified packet from the queue
        // NOTE: The function only compares the pointer values, so the object must be the
        //       exact same one that is contained in the queue
        // NOTE: The memory for the packet is not deallocated by this method
        // NOTE: Even though the remove is typically called for the first packet that was
        //       retrieved using peek(), this method is still written to check the whole
        //       list just in case another (higher priority) packet is inserted at the
        //       head of the queue; the method still checks the first packet so it is still
        //       efficient if the packet happens to be the first one
        int remove (PacketWrapper *pWrapper);

        // Deletes any packets that match the specified flow and contain the specified tag
        // Returns the number of packets deleted or a negative value in case of error
        // NOTE: The packets are deleted (i.e., the memory is deallocated)
        int cancel (bool bReliable, bool bSequenced, uint16 ui16Tag, uint8 * pui8HigherPriority = nullptr);

        // Indicates that no additional data will be extracted from this queue
        // Any threads blocked in insert() will be woken up and any future calls to insert() will fail
        void close (void);

    private:
        struct Node
        {
            Node (void);
            Node *pPrev;
            Node *pNext;
            PacketWrapper *pData;
        };

    private:
        friend class Transmitter;
        bool lowerOrSamePriority (PacketWrapper *pLHSWrapper, PacketWrapper *pRHSWrapper);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    private:
        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        Node *_pFirstNode;
        Node *_pLastNode;
        uint32 _ui32PacketsInQueue;
        uint32 _ui32BytesInQueue;
        uint32 _ui32MaximumSize;
        bool _bCrossSequencing;
        bool _bClosed;
};

inline PendingPacketQueue::PendingPacketQueue (uint32 ui32MaximumSize, bool bEnableCrossSequencing)
    : _cv (&_m)
{
    _pFirstNode = _pLastNode = nullptr;
    _ui32PacketsInQueue = 0;
    _ui32BytesInQueue = 0;
    _ui32MaximumSize = ui32MaximumSize;
    _bCrossSequencing = bEnableCrossSequencing;
    _bClosed = false;
}

inline PendingPacketQueue::~PendingPacketQueue (void)
{
    while (_pFirstNode != nullptr) {
        Node *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode->pData->getPacket();
        delete pTempNode->pData;
        delete pTempNode;
    }
    _pFirstNode = _pLastNode = nullptr;
    _ui32PacketsInQueue = 0;
    _ui32BytesInQueue = 0;
    _ui32MaximumSize = 0;
}

inline int PendingPacketQueue::lock (void)
{
    return _m.lock();
}

inline int PendingPacketQueue::unlock (void)
{
    return _m.unlock();
}

inline bool PendingPacketQueue::isEmpty (void)
{
    return (_ui32PacketsInQueue == 0);
}

inline uint32 PendingPacketQueue::getBytesInQueue (void)
{
    return _ui32BytesInQueue;
}

inline uint32 PendingPacketQueue::getPacketsInQueue (void)
{
    return _ui32PacketsInQueue;
}

inline uint32 PendingPacketQueue::getSpaceAvailable (void)
{
    return (_ui32MaximumSize - _ui32BytesInQueue);
}

inline bool PendingPacketQueue::insert (PacketWrapper *pWrapper, int64 i64Timeout)
{
    int64 i64StartTime = NOMADSUtil::getTimeInMilliseconds();
    _m.lock();
    while (true) {
        if(_bClosed) {
            return false;
        }
        //printf ("PendingPacketQueue::insert %d _ui32BytesInQueue %d\n", pWrapper->getMessageTSN(), _ui32BytesInQueue);
        if (_ui32BytesInQueue + pWrapper->getPacket()->getPacketSize() <= _ui32MaximumSize) {
            break;
        }
        else {
            if (i64Timeout <= 0) {
                _cv.wait ();             // waiting to be notified by extract() or cancel()
            }
            else {
                int64 i64TimeToWait = i64Timeout - (NOMADSUtil::getTimeInMilliseconds() - i64StartTime);
                if (i64TimeToWait <= 0) {
                    // Could not insert the packet within the timeout specified
                    _m.unlock();
                    return false;
                }
                else {
                    _cv.wait (i64TimeToWait);
                }
            }
        }
    }
    // There is room in the queue to insert the packet
    Node *pNewNode = new Node;
    pNewNode->pData = pWrapper;

    if (_pFirstNode == nullptr) {
        // There are no other elements - just insert at the beginning which is also the end
        _pFirstNode = _pLastNode = pNewNode;
        _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
        _ui32PacketsInQueue++;
        _m.unlock();
        return true;
    }

    else {
        // Find the right spot to insert the new node
        if ((pNewNode->pData->getPacket()->isIntermediateFragment()) || (pNewNode->pData->getPacket()->isLastFragment())) {
            // The new node to be inserted is an intermediate or a last fragment
            // Its place is after the last fragment in the queue with the same TSN// Check for the insertion starting at the end of the list
            Node *pTempNode = _pLastNode;
            while (pTempNode != nullptr) {
                if (pNewNode->pData->getMessageTSN() == pTempNode->pData->getMessageTSN()) {
                    // Insert after this node which is the last enqued fragment of the same packet
                    pNewNode->pPrev = pTempNode;
                    if (pTempNode == _pLastNode) {
                        _pLastNode = pNewNode;
                    }
                    else {
                        pNewNode->pNext = pTempNode->pNext;
                        pTempNode->pNext->pPrev = pNewNode;
                    }
                    pTempNode->pNext = pNewNode;
                    _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
                    _ui32PacketsInQueue++;
                    _m.unlock();
                    return true;
                }
                pTempNode = pTempNode->pPrev;
            }
            // There are no other fragments with this TSN (they have already been sent)
            // Enqueue this fragment at the beginning of the queue
            pNewNode->pNext = _pFirstNode;
            _pFirstNode->pPrev = pNewNode;
            _pFirstNode = pNewNode;
            _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
            _ui32PacketsInQueue++;
            _m.unlock();
            return true;
        }
        else {
            // This is a whole packet or a first fragment
            // Insert it accordingly to its priority = after the last packet with the same or higher priority
            Node *pTempNode = _pLastNode;
            while (pTempNode != nullptr) {
                if (lowerOrSamePriority (pNewNode->pData, pTempNode->pData)) {
                    // Insert the packet after Temp which is the first one with higher or same priority
                    pNewNode->pPrev = pTempNode;
                    if (pTempNode == _pLastNode) {
                        _pLastNode = pNewNode;
                    }
                    else {
                        pNewNode->pNext = pTempNode->pNext;
                        pTempNode->pNext->pPrev = pNewNode;
                    }
                    pTempNode->pNext = pNewNode;
                    _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
                    _ui32PacketsInQueue++;
                    _m.unlock();
                    return true;
                }
                pTempNode = pTempNode->pPrev;
            }
            // This packet has the highest priority
            // Check if we have intermediate or last fragments at the head of the queue
            // We don't have to break the order
            pTempNode = _pFirstNode; // packet at the beginning of the queue
            if ((pTempNode->pData->getPacket()->isIntermediateFragment()) || (pTempNode->pData->getPacket()->isLastFragment())) {
                // go to insert at the end of this fragmented packet
                // check when the fragments of a single packet are finished using the messageTSN
                uint32 ui32HeadMsgTSN = _pFirstNode->pData->getMessageTSN();
                while (pTempNode != nullptr) {
                    if (pTempNode->pData->getMessageTSN() != ui32HeadMsgTSN) {
                        // We skipped the fragments insert before pTempNode
                        pNewNode->pPrev = pTempNode->pPrev;
                        pNewNode->pNext = pTempNode;
                        pTempNode->pPrev->pNext = pNewNode;
                        pTempNode->pPrev = pNewNode;
                        _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
                        _ui32PacketsInQueue++;
                        _m.unlock();
                        return true;
                    }
                    pTempNode = pTempNode->pNext;
                }
                // Reached the end of the queue: insert as last element
                pNewNode->pPrev = _pLastNode;
                _pLastNode->pNext = pNewNode;
                _pLastNode = pNewNode;
                _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
                _ui32PacketsInQueue++;
                _m.unlock();
                return true;
            }
            else {
                // Enqueue this fragment at the beginning of the queue
                pNewNode->pNext = _pFirstNode;
                _pFirstNode->pPrev = pNewNode;
                _pFirstNode = pNewNode;
                _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
                _ui32PacketsInQueue++;
                _m.unlock();
                return true;
            }
        }
    }
}

inline PacketWrapper * PendingPacketQueue::peek (void)
{
    _m.lock();
    if (_pFirstNode == nullptr) {
        _m.unlock();
        return nullptr;
    }
    else {
        PacketWrapper *pData = _pFirstNode->pData;
        _m.unlock();
        return pData;
    }
}

inline int PendingPacketQueue::remove (PacketWrapper *pWrapper)
{
    _m.lock();
    if (_pFirstNode == nullptr) {
        // List is empty
        _m.unlock();
        return -1;
    }
    //printf ("PendingPacketQueue::remove %d\n", pWrapper->getMessageTSN());
    if (_pFirstNode->pData == pWrapper) {
        // The node is the first node in the list
        Node *pNodeToDelete = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        if (_pFirstNode == nullptr) {
            // The list is now empty
            _pLastNode = nullptr;
        }
        else {
            _pFirstNode->pPrev = nullptr;
        }
        delete pNodeToDelete;
        _ui32BytesInQueue -= pWrapper->getPacket()->getPacketSize();
        _ui32PacketsInQueue--;
        _cv.notifyAll();
        _m.unlock();
        return 0;
    }
    else {
        // We should never get into the else!!!
        Node *pTempNode = _pFirstNode->pNext;
        while (pTempNode != nullptr) {
            if (pTempNode->pData == pWrapper) {
                // Found the node to delete
                Node *pNodeToDelete = pTempNode;
                pTempNode->pPrev->pNext = pTempNode->pNext;
                if (pTempNode->pNext == nullptr) {
                    // Removed the last node in the list
                    _pLastNode = pTempNode->pPrev;
                }
                else {
                    pTempNode->pNext->pPrev = pTempNode->pPrev;
                }
                delete pTempNode;
                _ui32BytesInQueue -= pWrapper->getPacket()->getPacketSize();
                _ui32PacketsInQueue --;
                _cv.notifyAll();
                _m.unlock();
                return 0;
            }
        }
        // The specified packet was not found
        _m.unlock();
        return -2;
    }
}

inline int PendingPacketQueue::cancel (bool bReliable, bool bSequenced, uint16 ui16TagId, uint8 * pui8HigherPriority)
{
    int iPacketsDeleted = 0;
    _m.lock();
    if (_pFirstNode == nullptr) {
        // List is empty
        _m.unlock();
        return iPacketsDeleted;
    }
    // Check if the first node needs to be deleted
    while (true) {
        Packet * pPacket = _pFirstNode->pData->getPacket();
        if ((pPacket->getTagId() == ui16TagId) && (pPacket->isReliablePacket() == bReliable) && (pPacket->isSequencedPacket() == bSequenced)) {
            Node *pNodeToDelete = _pFirstNode;
            uint8 ui8MsgPriority = _pFirstNode->pData->getPriority();
            // Keep track of the priority
            if ((pui8HigherPriority != nullptr) && ((*pui8HigherPriority) < ui8MsgPriority)) {
                // Assign the priority of the message being cancelled if it is higher than the current one
                *pui8HigherPriority = ui8MsgPriority;
            }
            _ui32BytesInQueue -= pPacket->getPacketSize();
            _pFirstNode = _pFirstNode->pNext;
            _ui32PacketsInQueue--;
            delete pNodeToDelete->pData->getPacket();
            delete pNodeToDelete->pData;
            delete pNodeToDelete;
            iPacketsDeleted++;
            _cv.notifyAll();
            if (_pFirstNode == nullptr) {
                // The list is now empty
                _pLastNode = nullptr;
                break;
            }
            else {
                _pFirstNode->pPrev = nullptr;
            }
            // Now we want to loop again and check the (new) _pFirstNode
        }
        else {
            // Done processing first (and consecutive nodes)
            break;
        }
    }
    if (_pFirstNode) {
        Node *pTempNode = _pFirstNode->pNext;
        while (pTempNode != nullptr) {
            Packet *pPacket = pTempNode->pData->getPacket();
            if ((pPacket->getTagId() == ui16TagId) && (pPacket->isReliablePacket() == bReliable) && (pPacket->isSequencedPacket() == bSequenced)) {
                // Found a node to delete
                Node *pNodeToDelete = pTempNode;
                pTempNode = pTempNode->pNext;
                pNodeToDelete->pPrev->pNext = pNodeToDelete->pNext;
                if (pNodeToDelete->pNext == nullptr) {
                    // Removed the last node in the list
                    _pLastNode = pNodeToDelete->pPrev;
                }
                else {
                    pNodeToDelete->pNext->pPrev = pNodeToDelete->pPrev;
                }
                _ui32BytesInQueue -= pPacket->getPacketSize();
                _ui32PacketsInQueue--;
                delete pNodeToDelete->pData->getPacket();
                delete pNodeToDelete->pData;
                delete pNodeToDelete;
                iPacketsDeleted++;
                _cv.notifyAll();
            }
            else {
                pTempNode = pTempNode->pNext;
            }
        }
    }
    _m.unlock();
    return iPacketsDeleted;
}

inline void PendingPacketQueue::close (void)
{
    _m.lock();
    _bClosed = true;
    _cv.notifyAll();
    _m.unlock();
}

inline int PendingPacketQueue::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    objectFreezer.putUInt32 (_ui32MaximumSize);
    objectFreezer.putBool (_bCrossSequencing);
    objectFreezer.putUInt32 (_ui32PacketsInQueue);

/*    printf ("PendingPacketQueue\n");
    printf ("_ui32MaximumSize %lu\n", _ui32MaximumSize);
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/

    // Do not freeze _ui32BytesInQueue it is computable
    // Go through the whole list of nodes
    Node *pCurrNode = _pFirstNode;
    for (uint32 i=0; i<_ui32PacketsInQueue; i++) {
        //printf ("***** i = %d\n", i);
        if (0 != pCurrNode->pData->freeze (objectFreezer)) {
            // return -1 is if objectFreezer.endObject() don't end with success
            return -2;
        }
        pCurrNode = pCurrNode->pNext;
    }

    return 0;
}

inline int PendingPacketQueue::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    objectDefroster >> _ui32MaximumSize;
    objectDefroster >> _bCrossSequencing;
    objectDefroster >> _ui32PacketsInQueue;

/*    printf ("PendingPacketQueue\n");
    printf ("_ui32MaximumSize %lu\n", _ui32MaximumSize);
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/

    // Reconstruct the queue
    for (uint32 i=0; i<_ui32PacketsInQueue; i++){
        //printf ("***** i = %d\n", i);
        Node *pNewNode = new Node;
        PacketWrapper *pWrapper = new PacketWrapper ((uint32) 0, (int64) 0); // Fake values for the initialization
        if (0 != pWrapper->defrost (objectDefroster)) {
            return -2;
        }
        pNewNode->pData = pWrapper;
        _ui32BytesInQueue += pNewNode->pData->getPacket()->getPacketSize();
        if (_pFirstNode == nullptr) {
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

inline bool PendingPacketQueue::lowerOrSamePriority (PacketWrapper *pLHSWrapper, PacketWrapper *pRHSWrapper)
{
    if ((pLHSWrapper->getPriority() <= pRHSWrapper->getPriority()) ||
        ((pLHSWrapper->getPacket()->isSequencedPacket() && (pRHSWrapper->getPacket()->isSequencedPacket())) &&
        (_bCrossSequencing || (pLHSWrapper->getPacket()->isReliablePacket() == pRHSWrapper->getPacket()->isReliablePacket())))) {
        return true;
    }
    return false;
}

inline PendingPacketQueue::Node::Node (void)
{
    pPrev = pNext = nullptr;
    pData = nullptr;
}

#endif   // #ifndef INCL_PENDING_PACKET_QUEUE_H
