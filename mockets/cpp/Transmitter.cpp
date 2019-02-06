/*
 * Transmitter.cpp
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

#include "Transmitter.h"

#include "CommInterface.h"
#include "Mocket.h"
#include "MocketStatusNotifier.h"
#include "Receiver.h"

#include "Logger.h"
#include "TClass.h"
#include "DLList.h"
#include "NLFLib.h"

#if !defined (ANDROID) //No std support on ANDROID
    #include <cmath>
    #include <iostream>
#endif

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

Transmitter::Transmitter (Mocket *pMocket, bool bEnableXMitLogging)
    : _pendingPacketQueue (Mocket::DEFAULT_PENDING_PACKET_QUEUE_SIZE, Mocket::DEFAULT_CROSS_SEQUENCING_SETTING),
      _cv (&_m), _lckRemoteWindowSize(), _cvFlushData (&_mFlushData), _cvSuspend (&_mSuspend)
{
    setName ("Transmitter");
    _pMocket = pMocket;
    _pCommInterface = pMocket->getCommInterface();
    _ui32RemoteAddress = pMocket->getRemoteAddress();
    _ui16RemotePort = pMocket->getRemotePort();

    _filePacketXMitLog = nullptr;
    _i64LogStartTime = _i64LastXMitLogTime = getTimeInMilliseconds();
    enableTransmitLogging (bEnableXMitLogging);

    _ui32RemoteWindowSize = pMocket->getMaximumWindowSize();

    _ui32MessageTSN = 1;        // Zero is reserved for all the messages not fragmented
    _bSendingFragmentedMsg = false;

    _ui32ControlTSN = pMocket->getStateCookie()->getControlTSNA();
    _ui32ReliableSequencedTSN = pMocket->getStateCookie()->getReliableSequencedTSNA();
    _ui32UnreliableSequencedTSN = pMocket->getStateCookie()->getUnreliableSequencedTSNA();
    _ui32ReliableUnsequencedID = pMocket->getStateCookie()->getReliableUnsequencedIDA();
    _ui32UnreliableUnsequencedID = pMocket->getStateCookie()->getUnreliableUnsequencedIDA();

    // Initialize the times to the current time and not 0 to prevent an initial, unnecessary timeout for these actions
    _i64LastTransmitTime = _i64LastSAckTransmitTime = _i64LastCancelledTSNTransmitTime = _i64LastStatUpdateTime = getTimeInMilliseconds();

    _bSendSAckInformationNow = false;
    _bSendTimestamp = false;
    _bSendTimestampAck = false;
    _i64Timestamp = 0;
    _i64TimestampReceiveTime = 0;
    _i64ShutdownStartTime = 0;
    _bSendResumeAck = false;
    _bSendSuspendAck = false;
    _bSendReEstablishAck = false;
    _bSendSimpleSuspendAck = false;

    _pFastRetransmitControlPackets = nullptr;
    _pFastRetransmitReliableSequencedPackets = nullptr;
    _pFastRetransmitReliableUnsequencedPackets = nullptr;

    _fSRTT = (float) pMocket->getInitialAssumedRTT();
    _i64LastRTTEstimationTime = getTimeInMilliseconds();

    _pCongestionControl = nullptr;
    _pBandwidthEstimator = nullptr;

    _ui16NumberOfAcknowledgedPackets = 0;

    _i64NextTimeToTransmit = 0;

    _i64LastRecTimeTimestamp = 0;
    _ui32RecSideBytesReceived = 0;

    _pByteSentPerInterval = nullptr;

}

Transmitter::~Transmitter (void)
{
    if (_filePacketXMitLog) {
        fclose (_filePacketXMitLog);
        _filePacketXMitLog = nullptr;
    }

    if(_pBandwidthEstimator != nullptr) {
        delete _pBandwidthEstimator;
        _pBandwidthEstimator = nullptr;
    }

    if(_pCongestionControl != nullptr) {
        delete _pCongestionControl;
        _pCongestionControl = nullptr;
    }
    if (_pFastRetransmitControlPackets != nullptr) {
        delete _pFastRetransmitControlPackets;
        _pFastRetransmitControlPackets = nullptr;
    }
    if (_pFastRetransmitReliableSequencedPackets != nullptr) {
        delete _pFastRetransmitReliableSequencedPackets;
        _pFastRetransmitReliableSequencedPackets = nullptr;
    }
    if (_pFastRetransmitReliableUnsequencedPackets != nullptr) {
        delete _pFastRetransmitReliableUnsequencedPackets;
        _pFastRetransmitReliableUnsequencedPackets = nullptr;
    }
    if (_pByteSentPerInterval != nullptr) {
        delete _pByteSentPerInterval;
        _pByteSentPerInterval = nullptr;
    }
}

void Transmitter::enableTransmitLogging (bool bEnableXMitLogging)
{
    if (bEnableXMitLogging) {
        if (_filePacketXMitLog) {
            // Logging is already enabled - nothing to do
            return;
        }
        else {
            ATime logStartTime;
            char szLogFileName[MAX_PATH];
            InetAddr localAddr (_pMocket->getLocalAddress());
            String localIPAddr = localAddr.getIPAsString();
            InetAddr remoteAddr (_ui32RemoteAddress);
            String remoteIPAddr = remoteAddr.getIPAsString();
            sprintf (szLogFileName, "mmxmit_%s_%d_%s_%d.log",
                     (const char*) localIPAddr, _pMocket->getLocalPort(), (const char*) remoteIPAddr, _ui16RemotePort);
            if (nullptr != (_filePacketXMitLog = fopen (szLogFileName, "a"))) {
                fprintf (_filePacketXMitLog, "********** Starting logging at %s", logStartTime.ctime());
                fprintf (_filePacketXMitLog, "Time, DTime, Size, SeqNo, Reliable, Sequenced, Fragment, Tag, Purpose\n");
            }
            else {
                checkAndLogMsg ("Transmitter::enableTransmitLogging", Logger::L_MildError,
                                "failed to open file %s for appending transmit log\n", szLogFileName);
            }
            _i64LogStartTime = _i64LastXMitLogTime = getTimeInMilliseconds();
        }
    }
    else {
        if (_filePacketXMitLog) {
            fclose (_filePacketXMitLog);
            _filePacketXMitLog = nullptr;
        }
        else {
            // Logging is already disabled - nothing to do
        }
    }
}

int Transmitter::setTransmitRateLimit (uint32 ui32TransmitRateLimit)
{
    _resLimits.ui32RateLimit = ui32TransmitRateLimit;   // bytes per second

    // High bandwidth limit, use the bandwidth limit based on calculations of packets sent over time
    if (ui32TransmitRateLimit > _pMocket->BANDWIDTH_LIMITATION_THRESHOLD) {
        _resLimits.ui32BytesPerInterval = ui32TransmitRateLimit * _pMocket->BANDWIDTH_LIMITATION_DEFAULT_INTERVAL / 1000;
        if (_pByteSentPerInterval == nullptr) {
            _pByteSentPerInterval = new TimeIntervalAverage<uint32> (_pMocket->BANDWIDTH_LIMITATION_DEFAULT_INTERVAL);
        }
    }
    else {
        // Setting a small bandwidth limit (or removing the limit), stop enqueuing data in _pByteSentPerInterval
        if (_pByteSentPerInterval != nullptr) {
            delete _pByteSentPerInterval;
            _pByteSentPerInterval = nullptr;
        }
    }
    return 0;
}

uint32 Transmitter::getRetransmissionTimeout (void)
{
    return (uint32) ((_fSRTT + _pMocket->getRTOConstant()) * _pMocket->getRTOFactor());
}

BandwidthEstimator * Transmitter::getBandwidthEstimator (void)
{
    return _pBandwidthEstimator;
}

int Transmitter::send (bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                       uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    if (_pMocket->getStateMachine()->getCurrentState() != StateMachine::S_ESTABLISHED) {
        return -1;
    }

    uint16 ui16AvailSize = _pMocket->getMTU() - (Packet::HEADER_SIZE +
                                                 Packet::DATA_CHUNK_HEADER_SIZE);

    if (_pMocket->isCrossSequencingEnabled()) {
        ui16AvailSize -= Packet::DELIVERY_PREREQUISITES_SIZE;
    }

    bool bFragmentationNeeded = ui32BufSize > ((uint32) ui16AvailSize);
    uint16 ui16FragmentNum = 0;

    uint32 ui32BytesLeft = ui32BufSize;

    uint32 ui32MessageTSN = 0;
    if (bFragmentationNeeded) {
        _mSend.lock();
        // Used to keep fragments of the same message together when they are in the PendingPacketQueue
        ui32MessageTSN = _ui32MessageTSN++;
        _mSend.unlock();
    }

    while (ui32BytesLeft > 0) {
        uint32 ui32BytesToSend = ui32BytesLeft;
        if (ui32BytesToSend > ui16AvailSize) {
            ui32BytesToSend = ui16AvailSize;
        }
        Packet *pPacket = new Packet (_pMocket);
        if (_pMocket->isCrossSequencingEnabled()) {
            pPacket->allocateSpaceForDeliveryPrerequisites();
        }
        pPacket->setReliablePacket (bReliable);
        pPacket->setSequencedPacket (bSequenced);
        if (pPacket->addDataChunk (ui16Tag, ((char*)pBuf)+(ui32BufSize-ui32BytesLeft), ui32BytesToSend)) {
            checkAndLogMsg ("Transmitter::send", Logger::L_MildError,
                            "could not add data chunk to packet\n");
            delete pPacket;
            return -2;
        }
        if (bFragmentationNeeded) {
            if (ui32BytesLeft == ui32BytesToSend) {
                // No more bytes left to send
                pPacket->setAsLastFragment();
            }
            else {
                if (ui16FragmentNum == 0) {
                    pPacket->setAsFirstFragment();
                }
                else {
                    pPacket->setAsIntermediateFragment();
                }
                ui16FragmentNum++;
            }
        }

        uint32 ui32RTO = getRetransmissionTimeout();
        // Check the maximum and minimum RTO
        if (_pMocket->getMaximumRTO() > 0) {
            if (ui32RTO > _pMocket->getMaximumRTO()) {
                ui32RTO = _pMocket->getMaximumRTO();
            }
        }
        if (ui32RTO < _pMocket->getMinimumRTO()) {
            ui32RTO = _pMocket->getMinimumRTO();
        }
        PacketWrapper *pWrapper = new PacketWrapper (pPacket, 0, ui8Priority, ui32MessageTSN, ui32RetryTimeout, ui32RTO);
        if (!_pendingPacketQueue.insert (pWrapper, ui32EnqueueTimeout)) {
            checkAndLogMsg ("Transmitter::send", Logger::L_MediumDetailDebug,
                            "failed to enqueue packet into the packet queue within the specified timeout of %lu ms\n",
                            ui32EnqueueTimeout);
            delete pPacket;
            delete pWrapper;
            return -3;
        }
        ui32BytesLeft -= ui32BytesToSend;
        _m.lock();
        _pMocket->getStatistics()->_ui32PendingDataSize = _pendingPacketQueue.getBytesInQueue();
        _pMocket->getStatistics()->_ui32PendingPacketQueueSize = _pendingPacketQueue.getPacketsInQueue();
        _cv.notifyAll();    // Wake up the run so that the new packet will be processed (if it fits in the bandwidth limit)
        _m.unlock();
        this->yield();      // Need to yield to the run thread or when sending very fast it does not give up the control and the queue fills up
        /*!!*/ // mauro: is/should be access to mocket statistics synchronized?
        _mStat.lock();
        _pMocket->getStatistics()->_ui32SentBytes += ui32BytesToSend;
        if ((bReliable) && (bSequenced)) {
            _pMocket->getStatistics()->getOverallMessageStatistics()->ui32SentReliableSequencedMsgs++;
            if (ui16Tag > 0) {
                _pMocket->getStatistics()->getMessageStatisticsForType (ui16Tag)->ui32SentReliableSequencedMsgs++;
            }
        }
        else if (bReliable) {
            _pMocket->getStatistics()->getOverallMessageStatistics()->ui32SentReliableUnsequencedMsgs++;
            if (ui16Tag > 0) {
                _pMocket->getStatistics()->getMessageStatisticsForType (ui16Tag)->ui32SentReliableUnsequencedMsgs++;
            }
        }
        else if (bSequenced) {
            _pMocket->getStatistics()->getOverallMessageStatistics()->ui32SentUnreliableSequencedMsgs++;
            if (ui16Tag > 0) {
                _pMocket->getStatistics()->getMessageStatisticsForType (ui16Tag)->ui32SentUnreliableSequencedMsgs++;
            }
        }
        else {
            _pMocket->getStatistics()->getOverallMessageStatistics()->ui32SentUnreliableUnsequencedMsgs++;
            if (ui16Tag > 0) {
                _pMocket->getStatistics()->getMessageStatisticsForType (ui16Tag)->ui32SentUnreliableUnsequencedMsgs++;
            }
        }
        _mStat.unlock();
    }
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

