#ifndef INCL_UNACKNOWLEDGED_PACKET_QUEUE_H
#define INCL_UNACKNOWLEDGED_PACKET_QUEUE_H

/*
 * UnacknowledgePacketQueue.h
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
 
 * Maintains the queue of packets that are awaiting acknowledgement from the remote side
 * The operations that need to be supported include inserting packets that have been
 * transmitted, removing packets that have been acknowledged, and iterating over packets
 * that need to be retransmitted.
 *
 * The data structure is a dual double-linked list, with the first list sorted by the
 * sequence number of the packet and the second list sorted by the retransmission time.
 * The nodes in the first linked list point to the nodes in the second linked list in order
 * to efficiently handle removing packets that have been acknowledged.
 */

#include "Packet.h"
#include "PacketWrapper.h"
#include "TSNRangeHandler.h"
#include "TimeIntervalAverage.h"
#include "CancelledTSNManager.h"

#include "ConditionVariable.h"
#include "FTypes.h"
#include "Logger.h"
#include "Mutex.h"
#include "NLFLib.h"
#include "SequentialArithmetic.h"

#include <string.h>

class UnacknowledgedPacketQueue
{
    public:
        UnacknowledgedPacketQueue (bool bUseLostPacketsDetection = false);

        // Deletes any enqueued PacketWrappers and Packets contained in the wrappers
        ~UnacknowledgedPacketQueue (void);

        // Obtains a lock on the queue
        int lock (void);

        // Releases the lock on the queue
        int unlock (void);

        bool isEmpty (void);

        // Returns a count of the number of packets in the queue
        uint32 getPacketCount (void);

        // Returns the total number of bytes enqueued (a sum of all the packet sizes)
        uint32 getQueuedDataSize (void);

        int insert (PacketWrapper *pWrapper);

        // Deletes packets in this queue whose sequence numbers are <= the specified sequence number
        // Both the PacketWrappers and the Packets are deleted
        // Returns the number of packets removed
        int acknowledgePacketsUpto (uint32 ui32SequenceNum, uint64 &ui64NumberOfAcknowledgedBytes);

        // Deletes packets in this queue whose sequence numbers are in the range specified (inclusive)
        // Both the PacketWrappers and the Packets are deleted
        // Returns the number of packets removed
        int acknowledgePacketsWithin (uint32 ui32StartSequenceNum, uint32 ui32EndSequenceNum, uint64 &ui64NumberOfAcknowledgedBytes);

        // Returns the next packet in the queue that has timed out and needs to be retransmitted
        // or NULL if there is no such packet
        PacketWrapper * getNextTimedOutPacket (void);

        // Prioritizes the retransmission of the packet with sequence number ui32SeqNum by causing a timeout and
        // putting it at the head of the retransmission queue (becoming the next packet that will be retransmitted)
        int prioritizeRetransmissionOfPacket (uint32 ui32SeqNum);
	
        // Prioritizes the retransmission of all packet with sequence number < ui32SeqNum by causing a timeout and
        // putting it at the them of the retransmission queue
        int prioritizeRetransmissionOfPacketUpTo (uint32 ui32SeqNum);
	
        // Deletes the next packet in the retransmit list in the queue
        // Usually called after getNextTimedOutPacket() if the retry timeout for the packet has expired
        int deleteNextPacketInRetransmitList (void);

        // Resorts the packet in the queue given that it has been retransmitted
        // NOTE: The caller must have already updated the retransmit time (the IO time) in the wrapper
        // NOTE: The packet that has been retransmitted must currently be at the head of the queue
        //       (that is, it must have just been returned by getNextTimedOutPacket)
        int packetRetransmitted (PacketWrapper *pWrapper);

        // Cancel (delete) any packets enqueued that match the specified tag
        // The sequence numbers of the packets that are cancelled are added to the TSNRangeHandler, which can be used to generate the CancelledChunk
        // NOTE: The packets are deleted (i.e., the memory is deallocated)
        // Returns the number of packets that have been cancelled
        int cancel (uint16 ui16TagId, CancelledTSNManager *pCancelledTSNManager);

        // Reset the minimum acknowledgement time so that it is recomputed
        // The value is set to 0xFFFFFFFFUL
        void resetMinAckTime (void);

        // Returns the minimum acknowledgement time, which is the minimum length of
        // time that has elapsed between any packet that has been transmitted only
        // once (i.e., not retransmitted) and the time when the packet is acknowledged
        // Returns 0xFFFFFFFFUL if a value could not be computed since the last reset
        uint32 getMinAckTime (void);

        void dumpPacketSequenceNumbers (void);

