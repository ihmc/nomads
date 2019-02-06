/*
 * PacketProcessor.cpp
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

#include "PacketProcessor.h"

#include "DataBuffer.h"
#include "Mocket.h"
#include "Receiver.h"
#include "SequencedPacketQueue.h"
#include "UnsequencedPacketQueue.h"
#include "TSNRangeHandler.h"

#include "Logger.h"

#include <memory.h>


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

PacketProcessor::PacketProcessor (Mocket *pMocket)
    : _cv (&_m)
{
    setName ("PacketProcessor");
    _pMocket = pMocket;

    // The following four will be initialized in the init() method
    _pReceiver = nullptr;
    _pControlPacketQueue = nullptr;
    _pReliableSequencedPacketQueue = nullptr;
    _pUnreliableSequencedPacketQueue = nullptr;
    _pReliableUnsequencedPacketQueue = nullptr;
    _pReliableUnsequencedPacketTracker = nullptr;
    _pUnreliableUnsequencedPacketQueue = nullptr;

    _ui32NextControlPacketTSN = pMocket->getStateCookie()->getControlTSNZ();
    _ui32NextReliableSequencedPacketTSN = pMocket->getStateCookie()->getReliableSequencedTSNZ();
    _ui32NextUnreliableSequencedPacketTSN = pMocket->getStateCookie()->getUnreliableSequencedTSNZ();

    _pReliableSequencedFragments = nullptr;
    _pUnreliableSequencedFragments = nullptr;
    _pReliableUnsequencedFragments = nullptr;
    _pUnreliableUnsequencedFragments = nullptr;
    _pReliableUnsequencedPacketQueue = new UnsequencedPacketQueue (true);
    _pReliableUnsequencedPacketTracker = new ReceivedTSNRangeHandler();
    _pUnreliableUnsequencedPacketQueue = new UnsequencedPacketQueue (false);
}

PacketProcessor::~PacketProcessor (void)
{
    // NOTE: deleteFragments() calls doneWithPacket() which in turn calls a method on the the receiver
    //       Therefore, the packet processor must be deleted before the receiver
    if (_pReliableSequencedFragments) {
        deleteFragments (_pReliableSequencedFragments);
        delete _pReliableSequencedFragments;
        _pReliableSequencedFragments = nullptr;
    }
    if (_pUnreliableSequencedFragments) {
        deleteFragments (_pUnreliableSequencedFragments);
        delete _pUnreliableSequencedFragments;
        _pUnreliableSequencedFragments = nullptr;
    }
    if (_pReliableUnsequencedFragments) {
        deleteFragments (_pReliableUnsequencedFragments);
        delete _pReliableUnsequencedFragments;
        _pReliableUnsequencedFragments = nullptr;
    }
    if (_pUnreliableUnsequencedFragments) {
        deleteFragments (_pUnreliableUnsequencedFragments);
        delete _pUnreliableUnsequencedFragments;
        _pUnreliableUnsequencedFragments = nullptr;
    }
    if (_pReliableUnsequencedPacketQueue) {
        delete _pReliableUnsequencedPacketQueue;
        _pReliableUnsequencedPacketQueue = nullptr;
    }
    if (_pReliableUnsequencedPacketTracker) {
        delete _pReliableUnsequencedPacketTracker;
        _pReliableUnsequencedPacketTracker = nullptr;
    }
    if (_pUnreliableUnsequencedPacketQueue) {
        delete _pUnreliableUnsequencedPacketQueue;
        _pUnreliableUnsequencedPacketQueue = nullptr;
    }
}

int PacketProcessor::init (void)
{
    _pReceiver = _pMocket->getReceiver();
    _pControlPacketQueue = _pReceiver->getControlPacketQueue();
    _pReliableSequencedPacketQueue = _pReceiver->getReliableSequencedPacketQueue();
    _pUnreliableSequencedPacketQueue = _pReceiver->getUnreliableSequencedPacketQueue();
    return 0;
}

int PacketProcessor::reinitAfterDefrost (void)
{
    _pReceiver = _pMocket->getReceiver();
    _pControlPacketQueue = _pReceiver->getControlPacketQueue();
    _pReliableSequencedPacketQueue = _pReceiver->getReliableSequencedPacketQueue();
    _pUnreliableSequencedPacketQueue = _pReceiver->getUnreliableSequencedPacketQueue();
    return 0;
}

void PacketProcessor::packetArrived (void)
{
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
}

int PacketProcessor::processReliableUnsequencedPacket (Packet *pPacket)
{
    uint32 ui32SeqNum = pPacket->getSequenceNum();
    if (_pReliableUnsequencedPacketTracker->alreadyReceived (ui32SeqNum)) {
        // A packet with this sequence number has already been received and processed, so this is a duplicate - ignore
        checkAndLogMsg ("PacketProcessor::processReliableUnsequencedPacket", Logger::L_MediumDetailDebug,
                        "already received packet with sequence number %u\n", ui32SeqNum);
        dequeuedPacket (pPacket);
        _pMocket->getStatistics()->_ui32DuplicatedDiscardedPackets++;
        return 0;
    }
    if (!pPacket->isFragment()) {
        int rc = processPacket (pPacket);
        if (rc == 0) {
            _pReliableUnsequencedPacketTracker->addTSN (ui32SeqNum);
        }
        return rc;
    }
    else {
        // This is not a complete packet, it is a fragment
        uint32 ui32StartingPacketCount = _pReliableUnsequencedPacketQueue->getPacketCount();
        _pReliableUnsequencedFragments = _pReliableUnsequencedPacketQueue->insert (pPacket);
        dequeuedPacket (pPacket);
        _pReliableUnsequencedPacketTracker->addTSN (ui32SeqNum);
        if (_pReliableUnsequencedFragments != nullptr) {
            // The fragment added completed a packet, all the fragments are in _pReliableUnsequencedFragments now deliver them
            deliverFragments (_pReliableUnsequencedFragments);
            _pReliableUnsequencedFragments = nullptr;
            _pReliableUnsequencedPacketTracker->addTSN (ui32SeqNum);
        }
        else {
            if (_pReliableUnsequencedPacketQueue->getPacketCount() <= ui32StartingPacketCount) {
                // The packet was not inserted into the packet queue, so it must have been a duplicate
                _pMocket->getStatistics()->_ui32DuplicatedDiscardedPackets++;
            }
        }
    }
    return 0;
}

int PacketProcessor::processUnreliableSequencedPacketWithoutBuffering (Packet *pPacket)
{
    //printf ("PacketProcessor: waiting packet sequence number %d received %d\n", _ui32NextUnreliableSequencedPacketTSN, pPacket->getSequenceNum());
    // Need to handle reassembly here
    if (_ui32NextUnreliableSequencedPacketTSN > pPacket->getSequenceNum()) {
        // Drop this packet - it must have arrived out of order
        checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                        "dropping packet with TSN %lu, which is smaller than the next expected TSN of %lu\n",
                        pPacket->getSequenceNum(), _ui32NextUnreliableSequencedPacketTSN);
        dequeuedPacket (pPacket);
        delete pPacket;
        return 0;
    }
    if (_ui32NextUnreliableSequencedPacketTSN < pPacket->getSequenceNum()) {
        // Lost one or more packets - delete any assembled fragments before proceeding
        if (_pUnreliableSequencedFragments) {
            uint16 ui16Count = deleteFragments (_pUnreliableSequencedFragments);
            delete _pUnreliableSequencedFragments;
            _pUnreliableSequencedFragments = nullptr;
            _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
            checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                            "deleted message fragments because of missing packets; next expected TSN is %lu; received packet TSN is %lu\n",
                            _ui32NextUnreliableSequencedPacketTSN, pPacket->getSequenceNum());
        }
        _ui32NextUnreliableSequencedPacketTSN = pPacket->getSequenceNum();
    }

    if (!deliveryPrerequisitesSatisfied (pPacket)) {
        // The delivery prerequisites for this packet have not been satisfied
        // Since there is no buffering, just throw the packet away
        checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                        "dropping packet with TSN %lu because its delivery prerequisites have not been satisfied\n",
                        pPacket->getSequenceNum());
        // Since the packet has essentially been skipped over, need to delete any fragments also
        if (_pUnreliableSequencedFragments) {
            uint16 ui16Count = deleteFragments (_pUnreliableSequencedFragments);
            delete _pUnreliableSequencedFragments;
            _pUnreliableSequencedFragments = nullptr;
            _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
            checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                            "deleted message fragments because packet with sequence number %lu was dropped\n",
                            pPacket->getSequenceNum());
        }
        _ui32NextUnreliableSequencedPacketTSN = pPacket->getSequenceNum() + 1;
        dequeuedPacket (pPacket);
        delete pPacket;
        return 0;
    }

    // Now proceed with processing the packet
    if (!pPacket->isFragment()) {
        // This packet is not a fragment - contains a whole message
        if (_pUnreliableSequencedFragments) {
            // Problem - this is not a fragment but there are previous fragments waiting to be assembled
            checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                            "packet %lu is not a fragment but there are other fragments waiting to be reassembled\n",
                            pPacket->getSequenceNum());
            uint16 ui16Count = deleteFragments (_pUnreliableSequencedFragments);
            delete _pUnreliableSequencedFragments;
            _pUnreliableSequencedFragments = nullptr;
            _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
        }
        int rc;
        if (0 != (rc = processPacket (pPacket))) {
            checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_MildError,
                            "processPacket failed with rc = %d\n", rc);
        }
        _ui32NextUnreliableSequencedPacketTSN++;
    }
    else if (pPacket->isFirstFragment()) {
        if (_pUnreliableSequencedFragments) {
            // Problem - there is already a set of message fragments but this packet claims to be the first fragment
            checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                            "packet %lu is first fragment but already have fragments waiting to be reassembled\n",
                            pPacket->getSequenceNum());
            uint16 ui16Count = deleteFragments (_pUnreliableSequencedFragments);
            delete _pUnreliableSequencedFragments;
            _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
        }
        _pUnreliableSequencedFragments = new LList<Packet*>;
        _pUnreliableSequencedFragments->add (pPacket);
        dequeuedPacket (pPacket);
        _ui32NextUnreliableSequencedPacketTSN++;
    }
    else if (pPacket->isLastFragment()) {
        if (_pUnreliableSequencedFragments) {
            // Have the last fragment for this message
            // Reassemble the message and deliver it
            _pUnreliableSequencedFragments->add (pPacket);
            dequeuedPacket (pPacket);
            _ui32NextUnreliableSequencedPacketTSN++;
            deliverFragments (_pUnreliableSequencedFragments);
            // NOTE: Do not delete _pUnreliableSequencedFragments as the list is stored in the DataBuffer
            // by deliverFragments() and will be deleted after the application has received the data
            _pUnreliableSequencedFragments = nullptr;
        }
        else {
            // Problem - this is the last fragment but there are no previous fragments to assemble
            checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                            "packet %lu is last fragment but there are no fragments waiting to be reassembled\n",
                            pPacket->getSequenceNum());
            dequeuedPacket (pPacket);
            delete pPacket;
            _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets++;
            _ui32NextUnreliableSequencedPacketTSN++;
        }
    }
    else if (pPacket->isIntermediateFragment()) {
        if (_pUnreliableSequencedFragments) {
            _pUnreliableSequencedFragments->add (pPacket);
            dequeuedPacket (pPacket);
            _ui32NextUnreliableSequencedPacketTSN++;
        }
        else {
            // Problem - this is a fragment but there are no previous fragments to assemble
            checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                            "packet %lu is a fragment but there are no fragments waiting to be reassembled\n",
                            pPacket->getSequenceNum());
            dequeuedPacket (pPacket);
            delete pPacket;
            _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets++;
            _ui32NextUnreliableSequencedPacketTSN++;
        }
    }
    else {
        checkAndLogMsg ("PacketProcessor::processUnreliableSequencedPacketWithoutBuffering", Logger::L_LowDetailDebug,
                        "implementation error - packet %d is neither a fragment nor not a fragment\n",
                        pPacket->getSequenceNum());
    }
    return 0;
}

int PacketProcessor::processUnreliableUnsequencedPacket (Packet *pPacket)
{
    if (!pPacket->isFragment()) {
        return processPacket (pPacket);
    }
    else {
        // This is not a complete packet, it is a fragment
        //printf ("PacketProcessor::processUnreliableUnsequencedPacket insert packet with sequence number %d\n", pPacket->getSequenceNum());
        _pUnreliableUnsequencedFragments = _pUnreliableUnsequencedPacketQueue->insert(pPacket);
        dequeuedPacket (pPacket);
        if (_pUnreliableUnsequencedFragments != nullptr) {
            //printf ("PacketProcessor::processUnreliableUnsequencedPacket Deliver fragments\n");
            deliverFragments(_pUnreliableUnsequencedFragments);
            _pUnreliableUnsequencedFragments = nullptr;
        }
        else {
            //printf ("PacketProcessor::processUnreliableUnsequencedPacket Did not fill fragments list\n");
        }
    }
    return 0;
}

int PacketProcessor::processPacket (Packet *pPacket)
{
    bool bFoundDataChunk = false;
    pPacket->resetChunkIterator();
    while (true) {
        switch (pPacket->getChunkType()) {
            case Packet::CT_Shutdown:
                checkAndLogMsg ("PacketProcessor::processPacket", Logger::L_MediumDetailDebug, "received Shutdown\n");
                _pMocket->getStateMachine()->receivedShutdown();
                _pMocket->notifyTransmitter();
                break;

            case Packet::CT_ShutdownAck:
                checkAndLogMsg ("PacketProcessor::processPacket", Logger::L_MediumDetailDebug, "received ShutdownAck\n");
                _pMocket->getStateMachine()->receivedShutdownAck();
                _pMocket->notifyTransmitter();
                break;

            case Packet::CT_ShutdownComplete:
                checkAndLogMsg ("PacketProcessor::processPacket", Logger::L_MediumDetailDebug, "received ShutdownComplete\n");
                _pMocket->getStateMachine()->receivedShutdownComplete();
                _pMocket->notifyTransmitter();
                _pMocket->notifyPacketProcessor();
                break;

            case Packet::CT_Data:
            {
                uint32 ui32SequenceNum = pPacket->getSequenceNum();
                DataChunkAccessor accessor = pPacket->getDataChunk();
                _pMocket->getStatistics()->_ui32ReceivedBytes += accessor.getDataLength();
                updateMessageStatistics (pPacket->isReliablePacket(), pPacket->isSequencedPacket(), pPacket->getTagId());
                DataBuffer *pBuffer = new DataBuffer (this, pPacket, accessor.getDataLength());
                _receivedDataQueue.insert (pBuffer);       // NOTE: Should not do anything else with pPacket in this thread at this point
                bFoundDataChunk = true;
                checkAndLogMsg ("PacketProcessor::processPacket", Logger::L_MediumDetailDebug,
                                "delivered data chunk from packet with sequence number %lu\n",
                                ui32SequenceNum);
                break;
            }
        }
        if ((bFoundDataChunk) || (!pPacket->advanceToNextChunk())) {      // NOTE: It is important to check if a data chunk has been found first
            break;                                                        // If so, pPacket has been passed off to the receivedDataQueue, which
        }                                                                 // will delete pPacket when done in a separate thread
    }
    if (!bFoundDataChunk) {
        dequeuedPacket (pPacket);
        delete pPacket;
    }
    return 0;
}

int PacketProcessor::getNextMessageSize (int64 i64Timeout)
{
    _mReceive.lock();

    // Check if the connection has been closed
    if ((_receivedDataQueue.isEmpty()) && (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_CLOSED)) {
        _mReceive.unlock();
        return -1;
    }

    if (i64Timeout == 0) {
        i64Timeout = _pMocket->getReceiveTimeout();
    }
    DataBuffer *pBuffer = _receivedDataQueue.peek (i64Timeout);
    if (pBuffer == nullptr) {
        // If no data was returned, it could be because the connection has been closed
        // In that case, we want to return -1 and not 0
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_CLOSED) {
            _mReceive.unlock();
            return -1;
        }
        else {
            _mReceive.unlock();
            return 0;
        }
    }

    uint32 ui32MessageSize = pBuffer->getMessageSize();
    _mReceive.unlock();
    return ui32MessageSize;
}

uint32 PacketProcessor::getCumulativeSizeOfAvailableMessages (void)
{
    return _receivedDataQueue.getCumulativeMessageSize();
}

int PacketProcessor::receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout)
{
    _mReceive.lock();

    // Check if the connection has been closed
    if ((_receivedDataQueue.isEmpty()) && (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_CLOSED)) {
        _mReceive.unlock();
        return -1;
    }

    if (i64Timeout == 0) {
        i64Timeout = _pMocket->getReceiveTimeout();
    }
    DataBuffer *pBuffer = _receivedDataQueue.extract (i64Timeout);
    if (pBuffer == nullptr) {
        // If no data was returned, it could be because the connection has been closed
        // In that case, we want to return -1 and not 0
        const auto smCurrentState = _pMocket->getStateMachine()->getCurrentState();
        if ((smCurrentState == StateMachine::S_CLOSED) || (smCurrentState == StateMachine::S_APPLICATION_ABORT)) {
            _mReceive.unlock();
            return -1;
        }
        else {
            _mReceive.unlock();
            return 0;
        }
    }

    if (pBuffer->fragmentedMessage()) {
        Packet *pPacket;
        LList<Packet*> *pFragments = pBuffer->getFragments();
        pFragments->resetGet();
        uint32 ui32ByteCount = 0;
        while (pFragments->getNext (pPacket)) {
            uint32 ui32SpaceRemaining = ui32BufSize - ui32ByteCount;
            pPacket->resetChunkIterator();
            if (pPacket->getChunkType() == Packet::CT_Data) {       // NOTE: Assumes that the first chunk is the data chunk
                DataChunkAccessor dca = pPacket->getDataChunk();
                uint32 ui32BytesToCopy = dca.getDataLength();
                if (ui32BytesToCopy > ui32SpaceRemaining) {
                    ui32BytesToCopy = ui32SpaceRemaining;
                }
                memcpy (((char*)pBuf)+ui32ByteCount, dca.getData(), ui32BytesToCopy);
                ui32ByteCount += ui32BytesToCopy;
                ui32SpaceRemaining -= ui32BytesToCopy;
                if (ui32SpaceRemaining == 0) {
                    break;
                }
            }
        }
        delete pBuffer;
        _mReceive.unlock();
        return ui32ByteCount;
    }
    else {
        Packet *pPacket = pBuffer->getPacket();
        // The packet delivered is a full message thus reduce the enqueued data size (this increases the local window size)
        dequeuedPacket (pPacket);
        pPacket->resetChunkIterator();
        if (pPacket->getChunkType() == Packet::CT_Data) {           // NOTE: Assumes that the first chunk is the data chunk
            DataChunkAccessor dca = pPacket->getDataChunk();
            uint32 ui32BytesToCopy = dca.getDataLength();
            if (ui32BytesToCopy > ui32BufSize) {
                ui32BytesToCopy = ui32BufSize;
            }
            memcpy (pBuf, dca.getData(), ui32BytesToCopy);
            delete pBuffer;      // This will delete the packet (and deallocate memory) as well as adjust the queued data size
            _mReceive.unlock();
            return (int) ui32BytesToCopy;
        }
        else {
            _mReceive.unlock();
            return 0;
        }
    }
}

int PacketProcessor::sreceive (int64 i64Timeout, void *pBuf1, uint32 ui32BufSize1, va_list valist)
{
    _mReceive.lock();

    // Check if the connection has been closed
    if ((_receivedDataQueue.isEmpty()) && (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_CLOSED)) {
        _mReceive.unlock();
        return -1;
    }

    if (i64Timeout == 0) {
        i64Timeout = _pMocket->getReceiveTimeout();
    }
    DataBuffer *pBuffer = _receivedDataQueue.extract (i64Timeout);
    if (pBuffer == nullptr) {
        // If no data was returned, it could be because the connection has been closed
        // In that case, we want to return -1 and not 0
        const auto smCurrentState = _pMocket->getStateMachine()->getCurrentState();
        if ((smCurrentState == StateMachine::S_CLOSED) || (smCurrentState == StateMachine::S_APPLICATION_ABORT)) {
            _mReceive.unlock();
            return -1;
        }
        else {
            _mReceive.unlock();
            return 0;
        }
    }

    void *pRecvBuf = pBuf1;
    uint32 ui32CurrentBufSize = ui32BufSize1;
    uint32 ui32TotalBytesCopied = 0;
    uint32 ui32BytesCopiedIntoCurrentBuf = 0;
    bool bRecvBuffersFull = false;
    if (pBuffer->fragmentedMessage()) {
        Packet *pPacket;
        LList<Packet*> *pFragments = pBuffer->getFragments();
        pFragments->resetGet();
        while (pFragments->getNext (pPacket)) {
            pPacket->resetChunkIterator();
            if (pPacket->getChunkType() == Packet::CT_Data) {       // NOTE: Assumes that the first chunk is the data chunk
                DataChunkAccessor dca = pPacket->getDataChunk();
                uint32 ui32BytesInFragmentToCopy = dca.getDataLength();
                uint32 ui32BytesCopiedFromFragment = 0;
                while (ui32BytesInFragmentToCopy > 0) {
                    uint32 ui32SpaceRemainingInCurrentBuf = ui32CurrentBufSize - ui32BytesCopiedIntoCurrentBuf;
                    if (ui32SpaceRemainingInCurrentBuf == 0) {
                        // Advance to the next buffer
                        pRecvBuf = va_arg (valist, void*);
                        if (pRecvBuf != nullptr) {
                            ui32CurrentBufSize = va_arg (valist, uint32);
                            ui32BytesCopiedIntoCurrentBuf = 0;
                            ui32SpaceRemainingInCurrentBuf = ui32CurrentBufSize;
                        }
                        else {
                            // No more caller buffers left - return
                            bRecvBuffersFull = true;
                            break;
                        }
                    }
                    uint32 ui32BytesToCopyNow = ui32BytesInFragmentToCopy;
                    if (ui32BytesToCopyNow > ui32SpaceRemainingInCurrentBuf) {
                        ui32BytesToCopyNow = ui32SpaceRemainingInCurrentBuf;
                    }
                    memcpy (((char*)pRecvBuf)+ui32BytesCopiedIntoCurrentBuf, dca.getData(ui32BytesCopiedFromFragment), ui32BytesToCopyNow);
                    ui32BytesCopiedIntoCurrentBuf += ui32BytesToCopyNow;
                    ui32BytesInFragmentToCopy -= ui32BytesToCopyNow;
                    ui32BytesCopiedFromFragment += ui32BytesToCopyNow;
                    ui32TotalBytesCopied += ui32BytesToCopyNow;
                }
                if (bRecvBuffersFull) {
                    // Have run out of space in the caller's buffers
                    break;
                }
            }
        }
        delete pBuffer;
        _mReceive.unlock();
        return ui32TotalBytesCopied;
    }
    else {
        Packet *pPacket = pBuffer->getPacket();
        pPacket->resetChunkIterator();
        if (pPacket->getChunkType() == Packet::CT_Data) {           // NOTE: Assumes that the first chunk is the data chunk
            DataChunkAccessor dca = pPacket->getDataChunk();
            uint32 ui32BytesToCopy = dca.getDataLength();
            while (ui32BytesToCopy > 0) {
                uint32 ui32SpaceRemainingInCurrentBuf = ui32CurrentBufSize - ui32BytesCopiedIntoCurrentBuf;
                if (ui32SpaceRemainingInCurrentBuf == 0) {
                    // Advance to the next buffer
                    pRecvBuf = va_arg (valist, void*);
                    if (pRecvBuf != nullptr) {
                        ui32CurrentBufSize = va_arg (valist, uint32);
                        ui32BytesCopiedIntoCurrentBuf = 0;
                        ui32SpaceRemainingInCurrentBuf = ui32CurrentBufSize;
                    }
                    else {
                        // No more caller buffers left - return
                        bRecvBuffersFull = true;
                        break;
                    }
                }
                uint32 ui32BytesToCopyNow = ui32BytesToCopy;
                if (ui32BytesToCopyNow > ui32SpaceRemainingInCurrentBuf) {
                    ui32BytesToCopyNow = ui32SpaceRemainingInCurrentBuf;
                }
                memcpy (((char*)pRecvBuf)+ui32BytesCopiedIntoCurrentBuf, dca.getData(ui32TotalBytesCopied), ui32BytesToCopyNow);
                ui32BytesCopiedIntoCurrentBuf += ui32BytesToCopyNow;
                ui32SpaceRemainingInCurrentBuf -= ui32BytesToCopyNow;
                ui32BytesToCopy -= ui32BytesToCopyNow;
                ui32TotalBytesCopied += ui32BytesToCopyNow;
            }
            delete pBuffer;      // This will delete the packet (and deallocate memory) as well as adjust the queued data size
            _mReceive.unlock();
            return (int) ui32TotalBytesCopied;
        }
        else {
            _mReceive.unlock();
            return 0;
        }
    }
}

void PacketProcessor::run (void)
{
    bool bDone = false;

    _m.lock();
    while (!bDone) {
        bool bDequeuedPacket = false;
        if (tryToProcessFirstPacketFromControlQueue()) {
            bDequeuedPacket = true;
        }
        if (tryToProcessFirstPacketFromReliableSequencedQueue()) {
            bDequeuedPacket = true;
        }
        if (tryToProcessFirstPacketFromUnreliableSequencedQueue()) {
            bDequeuedPacket = true;
        }
        if (!bDequeuedPacket) {
            // If nothing has been dequeued, go to sleep until the next possible delivery timeout
            // Will also be woken up by the receiver calling packetArrived()
            /*!!*/ // Change the following so that there is a minimum wait time - right now getUnreliableSequencedDeliveryTimeout returns a constant time of 3000
            _cv.wait (_pMocket->getUnreliableSequencedDeliveryTimeout());
        }

        const auto smCurrentState = _pMocket->getStateMachine()->getCurrentState();
        if ((smCurrentState == StateMachine::S_CLOSED) || (smCurrentState == StateMachine::S_APPLICATION_ABORT) ||
            (smCurrentState == StateMachine::S_SUSPENDED)) {
            bDone = true;
        }
    }
    _m.unlock();
    _receivedDataQueue.close();
    _pMocket->packetProcessorTerminating();
}