int Transmitter::gsend (bool bReliable, bool bSequenced, uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout,
                        const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
{
    if (_pMocket->getStateMachine()->getCurrentState() != StateMachine::S_ESTABLISHED) {
        return -1;
    }

    uint16 ui16SpacePerPacket = _pMocket->getMTU() - (Packet::HEADER_SIZE +
                                                      Packet::DATA_CHUNK_HEADER_SIZE);

    if (_pMocket->isCrossSequencingEnabled()) {
        ui16SpacePerPacket -= Packet::DELIVERY_PREREQUISITES_SIZE;
    }
    uint32 ui32TotalBytes = ui32BufSize1;
    while (va_arg (valist1, const void*) != nullptr) {
        ui32TotalBytes += va_arg (valist1, uint32);
    }

    bool bFragmentationNeeded = ui32TotalBytes > ((uint32) ui16SpacePerPacket);
    uint16 ui16FragmentNum = 0;
    uint32 ui32MessageTSN = 0;
    if (bFragmentationNeeded) {
        // Used to keep fragments of the same message together when they are in the PendingPacketQueue
        ui32MessageTSN = _ui32MessageTSN++;
    }

    Packet *pPacket = nullptr;
    DataChunkMutator dcm;
    const void *pBuf = pBuf1;
    uint32 ui32BufSize = ui32BufSize1;
    while (pBuf != nullptr) {
        uint32 ui32BytesLeft = ui32BufSize;
        while (ui32BytesLeft > 0) {
            if (pPacket == nullptr) {
                pPacket = new Packet (_pMocket);
                if (_pMocket->isCrossSequencingEnabled()) {
                    pPacket->allocateSpaceForDeliveryPrerequisites();
                }
                pPacket->setReliablePacket (bReliable);
                pPacket->setSequencedPacket (bSequenced);
                dcm = pPacket->addDataChunk (ui16Tag);
            }
            uint32 ui32BytesToSend = ui32BytesLeft;
            if (ui32BytesToSend > dcm.getSpaceAvail()) {
                ui32BytesToSend = dcm.getSpaceAvail();
            }
            if (dcm.addDataFragment (((char*)pBuf)+(ui32BufSize-ui32BytesLeft), ui32BytesToSend)) {
                checkAndLogMsg ("Transmitter::gsend", Logger::L_MildError,
                                "could not add data fragment to packet\n");
                delete pPacket;
                return -2;
            }
            ui32BytesLeft -= ui32BytesToSend;
            ui32TotalBytes -= ui32BytesToSend;
            _pMocket->getStatistics()->_ui32SentBytes += ui32BytesToSend;
            if (dcm.getSpaceAvail() == 0) {
                // This packet is full - enqueue it
                if (bFragmentationNeeded) {
                    if (ui32TotalBytes == 0) {
                        // No more bytes left to send
                        pPacket->setAsLastFragment();
                    }
                    else {
                        if (ui16FragmentNum == 0) {
                            pPacket->setAsFirstFragment();
                        }
                        else {
                            pPacket->setAsIntermediateFragment();
                        }
                        ui16FragmentNum++;
                    }
                }
                PacketWrapper *pWrapper = new PacketWrapper (pPacket, 0, ui8Priority, ui32MessageTSN, ui32RetryTimeout, getRetransmissionTimeout());
                if (!_pendingPacketQueue.insert (pWrapper, ui32EnqueueTimeout)) {
                    checkAndLogMsg ("Transmitter::gsend", Logger::L_MediumDetailDebug,
                                    "failed to enqueue packet into the packet queue within the specified timeout of %lu ms\n",
                                    ui32EnqueueTimeout);
                    delete pPacket;
                    delete pWrapper;
                    return -3;
                }
                _m.lock();
                _cv.notifyAll();    // Wake up the run so that the new packet will be processed (if it fits in the bandwidth limit)
                _pMocket->getStatistics()->_ui32PendingDataSize = _pendingPacketQueue.getBytesInQueue();
                _pMocket->getStatistics()->_ui32PendingPacketQueueSize = _pendingPacketQueue.getPacketsInQueue();
                _m.unlock();
                pPacket = nullptr;
            }
        }
        pBuf = va_arg (valist2, const void*);
        ui32BufSize = va_arg (valist2, uint32);
    }
    if (pPacket) {
        // There is still a packet left to be enqueued
        if (bFragmentationNeeded) {
            // Must be the last fragment
                pPacket->setAsLastFragment();
        }
        PacketWrapper *pWrapper = new PacketWrapper (pPacket, 0, ui8Priority, ui32MessageTSN, ui32RetryTimeout, getRetransmissionTimeout());
        if (!_pendingPacketQueue.insert (pWrapper, ui32EnqueueTimeout)) {
            checkAndLogMsg ("Transmitter::gsend", Logger::L_MediumDetailDebug,
                            "failed to enqueue packet into the packet queue within the specified timeout of %lu ms\n",
                            ui32EnqueueTimeout);
            delete pPacket;
            delete pWrapper;
            return -3;
        }
        _m.lock();
        _cv.notifyAll();    // Wake up the run so that the new packet will be processed (if it fits in the bandwidth limit)
        _pMocket->getStatistics()->_ui32PendingDataSize = _pendingPacketQueue.getBytesInQueue();
        _pMocket->getStatistics()->_ui32PendingPacketQueueSize = _pendingPacketQueue.getPacketsInQueue();
        _m.unlock();
        this->yield();      // Need to yield to the run thread or when sending very fast it does not give up the control and the queue fills up
    }
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

int Transmitter::cancel (bool bReliable, bool bSequenced, uint16 ui16TagId, uint8 * pui8HigherPriority)
{
    int rc;
    int iPacketsDeleted = 0;

    // First delete any matching packets in the pending packet queue
    rc = _pendingPacketQueue.cancel (bReliable, bSequenced, ui16TagId, pui8HigherPriority);
    if (rc < 0) {
        return -1;
    }
    iPacketsDeleted += rc;
    _pMocket->getStatistics()->getOverallMessageStatistics()->ui32CancelledPackets += rc;
    _pMocket->getStatistics()->_ui32PendingDataSize = _pendingPacketQueue.getBytesInQueue();
    _pMocket->getStatistics()->_ui32PendingPacketQueueSize = _pendingPacketQueue.getPacketsInQueue();

    // If the messages being cancelled are reliable, also delete any matching packets in the unacknowledged packet queue
    if (bReliable) {
        if (bSequenced) {
            _pMocket->getCancelledTSNManager()->startCancellingReliableSequencedPackets();
            rc = _upqReliableSequencedPackets.cancel (ui16TagId, _pMocket->getCancelledTSNManager());
            if (rc > 0) {
                _pMocket->getStatistics()->_ui32ReliableSequencedDataSize = _upqReliableSequencedPackets.getQueuedDataSize();
                _pMocket->getStatistics()->_ui32ReliableSequencedPacketQueueSize = _upqReliableSequencedPackets.getPacketCount();
                checkAndLogMsg ("Transmitter::cancel", Logger::L_MediumDetailDebug,
                                "cancelled %d packets from the reliable sequenced flow\n", rc);
            }
            _pMocket->getCancelledTSNManager()->endCancellingPackets();
        }
        else {
            _pMocket->getCancelledTSNManager()->startCancellingReliableUnsequencedPackets();
            rc = _upqReliableUnsequencedPackets.cancel (ui16TagId, _pMocket->getCancelledTSNManager());
            if (rc > 0) {
                _pMocket->getStatistics()->_ui32ReliableUnsequencedDataSize = _upqReliableUnsequencedPackets.getQueuedDataSize();
                _pMocket->getStatistics()->_ui32ReliableUnsequencedPacketQueueSize = _upqReliableUnsequencedPackets.getPacketCount();
                checkAndLogMsg ("Transmitter::cancel", Logger::L_MediumDetailDebug,
                                "cancelled %d packets from the reliable unsequenced flow\n", rc);
            }
            _pMocket->getCancelledTSNManager()->endCancellingPackets();
        }
        if (rc < 0) {
            return -2;
        }
        iPacketsDeleted += rc;
        _pMocket->getStatistics()->getOverallMessageStatistics()->ui32CancelledPackets += rc;
    }
    return iPacketsDeleted;
}

void Transmitter::run (void)
{
    int rc;
    bool bDone = false;
    int iShutdownsSent = 0;
    int iShutdownAcksSent = 0;

    // mauro: i really wonder if this lock is too coarse-grained.
    // locking the whole transmitter here is an overkill since
    // we basically only need to lock on pending and unacknowledged
    // packet queues for small portions of code.
    _m.lock();
    while (!bDone) {
        int64 i64TimeToWait = _pMocket->getSAckTransmitTimeout();    /*!!*/ // This should be computed based on the next packet that will timeout and requires retransmission

        // NOTE: The following should be done even if the mocket has gone into a shutdown state because
        //       the other end may still be waiting for acknowledgements. This may not be needed when the
        //       mocket is in a SHUTDOWN_RECEIVED state, but it should not matter
        if (_bSendSAckInformationNow) {
            checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                            "transmitting an empty packet with SAck information due to an external request\n");
            if (0 != (rc = sendSAckPacket())) {
                checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                "sendSAckPacket() failed with rc = %d\n", rc);
            }
        }

        if (_bSendSimpleSuspendAck) {
            checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                            "transmitting an empty packet with SimpleSuspendAck due to an incoming Suspend message received\n");
            if (0 != (rc = sendSimpleSuspendAckPacket())) {
                checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                "sendSimpleSuspendAckPacket() failed with rc = %d\n", rc);
            }
            _bSendSimpleSuspendAck = false;
        }

        if (_bSendSuspendAck) {
            checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                            "transmitting an empty packet with SuspendAck due to an incoming Suspend message received\n");
            if (0 != (rc = sendSuspendAckPacket())) {
                checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                "sendSuspendAckPacket() failed with rc = %d\n", rc);
            }
            _bSendSuspendAck = false;
        }

        if (_bSendResumeAck) {
            checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                            "transmitting an empty packet with ResumeAck due to an incoming Resume message received\n");
            if (0 != (rc = sendResumeAckPacket())) {
                checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                "sendResumeAckPacket() failed with rc = %d\n", rc);
            }
            _bSendResumeAck = false;
        }

        if (_bSendReEstablishAck) {
            checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                            "transmitting an empty packet with ReEstablishAck due to an incoming reEstablish message received\n");
            if (0 != (rc = sendReEstablishAckPacket())) {
                checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                "sendReEstablishAckPacket() failed with rc = %d\n", rc);
            }
            _bSendReEstablishAck = false;
        }

        // Check the state of the connection
        const auto smCurrentState = _pMocket->getStateMachine()->getCurrentState();

        // If the state is S_SUSPEND_RECEIVED we don't want to send messages, only
        // suspend/resume related messages are allowed and they have been taken care of already
        if (smCurrentState != StateMachine::S_SUSPEND_RECEIVED) {
            // Check for packets to transmit
            bool bHavePacketsToTransmit = (!_pendingPacketQueue.isEmpty()) || (!_upqControlPackets.isEmpty()) ||
                                          (!_upqReliableSequencedPackets.isEmpty()) || (!_upqReliableUnsequencedPackets.isEmpty());

            if (smCurrentState == StateMachine::S_SHUTDOWN_SENT) {
                if (iShutdownsSent >= _pMocket->getMaxShutdownAttempts()) {
                    // Abort the connection
                    _pMocket->getStateMachine()->setClosed();
                }
                else {
                    // Send SHUTDOWN message
                    if (0 != (rc = sendShutdownPacket())) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "sendShutdownPacket() failed with rc = %d\n", rc);
                    }
                    // Increase count of sent SHUTDOWN messages
                    iShutdownsSent++;
                    // Wait for SHUTDOWN_TIMEOUT
                    i64TimeToWait = _pMocket->getShutdownTimeout();
                }
            }
            else if (smCurrentState == StateMachine::S_SHUTDOWN_ACK_SENT) {
                if (iShutdownAcksSent >= _pMocket->getMaxShutdownAttempts()) {
                    // Abort the connection
                    _pMocket->getStateMachine()->setClosed();
                }
                else {
                    // Send SHUTDOWN_ACK message
                    // printf ("Send shutdown ack\n");
                    if (0 != (rc = sendShutdownAckPacket())) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "sendShutdownAckPacket() failed with rc = %d\n", rc);
                    }
                    // Increase count of sent SHUTDOWN_ACK messages
                    iShutdownAcksSent++;
                    // Wait for SHUTDOWN_TIMEOUT
                    i64TimeToWait = _pMocket->getShutdownTimeout();
                }
            }
            else if (smCurrentState == StateMachine::S_CLOSED) {
                // Send SHUTDOWN_COMPLETE
                // Send three shutdown complete packets to improve the probability of the receiver getting it (in case of lossy channel)
                for (int i=0; i<3; i++) {
                    if (0 != (rc = sendShutdownCompletePacket())) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "sendShutdownCompletePacket() failed with rc = %d\n", rc);
                    }
                }
                _pMocket->notifyPacketProcessor();
                bDone = true;
            }
            else if (smCurrentState == StateMachine::S_APPLICATION_ABORT) {
                checkAndLogMsg ("Transmitter::run", Logger::L_LowDetailDebug,
                                "application abort sent to the trasmitter thread");
                bDone = true;
            }
            else if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPENDED) {
                bDone = true;
            }
            else if (smCurrentState == StateMachine::S_SUSPEND_SENT) {
                // Send suspend message
                if (_pMocket->areKeysExchanged()) {
                    if (0 != (rc = sendSimpleSuspendPacket())) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "sendSimpleSuspendPacket() failed with rc = %d\n", rc);
                    }
                }
                else {
                    if (0 != (rc = sendSuspendPacket())) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "sendSuspendPacket() failed with rc = %d\n", rc);
                    }
                }
                i64TimeToWait = _pMocket->getMaxSendNewSuspendResumeTimeout(); // wait until it is time to send a new suspend message
            }
            else if (bHavePacketsToTransmit) {
                uint32 ui32LingerTime = _pMocket->getConnectionLingerTime();
                // Check if we are flushing the queues
                if ((smCurrentState == StateMachine::S_SHUTDOWN_PENDING ||
                     smCurrentState == StateMachine::S_SHUTDOWN_RECEIVED) &&
                    ui32LingerTime && _i64ShutdownStartTime + ui32LingerTime < getTimeInMilliseconds()) {
                        // If linger time is expired, then close the connection
                        _pMocket->getStateMachine()->outstandingQueueFlushed(); // Enters the SHUTDOWN_SENT or the SHUTDOWN_ACK_SENT state
                        i64TimeToWait = 1; // Don't wait so that SHUTDOWN can be sent right away in the next iteration of the loop
                }
                // The connection is open go on and process the queues
                else {
                    // NOTE: we give priority to unacknowledged packets over new packets
                    if ((rc = processUnacknowledgedPacketQueues()) < 0) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "processUnacknowledgedPacketQueues() failed with rc = %d\n", rc);
                    }
                    // Get estimated number of packet to be transmitted to stay within the bandwidth limit
                    if ((rc = processPendingPacketQueue()) < 0) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "processPendingPacketQueue() failed with rc = %d\n", rc);
                    }
                    // mauro: in the Java implementation i64TimeToWait used to be the minimum value between
                    // 100 milliseconds and _outstandingPacketQueue.timeToNextRetransmission()
                    // Evaluating rc we evaluate the result of processPendingPacketQueue()
                    if (rc == 1) {
                        // Don't wait if a packet was successfully sent (there may be more packets in pending packet queue)
                        i64TimeToWait = 0;
                    }
                    else if (rc == -10) {
                        // Don't wait if a packet wasn't sent because of transmit rate limit, try again right away -or we could wait 1 ms
                        i64TimeToWait = 1;
                    }
                    else {
                        i64TimeToWait = 10;
                    }
                }
            }
            // There are no packets to be transmitted at this time
            else if ((smCurrentState == StateMachine::S_SHUTDOWN_PENDING) || (smCurrentState == StateMachine::S_SHUTDOWN_RECEIVED)) {
                // Since there are no more packets to be transmitted, proceed with shutdown
                //_i64ShutdownStartTime = getTimeInMilliseconds();
                _pMocket->getStateMachine()->outstandingQueueFlushed(); // Enters the SHUTDOWN_SENT or the SHUTDOWN_ACK_SENT state
                i64TimeToWait = 1; // Don't wait so that SHUTDOWN can be sent right away in the next iteration of the loop
            }
            else if (smCurrentState == StateMachine::S_SUSPEND_PENDING) {
                // No more data to transmit proceed with suspension
                _mFlushData.lock();
                _cvFlushData.notifyAll();
                _mFlushData.unlock();
                i64TimeToWait = 10; // Wait few milliseconds so Mocket can change to the next state of the suspension process
            }
            // Send control information if needed
            int64 i64CurrTime = getTimeInMilliseconds();
            if ((_pMocket->getCancelledTSNManager()->haveInformation()) &&
                ((_i64LastCancelledTSNTransmitTime + _pMocket->getCancelledTSNTransmitTimeout()) < i64CurrTime)) {
                checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                                "_pMocket->getCancelledTSNManager()->haveInformation() is: %d\n",
                                _pMocket->getCancelledTSNManager()->haveInformation());
                checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                                "transmitting an empty packet with Cancelled TSN information\n");
                if (0 != (rc = sendCancelledTSNPacket())) {
                    checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                    "sendCancelledTSNPacket() failed with rc = %d\n", rc);
                }
            }
            // If the timeout is expired && there are new information send an SAckPacket
            if (((_i64LastSAckTransmitTime + _pMocket->getSAckTransmitTimeout()) < i64CurrTime) &&
                (_pMocket->getACKManager()->haveNewInformationSince (_i64LastSAckTransmitTime))) {
                checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                                "transmitting an empty packet with updated SAck information\n");
                if (0 != (rc = sendSAckPacket())) {
                    checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                    "sendSAckPacket() failed with rc = %d\n", rc);
                }
                // _i64LastSAckTransmitTime will be updated by sendSAckPacket() i.e. by appendPiggybackDataAndTransmitPacket() called from sendSAckPacket
            }
            if (_pMocket->usingKeepAlive()) {
                if (((_i64LastTransmitTime + _pMocket->getKeepAliveTimeout()) < i64CurrTime) && (smCurrentState == StateMachine::S_ESTABLISHED)) {
                    checkAndLogMsg ("Transmitter::run", Logger::L_MediumDetailDebug,
                                    "transmitting a keep alive packet due to inactivity\n");
                    if (0 != (rc = sendHeartbeatPacket())) {
                        checkAndLogMsg ("Transmitter::run", Logger::L_MildError,
                                        "sendHeartbeatPacket() failed with tc = %d\n", rc);
                    }
                }
            }
            if ((i64CurrTime - _i64LastRTTEstimationTime) > _pMocket->getMaxIntervalBetweenRTTTimestamps()) {
                requestTimestampTransmission();
            }
            if ((_i64LastStatUpdateTime + _pMocket->getStatsUpdateInverval()) < i64CurrTime) {
                _pMocket->getMocketStatusNotifier()->sendStats (_pMocket->getIdentifier(), _pMocket->getStatistics());
                _i64LastStatUpdateTime = i64CurrTime;
            }
            if (i64TimeToWait > 0) {
                _cv.wait (i64TimeToWait);
            }
        }
    }
    _m.unlock();
    _pendingPacketQueue.close();
    _pMocket->transmitterTerminating();
}