        int resetRetrTimeoutRetrCount (uint32 ui32RetransmitTO);
        
        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);
        
    private:
        struct Node
        {
            Node (void);
            Node *pPrev;
            Node *pNext;
            Node *pOtherListNode;
            Node *pOtherListNode2;
            PacketWrapper *pData;
            virtual bool operator < (const Node &rhsNode) = 0;
        };

        struct PacketSeqListNode : public Node
        {
            bool operator < (const Node &rhsNode);
        };

        struct RetransmitTimeListNode : public Node
        {
            bool operator < (const Node &rhsNode);
        };

        struct SentTimeListNode : public Node
        {
            bool operator < (const Node &rhsNode);
        };

        struct List
        {
            Node *pFirstNode;
            Node *pLastNode;
        };

    private:
        int insertIntoLists (PacketWrapper *pWrapper);
        int insertIntoList (List *pList, Node *pNewNode);
        int insertIntoListFromEnd (List *pList, Node *pNewNode);
        int removeFromList (List *pList, Node *pNode);
        int expireLostPackets (Node *pNode);

    private:
        NOMADSUtil::Mutex _m;
        NOMADSUtil::ConditionVariable _cv;
        bool _bUseLostPacketsDetection;
        List _packetSeqList;
        List _retransmitTimeList;
        List _sentTimeList;
        uint32 _ui32PacketsInQueue;
        uint32 _ui32BytesInQueue;
        uint32 _ui32MinAckTime;   // Keeps track of the minimum time that has elapsed between
                                  // the transmission of a packet (that has not been retransmitted)
                                  // and the acknowledgement of that packet
};

inline UnacknowledgedPacketQueue::UnacknowledgedPacketQueue (bool bUseLostPacketsDetection)
    : _cv (&_m)
{
    _packetSeqList.pFirstNode = _packetSeqList.pLastNode = NULL;
    _retransmitTimeList.pFirstNode = _retransmitTimeList.pLastNode = NULL;
    _sentTimeList.pFirstNode = _sentTimeList.pLastNode = NULL;
    _ui32PacketsInQueue = 0;
    _ui32BytesInQueue = 0;
    _ui32MinAckTime = 0xFFFFFFFFUL;
    _bUseLostPacketsDetection = bUseLostPacketsDetection;
}

inline UnacknowledgedPacketQueue::~UnacknowledgedPacketQueue (void)
{
    Node *pTempNode = _packetSeqList.pFirstNode;
    while (pTempNode != NULL) {
        Node *pNodeToDelete = pTempNode;
        pTempNode = pTempNode->pNext;
        delete pNodeToDelete->pData->getPacket();
        delete pNodeToDelete->pData;
        delete pNodeToDelete->pOtherListNode;
        delete pNodeToDelete->pOtherListNode2;
        delete pNodeToDelete;
    }
    _packetSeqList.pFirstNode = _packetSeqList.pLastNode = NULL;
    _retransmitTimeList.pFirstNode = _retransmitTimeList.pLastNode = NULL;
    _sentTimeList.pFirstNode = _sentTimeList.pLastNode = NULL;
    _ui32PacketsInQueue = 0;
    _ui32BytesInQueue = 0;
    _ui32MinAckTime = 0xFFFFFFFFUL;
}

inline int UnacknowledgedPacketQueue::lock (void)
{
    return _m.lock();
}

inline int UnacknowledgedPacketQueue::unlock (void)
{
    return _m.unlock();
}

inline bool UnacknowledgedPacketQueue::isEmpty (void)
{
    return (_ui32PacketsInQueue == 0);
}

inline uint32 UnacknowledgedPacketQueue::getPacketCount (void)
{
    return _ui32PacketsInQueue;
}

inline uint32 UnacknowledgedPacketQueue::getQueuedDataSize (void)
{
    return _ui32BytesInQueue;
}

inline int UnacknowledgedPacketQueue::insert (PacketWrapper *pWrapper)
{
    int64 i64Timeout = pWrapper->getLastIOTime() + pWrapper->getRetransmitTimeout();
    if (i64Timeout <= 0) {
        return -1;
    }

    return insertIntoLists (pWrapper);
}