int PacketProcessor::freeze (ObjectFreezer &objectFreezer)
{
    objectFreezer.beginNewObject ("PacketProcessor");
    objectFreezer.putUInt32 (_ui32NextControlPacketTSN);
    objectFreezer.putUInt32 (_ui32NextReliableSequencedPacketTSN);
    objectFreezer.putUInt32 (_ui32NextUnreliableSequencedPacketTSN);

/*    printf ("PacketProcessor\n");
    printf ("_ui32NextControlPacketTSN %lu\n", _ui32NextControlPacketTSN);
    printf ("_ui32NextReliableSequencedPacketTSN %lu\n", _ui32NextReliableSequencedPacketTSN);
    printf ("_ui32NextUnreliableSequencedPacketTSN %lu\n", _ui32NextUnreliableSequencedPacketTSN);*/

    // _pControlPacketQueue _pReliableSequencedPacketQueue _pUnreliableSequencedPacketQueue
    // are freezed in Receiver and then initialized with reinitAfterDefrost()

    if (0 != _pReliableUnsequencedPacketQueue->freeze (objectFreezer)) {
        // return -1 is if objectFreezer.endObject() does not end with success
        return -2;
    }

    if (0 != _pReliableUnsequencedPacketTracker->freeze (objectFreezer)) {
        return -3;
    }

    if (0 != _pUnreliableUnsequencedPacketQueue->freeze (objectFreezer)) {
        return -4;
    }

    if (0 != _receivedDataQueue.freeze (objectFreezer)) {
        return -5;
    }

    // Freeze linked list of fragments _pReliableSequencedFragments if it is instantiated
    if (0 != freezeLinkedList (_pReliableSequencedFragments, objectFreezer)) {
        return -6;
    }
    // Freeze linked list of fragments _pUnreliableSequencedFragments
    if (0 != freezeLinkedList (_pUnreliableSequencedFragments, objectFreezer)) {
        return -7;
    }
    // Freeze linked list of fragments _pReliableSequencedFragments if it is instantiated
    if (0 != freezeLinkedList (_pReliableUnsequencedFragments, objectFreezer)) {
        return -8;
    }
    // Freeze linked list of fragments _pUnreliableSequencedFragments
    if (0 != freezeLinkedList (_pUnreliableUnsequencedFragments, objectFreezer)) {
        return -9;
    }

    return objectFreezer.endObject();
}