int Transmitter::processSAckChunk (SAckChunkAccessor sackChunkAccessor)
{
    // Reset the Minimum acknowledgement times in the various queues
    // Compute the minimum acknowledgement time after processing the SAck chunk to update the RTT
    _upqControlPackets.resetMinAckTime();
    _upqReliableSequencedPackets.resetMinAckTime();
    _upqReliableUnsequencedPackets.resetMinAckTime();

    _ui16NumberOfAcknowledgedPackets = 0;
    uint64 ui64NumberOfAcknowledgedBytes = 0;

    uint32 ui32LastAcknowledgedControlPacket = sackChunkAccessor.getControlCumulateAck();
    uint32 ui32LastAcknowledgedReliableSequencedPacket = sackChunkAccessor.getReliableSequencedCumulateAck();
    uint32 ui32LastAcknowledgedReliableUnsequencedPacket = sackChunkAccessor.getReliableUnsequencedCumulativeAck();

    // Check if Receiver Bandwidth Estimation is active
    int64 i64Timestamp = 0;
    i64Timestamp = sackChunkAccessor.getTimestamp();
    if (i64Timestamp != 0) {
        uint32 ui32BytesReceived = 0;
        ui32BytesReceived = sackChunkAccessor.getBytesReceived();
        // Estimate the bandwidth
        int32 i32Interval = (int32)(i64Timestamp - _i64LastRecTimeTimestamp);
        _i64LastRecTimeTimestamp = i64Timestamp;
        if (i32Interval > 0) {
            uint32 ui32BytesRecInInterval = 0;
            if (ui32BytesReceived < _ui32RecSideBytesReceived) {
                ui32BytesRecInInterval = UINT32_MAX_VALUE - _ui32RecSideBytesReceived + ui32BytesReceived;
            }
            else {
                ui32BytesRecInInterval = ui32BytesReceived - _ui32RecSideBytesReceived;
            }
            _ui32RecSideBytesReceived = ui32BytesReceived;
            // Bytes per ms
            int32 i32BytesPerSec = ui32BytesRecInInterval * 1000 / i32Interval;
            checkAndLogMsg ("Transmitter::processSAckChunk", Logger::L_MediumDetailDebug,
                            "usingRecBandEst timestamp=%lld interval %u bytes received=%u Bytes per sec=%d\n", i64Timestamp, i32Interval, ui32BytesReceived, i32BytesPerSec);
        }
        else {
            checkAndLogMsg ("Transmitter::processSAckChunk", Logger::L_MediumDetailDebug,
                            "usingRecBandEst timestamp=%lld bytes received=%u - same timestamp!!\n", i64Timestamp, ui32BytesReceived);
        }
    }


    if (_pMocket->usingFastRetransmit()) {
        _pFastRetransmitControlPackets = new DLList<uint32>(0);
        _pFastRetransmitReliableSequencedPackets = new DLList<uint32>(0);
        _pFastRetransmitReliableUnsequencedPackets = new DLList<uint32>(0);
    }

    int temp = 0;
    _ui16NumberOfAcknowledgedPackets += _upqControlPackets.acknowledgePacketsUpto (ui32LastAcknowledgedControlPacket, ui64NumberOfAcknowledgedBytes);
    if (temp != _ui16NumberOfAcknowledgedPackets) {
        checkAndLogMsg ("Transmitter::processSAckChunk", Logger::L_MediumDetailDebug,
                        "c (%u)\n", ui32LastAcknowledgedReliableSequencedPacket);
        temp = _ui16NumberOfAcknowledgedPackets;
    }
    _ui16NumberOfAcknowledgedPackets += _upqReliableSequencedPackets.acknowledgePacketsUpto (ui32LastAcknowledgedReliableSequencedPacket, ui64NumberOfAcknowledgedBytes);
    if (temp != _ui16NumberOfAcknowledgedPackets) {
        checkAndLogMsg ("Transmitter::processSAckChunk", Logger::L_MediumDetailDebug,
                        "rs (%u)\n", ui32LastAcknowledgedReliableSequencedPacket);
        temp = _ui16NumberOfAcknowledgedPackets;
    }
    _ui16NumberOfAcknowledgedPackets += _upqReliableUnsequencedPackets.acknowledgePacketsUpto (ui32LastAcknowledgedReliableUnsequencedPacket, ui64NumberOfAcknowledgedBytes);
    if (temp != _ui16NumberOfAcknowledgedPackets) {
        checkAndLogMsg ("Transmitter::processSAckChunk", Logger::L_MediumDetailDebug,
                        "ru (%u)\n", ui32LastAcknowledgedReliableSequencedPacket);
        temp = _ui16NumberOfAcknowledgedPackets;
    }

    while (sackChunkAccessor.haveMoreBlocks()) {
        while (sackChunkAccessor.haveMoreElements()) {
            switch (sackChunkAccessor.getBlockType()) {
                case SAckChunkAccessor::BT_RANGE_CONTROL:
                    _ui16NumberOfAcknowledgedPackets += _upqControlPackets.acknowledgePacketsWithin (sackChunkAccessor.getStartTSN(), sackChunkAccessor.getEndTSN(), ui64NumberOfAcknowledgedBytes);
                    checkAndLogMsg ("Transmitter::processSAckChunk", Logger::L_MediumDetailDebug,
                                    "c (%u) %u - %u\n", ui32LastAcknowledgedControlPacket, sackChunkAccessor.getStartTSN(), sackChunkAccessor.getEndTSN());
                    if (_pMocket->usingFastRetransmit()) {
                        for (uint32 i=ui32LastAcknowledgedControlPacket+1; i<sackChunkAccessor.getStartTSN(); i++) {
                            _pFastRetransmitControlPackets->pushTail(i);
                        }
                        // Erika: this way we may be retransmitting packets that have been acknowledged singularly
                        // (not a range) since these are at the end of the range chuncks in an SACK.
                    }
                    break;
                case SAckChunkAccessor::BT_SINGLE_CONTROL:
                    _ui16NumberOfAcknowledgedPackets += _upqControlPackets.acknowledgePacketsWithin (sackChunkAccessor.getTSN(), sackChunkAccessor.getTSN(), ui64NumberOfAcknowledgedBytes);
                    checkAndLogMsg ("Transmitter::processSAckChuck", Logger::L_MediumDetailDebug,
                                    "c (%u) %u\n", ui32LastAcknowledgedControlPacket, sackChunkAccessor.getTSN());
                    break;
                case SAckChunkAccessor::BT_RANGE_RELIABLE_SEQUENCED:
                    _ui16NumberOfAcknowledgedPackets += _upqReliableSequencedPackets.acknowledgePacketsWithin (sackChunkAccessor.getStartTSN(), sackChunkAccessor.getEndTSN(), ui64NumberOfAcknowledgedBytes);
                    checkAndLogMsg ("Transmitter::processSAckChuck", Logger::L_MediumDetailDebug,
                                    "rs (%u) %u - %u\n", ui32LastAcknowledgedReliableSequencedPacket, sackChunkAccessor.getStartTSN(), sackChunkAccessor.getEndTSN());
                    if (_pMocket->usingFastRetransmit()) {
                        for (uint32 i=ui32LastAcknowledgedReliableSequencedPacket+1; i<sackChunkAccessor.getStartTSN(); i++) {
                           _pFastRetransmitReliableSequencedPackets->pushTail(i);
                        }
                        ui32LastAcknowledgedReliableSequencedPacket = sackChunkAccessor.getEndTSN();
                        // Erika: this way we may be retransmitting packets that have been acknowledged singularly
                        // (not a range) since these are at the end of the range chunks in an SACK.
                    }
                    break;
                case SAckChunkAccessor::BT_SINGLE_RELIABLE_SEQUENCED:
                    _ui16NumberOfAcknowledgedPackets += _upqReliableSequencedPackets.acknowledgePacketsWithin (sackChunkAccessor.getTSN(), sackChunkAccessor.getTSN(), ui64NumberOfAcknowledgedBytes);
                    checkAndLogMsg ("Transmitter::processSAckChuck", Logger::L_MediumDetailDebug,
                                    "rs (%u) %u\n", ui32LastAcknowledgedReliableSequencedPacket, sackChunkAccessor.getTSN());
                    break;
                case SAckChunkAccessor::BT_RANGE_RELIABLE_UNSEQUENCED:
                    _ui16NumberOfAcknowledgedPackets += _upqReliableUnsequencedPackets.acknowledgePacketsWithin (sackChunkAccessor.getStartTSN(), sackChunkAccessor.getEndTSN(), ui64NumberOfAcknowledgedBytes);
                    checkAndLogMsg ("Transmitter::processSAckChuck", Logger::L_MediumDetailDebug,
                                    "ru (%u) %u - %u\n", ui32LastAcknowledgedReliableUnsequencedPacket, sackChunkAccessor.getStartTSN(), sackChunkAccessor.getEndTSN());
                    if (_pMocket->usingFastRetransmit()) {
                        for (uint32 i=ui32LastAcknowledgedReliableUnsequencedPacket+1; i<sackChunkAccessor.getStartTSN(); i++) {
                            _pFastRetransmitReliableUnsequencedPackets->pushTail(i);
                        }
                        ui32LastAcknowledgedReliableUnsequencedPacket = sackChunkAccessor.getEndTSN();
                        // Erika: this way we may be retransmitting packets that have been acknowledged singularly
                        // (not a range) since these are at the end of the range chuncks in an SACK.
                    }
                    break;
                case SAckChunkAccessor::BT_SINGLE_RELIABLE_UNSEQUENCED:
                    _ui16NumberOfAcknowledgedPackets += _upqReliableUnsequencedPackets.acknowledgePacketsWithin (sackChunkAccessor.getTSN(), sackChunkAccessor.getTSN(), ui64NumberOfAcknowledgedBytes);
                    checkAndLogMsg ("Transmitter::processSAckChuck", Logger::L_MediumDetailDebug,
                                    "ru (%u) %u\n", ui32LastAcknowledgedReliableUnsequencedPacket, sackChunkAccessor.getTSN());
                    break;
            }
            sackChunkAccessor.advanceToNextElement();
        }
        sackChunkAccessor.advanceToNextBlock();
    }

    checkAndLogMsg ("Transmitter::processSAckChuck", Logger::L_MediumDetailDebug,
                    "%d packets acknowledged\n", _ui16NumberOfAcknowledgedPackets);
    if (_pMocket->usingFastRetransmit()) {
        uint32 ui32SeqNum;
        _pFastRetransmitControlPackets->resetToTail();
        while (_pFastRetransmitControlPackets->getPrev(ui32SeqNum)) {
            _upqControlPackets.prioritizeRetransmissionOfPacket(ui32SeqNum);
        }
        _pFastRetransmitReliableSequencedPackets->resetToTail();
        while (_pFastRetransmitReliableSequencedPackets->getPrev(ui32SeqNum)) {
            _upqReliableSequencedPackets.prioritizeRetransmissionOfPacket(ui32SeqNum);
        }
        _pFastRetransmitReliableUnsequencedPackets->resetToTail();
        while (_pFastRetransmitReliableUnsequencedPackets->getPrev(ui32SeqNum)) {
            _upqReliableUnsequencedPackets.prioritizeRetransmissionOfPacket(ui32SeqNum);
        }

        delete _pFastRetransmitControlPackets;
        _pFastRetransmitControlPackets = nullptr;
        delete _pFastRetransmitReliableSequencedPackets;
        _pFastRetransmitReliableSequencedPackets = nullptr;
        delete _pFastRetransmitReliableUnsequencedPackets;
        _pFastRetransmitReliableUnsequencedPackets = nullptr;
    }

    // Update Statistics
    _pMocket->getStatistics()->_ui32ReliableSequencedDataSize = _upqReliableSequencedPackets.getQueuedDataSize();
    _pMocket->getStatistics()->_ui32ReliableSequencedPacketQueueSize = _upqReliableSequencedPackets.getPacketCount();
    _pMocket->getStatistics()->_ui32ReliableUnsequencedDataSize = _upqReliableUnsequencedPackets.getQueuedDataSize();
    _pMocket->getStatistics()->_ui32ReliableUnsequencedPacketQueueSize = _upqReliableUnsequencedPackets.getPacketCount();

    if (_pBandwidthEstimator != nullptr) {
        // Fetch the number of packets in pending packet queue to use for the next
        // bandwidth estimation: if few packets are in the queue it means the sender
        // is not sending at full speed which means this sample should have little weight in the bandwidth estimation.
        uint32 ui32PacketsInQueue = _pendingPacketQueue.getPacketsInQueue();
        uint64 ui64SAckTimestamp = getTimeInMilliseconds();
        int32 i32BandwidthEstimation = _pBandwidthEstimator->addSample (ui64SAckTimestamp, ui64NumberOfAcknowledgedBytes, ui32PacketsInQueue);
        _pMocket->getStatistics()->setEstimatedBandwidth (i32BandwidthEstimation);
        checkAndLogMsg ("Transmitter::processSAckChuck", Logger::L_MediumDetailDebug,
                        "estimated new bandwidth %d Bps; PPQ length %u; Timestamp %llu\n",
                        i32BandwidthEstimation, ui32PacketsInQueue, ui64SAckTimestamp);
    }

    // Update Congestion Control
    if(_pCongestionControl != nullptr) {
        _pCongestionControl->update();
    }

    // Compute the minimum acknowledgement time
    uint32 ui32MinAckTime = 0xFFFFFFFFUL;
    if (_upqControlPackets.getMinAckTime() < ui32MinAckTime) {
        ui32MinAckTime = _upqControlPackets.getMinAckTime();
    }
    if (_upqReliableSequencedPackets.getMinAckTime() < ui32MinAckTime) {
        ui32MinAckTime = _upqReliableSequencedPackets.getMinAckTime();
    }
    if (_upqReliableUnsequencedPackets.getMinAckTime() < ui32MinAckTime) {
        ui32MinAckTime = _upqReliableUnsequencedPackets.getMinAckTime();
    }

    if (ui32MinAckTime != 0xFFFFFFFFUL) {
        computeAckBasedRTT (ui32MinAckTime);
    }

    // Notify: the run may be blocked because the window of the receiver was closed.
    // Wake it up after the sack has been processed
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
    return 0;
}