inline int UnacknowledgedPacketQueue::acknowledgePacketsUpto (uint32 ui32SequenceNum, uint64 &ui64NumberOfAcknowledgedBytes)
{
    int iCount = 0;
    _m.lock();
    int64 i64CurrTime = NOMADSUtil::getTimeInMilliseconds();
    Node *pTemp = _packetSeqList.pFirstNode;

    uint32 ui32AckTime;
    while (pTemp != NULL) {
        if (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pTemp->pData->getPacket()->getSequenceNum(), ui32SequenceNum)) {
            // Compute the acknowledgement time and update MinAckTime if appropriate
            if (pTemp->pData->getRetransmitCount() == 0) {
                ui32AckTime = (uint32) (i64CurrTime - pTemp->pData->getLastIOTime());
                if (ui32AckTime < _ui32MinAckTime) {
                    _ui32MinAckTime = ui32AckTime;
                }
            }

            Node *pNodeToDelete = pTemp;
            Node *pOtherNodeToDelete = pTemp->pOtherListNode;
            Node *pOtherNodeToDelete2 = pTemp->pOtherListNode2;
            if (_bUseLostPacketsDetection) {
                expireLostPackets (pOtherNodeToDelete2);
            }
            pTemp = pTemp->pNext;
            _ui32PacketsInQueue--;
            _ui32BytesInQueue -= pNodeToDelete->pData->getPacket()->getPacketSize();
            ui64NumberOfAcknowledgedBytes += pNodeToDelete->pData->getPacket()->getPacketSize();

            delete pNodeToDelete->pData->getPacket();
            delete pNodeToDelete->pData;
            removeFromList (&_packetSeqList, pNodeToDelete);
            removeFromList (&_retransmitTimeList, pOtherNodeToDelete);
            removeFromList (&_sentTimeList, pOtherNodeToDelete2);
            iCount++;
        }
        else {
            // Packets are sequentially ordered - so if the sequence number of the packet is greater than the specified
            // sequence number, there is no need to check further
            break;
        }
    }
    _m.unlock();
    return iCount;
}

inline int UnacknowledgedPacketQueue::acknowledgePacketsWithin (uint32 ui32StartSequenceNum, uint32 ui32EndSequenceNum, uint64 &ui64NumberOfAcknowledgedBytes)
{
    int iCount = 0;
    _m.lock();
    int64 i64CurrTime = NOMADSUtil::getTimeInMilliseconds();
    Node *pTemp = _packetSeqList.pFirstNode;

    while (pTemp != NULL) {
        uint32 ui32SequenceNum = pTemp->pData->getPacket()->getSequenceNum();
        if (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (ui32StartSequenceNum, ui32SequenceNum) &&
            NOMADSUtil::SequentialArithmetic::lessThanOrEqual (ui32SequenceNum, ui32EndSequenceNum)) {
            // Compute the acknowledgement time and update MinAckTime if appropriate
            if (pTemp->pData->getRetransmitCount() == 0) {
                uint32 ui32AckTime = (uint32) (i64CurrTime - pTemp->pData->getLastIOTime());
                if (ui32AckTime < _ui32MinAckTime) {
                    _ui32MinAckTime = ui32AckTime;
                }
            }

            Node *pNodeToDelete = pTemp;
            Node *pOtherNodeToDelete = pTemp->pOtherListNode;
            Node *pOtherNodeToDelete2 = pTemp->pOtherListNode2;
            if (_bUseLostPacketsDetection) {
                expireLostPackets (pOtherNodeToDelete2);
            }
            pTemp = pTemp->pNext;
            _ui32PacketsInQueue--;
            _ui32BytesInQueue -= pNodeToDelete->pData->getPacket()->getPacketSize();
            ui64NumberOfAcknowledgedBytes += pNodeToDelete->pData->getPacket()->getPacketSize();

            delete pNodeToDelete->pData->getPacket();
            delete pNodeToDelete->pData;
            removeFromList (&_packetSeqList, pNodeToDelete);
            removeFromList (&_retransmitTimeList, pOtherNodeToDelete);
            removeFromList (&_sentTimeList, pOtherNodeToDelete2);
            iCount++;
        }
        else if (NOMADSUtil::SequentialArithmetic::lessThan (ui32SequenceNum, ui32StartSequenceNum)) {
            pTemp = pTemp->pNext;      // Cannot stop yet - since the current packet is below the specified range
        }
        else {
            break;
        }
    }
    _m.unlock();
    return iCount;
}

inline PacketWrapper * UnacknowledgedPacketQueue::getNextTimedOutPacket (void)
{
    if (_retransmitTimeList.pFirstNode) {
        PacketWrapper *pWrapper = _retransmitTimeList.pFirstNode->pData;
        if ((pWrapper->getLastIOTime() + pWrapper->getRetransmitTimeout()) < NOMADSUtil::getTimeInMilliseconds()) {
            return pWrapper;
        }
    }
    return NULL;
}

