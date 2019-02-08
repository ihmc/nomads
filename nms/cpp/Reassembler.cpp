/*
 * Reassembler.cpp
 *
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
 */

#include "Reassembler.h"

#include "BufferWriter.h"
#include "NetworkMessage.h"
#include "NLFLib.h"
#include "SequentialArithmetic.h"
#include "MessageFactory.h"

#include <stdlib.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

bool isNextSequenceId (uint16 ui16SeqId, uint16 ui16NextSeqId)
{
    if (ui16SeqId == 0xFFFU && ui16NextSeqId == 0) {
        // Handle the wrap around
        return true;
    }
    if (ui16NextSeqId - ui16SeqId == 1) {
        return true;
    }
    return false;
}


Reassembler::Reassembler (uint32 ui32RetransmissionTime, bool bSequenced)
    : _msgsBySourceAddress (US_INITSIZE, true)
{
    _bSequenced = bSequenced;
    _ui32RetransmissionTime = ui32RetransmissionTime;
    _ui8MaxTimeOfLostRetransmissions = MAX_NUM_OF_LOST_RETRANSMISSIONS;
}

Reassembler::~Reassembler (void)
{
    _msgsBySourceAddress.removeAll();
}

uint32 * Reassembler::getNeighborsToBeAcknowledged (uint32 &ui32NumOfNeighbors)
{
    ui32NumOfNeighbors = _msgsBySourceAddress.getCount();
    if (ui32NumOfNeighbors == 0) {
        return NULL;
    }
    uint32 *pRet = new uint32[ui32NumOfNeighbors];
    if (pRet != NULL) {
        uint32 ui32CurrentCount = 0;
        for (UInt32Hashtable<MsgQueue>::Iterator i = _msgsBySourceAddress.getAllElements();
                (!i.end()) && (ui32CurrentCount < ui32NumOfNeighbors); i.nextElement()) {
            MsgQueue *pMQ = i.getValue();
            if (pMQ != NULL) {
                // Check if the neighbor needs to receive an acknowledgment
                MsgWrapper *pMW = pMQ->_msgs.getFirst();
                bool bNeighborToBeAcknowledged = false;
                while (pMW != NULL) {
                    if (pMW->_pNetMsg->isReliableMsg()) {
                        // This is a reliable message, thus the neighbor needs
                        // acknowledgment
                        bNeighborToBeAcknowledged = true;
                        break;
                    }
                    pMW = pMQ->_msgs.getNext();
                }
                if (bNeighborToBeAcknowledged) {
                    pRet[ui32CurrentCount] = i.getKey();
                    ui32CurrentCount++;
                }
            }
        }
        ui32NumOfNeighbors = ui32CurrentCount;
    }
    else {
        // No enough memory to allocate the array
        ui32NumOfNeighbors = 0;
    }
    return pRet;
}

void * Reassembler::getSacks (uint32 ui32SourceAddress, uint32 ui32MaxLength, uint32 &ui32Lentgh)
{
    MsgQueue *pMQ = _msgsBySourceAddress.get (ui32SourceAddress);
    if (pMQ) {
        // If the _ui8MaxTimeOfLostRetransmissions is reached, the recipient
        // of the selective acknowledgment is assumed dead. No more SAck are
        // sent until a new message is received from this node.
        // If _ui8MaxTimeOfLostRetransmissions == 0, SAck messages are always
        // sent.
        if ((_ui8MaxTimeOfLostRetransmissions == 0) ||
            (getTimeInMilliseconds () - pMQ->_i64LastMsgRcvdTime) <
            (_ui8MaxTimeOfLostRetransmissions *_ui32RetransmissionTime)) {
            BufferWriter bw (ui32MaxLength, 0);
            pMQ->_sAcks.write (&bw, ui32MaxLength);
            ui32Lentgh = bw.getBufferLength();
            return bw.relinquishBuffer();
        }
    }
    ui32Lentgh = 0;
    return NULL;
}

