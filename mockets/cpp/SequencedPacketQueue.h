#ifndef INCL_SEQUENCED_PACKET_QUEUE_H
#define INCL_SEQUENCED_PACKET_QUEUE_H

/*
 * SequencedPacketQueue.h
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
#include "SequentialArithmetic.h"

#include <string.h>


class SequencedPacketQueue
{
    public:
        SequencedPacketQueue (void);

        // Deletes any enqueued wrappers and packets
        ~SequencedPacketQueue (void);

        // Obtains a lock on the queue
        int lock (void);

        // Releases the lock on the queue
        int unlock (void);

        // Returns a count of the number of packets in the queue
        uint32 getPacketCount (void);

        // Inserts the specified packet into the queue, sorted by the packet sequence number
        // The packet is discarded if the sequence number is less than the next expected sequence number
        //     or if the packet is a duplicate (i.e., there is another packet in the queue with the same
        //     sequence number)
        // Returns true if the packet was inserted, or false if the packet was discarded
        bool insert (PacketWrapper *pWrapper);

        // Returns true if this packet (or a wrapper with an empty packet) can be inserted into the queue
        // This will return false if a packet for this TSN already exists in the queue or the TSN is less than
        // the next expected sequence number
        // NOTE: The caller should lock the queue between a call to canInsert() and insert()
        bool canInsert (uint32 ui32TSN);

        PacketWrapper * peek (void);

        // Removes the specified packet from the queue
        // NOTE: The function only compares the pointer values, so the object must be the
        // exact same one that is contained in the queue
        // NOTE: The memory for the packet is not deallocated by this method
        // NOTE: Even though the remove is typically called for the first packet that was
        //       retrieved using peek(), this method is still written to check the whole
        //       list; the method still checks the first packet so it is still efficient
        //       if the packet happens to be the first one
        int remove (PacketWrapper *pWrapper);

        void setNextExpectedSequenceNum (uint32 ui32NextExpectedSequenceNum);
        uint32 getNextExpectedSequenceNum (void);

        void dumpPacketSequenceNumbers (void);

        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    private:
        struct Node
        {
            Node (void);
            Node *pPrev;
            Node *pNext;
            PacketWrapper *pData;
        };

        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        Node *_pFirstNode;
        Node *_pLastNode;
        uint32 _ui32PacketsInQueue;
        uint32 _ui32NextExpectedSequenceNum;
};

inline SequencedPacketQueue::SequencedPacketQueue (void)
    : _cv (&_m)
{
    _pFirstNode = _pLastNode = nullptr;
    _ui32PacketsInQueue = 0;
    _ui32NextExpectedSequenceNum = 0;
}

inline SequencedPacketQueue::~SequencedPacketQueue (void)
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
    _ui32NextExpectedSequenceNum = 0;
}

inline int SequencedPacketQueue::lock (void)
{
    return _m.lock();
}

inline int SequencedPacketQueue::unlock (void)
{
    return _m.unlock();
}

inline uint32 SequencedPacketQueue::getPacketCount (void)
{
    return _ui32PacketsInQueue;
}

inline bool SequencedPacketQueue::insert (PacketWrapper *pWrapper)
{
    _m.lock();
    uint32 ui32NewSequenceNum = pWrapper->getSequenceNum();
    if (NOMADSUtil::SequentialArithmetic::lessThan (ui32NewSequenceNum, _ui32NextExpectedSequenceNum)) {
        // Discarding packet because it is below the threshold of sequence numbers that are acceptable
        _m.unlock();
        return false;
    }

    // Check for the insertion starting at the end of the list
    if (_pLastNode == nullptr) {
        // There are no other elements - just insert at the end (which is also the beginning)
        Node *pNewNode = new Node;
        pNewNode->pData = pWrapper;
        _pFirstNode = _pLastNode = pNewNode;
        _ui32PacketsInQueue++;
        _m.unlock();
        return true;
    }
    else if (_pLastNode->pData->getSequenceNum() == ui32NewSequenceNum) {
        // This is a duplicate of the last packet - no need to insert
        _m.unlock();
        return false;
    }
    else if (NOMADSUtil::SequentialArithmetic::lessThan (_pLastNode->pData->getSequenceNum(), ui32NewSequenceNum)) {
        // Need to insert this new packet at the end of the list
        Node *pNewNode = new Node;
        pNewNode->pData = pWrapper;
        pNewNode->pPrev = _pLastNode;
        _pLastNode->pNext = pNewNode;
        _pLastNode = pNewNode;
        _ui32PacketsInQueue++;
        _m.unlock();
        return true;
    }
    else if (NOMADSUtil::SequentialArithmetic::lessThan (ui32NewSequenceNum, _pFirstNode->pData->getSequenceNum())) {
        // Insert at the head, before the first node
        Node *pNewNode = new Node;
        pNewNode->pData = pWrapper;
        pNewNode->pNext = _pFirstNode;
        _pFirstNode->pPrev = pNewNode;
        _pFirstNode = pNewNode;
        _ui32PacketsInQueue++;
        _m.unlock();
        return true;
    }
    else {
        // Find the right spot to insert the new node
        Node *pTempNode = _pLastNode->pPrev;
        while (pTempNode != nullptr) {
            if (pTempNode->pData->getSequenceNum() == ui32NewSequenceNum) {
                // This is a duplicate packet
                _m.unlock();
                return false;
            }
            else if (NOMADSUtil::SequentialArithmetic::lessThan (pTempNode->pData->getSequenceNum(), ui32NewSequenceNum)) {
                // Need to insert after pTempNode
                Node *pNewNode = new Node;
                pNewNode->pData = pWrapper;
                pNewNode->pPrev = pTempNode;
                pNewNode->pNext = pTempNode->pNext;
                pTempNode->pNext->pPrev = pNewNode;
                pTempNode->pNext = pNewNode;
                _ui32PacketsInQueue++;
                _m.unlock();
                return true;
            }
            else {
                pTempNode = pTempNode->pPrev;
            }
        }
        /*// No need to check if this is a duplicate of the packet in the first node, because
        // that would have been checked in the loop above
        Node *pNewNode = new Node;
        pNewNode->pData = pWrapper;
        pNewNode->pNext = _pFirstNode;
        _pFirstNode->pPrev = pNewNode;
        _pFirstNode = pNewNode;
        _ui32PacketsInQueue++;
        _m.unlock();
        return true;*/
    }
    _m.unlock();
    return false;
}

