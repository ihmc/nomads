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

#include <mutex>
#include <condition_variable>

#include "Logger.h"

#include "RemoteTCPTransmitter.h"
#include "Entry.h"
#include "Connection.h"
#include "TCPConnTable.h"
#include "ConnectionManager.h"
#include "TCPManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    void RemoteTCPTransmitterThread::run (void)
    {
        started();

        int rc;
        std::unique_lock<std::mutex> ul{_mtx};
        while (!terminationRequested()) {
            const int64 i64NextCycleTime = NOMADSUtil::getTimeInMilliseconds() + I64_RTT_TIME_BETWEEN_ITERATIONS;
            _cv.wait_for (ul, std::chrono::milliseconds{I64_RTT_TIME_BETWEEN_ITERATIONS}, [this, i64NextCycleTime]
                          {
                            return (NOMADSUtil::getTimeInMilliseconds() >= i64NextCycleTime) ||
                                _bNotified || terminationRequested();
                          });
            if (terminationRequested()) {
                break;
            }

            // Check the incoming buffers in the TCPConnTable to see if there is data to be transmitted
            Entry * pEntry;
            std::lock_guard<std::mutex> lgTCPConnTable{_rTCPConnTable.getMutexRef()};
            _rTCPConnTable.resetGet();
            while ((pEntry = _rTCPConnTable.getNextActiveRemoteEntry()) != nullptr) {
                std::unique_lock<std::mutex> ul{pEntry->getMutexRef(), std::try_to_lock};
                if (ul.owns_lock()) {
                    if (NetProxyApplicationParameters::ENABLE_PRIORITIZATION_MECHANISM) {
                        if (pEntry->assignedPriority > _rTCPConnTable.getHighestKnownPriority()) {
                            _rTCPConnTable.setNewHighestPriority (pEntry->assignedPriority);
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "Table's New Highest Priority changed to %u\n", _rTCPConnTable.getNewHighestPriority());
                        }
                        else {
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "Assigned priority: %u is not higher than the current highest priority: %u",
                                            pEntry->assignedPriority, _rTCPConnTable.getHighestKnownPriority());
                        }

                        if (pEntry->currentPriority < _rTCPConnTable.getHighestKnownPriority()) {
                            pEntry->currentPriority++;
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "Entry's current priority incremented to %u; switching to the next "
                                            "entry since the minimum priority is %u", pEntry->currentPriority,
                                            _rTCPConnTable.getHighestKnownPriority());
                            continue;
                        }
                        else {
                            pEntry->currentPriority = pEntry->assignedPriority;
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "Entry priority is high enough to be processed in this cicle; resetting "
                                            "priority value to the original value %u and processing the entry",
                                            pEntry->currentPriority);
                        }
                    }

                    // Check if the connection is available and it has not failed to connect
                    auto * const pConnection = pEntry->getConnection();
                    if (!pConnection) {
                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                        "L%hu-R%hu: getConnection() returned a NULL pointer; sending an RST packet to local host "
                                        "and clearing connection\n", pEntry->ui16ID, pEntry->ui16RemoteID);

                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: failed to send an RST packet to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        pEntry->reset();
                        continue;
                    }
                    if (pConnection->hasFailedConnection()) {
                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R%hu: connection attempt to the remote NetProxy with UniqueID %u on address "
                                        "<%s:%hu> failed; sending an RST packet to local host and resetting the Entry\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pConnection->getRemoteNetProxyID(),
                                        pConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                                        pConnection->getRemoteNetProxyInetAddr()->getPort());

                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: failed to send an RST packet to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        pEntry->reset();
                        continue;
                    }

                    // Check if the connection timeout has expired
                    const auto i64CurrTime = NOMADSUtil::getTimeInMilliseconds();
                    if (((pEntry->remoteState == TCTRS_WaitingConnEstablishment) || (pEntry->remoteState == TCTRS_ConnRequestSent)) &&
                        ((i64CurrTime - pEntry->i64RemoteActionTime) > NetworkConfigurationSettings::VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT)) {
                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Info,
                                        "L%hu-R%hu: the state of the remote connection with the NetProxy at address <%s:%hu> is %d, but no action "
                                        "was detected in the last %lldms. This caused the VirtualConnectionEstablishmentTimeout to expire; "
                                        "NetProxy will reset both the local TCP connection and the connection to the remote NetProxy\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->iaRemoteProxyAddr.getIPAsString(),
                                        pEntry->iaRemoteProxyAddr.getPort(), pEntry->remoteState, (i64CurrTime - pEntry->i64RemoteActionTime));
                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("MemoryCleanerManagerThread::run", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        continue;
                    }

                    // Check if the connection is still being established
                    if (pConnection->isConnecting()) {
                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_NetDetailDebug,
                                        "L%hu-R%hu: the %sConnection object still has to finish opening the connection to the remote NetProxy with "
                                        "UniqueID %u at address <%s:%hu>; NetProxy will skip this Entry for now\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                        pConnection->getConnectorTypeAsString(), pConnection->getRemoteNetProxyID(),
                                        pConnection->getRemoteNetProxyInetAddr()->getIPAsString(), pConnection->getRemoteNetProxyInetAddr()->getPort());
                        continue;
                    }

                    // Check if enqueuing is allowed by the Connection object
                    if (!pConnection->isEnqueueingAllowed()) {
                        // Impossible to enqueue the TCPConnectionOpened ProxyMessage for transmission
                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_NetDetailDebug,
                                        "L%hu-R%hu: impossible to enqueue a TCPConnectionOpened ProxyMessage with the Connection object\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        continue;
                    }

                    // Send if the local NetProxy still has to send an OpenTCPConnectionRequest or reply with a TCPConnectionOpenedResponse
                    if (pEntry->remoteState == TCTRS_WaitingConnEstablishment) {
                        // It is still necessary to send the OpenTCPConnectionRequest ProxyMessage to the remote NetProxy
                        auto bReachable =
                            _rConnectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (pConnection->getRemoteNetProxyID(),
                                                                                                    pConnection->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                                    pConnection->getRemoteInterfaceLocalInetAddr()->getIPAddress());

                        // TODO: this method may have to be fixed, too, to be non-blocking
                        if (0 != (rc = pConnection->sendOpenTCPConnectionRequest (pEntry, bReachable))) {
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R0: sendOpenTCPConnectionRequest() failed with rc = %d\n",
                                            pEntry->ui16ID, rc);

                            pEntry->ui32StartingOutSeqNum = 0;
                            if (NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE) {
                                pEntry->ui32StartingOutSeqNum = 0;
                                rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0);
                            }
                            else {
                                rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum);
                            }
                            if (rc != 0) {
                                checkAndLogMsg ("MemoryCleanerManagerThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                            "L%hu-R0: impossible to send an OpenTCPConnection ProxyMessage to remote host; "
                                            "sent an RST back to the local application\n", pEntry->ui16ID);
                            pEntry->reset();
                        }
                        else {
                            pEntry->remoteState = TCTRS_ConnRequestSent;
                            pEntry->i64RemoteActionTime = i64CurrTime;
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "L%hu-R0: successfully sent an OpenTCPConnection ProxyMessage "
                                            "via %s to the remote NetProxy at address <%s:%hu>\n",
                                            pEntry->ui16ID, pConnection->getConnectorTypeAsString(),
                                            pEntry->iaRemoteProxyAddr.getIPAsString(),
                                            pEntry->iaRemoteProxyAddr.getPort());
                        }
                        continue;
                    }
                    else if ((pEntry->remoteState == TCTRS_ConnRequestReceived) && (pEntry->localState == TCTLS_ESTABLISHED)) {
                        // It is still necessary to send a TCPConnectionOpenedResponse ProxyMessage to the remote NetProxy
                        auto bReachable =
                            _rConnectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (pConnection->getRemoteNetProxyID(),
                                                                                                    pConnection->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                                    pConnection->getRemoteInterfaceLocalInetAddr()->getIPAddress());
                        // TODO: this method may have to be fixed too to be non-blocking
                        if (0 != (rc = pConnection->sendTCPConnectionOpenedResponse (pEntry, bReachable))) {
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: sendTCPConnectionOpenedResponse() failed with rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            if (0 != (rc = _rTCPManager.sendTCPPacketToHost(pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send an RST+ACK packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            else {
                                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                                "L%hu-R%hu: impossible to send a TCPConnectionOpened ProxyMessage to the remote NetProxy at address <%s:%hu>; "
                                                "sent an RST back to the local host and resetting entry\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort());
                            }
                            pEntry->reset();
                            continue;
                        }
                        else {
                            pEntry->remoteState = TCTRS_ConnEstablished;
                            pEntry->i64RemoteActionTime = i64CurrTime;
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "L%hu-R%hu: successfully sent a TCPConnectionOpened response via %s to the remote "
                                            "NetProxy at address <%s:%hu>\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pConnection->getConnectorTypeAsString(), pEntry->iaRemoteProxyAddr.getIPAsString(),
                                            pEntry->iaRemoteProxyAddr.getPort());
                            // Continue after having moved to TCTRS_ConnEstablished
                        }
                    }

                    // While enqueing is allowed by the transport layer, transmit any available data
                    double beginningTCPWindowPercentage = pEntry->getOutgoingBufferRemainingSpacePercentage();
                    while (pConnection->isEnqueueingAllowed() && pEntry->areDataAvailableInTheIncomingBuffer() &&
                        (pConnection->getConnectorAdapter()->getOutgoingBufferSize() >
                        (sizeof(TCPDataProxyMessage) + NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE))) {
                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "L%hu-R%hu: there are %u bytes of data in the buffer, of which %u are ready to be transmitted\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingTotalBytesCount(),
                                        pEntry->getOutgoingReadyBytesCount());
                        // Transmission to remote proxy is allowed only if the remote connection is either in state ConnEstablished or in state DisconnRequestReceived
                        if ((pEntry->remoteState == TCTRS_ConnEstablished) || (pEntry->remoteState == TCTRS_DisconnRequestReceived)) {
                            //if we are not holding a segment from the previous cycle we dequeue another segment
                            if (pEntry->pTCPSegment == nullptr) {
                                pEntry->pTCPSegment = pEntry->dequeueLocallyReceivedData (NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
                            }
                            //check that the call to dequeueLocallyReceivedData() was successful
                            if (pEntry->pTCPSegment == nullptr) {
                                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: dequeueLocallyReceivedData() failed; sending RST packet to local host and clearing connection\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID);

                                if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                                    "L%hu-R%hu: sendTCPPacketToHost() failed sending an RST to local host; with rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                break;
                            }

                            // Check if the dequeued TCP packet has a FIN flag
                            if (pEntry->pTCPSegment->getTCPFlags() & NOMADSUtil::TCPHeader::TCPF_FIN) {
                                // Check if there is any data with the FIN
                                if (pEntry->pTCPSegment->getItemLength() > 0) {
                                    // If the priority mechanism is active, this method will be non-blocking
                                    if (0 != (rc = pConnection->sendTCPDataToRemoteHost (pEntry, pEntry->pTCPSegment->getData(),
                                                                                         pEntry->pTCPSegment->getItemLength(),
                                                                                         pEntry->pTCPSegment->getTCPFlags() & TCP_DATA_FLAGS_MASK))) {

                                        if (rc == -4) {
                                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Info,
                                                            "L%hu-R%hu: sendTCPPacketToHost() dropping the send for this cycle\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                                            break;
                                        }
                                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                        "L%hu-R%hu: sendTCPDataToRemoteHost() failed with rc = %d; sending an RST packet to the "
                                                        "local application and clearing the connection\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);

                                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                            "L%hu-R%hu: sendTCPPacketToHost() failed sending an RST to local host; with rc = %d\n",
                                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                        }
                                        pEntry->reset();
                                        delete pEntry->pTCPSegment;
                                        pEntry->pTCPSegment = nullptr;
                                        break;
                                    }

                                    pEntry->i64RemoteActionTime = i64CurrTime;
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                                    "L%hu-R%hu: transmitted %hu bytes of TCP data with FLAGs %hhu (FIN flag set) "
                                                    "via %s to the remote NetProxy\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                    pEntry->pTCPSegment->getItemLength(), pEntry->pTCPSegment->getTCPFlags(),
                                                    pConnection->getConnectorTypeAsString());
                                }

                                // Look at the local state to check if NetProxy needs to ACK the FIN
                                if ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_FIN_WAIT_1) ||
                                    (pEntry->localState == TCTLS_FIN_WAIT_2)) {
                                    // FIN packet has been received and it has to be ACKed --> ui32NextExpectedInSeqNum has to be incremented by 1
                                    pEntry->ui32NextExpectedInSeqNum++;
                                    pEntry->i64LocalActionTime = pEntry->i64LastAckTime;
                                    if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                                        checkAndLogMsg ("TCPManager::remoteTransmitterThreadRun", NOMADSUtil::Logger::L_MildError,
                                                        "L%hu-R%hu: failed to send FIN+ACK packet; rc = %d\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                        TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                        pEntry->reset();
                                        break;
                                    }

                                    // Update the local state
                                    if (pEntry->localState == TCTLS_ESTABLISHED) {
                                        // Moving to CLOSE_WAIT
                                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                        "L%hu-R%hu: received all missing packets, local state moved from %d to CLOSE_WAIT;\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState);
                                        pEntry->localState = TCTLS_CLOSE_WAIT;
                                    }
                                    else if (pEntry->localState == TCTLS_FIN_WAIT_1) {
                                        // Previously sent FIN still has to be ACKed and a FIN has been received --> moving to local state CLOSING
                                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                        "L%hu-R%hu: received all missing packets while local state was FIN_WAIT_1 - "
                                                        "sent ACK to local application and moved to CLOSING (Simultaneous Close Sequence)\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                                        pEntry->localState = TCTLS_CLOSING;
                                    }
                                    else if (pEntry->localState == TCTLS_FIN_WAIT_2) {
                                        // Moving to TIME_WAIT
                                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Info,
                                                        "L%hu-R%hu: received all missing packets while local state was FIN_WAIT_2 - "
                                                        "sent ACK to local application and moved to TIME_WAIT\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                                        pEntry->localState = TCTLS_TIME_WAIT;
                                    }
                                }

                                // Flush any data left in the compressor buffer. NOTE: this method might have to be made non-blocking
                                if (0 != (rc = TCPManager::flushAndSendCloseConnectionRequest (pEntry))) {
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                                    "L%hu-R%hu: failed to flush data and to send a CloseTCPConnection request to remote proxy; rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                    if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                        checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                        "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                    }
                                    pEntry->reset();
                                    break;
                                }

                                // Update the remote state
                                if (pEntry->remoteState == TCTRS_ConnRequestSent) {
                                    pEntry->remoteState = TCTRS_DisconnRequestSent;
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent a CloseTCPConnection response to the remote NetProxy; "
                                                    "remote state moved from ConnRequestSent to DisconnRequestedSent\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                                }
                                else if (pEntry->remoteState == TCTRS_ConnEstablished) {
                                    pEntry->remoteState = TCTRS_DisconnRequestSent;
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent a CloseTCPConnection request to the remote NetProxy; "
                                                    "remote state moved from ConnEstablished to DisconnRequestedSent\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                                }
                                else if (pEntry->remoteState == TCTRS_DisconnRequestReceived) {
                                    pEntry->remoteState = TCTRS_Disconnected;
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent a CloseTCPConnection response to the remote NetProxy; "
                                                    "remote state moved from DisconnRequestReceived to Disconnected\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                                    pEntry->resetConnectors();
                                }
                                pEntry->i64RemoteActionTime = i64CurrTime;

                                // CloseTCPConnection request has been sent to remote proxy --> nothing else to do for this entry
                                delete pEntry->pTCPSegment;
                                pEntry->pTCPSegment = nullptr;
                                break;
                            }
                            else if (pEntry->pTCPSegment->getItemLength() == 0) {
                                // Empty packet (no data), no FIN flag
                                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_HighDetailDebug,
                                                "L%hu-R%hu: dequeue() returned an empty data packet (buffering compression?) with FLAGs %hhu; NetProxy "
                                                "will discard it\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->pTCPSegment->getTCPFlags());
                                delete pEntry->pTCPSegment;
                                pEntry->pTCPSegment = nullptr;
                                continue;
                            }

                            // Send data to the remote NetProxy over the connection
                            if (0 != (rc = pConnection->sendTCPDataToRemoteHost (pEntry, pEntry->pTCPSegment->getData(), pEntry->pTCPSegment->getItemLength(),
                                                                                 pEntry->pTCPSegment->getTCPFlags()))) {
                                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: sendTCPDataToRemoteHost() failed with rc = %d; sending an RST packet to the local "
                                                "application and clearing the connection\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                    "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                break;
                            }

                            pEntry->i64RemoteActionTime = i64CurrTime;
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "L%hu-R%hu: transmitted %hu bytes of TCP data with FLAGs %hhu via %s to the remote Netproxy\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->pTCPSegment->getItemLength(),
                                            pEntry->pTCPSegment->getTCPFlags(), pConnection->getConnectorTypeAsString());
                            delete pEntry->pTCPSegment;
                            pEntry->pTCPSegment = nullptr;
                        }
                        else if ((pEntry->remoteState == TCTRS_WaitingConnEstablishment) || (pEntry->remoteState == TCTRS_ConnRequestSent)) {
                            // Connection with the remote proxy is being established and there are already bytes in the buffer
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                            "L%hu-R%hu: there are %u bytes in the input buffer and %u bytes ready to transmit, but the "
                                            "connection to the remote proxy is not yet established: local state is %d, remote state is %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingTotalBytesCount(),
                                            pEntry->getOutgoingReadyBytesCount(), pEntry->localState, pEntry->remoteState);
                            break;
                        }
                        else if (pEntry->getOutgoingTotalBytesCount() > 0) {
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: there are %u bytes in the input buffer and %u bytes ready to transmit, but "
                                            "the state of the connection to the remote NetProxy is %d; resetting the connection\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingTotalBytesCount(),
                                            pEntry->getOutgoingReadyBytesCount(), pEntry->remoteState);

                            if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                                "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            break;
                        }
                        else {
                            // I don't need to store this since it is a duplicate
                            const TCPSegment * const pTCPSegment =
                                pEntry->dequeueLocallyReceivedData (NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
                            if (pTCPSegment == nullptr) {
                                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: dequeue() failed; sending RST packet to local host and clearing connection\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID);
                                if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                                    "L%hu-R%hu: sendTCPPacketToHost() failed with rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                break;
                            }

                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: dequeueLocallyReceivedData returned a (duplicate?) packet with FLAGs "
                                            "%hhu and %hu bytes of data; deleting packet and continuing execution\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pTCPSegment->getTCPFlags(),
                                            pTCPSegment->getItemLength());
                            delete pTCPSegment;
                        }
                    }

                    // If enough space has been freed in the TCP window, the NetProxy notifies the local host
                    if ((beginningTCPWindowPercentage < 30.0) && (pEntry->getOutgoingBufferRemainingSpacePercentage() >= 30.0) &&
                        ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_FIN_WAIT_1) ||
                        (pEntry->localState == TCTLS_FIN_WAIT_2))) {
                        if (0 != (rc = _rTCPManager.sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: failed to send ACK packet to signal present TCP Window Size to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            TCPManager::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                        }
                        else {
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            pEntry->i64LocalActionTime = i64CurrTime;
                            checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "L%hu-R%hu: sent a TCP Window Update to notify the remote host about a significant "
                                            "increase in the receiver's buffer free space (new: %.2f%% free - old: %.2f%% free)\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getOutgoingBufferRemainingSpacePercentage(),
                                            beginningTCPWindowPercentage);
                        }
                    }
                }
            }

            // Update the variable that keeps track of the highest priority seen
            if (_rTCPConnTable.getNewHighestPriority() != _rTCPConnTable.getHighestKnownPriority()) {
                checkAndLogMsg ("RemoteTCPTransmitterThread::run", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "The TCPConnTable's highest known priority value changed from %u to %u\n",
                                _rTCPConnTable.getHighestKnownPriority(), _rTCPConnTable.getNewHighestPriority());
                _rTCPConnTable.setHighestKnownPriority (_rTCPConnTable.getNewHighestPriority());
            }
            _bNotified = false;
        }

        terminating();
    }

    const int64 RemoteTCPTransmitterThread::I64_RTT_TIME_BETWEEN_ITERATIONS;
}