inline int UnacknowledgedPacketQueue::prioritizeRetransmissionOfPacket (uint32 ui32SeqNum)
{
    Node *pTempSeq = _packetSeqList.pFirstNode;       
    Node *pTempRetr;

    _m.lock();
    while (pTempSeq != NULL) { //lookup for the packet with sequence number ui32SeqNum
	if (pTempSeq->pData->getSequenceNum() == ui32SeqNum && !pTempSeq->pData->getRetransmitCount()) { //Note: maybe the check of the retransmit count should be done at a upper level
            //when the correct packet is found, its lastIOTime is set to cause a timeout
	    pTempSeq->pData->setLastIOTime (pTempSeq->pData->getLastIOTime() - pTempSeq->pData->getRetransmitTimeout());
	    pTempRetr = pTempSeq->pOtherListNode;
	    //then the node is reordered in the retransmission list (it is positioned as first node)
	    if (pTempRetr != _retransmitTimeList.pFirstNode) {
		if (pTempRetr == _retransmitTimeList.pLastNode) {
		    pTempRetr->pPrev->pNext = NULL;
		    _retransmitTimeList.pLastNode = pTempRetr->pPrev;
		}
		else {
		    pTempRetr->pPrev->pNext = pTempRetr->pNext;
	            pTempRetr->pNext->pPrev = pTempRetr->pPrev;
		}
		pTempRetr->pPrev = NULL;
		pTempRetr->pNext = _retransmitTimeList.pFirstNode;
	        _retransmitTimeList.pFirstNode->pPrev = pTempRetr;
	        _retransmitTimeList.pFirstNode = pTempRetr;
		pTempRetr = NULL;
	    }
	    _m.unlock();
	    return 1;
	}
        pTempSeq = pTempSeq->pNext;
    }
    
    //The packet with ui32SeqNum was not found
    _m.unlock();
    return 0;
}

inline int UnacknowledgedPacketQueue::prioritizeRetransmissionOfPacketUpTo (uint32 ui32SeqNum)
{
    Node *pTempSeq = _packetSeqList.pFirstNode;
    Node *pTempRetr;

    _m.lock();
    while ((pTempSeq != NULL) && (pTempSeq->pData->getSequenceNum() < ui32SeqNum)) { //lookup for the packet with sequence number ui32SeqNum
	if (!pTempSeq->pData->getRetransmitCount()) { //Note: maybe the check of the retransmit count should be done at a upper level
            //when the correct packet is found, its lastIOTime is set to cause a timeout
	    //pTempSeq->pData->setLastIOTime (pTempSeq->pData->getLastIOTime() - pTempSeq->pData->getRetransmitTimeout());
	    pTempSeq->pData->setRetransmitTimeout(0);
	    pTempRetr = pTempSeq->pOtherListNode;
	    //then the node is reordered in the retransmission list
	    if (pTempRetr != _retransmitTimeList.pFirstNode) {
		if (pTempRetr == _retransmitTimeList.pLastNode) {
		    pTempRetr->pPrev->pNext = NULL;
		    _retransmitTimeList.pLastNode = pTempRetr->pPrev;
		}
		else {
		    pTempRetr->pPrev->pNext = pTempRetr->pNext;
	            pTempRetr->pNext->pPrev = pTempRetr->pPrev;
		}
		pTempRetr->pPrev = NULL;
		pTempRetr->pNext = _retransmitTimeList.pFirstNode;
	        _retransmitTimeList.pFirstNode->pPrev = pTempRetr;
	        _retransmitTimeList.pFirstNode = pTempRetr;
		pTempRetr = NULL;
	    }
	    _m.unlock();
	    return 1;
	}
        pTempSeq = pTempSeq->pNext;
    }
    
    //The packet with ui32SeqNum was not found
    _m.unlock();
    return 0;
}

inline int UnacknowledgedPacketQueue::deleteNextPacketInRetransmitList (void)
{
    _m.lock();
    if (_retransmitTimeList.pFirstNode == NULL) {
        _m.unlock();
        return -1;
    }
    Node *pNodeToDelete = _retransmitTimeList.pFirstNode;
    Node *pOtherNodeToDelete = pNodeToDelete->pOtherListNode;
    Node *pOtherNodeToDelete2 = pNodeToDelete->pOtherListNode2;
    _ui32PacketsInQueue--;
    _ui32BytesInQueue -= pNodeToDelete->pData->getPacket()->getPacketSize();
    delete pNodeToDelete->pData->getPacket();
    delete pNodeToDelete->pData;
    if ((removeFromList (&_retransmitTimeList, pNodeToDelete)) || (removeFromList (&_packetSeqList, pOtherNodeToDelete)) || (removeFromList (&_sentTimeList, pOtherNodeToDelete2))) {
        _m.unlock();
        return -2;
    }
    _m.unlock();
    return 0;
}