int PacketProcessor::defrost (ObjectDefroster &objectDefroster)
{
    objectDefroster.beginNewObject ("PacketProcessor");
    objectDefroster >> _ui32NextControlPacketTSN;
    objectDefroster >> _ui32NextReliableSequencedPacketTSN;
    objectDefroster >> _ui32NextUnreliableSequencedPacketTSN;

/*    printf ("PacketProcessor\n");
    printf ("_ui32NextControlPacketTSN %lu\n", _ui32NextControlPacketTSN);
    printf ("_ui32NextReliableSequencedPacketTSN %lu\n", _ui32NextReliableSequencedPacketTSN);
    printf ("_ui32NextUnreliableSequencedPacketTSN %lu\n", _ui32NextUnreliableSequencedPacketTSN);*/

    if (0 != _pReliableUnsequencedPacketQueue->defrost (objectDefroster)) {
        return -2;
    }
    if (0 != _pReliableUnsequencedPacketTracker->defrost (objectDefroster)) {
        return -3;
    }
    if (0 != _pUnreliableUnsequencedPacketQueue->defrost (objectDefroster)) {
        return -4;
    }

    // Reconstruct _receivedDataQueue
    if (0 != _receivedDataQueue.defrost (objectDefroster, this)) {
        return -5;
    }

    // Reconstruct Linked list of fragments _pReliableSequencedFragments if it was instantiated
    unsigned char isLList;
    objectDefroster >> isLList;
    if (isLList) {
        _pReliableSequencedFragments = new LList<Packet*>;
        if (0 != defrostLinkedList (_pReliableSequencedFragments, objectDefroster)) {
            return -6;
        }
    }
/*    else {
        printf ("linked list not instantiated\n");
    }*/

    // Reconstruct Linked list of fragments _pUnreliableSequencedFragments if it was instantiated
    objectDefroster >> isLList;
    if (isLList) {
        _pUnreliableSequencedFragments = new LList<Packet*>;
        if (0 != defrostLinkedList (_pUnreliableSequencedFragments, objectDefroster)) {
            return -7;
        }
    }
/*    else {
        printf ("linked list not instantiated\n");
    }*/
    objectDefroster >> isLList;
    if (isLList) {
        _pReliableUnsequencedFragments = new LList<Packet*>;
        if (0 != defrostLinkedList (_pReliableUnsequencedFragments, objectDefroster)) {
            return -8;
        }
    }
/*    else {
        printf ("linked list not instantiated\n");
    }*/

    // Reconstruct Linked list of fragments _pUnreliableSequencedFragments if it was instantiated
    objectDefroster >> isLList;
    if (isLList) {
        _pUnreliableUnsequencedFragments = new LList<Packet*>;
        if (0 != defrostLinkedList (_pUnreliableUnsequencedFragments, objectDefroster)) {
            return -9;
        }
    }
/*    else {
        printf ("linked list not instantiated\n");
    }*/

    return objectDefroster.endObject();
}

