/*
 * LocalTCPTransmitter.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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

#if defined (WIN32)
#define NOMINMAX    // For interference with std::min and std::max
#endif

#include <mutex>
#include <condition_variable>
#include <algorithm>

#include "Logger.h"

#include "LocalTCPTransmitter.h"
#include "Connection.h"
#include "TCPConnTable.h"
#include "TCPManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    void LocalTCPTransmitterThread::run (void)
    {
        started();

        int rc;
        std::unique_lock<std::mutex> ul{_mtx};
        while (!terminationRequested()) {
            const int64 i64NextCycleTime = NOMADSUtil::getTimeInMilliseconds() + I64_LTT_TIME_BETWEEN_ITERATIONS;
            _cv.wait_for (ul, std::chrono::milliseconds{I64_LTT_TIME_BETWEEN_ITERATIONS}, [this, i64NextCycleTime]
                          {
                            return (NOMADSUtil::getTimeInMilliseconds() >= i64NextCycleTime) ||
                                _bNotified || terminationRequested();
                          });
            if (terminationRequested()) {
                break;
            }

            // Check all active outgoing buffers in the TCPConnTable to see if there is data to be transmitted
            Entry * pEntry;
            std::lock_guard<std::mutex> lgTCPConnTable{_rTCPConnTable.getMutexRef()};
            _rTCPConnTable.resetGet();
            while ((pEntry = _rTCPConnTable.getNextActiveLocalEntry()) != nullptr) {
                std::unique_lock<std::mutex> ul{pEntry->getMutexRef(), std::try_to_lock};
                if (ul.owns_lock()) {
                    const auto i64CurrTime = NOMADSUtil::getTimeInMilliseconds();
                    auto itOutgoingPackets = pEntry->dqLocalHostOutBuf.begin();

                    /* Check if the remote connection was lost. Exclude the SYN_SENT state, because the local application has not
                     * replied with a SYN+ACK yet, and the SYN_RCVD state, in which case the NetProxy has not yet replied to the SYN. */
                    if (!pEntry->getConnection() && (pEntry->localState != TCTLS_SYN_SENT) && (pEntry->localState != TCTLS_SYN_RCVD) &&
                        (pEntry->remoteState != TCTRS_Unknown) && (pEntry->remoteState != TCTRS_Disconnected)) {
                        checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R%hu: Connection object to remote proxy was lost; sending an RST to local host\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: sendTCPPacketToHost() failed sending an RST to local host; with rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        pEntry->reset();
                        continue;
                    }

                    if (itOutgoingPackets != pEntry->dqLocalHostOutBuf.end()) {
                        if ((itOutgoingPackets->getLastTransmitTime() > 0) && ((itOutgoingPackets->getLastTransmitTime() - pEntry->i64LastAckTime) > Entry::STANDARD_MSL)) {
                            // It's not the first transmission attempt and it has already been tried to retransmit many times --> LOCAL CONNECTION IS DOWN: CLOSE IT.
                            if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send an RST packet to host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: local host seems not being answering to packets; sent a reset request "
                                            "to both remote and local applications\n", pEntry->ui16ID, pEntry->ui16RemoteID);
                            TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            continue;
                        }

                        if ((pEntry->localState == TCTLS_SYN_SENT) &&
                            ((i64CurrTime - pEntry->i64LocalActionTime) > NetworkConfigurationSettings::SYN_SENT_RETRANSMISSION_TIMEOUTS[pEntry->ui8RetransmissionAttempts])) {
                            // Check if the timeout for waiting for a SYN+ACK has expired (i64LastAckTime is set to the time the first SYN was sent)
                            if ((i64CurrTime - pEntry->i64LastAckTime) >= NetworkConfigurationSettings::SYN_SENT_FAILED_TIMEOUT) {
                                checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_Info,
                                                "L%hu-R%hu: SYN_SENT timeout expired; sending an RST to the local application "
                                                "and a ResetTCPConnection ProxyMessage to the remote NetProxy\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID);
                                if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                    "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                            }
                            else if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, itOutgoingPackets->getTCPFlags(), itOutgoingPackets->getSequenceNumber(),
                                                                                  itOutgoingPackets->getData(), itOutgoingPackets->getItemLength()))) {
                                checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send packet with flag %hu to host; rc = %d\n",
                                                itOutgoingPackets->getTCPFlags(), rc);
                                TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                            }
                            else {
                                // Successfully performed SYN retransmission
                                ++pEntry->ui8RetransmissionAttempts;
                                pEntry->i64LocalActionTime = i64CurrTime;
                                itOutgoingPackets->setLastTransmitTime (i64CurrTime);
                                checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                "L%hu-R%hu: RETRANSMITTED a SYN packet to the local application; next retransmission attempt will "
                                                "be after %lldms\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                NetworkConfigurationSettings::SYN_SENT_RETRANSMISSION_TIMEOUTS[pEntry->ui8RetransmissionAttempts]);
                            }
                            // In any case, continue to the next Entry of the TCPConnTable
                            continue;
                        }

                        // Connection to local host is active: check if there are any packets with data segments to send to the local application
                        if ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_CLOSE_WAIT) ||
                            (pEntry->localState == TCTLS_FIN_WAIT_1) || (pEntry->localState == TCTLS_CLOSING)) {
                            while ((itOutgoingPackets != pEntry->dqLocalHostOutBuf.end()) && (itOutgoingPackets->getItemLength() > 0)) {
                                // Check if in the receiver buffer there is enough room to receive the packet
                                if (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (
                                        NOMADSUtil::SequentialArithmetic::delta (itOutgoingPackets->getFollowingSequenceNumber(), pEntry->ui32LastAckSeqNum),
                                        static_cast<uint32> (pEntry->ui16ReceiverWindowSize))) {
                                    // Check if enough time has elapsed to perform a (re-)transmission
                                    if ((i64CurrTime - itOutgoingPackets->getLastTransmitTime()) > pEntry->ui16RTO) {
                                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, itOutgoingPackets->getTCPFlags(), itOutgoingPackets->getSequenceNumber(),
                                                                                         itOutgoingPackets->getData(), itOutgoingPackets->getItemLength()))) {
                                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                                            "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                            TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                            pEntry->reset();
                                            itOutgoingPackets = pEntry->dqLocalHostOutBuf.end();
                                            break;
                                        }
                                        if (itOutgoingPackets->getLastTransmitTime() == 0) {
                                            // First transmission --> incrementing outgoing SEQ number
                                            pEntry->ui32OutSeqNum = itOutgoingPackets->getFollowingSequenceNumber();
                                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                                            "L%hu-R%hu: transmitted to local host %hu bytes of data with FLAGs %hhu starting at SEQ number %u "
                                                            "(relative %u); next packet will have SEQ number %u\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                            itOutgoingPackets->getItemLength(), itOutgoingPackets->getTCPFlags(), itOutgoingPackets->getSequenceNumber(),
                                                            NOMADSUtil::SequentialArithmetic::delta (itOutgoingPackets->getSequenceNumber(), pEntry->ui32StartingOutSeqNum),
                                                            pEntry->ui32OutSeqNum);
                                        }
                                        else {
                                            // Check if outgoing SEQ number needs to be updated (this happens when the first transmission of the packet was an octet)
                                            if (NOMADSUtil::SequentialArithmetic::lessThan (pEntry->ui32OutSeqNum, itOutgoingPackets->getFollowingSequenceNumber())) {
                                                pEntry->ui32OutSeqNum = itOutgoingPackets->getFollowingSequenceNumber();
                                            }
                                            while (pEntry->ui16SRTT <= pEntry->ui16RTO) {
                                                // Doubling RTO and SRTT because of the retransmission
                                                if (pEntry->ui16SRTT == Entry::UB_RTO) {
                                                    break;
                                                }
                                                if (pEntry->ui16SRTT == 0) {
                                                    pEntry->ui16SRTT = Entry::LB_RTO;
                                                    continue;
                                                }
                                                else if (pEntry->ui16SRTT > (Entry::UB_RTO / 2)) {
                                                    pEntry->ui16SRTT = Entry::UB_RTO / 2;
                                                }
                                                pEntry->ui16SRTT *= 2;
                                            }
                                            pEntry->ui16RTO = pEntry->ui16SRTT;
                                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                                            "L%hu-R%hu: RETRANSMITTED %hu bytes of data with FLAGs %hhu starting at sequence number %u (relative %u); "
                                                            "new RTO is %hu and next packet will have SEQ number %u\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                            itOutgoingPackets->getItemLength(), itOutgoingPackets->getTCPFlags(), itOutgoingPackets->getSequenceNumber(),
                                                            NOMADSUtil::SequentialArithmetic::delta (itOutgoingPackets->getSequenceNumber(), pEntry->ui32StartingOutSeqNum),
                                                            pEntry->ui16RTO, pEntry->ui32OutSeqNum);
                                        }

                                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                                        pEntry->i64LocalActionTime = i64CurrTime;
                                        itOutgoingPackets->setLastTransmitTime (i64CurrTime);
                                    }
                                    // Subsequent packets might also satisfy the timeout requirements to perform (re)transmissions
                                    ++itOutgoingPackets;
                                }
                                else {
                                    // Packet should be (re)transmitted, but there is not enough space in the receiver buffer
                                    checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: impossible to transmit data to local host; last ACK received is %u, next packet "
                                                    "to transmit has SEQ number %u (relative %u) and is %hu bytes long; receiver window size is "
                                                    "%hu and there are %d packets in the outgoing queue; waiting before transmitting\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32LastAckSeqNum, itOutgoingPackets->getSequenceNumber(),
                                                    NOMADSUtil::SequentialArithmetic::delta (itOutgoingPackets->getSequenceNumber(), pEntry->ui32StartingOutSeqNum),
                                                    itOutgoingPackets->getItemLength(), pEntry->ui16ReceiverWindowSize, pEntry->dqLocalHostOutBuf.size());

                                    // Check if we can send an octet to force update
                                    if ((i64CurrTime - itOutgoingPackets->getLastTransmitTime()) > pEntry->ui16RTO) {
                                        while (pEntry->ui16SRTT <= pEntry->ui16RTO) {
                                            // Doubling RTO and SRTT to avoid forcing updates too often
                                            if (pEntry->ui16SRTT == Entry::UB_RTO) {
                                                break;
                                            }
                                            if (pEntry->ui16SRTT == 0) {
                                                pEntry->ui16SRTT = Entry::LB_RTO;
                                                continue;
                                            }
                                            else if (pEntry->ui16SRTT > (Entry::UB_RTO / 2)) {
                                                pEntry->ui16SRTT = Entry::UB_RTO / 2;
                                            }
                                            pEntry->ui16SRTT *= 2;
                                        }
                                        pEntry->ui16RTO = pEntry->ui16SRTT;

                                        const int iPeekedOctet = itOutgoingPackets->peekOctet (pEntry->ui32OutSeqNum);
                                        if ((iPeekedOctet < 0) || (iPeekedOctet > 255)) {
                                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                                            "L%hu-R%hu: failed to extract an octed with SEQ number %u from a packet with SEQ number %u "
                                                            "and %hu bytes long; rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum,
                                                            itOutgoingPackets->getSequenceNumber(), itOutgoingPackets->getItemLength(), iPeekedOctet);
                                            TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                            pEntry->reset();
                                        }
                                        else {
                                            // Octet correctly peeked
                                            const uint8 ui8PeekedOctet = iPeekedOctet;
                                            if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_PSH | NOMADSUtil::TCPHeader::TCPF_ACK,
                                                                                             pEntry->ui32OutSeqNum, &ui8PeekedOctet, 1))) {
                                                checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                                "L%hu-R%hu: failed to send PSH+ACK packet to host to force sending a window update; rc = %d\n",
                                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                                TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                                pEntry->reset();
                                            }
                                            else {
                                                itOutgoingPackets->setLastTransmitTime (i64CurrTime);
                                                pEntry->i64LocalActionTime = i64CurrTime;
                                                pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                                                if (NOMADSUtil::SequentialArithmetic::lessThan (NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32OutSeqNum, pEntry->ui32LastAckSeqNum),
                                                    (uint32) pEntry->ui16ReceiverWindowSize)) {
                                                    pEntry->ui32OutSeqNum++;
                                                    checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                                    "L%hu-R%hu: successfully sent one octet with SEQ number %lu to force local host to send a window update; "
                                                                    "next packet will be sent with SEQ number %lu\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                                    pEntry->ui32OutSeqNum - 1, pEntry->ui32OutSeqNum);
                                                }
                                                else {
                                                    checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                                    "L%hu-R%hu: successfully sent one octet with SEQ number %u to force local host to send a window update; "
                                                                    "next packet will still be sent with the same SEQ number, as receiver window is full\n",
                                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum, pEntry->ui32OutSeqNum);
                                                }
                                            }
                                        }
                                    }

                                    // Making sure that we transmit an octet, if it was possible to do so, only once
                                    itOutgoingPackets = pEntry->dqLocalHostOutBuf.end();
                                    break;
                                }
                            }
                        }

                        // Check if there is an enqueued FIN packet
                        if ((itOutgoingPackets != pEntry->dqLocalHostOutBuf.end()) && ((itOutgoingPackets->getTCPFlags() & NOMADSUtil::TCPHeader::TCPF_FIN) != 0) &&
                            ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_CLOSE_WAIT) ||
                            ((i64CurrTime - itOutgoingPackets->getLastTransmitTime()) >= pEntry->ui16RTO))) {
                            /*
                            * Placeholder for a FIN packet. Note that, according to the RFC, before transmitting the FIN packet, the local
                            * endpoint should check that the remote window size has at least one free byte to enqueue the FIN packet.
                            */
                            if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, itOutgoingPackets->getTCPFlags(), itOutgoingPackets->getSequenceNumber(),
                                                                             itOutgoingPackets->getData(), itOutgoingPackets->getItemLength()))) {
                                checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send packet with flag %hu to host; rc = %d\n",
                                                itOutgoingPackets->getTCPFlags(), rc);
                                TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                continue;
                            }
                            else {
                                if (pEntry->localState == TCTLS_ESTABLISHED) {
                                    pEntry->localState = TCTLS_FIN_WAIT_1;
                                    pEntry->ui32OutSeqNum++;
                                    checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent FIN while in state ESTABLISHED, moving to FIN_WAIT_1; SEQ number is: %u (relative %u)\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, itOutgoingPackets->getSequenceNumber(),
                                                    NOMADSUtil::SequentialArithmetic::delta (itOutgoingPackets->getSequenceNumber(), pEntry->ui32StartingOutSeqNum));
                                }
                                else if (pEntry->localState == TCTLS_CLOSE_WAIT) {
                                    pEntry->localState = TCTLS_LAST_ACK;
                                    pEntry->ui32OutSeqNum++;
                                    checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent FIN while in state CLOSE_WAIT, moving to LAST_ACK; SEQ number is: %u (relative %u)\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, itOutgoingPackets->getSequenceNumber(),
                                                    NOMADSUtil::SequentialArithmetic::delta (itOutgoingPackets->getSequenceNumber(), pEntry->ui32StartingOutSeqNum));
                                }
                                else if ((pEntry->localState == TCTLS_FIN_WAIT_1) || (pEntry->localState == TCTLS_LAST_ACK)) {
                                    while (pEntry->ui16SRTT <= pEntry->ui16RTO) {
                                        // Doubling RTO and SRTT because of the retransmission
                                        if (pEntry->ui16SRTT == Entry::UB_RTO) {
                                            break;
                                        }
                                        if (pEntry->ui16SRTT == 0) {
                                            pEntry->ui16SRTT = Entry::LB_RTO;
                                            continue;
                                        }
                                        else if (pEntry->ui16SRTT > (Entry::UB_RTO / 2)) {
                                            pEntry->ui16SRTT = Entry::UB_RTO / 2;
                                        }
                                        pEntry->ui16SRTT *= 2;
                                    }
                                    pEntry->ui16RTO = pEntry->ui16SRTT;
                                    checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: retransmitted FIN with SEQ number %u (relative %u) and ACK number "
                                                    "%u due to an expired timeout while in local state %d; new RTO is %hu\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, itOutgoingPackets->getSequenceNumber(),
                                                    NOMADSUtil::SequentialArithmetic::delta (itOutgoingPackets->getSequenceNumber(), pEntry->ui32StartingOutSeqNum),
                                                    pEntry->ui32NextExpectedInSeqNum, pEntry->localState, pEntry->ui16RTO);
                                }
                                pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                                pEntry->i64LocalActionTime = i64CurrTime;
                                itOutgoingPackets->setLastTransmitTime (i64CurrTime);
                            }
                        }
                    }
                    else if (((i64CurrTime - pEntry->i64LocalActionTime) > NetworkConfigurationSettings::IDLE_TCP_FAST_RETRANSMIT_TRIGGER_TIMEOUT) &&
                             pEntry->areThereHolesInTheIncomingDataBuffer() && !pEntry->areDataAvailableInTheIncomingBuffer()) {
                        // Packets are missing in the incoming data buffer and we can trigger fast retransmit
                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: failed to send ACK packet to local host to trigger fast retransmit; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            continue;
                        }
                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                        pEntry->i64LocalActionTime = i64CurrTime;
                        checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: sent ACK to local host to trigger fast retransmit; first missing packet has SEQ number %u (relative %u)\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                        NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                    }
                    else if (pEntry->getConnection() &&
                        ((i64CurrTime - pEntry->i64LocalActionTime) > NetworkConfigurationSettings::IDLE_TCP_CONNECTION_NOTIFICATION_TIME) &&
                        ((i64CurrTime - pEntry->i64RemoteActionTime) > NetworkConfigurationSettings::IDLE_TCP_CONNECTION_NOTIFICATION_TIME) &&
                        ((i64CurrTime - pEntry->i64IdleTime) > NetworkConfigurationSettings::IDLE_TCP_CONNECTION_NOTIFICATION_TIME)) {
                        // Check performed only if there is no data to transmit
                        if (!(pEntry->getConnection()->isConnected() || pEntry->getConnection()->isConnecting())) {
                            if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send an RST packet to host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: the %s connection to the remote NetProxy with uniqueID %u, local endpoint <%s:%hu>, and remote endpoint "
                                            "<%s:%hu> has been lost; sent an RST packet to the local applications\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pEntry->getConnection()->getConnectorTypeAsString(), pEntry->getConnection()->getRemoteNetProxyID(),
                                            pEntry->getConnection()->getLocalInterfaceInetAddr()->getIPAsString(),
                                            pEntry->getConnection()->getLocalInterfaceInetAddr()->getPort(),
                                            pEntry->getConnection()->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                            pEntry->getConnection()->getRemoteInterfaceLocalInetAddr()->getPort());
                            pEntry->reset();
                            continue;
                        }

                        pEntry->i64IdleTime = i64CurrTime;
                        checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: no activity detected (local state = %d; remote state = %d) for the last %llu milliseconds;\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState, pEntry->remoteState,
                                        i64CurrTime - pEntry->i64LocalActionTime);
                    }

                    // Check if it is necessary to send an ACK to the local host to update the LastACKedSeqNum
                    if (NOMADSUtil::SequentialArithmetic::lessThan (pEntry->ui32LastACKedSeqNum, pEntry->ui32NextExpectedInSeqNum) &&
                        ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_FIN_WAIT_1) || (pEntry->localState == TCTLS_FIN_WAIT_2))) {
                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: failed to send ACK packet with number %u (relative %u) and SEQ number %u (relative %u) "
                                            "to local host; rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                            NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                            pEntry->ui32OutSeqNum, NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32OutSeqNum, pEntry->ui32StartingOutSeqNum), rc);
                            TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            continue;
                        }
                        else {
                            // ACK sent --> last ACKed SEQ number updated
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            pEntry->i64LocalActionTime = i64CurrTime;
                            checkAndLogMsg ("LocalTCPTransmitterThread::run", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: sent an ACK to local host to update ACK status\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }
                }
            }

            _bNotified = false;
        }

        terminating();
    }

    const int64 LocalTCPTransmitterThread::I64_LTT_TIME_BETWEEN_ITERATIONS;
}