void * Reassembler::getAllSacks (uint32 ui32MaxLength, uint32 &ui32Lentgh)
{
    char *pBuf = new char[ui32MaxLength];
    char *pRet = pBuf;
    char *pSubBuf;
    uint32 ui32SubLentgh = 0;
    uint32 ui32CurrentMaxLen = ui32MaxLength;
    ui32Lentgh = 0;
    for (UInt32Hashtable<MsgQueue>::Iterator i = _msgsBySourceAddress.getAllElements(); !i.end(); i.nextElement()) {
        ui32CurrentMaxLen -= ui32SubLentgh;
        pSubBuf = (char *)getSacks (i.getKey(), ui32CurrentMaxLen, ui32SubLentgh);
        // add the last SAck retrieved to the buffer
        memcpy (pBuf, pSubBuf, ui32SubLentgh);
        // shift the pointer to position the next SAck will be appended
        pBuf += ui32SubLentgh;
        // increment the current length
        ui32Lentgh += ui32SubLentgh;
    }
    return pRet;
}

NetworkMessage * Reassembler::pop (uint32 ui32SourceAddress)
{
    _m.lock();
    const char * const pszMethodName = "Reassembler::pop";
    bool bSkipToNextCompleteMsg = true;
    uint16 ui16FirstSeqId;
    uint16 ui16LatestSeqId;
    uint32 ui32MetaDataLen;
    uint32 ui32DataLen;
    bool bIsEncrypted = false;

    MsgQueue *pMsgQueue = _msgsBySourceAddress.get (ui32SourceAddress);
    if (!pMsgQueue) {
        _m.unlock();
        return NULL;
    }

    NetworkMessage *pNetMsg = NULL;
    pMsgQueue->_msgs.resetGet();
    for (MsgWrapper *pMsgWrap = pMsgQueue->_msgs.getFirst(); pMsgWrap; pMsgWrap = pMsgQueue->_msgs.getNext()) {
        pNetMsg = pMsgWrap->_pNetMsg;
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug,
                        "found msg %u\n", pNetMsg->getMsgId());
        switch (pNetMsg->getChunkType()) {
            case NetworkMessage::CT_DataMsgComplete:
            {
                NetworkMessage *pRet = (pMsgQueue->_msgs.remove(pMsgWrap))->_pNetMsg;
                _m.unlock();
                return pRet;
            }
            case NetworkMessage::CT_DataMsgStart:
            {
                bSkipToNextCompleteMsg = false;
                ui16FirstSeqId = pNetMsg->getMsgId();
                ui16LatestSeqId = ui16FirstSeqId;
                ui32MetaDataLen = pNetMsg->getMetaDataLen();
                ui32DataLen = pNetMsg->getMsgLen();
                bIsEncrypted = pNetMsg->isEncrypted();
                break;
            }
            case NetworkMessage::CT_DataMsgInter:
            {
                if (bSkipToNextCompleteMsg) {
                    continue;
                }
                if (!isNextSequenceId (ui16LatestSeqId, pNetMsg->getMsgId())) {
                    if (_bSequenced) {
                        _m.unlock();
                        return NULL;
                    }
                    bSkipToNextCompleteMsg = true;
                }
                else {
                    ui16LatestSeqId = pNetMsg->getMsgId();
                    ui32MetaDataLen += pNetMsg->getMetaDataLen();
                    ui32DataLen += pNetMsg->getMsgLen();
                }
                break;
            }
            case NetworkMessage::CT_DataMsgEnd:
            {
                if (bSkipToNextCompleteMsg) {
                    continue;
                }
                if (!isNextSequenceId (ui16LatestSeqId, pNetMsg->getMsgId())) {
                    bSkipToNextCompleteMsg = true;
                    if (_bSequenced) {
                        _m.unlock();
                        return NULL;
                    }
                }
                // if it arrives here, it means that the message is complete
                ui16LatestSeqId = pNetMsg->getMsgId();
                ui32MetaDataLen += pNetMsg->getMetaDataLen();
                ui32DataLen += pNetMsg->getMsgLen();

                // reassemble and return it
                NetworkMessage *pRet = reassemble (pMsgQueue, ui16FirstSeqId,
                                                   ui16LatestSeqId, ui32MetaDataLen,
                                                   ui32DataLen);
                if ((pRet != NULL) && (bIsEncrypted)) {
                    pRet->setEncrypted();
                }
                _m.unlock();
                return pRet;
            }
        }
    }

    _m.unlock();
    return NULL;
}