void PacketProcessor::dequeuedPacket (Packet *pPacket)
{
    _pReceiver->decrementQueuedDataSize (pPacket->getPacketSize());
}

bool PacketProcessor::tryToProcessFirstPacketFromControlQueue (void)
{
    PacketWrapper *pWrapper = _pControlPacketQueue->peek();
    if (pWrapper == nullptr) {
        // Queue is empty
        return false;
    }

    Packet *pPacket = pWrapper->getPacket();

    if (pPacket == nullptr) {
        // This is a place holder for a cancelled packet
        if (_ui32NextControlPacketTSN == pWrapper->getSequenceNum()) {
            _pControlPacketQueue->remove (pWrapper);
            delete pWrapper;
            _ui32NextControlPacketTSN++;
            _pControlPacketQueue->setNextExpectedSequenceNum (_ui32NextControlPacketTSN);
            return true;
        }
    }
    else {
        if (_ui32NextControlPacketTSN == pPacket->getSequenceNum()) {
            if (deliveryPrerequisitesSatisfied (pPacket)) {
                int rc;
                if (0 != (rc = processPacket (pPacket))) {
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromQueue", Logger::L_MildError,
                                    "processPacket failed with rc = %d\n", rc);
                }
                else {
                    _pControlPacketQueue->remove (pWrapper);
                    delete pWrapper;    // No need to delete pPacket - processPacket would have taken care of that
                    _ui32NextControlPacketTSN++;
                    _pControlPacketQueue->setNextExpectedSequenceNum (_ui32NextControlPacketTSN);
                   return true;
                }
            }
        }
    }
    return false;
}