int Transmitter::processTimestampChunk (TimestampChunkAccessor tsChunkAccessor)
{
    _bSendTimestampAck = true;
    _i64Timestamp = tsChunkAccessor.getTimestamp();
    _i64TimestampReceiveTime = getTimeInMilliseconds();
    return 0;
}

int Transmitter::processTimestampAckChunk (TimestampAckChunkAccessor tsaChunkAccessor)
{
    computeTimestampBasedRTT (tsaChunkAccessor.getTimestamp());
    return 0;
}

bool Transmitter::waitForFlush (void)
{
    // Enters the SUSPEND_PENDING state
    _pMocket->getStateMachine()->suspend();
    if (_pMocket->getMaxFlushDataTimeout() == 0) {
        return true;
    }
    // Wait for flushing data or timeout
    _mFlushData.lock();
    _cvFlushData.wait(_pMocket->getMaxFlushDataTimeout());
    _mFlushData.unlock();
    if (_pMocket->getStateMachine()->getCurrentState() != StateMachine::S_SUSPEND_PENDING) {
        // If mocket receives a shutdown message while flushing queue it goes in SHUTDOWN_PENDING state
        // If it receives a suspend message from the other side it goes in SUSPEND_RECEIVED
        // return an error during suspension process to the application
        return false;
    }
    return true;
}

bool Transmitter::suspend (void)
{
    // Enters the SUSPEND_SENT state
    if (!_pMocket->getStateMachine()->queueFlushedOrTimeout()) {
        return false;
    }
    _mSuspend.lock();
    _cvSuspend.wait (_pMocket->getMaxSuspendTimeout());
    _mSuspend.unlock();
    if (_pMocket->getStateMachine()->getCurrentState() != StateMachine::S_SUSPENDED) {
        //printf ("Transmitter::suspend timeout expired while waiting suspend_ack\n");
        return false;
    }
    return true;
}

void Transmitter::processSimpleSuspendPacket (SimpleSuspendChunkAccessor simpleSuspendChunkAccessor)
{
    if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_RECEIVED) {
        // Received simpleSuspend message when already suspended
        // so the simpleSuspend_ack has been lost we need to send another one
        //printf ("Transmitter::processSimpleSuspendPacket already suspended call requestSimpleSuspendAckTransmission()\n");
        requestSimpleSuspendAckTransmission();
        return;
    }
    // Check for simultaneous suspension
    if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_SENT) {
        //printf ("Transmitter::processSimpleSuspendPacket simultaneous suspension\n");
        int iSimultaneousSuspension = _pMocket->resolveSimultaneousSuspension();
        // Local node continue the suspension
        if (iSimultaneousSuspension == 0) {
            // Continue to wait for suspend_ack message
            // No need to force to send suspend, Transmitter will take care of that
            return;
        }
        // No way to resolve the conflict
        else if (iSimultaneousSuspension == -1) {
            // SuspendTimeout will expire and so the suspension process will end with an error
            checkAndLogMsg ("Transmitter::processSimpleSuspendPacket", Logger::L_MildError,
                            "simultaneous suspension, no way to solve the conflict\n");
            return;
        }
    }

    // Local node is being suspended
    // Enters the SUSPEND_RECEIVED state
    if (!_pMocket->getStateMachine()->receivedSuspend()) {
        //**// Ignore the message, not in a consistent state to suspend
        checkAndLogMsg ("Transmitter::processSimpleSuspendPacket", Logger::L_MildError,
                        "not in a consistent state for being suspended\n");
        return;
    }

    // Proceed with the suspension process
    requestSimpleSuspendAckTransmission();
    return;
}

void Transmitter::processSimpleSuspendAckPacket (SimpleSuspendAckChunkAccessor simpleSuspendAckChunkAccessor)
{
    if (!_pMocket->getStateMachine()->receivedSuspendAck()) {
        //**// Ignore the message, not in a consistent state
        checkAndLogMsg ("Transmitter::processSuspendAckPacket", Logger::L_MildError,
                        "not in a consistent state\n");
        return;
    }
    // Wake up the thread blocked
    _mSuspend.lock();
    _cvSuspend.notifyAll();
    _mSuspend.unlock();
    return;
}