int Reassembler::push (uint32 ui32SourceAddress, NetworkMessage *pNetMsg)
{
    if (pNetMsg == NULL) {
        return -1;
    }
    const uint16 ui16MsgId = pNetMsg->getMsgId();

    _m.lock();
    MsgQueue *pMsgQueue = _msgsBySourceAddress.get (ui32SourceAddress);
    if (pMsgQueue == NULL) {
        pMsgQueue = new MsgQueue();
        if (pMsgQueue == NULL) {
            _m.unlock();
            return -1;
        }
        pMsgQueue->_ui16SessionId = pNetMsg->getSessionId();
        pMsgQueue->_sAcks.setCumulativeTSN (ui16MsgId);
        _msgsBySourceAddress.put (ui32SourceAddress, pMsgQueue);
    }
    else if (pMsgQueue->_ui16SessionId != pNetMsg->getSessionId()) {
        pMsgQueue->_sAcks.reset();
        pMsgQueue->_ui16SessionId = pNetMsg->getSessionId();
        pMsgQueue->_sAcks.setCumulativeTSN (ui16MsgId);
    }

    pMsgQueue->_i64LastMsgRcvdTime = getTimeInMilliseconds();

    MsgWrapper *pMsgWrap = new MsgWrapper (pNetMsg, getTimeInMilliseconds());
    if (pMsgWrap != NULL) {
        MsgWrapper *pMsgWrapTmp = (pMsgQueue->_msgs).search (pMsgWrap);
        if (pMsgWrapTmp != NULL) {
            //  only update the arrival time
            pMsgWrapTmp->_ui64ArrivalTime = getTimeInMilliseconds();
            delete pMsgWrap;     // This deletes the inner pNetMsg also!
            pMsgWrap = NULL;
            _m.unlock ();
            return 1;
        }
        // add Message in the queue and update the received TSN list
        (pMsgQueue->_msgs).insert (pMsgWrap);
        checkAndLogMsg ("Reassembler::push", Logger::L_Info,
                        "adding message id %d to the SAck list\n", (int)ui16MsgId);
        (pMsgQueue->_sAcks).addTSN (ui16MsgId);
        _m.unlock();
        return 0;
    }

    _m.unlock();
    return -1;
}

int Reassembler::refresh (uint32 ui32SourceAddress)
{
    _m.lock();
    MsgQueue *pMsgQueue = _msgsBySourceAddress.get (ui32SourceAddress);
    if (pMsgQueue != NULL) {
        pMsgQueue->_i64LastMsgRcvdTime = getTimeInMilliseconds();
    }
    _m.unlock();
    return 0;
}