inline bool SequencedPacketQueue::canInsert (uint32 ui32TSN)
{
    _m.lock();
    if (NOMADSUtil::SequentialArithmetic::lessThan (ui32TSN, _ui32NextExpectedSequenceNum)) {
        // The TSN specified is below the threshold of sequence numbers that are acceptable
        _m.unlock();
        return false;
    }
    Node *pTempNode = _pFirstNode;
    while (pTempNode != nullptr) {
        if (pTempNode->pData->getSequenceNum() == ui32TSN) {
            // This is a duplicate sequence number
            _m.unlock();
            return false;
        }
        else if (NOMADSUtil::SequentialArithmetic::greaterThan (pTempNode->pData->getSequenceNum(), ui32TSN)) {
            // Went past the point in the list where this sequence number would have existed
            break;
        }
        else {
            pTempNode = pTempNode->pNext;
        }
    }
    // Did not find the sequence number in the list
    _m.unlock();
    return true;
}

inline PacketWrapper * SequencedPacketQueue::peek (void)
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

inline int SequencedPacketQueue::remove (PacketWrapper *pWrapper)
{
    _m.lock();
    if (_pFirstNode == nullptr) {
        // List is empty
        _m.unlock();
        return -1;
    }
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
        _ui32PacketsInQueue--;
        _m.unlock();
        return 0;
    }
    else {
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
                _ui32PacketsInQueue --;
                _m.unlock();
                return 0;
            }
        }
        // The specified packet was not found
        _m.unlock();
        return -2;
    }
}