void Transmitter::processSuspendPacket (SuspendChunkAccessor suspendChunkAccessor)
{
    #ifdef MOCKETS_NO_CRYPTO
        checkAndLogMsg ("Mocket::processSuspendPacket", Logger::L_SevereError,
                        "crypto disabled at build time\n");
    #else
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_RECEIVED) {
            // Received suspend message when already suspended
            // so the suspend_ack has been lost we need to send another one
            //printf ("Transmitter::processSuspendPacket already suspended call requestSuspendAckTransmission()\n");
            requestSuspendAckTransmission();
            return;
        }
        // Check for simultaneous suspension
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_SENT) {
            //printf ("Transmitter::processSuspendPacket simultaneous suspension\n");
            int iSimultaneousSuspension = _pMocket->resolveSimultaneousSuspension();
            // Local node continue the suspension
            if (iSimultaneousSuspension == 0) {
                // Continue to wait for suspend_ack message
                // No need to force to send suspend, Transmitter will take care of that
                return;
            }
            // No way to resolve the conflict
            else if (iSimultaneousSuspension == -1) {
                // SuspendTimeout will expire and so the suspension process will end with an error
                checkAndLogMsg ("Transmitter::processSuspendPacket", Logger::L_MildError,
                                "simultaneous suspension, no way to solve the conflict\n");
                return;
            }
        }

        // Local node is being suspended
        //printf ("Transmitter::processSuspendPacket local node is being suspended\n");
        // Reconstruct and save the public key received with the Suspend packet
        uint32 ui32KeyLength = suspendChunkAccessor.getKeyLength();
        //printf ("ui32KeyLength = %lu\n", ui32KeyLength);
        const char *pKeyDataBuff = suspendChunkAccessor.getKeyData();

        PublicKey *pPubKey = new PublicKey();
        Key::KeyData *pKeyData = new Key::KeyData (Key::KeyData::X509, pKeyDataBuff, ui32KeyLength);

        if (0 != pPubKey->setKeyFromDEREncodedX509Data (pKeyData)) {
            checkAndLogMsg ("Transmitter::processSuspendPacket", Logger::L_MildError,
                            "unable to create the public key\n");
            return;
        }

        delete pKeyData;
        pKeyData = nullptr;

        //**// Print the received key reconstructed
        //pPubKey->storeKeyAsDEREncodedX509Data("publicKeyRecReconstr.txt");

        _pMocket->_pKeyPair = new PublicKeyPair();
        // Save the key
        _pMocket->_pKeyPair->setPublicKey(pPubKey);

        pPubKey = nullptr;

        // Create a new UUID for the current mocket
        _pMocket->newMocketUUID();

        // Create a new secret key
        _pMocket->newSecretKey();

        // Enters the SUSPEND_RECEIVED state
        if (!_pMocket->getStateMachine()->receivedSuspend()) {
            //**// Ignore the message, not in a consistent state to suspend
            checkAndLogMsg ("Transmitter::processSuspendPacket", Logger::L_MildError,
                            "not in a consistent state for being suspended\n");
            return;
        }

        // Proceed with the suspension process
        requestSuspendAckTransmission();
        return;
    #endif
}

void Transmitter::processSuspendAckPacket (SuspendAckChunkAccessor suspendAckChunkAccessor)
{
    #ifdef MOCKETS_NO_CRYPTO
        checkAndLogMsg ("Mocket::processSuspendAckPacket", Logger::L_SevereError,
                        "crypto disabled at build time\n");
    #else
        //**// Extract, decrypt and save UUID and password to create Ks
        // Extract
        uint32 ui32EncryptedDataLength = suspendAckChunkAccessor.getEncryptedDataLength();
        const char *pDataBuff = suspendAckChunkAccessor.getEncryptedData();

        // Decrypt
        uint32 ui32ReceivedDataLength;
        void *pReceivedData;
        pReceivedData = CryptoUtils::decryptDataUsingPrivateKey (_pMocket->_pKeyPair->getPrivateKey(), pDataBuff, ui32EncryptedDataLength, &ui32ReceivedDataLength);

        // Extract and save the password
        // No need to create the new secret key from the password received
        // the mocket will be frozen and the secret key will be created in resumeFromSuspension()
        uint32 ui32BuffPos = 0;
        char pwd[9];
        memcpy (pwd, pReceivedData, 9);
        _pMocket->setPassword (pwd);
        ui32BuffPos += 9;

        // Extract and save the UUID
        uint32 ui32UUID = *((uint32*)(((char*)pReceivedData) + ui32BuffPos));
        _pMocket->setMocketUUID (ui32UUID);

        if (!_pMocket->getStateMachine()->receivedSuspendAck()) {
            //**// Ignore the message, not in a consistent state
            checkAndLogMsg ("Transmitter::processSuspendAckPacket", Logger::L_MildError,
                            "not in a consistent state\n");
            return;
        }
        // Wake up the thread blocked
        _mSuspend.lock();
        _cvSuspend.notifyAll();
        _mSuspend.unlock();
        return;
    #endif
}

void Transmitter::processResumePacket (ResumeChunkAccessor resumeChunkAccessor, uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort)
{
    #ifdef MOCKETS_NO_CRYPTO
         checkAndLogMsg ("Mocket::processResumePacket", Logger::L_SevereError,
                         "crypto disabled at build time\n");
    #else
        //printf ("* REMOTE ip %lu * port %d *\n", ui32NewRemoteAddress, ui16NewRemotePort);
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_ESTABLISHED) {
            // Already established, send a new resume_ack
            requestResumeAckTransmission();
            //printf ("Transmitter::processResumePacket already established requestResumeAckTransmission\n");
            return;
        }
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_SUSPEND_RECEIVED) {
            // Extract the UUID
            uint32 ui32EncryptedUUIDLength = resumeChunkAccessor.getEncryptedNonceLength();
            const char *pNonceDataBuff = resumeChunkAccessor.getEncryptedNonce();
            // Decrypt the UUID
            uint32 ui32ReceivedNonceLength;
            void *pReceivedData = CryptoUtils::decryptDataUsingSecretKey (_pMocket->_pSecretKey, pNonceDataBuff, ui32EncryptedUUIDLength, &ui32ReceivedNonceLength);

            uint32 ui32BuffPos = 0;
            // Extract and save the UUID
            uint32 ui32ReceivedNonce = *((uint32*)((char*)pReceivedData));
            ui32BuffPos += 4;
            //printf ("Received UUID %lu\n", ui32ReceivedNonce);
            // Extract and save the IP
            uint32 ui32ReceivedIP = *((uint32*)(((char*)pReceivedData) + ui32BuffPos));
            ui32BuffPos += 4;
            // Extract and save the port
            uint16 ui16ReceivedPort = *((uint32*)(((char*)pReceivedData) + ui32BuffPos));

            // Check the nonce (UUID), IP and port
            if ((ui32ReceivedNonce == _pMocket->getMocketUUID()) /*&& (ui32ReceivedIP == ui32NewRemoteAddress)*/ && (ui16ReceivedPort == ui16NewRemotePort)) {
                // Received UUID is equal to the one sent and encrypted [IP, port] equal to the one in the packet
                // so we can go on with resume process
                // We have received a valid resume packet from a new endpoint we need to save the address
                _ui32RemoteAddress = ui32NewRemoteAddress;
                _ui16RemotePort = ui16NewRemotePort;
                _pMocket->resetRemoteAddress (ui32NewRemoteAddress, ui16NewRemotePort);
                _pMocket->getReceiver()->resetRemoteAddress (ui32NewRemoteAddress, ui16NewRemotePort);
                _pMocket->getStateMachine()->receivedResume();
                requestResumeAckTransmission();
            }
            else {
                // Error: the nonce received is not the one sent
                checkAndLogMsg ("Transmitter::processResumePacket", Logger::L_MildError,
                                "the received nonce is not the one sent, or IP, port are not the one in the UDP packet\n");
            }
            //free pReceivedData;
        }
        // Ignore this message if it is received in a state different from ESTABLISHED or SUSPEND_RECEIVED
        return;
    #endif
}

void Transmitter::processReEstablishPacket (ReEstablishChunkAccessor reEstablishChunkAccessor, uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort)
{
    #ifdef MOCKETS_NO_CRYPTO
        checkAndLogMsg ("Mocket::processReEstablishPacket", Logger::L_SevereError,
                        "crypto disabled at build time\n");
    #else
        //printf ("* REMOTE ip %lu * port %d *\n", ui32NewRemoteAddress, ui16NewRemotePort);
        // TODO: check if security information are set
        if (_pMocket->_pSecretKey == nullptr) {
            checkAndLogMsg ("Transmitter::processReEstablishPacket", Logger::L_MildError,
                            "secret key non exchanged: illegal reconnection\n");
            return;
        }

        // Extract the UUID
        uint32 ui32EncryptedUUIDLength = reEstablishChunkAccessor.getEncryptedNonceLength();
        const char *pNonceDataBuff = reEstablishChunkAccessor.getEncryptedNonce();
        // Decrypt the UUID
        uint32 ui32ReceivedNonceLength;
        void *pReceivedData = CryptoUtils::decryptDataUsingSecretKey (_pMocket->_pSecretKey, pNonceDataBuff, ui32EncryptedUUIDLength, &ui32ReceivedNonceLength);

        uint32 ui32BuffPos = 0;
        // Extract and save the UUID
        uint32 ui32ReceivedNonce = *((uint32*)((char*)pReceivedData));
        ui32BuffPos += 4;
        // Extract and save the IP
        uint32 ui32ReceivedIP = *((uint32*)(((char*)pReceivedData) + ui32BuffPos));
        ui32BuffPos += 4;
        // Extract and save the port
        uint16 ui16ReceivedPort = *((uint32*)(((char*)pReceivedData) + ui32BuffPos));

        // TODO: Check if we are already connected to this IP and port the ReEstablishAck was lost. Send it again with no additional processing

        // Check the nonce (UUID), IP and port
        if ((ui32ReceivedNonce == _pMocket->getMocketUUID()) /*&& (ui32ReceivedIP == ui32NewRemoteAddress)*/ && (ui16ReceivedPort == ui16NewRemotePort)) {
            // Received UUID is equal to the one sent and encrypted [IP, port] equal to the one in the packet
            // so we can go on with resume process
            // We have received a valid resume packet from a new endpoint we need to save the address
            _ui32RemoteAddress = ui32NewRemoteAddress;
            _ui16RemotePort = ui16NewRemotePort;
            _pMocket->resetRemoteAddress (ui32NewRemoteAddress, ui16NewRemotePort);
            _pMocket->getReceiver()->resetRemoteAddress (ui32NewRemoteAddress, ui16NewRemotePort);
            requestReEstablishAckTransmission();
        }
        else {
            // Error: the nonce received is not the one sent
            checkAndLogMsg ("Transmitter::processReEstablishPacket", Logger::L_MildError,
                            "the received nonce is not the one sent, or IP, port are not the one in the UDP packet\n");
        }
        //free pReceivedData;
        return;
    #endif
}

int Transmitter::freeze (ObjectFreezer &objectFreezer)
{
    objectFreezer.beginNewObject ("Transmitter");
    objectFreezer.putUInt32 (_ui32RemoteWindowSize);
    objectFreezer.putUInt32 (_ui32MessageTSN);
    objectFreezer.putBool (_bSendingFragmentedMsg);
    objectFreezer.putUInt32 (_ui32SendingMessageTSN);
    objectFreezer.putUInt32 (_ui32ControlTSN);
    objectFreezer.putUInt32 (_ui32ReliableSequencedTSN);
    objectFreezer.putUInt32 (_ui32UnreliableSequencedTSN);
    objectFreezer.putUInt32 (_ui32ReliableUnsequencedID);
    objectFreezer.putUInt32 (_ui32UnreliableUnsequencedID);
    objectFreezer.putFloat (_fSRTT);

/*    printf ("Transmitter\n");
    printf ("_ui32RemoteWindowSize %lu\n", _ui32RemoteWindowSize);
    printf ("_ui32ControlTSN %lu\n", _ui32ControlTSN);
    printf ("_ui32ReliableSequencedTSN %lu\n", _ui32ReliableSequencedTSN);
    printf ("_ui32UnreliableSequencedTSN %lu\n", _ui32UnreliableSequencedTSN);
    printf ("_ui32ReliableUnsequencedID %lu\n", _ui32ReliableUnsequencedID);
    printf ("_ui32UnreliableUnsequencedID %lu\n", _ui32UnreliableUnsequencedID);
    printf ("_fSRTT %.2f\n", _fSRTT);*/

    if (0 != _pendingPacketQueue.freeze (objectFreezer)) {
        // return -1 is if objectFreezer.endObject() don't end with success
        return -2;
    }
    if (0 != _upqControlPackets.freeze (objectFreezer)) {
        return -3;
    }
    if (0 != _upqReliableSequencedPackets.freeze (objectFreezer)) {
        return -4;
    }
    if (0 != _upqReliableUnsequencedPackets.freeze (objectFreezer)) {
        return -5;
    }
    if (0 != _resLimits.freeze (objectFreezer)) {
        return -6;
    }

    return objectFreezer.endObject();
}