bool PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue (void)
{
    _pReliableSequencedPacketQueue->lock();
    PacketWrapper *pWrapper = _pReliableSequencedPacketQueue->peek();
    if (pWrapper == nullptr) {
        // Queue is empty
        _pReliableSequencedPacketQueue->unlock();
        return false;
    }

    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_MediumDetailDebug,
            "waiting packet sequence number %d peek returned packet %d\n", _ui32NextReliableSequencedPacketTSN, pWrapper->getSequenceNum());

    if (_ui32NextReliableSequencedPacketTSN != pWrapper->getSequenceNum()) {
        // We do not have the next packet to process
        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_MediumDetailDebug,
            "missing packets\n");
        _pReliableSequencedPacketQueue->unlock();
        return false;
    }

    _pReliableSequencedPacketQueue->remove (pWrapper);
    _ui32NextReliableSequencedPacketTSN++;
    _pReliableSequencedPacketQueue->setNextExpectedSequenceNum (_ui32NextReliableSequencedPacketTSN);
    _pReliableSequencedPacketQueue->unlock();


    Packet *pPacket = pWrapper->getPacket();

    if (pPacket == nullptr) {
        // This is a place holder for a cancelled packet
        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_MediumDetailDebug,
                        "packet %d is a place holder for a cancelled packet\n", pWrapper->getSequenceNum());
        if (_pReliableSequencedFragments) {
            // There are some saved message fragments
            // If this packet is being cancelled, the message cannot be reassembled
            uint16 ui16Count = deleteFragments (_pReliableSequencedFragments);
            delete _pReliableSequencedFragments;
            _pReliableSequencedFragments = nullptr;
            _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
            checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_Warning,
                            "deleted message fragments because a packet containing a fragment was cancelled\n");
        }
    }
    else {
        if (deliveryPrerequisitesSatisfied (pPacket)) {
            int rc;
            if (!pPacket->isFragment()) {
                // This packet is not a fragment - contains a whole message
                if (_pReliableSequencedFragments) {
                    // Problem - this is not a fragment but there are previous fragments waiting to be assembled
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_Warning,
                                    "packet %lu is not a fragment but there are other fragments waiting to be reassembled\n",
                                    pPacket->getSequenceNum());
                    uint16 ui16Count = deleteFragments (_pReliableSequencedFragments);
                    delete _pReliableSequencedFragments;
                    _pReliableSequencedFragments = nullptr;
                    _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
                }
                if (0 != (rc = processPacket (pPacket))) {
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_MildError,
                                    "processPacket failed with rc = %d\n", rc);
                }
                else {
                    delete pWrapper;    // No need to delete pPacket - processPacket would have taken care of that
                }
            }
            else if (pPacket->isFirstFragment()) {
                if (_pReliableSequencedFragments) {
                    // Problem - there is already a set of message fragments but this packet claims to be the first fragment
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_Warning,
                                    "packet %lu is first fragment but already have fragments waiting to be reassembled\n",
                                    pPacket->getSequenceNum());
                    uint16 ui16Count = deleteFragments (_pReliableSequencedFragments);
                    delete _pReliableSequencedFragments;
                    _pReliableSequencedFragments = nullptr;
                    _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
                }
                _pReliableSequencedFragments = new LList<Packet*>;
                _pReliableSequencedFragments->add (pPacket);
                dequeuedPacket (pPacket);
                delete pWrapper;    // No need to delete pPacket - it is in the list of fragments
            }
            else if (pPacket->isLastFragment()) {
                if (_pReliableSequencedFragments) {
                    // Have the last fragment for this message
                    // Reassemble the message and deliver it
                    _pReliableSequencedFragments->add (pPacket);
                    dequeuedPacket (pPacket);
                    delete pWrapper;    // No need to delete pPacket - it is in the list of fragments
                    deliverFragments (_pReliableSequencedFragments);
                    // NOTE: Do not delete _pReliableSequencedFragments as the list is stored in the DataBuffer
                    // by deliverFragments() and will be deleted after the application has received the data
                    _pReliableSequencedFragments = nullptr;
                }
                else {
                    // Problem - this is the last fragment but there are no previous fragments to assemble
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_Warning,
                                    "packet %lu is last fragment but there are no fragments waiting to be reassembled\n",
                                    pPacket->getSequenceNum());
                    dequeuedPacket (pPacket);
                    delete pPacket;
                    _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets++;
                    delete pWrapper;
                }
            }
            else if (pPacket->isIntermediateFragment()) {
                if (_pReliableSequencedFragments) {
                    _pReliableSequencedFragments->add (pPacket);
                    dequeuedPacket (pPacket);
                    delete pWrapper;    // No need to delete pPacket - it is in the list of fragments
                }
                else {
                    // Problem - this is a fragment but there are no previous fragments to assemble
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_Warning,
                                    "packet %lu is a fragment but there are no fragments waiting to be reassembled\n",
                                    pPacket->getSequenceNum());
                    dequeuedPacket (pPacket);
                    delete pPacket;
                    _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets++;
                    delete pWrapper;
                }
            }
            else {
                checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromReliableSequencedQueue", Logger::L_MildError,
                                "implementation error - packet %d is neither a fragment nor not a fragment\n",
                                pPacket->getSequenceNum());
            }
        }
    }
    return true;
}