inline void SequencedPacketQueue::setNextExpectedSequenceNum (uint32 ui32NextExpectedSequenceNum)
{
    _m.lock();

    _ui32NextExpectedSequenceNum = ui32NextExpectedSequenceNum;

    // Check and remove any packets in the queue that are below the new threshold
    while (_pFirstNode) {
        if (NOMADSUtil::SequentialArithmetic::lessThan (_pFirstNode->pData->getSequenceNum(), ui32NextExpectedSequenceNum)) {
            Node *pNodeToDelete = _pFirstNode;
            _pFirstNode = _pFirstNode->pNext;
            if (_pFirstNode == nullptr) {
                _pLastNode = nullptr;
            }
            else {
                _pFirstNode->pPrev = nullptr;
            }
            delete pNodeToDelete->pData->getPacket();
            delete pNodeToDelete->pData;
            delete pNodeToDelete;
            _ui32PacketsInQueue--;
        }
        else {
            break;
        }
    }
    _m.unlock();
}

inline uint32 SequencedPacketQueue::getNextExpectedSequenceNum (void)
{
    return _ui32NextExpectedSequenceNum;
}

inline void SequencedPacketQueue::dumpPacketSequenceNumbers (void)
{
    if (NOMADSUtil::pLogger) {
        _m.lock();
        char szBuf[8192];
        szBuf[0] = '\0';
        Node *pTempNode = _pFirstNode;
        while (pTempNode != nullptr) {
            char szPacketNum[10];
            sprintf (szPacketNum, "%u ", pTempNode->pData->getSequenceNum());
            strcat (szBuf, szPacketNum);
            pTempNode = pTempNode->pNext;
        }
        NOMADSUtil::pLogger->logMsg ("SequencedPacketQueue::dumpPacketSequenceNumbers", NOMADSUtil::Logger::L_MediumDetailDebug,
                                     "%s\n", szBuf);
        _m.unlock();
    }
}

inline int SequencedPacketQueue::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    objectFreezer.putUInt32 (_ui32NextExpectedSequenceNum);
    objectFreezer.putUInt32 (_ui32PacketsInQueue);

/*    printf ("SequencedPacketQueue\n");
    printf ("_ui32NextExpectedSequenceNum %lu\n", _ui32NextExpectedSequenceNum);
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/

    // Go through the whole list of nodes
    Node *pCurrNode = _pFirstNode;
    for (uint32 i=0; i<_ui32PacketsInQueue; i++) {
        //printf ("*** %d\n", i);
        if (0 != pCurrNode->pData->freeze (objectFreezer)) {
            // return -1 is if objectFreezer.endObject() does not end with success
            return -2;
        }
        pCurrNode = pCurrNode->pNext;
    }

    return 0;
}

inline int SequencedPacketQueue::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    objectDefroster >> _ui32NextExpectedSequenceNum;
    objectDefroster >> _ui32PacketsInQueue;

/*    printf ("SequencedPacketQueue\n");
    printf ("_ui32NextExpectedSequenceNum %lu\n", _ui32NextExpectedSequenceNum);
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/

    // Insert the nodes
    for (uint32 i=0; i<_ui32PacketsInQueue; i++){
        //printf ("*** %d\n", i);
        Node *pNewNode = new Node;
        PacketWrapper *pWrapper = new PacketWrapper ((uint32) 0, (int64) 0); // Fake values for the initialization
        if (0 != pWrapper->defrost (objectDefroster)) {
            return -2;
        }
        pNewNode->pData = pWrapper;
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

inline SequencedPacketQueue::Node::Node (void)
{
    pPrev = pNext = nullptr;
    pData = nullptr;
}

#endif   // #ifndef INCL_SEQUENCED_PACKET_QUEUE_H