int Transmitter::defrost (ObjectDefroster &objectDefroster)
{
    objectDefroster.beginNewObject ("Transmitter");
    objectDefroster >> _ui32RemoteWindowSize;
    objectDefroster >> _ui32MessageTSN;
    objectDefroster >> _bSendingFragmentedMsg;
    objectDefroster >> _ui32SendingMessageTSN;
    objectDefroster >> _ui32ControlTSN;
    objectDefroster >> _ui32ReliableSequencedTSN;
    objectDefroster >> _ui32UnreliableSequencedTSN;
    objectDefroster >> _ui32ReliableUnsequencedID;
    objectDefroster >> _ui32UnreliableUnsequencedID;
    objectDefroster >> _fSRTT;

/*    printf ("Transmitter\n");
    printf ("_ui32RemoteWindowSize %lu\n", _ui32RemoteWindowSize);
    printf ("_ui32ControlTSN %lu\n", _ui32ControlTSN);
    printf ("_ui32ReliableSequencedTSN %lu\n", _ui32ReliableSequencedTSN);
    printf ("_ui32UnreliableSequencedTSN %lu\n", _ui32UnreliableSequencedTSN);
    printf ("_ui32ReliableUnsequencedID %lu\n", _ui32ReliableUnsequencedID);
    printf ("_fSRTT %.2f\n", _fSRTT);*/

    if (0 != _pendingPacketQueue.defrost (objectDefroster)) {
        return -2;
    }
    if (0 != _upqControlPackets.defrost (objectDefroster)) {
        return -3;
    }
    if (0 != _upqReliableSequencedPackets.defrost (objectDefroster)) {
        return -4;
    }
    if (0 != _upqReliableUnsequencedPackets.defrost (objectDefroster)) {
        return -5;
    }
    if (0 != _resLimits.defrost (objectDefroster)) {
        return -6;
    }

    return objectDefroster.endObject();
}

int Transmitter::computeAckBasedRTT (uint32 ui32MinAckTime)
{
    static const float ALPHA = 0.875;
    _fSRTT = ALPHA * _fSRTT + (1.0f - ALPHA) * ((float) ui32MinAckTime);
    if (_fSRTT < 1) {
        _fSRTT = (float) 1;
    }
    _pMocket->getStatistics()->_fSRTT = _fSRTT;
    _i64LastRTTEstimationTime = getTimeInMilliseconds();
    checkAndLogMsg ("Transmitter::computeAckBasedRTT", Logger::L_MediumDetailDebug,
                    "updated Estimated RTT to %.2f based on new min ack time of %lu\n",
                    _fSRTT, ui32MinAckTime);
    return 0;
}

int Transmitter::computeTimestampBasedRTT (int64 i64Timestamp)
{
    static const float ALPHA = 0.875;
    int64 i64RTT = getTimeInMilliseconds() - i64Timestamp;
    if (i64RTT < 0) {
        return -1;
    }
    _fSRTT = ALPHA * _fSRTT + (1.0f - ALPHA) * ((float) i64RTT);
    if (_fSRTT < 1) {
        _fSRTT = (float) 1;
    }
    _pMocket->getStatistics()->_fSRTT = _fSRTT;
    _i64LastRTTEstimationTime = getTimeInMilliseconds();
    checkAndLogMsg ("Transmitter::computeTimestampBasedRTT", Logger::L_MediumDetailDebug,
                    "updated Estimated RTT to %.2f based on new timestamp-based RTT of %lu\n",
                    _fSRTT, (uint32) i64RTT);
    return 0;
}

bool Transmitter::allowedToSend (void)
{
    if (_resLimits.ui32RateLimit == 0) {
        // There is no bandwidth limitation
        return true;
    }
    // Small bandwidth limit. Mechanism: the time to send the next message has
    // been calculated given the bandwidth limit and the size of the last message
    // sent. I.e. it takes: message_size*1000/rate_limit
    if (_resLimits.ui32RateLimit < _pMocket->BANDWIDTH_LIMITATION_THRESHOLD) {
        if (getTimeInMilliseconds() >= _i64NextTimeToTransmit) {
            return true;
        }
    }
    // Large bandwidth limit. Mechanism: check TimeIntervalAverage _pByteSentPerInterval
    // to see if it is possible to send now
    else {
        if (_resLimits.ui32BytesPerInterval > _pByteSentPerInterval->getSum()) {
            checkAndLogMsg ("Transmitter::allowedToSend", Logger::L_MediumDetailDebug, "allowed bytes per interval %d sent bytes per interval %d\n", _resLimits.ui32BytesPerInterval, _pByteSentPerInterval->getSum());
            return true;
        }
        checkAndLogMsg ("Transmitter::allowedToSend", Logger::L_MediumDetailDebug, "allowed bytes per interval %d sent bytes per interval %d\n", _resLimits.ui32BytesPerInterval, _pByteSentPerInterval->getSum());
    }
    checkAndLogMsg ("Transmitter::allowedToSend", Logger::L_MediumDetailDebug, "Reached bandwidth limit: cannot send now!\n");
    return false;
}

int Transmitter::resetSRTT (void)
{
    _fSRTT = (float)_pMocket->getInitialAssumedRTT();
    _pMocket->getStatistics()->_fSRTT = _fSRTT;
    return 0;
}

int Transmitter::resetUnackPacketsRetransmitTimeoutRetransmitCount (uint32 ui32RetransmitTO)
{
    _upqControlPackets.lock();
    _upqControlPackets.resetRetrTimeoutRetrCount (ui32RetransmitTO);
    _upqControlPackets.unlock();

    _upqReliableSequencedPackets.lock();
    _upqReliableSequencedPackets.resetRetrTimeoutRetrCount (ui32RetransmitTO);
    _upqReliableSequencedPackets.unlock();

    _upqReliableUnsequencedPackets.lock();
    _upqReliableUnsequencedPackets.resetRetrTimeoutRetrCount (ui32RetransmitTO);
    _upqReliableUnsequencedPackets.unlock();

    return 0;
}

int Transmitter::processPendingPacketQueue (void)
{
    _pendingPacketQueue.lock();
    if (!_pendingPacketQueue.isEmpty()) {
        if (allowedToSend()) {
            PacketWrapper *pWrapper = _pendingPacketQueue.peek();
            if (!_bSendingFragmentedMsg) {
                if (pWrapper->getPacket()->isFirstFragment()) {
                    _bSendingFragmentedMsg = true;
                    _ui32SendingMessageTSN = pWrapper->getMessageTSN();
                }
            }
            else {      // we are sending a fragmented msg
                if (pWrapper->getPacket()->isLastFragment()) {
                    if (pWrapper->getMessageTSN() != _ui32SendingMessageTSN) {
                        checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_SevereError,
                                        "trying to transmit a fragment that does not match the current packet TSN\n");
                        _pendingPacketQueue.unlock();
                        return -1;
                    }
                    _bSendingFragmentedMsg = false;
                }
                else if (pWrapper->getPacket()->isIntermediateFragment()) {
                    if (pWrapper->getMessageTSN() != _ui32SendingMessageTSN) {
                        checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_SevereError,
                                        "trying to transmit a fragment that does not match the current packet TSN\n");
                        _pendingPacketQueue.unlock();
                        return -2;
                    }
                }
            }

            uint32 ui32TotalQueuedDataSize = _upqControlPackets.getQueuedDataSize() +
                                            _upqReliableSequencedPackets.getQueuedDataSize() +
                                            _upqReliableUnsequencedPackets.getQueuedDataSize();
            _lckRemoteWindowSize.lock();
            uint32 ui32SpaceAvailable = _ui32RemoteWindowSize;
            if( _pCongestionControl != nullptr) {
                ui32SpaceAvailable = _pCongestionControl->adaptToCongestionWindow (ui32SpaceAvailable);
                checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_MediumDetailDebug,
                                "congestion window applied, space available is %lu; remote window size is %lu\n",
                                ui32SpaceAvailable, _ui32RemoteWindowSize);
            }
            _lckRemoteWindowSize.unlock();
            if (ui32SpaceAvailable > ui32TotalQueuedDataSize) {
                ui32SpaceAvailable -= ui32TotalQueuedDataSize;
            }
            else {
                ui32SpaceAvailable = 0;
            }
            if ((pWrapper) && (pWrapper->getPacket()->getPacketSize() < ui32SpaceAvailable)) {
                checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_MediumDetailDebug,
                                "transmitting a packet from the pending packet queue of size %lu; remote window size is %lu; space available is %lu\n",
                                (uint32) pWrapper->getPacket()->getPacketSize(), _ui32RemoteWindowSize, ui32SpaceAvailable);
                _pendingPacketQueue.remove (pWrapper);
                Packet *pPacket = pWrapper->getPacket();
                if (pPacket->isControlPacket()) {
                    pPacket->setSequenceNum (_ui32ControlTSN++);
                }
                else if (pPacket->isReliablePacket() && pPacket->isSequencedPacket()) {
                    pPacket->setSequenceNum (_ui32ReliableSequencedTSN++);
                    //printf ("Transmitter::processPendingPacketQueue sending packet with sequence number %lu\n", pPacket->getSequenceNum());
                }
                else if (pPacket->isSequencedPacket()) {
                    pPacket->setSequenceNum (_ui32UnreliableSequencedTSN++);
                }
                else if (pPacket->isReliablePacket()) {
                    pPacket->setSequenceNum (_ui32ReliableUnsequencedID++);
                }
                else {
                    pPacket->setSequenceNum (_ui32UnreliableUnsequencedID++);
                }
                int rc;
                if (0 != (rc = appendPiggybackDataAndTransmitPacket (pWrapper->getPacket(), "New From PPQ"))) {
                    checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_MildError,
                                    "appendPiggybackDataAndTransmitPacket() failed with rc = %d\n", rc);
                }
                else {
                    pWrapper->setLastIOTime (getTimeInMilliseconds());
                }
                pWrapper->setEnqueueTime (getTimeInMilliseconds());
                if (pPacket->isControlPacket()) {
                    _upqControlPackets.insert (pWrapper);
                }
                else if (pPacket->isReliablePacket() && pPacket->isSequencedPacket()) {
                    _upqReliableSequencedPackets.insert (pWrapper);
                    _pMocket->getStatistics()->_ui32ReliableSequencedDataSize = _upqReliableSequencedPackets.getQueuedDataSize();
                    _pMocket->getStatistics()->_ui32ReliableSequencedPacketQueueSize = _upqReliableSequencedPackets.getPacketCount();
                }
                else if (pPacket->isReliablePacket()) {
                    _upqReliableUnsequencedPackets.insert (pWrapper);
                    _pMocket->getStatistics()->_ui32ReliableUnsequencedDataSize = _upqReliableUnsequencedPackets.getQueuedDataSize();
                    _pMocket->getStatistics()->_ui32ReliableUnsequencedPacketQueueSize = _upqReliableUnsequencedPackets.getPacketCount();
                }
                else {
                    delete pWrapper->getPacket();
                    delete pWrapper;
                }
                _pMocket->getStatistics()->_ui32PendingDataSize = _pendingPacketQueue.getBytesInQueue();
                _pMocket->getStatistics()->_ui32PendingPacketQueueSize = _pendingPacketQueue.getPacketsInQueue();
                _pendingPacketQueue.unlock();
                return 1;
            }
            else {
                checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_MediumDetailDebug,
                                "can't send packet %p - no space available; "
                                "remote window size is %lu; total queued data size is %d; space available is %lu\n",
                                pWrapper, _ui32RemoteWindowSize, ui32TotalQueuedDataSize, ui32SpaceAvailable);
                _pendingPacketQueue.unlock();
                return 0;
            }
        }
        else {
            checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_MediumDetailDebug,
                            "not allowed to send yet, transmit rate limit in place\n");
            _pendingPacketQueue.unlock();
            return -10;
        }
    }
    else {
        checkAndLogMsg ("Transmitter::processPendingPacketQueue", Logger::L_HighDetailDebug,
                        "no packets to send\n");
    }
    _pendingPacketQueue.unlock();
    return 0;
}