inline int UnacknowledgedPacketQueue::packetRetransmitted (PacketWrapper *pWrapper)
{
    _m.lock();
    if (_retransmitTimeList.pFirstNode == NULL) {
        _m.unlock();
        return -1;
    }
    else if (_retransmitTimeList.pFirstNode->pData != pWrapper) {
        _m.unlock();
        return -2;
    }
    else if (_retransmitTimeList.pFirstNode == _retransmitTimeList.pLastNode) {
        // There is only one node, so nothing needs to be done
        _m.unlock();
        return 0;
    }
    // Reorder _sendTimeList: find the node and enqueue it at the end of the list
    Node *pSentTimeListNode = _retransmitTimeList.pFirstNode->pOtherListNode2;
    Node *pNode = _sentTimeList.pFirstNode;
    if (pNode == pSentTimeListNode) {
        if (_sentTimeList.pLastNode == pSentTimeListNode) {
            // nothing to do
            pNode = NULL;
        }
        else {
            _sentTimeList.pFirstNode = pNode->pNext;
            _sentTimeList.pFirstNode->pPrev = NULL;
        }
    }
    else {
        pNode = pNode->pNext;
        while (pNode != NULL) {
            if (pNode == pSentTimeListNode) {
                // Found the node to be reordered
                if (_sentTimeList.pLastNode == pNode) {
                    // The node is already at the end of the list, do nothing
                    pNode = NULL;
                }
                else {
                    pNode->pPrev->pNext = pNode->pNext;
                    pNode->pNext->pPrev = pNode->pPrev;
                    pNode->pPrev = NULL;
                    pNode->pNext = NULL;
                }
                break;
            }
            pNode = pNode->pNext;
        }
    }
    if (pNode != NULL) {
        // The node needs to be moved to the end of the list
        pNode->pPrev = _sentTimeList.pLastNode;
        _sentTimeList.pLastNode->pNext = pNode;
        _sentTimeList.pLastNode = pNode;
    }

    // Reorder _retransmitTimeList
    pNode = _retransmitTimeList.pFirstNode;
    _retransmitTimeList.pFirstNode = _retransmitTimeList.pFirstNode->pNext;
    _retransmitTimeList.pFirstNode->pPrev = NULL;
    pNode->pNext = NULL;
    pNode->pPrev = NULL;
    insertIntoListFromEnd (&_retransmitTimeList, pNode);
    
    _m.unlock();
    return 0;
}

inline int UnacknowledgedPacketQueue::cancel (uint16 ui16TagId, CancelledTSNManager *pCancelledTSNManager)
{
    int iDeletedPackets = 0;
    _m.lock();
    Node *pTemp = _packetSeqList.pFirstNode;
    while (pTemp != NULL) {
        Packet *pPacket = pTemp->pData->getPacket();
        if (pPacket->getTagId() == ui16TagId) {
            Node *pNodeToDelete = pTemp;
            Node *pOtherNodeToDelete = pTemp->pOtherListNode;
            Node *pOtherNodeToDelete2 = pTemp->pOtherListNode2;
            pTemp = pTemp->pNext;
            _ui32PacketsInQueue--;
            _ui32BytesInQueue -= pPacket->getPacketSize();
            pCancelledTSNManager->addCancelledPacketTSN (pPacket->getSequenceNum());
            delete pPacket;
            delete pNodeToDelete->pData;
            removeFromList (&_packetSeqList, pNodeToDelete);
            removeFromList (&_retransmitTimeList, pOtherNodeToDelete);
            removeFromList (&_sentTimeList, pOtherNodeToDelete2);
            iDeletedPackets++;
        }
        else {
            pTemp = pTemp->pNext;
        }
    }
    _m.unlock();
    return iDeletedPackets;
}

inline void UnacknowledgedPacketQueue::resetMinAckTime (void)
{
    _ui32MinAckTime = 0xFFFFFFFFUL;
}

inline uint32 UnacknowledgedPacketQueue::getMinAckTime (void)
{
    return _ui32MinAckTime;
}

inline void UnacknowledgedPacketQueue::dumpPacketSequenceNumbers (void)
{
    if (NOMADSUtil::pLogger) {
        _m.lock();
        char szBuf[8192];
        szBuf[0] = '\0';
        Node *pTempNode = _packetSeqList.pFirstNode;
        while (pTempNode != NULL) {
            char szPacketNum[10];
            sprintf (szPacketNum, "%u ", pTempNode->pData->getPacket()->getSequenceNum());
            strcat (szBuf, szPacketNum);
            pTempNode = pTempNode->pNext;
        }
        NOMADSUtil::pLogger->logMsg ("UnacknowledgedPacketQueue::dumpPacketSequenceNumbers", NOMADSUtil::Logger::L_MediumDetailDebug,
                                     "%s\n", szBuf);
        _m.unlock();
    }
}