bool PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue (void)
{
    _pUnreliableSequencedPacketQueue->lock();
    PacketWrapper *pWrapper = _pUnreliableSequencedPacketQueue->peek();
    if (pWrapper == nullptr) {
        // Queue is empty
        _pUnreliableSequencedPacketQueue->unlock();
        return false;
    }

    // Check if this is the packet I was expecting and if there are expired packets
    bool bExpectedPacket = false;
    bool bExpiredPackets = false;
    if (_ui32NextUnreliableSequencedPacketTSN == pWrapper->getSequenceNum()) {
        _ui32NextUnreliableSequencedPacketTSN++;
        bExpectedPacket = true;
    }
    if (checkForUnreliablePacketTimeout (_ui32NextUnreliableSequencedPacketTSN, pWrapper->getSequenceNum(), pWrapper->getLastIOTime())) {
        _ui32NextUnreliableSequencedPacketTSN = pWrapper->getSequenceNum() + 1;
        bExpiredPackets = true;
    }

    if ((bExpectedPacket == false) && (bExpiredPackets == false)) {
        // nothing to be done
        _pUnreliableSequencedPacketQueue->unlock();
        return false;
    }

    _pUnreliableSequencedPacketQueue->remove (pWrapper);
    _pUnreliableSequencedPacketQueue->setNextExpectedSequenceNum (_ui32NextUnreliableSequencedPacketTSN);
    _pUnreliableSequencedPacketQueue->unlock();


    Packet *pPacket = pWrapper->getPacket();
    delete pWrapper;
    if (pPacket == nullptr) {
        // This is a place holder for a cancelled packet
        if (bExpectedPacket) {
            if (_pUnreliableSequencedFragments) {
                // There are some saved message fragments
                // This wrapper is a cancelled packet, the message cannot be reassembled
                deleteFragments (_pUnreliableSequencedFragments);
                delete _pUnreliableSequencedFragments;
                _pUnreliableSequencedFragments = nullptr;
                checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_Warning,
                                "deleted message fragments because a packet containing a fragment was cancelled\n");
            }
        }
    }
    else {
        if (deliveryPrerequisitesSatisfied (pPacket)) {
            if (bExpiredPackets) {
                // We are skipping one or more packets because they timed out
                // This implies that any fragments collected up to now should be discarded
                if (_pUnreliableSequencedFragments) {
                    // There are some saved message fragments - delete them
                    /*!!*/ // NOTE - This can be changed if partially reassembled packets should be delivered
                    uint16 ui16Count = deleteFragments (_pUnreliableSequencedFragments);
                    delete _pUnreliableSequencedFragments;
                    _pUnreliableSequencedFragments = nullptr;
                    _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_MediumDetailDebug,
                                    "deleted message fragments because a packet was skipped over; current packet number is %lu\n", pPacket->getSequenceNum());
                }
                if ((pPacket->isIntermediateFragment()) || (pPacket->isLastFragment())) {
                    // This is an intermediate or the last fragment
                    // Since we skipped a packet, the first or an intermediate fragment has been lost
                    // This packet must be discarded as well
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_Warning,
                                    "packet %lu is an intermediate or the last fragment but is being discarded because one or more previous packets have been skipped over\n",
                                    pPacket->getSequenceNum());
                    dequeuedPacket (pPacket);
                    delete pPacket;
                    _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets++;
                }
                else if (pPacket->isFirstFragment()) {
                    _pUnreliableSequencedFragments = new LList<Packet*>;
                    _pUnreliableSequencedFragments->add (pPacket);
                    dequeuedPacket (pPacket); // No need to delete pPacket - it is in the list of fragments
                    checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_MediumDetailDebug,
                                    "packet %lu is the first fragment - created a new list for the fragments\n",
                                    pPacket->getSequenceNum());
                }
                else {
                    // This is a whole message, not a fragment - just go ahead and process ir
                    int rc;
                    if (0 != (rc = processPacket (pPacket))) {
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_MildError,
                                        "processPacket failed with rc = %d\n", rc);
                    }
                    else {
                        // No need to delete pPacket - processPacket would have taken care of that
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_MediumDetailDebug,
                                        "processed packet %lu (which was not a fragment) after skipping over some packets\n",
                                        pPacket->getSequenceNum());
                    }
                }
            }
            else {
                // No packet has been skipped over - so the algorithm is the same as the reliable sequenced case
                int rc;
                if (!pPacket->isFragment()) {
                    // This packet is not a fragment - contains a whole message
                    if (_pUnreliableSequencedFragments) {
                        // Problem - this is not a fragment but there are previous fragments waiting to be assembled
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_Warning,
                                        "packet %d is not a fragment but there are other fragments waiting to be reassembled\n",
                                        pPacket->getSequenceNum());
                        uint16 ui16Count = deleteFragments (_pUnreliableSequencedFragments);
                        delete _pUnreliableSequencedFragments;
                        _pUnreliableSequencedFragments = nullptr;
                        _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
                    }
                    if (0 != (rc = processPacket (pPacket))) {
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_MildError,
                                        "processPacket failed with rc = %d\n", rc);
                    }
                    else {
                        // No need to delete pPacket - processPacket would have taken care of that
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_MediumDetailDebug,
                                        "processed packet %lu (which was not a fragment)\n",
                                        pPacket->getSequenceNum());
                    }
                }
                else if (pPacket->isFirstFragment()) {
                    if (_pUnreliableSequencedFragments) {
                        // Problem - there is already a set of message fragments but this packet claims to be the first fragment
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_Warning,
                                        "packet %d is first fragment but already have fragments waiting to be reassembled\n",
                                        pPacket->getSequenceNum());
                        uint16 ui16Count = deleteFragments (_pUnreliableSequencedFragments);
                        delete _pUnreliableSequencedFragments;
                        _pUnreliableSequencedFragments = nullptr;
                        _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets += ui16Count;
                    }
                    _pUnreliableSequencedFragments = new LList<Packet*>;
                    _pUnreliableSequencedFragments->add (pPacket);
                    dequeuedPacket (pPacket);
                    // No need to delete pPacket - it is in the list of fragments
                }
                else if (pPacket->isLastFragment()) {
                    if (_pUnreliableSequencedFragments) {
                        // Have the last fragment for this message
                        // Reassemble the message and deliver it
                        _pUnreliableSequencedFragments->add (pPacket);
                        dequeuedPacket (pPacket);
                        // No need to delete pPacket - it is in the list of fragments
                        deliverFragments (_pUnreliableSequencedFragments);
                        // NOTE: Do not delete _pUnreliableSequencedFragments as the list is stored in the DataBuffer
                        // by deliverFragments() and will be deleted after the application has received the data
                        _pUnreliableSequencedFragments = nullptr;
                    }
                    else {
                        // Problem - this is the last fragment but there are no previous fragments to assemble
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_Warning,
                                        "packet %d is last fragment but there are no fragments waiting to be reassembled\n",
                                        pPacket->getSequenceNum());
                        dequeuedPacket (pPacket);
                        delete pPacket;
                        _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets++;
                    }
                }
                else if (pPacket->isIntermediateFragment()) {
                    if (_pUnreliableSequencedFragments) {
                        _pUnreliableSequencedFragments->add (pPacket);
                        dequeuedPacket (pPacket);
                        // No need to delete pPacket - it is in the list of fragments
                    }
                    else {
                        // Problem - this is a fragment but there are no previous fragments to assemble
                        checkAndLogMsg ("PacketProcessor::tryToProcessFirstPacketFromUnreliableSequencedQueue", Logger::L_Warning,
                                        "packet %d is a fragment but there are no fragments waiting to be reassembled\n",
                                        pPacket->getSequenceNum());
                        dequeuedPacket (pPacket);
                        delete pPacket;
                        _pMocket->getStatistics()->_ui32ReassemblySkippedDiscardedPackets++;
                    }
                }
            }
        }
    }

    return true;
}