int Transmitter::processUnacknowledgedPacketQueues (void)
{
    int rc;
    int iCount = 0;
    PacketWrapper *pWrapper;
    int64 i64CurrTime = getTimeInMilliseconds();

    int iFirstTimeoutNum = 0;
    int iOtherTimeoutNum = 0;

    while (allowedToSend()) {
        // Update current time
        i64CurrTime = getTimeInMilliseconds();
        bool bSentPackets = false;
        _upqControlPackets.lock();
        if (nullptr != (pWrapper = _upqControlPackets.getNextTimedOutPacket())) {
            // Here i could check if (iAllowedNumOfBytesToSend > pWrapper->getPacket()->getPacketSize()) and break otherwise,
            // or risk to send at most 1 MTU-1 bytes outside the bandwidth limit
            pWrapper->getPacket()->setRetransmittedPacket();
            if (0 != (rc = appendPiggybackDataAndTransmitPacket (pWrapper->getPacket(), "Retrans Ctrl"))) {
                checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MildError,
                                "appendPiggybackDataAndTransmitPacket() failed with rc = %d\n", rc);
                _upqControlPackets.unlock();
                return -1;
            }
            int64 i64RetrInterval = i64CurrTime - pWrapper->getLastIOTime();
            pWrapper->incrementRetransmitCount();
            pWrapper->setLastIOTime (i64CurrTime);
            uint32 ui32RTO = getRetransmissionTimeout();
            if (_pMocket->_bUseRetransmitCountInRTO) {
                ui32RTO = ui32RTO * (1 + pWrapper->getRetransmitCount());
            }
            // Check the maximum and minimum RTO
            if (_pMocket->getMaximumRTO() > 0) {
                if (ui32RTO > _pMocket->getMaximumRTO()) {
                    ui32RTO = _pMocket->getMaximumRTO();
                }
            }
            if (ui32RTO < _pMocket->getMinimumRTO()) {
                ui32RTO = _pMocket->getMinimumRTO();
            }
            pWrapper->setRetransmitTimeout (ui32RTO);
            if (0 != (rc = _upqControlPackets.packetRetransmitted (pWrapper))) {
                checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MildError,
                                "UnacknowledgedPacketQueue::packetRetransmitted() failed with rc = %d\n", rc);
                _upqControlPackets.unlock();
                return -2;
            }
            iCount++;
            _pMocket->getStatistics()->_ui32Retransmits++;

            if (_pCongestionControl != nullptr) {
                if (pWrapper->getRetransmitCount() == 1) {
                    iFirstTimeoutNum++;
                }
                else {
                    iOtherTimeoutNum++;
                }
            }

            checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MediumDetailDebug,
                            "CN Packet %lu retransmitted (%d %lu)\n",
                            pWrapper->getSequenceNum(), pWrapper->getRetransmitCount(), i64RetrInterval);
            bSentPackets = true;
        }
        _upqControlPackets.unlock();

        if (allowedToSend()) {
            _upqReliableSequencedPackets.lock();
            if (nullptr != (pWrapper = _upqReliableSequencedPackets.getNextTimedOutPacket())) {
                if ((pWrapper->getRetryTimeout() != 0) && (pWrapper->getEnqueueTime() + pWrapper->getRetryTimeout()) < i64CurrTime) {
                    // The retry timeout has expired for the packet
                    checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MediumDetailDebug,
                                    "discarding reliable sequenced packet %lu because its retry timeout has expired\n",
                                    pWrapper->getSequenceNum());
                    _pMocket->getCancelledTSNManager()->cancelReliableSequencedPacket (pWrapper->getSequenceNum());
                    _pMocket->getStatistics()->getOverallMessageStatistics()->ui32CancelledPackets++;
                    uint16 ui16Tag;
                    if (0 != (ui16Tag = pWrapper->getPacket()->getTagId())) {
                        _pMocket->getStatistics()->getMessageStatisticsForType (ui16Tag)->ui32CancelledPackets++;
                    }
                    _pMocket->getStatistics()->_ui32ReliableSequencedDataSize = _upqReliableSequencedPackets.getQueuedDataSize();
                    _pMocket->getStatistics()->_ui32ReliableSequencedPacketQueueSize = _upqReliableSequencedPackets.getPacketCount();
                    _upqReliableSequencedPackets.deleteNextPacketInRetransmitList();
                }
                else {
                    pWrapper->getPacket()->setRetransmittedPacket();
                    if (0 != (rc = appendPiggybackDataAndTransmitPacket (pWrapper->getPacket(), "Retrans RelSeq"))) {
                        checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MildError,
                                        "appendPiggybackDataAndTransmitPacket() failed with rc = %d\n", rc);
                        _upqReliableSequencedPackets.unlock();
                        return -3;
                    }
                    int64 i64TimeSinceLastIO = i64CurrTime - pWrapper->getLastIOTime();
                    pWrapper->incrementRetransmitCount();
                    pWrapper->setLastIOTime (i64CurrTime);
                    uint32 ui32RTO = getRetransmissionTimeout();
                    if (_pMocket->_bUseRetransmitCountInRTO) {
                        ui32RTO = ui32RTO * (1 + pWrapper->getRetransmitCount());
                    }
                    // Check the maximum and minimum RTO
                    if (_pMocket->getMaximumRTO() > 0) {
                        if (ui32RTO > _pMocket->getMaximumRTO()) {
                            ui32RTO = _pMocket->getMaximumRTO();
                        }
                    }
                    if (ui32RTO < _pMocket->getMinimumRTO()) {
                        ui32RTO = _pMocket->getMinimumRTO();
                    }
                    pWrapper->setRetransmitTimeout (ui32RTO);
                    if (0 != (rc = _upqReliableSequencedPackets.packetRetransmitted (pWrapper))) {
                        checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MildError,
                                        "UnacknowledgedPacketQueue::packetRetransmitted() failed with rc = %d\n", rc);
                        _upqReliableSequencedPackets.unlock();
                        return -4;
                    }
                    iCount++;
                    _pMocket->getStatistics()->_ui32Retransmits++;

                    if (_pCongestionControl != nullptr) {
                        if (pWrapper->getRetransmitCount()==1) {
                            iFirstTimeoutNum++;
                        }
                        else {
                            iOtherTimeoutNum++;
                        }
                    }

                    checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MediumDetailDebug,
                                    "RS Packet %lu retransmitted (%d %lld %d)\n",
                                    pWrapper->getSequenceNum(), pWrapper->getRetransmitCount(), i64TimeSinceLastIO, pWrapper->getRetransmitTimeout());
                    bSentPackets = true;
                }
            }
            _upqReliableSequencedPackets.unlock();
        }

        if (allowedToSend()) {
            _upqReliableUnsequencedPackets.lock();
            if (nullptr != (pWrapper = _upqReliableUnsequencedPackets.getNextTimedOutPacket())) {
                if ((pWrapper->getRetryTimeout() != 0) && (pWrapper->getEnqueueTime() + pWrapper->getRetryTimeout()) < i64CurrTime) {
                    // The retry timeout has expired for the packet
                    checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MediumDetailDebug,
                                    "discarding reliable unsequenced packet %lu because its retry timeout has expired\n",
                                    pWrapper->getSequenceNum());
                    _pMocket->getCancelledTSNManager()->cancelReliableUnsequencedPacket (pWrapper->getSequenceNum());
                    _pMocket->getStatistics()->getOverallMessageStatistics()->ui32CancelledPackets++;
                    uint16 ui16Tag;
                    if (0 != (ui16Tag = pWrapper->getPacket()->getTagId())) {
                        _pMocket->getStatistics()->getMessageStatisticsForType (ui16Tag)->ui32CancelledPackets++;
                    }
                    _pMocket->getStatistics()->_ui32ReliableUnsequencedDataSize = _upqReliableUnsequencedPackets.getQueuedDataSize();
                    _pMocket->getStatistics()->_ui32ReliableUnsequencedPacketQueueSize = _upqReliableUnsequencedPackets.getPacketCount();
                    _upqReliableUnsequencedPackets.deleteNextPacketInRetransmitList();
                }
                else {
                    pWrapper->getPacket()->setRetransmittedPacket();
                    if (0 != (rc = appendPiggybackDataAndTransmitPacket (pWrapper->getPacket(), "Retrans RelUnseq"))) {
                        checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MildError,
                                        "appendPiggybackDataAndTransmitPacket() failed with rc = %d\n", rc);
                        _upqReliableUnsequencedPackets.unlock();
                        return -5;
                    }
                    int64 i64TimeSinceLastIO = i64CurrTime - pWrapper->getLastIOTime();
                    pWrapper->incrementRetransmitCount();
                    pWrapper->setLastIOTime (i64CurrTime);
                    uint32 ui32RTO = getRetransmissionTimeout();
                    if (_pMocket->_bUseRetransmitCountInRTO) {
                        ui32RTO = ui32RTO * (1 + pWrapper->getRetransmitCount());
                    }
                    // Check the maximum and minimum RTO
                    if (_pMocket->getMaximumRTO() > 0) {
                        if (ui32RTO > _pMocket->getMaximumRTO()) {
                            ui32RTO = _pMocket->getMaximumRTO();
                        }
                    }
                    if (ui32RTO < _pMocket->getMinimumRTO()) {
                        ui32RTO = _pMocket->getMinimumRTO();
                    }
                    pWrapper->setRetransmitTimeout (ui32RTO);
                    if (0 != (rc = _upqReliableUnsequencedPackets.packetRetransmitted (pWrapper))) {
                        checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MildError,
                                        "UnacknowledgedPacketQueue::packetRetransmitted() failed with rc = %d\n", rc);
                        _upqReliableUnsequencedPackets.unlock();
                        return -6;
                    }
                    iCount++;
                    _pMocket->getStatistics()->_ui32Retransmits++;

                    if(_pCongestionControl != nullptr) {
                        if (pWrapper->getRetransmitCount()==1) {
                            iFirstTimeoutNum++;
                        }
                        else {
                            iOtherTimeoutNum++;
                        }
                    }

                    checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_MediumDetailDebug,
                                    "RU Packet %lu retransmitted (%d %lld %d)\n",
                                    pWrapper->getSequenceNum(), pWrapper->getRetransmitCount(), i64TimeSinceLastIO, pWrapper->getRetransmitTimeout());

                    bSentPackets = true;
                }
            }
            _upqReliableUnsequencedPackets.unlock();
        }
        if (!bSentPackets) {
            checkAndLogMsg ("Transmitter::processUnacknowledgedPacketQueues", Logger::L_HighDetailDebug,
                            "no packets expired\n");
            break;
        }
    }

    if(_pCongestionControl != nullptr) {
        if (iOtherTimeoutNum) {
            _pCongestionControl->reactToLosses (0);
        }
        else if (iFirstTimeoutNum) {
            _pCongestionControl->reactToLosses (iFirstTimeoutNum);
        }
    }

    return iCount;
}

int Transmitter::sendControlPacket (Packet *pPacket, uint8 ui8Priority)
{
    int rc;
    pPacket->setControlPacket (true);
    pPacket->setSequenceNum (_ui32ControlTSN++);
    PacketWrapper *pWrapper = new PacketWrapper (pPacket, getTimeInMilliseconds(), ui8Priority, 0, 0, getRetransmissionTimeout());
    _upqControlPackets.lock();
    _upqControlPackets.insert (pWrapper);
    if (0 != (rc = appendPiggybackDataAndTransmitPacket (pWrapper->getPacket(), "New Ctrl"))) {
        checkAndLogMsg ("Transmitter::sendControlPacket", Logger::L_MildError,
                        "appendPiggybackDataAndTransmitPacket() failed with rc = %d\n", rc);
        _upqControlPackets.unlock();
        return -1;
    }
    _upqControlPackets.unlock();
    return 0;
}

int Transmitter::sendHeartbeatPacket (void)
{
    Packet packet (_pMocket);
    if (packet.addHeartbeatChunk (getTimeInMilliseconds())) {
        return -1;
    }
    if (appendPiggybackDataAndTransmitPacket (&packet, "Heartbeat")) {
        return -2;
    }
    return 0;
}

int Transmitter::sendSAckPacket (void)
{
    Packet packet (_pMocket);
    // NOTE: Changed this to simply call appendPiggybackDataAndTransmitPacket
    // since that method will append the SAck data anyway
    // With this approach, other piggyback data (specifically, the Timestamp chunk)
    // will also be added when required
    if (appendPiggybackDataAndTransmitPacket (&packet, "SAck")) {
        return -1;
    }
    return 0;
}

int Transmitter::sendCancelledTSNPacket (void)
{
    Packet packet (_pMocket);
    if (_pMocket->getCancelledTSNManager()->appendCancelledTSNInformation (&packet)) {
        checkAndLogMsg ("Transmitter::sendCancelledTSNPacket", Logger::L_MildError,
                        "failed to add Cancelled TSN chunk to an empty packet\n");
        return -1;
    }
    if (transmitPacket (&packet, "Cancelled")) {
        return -2;
    }
    _i64LastCancelledTSNTransmitTime = getTimeInMilliseconds();
    return 0;
}

int Transmitter::sendShutdownPacket (void)
{
    Packet packet (_pMocket);
    if (packet.addShutdownChunk()) {
        return -1;
    }
    if (appendPiggybackDataAndTransmitPacket (&packet, "Shutdown")) {
        return -2;
    }
    checkAndLogMsg ("Transmitter::sendShutdownPacket", Logger::L_MediumDetailDebug, "\n");
    return 0;
}

int Transmitter::sendShutdownAckPacket (void)
{
    Packet packet (_pMocket);
    if (packet.addShutdownAckChunk()) {
        return -1;
    }
    if (transmitPacket (&packet, "Shutdown Ack")) {
        return -2;
    }
    checkAndLogMsg ("Transmitter::sendShutdownAckPacket", Logger::L_MediumDetailDebug, "\n");
    return 0;
}

int Transmitter::sendShutdownCompletePacket (void)
{
    Packet packet (_pMocket);
    if (packet.addShutdownCompleteChunk()) {
        return -1;
    }
    if (transmitPacket (&packet, "Shutdown Complete")) {
        return -2;
    }
    checkAndLogMsg ("Transmitter::sendShutdownCompletePacket", Logger::L_MediumDetailDebug, "\n");
    return 0;
}

int Transmitter::sendSimpleSuspendPacket (void)
{
    //printf ("Transmitter::sendSimpleSuspendPacket\n");
    Packet packet (_pMocket);

    if (packet.addSimpleSuspendChunk ()) {
        return -1;
    }
    if (transmitPacket (&packet, "SimpleSuspend")) {
        return -2;
    }
    return 0;
}