inline int UnacknowledgedPacketQueue::resetRetrTimeoutRetrCount (uint32 ui32RetransmitTO)
{
    _m.lock();
    // Loop through all the nodes (packets) in the list.
    // Loop through _packetSeqList, but reorder nodes in _retransmitTimeList
    Node *pTempNode = _packetSeqList.pFirstNode;
    Node *pTempNodeRetrTimeList;
    while (pTempNode != NULL) {
        pTempNodeRetrTimeList = pTempNode->pOtherListNode;
        pTempNodeRetrTimeList->pData->setRetransmitTimeout (ui32RetransmitTO);
        pTempNodeRetrTimeList->pData->resetRetransmitCount();
        // Reorder _retransmitTimeList (remove and insert node)
        // REMOVE
        if (_retransmitTimeList.pFirstNode == pTempNodeRetrTimeList) {
            // Removing the first element in the list
            if (_retransmitTimeList.pLastNode == pTempNodeRetrTimeList) {
                // This was also the only element in the list
                _retransmitTimeList.pFirstNode = _retransmitTimeList.pLastNode = NULL;
            }
            else {
                _retransmitTimeList.pFirstNode = _retransmitTimeList.pFirstNode->pNext;
                _retransmitTimeList.pFirstNode->pPrev = NULL;
            }
        }
        else if (_retransmitTimeList.pLastNode == pTempNodeRetrTimeList) {
            // Removing the last element in the list
            _retransmitTimeList.pLastNode = pTempNodeRetrTimeList->pPrev;
            pTempNodeRetrTimeList->pPrev->pNext = NULL;
        }
        else {
            // Removing a non-boundary element in the list
            pTempNodeRetrTimeList->pPrev->pNext = pTempNodeRetrTimeList->pNext;
            pTempNodeRetrTimeList->pNext->pPrev = pTempNodeRetrTimeList->pPrev;
        }

        // INSERT in the correct order
        if (insertIntoList (&_retransmitTimeList, pTempNodeRetrTimeList)) {
            return -2;
        }
        pTempNode = pTempNode->pNext;
    }
    _m.unlock();
    return 0;
}

inline int UnacknowledgedPacketQueue::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    // List _packetSeqList, List _retransmitTimeList and List _sentTimeList contain the same nodes
    // but they are not in the same order. I can freeze only _retransmitTimeList,
    // because it has newer information about the last transmission time, and during
    // the insertion the two queues will be recreated in the correct order.
    objectFreezer.putUInt32 (_ui32PacketsInQueue);
    
/*    printf ("UnacknowledgedPacketQueue\n");
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/
    
    // Go through the whole list of nodes
    Node *pCurrNode = _retransmitTimeList.pFirstNode;
    for (uint32 i=0; i<_ui32PacketsInQueue; i++) {
        //printf ("***** i = %d\n", i);
        if (0 != pCurrNode->pData->freeze (objectFreezer)) {
            // return -1 is if objectFreezer.endObject() don't end with success
            return -2;
        }
        pCurrNode = pCurrNode->pNext;
    }
    // Do not freeze _ui32BytesInQueue, it is computable
    // Do not frezze _ui32MinAckTime we can initializate it with the maximum
    // value and at the first ack it will be calculated
    
    return 0;
}

inline int UnacknowledgedPacketQueue::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    objectDefroster >> _ui32PacketsInQueue;
    
/*    printf ("UnacknowledgedPacketQueue\n");
    printf ("_ui32PacketsInQueue %lu\n", _ui32PacketsInQueue);*/
    
    // Insert all nodes in the queues
    for (uint32 i=0; i<_ui32PacketsInQueue; i++){
        //printf ("***** i = %d\n", i);
        PacketWrapper *pWrapper = new PacketWrapper ((uint32) 0, (int64) 0); // Fake values for the initialization
        if (0 != pWrapper->defrost (objectDefroster)) {
            return -2;
        }
        insertIntoLists (pWrapper);
    }
    return 0;
}

inline int UnacknowledgedPacketQueue::insertIntoLists (PacketWrapper *pWrapper)
{
    PacketSeqListNode *pPacketSeqListNode = new PacketSeqListNode;
    RetransmitTimeListNode *pRetransmitTimeListNode = new RetransmitTimeListNode;
    SentTimeListNode *pSentTimeListNode = new SentTimeListNode;
    pPacketSeqListNode->pData = pWrapper;
    pRetransmitTimeListNode->pData = pWrapper;
    pSentTimeListNode->pData = pWrapper;
    pPacketSeqListNode->pOtherListNode = pRetransmitTimeListNode;
    pPacketSeqListNode->pOtherListNode2 = pSentTimeListNode;
    pRetransmitTimeListNode->pOtherListNode = pPacketSeqListNode;
    pRetransmitTimeListNode->pOtherListNode2 = pSentTimeListNode;
    pSentTimeListNode->pOtherListNode = pPacketSeqListNode;
    pSentTimeListNode->pOtherListNode2 = pRetransmitTimeListNode;


    _m.lock();
    
    if (insertIntoListFromEnd (&_packetSeqList, pPacketSeqListNode)) {
        _m.unlock();
        return -2;
    }

    if (insertIntoListFromEnd (&_retransmitTimeList, pRetransmitTimeListNode)) {
        _m.unlock();
        return -3;
    }

    // Insert into _sentTimeList: always insert at the end
    if (_sentTimeList.pFirstNode == NULL) {
        // There are no other elements - just insert at the beginning
        _sentTimeList.pFirstNode = _sentTimeList.pLastNode = pSentTimeListNode;
    }
    else {
        // Insert at the end
        pSentTimeListNode->pPrev = _sentTimeList.pLastNode;
        _sentTimeList.pLastNode->pNext = pSentTimeListNode;
        _sentTimeList.pLastNode = pSentTimeListNode;
    }

    _ui32PacketsInQueue++;
    _ui32BytesInQueue += pWrapper->getPacket()->getPacketSize();
    _m.unlock();
    return 0;
}