bool PacketProcessor::checkForUnreliablePacketTimeout (uint32 ui32NextTSN, uint32 ui32PacketTSN, int64 i64TimeReceived)
{
    uint32 ui32PacketElapsedTime = (uint32) (getTimeInMilliseconds() - i64TimeReceived);
    uint32 ui32MissingPacketCount = (NOMADSUtil::SequentialArithmetic::delta (ui32PacketTSN, ui32NextTSN) + 1);
    if (ui32MissingPacketCount > 5) {
        ui32MissingPacketCount = 5;
    }
    uint32 ui32Timeout = _pMocket->getUnreliableSequencedDeliveryTimeout() * ui32MissingPacketCount;
    return (ui32PacketElapsedTime > ui32Timeout);
}

bool PacketProcessor::deliveryPrerequisitesSatisfied (Packet *pPacket)
{
    if (!pPacket->areDeliveryPrerequisitesSet()) {
        return true;
    }
    DeliveryPrerequisitesAccessor dpa = pPacket->getDeliveryPrerequisites();
    if (pPacket->isControlPacket()) {
        if ((NOMADSUtil::SequentialArithmetic::lessThanOrEqual (dpa.getReliableSequencedTSN(), _ui32NextReliableSequencedPacketTSN)) &&
            (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (dpa.getUnreliableSequencedTSN(), _ui32NextUnreliableSequencedPacketTSN))) {
            return true;
        }
    }
    else if ((pPacket->isReliablePacket()) && (pPacket->isSequencedPacket())) {
        if ((NOMADSUtil::SequentialArithmetic::lessThanOrEqual (dpa.getControlTSN(), _ui32NextControlPacketTSN)) &&
            (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (dpa.getUnreliableSequencedTSN(), _ui32NextUnreliableSequencedPacketTSN))) {
            return true;
        }
    }
    else if (pPacket->isSequencedPacket()) {
        if ((NOMADSUtil::SequentialArithmetic::lessThanOrEqual (dpa.getControlTSN(), _ui32NextControlPacketTSN)) &&
            (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (dpa.getReliableSequencedTSN(), _ui32NextReliableSequencedPacketTSN))) {
            return true;
        }
    }
    else {
        return true;
    }
    return false;
}

