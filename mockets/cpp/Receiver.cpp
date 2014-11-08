/*
 * Receiver.cpp
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

#include "Receiver.h"

#include "CommInterface.h"
#include "Mocket.h"
#include "MocketStatusNotifier.h"
#include "PacketAccessors.h"
#include "PacketProcessor.h"
#include "Transmitter.h"

#include "InetAddr.h"
#include "Logger.h"
#include "TClass.h"

#if defined (UNIX)
    #if defined (OSX)
        #ifndef PATH_MAX
            #define PATH_MAX 1024
        #endif
    #endif
    #define MAX_PATH PATH_MAX
#endif


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

Receiver::Receiver (Mocket *pMocket, bool bEnableRecvLogging)
{
    setName ("Receiver");
    _pMocket = pMocket;
    _pCommInterface = pMocket->getCommInterface();
    _pPacketProcessor = pMocket->getPacketProcessor();
    _ui32RemoteAddress = pMocket->getRemoteAddress();
    _ui16RemotePort = pMocket->getRemotePort();
    _ui32IncomingValidation = pMocket->getIncomingValidation();

    _ui32QueuedDataSize = 0;
    _i64LastRecvTime = getTimeInMilliseconds();
    _i64LastSentPacketTime = 0;

    _ui32BytesReceived = 0;

    _ctrlPacketQueue.setNextExpectedSequenceNum (pMocket->getStateCookie()->getControlTSNZ());
    _reliableSequencedPacketQueue.setNextExpectedSequenceNum (pMocket->getStateCookie()->getReliableSequencedTSNZ());
    _unreliableSequencedPacketQueue.setNextExpectedSequenceNum (pMocket->getStateCookie()->getUnreliableSequencedTSNZ());

    InetAddr remoteEndPoint;
    remoteEndPoint.setIPAddress (_ui32RemoteAddress);
    const char *pszRemoteAddress = remoteEndPoint.getIPAsString();
    if (pszRemoteAddress) {
        _pszRemoteAddress = strDup (pszRemoteAddress);
    }
    else {
        _pszRemoteAddress = strDup ("<unknown_ip>");
    }

    if (bEnableRecvLogging) {
        ATime logStartTime;
        char szLogFileName[MAX_PATH];
        InetAddr localAddr (_pMocket->getLocalAddress());
        String localIPAddr = localAddr.getIPAsString();
        InetAddr remoteAddr (_ui32RemoteAddress);
        String remoteIPAddr = remoteAddr.getIPAsString();
        sprintf (szLogFileName, "mmrecv_%s_%d_%s_%d.log",
                 (const char*) localIPAddr, _pMocket->getLocalPort(), (const char*) remoteIPAddr, _ui16RemotePort);
        if (NULL != (_filePacketRecvLog = fopen (szLogFileName, "a"))) {
            fprintf (_filePacketRecvLog, "********** Starting logging at %s", logStartTime.ctime());
            fprintf (_filePacketRecvLog, "Time, DTime, Size, SeqNo, Reliable, Sequenced, Fragment, Tag\n");
        }
        else {
            checkAndLogMsg ("Receiver::receiver", Logger::L_MildError,
                            "failed to open file %s for appending receive log\n", szLogFileName);
        }
    }
    else {
        _filePacketRecvLog = NULL;
    }
    _i64LogStartTime = _i64LastRecvLogTime = getTimeInMilliseconds();
}

Receiver::~Receiver (void)
{
    if (_filePacketRecvLog) {
        fclose (_filePacketRecvLog);
        _filePacketRecvLog = NULL;
    }
    free (_pszRemoteAddress);
    _pszRemoteAddress = NULL;
}

uint32 Receiver::getWindowSize (void)
{
    _m.lock();
    uint32 ui32Temp = _pMocket->getMaximumWindowSize();
    if (ui32Temp > _ui32QueuedDataSize) {
        ui32Temp -= _ui32QueuedDataSize;
        _m.unlock();
        return ui32Temp;
    }
    _m.unlock();
    return 0;
}

void Receiver::incrementQueuedDataSize (uint32 ui32Delta)
{
    _m.lock();
    _ui32QueuedDataSize += ui32Delta;
    _m.unlock();
}

void Receiver::decrementQueuedDataSize (uint32 ui32Delta)
{
    _m.lock();
    if (_ui32QueuedDataSize < ui32Delta) {
        checkAndLogMsg ("Receiver::decrementQueuedDataSize", Logger::L_MildError,
                        "tying to decrement queued data size below 0\n");
        _ui32QueuedDataSize = 0;
    }
    else {
        _ui32QueuedDataSize -= ui32Delta;
    }
    _m.unlock();
}

int Receiver::freeze (ObjectFreezer &objectFreezer)
{
    objectFreezer.beginNewObject ("Receiver");

    // do not migrate _ui32IncomingValidation it is setted from values in the StateCookie
    // _i64LastRecvTime _i64LastSentPacketTime times are always local on the machine
    // no time variables will be migrated
    // Do not migrate _ui32QueuedDataSize, it is computable
    // Do not freeze _ui32RemoteAddress _ui16RemotePort _pszRemoteAddress they are set in the constructor from values in mockets
    if (0 != _ctrlPacketQueue.freeze (objectFreezer)) {
        // return -1 is if objectFreezer.endObject() don't end with success
        return -2;
    }
    if (0 != _reliableSequencedPacketQueue.freeze (objectFreezer)) {
        return -3;
    }
    if (0 != _unreliableSequencedPacketQueue.freeze (objectFreezer)) {
        return -4;
    }
    
    return objectFreezer.endObject();
}

int Receiver::defrost (ObjectDefroster &objectDefroster)
{
    objectDefroster.beginNewObject ("Receiver");
    
    if (0 != _ctrlPacketQueue.defrost (objectDefroster)) {
        return -2;
    }
    if (0 != _reliableSequencedPacketQueue.defrost (objectDefroster)) {
        return -3;
    }
    if (0 != _unreliableSequencedPacketQueue.defrost (objectDefroster)) {
        return -4;
    }
    
    return objectDefroster.endObject();
}

void Receiver::run (void)
{
    bool bDone = false;
    bool bCloseConn = false;
    int64 i64CloseWaitTime = 0;
    char *pRecBuf = (char*) malloc (_pMocket->getMaximumMTU());
    _pCommInterface->setReceiveTimeout (_pMocket->getUDPReceiveTimeout());
    int64 i64LastAppCallbackTime = 0;

    while (!bDone) {
        bool bReceiveError = false;
        bool bNewRemoteNode = false;
        InetAddr remoteAddr;
        Packet *pRecvPacket = NULL;
        int rc = _pCommInterface->receive (pRecBuf, _pMocket->getMaximumMTU(), &remoteAddr);
        
        // Check the return value of receive
        if (rc < 0) {
            checkAndLogMsg ("Receiver::run", Logger::L_LowDetailDebug,
                            "error receiving a packet; remote endpoint = %s:%d; rc = %d; os error = %d\n",
                            _pszRemoteAddress, (int) _ui16RemotePort, rc, _pCommInterface->getLastError());
            bReceiveError = true;
        }
        else if (rc == 0) {
            checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                            "error receiving a packet; socket timed out\n");
            bReceiveError = true;
        }
        else if (rc < Packet::HEADER_SIZE) {
            checkAndLogMsg ("Receiver::run", Logger::L_MildError,
                            "error receiving a packet; received a short packet of size %d\n", rc);
            bReceiveError = true;
        }
        // Check the packet itself
        else {
            pRecvPacket = new Packet (pRecBuf, rc);
            
            if (pRecvPacket->getValidation() != _ui32IncomingValidation) {
                checkAndLogMsg ("Receiver::run", Logger::L_Warning,
                                "error receiving a packet; received a packet with an incorrect validation - expecting %lu, got %lu\n",
                                _ui32IncomingValidation, pRecvPacket->getValidation());
                delete pRecvPacket;
                pRecvPacket = NULL;
                bReceiveError = true;
            }
            else if((remoteAddr.getIPAddress() != _ui32RemoteAddress) || (remoteAddr.getPort() != _ui16RemotePort)) {
                
                // It is possible that this is not an error condition if current node 
                // is in suspend_received and the received packet is a resume
                // or if the message is a Reestablish message due to a change in the network attachment
                if (((_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_RECEIVED) &&
                        (pRecvPacket->getChunkType() == Packet::CT_Resume)) || (pRecvPacket->getChunkType() == Packet::CT_ReEstablish)) {
                    bReceiveError = false;
                }
                // error
                else {
                    checkAndLogMsg ("Receiver::run", Logger::L_Warning,
                                    "error receiving a packet; received a packet from some other endpoint %s:%d\n",
                                    remoteAddr.getIPAsString(), (int) remoteAddr.getPort());
                    delete pRecvPacket;
                    pRecvPacket = NULL;
                    bReceiveError = true;
                }
            }
        }

        if (bReceiveError) {
            if (!bCloseConn) {
                int64 i64CurrTime = getTimeInMilliseconds();
                uint32 ui32ElapsedTime = (uint32) (i64CurrTime - _i64LastRecvTime);
                if (ui32ElapsedTime > (_pMocket->getKeepAliveTimeout() * 2UL)) {
                    // Call a specific callback function if the state is S_SUSPEND_RECEIVED
                    // the application should know that the mocket has been suspended
                    if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_RECEIVED) {
                        // If a suspendReceived callback function is defined
                        if (_pMocket->_pSuspendReceivedWarningCallbackFn) {
                            if ((i64CurrTime - i64LastAppCallbackTime) > 500) {
                                // Only call the application once every 500 ms
                                checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug, "suspend received callback invoked\n");
                                if (_pMocket->_pSuspendReceivedWarningCallbackFn (_pMocket->_pSuspendReceivedCallbackArg, ui32ElapsedTime)) {
                                    // Application has requested that the connection be closed
                                    bCloseConn = true;
                                    _pMocket->setConnectionLingerTime (1);   // Do not want to linger if the application has requested a close
                                    _pMocket->close();
                                }
                                i64LastAppCallbackTime = i64CurrTime;
                            }
                        }
                    }
                    else {
                        // If a peer unreachable callback function is defined
                        if (_pMocket->_pPeerUnreachableWarningCallbackFn) {
                            if ((i64CurrTime - i64LastAppCallbackTime) > 500) {
                                // Only call the application once every 500 ms
                                checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug, "peer unreachable callback invoked\n");
                                if (_pMocket->_pPeerUnreachableWarningCallbackFn (_pMocket->_pPeerUnreachableCallbackArg, ui32ElapsedTime)) {
                                    // Application has requested that the connection be closed
                                    bCloseConn = true;
                                    _pMocket->setConnectionLingerTime (1);   // Do not want to linger if the application has requested a close
                                    _pMocket->close();
                                }
                                i64LastAppCallbackTime = i64CurrTime;
                            }
                        }
                    }
                }
            }
            /*!!*/ // NOTE: The following sleep is to protect from the problem where sometimes receive on the Datagram Socket
                   // returns imemdiately, causing a loop that consumes too much CPU time.
                   // Seems to occur on Win32 when the OS detects the condition where the remote application has gone away
                   // (or some such condition)
                   // Need to find out whether receive() is returning 0 or < 0 and only sleep in that case
            sleepForMilliseconds (10);
        }
        else {
            // At this point, we have a validated packet
            // If the state is S_SUSPEND_RECEIVED accept only resume and suspend packets
            if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_RECEIVED) {
                if (pRecvPacket->getChunkType() == Packet::CT_Resume) {
                    _i64LastRecvTime = getTimeInMilliseconds();
                    _pMocket->getMocketStatusNotifier()->setLastContactTime (_i64LastRecvTime);
                    // printf ("Receiver::run Received resume packet\n");
                    // Extract, decrypt and check the nonce with this method
                    _pMocket->getTransmitter()->processResumePacket(pRecvPacket->getResumeChunk(), remoteAddr.getIPAddress(), remoteAddr.getPort());
                }
                else if (pRecvPacket->getChunkType() == Packet::CT_Suspend) {
                    _i64LastRecvTime = getTimeInMilliseconds();
                    _pMocket->getMocketStatusNotifier()->setLastContactTime (_i64LastRecvTime);
                    checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                    "received a valid packet of size %d from %s:%d\n",
                                    rc, _pszRemoteAddress, _ui16RemotePort);
                    //printf ("Receiver::run Received suspend packet when already suspended, send new suspend_ack\n");
                    _pMocket->getTransmitter()->processSuspendPacket(pRecvPacket->getSuspendChunk());
                }
            }
            else {
                if (i64LastAppCallbackTime > 0) {
                    // The peer has been unreacheable and just came back, notify the application if a callback has been registered
                    if (_pMocket->_pPeerReachableCallbackFn) {
                        uint32 ui32UnreachabilityIntervalLength = (uint32)(getTimeInMilliseconds() - _i64LastRecvTime);
                        _pMocket->_pPeerReachableCallbackFn (_pMocket->_pPeerReachableCallbackArg, ui32UnreachabilityIntervalLength);
                    }
                    // Reset callback time
                    i64LastAppCallbackTime = 0;
                }
                _i64LastRecvTime = getTimeInMilliseconds();
                _pMocket->getMocketStatusNotifier()->setLastContactTime (_i64LastRecvTime);
                checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                "received a valid packet of size %d from %s:%d\n",
                                rc, _pszRemoteAddress, _ui16RemotePort);
                _ui32BytesReceived += rc;

                if (_filePacketRecvLog) {
                    const char *pszFragment = "no";
                    if (pRecvPacket->isFirstFragment()) {
                        pszFragment = "First";
                    }
                    else if (pRecvPacket->isIntermediateFragment()) {
                        pszFragment = "Int";
                    }
                    else if (pRecvPacket->isLastFragment()) {
                        pszFragment = "Last";
                    }
                    #if defined (WIN32)
                        fprintf (_filePacketRecvLog, "%I64d, %I64d, %d, %d, %s, %s, %s, %d\n",
                    #else
                        fprintf (_filePacketRecvLog, "%lld, %lld, %d, %d, %s, %s, %s, %d\n",
                    #endif
                                 (_i64LastRecvTime - _i64LogStartTime),
                                 (_i64LastRecvTime - _i64LastRecvLogTime),
                                 (int) pRecvPacket->getPacketSize(), (int) pRecvPacket->getSequenceNum(),
                                 pRecvPacket->isReliablePacket() ? "yes" : "no",
                                 pRecvPacket->isSequencedPacket() ? "yes" : "no",
                                 pszFragment, (int) pRecvPacket->getTagId());
                    _i64LastRecvLogTime = _i64LastRecvTime;
                }

                // Update the remote window size
                _pMocket->getTransmitter()->setRemoteWindowSize (pRecvPacket->getWindowSize());
                checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                "remote window size is %d\n", (int) pRecvPacket->getWindowSize());

                // Check for SAck Chunks and update the Transmitter
                pRecvPacket->resetChunkIterator();
                while (true) {
                    switch (pRecvPacket->getChunkType()) {
                        case Packet::CT_Init:
                        case Packet::CT_InitAck:
                        case Packet::CT_CookieEcho:
                        case Packet::CT_CookieAck:
                        case Packet::CT_ResumeAck:
                        case Packet::CT_ReEstablishAck:
                            //printf ("Receiver::run Received an unexpected packet\n");
                            checkAndLogMsg ("Receiver::run", Logger::L_Warning,
                                            "received an un expected chunk of type %d\n", (int) pRecvPacket->getChunkType());
                            break;
                        case Packet::CT_SAck:
                            //printf ("Receiver::run Received SAck packet\n");
                            _pMocket->getTransmitter()->processSAckChunk (pRecvPacket->getSAckChunk());
                            _pMocket->getCancelledTSNManager()->processSAckChunk (pRecvPacket->getSAckChunk());
                            break;
                        case Packet::CT_SAckRecBandEst:
                            //printf ("Receiver::run Received SAck packet\n");
                            _pMocket->getTransmitter()->processSAckChunk (pRecvPacket->getSAckRecBandEstChunk());
                            _pMocket->getCancelledTSNManager()->processSAckChunk (pRecvPacket->getSAckRecBandEstChunk());
                            break;
                        case Packet::CT_Cancelled:
                            //printf ("Receiver::run Received cancelled packet\n");
                            processCancelledChunk (pRecvPacket->getCancelledChunk());
                            break;
                        case Packet::CT_Timestamp:
                            //printf ("Receiver::run Received timestamp chunk\n");
                            _pMocket->getTransmitter()->processTimestampChunk (pRecvPacket->getTimestampChunk());
                            break;
                        case Packet::CT_TimestampAck:
                            //printf ("Receiver::run Received timestampAck chunk\n");
                            _pMocket->getTransmitter()->processTimestampAckChunk (pRecvPacket->getTimestampAckChunk());
                            break;
                            
                        // Suspend/Resume process messages
                        case Packet::CT_SimpleSuspend:
                            //printf ("Receiver::run Received simpleSuspend packet\n");
                            // Check for simultaneous suspension.
                            // Continue to wait suspend_ack or go in SUSPEND_RECEIVED state
                            _pMocket->getTransmitter()->processSimpleSuspendPacket(pRecvPacket->getSimpleSuspendChunk());
                            break;
                        case Packet::CT_SimpleSuspendAck:
                            //printf ("Receiver::run Received simpleSuspendAck packet\n");
                            // Extract, decrypt and save nonce (UUID) and Ks
                            _pMocket->getTransmitter()->processSimpleSuspendAckPacket(pRecvPacket->getSimpleSuspendAckChunk());
                            break;
                        case Packet::CT_Suspend:
                            //printf ("Receiver::run Received suspend packet\n");
                            // Check for simultaneous suspension.
                            // Continue to wait suspend_ack or go in SUSPEND_RECEIVED state
                            _pMocket->getTransmitter()->processSuspendPacket(pRecvPacket->getSuspendChunk());
                            break;
                        case Packet::CT_SuspendAck:
                            //printf ("Receiver::run Received suspendAck packet\n");
                            // Extract, decrypt and save nonce (UUID) and Ks
                            _pMocket->getTransmitter()->processSuspendAckPacket(pRecvPacket->getSuspendAckChunk());
                            break;
                        case Packet::CT_Resume:
                            //printf ("Receiver::run Received resume packet\n");
                            // Extract, decrypt and check the nonce
                            _pMocket->getTransmitter()->processResumePacket(pRecvPacket->getResumeChunk(), remoteAddr.getIPAddress(), remoteAddr.getPort());
                            break;
                        case Packet::CT_ReEstablish:
                            //printf ("Receiver::run Received reEstablish packet\n");
                            // Extract, decrypt and check the nonce
                            _pMocket->getTransmitter()->processReEstablishPacket(pRecvPacket->getReEstablishChunk(), remoteAddr.getIPAddress(), remoteAddr.getPort());
                            break;
                    };
                    if (!pRecvPacket->advanceToNextChunk()) {
                        break;
                    }
                }
  
                // Check the window size to see if there is room to accept this packet
                /*if (getWindowSize() < pRecvPacket->getPacketSizeWithoutPiggybackChunks()) {
                    checkAndLogMsg ("Receiver::run", Logger::L_LowDetailDebug,
                                    "discarding packet with sequence number %lu because of insufficient room; there are %lu %lu %lu packets in the control, reliable sequenced, and unreliable sequenced queue\n",
                                    pRecvPacket->getSequenceNum(), _ctrlPacketQueue.getPacketCount(), _reliableSequencedPacketQueue.getPacketCount(), _unreliableSequencedPacketQueue.getPacketCount());
                    delete pRecvPacket;
                    pRecvPacket = NULL;
                    // Update the discarded packet count statistic
                    _pMocket->getStatistics()->_ui32NoRoomDiscardedPackets++;
                }
                else {*/
                    // Update the received packet count statistic
                    _pMocket->getStatistics()->_ui32ReceivedPackets++;

                    // Duplicate the buffer in the packet (and remove piggyback chunks if present)
                    pRecvPacket->prepareForProcessing();

                    incrementQueuedDataSize (pRecvPacket->getPacketSize());   // No need to use getPacketSizeWithoutPiggybackChunks() anymore because of the call to prepareForProcessing() above

                    // Enqueue the packet if necessary
                    if (pRecvPacket->isControlPacket()) {
                        uint32 ui32SequenceNum = pRecvPacket->getSequenceNum();
                        PacketWrapper *pWrapper = new PacketWrapper (pRecvPacket, _i64LastRecvTime);
                        if (_ctrlPacketQueue.insert (pWrapper)) {
                            checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                            "enqueued control packet with sequence number %lu into control packet queue\n", ui32SequenceNum);
                            _pMocket->getACKManager()->receivedControlPacket (pRecvPacket->getSequenceNum());
                            _pPacketProcessor->packetArrived();
                        }
                        else {
                            decrementQueuedDataSize (pRecvPacket->getPacketSize());
                            delete pWrapper;
                            delete pRecvPacket;
                            pRecvPacket = NULL;
                            _pMocket->getStatistics()->_ui32DuplicatedDiscardedPackets++;
                            _pMocket->getTransmitter()->requestSAckTransmission();
                            checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                            "dropped control packet with sequence number %lu\n", ui32SequenceNum);
                        }
                    }
                    else if ((pRecvPacket->isReliablePacket()) && (pRecvPacket->isSequencedPacket())) {
                        // This is a reliable sequenced packet
                        uint32 ui32SequenceNum = pRecvPacket->getSequenceNum();
                        PacketWrapper *pWrapper = new PacketWrapper (pRecvPacket, _i64LastRecvTime);
                        if (_reliableSequencedPacketQueue.insert (pWrapper)) {
                            checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                            "enqueued reliable sequenced packet with sequence number %lu into reliable sequenced packet queue\n", ui32SequenceNum);
                            _pMocket->getACKManager()->receivedReliableSequencedPacket (ui32SequenceNum);
                            _pPacketProcessor->packetArrived();
                        }
                        else {
                            decrementQueuedDataSize (pRecvPacket->getPacketSize());
                            delete pWrapper;
                            delete pRecvPacket;
                            pRecvPacket = NULL;
                            _pMocket->getStatistics()->_ui32DuplicatedDiscardedPackets++;
                            _pMocket->getTransmitter()->requestSAckTransmission();
                            checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                            "dropped reliable sequenced packet with sequence number %lu\n", ui32SequenceNum);
                        }
                    }
                    else if (pRecvPacket->isReliablePacket()) {
                        // This is a reliable unsequenced packet
                        uint32 ui32SequenceNum = pRecvPacket->getSequenceNum();
                        _pMocket->getACKManager()->receivedReliableUnsequencedPacket (pRecvPacket->getSequenceNum());
                        _pPacketProcessor->processReliableUnsequencedPacket (pRecvPacket);
                        checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                        "passed reliable unsequenced packet with sequence number %lu to the packet processor\n", ui32SequenceNum);
                    }
                    else if (pRecvPacket->isSequencedPacket()) {
                        // This is an unreliable sequenced packet
                        #if defined (USE_BUFFERING_FOR_UNRELIABLE_SEQUENCED)
                            uint32 ui32SequenceNum = pRecvPacket->getSequenceNum();
                            PacketWrapper *pWrapper = new PacketWrapper (pRecvPacket, _i64LastRecvTime);
                            if (_unreliableSequencedPacketQueue.insert (pWrapper)) {
                                checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                                "enqueuing packet with sequence number %lu into unreliable sequenced packet queue\n", ui32SequenceNum);
                                                        _pPacketProcessor->packetArrived();
                            }
                            else {
                                // Should not have received a duplicate packet as the sender does not retransmit packets for unreliable flows
                                checkAndLogMsg ("Receiver::run", Logger::L_Warning,
                                                "received a duplicate unreliable sequenced packet with sequence number %lu\n", ui32SequenceNum);
                                decrementQueuedDataSize (pRecvPacket->getPacketSize());
                                delete pWrapper;
                                delete pRecvPacket;
                                pRecvPacket = NULL;
                                _pMocket->getStatistics()->_ui32DuplicatedDiscardedPackets++;
                            }
                        #else
                            _pPacketProcessor->processUnreliableSequencedPacketWithoutBuffering (pRecvPacket);
                        #endif
                    }
                    else {
                        // This is an unreliable unsequenced packet
                        // NOTE: Could just be something like a heartbeat packet also!
                        _pPacketProcessor->processUnreliableUnsequencedPacket (pRecvPacket);
                        checkAndLogMsg ("Receiver::run", Logger::L_MediumDetailDebug,
                                        "passed unreliable unsequenced packet to the packet processor\n");
                    }
                //} // This is part of the if/else that checks the window size which is now commented out
            }
        }
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_CLOSED) {
            bDone = true;
        }
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPENDED) {
            bDone = true;
        }
    }
    free (pRecBuf);
    _pMocket->receiverTerminating();
}