inline int UnacknowledgedPacketQueue::insertIntoList (List *pList, Node *pNewNode)
{
    if (pList->pFirstNode == NULL) {
        // There are no other elements - just insert at the beginning
        pList->pFirstNode = pList->pLastNode = pNewNode;
        return 0;
    }
    else if ((*pNewNode) < (*(pList->pFirstNode))) {
        // Need to insert this new packet at the head of the list
        pNewNode->pNext = pList->pFirstNode;
        pList->pFirstNode->pPrev = pNewNode;
        pList->pFirstNode = pNewNode;
        return 0;
    }
    else {
        // Find the right spot to insert the new node
        Node *pTempNode = pList->pFirstNode->pNext;
        while (pTempNode != NULL) {
            if ((*pTempNode) < (*pNewNode)) {
                // Have not found the right place yet - continue
                pTempNode = pTempNode->pNext;
            }
            else {
                // Need to insert before pTempNode
                pNewNode->pNext = pTempNode;
                pNewNode->pPrev = pTempNode->pPrev;
                pTempNode->pPrev->pNext = pNewNode;
                pTempNode->pPrev = pNewNode;
                return 0;
            }
        }
        // Need to insert the node at the end
        pNewNode->pPrev = pList->pLastNode;
        pNewNode->pNext = NULL;
        pList->pLastNode->pNext = pNewNode;
        pList->pLastNode = pNewNode;
        return 0;
    }
}
inline int UnacknowledgedPacketQueue::insertIntoListFromEnd (List *pList, Node *pNewNode)
{
    if (pList->pLastNode == NULL) {
        // There are no other elements - just insert at the end (which is also the beginning)
        pList->pFirstNode = pList->pLastNode = pNewNode;
        return 0;
    }
    else if ((*(pList->pLastNode)) < (*pNewNode)) {
        // Need to insert this new packet at the tail of the list
        pNewNode->pPrev = pList->pLastNode;
        pList->pLastNode->pNext = pNewNode;
        pList->pLastNode = pNewNode;
        return 0;
    }
    else {
        // Find the right spot to insert the new node
        Node *pTempNode = pList->pLastNode->pPrev;
        while (pTempNode != NULL) {
            if ((*pNewNode) < (*pTempNode)) {
                // Have not found the right place yet - continue
                pTempNode = pTempNode->pPrev;
            }
            else {
                // Need to insert after pTempNode
                pNewNode->pPrev = pTempNode;
                pNewNode->pNext = pTempNode->pNext;
                pTempNode->pNext->pPrev = pNewNode;
                pTempNode->pNext = pNewNode;
                return 0;
            }
        }
        // Need to insert the node at the beginning
        pNewNode->pNext = pList->pFirstNode;
        pList->pFirstNode->pPrev = pNewNode;
        pList->pFirstNode = pNewNode;
        return 0;
    }
}

inline int UnacknowledgedPacketQueue::removeFromList (List *pList, Node *pNode)
{
    if (pList->pFirstNode == NULL) {
        // There are no elements in the list
        return -1;
    }
    else if (pList->pFirstNode == pNode) {
        // Removing the first element in the list
        if (pList->pLastNode == pNode) {
            // This was also the only element in the list
            pList->pFirstNode = pList->pLastNode = NULL;
        }
        else {
            pList->pFirstNode = pList->pFirstNode->pNext;
            pList->pFirstNode->pPrev = NULL;
        }
    }
    else if (pList->pLastNode == pNode) {
        // Removing the last element in the list
        pList->pLastNode = pNode->pPrev;
        pNode->pPrev->pNext = NULL;
    }
    else {
        // Removing a non-boundary element in the list
        pNode->pPrev->pNext = pNode->pNext;
        pNode->pNext->pPrev = pNode->pPrev;
    }
    delete pNode;
    return 0;
}