NetworkMessage * Reassembler::reassemble (MsgQueue *pMsgQueue, uint16 ui16FirstSeqId,
                                          uint16 ui16LatestSeqId, uint32 ui32MetaDataLen,
                                          uint32 ui32DataLen)
{
    // Find the start message
    MsgWrapper *pMsgWrapper = NULL;
    pMsgQueue->_msgs.resetGet();
    for (pMsgWrapper = pMsgQueue->_msgs.getFirst(); pMsgWrapper; pMsgWrapper = pMsgQueue->_msgs.getNext()) {
        if (pMsgWrapper->_pNetMsg->getMsgId() == ui16FirstSeqId) {
            break;
        }
    }

    if (pMsgWrapper == NULL) {
        return NULL;
    }

    uint32 ui32Offset = 0;
    void *pMetaData = malloc (ui32MetaDataLen);
    if (pMetaData == NULL) {
        return NULL;
    }

    void *pData = malloc (ui32DataLen);
    if (pData == NULL) {
        // no memory available
        free (pMetaData);
        pMetaData = NULL;
        return NULL;
    }

    uint16 ui16Len;
    MsgWrapper *pMsgWrapperTmp = NULL;

    // copy the MetaData
    do {
        // copy the MetaData
        ui16Len = pMsgWrapper->_pNetMsg->getMetaDataLen();
        if (ui16Len > 0) {
            memcpy (((char *)pMetaData) + ui32Offset, pMsgWrapper->_pNetMsg->getMetaData(), ui16Len);
            ui32Offset += ui16Len;
        }

        if (pMsgWrapper->_pNetMsg->getMsgLen() > 0) {
            // NOTE: because of the way the fragmentation is implemented, the
            // first fragments contain MetaData only, the final fragments only
            // parts of the Data.
            // There may be a case of a fragment that contains both MetaData and
            // Data. There is at most one fragment the contains both MetaData
            // and this is the case.
            break;
        }

        pMsgWrapperTmp = pMsgQueue->_msgs.getNext();
        pMsgQueue->_msgs.remove (pMsgWrapper);
        delete pMsgWrapper;
        pMsgWrapper = pMsgWrapperTmp;
    } while (pMsgWrapper && (pMsgWrapper->_pNetMsg->getMsgId() <= ui16LatestSeqId));

    ui32Offset = 0;

    // copy the Data
    NetworkMessage *pRet = NULL;
    do {
        // copy the Data
        ui16Len = pMsgWrapper->_pNetMsg->getMsgLen();
        if (ui16Len > 0) {
            memcpy (((char *)pData) + ui32Offset, pMsgWrapper->_pNetMsg->getMsg(), ui16Len);
            ui32Offset += ui16Len;
        }

        if (ui32Offset == ui32DataLen) {
            NetworkMessage *pNetMsg = pMsgWrapper->_pNetMsg;
             // Create a message of version 1 (without queue length) regardless of
             // the version of the original message. we don't need the queue length
             // info anymore at this point. This also solves the problem of
             // deciding which queue length to use, since the message is the
             // "fusion" of various messages
            pRet = MessageFactory::createNetworkMessageFromFields (pNetMsg->getMsgType(), pNetMsg->getSourceAddr(),
                                                                   pNetMsg->getDestinationAddr(), pNetMsg->getSessionId(),
                                                                   pNetMsg->getMsgId(), pNetMsg->getHopCount(),
                                                                   pNetMsg->getTTL(),
                                                                   (NetworkMessage::ChunkType) pNetMsg->getChunkType(),
                                                                   pNetMsg->isReliableMsg(), pMetaData, ui32MetaDataLen,
                                                                   pData, ui32DataLen, 1);
            if (pNetMsg->isEncrypted()) {
                pRet->setEncrypted();
            }
        }

        pMsgWrapperTmp = pMsgQueue->_msgs.getNext();
        pMsgQueue->_msgs.remove (pMsgWrapper);
        delete pMsgWrapper;
        pMsgWrapper = pMsgWrapperTmp;
    } while (pMsgWrapper && (pMsgWrapper->_pNetMsg->getMsgId() <= ui16LatestSeqId));

    if (pRet == NULL) {
        // free temporary buffer
        free (pMetaData);
        pMetaData = NULL;
        free (pData);
        pData = NULL;
    }
    return pRet;
}

//==============================================================================
// MsgWrapper
//==============================================================================

Reassembler::MsgWrapper::MsgWrapper (NetworkMessage *pNetMsg, uint64 ui64ArrivalTime)
{
    _pNetMsg = pNetMsg;
    _ui64ArrivalTime = ui64ArrivalTime;
}

Reassembler::MsgWrapper::~MsgWrapper()
{
    delete _pNetMsg;
    _pNetMsg = NULL;
}

bool Reassembler::MsgWrapper::operator == (const MsgWrapper &rhsMsgWrapper) const
{
    return ((_pNetMsg->getSourceAddr() == rhsMsgWrapper._pNetMsg->getSourceAddr()) &&
            (_pNetMsg->getMsgId() == rhsMsgWrapper._pNetMsg->getMsgId()));
}

bool Reassembler::MsgWrapper::operator > (const MsgWrapper &rhsMsgWrapper) const
{
    return ((_pNetMsg->getSourceAddr() == rhsMsgWrapper._pNetMsg->getSourceAddr()) &&
             SequentialArithmetic::greaterThan(_pNetMsg->getMsgId(), rhsMsgWrapper._pNetMsg->getMsgId()));
}

bool Reassembler::MsgWrapper::operator < (const MsgWrapper &rhsMsgWrapper) const
{
    return ((_pNetMsg->getSourceAddr() == rhsMsgWrapper._pNetMsg->getSourceAddr()) &&
            SequentialArithmetic::lessThan(_pNetMsg->getMsgId(), rhsMsgWrapper._pNetMsg->getMsgId()));
}

//==============================================================================
// MsgQueue
//==============================================================================

Reassembler::MsgQueue::MsgQueue()
    : _msgs (false)
{
    _ui16SessionId = 0;
}

Reassembler::MsgQueue::~MsgQueue()
{
    MsgWrapper *pMsgWrap = _msgs.getFirst();
    MsgWrapper *pMsgWrapperTmp;
    while (pMsgWrap) {
        pMsgWrapperTmp = _msgs.getNext();
        _msgs.remove(pMsgWrap);
        pMsgWrap = pMsgWrapperTmp;
    }
}

