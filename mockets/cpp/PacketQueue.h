#ifndef INCL_PACKET_QUEUE_H
#define INCL_PACKET_QUEUE_H

/*
 * PacketQueue.h
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

#include "ConditionVariable.h"
#include "FTypes.h"
#include "Mutex.h"
#include "NLFLib.h"
#include "ObjectDefroster.h"
#include "ObjectFreezer.h"

#include "DataBuffer.h"


class PacketQueue
{
    public:
        PacketQueue (void);

        // Deletes any enqueued DataBuffer
        ~PacketQueue (void);

        bool isEmpty (void);
        uint32 getPacketCount (void);
        uint32 getCumulativeMessageSize (void);

        void insert (DataBuffer *pBuffer);

        // A negative timeout value implies wait indefinitely
        // A zero timeout value implies do not block
        // Return nullptr if no packet was present in the queue
        DataBuffer * peek (int64 i64Timeout);

        // A negative timeout value implies wait indefinitely
        // A zero timeout value implies do not block
        // Returns nullptr if no packet was avaiable to extract
        DataBuffer * extract (int64 i64Timeout);

        // Indicates that no additional data will be inserted into this queue
        // If there is no data left in the queue, calls to extract will return nullptr without blocking
        void close (void);
        
        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster, PacketProcessor *pPacketProcessor);

    private:
        struct Node
        {
            Node (void);
            Node *pNext;
            DataBuffer *pData;
        };
        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        Node *_pFirstNode;
        Node *_pLastNode;
        uint32 _ui32PacketsInQueue;
        uint32 _ui32CumulativeMessageSize;
        bool _bClosed;
};

inline PacketQueue::PacketQueue (void)
    : _cv (&_m)
{
    _pFirstNode = _pLastNode = nullptr;
    _ui32PacketsInQueue = 0;
    _ui32CumulativeMessageSize = 0;
    _bClosed = false;
}

inline PacketQueue::~PacketQueue (void)
{
    while (_pFirstNode != nullptr) {
        Node *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode->pData;
        delete pTempNode;
    }
    _pFirstNode = _pLastNode = nullptr;
}

inline bool PacketQueue::isEmpty (void)
{
    return (_pFirstNode == nullptr);
}

inline uint32 PacketQueue::getPacketCount (void)
{
    return _ui32PacketsInQueue;
}

inline uint32 PacketQueue::getCumulativeMessageSize (void)
{
    return _ui32CumulativeMessageSize;
}

inline void PacketQueue::insert (DataBuffer *pBuffer)
{
    Node *pNewNode = new Node();
    pNewNode->pData = pBuffer;
    _m.lock();
    if (_pLastNode == nullptr) {
        _pFirstNode = pNewNode;
    }
    else {
        _pLastNode->pNext = pNewNode;
    }
    _pLastNode = pNewNode;
    _ui32PacketsInQueue++;
    _ui32CumulativeMessageSize += pBuffer->getMessageSize();
    _cv.notifyAll();
    _m.unlock();
}

inline DataBuffer * PacketQueue::peek (int64 i64Timeout)
{
    int64 i64StartTime = NOMADSUtil::getTimeInMilliseconds();
    _m.lock();
    while (true) {
        if (_ui32PacketsInQueue > 0) {
            DataBuffer *pBuffer = _pFirstNode->pData;
            _m.unlock();
            return pBuffer;
        }
        else {
            if (_bClosed) {
                _m.unlock();
                return nullptr;
            }
            if (i64Timeout < 0) {
                _cv.wait();
            }
            else {
                int64 i64RemainingTime = (i64StartTime + i64Timeout) - NOMADSUtil::getTimeInMilliseconds();
                if (i64RemainingTime <= 0) {
                    _m.unlock();
                    return nullptr;
                }
                else {
                    _cv.wait (i64RemainingTime);
                }
            }
        }
    }
}

inline DataBuffer * PacketQueue::extract (int64 i64Timeout)
{
    int64 i64StartTime = NOMADSUtil::getTimeInMilliseconds();
    _m.lock();
    while (true) {
        if (_ui32PacketsInQueue > 0) {
            Node *pTempNode = _pFirstNode;
            if (_pFirstNode == _pLastNode) {
                _pLastNode = nullptr;
            }
            _pFirstNode = _pFirstNode->pNext;
            _ui32PacketsInQueue--;
            DataBuffer *pBuffer = pTempNode->pData;
            _ui32CumulativeMessageSize -= pBuffer->getMessageSize();
            delete pTempNode;
            _m.unlock();
            return pBuffer;
        }
        else {
            if (_bClosed) {
                _m.unlock();
                return nullptr;
            }
            if (i64Timeout < 0) {
                _cv.wait();
            }
            else {
                int64 i64RemainingTime = (i64StartTime + i64Timeout) - NOMADSUtil::getTimeInMilliseconds();
                if (i64RemainingTime <= 0) {
                    _m.unlock();
                    return nullptr;
                }
                else {
                    _cv.wait (i64RemainingTime);
                }
            }
        }
    }
}

inline void PacketQueue::close (void)
{
    _m.lock();
    _bClosed = true;
    _cv.notifyAll();
    _m.unlock();
}

inline int PacketQueue::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    objectFreezer.putUInt32 (_ui32PacketsInQueue);
    //printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);
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
    // Do not freeze _ui32CumulativeMessageSize it is computable
    
    return 0;
}

inline int PacketQueue::defrost (NOMADSUtil::ObjectDefroster &objectDefroster, PacketProcessor *pPacketProcessor)
{
    objectDefroster >> _ui32PacketsInQueue;
    //printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);
    // Reconstruct the node
    for (uint32 i=0; i<_ui32PacketsInQueue; i++) {
        //printf ("***** i = %d\n", i);
        Node *pNewNode = new Node();
        DataBuffer *pDataBuffer = new DataBuffer (pPacketProcessor);
        if (0 != pDataBuffer->defrost (objectDefroster)) {
            return -2;
        }
        pNewNode->pData = pDataBuffer;
        if (_pFirstNode == nullptr) {
            _pFirstNode = _pLastNode = pNewNode;
        }
        else {
            _pLastNode->pNext = pNewNode;
            _pLastNode = pNewNode;
        }
        _ui32CumulativeMessageSize += pNewNode->pData->getMessageSize();
    }
    return 0;
}

inline PacketQueue::Node::Node (void)
{
    pNext = nullptr;
    pData = nullptr;
}

#endif   // #ifndef INCL_PACKET_QUEUE_H