inline int UnacknowledgedPacketQueue::expireLostPackets (Node *pNode)
{
    //if (NOMADSUtil::pLogger) {
    //    NOMADSUtil::pLogger->logMsg ("UnacknowledgedPacketQueue::expireLostPackets", NOMADSUtil::Logger::L_MediumDetailDebug,
    //                                 "removing %d\n", pNode->pData->getSequenceNum());
    //}
    _m.lock();
    if ((_sentTimeList.pFirstNode == NULL) || (_sentTimeList.pFirstNode->pData->getSequenceNum() == pNode->pData->getSequenceNum())) {
        // There are no elements in the list or the element is the first one
        //if (NOMADSUtil::pLogger) {
        //    NOMADSUtil::pLogger->logMsg ("UnacknowledgedPacketQueue::expireLostPackets", NOMADSUtil::Logger::L_MediumDetailDebug,
        //                                 "no lost packets to expire\n");
        //}
        _m.unlock();
        return 0;
    }

    int iExpiredPackets = 0;
    // Cycle from the head of the list because if we did not exit with the first
    // 'if' we need to expire the timeout of the head of the queue
    Node *pTempSentTimeListNode = _sentTimeList.pFirstNode;
    while (pTempSentTimeListNode != NULL) {
        if (pTempSentTimeListNode != pNode) {
            // Mark this node for retransmission (i.e. expire the retransmission timeout)
            // Only if it has not been marked already
            if (pTempSentTimeListNode->pData->getRetransmitTimeout() != 0) {
                if (NOMADSUtil::pLogger) {
                    NOMADSUtil::pLogger->logMsg ("UnacknowledgedPacketQueue::expireLostPackets", NOMADSUtil::Logger::L_MediumDetailDebug,
                                                 "Expire retransmission timeout of packet %d\n", pTempSentTimeListNode->pData->getSequenceNum());
                }
                pTempSentTimeListNode->pData->setRetransmitTimeout (0);
                iExpiredPackets++;

                // Reorder _retransmitTimeList
                // Find the node and remove it from its current location
                Node *pRetransmitTimeListNode = pTempSentTimeListNode->pOtherListNode2;
                Node *pTempNode = _retransmitTimeList.pFirstNode;
                if (pTempNode == pRetransmitTimeListNode) {
                    if (_retransmitTimeList.pLastNode == pRetransmitTimeListNode) {
                        // nothing to do
                        _m.unlock();
                        return 0;
                    }
                    _retransmitTimeList.pFirstNode = pRetransmitTimeListNode->pNext;
                    _retransmitTimeList.pFirstNode->pPrev = NULL;
                }
                else {
                    pTempNode = pTempNode->pNext;
                    while (pTempNode != NULL) {
                        if (pTempNode == pRetransmitTimeListNode) {
                            // Found the node to be reordered
                            if (_retransmitTimeList.pLastNode == pRetransmitTimeListNode) {
                                pTempNode->pPrev->pNext = NULL;
                                _retransmitTimeList.pLastNode = pTempNode->pPrev;
                            }
                            else {
                                pTempNode->pPrev->pNext = pTempNode->pNext;
                                pTempNode->pNext->pPrev = pTempNode->pPrev;
                            }
                            pTempNode->pPrev = NULL;
                            pTempNode->pNext = NULL;
                            break;
                        }
                        pTempNode = pTempNode->pNext;
                    }
                }
                // Reinsert in the correct order
                insertIntoList (&_retransmitTimeList, pRetransmitTimeListNode);
            }
        }
        else {
            break;
        }
        pTempSentTimeListNode = pTempSentTimeListNode->pNext;
    }

    _m.unlock();
    return iExpiredPackets;
}

inline UnacknowledgedPacketQueue::Node::Node (void)
{
    pPrev = NULL;
    pNext = NULL;
    pOtherListNode = NULL;
    pOtherListNode2 = NULL;
    pData = NULL;
}

inline bool UnacknowledgedPacketQueue::PacketSeqListNode::operator < (const Node &rhsNode)
{
    return NOMADSUtil::SequentialArithmetic::lessThan (pData->getPacket()->getSequenceNum(), rhsNode.pData->getPacket()->getSequenceNum());
}

inline bool UnacknowledgedPacketQueue::RetransmitTimeListNode::operator < (const Node &rhsNode)
{
    int64 i64LHSTime = pData->getLastIOTime() + pData->getRetransmitTimeout();
    int64 i64RHSTime = rhsNode.pData->getLastIOTime() + rhsNode.pData->getRetransmitTimeout();
    if (i64LHSTime == i64RHSTime) {
        // Break the tie via the sequence number
        return NOMADSUtil::SequentialArithmetic::lessThan (pData->getPacket()->getSequenceNum(), rhsNode.pData->getPacket()->getSequenceNum());
    }
    else {
        return i64LHSTime < i64RHSTime;
    }
}

inline bool UnacknowledgedPacketQueue::SentTimeListNode::operator < (const Node &rhsNode)
{
    // Not used: we always insert at the end of SentTimeList
    // NB: the IO time will be the same for a lot of packets because we send many
    // packets at the same time ordering by sequence number does not make much sense
    if (pData->getLastIOTime() == rhsNode.pData->getLastIOTime()) {
        // Break the tie via the sequence number
        return NOMADSUtil::SequentialArithmetic::lessThan (pData->getPacket()->getSequenceNum(), rhsNode.pData->getPacket()->getSequenceNum());
    }
    return pData->getLastIOTime() < rhsNode.pData->getLastIOTime();
}

#endif   // #ifndef INCL_UNACKNOWLEDGED_PACKET_QUEUE_H