int Transmitter::sendSimpleSuspendAckPacket (void)
{
    //printf ("Transmitter::sendSimpleSuspendAckPacket\n");
    Packet packet (_pMocket);
    if (packet.addSimpleSuspendAckChunk()) {
        return -1;
    }
    if (transmitPacket (&packet, "SimpleSuspendAck")) {
        return -2;
    }
    return 0;
}

int Transmitter::sendSuspendPacket (void)
{
    #ifdef MOCKETS_NO_CRYPTO
        checkAndLogMsg ("Mocket::processIncomingPacket", Logger::L_SevereError,
                        "crypto disabled at build time\n");
        return -3;
    #else
        //printf ("Transmitter::sendSuspendPacket\n");
        Packet packet (_pMocket);
        PublicKey::KeyData *pKeyData = _pMocket->_pKeyPair->getPublicKey()->getKeyAsDEREncodedX509Data();

        //**// Print the key to send
        //_pMocket->_pKeyPair->getPublicKey()->storeKeyAsDEREncodedX509Data("publicKeySent.txt");

        if (packet.addSuspendChunk (pKeyData->getData(), pKeyData->getLength())) {
            return -1;
        }
        if (transmitPacket (&packet, "Suspend")) {
            return -2;
        }
        delete pKeyData;
        return 0;
    #endif
}

int Transmitter::sendSuspendAckPacket (void)
{
    #ifdef MOCKETS_NO_CRYPTO
        checkAndLogMsg ("Mocket::sendSuspendAckPacket", Logger::L_SevereError,
                        "crypto disabled at build time\n");
        return -3;
    #else
        //printf ("Transmitter::sendSuspendAckPacket\n");
        Packet packet (_pMocket);
        // Insert in a buffer the UUID and the password to initialize Ks
        char pchBuff[1024];
        uint32 ui32BuffSize = 0;

        // Insert password
        memset (pchBuff, 0, 1024);
        strncpy (pchBuff, _pMocket->getPassword(), _pMocket->getPasswordLength());
        ui32BuffSize += _pMocket->getPasswordLength()+1;

        // Insert the UUID
        *((uint32*)(pchBuff+ui32BuffSize)) = _pMocket->getMocketUUID();
        ui32BuffSize += 4;

        // Encrypt the buffer with the received Ka
        uint32 ui32EncryptedParamLen = 0;
        void *pEncryptedParam = CryptoUtils::encryptDataUsingPublicKey (_pMocket->_pKeyPair->getPublicKey() , pchBuff, ui32BuffSize, &ui32EncryptedParamLen);

        if (packet.addSuspendAckChunk (pEncryptedParam, ui32EncryptedParamLen)) {
            return -1;
        }
        if (transmitPacket (&packet, "SuspendAck")) {
            return -2;
        }
        return 0;
    #endif
}

int Transmitter::sendResumeAckPacket (void)
{
    //printf ("Transmitter::sendResumeAckPacket\n");
    Packet packet (_pMocket);
    if (packet.addResumeAckChunk()) {
        return -1;
    }
    if (transmitPacket (&packet, "ResumeAck")) {
        return -2;
    }
    return 0;
}

int Transmitter::sendReEstablishAckPacket (void)
{
    //printf ("Transmitter::sendReEstablishAckPacket\n");
    Packet packet (_pMocket);
    if (packet.addReEstablishAckChunk()) {
        return -1;
    }
    if (transmitPacket (&packet, "ReEstablishAck")) {
        return -2;
    }
    return 0;
}

int Transmitter::appendPiggybackDataAndTransmitPacket (Packet *pPacket, const char *pszPurpose)
{
    if (_bSendTimestamp) {
        if (pPacket->addTimestampChunk (getTimeInMilliseconds())) {
            checkAndLogMsg ("Transmitter::appendPiggybackDataAndTransmitPacket", Logger::L_MediumDetailDebug,
                            "failed to append Timestamp information to packet before transmitting\n");
        }
        else {
            _bSendTimestamp = false;
        }
    }
    if (_bSendTimestampAck) {
        // Adjust timestamp based on the delta value (time that has elapsed since the timestamp was received)
        int64 i64Timestamp = _i64Timestamp + ((getTimeInMilliseconds() - _i64TimestampReceiveTime));
        if (pPacket->addTimestampAckChunk (i64Timestamp)) {
            checkAndLogMsg ("Transmitter::appendPiggybackDataAndTransmitPacket", Logger::L_MediumDetailDebug,
                            "failed to append Timestamp Ack information to packet before transmitting\n");
        }
        else {
            _bSendTimestampAck = false;
            _i64Timestamp = 0;
            _i64TimestampReceiveTime = 0;
        }
    }
    if (_pMocket->getCancelledTSNManager()->haveInformation()) {
        if (_pMocket->getCancelledTSNManager()->appendCancelledTSNInformation (pPacket)) {
            checkAndLogMsg ("Transmitter::appendPiggybackDataAndTransmitPacket", Logger::L_MediumDetailDebug,
                            "failed to append Cancelled TSN information to packet before transmitting\n");
        }
        else {
            _i64LastCancelledTSNTransmitTime = getTimeInMilliseconds();
        }
    }
    if ((_pMocket->getACKManager()->haveNewInformationSince (_i64LastSAckTransmitTime)) ||
        (_bSendSAckInformationNow)) {
        // Append Sack if there are new info and also if it was specifically requested
        _bSendSAckInformationNow = false;
        if (_pMocket->usingRecBandEst()) {
            if (_pMocket->getACKManager()->appendACKInformation (pPacket, _pMocket->getReceiver()->getLastRecTime(), _pMocket->getReceiver()->getBytesReceived())) {
                checkAndLogMsg ("Transmitter::appendPiggybackDataAndTransmitPacket", Logger::L_MediumDetailDebug,
                                "failed to append SAck information to packet before transmitting\n");
            }
            else {
                _i64LastSAckTransmitTime = getTimeInMilliseconds();
            }
        }
        else {
            if (_pMocket->getACKManager()->appendACKInformation (pPacket)) {
                checkAndLogMsg ("Transmitter::appendPiggybackDataAndTransmitPacket", Logger::L_MediumDetailDebug,
                                "failed to append SAck information to packet before transmitting\n");
            }
            else {
                _i64LastSAckTransmitTime = getTimeInMilliseconds();
            }
        }
    }

    if (transmitPacket (pPacket, pszPurpose)) {
        // Blindly remove the piggyback, if there is no piggyback nothing will change!
        pPacket->removePiggybackChunks();
        return -3;
    }
    // Blindly remove the piggyback, if there is no piggyback nothing will change!
    pPacket->removePiggybackChunks();

    return 0;
}

int Transmitter::transmitPacket (Packet *pPacket, const char *pszPurpose)
{
    #ifdef DEBUG_MIGRATION
    // This IF is necessary to test the freeze/defrost process
    if (_pMocket->isDebugStateCapture()) {
        if ((pPacket->getSequenceNum() % 2) != 0) {
            // The state capture debugging is active and the sequence
            //  number of this packet is odd so we don't send the packet
            printf ("Packet with odd sequence number *%lu* don't send\n", pPacket->getSequenceNum());
            return -3;
        }
    }
    #endif
    pPacket->setWindowSize (_pMocket->getReceiver()->getWindowSize());
    InetAddr sendToAddr (_ui32RemoteAddress, _ui16RemotePort);
    int rc = _pCommInterface->sendTo (&sendToAddr, pPacket->getPacket(), pPacket->getPacketSize());
    if (rc <= 0) {
        if (_pMocket->getStateMachine()->getCurrentState() == StateMachine::S_APPLICATION_ABORT) {
            checkAndLogMsg("Transmitter::transmitPacket", Logger::L_MildError,"failed to send packet, State Machine set to Application Abort");
            return -1;
        }
        checkAndLogMsg ("Transmitter::transmitPacket", Logger::L_MildError,
                        "failed to send packet; rc = %d\n", rc);
        return -1;
    }
    else if (rc < pPacket->getPacketSize()) {
        checkAndLogMsg ("Transmitter::transmitPacket", Logger::L_MildError,
                        "sent a short packet; rc = %d; packet size = %d\n",
                        rc, (int) pPacket->getPacketSize());
        return -2;
    }
    if (pPacket->isControlPacket()) {
        checkAndLogMsg ("Transmitter::transmitPacket", Logger::L_MediumDetailDebug,
                        "transmitted a control packet with sequence number %lu and size %d; local window size is %lu\n",
                        pPacket->getSequenceNum(), (int) pPacket->getPacketSize(), pPacket->getWindowSize());
    }
    else {
        checkAndLogMsg ("Transmitter::transmitPacket", Logger::L_MediumDetailDebug,
                        "transmitted a data packet of type %s:%s with sequence number %u and size %d; local window size is %u\n",
                        pPacket->isReliablePacket() ? "reliable" : "unreliable",
                        pPacket->isSequencedPacket() ? "sequenced" : "unsequenced",
                        pPacket->getSequenceNum(), (int) pPacket->getPacketSize(),
                        pPacket->getWindowSize());
        /*
        ("Transmitter::transmitPacket transmitted a data packet of type %s:%s with sequence number %u and size %d; local window size is %lu\n",
                        pPacket->isReliablePacket() ? "reliable" : "unreliable",
                        pPacket->isSequencedPacket() ? "sequenced" : "unsequenced",
                        pPacket->getSequenceNum(), (int) pPacket->getPacketSize(),
                        pPacket->getWindowSize());*/
    }
    _i64LastTransmitTime = getTimeInMilliseconds();
    if (_resLimits.ui32RateLimit != 0) {
        if (_resLimits.ui32RateLimit > _pMocket->BANDWIDTH_LIMITATION_THRESHOLD) {
            // There is a large bandwidth limit in place, update _pByteSentPerInterval
            _pByteSentPerInterval->add (pPacket->getPacketSizeWithoutPiggybackChunks());
        }
        else {
            // There is a small bandwidth limit in place, the next time to transmit
            // is now + how long it takes to send this packet given the bandwidth limit
            _i64NextTimeToTransmit = _i64LastTransmitTime + (pPacket->getPacketSize() * 1000 / _resLimits.ui32RateLimit);
            checkAndLogMsg ("Transmitter::transmitPacket", Logger::L_MediumDetailDebug,
                            "NextTimeToTransmit %llu LastTransmitTime %llu time to send packet %4.2f\n", _i64NextTimeToTransmit, _i64LastTransmitTime, (float)(pPacket->getPacketSize() * 1000 / _resLimits.ui32RateLimit));
        }
    }
    _pMocket->getStatistics()->_ui32SentPackets++;
    if (_filePacketXMitLog) {
            const char *pszFragment = "No";
            if (pPacket->isFirstFragment()) {
                pszFragment = "First";
            }
            else if (pPacket->isIntermediateFragment()) {
                pszFragment = "Int";
            }
            else if (pPacket->isLastFragment()) {
                pszFragment = "Last";
            }
        #if defined (WIN32)
            fprintf (_filePacketXMitLog, "%I64d, %I64d, %d, %d, %s, %s, %s, %d, %s\n",
        #else
            fprintf (_filePacketXMitLog, "%lld, %lld, %d, %d, %s, %s, %s, %d, %s\n",
        #endif
                (_i64LastTransmitTime - _i64LogStartTime),
                (_i64LastTransmitTime - _i64LastXMitLogTime),
                (int) pPacket->getPacketSize(), (int) pPacket->getSequenceNum(),
                pPacket->isReliablePacket() ? "yes" : "no",
                pPacket->isSequencedPacket() ? "yes" : "no",
                pszFragment, (int) pPacket->getTagId(),
                pszPurpose ? pszPurpose : "<unknown>");
        _i64LastXMitLogTime = _i64LastTransmitTime;
    }
    return 0;
}

int Transmitter::setBandwidthEstimationActive (uint16 ui16InitialAssumedBandwidth)
{
    _pBandwidthEstimator = new BandwidthEstimator(_pMocket->getBandEstMaxSamplesNumber(), _pMocket->getBandEstTimeInterval(), _pMocket->getBandEstSamplingTime(), ui16InitialAssumedBandwidth);
    _pMocket->getStatistics()->setEstimatedBandwidth (-1);
    return 0;
}

int Transmitter::setCongestionControlActive (const char *pszCongestionControl)
{
    // For each new subclass of CongestionControl some code needs to be added here
    if (pszCongestionControl == nullptr) {
        checkAndLogMsg ("Transmitter::setCongestionControlActive", Logger::L_MildError,
                        "ERROR: must define a CongestionControl subclass to instanciate\n");
        return -1;
    }
    if (strcmp("CongestionController", pszCongestionControl) == 0) {
        _pCongestionControl = new CongestionController (_pMocket);
        return 0;
    }
    if (strcmp("TransmissionRateModulation", pszCongestionControl) == 0) {
        _pCongestionControl = new TransmissionRateModulation (_pMocket);
        return 0;
    }
    checkAndLogMsg ("Transmitter::setCongestionControlActive", Logger::L_MildError,
                    "ERROR: the requested congestion control mechanism: %s, is not implemented\n", pszCongestionControl);
    return -1;
}