int PacketProcessor::deliverFragments (LList<Packet*> *pFragmentList)
{
    // NOTE: The assumption here is that the only chunks in the packets in the fragment list are data chunks
    Packet *pPacket;
    pFragmentList->getFirst (pPacket);
    uint32 ui32StartSequenceNum = pPacket->getSequenceNum();
    uint32 ui32PacketCount = 0;
    uint32 ui32MessageSize = 0;
    updateMessageStatistics (pPacket->isReliablePacket(), pPacket->isSequencedPacket(), pPacket->getTagId());
    do {
        ui32PacketCount++;
        pPacket->resetChunkIterator();
        if (pPacket->getChunkType() != Packet::CT_Data) {
            checkAndLogMsg ("PacketProcessor::deliverFragments", Logger::L_Warning,
                            "encountered a chunk type of %d instead of a data chunk - ignoring\n",
                            (int) pPacket->getChunkType());
        }
        else {
            DataChunkAccessor accessor = pPacket->getDataChunk();
            ui32MessageSize += accessor.getDataLength();
        }
    } while (pFragmentList->getNext (pPacket));
    DataBuffer *pBuffer = new DataBuffer (this, pFragmentList, ui32MessageSize);
    _receivedDataQueue.insert (pBuffer);    // pPacket has been passed off to the receivedDataQueue, which
                                            // will delete pPacket when done in a separate thread
    _pMocket->getStatistics()->_ui32ReceivedBytes += ui32MessageSize;
    checkAndLogMsg ("PacketProcessor::deliverFragments", Logger::L_MediumDetailDebug,
                    "delivered fragmented message with starting packet sequence number %lu and containing %lu fragments\n",
                    ui32StartSequenceNum, ui32PacketCount);
    return 0;
}

uint16 PacketProcessor::deleteFragments (LList<Packet*> *pFragmentList)
{
    uint16 ui16Count = 0;
    Packet *pPacket;
    pFragmentList->resetGet();
    while (pFragmentList->getNext (pPacket)) {
        delete pPacket;
        ui16Count++;
    }
    return ui16Count;
}

void PacketProcessor::updateMessageStatistics (bool bReliable, bool bSequenced, uint16 ui16Tag)
{
    if ((bReliable) && (bSequenced)) {
        _pMocket->getStatistics()->_globalMessageStats.ui32ReceivedReliableSequencedMsgs++;
        if (ui16Tag != 0) {
            _pMocket->getStatistics()->_perTypeMessageStats[ui16Tag].ui32ReceivedReliableSequencedMsgs++;
        }
    }
    else if (bReliable) {
        _pMocket->getStatistics()->_globalMessageStats.ui32ReceivedReliableUnsequencedMsgs++;
        if (ui16Tag != 0) {
            _pMocket->getStatistics()->_perTypeMessageStats[ui16Tag].ui32ReceivedReliableUnsequencedMsgs++;
        }
    }
    else if (bSequenced) {
        _pMocket->getStatistics()->_globalMessageStats.ui32ReceivedUnreliableSequencedMsgs++;
        if (ui16Tag != 0) {
            _pMocket->getStatistics()->_perTypeMessageStats[ui16Tag].ui32ReceivedUnreliableSequencedMsgs++;
        }
    }
    else {
        _pMocket->getStatistics()->_globalMessageStats.ui32ReceivedUnreliableUnsequencedMsgs++;
        if (ui16Tag != 0) {
            _pMocket->getStatistics()->_perTypeMessageStats[ui16Tag].ui32ReceivedUnreliableUnsequencedMsgs++;
        }
    }
}

int PacketProcessor::freezeLinkedList (LList<Packet*> * pSequencedFragments, ObjectFreezer &objectFreezer)
{
    if (pSequencedFragments == nullptr) {
        //printf ("linked list not instantiated\n");
        //Insert a control char to signal that no list follow
        objectFreezer << (unsigned char) 0;

        return 0;
    }
    //Insert a control char to signal that the list follow
    objectFreezer << (unsigned char) 1;
    //objectFreezer.beginNewObject ("SequencedFragments");

    //printf ("SequencedFragments\n");

    // Check if there are elements and go through the whole list
    Packet *pCurrPacket = nullptr;
    if (1 == pSequencedFragments->getFirst (pCurrPacket)) {
        // Insert a control char to signal that another fragment Packet follows
        objectFreezer << (unsigned char) 1;
        //Data from the current Packet
        pCurrPacket->freeze (objectFreezer);

        while (1 ==  pSequencedFragments->getNext (pCurrPacket)) {
            // Insert a control char to signal that another fragment Packet follows
            objectFreezer << (unsigned char) 1;
            //Data from the current Packet
            pCurrPacket->freeze (objectFreezer);
        }
        // Insert a control char to signal that there are no more data
        objectFreezer << (unsigned char) 0;
    }
    return 0;
}

int PacketProcessor::defrostLinkedList (LList<Packet*> * pSequencedFragments, ObjectDefroster &objectDefroster)
{
    //printf ("SequencedFragments\n");
    unsigned char moreData;
    objectDefroster >> moreData;
    while (moreData) {
        Packet * pPacket = new Packet (objectDefroster);
        pSequencedFragments->add (pPacket);
        objectDefroster >> moreData;
    }
    return 0;
}