// Process a Cancelled Chunk, which contains sequence numbers of packets that have been
// cancelled and will not be retransmitted from the sender side
// In the case of senquenced packets, this method creates dummy packets that are inserted
// into the appropriate sequenced packet queues so that the packet processor knows not to
// wait for the packets with those sequence numbers
// In the case of reliable unsequenced packets, the only requirement is to "acknowledge"
// the sequence numbers (ids) of the cancelled packets so that the cumulative ACK can be
// incremented normally
int Receiver::processCancelledChunk (CancelledChunkAccessor cancelledChunkAccessor)
{
    /*!!*/ // NOTE: If a duplicate cancelled chunk is received, it is possible that the
           // mocket on the other side is not getting the SAck
           // Therefore, consider requesting that the SAck packet should be retransmitted (or see if
           // this happens already)
    int64 i64CurrTime = getTimeInMilliseconds();
    checkAndLogMsg ("Receiver::processCancelledChunk", Logger::L_MediumDetailDebug,
                    "processing a cancelled chunk\n");
    while (cancelledChunkAccessor.haveMoreBlocks()) {
        while (cancelledChunkAccessor.haveMoreElements()) {
            switch (cancelledChunkAccessor.getBlockType()) {
                case CancelledChunkAccessor::BT_RANGE_RELIABLE_SEQUENCED:
                    //printf ("Received cancelled chunck msg. TSN range: %d - %d\n", cancelledChunkAccessor.getStartTSN(), cancelledChunkAccessor.getEndTSN());
                    for (uint32 ui32TSN = cancelledChunkAccessor.getStartTSN(); ui32TSN <= cancelledChunkAccessor.getEndTSN(); ui32TSN++) {
                        _reliableSequencedPacketQueue.lock();
                        if (_reliableSequencedPacketQueue.canInsert (ui32TSN)) {
                            PacketWrapper *pWrapper = new PacketWrapper (ui32TSN, i64CurrTime);
                            if (!_reliableSequencedPacketQueue.insert (pWrapper)) {
                                checkAndLogMsg ("Receiver::processCancelledChunk", Logger::L_Warning,
                                                "failed to insert cancelled packet for sequence number %lu into reliable sequenced packet queue\n",
                                                ui32TSN);
                                delete pWrapper;
                            }
                            _pMocket->getACKManager()->receivedReliableSequencedPacket (ui32TSN);
                        }
                        _reliableSequencedPacketQueue.unlock();
                    }
                    checkAndLogMsg ("Receiver::processCancelledChunk", Logger::L_MediumDetailDebug,
                                    "inserted cancelled packets (as needed) into reliable sequenced queue with sequence numbers from %lu to %lu\n",
                                    cancelledChunkAccessor.getStartTSN(), cancelledChunkAccessor.getEndTSN());
                    break;
                case CancelledChunkAccessor::BT_SINGLE_RELIABLE_SEQUENCED:
                {
                    uint32 ui32CancelledTSN = cancelledChunkAccessor.getTSN();
                    //printf ("Received cancelled chunck msg. TSN: %d\n", ui32CancelledTSN);
                    _reliableSequencedPacketQueue.lock();
                    if (_reliableSequencedPacketQueue.canInsert (ui32CancelledTSN)) {
                        PacketWrapper *pWrapper = new PacketWrapper (ui32CancelledTSN, i64CurrTime);
                        if (!_reliableSequencedPacketQueue.insert (pWrapper)) {
                            checkAndLogMsg ("Receiver::processCancelledChunk", Logger::L_Warning,
                                            "failed to insert cancelled packet for sequence number %lu into reliable sequenced packet queue\n",
                                            ui32CancelledTSN);
                            delete pWrapper;
                        }
                    }
                    else {
                        checkAndLogMsg ("Receiver::processCancelledChunk", Logger::L_MediumDetailDebug,
                                        "inserted a cancelled packet into reliable sequenced queue with sequence number %lu\n", ui32CancelledTSN);
                    }
                    _reliableSequencedPacketQueue.unlock();
                    _pMocket->getACKManager()->receivedReliableSequencedPacket (cancelledChunkAccessor.getTSN());
                    break;
                }
                case CancelledChunkAccessor::BT_RANGE_RELIABLE_UNSEQUENCED:
                    for (uint32 ui32TSN = cancelledChunkAccessor.getStartTSN(); ui32TSN <= cancelledChunkAccessor.getEndTSN(); ui32TSN++) {
                        _pMocket->getACKManager()->receivedReliableUnsequencedPacket (ui32TSN);
                    }
                    break;
                case CancelledChunkAccessor::BT_SINGLE_RELIABLE_UNSEQUENCED:
                    _pMocket->getACKManager()->receivedReliableUnsequencedPacket (cancelledChunkAccessor.getTSN());
                    break;
                case CancelledChunkAccessor::BT_RANGE_UNRELIABLE_SEQUENCED:
                    _unreliableSequencedPacketQueue.lock();
                    for (uint32 ui32TSN = cancelledChunkAccessor.getStartTSN(); ui32TSN <= cancelledChunkAccessor.getEndTSN(); ui32TSN++) {
                        if (_unreliableSequencedPacketQueue.canInsert (ui32TSN)) {
                            PacketWrapper *pWrapper = new PacketWrapper (ui32TSN, i64CurrTime);
                            if (!_unreliableSequencedPacketQueue.insert (pWrapper)) {
                                checkAndLogMsg ("Receiver::processCancelledChunk", Logger::L_Warning,
                                                "failed to insert cancelled packet for sequence number %lu into unreliable sequenced packet queue\n",
                                                ui32TSN);
                                delete pWrapper;
                            }
                        }
                    }
                    _unreliableSequencedPacketQueue.unlock();
                    break;
                case CancelledChunkAccessor::BT_SINGLE_UNRELIABLE_SEQUENCED:
                {
                    uint32 ui32CancelledTSN = cancelledChunkAccessor.getTSN();
                    _unreliableSequencedPacketQueue.lock();
                    if (_unreliableSequencedPacketQueue.canInsert (ui32CancelledTSN)) {
                        PacketWrapper *pWrapper = new PacketWrapper (ui32CancelledTSN, i64CurrTime);
                        if (!_unreliableSequencedPacketQueue.insert (pWrapper)) {
                            checkAndLogMsg ("Receiver::processCancelledChunk", Logger::L_Warning,
                                            "failed to insert cancelled packet for sequence number %lu into unreliable sequenced packet queue\n",
                                            ui32CancelledTSN);
                            delete pWrapper;
                        }
                    }
                    _unreliableSequencedPacketQueue.unlock();
                    break;
                }
            }
            cancelledChunkAccessor.advanceToNextElement();
        }
        cancelledChunkAccessor.advanceToNextBlock();
    }
    return 0;
}
