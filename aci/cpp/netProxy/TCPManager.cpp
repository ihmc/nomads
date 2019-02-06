/*
 * TCPManager.h
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

#include <algorithm>

#include "TCPManager.h"
#include "PacketBufferManager.h"
#include "Entry.h"
#include "TCPConnTable.h"
#include "ConnectorReader.h"
#include "ConnectorWriter.h"
#include "NetworkInterface.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "PacketRouter.h"
#include "StatisticsManager.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    int TCPManager::handleTCPPacketFromHost (const uint8 * const pPacket, uint16 ui16PacketLen, LocalTCPTransmitterThread & rLocalTCPTransmitterThread,
                                             RemoteTCPTransmitterThread & rRemoteTCPTransmitterThread)
    {
        static int rc;

        // Assumptions: pPacket does not include EtherFrameHeader (points to IPHeader); pPacket is in host byte order
        const auto * const pIPHeader = reinterpret_cast<const NOMADSUtil::IPHeader *> (pPacket);
        uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        const auto * const pTCPHeader = reinterpret_cast<const NOMADSUtil::TCPHeader *> (pPacket + ui16IPHeaderLen);
        uint16 ui16TCPHeaderLen = (pTCPHeader->ui8Offset >> 4) * 4;
        uint16 ui16DataSize = ui16PacketLen - (ui16IPHeaderLen + ui16TCPHeaderLen);
        uint32 ui32ACKedPackets = 0;
        uint64 packetTime = NOMADSUtil::getTimeInMilliseconds();

        auto * const pEntry = _rTCPConnTable.getEntry (pIPHeader->srcAddr.ui32Addr, pTCPHeader->ui16SPort,
                                                       pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort);

        std::unique_lock<std::mutex> ulEntry{pEntry->getMutexRef()};
        uint64 oldWinRecSize = pEntry->ui16ReceiverWindowSize;
        if (pEntry->localState == TCTLS_CLOSED) {
            // Check if it is necessary to send an RST, and then drop the packet
            if (!(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST)) {
                uint32 ui32OutSeqNum = (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) ? pTCPHeader->ui32AckNum : 0;
                uint8 ui8Flags = (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) ?
                    NOMADSUtil::TCPHeader::TCPF_RST : NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK;
                pEntry->ui32NextExpectedInSeqNum = (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) ? 0 :
                    pTCPHeader->ui32SeqNum + ui16DataSize + ((pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_SYN) ? 1 : 0);
                if (0 != (rc = sendTCPPacketToHost (pEntry, ui8Flags, ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                    "L%hu-R0: failed to send an RST packet; rc = %d\n",
                                    pEntry->ui16ID, rc);
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                    "L%hu-R0: received a packet with flag %hhu from <%s:%hu> to <%s:%hu>, but the TCP "
                                    "connection is in state CLOSED; sent an RST\n", pEntry->ui16ID, pTCPHeader->ui8Flags,
                                    NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16SPort,
                                    NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort);
                }
                pEntry->reset();
            }
            else {
                if ((pEntry->ui32OutSeqNum == 0) && (pEntry->ui32NextExpectedInSeqNum == 0)) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R0: received a valid RST packet for the TCP connection from <%s:%hu> to <%s:%hu>, "
                                    "which is in state CLOSED; clearing the entry\n", pEntry->ui16ID, pTCPHeader->ui8Flags,
                                    NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16SPort,
                                    NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort);
                    pEntry->clear();
                }
                else if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) {
                    // Check if the RST is acceptable
                    if (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui32SeqNum) &&
                        NOMADSUtil::SequentialArithmetic::lessThan (pTCPHeader->ui32SeqNum, (pEntry->ui32NextExpectedInSeqNum + pEntry->getOutgoingBufferRemainingSpace()))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R0: received a valid RST+ACK packet for the TCP connection from <%s:%hu> to <%s:%hu>, "
                                        "which is in state CLOSED; clearing the entry\n", pEntry->ui16ID, pTCPHeader->ui8Flags,
                                        NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16SPort,
                                        NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort);
                        pEntry->clear();
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R0: received an invalid RST+ACK packet for the TCP connection from <%s:%hu> to <%s:%hu>, "
                                        "which is in state CLOSED; current SEQ NUM is %u and the next expected SEQ NUM is %u; the "
                                        "received RST packet has SEQ NUM %u and ACK NUM %u; dropping the received RST packet\n", pEntry->ui16ID,
                                        pTCPHeader->ui8Flags, NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(),
                                        pTCPHeader->ui16SPort, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(),
                                        pTCPHeader->ui16DPort, pEntry->ui32OutSeqNum, pEntry->ui32NextExpectedInSeqNum,
                                        pTCPHeader->ui32SeqNum, pTCPHeader->ui32AckNum);
                    }
                }
                else {
                    // Received a RST without ACK flag when in state CLOSED --> ignore it
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R0: received an invalid RST packet with no ACK flag set for the TCP connection from <%s:%hu> "
                                    "to <%s:%hu>, which is in state CLOSED; current SEQ NUM is %u and the next expected SEQ NUM is %u; "
                                    "the received RST packet has SEQ NUM %u; dropping the received RST packet\n", pEntry->ui16ID,
                                    pTCPHeader->ui8Flags, NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(),
                                    pTCPHeader->ui16SPort, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(),
                                    pTCPHeader->ui16DPort, pEntry->ui32OutSeqNum, pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui32SeqNum);
                }
            }

            return 0;
        }

        if (pEntry->localState == TCTLS_LISTEN) {
            // The following is an equality test on purpose: it should only be true for SYN-only packets, not for SYN+ACK packets
            if (pTCPHeader->ui8Flags == NOMADSUtil::TCPHeader::TCPF_SYN) {
                /* Need to release the lock on the entry and acquire both the lock on the TCPConnTable and the Entry atomically
                 * to avoid a potential deadlock with the Connection::prepareForDelete() method, which acquires the lock on the
                 * TCPConnTable first and then tries to acquire the lock on all Entries, one by one */
                ulEntry.unlock();
                std::lock (_rTCPConnTable.getMutexRef(), pEntry->getMutexRef());
                std::unique_lock<std::mutex> ulTCPConnTable{_rTCPConnTable.getMutexRef(), std::adopt_lock};
                ulEntry = std::unique_lock<std::mutex>{pEntry->getMutexRef(), std::adopt_lock};

                // Received a SYN --> moving to SYN_RCVD
                pEntry->prepareNewConnection();
                pEntry->localState = TCTLS_SYN_RCVD;
                pEntry->ui32StartingInSeqNum = pTCPHeader->ui32SeqNum;                      // Starting SEQ Number of the communication used by the local application
                pEntry->setNextExpectedInSeqNum (pTCPHeader->ui32SeqNum + 1);               // Next expected sequence number sent by the host, i.e., the next ACK NetProxy will send to the local application

                // SYN received --> this is the only time we need to look up the remote NetProxy address
                const auto * pProtocolSetting = _rConfigurationManager.mapAddrToProtocol (pIPHeader->srcAddr.ui32Addr, pTCPHeader->ui16SPort,
                                                                                          pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort,
                                                                                          NOMADSUtil::IP_PROTO_TCP);
                if (!pProtocolSetting) {
                    pProtocolSetting = ProtocolSetting::getDefaultTCPProtocolSetting();
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                    "L%hu-R0: received a TCP SYN packet with SEQ NUM %u, source <%s:%hu>, and destination <%s:%hu> for which "
                                    "it was not possible to retrieve the ProtocolSettings; NetProxy will use the default protocol %s\n",
                                    pEntry->ui16ID, pTCPHeader->ui32SeqNum, NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(),
                                    pTCPHeader->ui16SPort, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort,
                                    pProtocolSetting->getProxyMessageProtocolAsString());
                }

                const auto connectorType = pProtocolSetting->getConnectorTypeFromProtocol();
                QueryResult query{_rConnectionManager.queryConnectionToRemoteHostForConnectorType (pIPHeader->srcAddr.ui32Addr, pTCPHeader->ui16SPort,
                                                                                                   pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort,
                                                                                                   connectorType, pProtocolSetting->getEncryptionType())};
                const auto iaRemoteProxyAddr = query.getBestConnectionAddressSolution();
                if (iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                    "L%hu-R0: received a SYN packet with SEQ NUM %u, source <%s:%hu>, and destination <%s:%hu> that could not be "
                                    "remapped to a remote NetProxy (remote NetProxy reachability set to false?); NetProxy will send back an RST\n",
                                    pEntry->ui16ID, pTCPHeader->ui32SeqNum, NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(),
                                    pTCPHeader->ui16SPort, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort);
                    pEntry->ui32StartingOutSeqNum = 0;
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                        "L%hu-R0: failed to send an RST+ACK packet; rc = %d\n",
                                        pEntry->ui16ID, rc);
                    }
                    pEntry->reset();
                    return 0;
                }
                pEntry->ui32RemoteProxyUniqueID = query.getRemoteProxyUniqueID();
                pEntry->iaLocalInterfaceAddr = query.getLocalProxyInterfaceAddress();
                pEntry->iaRemoteProxyAddr = iaRemoteProxyAddr;
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "L%hu-R0: received a SYN packet with SEQ number %u from an application at address <%s:%hu> "
                                "with destination address <%s:%hu>\n", pEntry->ui16ID, pTCPHeader->ui32SeqNum,
                                NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16SPort,
                                NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort);

                pEntry->ui32StartingInSeqNum = pTCPHeader->ui32SeqNum;                      // Starting SEQ Number of the communication used by the local host
                pEntry->ui32ReceivedDataSeqNum = pTCPHeader->ui32SeqNum;
                pEntry->ui16ReceiverWindowSize = pTCPHeader->ui16Window;
                pEntry->i64LocalActionTime = packetTime;                                    // Time at which the last event involving this local TCP connection occurred
                pEntry->i64LastAckTime = packetTime;                                        // When receiving a SYN, the LastAckTime is updated even though no ACKs were effectively received

                pEntry->setProtocol (pProtocolSetting->getProxyMessageProtocol());
                pEntry->assignedPriority = pProtocolSetting->getPriorityLevel();
                delete pEntry->setProperConnectorWriter (ConnectorWriter::connectorWriterFactory (pProtocolSetting->getCompressionSetting()));

                auto * pConnection = query.getActiveConnectionToRemoteProxy();
                // openNewConnectionToRemoteProxy() returns nullptr unless the new connection has been established
                pConnection = (pConnection && (pConnection->isConnected() || pConnection->isConnecting())) ?
                    pConnection : Connection::openNewConnectionToRemoteProxy (_rConnectionManager, _rTCPConnTable, *this, _rPacketRouter,
                                                                              _rStatisticsManager, query, false);
                // Set connection even if not yet connected
                pConnection = pConnection ? pConnection :
                    Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                        iaRemoteProxyAddr, connectorType, pProtocolSetting->getEncryptionType());
                if (!pConnection || pConnection->hasFailedConnection()) {
                    pEntry->ui32StartingOutSeqNum = 0;
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R0: failed to send an RST+ACK packet; rc = %d\n",
                                        pEntry->ui16ID, rc);
                    }
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                    "L%hu-R0: received a SYN, but it was impossible to create a new %sConnection to connect the "
                                    "remote NetProxy at address %s in order to remap a new TCP Connection from %s:%hu to %s:%hu\n",
                                    pEntry->ui16ID, connectorTypeToString (connectorType), iaRemoteProxyAddr.getIPAsString(),
                                    NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16SPort,
                                    NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort);
                    pEntry->reset();
                    return -2;
                }
                pEntry->setConnection (pConnection);
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                "L%hu-R0: the new TCP Connection from %s:%hu to %s:%hu will be remapped over the %s connection "
                                "with local endpoint <%s:%hu> and remote endpoint <%s:%hu> (remote NetProxy with UniqueID %u)\n",
                                pEntry->ui16ID, NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16SPort,
                                NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort,
                                pConnection->getConnectorTypeAsString(), pConnection->getLocalInterfaceInetAddr()->getIPAsString(),
                                pConnection->getLocalInterfaceInetAddr()->getPort(), pConnection->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                pConnection->getRemoteInterfaceLocalInetAddr()->getPort(), pConnection->getRemoteNetProxyID());

                // Check if is necessary to send an OpenTCPConnectionRequest to avoid sending multiple openTCPConnection requests
                if (pEntry->remoteState == TCTRS_Unknown) {
                    pEntry->i64RemoteActionTime = packetTime;
                    auto * const pConnection = pEntry->getConnection();
                    if (pConnection && pConnection->isConnected()) {
                        bool bReachable = _rConnectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (pEntry->ui32RemoteProxyUniqueID,
                                                                                                                  query.getLocalProxyInterfaceAddress().getIPAddress(),
                                                                                                                  iaRemoteProxyAddr.getIPAddress());
                        if (0 != (rc = pConnection->sendOpenTCPConnectionRequest (pEntry, bReachable))) {
                            pEntry->ui32StartingOutSeqNum = 0;
                            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                                "L%hu-R0: failed to send an RST+ACK packet; rc = %d\n",
                                                pEntry->ui16ID, rc);
                            }
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R0: received a SYN, but it was impossible to send an OpenTCPConnection request to the remote NetProxy at "
                                            "address <%s:%hu>: sendOpenTCPConnectionRequest() failed with rc = %d; sent back an RST to local application\n",
                                            pEntry->ui16ID, pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort(), rc);
                            pEntry->reset();
                            return -3;
                        }
                        else {
                            pEntry->remoteState = TCTRS_ConnRequestSent;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "L%hu-R0: received a SYN packet; sent a OpenTCPConnection request to remote proxy at address <%s:%hu>\n",
                                            pEntry->ui16ID, pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort());
                        }
                    }
                    else if (pConnection) {
                        pEntry->remoteState = TCTRS_WaitingConnEstablishment;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "L%hu-R0: received a SYN packet when remote connection is in state %d; waiting to "
                                        "establish a connection via %s with the remote NetProxy at address <%s:%hu>\n",
                                        pConnection->getStatus(), pEntry->ui16ID, connectorTypeToString(connectorType),
                                        pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort());
                    }
                    else {
                        pEntry->ui32StartingOutSeqNum = 0;
                        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                            "L%hu-R0: failed to send an RST+ACK packet; rc = %d\n",
                                            pEntry->ui16ID, rc);
                        }
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R0: received a SYN, but it was impossible to open a new Connection to the remote NetProxy "
                                        "with address <%s:%hu>; an RST was sent back to the local application\n", pEntry->ui16ID,
                                        pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort());
                        pEntry->reset();
                        return -4;
                    }
                }

                if (!NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE) {
                    // Do not synchronize the local TCP handshake with the remote one --> immediately reply with a SYN-ACK
                    if (0 != (rc = sendTCPPacketToHost (pEntry, (NOMADSUtil::TCPHeader::TCPF_SYN | NOMADSUtil::TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                        "L%hu-R0: failed to send a SYN+ACK packet in response to a SYN packet ("
                                        "SynchronizeTCPHandshake option disabled); rc = %d\n", pEntry->ui16ID, rc);
                        pEntry->reset();
                        return -5;
                    }
                    pEntry->ui32OutSeqNum++;
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R0: received a SYN packet; SynchronizeTCPHandshake option disabled; sent a SYN+ACK packet with ACK %u "
                                    "(relative %u) and SEQ num %u (relative %u); moved to SYN_RCVD\n", pEntry->ui16ID, pEntry->ui32NextExpectedInSeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                    pEntry->ui32OutSeqNum - 1, NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1, pEntry->ui32StartingOutSeqNum));
                }

                return 0;
            }
            // Connection is not open --> sending back an RST if the received packet is not an RST itself
            else if (!(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST)) {
                uint32 ui32OutSeqNum = (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) ? pTCPHeader->ui32AckNum : 0;
                uint8 ui8Flags = (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) ?
                    NOMADSUtil::TCPHeader::TCPF_RST : NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK;
                pEntry->ui32NextExpectedInSeqNum = (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) ?
                    0 : pTCPHeader->ui32SeqNum + ui16DataSize;      // This cannot be a SYN packet because that is handled above --> do not increment ACK NUM by 1
                if (0 != (rc = sendTCPPacketToHost (pEntry, ui8Flags, ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R0: failed to send an RST packet; rc = %d\n",
                                    pEntry->ui16ID, rc);
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R0: received a packet with flag %hhu but connection is not established (state is %d); sent an RST\n",
                                    pEntry->ui16ID, pTCPHeader->ui8Flags, pEntry->localState);
                }
            }

            if (pTCPHeader->ui8Flags != NOMADSUtil::TCPHeader::TCPF_SYN) {
                // If it's not a SYN, either we have sent an RST or received one --> resetting connection
                pEntry->reset();
            }

            return 0;
        }
        else if (pEntry->localState == TCTLS_SYN_SENT) {
            // Local state is SYN_SENT --> prepareNewConnection() has already been invoked in the openTCPConnectionToHost() method
            if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) {
                if (pTCPHeader->ui32AckNum != pEntry->ui32OutSeqNum) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a SYN+ACK but with the wrong ack num - expected %u, got %u; sending RST\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum, pTCPHeader->ui32AckNum);
                    // Probably the local application responded to an OLD SYN request; sending back an RST and dropping the packet
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pTCPHeader->ui32AckNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    }

                    // Don't modify neither local state nor pEntry. Wait for right SYN+ACK response
                    return  0;
                }
            }

            // If we get here, either the ACK is acceptable or there is no ACK flag set
            if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST) {
                if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                    "L%hu-R%hu: received an RST while in state SYN_SENT; reset connection\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                    sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->clear();
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                    "L%hu-R%hu: received an RST without ACK flag set while in state SYN_SENT; the "
                                    "NetProxy will ignore and drop the packet\n", pEntry->ui16ID, pEntry->ui16RemoteID);
                }

                return 0;
            }

            // BEFORE PROCEEDING THERE SHOULD BE A CHECK FOR SECURITY AND PRECEDENCE <--- RFC 793
            if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_SYN) {
                // Move to the ESTABLISHED state and send an ACK
                pEntry->ui32StartingInSeqNum = pTCPHeader->ui32SeqNum;                  // Starting Sequence Number of the communication used by the local host.
                pEntry->setNextExpectedInSeqNum (pTCPHeader->ui32SeqNum + 1);           // Next expected sequence number sent by the host, i.e. the next ACK the host will send to local host
                pEntry->ui16ReceiverWindowSize = pTCPHeader->ui16Window;
                pEntry->i64LocalActionTime = packetTime;
                pEntry->dqLocalHostOutBuf.pop_front();                                  // Remove the SYN packet from the outgoing packets queue regardless of whether the local state is moving to ESTABLISHED or SYN_RCVD

                if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) {
                    // Received a SYN+ACK with the correct ACK number --> connection ESTABLISHED
                    pEntry->ui32ReceivedDataSeqNum = pTCPHeader->ui32SeqNum;            // last SEQNUM received
                    pEntry->ui32LastAckSeqNum = pTCPHeader->ui32AckNum;                 // last ACKNUM received
                    pEntry->i64LastAckTime = packetTime;                                // last received ACK time
                    pEntry->ui8RetransmissionAttempts = 0;                              // Reset retransmission attempts
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK packet to finish establishing the connection; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send an RST packet to local host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -6;
                    }

                    pEntry->localState = TCTLS_ESTABLISHED;
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                    "L%hu-R%hu: local connection ESTABLISHED. Proxy packets with SEQ %u, host packets with SEQ %u\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum, pEntry->ui32NextExpectedInSeqNum);

                    if (pEntry->remoteState == TCTRS_ConnRequestReceived) {
                        // Sending a TCPConnectionOpened Response to remote proxy
                        if (pEntry->getConnection() && pEntry->getConnection()->isConnected()) {
                            if (pEntry->getConnection()->isEnqueueingAllowed()) {
                                bool bReachable =
                                    _rConnectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (pEntry->ui32RemoteProxyUniqueID,
                                                                                                            pEntry->iaLocalInterfaceAddr.getIPAddress(),
                                                                                                            pEntry->iaRemoteProxyAddr.getIPAddress());
                                if (0 != (rc = pEntry->getConnection()->sendTCPConnectionOpenedResponse (pEntry, bReachable))) {
                                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                                    "L%hu-R%hu: could not send a TCPConnectionOpened ProxyMessage to the remote NetProxy at address "
                                                    "<%s:%hu>; rc = %d; sending an RST to the local application and resetting the Entry\n", pEntry->ui16ID,
                                                    pEntry->ui16RemoteID, pEntry->getConnection()->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                                    pEntry->getConnection()->getRemoteInterfaceLocalInetAddr()->getPort(), rc);
                                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                                        "L%hu-R%hu: failed to send an RST packet to local host; rc = %d\n",
                                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                    }

                                    pEntry->reset();
                                    return -7;
                                }
                            }
                            else {
                                // Enqueuing not allowed
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_NetDetailDebug,
                                                "L%hu-R%hu: impossible to enqueue a TCPConnectionOpened ProxyMessage with the Connection object\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID);
                                return 0;
                            }
                        }
                        else if (pEntry->getConnection() && pEntry->getConnection()->isConnecting()) {
                            // Local connection state is TCTLS_ESTABLISHED, remote connection state is TCTRS_ConnRequestReceived
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "L%hu-R%hu: received a SYN+ACK packet when the remote connection is in state %d; waiting "
                                            "to establish a connection via %s with the remote NetProxy at address <%s:%hu>\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getConnection()->getStatus(),
                                            pEntry->getConnection()->getConnectorTypeAsString(), pEntry->iaRemoteProxyAddr.getIPAsString(),
                                            pEntry->iaRemoteProxyAddr.getPort());
                            return 0;
                        }
                        else {
                            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send an RST packet to local host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }

                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: received a valid SYN+ACK packet while in SYN_SENT state, but it was impossible "
                                            "to send a TCPConnectionOpened ProxyMessage to the remote NetProxy at address <%s:%hu>; "
                                            "sent back an RST to the local application\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort());
                            pEntry->reset();
                            return -8;
                        }

                        const NOMADSUtil::InetAddr iaSourceIP{pEntry->ui32LocalIP};
                        const NOMADSUtil::InetAddr iaDestinationIP{pEntry->ui32RemoteIP};
                        pEntry->remoteState = TCTRS_ConnEstablished;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                        "L%hu-R%hu: successfully sent a TCPConnectionOpened ProxyMessage for the TCP connection from <%s:%hu> to <%s:%hu>, "
                                        "remapped over to the NetProxy with UniqueID %u at address <%s:%hu>; both the local and the remote connection are "
                                        "now in state ESTABLISHED (the local state went from SYN_SENT to ESTABLISHED); data will be coded to %s:%hhu and "
                                        "decoded from %s:%hhu\n", pEntry->ui16ID, pEntry->ui16RemoteID, iaSourceIP.getIPAsString(), pEntry->ui16LocalPort,
                                        iaDestinationIP.getIPAsString(), pEntry->ui16RemotePort, pEntry->ui32RemoteProxyUniqueID,
                                        pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort(),
                                        pEntry->getConnectorWriter()->getCompressionName(), pEntry->getConnectorWriter()->getCompressionLevel(),
                                        pEntry->getConnectorReader()->getCompressionName(), pEntry->getConnectorReader()->getCompressionLevel());
                    }
                    else if (pEntry->remoteState == TCTRS_ConnEstablished) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R%hu: received the first SYN+ACK from the local application when the connection with the remote "
                                        "NetProxy at address <%s:%hu> was already established\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                        pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort());
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R%hu: received the first SYN+ACK from the local application, but the connection with the remote "
                                        "NetProxy at address %s is in state %d; local and remote connections are out of sync; resetting\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->remoteState);
                        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                            "L%hu-R%hu: failed to send an RST packet to local host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -9;
                    }
                }
                else {
                    // localState is SYN_SENT, remote state must be TCTRS_ConnEstablished --> we can proceed regardless of the SynchronizeTCPHandshake option
                    pEntry->localState = TCTLS_SYN_RCVD;
                    if (0 != (rc = sendTCPPacketToHost (pEntry, (NOMADSUtil::TCPHeader::TCPF_SYN | NOMADSUtil::TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send SYN+ACK packet after having moved from SYN_SENT to SYN_RCVD; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -10;
                    }
                    pEntry->ui32OutSeqNum++;
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                    "L%hu-R%hu: received a SYN packet while in state SYN_SENT; sent a SYN+ACK packet with SEQ number %u "
                                    "and moved to SYN_RCVD\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum - 1);
                }
            }

            // If there is already data in the buffer, wake up the LTTT
            if (pEntry->dqLocalHostOutBuf.size() > 0) {
                ulEntry.unlock();
                rLocalTCPTransmitterThread.wakeUpThread();
            }

            // Returning after having moved to state ESTABLISHED or SYN_RCVD
            return 0;
        }
        else if ((pEntry->localState == TCTLS_SYN_RCVD) && (pTCPHeader->ui8Flags == NOMADSUtil::TCPHeader::TCPF_SYN)) {
            /* Duplicate SYN packet: only reply if the remote connection is in status ConnEstablished or if TCP synchronization is disabled.
             * Note that there is no need to call prepareNewConnection(), as it was called when the first SYN was received. */
            if ((pTCPHeader->ui32SeqNum == (pEntry->ui32NextExpectedInSeqNum - 1)) &&
                (!NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE || (pEntry->remoteState == TCTRS_ConnEstablished))) {
                // SYN+ACK was lost and can be retransmitted safely: expected SEQ NUM + SynchronizeTCPHandshake disabled OR remote connection established
                if (0 != (rc = sendTCPPacketToHost (pEntry, (NOMADSUtil::TCPHeader::TCPF_SYN | NOMADSUtil::TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send a SYN+ACK packet having received a duplicate SYN in state SYN_RCVD; "
                                    "rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    return -11;
                }
                pEntry->i64LocalActionTime = packetTime;
                pEntry->i64LastAckTime = packetTime;
                checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_LowDetailDebug,
                                "L%hu-R%hu: sent SYN+ACK packet with ACK %u (relative %u) and SEQ num %u (relative %u) to reply "
                                "to a duplicate SYN\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                pEntry->ui32OutSeqNum - 1, NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1, pEntry->ui32StartingOutSeqNum));
            }
            else {
                checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: received a duplicate SYN packet while remote connection state is %d; "
                                "NetProxy cannot reply to it yet and will therefore discard the packet\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteState);
            }

            // If the previous equality does not hold true, then we are still trying to connect to the remote NetProxy --> we simply ignore the SYN
            return 0;
        }

        // Following checkings obey to RFC 793 directives
        if ((pEntry->getOutgoingBufferRemainingSpace() == 0) && (ui16DataSize == 0)) {
            if (!(pEntry->ui32NextExpectedInSeqNum == pTCPHeader->ui32SeqNum)) {
                if (!(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST)) {
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK in answer to a packet that cannot be received because "
                                        "the local buffer is full; rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -12;
                    }
                    else {
                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                        pEntry->i64LocalActionTime = packetTime;
                        pEntry->i64LastAckTime = packetTime;                                    // We probably just received an ACK or FIN+ACK packet
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received a packet with a wrong SEQ number (received %u, expecting %u) "
                                        "and flags %hhu while buffer window is full; sent back an ACK\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                        pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui8Flags);
                    }
                }
                return 0;
            }
            else {
                if (!(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) && !(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_URG) &&
                    !(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST)) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with FLAGs %hhu that cannot be processed because buffer window is full; dropped\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui8Flags);
                    return 0;
                }
            }
        }
        else if ((pEntry->getOutgoingBufferRemainingSpace() > 0) && (ui16DataSize == 0)) {
            if ((pTCPHeader->ui32SeqNum == pEntry->ui32StartingInSeqNum) && (pEntry->localState == TCTLS_SYN_RCVD)) {
                // The previous SYN+ACK was lost --> retransmit it
                if (0 != (rc = sendTCPPacketToHost (pEntry, (NOMADSUtil::TCPHeader::TCPF_SYN | NOMADSUtil::TCPHeader::TCPF_ACK),
                                                    (pEntry->ui32OutSeqNum - 1)))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R0: failed to send SYN+ACK packet while in local state SYN_RCVD and following the reception of a "
                                    "duplicated packet with the initial SEQ NUM %u; rc = %d\n", pEntry->ui16ID, pTCPHeader->ui32SeqNum, rc);
                    sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    return -13;
                }
                else {
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    pEntry->i64LocalActionTime = packetTime;
                    pEntry->i64LastAckTime = packetTime;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R0: received a duplicate SYN packet (likely loss of SYN+ACK packet); sent a SYN+ACK "
                                    "packet with ACK %u (relative %u) and SEQ NUM %u (relative %u); moved to SYN_RCVD\n",
                                    pEntry->ui16ID, pEntry->ui32NextExpectedInSeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                    pEntry->ui32OutSeqNum - 1, NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1, pEntry->ui32StartingOutSeqNum));
                }
                return 0;
            }
            else if (!NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui32SeqNum) ||
                !NOMADSUtil::SequentialArithmetic::lessThan (pTCPHeader->ui32SeqNum,
                                                             (pEntry->ui32NextExpectedInSeqNum + pEntry->getOutgoingBufferRemainingSpace()))) {
                // Packet not acceptable --> sending back an ACK with right SEQ NUM and window size
                if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                    "failed to send ACK in answer to a packet that couldn't be saved in window buffer; rc = %d\n", rc);
                    sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    return -14;
                }
                else {
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    pEntry->i64LocalActionTime = packetTime;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with SEQ %u (relative %u), 0 bytes of data and flags %hhu while in state %d; "
                                    "expecting SEQ number %u (relative %u); window size is %u; sent back an ACK with correct values\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    pTCPHeader->ui8Flags, pEntry->localState, pEntry->ui32NextExpectedInSeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                    pEntry->getOutgoingBufferRemainingSpace());
                }
                return 0;
            }
        }
        else if ((pEntry->getOutgoingBufferRemainingSpace() == 0) && (ui16DataSize > 0)) {
            // Impossible to store a packet with data since window free space is zero --> sending back an ACK with window status and correct ACK and SEQ numbers
            if (!(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST)) {
                if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send ACK in answer to a packet that couldn't be saved in window buffer; rc = %d\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    return -15;
                }
                else {
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    pEntry->i64LocalActionTime = packetTime;
                    pEntry->i64LastAckTime = packetTime;                                    // We probably received a valid ACK, but local buffer is full
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with with SEQ %u (relative %u), flags %hhu and %hu bytes "
                                    "of data that couldn't be stored in the full buffer window; sent back an ACK\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    pTCPHeader->ui8Flags, ui16DataSize);
                }
            }
            // Invalid RST received or sent ACK with correct values --> drop packet and return
            return 0;
        }
        else if ((pEntry->getOutgoingBufferRemainingSpace() > 0) && (ui16DataSize > 0)) {
            if (!((NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui32SeqNum) &&
                NOMADSUtil::SequentialArithmetic::lessThan (pTCPHeader->ui32SeqNum, (pEntry->ui32NextExpectedInSeqNum + pEntry->getCurrentTCPWindowSize()))) ||
                (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pEntry->ui32NextExpectedInSeqNum, (pTCPHeader->ui32SeqNum + ui16DataSize - 1)) &&
                NOMADSUtil::SequentialArithmetic::lessThan ((pTCPHeader->ui32SeqNum + ui16DataSize - 1), (pEntry->ui32NextExpectedInSeqNum + pEntry->getCurrentTCPWindowSize()))))) {
                if (!(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST)) {
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK in answer to a packet that couldn't be saved in window buffer; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -16;
                    }
                    else {
                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                        pEntry->i64LocalActionTime = packetTime;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received a packet with with SEQ %u (relative %u), flags %hhu and %hu bytes of data "
                                        "that did not belong to the buffer window (expecting packet with SEQ %u); sent back an ACK\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                        NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                        pTCPHeader->ui8Flags, ui16DataSize, pEntry->ui32NextExpectedInSeqNum);
                    }
                }
                // Invalid RST received or sent ACK with correct values --> drop packet and return
                return 0;
            }
        }

        if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_RST) {
            // Received an acceptable RST packet
            if ((pEntry->localState == TCTLS_SYN_RCVD) || (pEntry->localState == TCTLS_ESTABLISHED) ||
                (pEntry->localState == TCTLS_FIN_WAIT_1) || (pEntry->localState == TCTLS_FIN_WAIT_2) ||
                (pEntry->localState == TCTLS_CLOSE_WAIT)) {
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                "L%hu-R%hu: received an RST while in state %d; sending a ResetTCPConnection ProxyMessage "
                                "to the remote NetProxy and clearing the related entry in the TCPConnTable\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState);
                sendRemoteResetRequestIfNeeded (pEntry);
            }
            else {
                if (((pEntry->localState == TCTLS_CLOSING) || (pEntry->localState == TCTLS_TIME_WAIT)) &&
                    (pEntry->remoteState == TCTRS_DisconnRequestReceived)) {
                    if (pEntry->getConnection() == nullptr) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                        "L%hu-R%hu: getConnection() returned a NULL pointer; impossible to "
                                        "send a CloseTCPConnection ProxyMessage to the remote NetProxy\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        pEntry->reset();
                        return -17;
                    }
                    else if (0 != (rc = pEntry->getConnection()->sendCloseTCPConnectionRequest (pEntry))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                        "L%hu-R%hu: sendCloseTCPConnectionRequest() failed with rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        pEntry->reset();
                        return -18;
                    }
                }
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: received an RST while in state %d; connection closed normally\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState);
            }

            pEntry->clear();
            return 0;
        }

        if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_SYN) {
            // Receiving a SYN here is an error --> sending back an RST to the local application
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: received a SYN packet with SEQ num %u while in state %d; sending back "
                            "an RST to the local applications and a RemoteResetRequest to the remote NetProxy\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum, static_cast<int> (pEntry->localState));

            uint8 ui8RSTFlags = NOMADSUtil::TCPHeader::TCPF_RST;
            uint32 ui32RSTSeqNum = 0;
            if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK) {
                ui32RSTSeqNum = pTCPHeader->ui32AckNum;
            }
            else {
                ui8RSTFlags |= NOMADSUtil::TCPHeader::TCPF_ACK;
                pEntry->ui32NextExpectedInSeqNum = pTCPHeader->ui32SeqNum + ui16DataSize + 1;      // Received a SYN packet --> increment the ACK NUM by 1
            }
            if (0 != (rc = sendTCPPacketToHost (pEntry, ui8RSTFlags, ui32RSTSeqNum))) {
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send ACK to an unexpected packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
                return -19;
            }
            sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();

            return 0;
        }

        if (!(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_ACK)) {
            // If there is no ACK flag, processing is over --> unlock() table and return
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: received a packet without ACK flag set; flags are %hhu; ignoring\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui8Flags);
            return 0;
        }

        // ACK Flag is on --> continue processing
        if (pEntry->localState == TCTLS_SYN_RCVD) {
            if (pTCPHeader->ui32AckNum == pEntry->ui32OutSeqNum) {
                // Received an ACK while in state SYN_RCVD --> moving to ESTABLISHED and continue processing
                pEntry->localState = TCTLS_ESTABLISHED;
                pEntry->ui32LastAckSeqNum = pTCPHeader->ui32AckNum;
                pEntry->i64LocalActionTime = packetTime;
                pEntry->i64LastAckTime = packetTime;
                // Check if we need to complete the connection with the remote NetProxy, which might happen if the local state moved from SYN_SENT to SYN_RCVD
                if (pEntry->remoteState == TCTRS_ConnRequestReceived) {
                    // Sending a TCPConnectionOpened Response to the remote NetProxy
                    if (pEntry->getConnection() && pEntry->getConnection()->isConnected()) {
                        if (pEntry->getConnection()->isEnqueueingAllowed()) {
                            bool bReachable =
                                _rConnectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (pEntry->ui32RemoteProxyUniqueID,
                                                                                                        pEntry->iaLocalInterfaceAddr.getIPAddress(),
                                                                                                        pEntry->iaRemoteProxyAddr.getIPAddress());
                            if (0 != (rc = pEntry->getConnection()->sendTCPConnectionOpenedResponse (pEntry, bReachable))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: could not send a TCPConnectionOpened ProxyMessage to the remote NetProxy at address "
                                                "<%s:%hu>; rc = %d; sending an RST to the local application and resetting the Entry\n", pEntry->ui16ID,
                                                pEntry->ui16RemoteID, pEntry->getConnection()->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                                pEntry->getConnection()->getRemoteInterfaceLocalInetAddr()->getPort(), rc);
                                if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                                    "L%hu-R%hu: failed to send an RST packet to local host; rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }

                                pEntry->reset();
                                return -20;
                            }
                        }
                        else {
                            // Enqueuing not allowed
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_NetDetailDebug,
                                            "L%hu-R%hu: impossible to enqueue a TCPConnectionOpened ProxyMessage with the Connection object\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }
                    else if (pEntry->getConnection() && pEntry->getConnection()->isConnecting()) {
                        // Local connection state is TCTLS_ESTABLISHED, remote connection state is TCTRS_ConnRequestReceived
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: received a SYN+ACK packet when the remote connection is in state %d; waiting "
                                        "to establish a connection via %s with the remote NetProxy at address <%s:%hu>\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getConnection()->getStatus(),
                                        pEntry->getConnection()->getConnectorTypeAsString(), pEntry->iaRemoteProxyAddr.getIPAsString(),
                                        pEntry->iaRemoteProxyAddr.getPort());
                    }
                    else {
                        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                            "L%hu-R%hu: failed to send an RST packet to local host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }

                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received an ACK while in SYN_RCVD state, but it was impossible to send a TCPConnectionOpened "
                                        "ProxyMessage to the remote NetProxy at address <%s:%hu>; sent back an RST to local application\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort());
                        pEntry->reset();
                        return -21;
                    }

                    const NOMADSUtil::InetAddr iaSourceIP{pEntry->ui32LocalIP};
                    const NOMADSUtil::InetAddr iaDestinationIP{pEntry->ui32RemoteIP};
                    pEntry->remoteState = TCTRS_ConnEstablished;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                    "L%hu-R%hu: successfully sent a TCPConnectionOpened ProxyMessage for the TCP connection from <%s:%hu> to <%s:%hu>, "
                                    "remapped over to the NetProxy with UniqueID %u at address <%s:%hu>; both the local and the remote connection are "
                                    "now in state ESTABLISHED (the local state went from SYN_SENT, to SYN_RCVD, to ESTABLISHED); data will be coded to "
                                    "%s:%hhu and decoded from %s:%hhu\n", pEntry->ui16ID, pEntry->ui16RemoteID, iaSourceIP.getIPAsString(),
                                    pEntry->ui16LocalPort, iaDestinationIP.getIPAsString(), pEntry->ui16RemotePort, pEntry->ui32RemoteProxyUniqueID,
                                    pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort(),
                                    pEntry->getConnectorWriter()->getCompressionName(), pEntry->getConnectorWriter()->getCompressionLevel(),
                                    pEntry->getConnectorReader()->getCompressionName(), pEntry->getConnectorReader()->getCompressionLevel());
                }
            }
            else {
                // Received an out of order ACK num while in state SYN_RCVD --> sending back an RST
                if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pTCPHeader->ui32AckNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R0: failed to send an RST packet to local host; rc = %d\n",
                                    pEntry->ui16ID, rc);
                    sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    return -22;
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                    "L%hu-R0: received an ACK with a wrong number while in SYN_RCVD state; sent an RST\n",
                                    pEntry->ui16ID);
                }
                sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
                return 0;
            }
        }

        // The following is not an else if to accommodate the case in which the local state moved from SYN_RCVD to ESTABLISHED
        if ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_FIN_WAIT_1) ||
            (pEntry->localState == TCTLS_FIN_WAIT_2) || (pEntry->localState == TCTLS_CLOSE_WAIT) ||
            (pEntry->localState == TCTLS_CLOSING) || (pEntry->localState == TCTLS_LAST_ACK) ||
            (pEntry->localState == TCTLS_TIME_WAIT)) {
            // Check ACK validity
            if (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pEntry->ui32LastAckSeqNum, pTCPHeader->ui32AckNum) &&
                NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pTCPHeader->ui32AckNum, pEntry->ui32OutSeqNum)) {
                // Acceptable ACK
                pEntry->i64LocalActionTime = packetTime;
                pEntry->i64LastAckTime = packetTime;
                if ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_CLOSE_WAIT)) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received an ACK with number %u (relative %u); first unacknowledged packet in the output buffer had SEQ number %u; "
                                    "next sent packet will have SEQ number %u (relative %u); receiver window size has been updated from %hu to %hu\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32AckNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32AckNum, pEntry->ui32StartingOutSeqNum), pEntry->ui32LastAckSeqNum,
                                    pEntry->ui32OutSeqNum, NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32OutSeqNum, pEntry->ui32StartingOutSeqNum),
                                    pEntry->ui16ReceiverWindowSize, pTCPHeader->ui16Window);
                }
                pEntry->updateOutgoingWindow (pTCPHeader);                                          // Updates variables of pEntry with the data in the received packet
                ui32ACKedPackets = pEntry->ackOutgoingDataUpto (pTCPHeader->ui32AckNum);            // Removes ACKed packets from memory

                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                "L%hu-R%hu: %u packets have been ACKnowledged; there are %u packets left in the local host outgoing packets "
                                "buffer and there are %hu free bytes in the receiver window buffer\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                ui32ACKedPackets, pEntry->dqLocalHostOutBuf.size(), pTCPHeader->ui16Window);

                if (pTCPHeader->ui32AckNum == pEntry->ui32OutSeqNum) {
                    // Received ACK is up-to-date: check if local state needs to be changed
                    if (pEntry->localState == TCTLS_FIN_WAIT_1) {
                        pEntry->localState = TCTLS_FIN_WAIT_2;
                        pEntry->dqLocalHostOutBuf.pop_front();
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: received an ACK while in FIN_WAIT_1 - moved to FIN_WAIT_2\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                    }
                    else if (pEntry->localState == TCTLS_FIN_WAIT_2) {
                        if (pEntry->dqLocalHostOutBuf.size() == 0) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: received an updated ACK while in state FIN_WAIT_2 and with an empty retransmission queue: all sent packets have been ACKed\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }
                    else if (pEntry->localState == TCTLS_CLOSING) {
                        pEntry->localState = TCTLS_TIME_WAIT;
                        pEntry->dqLocalHostOutBuf.pop_front();
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                        "L%hu-R%hu: received an updated ACK while in state CLOSING; moved to TIME_WAIT\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                    }
                    else if (pEntry->localState == TCTLS_LAST_ACK) {
                        pEntry->dqLocalHostOutBuf.pop_front();
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                        "L%hu-R%hu: received an ACK while in state LAST_ACK; moving to CLOSED\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        pEntry->localState = TCTLS_CLOSED;
                    }
                    if (pEntry->localState == TCTLS_TIME_WAIT) {
                        if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_FIN) {
                            // Received a duplicate FIN+ACK --> send back an ACK
                            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send ACK to a duplicate FIN+ACK while in state TIME_WAIT; "
                                                "rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                return -23;
                            }
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: received a duplicate ACK while in state TIME_WAIT; sent back an ACK\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }
                }
            }
            else if (NOMADSUtil::SequentialArithmetic::lessThan (pTCPHeader->ui32AckNum, pEntry->ui32LastAckSeqNum)) {
                // Duplicate ACK --> just drop it
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: received an ACK with number %u while in state %d, but first unacknowledged "
                                "packet in buffer has SEQ number %u; dropping packet\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                pTCPHeader->ui32AckNum, pEntry->localState, pEntry->ui32LastAckSeqNum);
                return 0;
            }
            else {
                // Received an ACK that is greater than the sent SEQ number --> sending back an ACK with right values and returning
                pEntry->i64LocalActionTime = packetTime;
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                "L%hu-R%hu: received an ACK with number %u while in state %d; expecting ACK with number %u; "
                                "sending back an ACK with the expected value\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                pTCPHeader->ui32AckNum, pEntry->localState, pEntry->ui32OutSeqNum);
                if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send ACK to a packet with an ACK number greater than expected; "
                                    "rc = %d; resetting remote connection\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    return -24;
                }

                return 0;
            }
        }

        if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_URG) {
            // URG flag not yet handled
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: received a packet with the URGent flag set; FUNCTION NOT YET IMPLEMENTED\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
        }

        // Check if there is any data in this packet (segment text)
        if (ui16DataSize > 0) {
            if ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_FIN_WAIT_1) ||
                (pEntry->localState == TCTLS_FIN_WAIT_2)) {
                int enqueuedDataSize = 0;
                if (pTCPHeader->ui32SeqNum == pEntry->ui32NextExpectedInSeqNum) {
                    // TO-DO:   check what happens if this packets fills a hole and the FIN had already been enqueued (we should ACK the FIN, too!)
                    //          As of now, the RTT (retransmissions) takes care of this. The present solution could increase latency.
                    // Received data with correct SEQ number --> correct ordered data
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                    "L%hu-R%hu: received a TCP packet with SEQ number %u (relative %u), %hu bytes of data and FLAGs %hhu\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    ui16DataSize, pTCPHeader->ui8Flags);
                }
                else {
                    // Received data with a higher SEQ number than expected --> out-of-order data
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                    "L%hu-R%hu: received a TCP packet with %hu bytes of data in the wrong order and FLAGs %hhu - enqueuing; "
                                    "SEQ number is %u (relative %u) and ACK num is %u; expected seq num is: %u (relative %u)\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, ui16DataSize, pTCPHeader->ui8Flags, pTCPHeader->ui32SeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum), pTCPHeader->ui32AckNum,
                                    pEntry->ui32NextExpectedInSeqNum, NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                }

                bool wereThereHolesInBuffer = pEntry->areThereHolesInTheIncomingDataBuffer();      // Check if there are any holes in the incoming data buffer before inserting the new packet
                if ((enqueuedDataSize = pEntry->insertTCPSegmentIntoOutgoingBuffer (new TCPSegment (pTCPHeader->ui32SeqNum, ui16DataSize,
                                                                                                    (unsigned char*) pTCPHeader + ui16TCPHeaderLen,
                                                                                                    (pTCPHeader->ui8Flags & TCP_DATA_FLAG_FILTER)))) < 0) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                    "L%hu-R%hu: failed to enqueue segment with SEQ number %u (relative %u), ACK num %u and %hu bytes of data; "
                                    "next expected SEQ num is: %u (relative %u); rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                    pTCPHeader->ui32SeqNum, NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    pTCPHeader->ui32AckNum, ui16DataSize, pEntry->ui32NextExpectedInSeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum), enqueuedDataSize);
                }
                else {
                    if (enqueuedDataSize == ui16DataSize) {
                        // Correctly enqueued all data
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "L%hu-R%hu: successfully enqueued %d bytes in the buffer\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, enqueuedDataSize);
                    }
                    else {
                        // Correctly enqueued necessary data only
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: enqueued only %d necessary bytes over %hu bytes received\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, enqueuedDataSize, ui16DataSize);
                    }

                    _rStatisticsManager.increaseInboundTraffic (pEntry->getConnection()->getConnectorType(), pEntry->ui32RemoteProxyUniqueID,
                                                                pEntry->iaRemoteProxyAddr.getIPAddress(), pEntry->iaRemoteProxyAddr.getPort(),
                                                                pEntry->getConnection()->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                pEntry->getConnection()->getLocalInterfaceInetAddr()->getPort(), PT_TCP, ui16DataSize);

                    /* Sending ACK only if the FIN flag is not set and either the PSH flag was set, the previous packet wasn't ACKed, or more
                     * than 536 bytes were received --> this allows the TCP protocol to reduce the amount of ACKs generated to about 1/2. */
                    if ((enqueuedDataSize >= 0) && !(pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_FIN) &&
                        (NOMADSUtil::SequentialArithmetic::greaterThan (pTCPHeader->ui32SeqNum, pEntry->ui32LastACKedSeqNum) ||
                        (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_PSH) || wereThereHolesInBuffer ||
                         (enqueuedDataSize > NetworkConfigurationSettings::MAX_TCP_UNACKED_DATA))) {
                        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                            "L%hu-R%hu: failed to send ACK packet to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            return -25;
                        }
                        else {
                            // ACK sent --> last ACKed SEQ number has to be updated
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: sent ACK to local host; last ACKed SEQ number and next expected SEQ number are %u (relative %u)\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32LastACKedSeqNum,
                                            NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32LastACKedSeqNum, pEntry->ui32StartingInSeqNum));
                        }
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "L%hu-R%hu: ACK to local host skipped; last ACKed SEQ num is %u (relative %u), next expected SEQ num "
                                        "is %u (relative %u)\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32LastACKedSeqNum,
                                        NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32LastACKedSeqNum, pEntry->ui32StartingInSeqNum),
                                        pEntry->ui32NextExpectedInSeqNum,
                                        NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                    }
                }
            }
            else if ((pEntry->localState == TCTLS_CLOSE_WAIT) || (pEntry->localState == TCTLS_CLOSING) ||
                    (pEntry->localState == TCTLS_LAST_ACK) || (pEntry->localState == TCTLS_TIME_WAIT)) {
                    // Received a packet with data when local host has already closed his side of the connection --> discarding the packet
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with %hu bytes of data while already in state %d; ignored\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, ui16DataSize, pEntry->localState);
            }
        }

        // Handling FIN packet
        if (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_FIN) {
            if ((pEntry->localState == TCTLS_SYN_SENT) || (pEntry->localState == TCTLS_CLOSED)) {
                // Segment cannot be processed --> dropping it
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: received a FIN packet while in local state SYN_SENT or CLOSED (state is %d); ignoring\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState);
                return 0;
            }
            else if ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_FIN_WAIT_1) || (pEntry->localState == TCTLS_FIN_WAIT_2)) {
                // Ready to receive and acknowledge FIN packet
                if ((pTCPHeader->ui32SeqNum + ui16DataSize) == pEntry->ui32NextExpectedInSeqNum) {
                    // All previous packets have been received <--> SEQ NUM is consistent with the expected one
                    // Check if we can send a CloseTCPConnection packet right away
                    if (!pEntry->isOutgoingBufferEmpty()) {
                        // Outgoing buffer is not empty --> enqueue a TCP packet with FIN+ACK flags for later processing
                        if ((rc = pEntry->insertTCPSegmentIntoOutgoingBuffer (new TCPSegment (pTCPHeader->ui32SeqNum + ui16DataSize, 0, nullptr,
                                                                                              NOMADSUtil::TCPHeader::TCPF_FIN | NOMADSUtil::TCPHeader::TCPF_ACK))) < 0) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                            "L%hu-R%hu: failed to enqueue FIN packet received from host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            return -26;
                        }
                    }
                    else {
                        // Outgoing buffer is empty --> send a CloseTCPConnection ProxyMessage and advance the remote state
                        if (0 != (rc = flushAndSendCloseConnectionRequest (pEntry))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: failed to flush and send a CloseConnection response to remote proxy; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            return -27;
                        }

                        // Advance the remote state
                        if (pEntry->remoteState == TCTRS_ConnRequestSent) {
                            pEntry->remoteState = TCTRS_DisconnRequestSent;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                            "L%hu-R%hu: sent a CloseTCPConnection request to the remote NetProxy; "
                                            "remote state moved from ConnRequestSent to DisconnRequestedSent\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                        else if (pEntry->remoteState == TCTRS_ConnEstablished) {
                            pEntry->remoteState = TCTRS_DisconnRequestSent;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                            "L%hu-R%hu: sent a CloseTCPConnection request to the remote NetProxy; "
                                            "remote state moved from ConnEstablished to DisconnRequestedSent\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                        else if (pEntry->remoteState == TCTRS_DisconnRequestReceived) {
                            pEntry->remoteState = TCTRS_Disconnected;
                            pEntry->resetConnectors();
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                            "L%hu-R%hu: sent a CloseTCPConnection request to the remote NetProxy; "
                                            "remote state moved from DisconnRequestReceived to Disconnected\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                        else {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                                            "L%hu-R%hu: local connection state (%d) and remote connection state (%d) are out of sync; "
                                            "impossible to handle received FIN packet correctly; resetting the TCP connection\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState, pEntry->remoteState);
                            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_MildError,
                                                "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            return -28;
                        }
                        pEntry->i64RemoteActionTime = packetTime;
                    }


                    // ACK the FIN and advance the local state
                    pEntry->ui32NextExpectedInSeqNum++;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a FIN packet with SEQ number %u (relative %u) and %hu bytes of data in the correct order "
                                    "(present state is %d); a packet with ACK number %u (relative %u) will be sent back to local host\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    ui16DataSize, pEntry->localState, pEntry->ui32NextExpectedInSeqNum,
                                    NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK packet to answer a received FIN packet; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -29;
                    }
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;

                    if ((pEntry->localState == TCTLS_SYN_RCVD) || (pEntry->localState == TCTLS_ESTABLISHED)) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received FIN+ACK while in state %d, moved to CLOSE_WAIT and sent ACK %u (relative %u) with "
                                        "sequence number %u\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState, pEntry->ui32NextExpectedInSeqNum,
                                        NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                        pEntry->ui32OutSeqNum);
                        pEntry->localState = TCTLS_CLOSE_WAIT;
                    }
                    else if (pEntry->localState == TCTLS_FIN_WAIT_1) {
                        /* It cannot be that (pTCPHeader->ui32AckNum == pEntry->ui32OutSeqNum), or the state would have been moved to FIN_WAIT_2 above.
                         * NetProxy will proceed with the Simultaneous Close Sequence, which moves the state to CLOSING. */
                        pEntry->localState = TCTLS_CLOSING;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received a FIN+ACK while in FIN_WAIT_1 - moved to CLOSING (Simultaneous Close Sequence)\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                    }
                    else if (pEntry->localState == TCTLS_FIN_WAIT_2) {
                        // Moving to TIME_WAIT
                        pEntry->localState = TCTLS_TIME_WAIT;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Info,
                                        "L%hu-R%hu: received FIN+ACK while in state FIN_WAIT_2, moved to TIME_WAIT and sent ACK %u (relative %u) "
                                        "with sequence number %u\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                        NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                        pEntry->ui32OutSeqNum);
                    }
                }
                else {
                    // Enqueueing received out-of-order FIN packet for later processing
                    if ((rc = pEntry->insertTCPSegmentIntoOutgoingBuffer (new TCPSegment (pTCPHeader->ui32SeqNum + ui16DataSize, 0, nullptr,
                                                                                          NOMADSUtil::TCPHeader::TCPF_FIN | NOMADSUtil::TCPHeader::TCPF_ACK))) < 0) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R%hu: failed to enqueue an out-of-order FIN packet received from local host; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -30;
                    }

                    // Sending a DUP ACK to trigger fast retransmission
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R%hu: failed to ACK a FIN packet containing %hu bytes of data; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, ui16DataSize, rc);
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        return -31;
                    }

                    if (ui16DataSize > 0) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: received an out-of-order FIN packet (local state is still %d, remote state is %d); "
                                        "%hu bytes of data with the FIN flag have been enqueued for later processing\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState,
                                        pEntry->remoteState, ui16DataSize);
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: received an out-of-order FIN packet (local state is still %d, remote state is %d) carrying no data; "
                                        "FIN has been enqueued for later processing\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                        pEntry->localState, pEntry->remoteState);
                    }
                }
            }
            else if ((pEntry->localState == TCTLS_CLOSE_WAIT) || (pEntry->localState == TCTLS_LAST_ACK) ||
                (pEntry->localState == TCTLS_CLOSING) || (pEntry->localState == TCTLS_TIME_WAIT)) {
                // Duplicate FIN+ACK packet
                if (pEntry->localState == TCTLS_TIME_WAIT) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a duplicate FIN+ACK (already in state TCTLS_TIME_WAIT); ignoring\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a duplicate FIN+ACK (already in state %d); sending back an updated ACK\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState);
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                                        "L%hu-R%hu: failed to send ACK packet to answer a duplicate FIN+ACK packet; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->clear();
                        return -32;
                    }
                }
            }
        }

        bool isRTTDataReady = pEntry->areDataAvailableInTheIncomingBuffer() && ((pEntry->remoteState == TCTRS_ConnEstablished) || (pEntry->remoteState == TCTRS_DisconnRequestReceived));
        bool isLTTDataReady = (pEntry->dqLocalHostOutBuf.size() > 0) && ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_CLOSE_WAIT));
        bool rttShouldWork = isRTTDataReady &&          // There is data ready in the buffer and...
                            // ... EITHER some data and a PSH flag have been received...
                            (((ui16DataSize > 0) && (pTCPHeader->ui8Flags & NOMADSUtil::TCPHeader::TCPF_PSH)) ||
                            // ... OR we have just received a FIN+ACK from localhost
                            ((pTCPHeader->ui8Flags & (NOMADSUtil::TCPHeader::TCPF_FIN | NOMADSUtil::TCPHeader::TCPF_ACK)) == (NOMADSUtil::TCPHeader::TCPF_FIN | NOMADSUtil::TCPHeader::TCPF_ACK)));
        bool lttShouldWork = (pEntry->ui16ReceiverWindowSize > oldWinRecSize) && isLTTDataReady;
        ulEntry.unlock();

        if (rttShouldWork || (!lttShouldWork && isRTTDataReady)) {
            rRemoteTCPTransmitterThread.wakeUpThread();
        }
        else if (isLTTDataReady) {
            rLocalTCPTransmitterThread.wakeUpThread();
        }

        return 0;
    }

    int TCPManager::sendTCPPacketToHost (Entry * const pEntry, uint8 ui8TCPFlags, uint32 ui32SeqNum, const uint8 * const pui8Payload, uint16 ui16PayloadLen)
    {
        static const uint16 ui16TCPHeaderLen = sizeof(NOMADSUtil::TCPHeader);

        int rc;
        auto * pui8Packet = reinterpret_cast<uint8 *> (_rPBM.getAndLockWriteBuf());
        const uint16 ui16PacketLen = sizeof(NOMADSUtil::IPHeader) + ui16TCPHeaderLen + ui16PayloadLen;
        if (ui16PacketLen > NetworkConfigurationSettings::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("TCPManager::sendTCPPacketToHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: packet length with data (%hu - ack %u) exceeds maximum packet size (%hu)\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, ui16PacketLen,
                            pEntry->ui32NextExpectedInSeqNum, NetworkConfigurationSettings::PROXY_MESSAGE_MTU);
            if (0 != _rPBM.findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("TCPManager::sendTCPPacketToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }

        auto * pIPHeader = reinterpret_cast<NOMADSUtil::IPHeader *> (pui8Packet + sizeof(NOMADSUtil::EtherFrameHeader));
        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof(NOMADSUtil::IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16TLen = sizeof(NOMADSUtil::IPHeader) + ui16TCPHeaderLen + ui16PayloadLen;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui16FlagsAndFragOff = 0;
        pIPHeader->ui8TTL = 8;
        pIPHeader->ui8Proto = NOMADSUtil::IP_PROTO_TCP;
        pIPHeader->srcAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (pEntry->ui32RemoteIP);
        pIPHeader->destAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (pEntry->ui32LocalIP);
        pIPHeader->computeChecksum();

        const uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        NOMADSUtil::TCPHeader * pTCPHeader = reinterpret_cast<NOMADSUtil::TCPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
        pTCPHeader->ui16SPort = pEntry->ui16RemotePort;
        pTCPHeader->ui16DPort = pEntry->ui16LocalPort;
        pTCPHeader->ui32SeqNum = ui32SeqNum;
        pTCPHeader->ui32AckNum = (ui8TCPFlags & NOMADSUtil::TCPHeader::TCPF_ACK) ? pEntry->ui32NextExpectedInSeqNum : 0;
        pTCPHeader->ui8Offset = ((ui16TCPHeaderLen / 4) << 4);
        pTCPHeader->ui8Flags = ui8TCPFlags;
        pTCPHeader->ui16Window = pEntry->getOutgoingBufferRemainingSpace();
        pTCPHeader->ui16Urgent = 0;
        memcpy (reinterpret_cast<uint8 *> (pTCPHeader) + ui16TCPHeaderLen, pui8Payload, ui16PayloadLen);
        NOMADSUtil::TCPHeader::computeChecksum (reinterpret_cast<uint8 *> (pIPHeader));
        checkAndLogMsg ("TCPManager::sendTCPPacketToHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "L%hu-R%hu: sending a packet with %hu bytes of data, flags %hhu, SEQ number %u (relative %u) and ACK number %u "
                        "(relative %u) to %s:%hu\n", pEntry->ui16ID, pEntry->ui16RemoteID, ui16PayloadLen, ui8TCPFlags, ui32SeqNum,
                        NOMADSUtil::SequentialArithmetic::delta (ui32SeqNum, pEntry->ui32StartingOutSeqNum), pEntry->ui32NextExpectedInSeqNum,
                        NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                        NOMADSUtil::InetAddr(pEntry->ui32LocalIP).getIPAsString(), pTCPHeader->ui16DPort);

        if (0 != (rc = _rPacketRouter.wrapEthernetIPFrameAndSendToHost (_rPacketRouter.getInternalNetworkInterface().get(), pui8Packet,
                                                                        sizeof(NOMADSUtil::EtherFrameHeader) + ui16PacketLen))) {
            checkAndLogMsg ("TCPManager::sendTCPPacketToHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: wrapEthernetFrameAndSendToHost() failed with rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            if (0 != _rPBM.findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("TCPManager::sendTCPPacketToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }

        if (0 != _rPBM.findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("TCPManager::sendTCPPacketToHost", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -3;
        }

        return 0;
    }

    int TCPManager::openTCPConnectionToHost (uint32 ui32RemoteProxyIP, uint32 ui32RemoteProxyUniqueID, uint16 ui16RemoteID, uint32 ui32LocalHostIP,
                                             uint16 ui16LocalPort, uint32 ui32RemoteHostIP, uint16 ui16RemotePort, uint8 ui8CompressionTypeAndLevel)
    {
        int rc;
        // First check if we can find the proxy address for the remote IP
        const auto * pProtocolSetting = _rConfigurationManager.mapAddrToProtocol (ui32LocalHostIP, ui16LocalPort, ui32RemoteHostIP,
                                                                                  ui16RemotePort, NOMADSUtil::IP_PROTO_TCP);
        if (!pProtocolSetting) {
            pProtocolSetting = ProtocolSetting::getDefaultTCPProtocolSetting();
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_Warning,
                            "L0-R%hu: received an OpenTCPConnection ProxyMessage to open a TCP connection from <%s:%hu> to <%s:%hu>; "
                            "NetProxy could not retrieve the associated ProtocolSettings and will use the default protocol %s\n",
                            ui16RemoteID, NOMADSUtil::InetAddr{ui32RemoteHostIP}.getIPAsString(), ui16RemotePort,
                            NOMADSUtil::InetAddr{ui32LocalHostIP}.getIPAsString(), ui16LocalPort,
                            pProtocolSetting->getProxyMessageProtocolAsString());
        }

        const auto connectorType = pProtocolSetting->getConnectorTypeFromProtocol();
        const auto encryptionType = pProtocolSetting->getEncryptionType();
        QueryResult query{_rConnectionManager.queryConnectionToRemoteHostForConnectorType (ui32LocalHostIP, ui16LocalPort, ui32RemoteHostIP,
                                                                                           ui16RemotePort, connectorType, encryptionType)};
        const auto iaRemoteProxyAddr = query.getBestConnectionAddressSolution();
        if (iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_MildError,
                            "L0-R%hu: impossible to find an entry in the AddressMappingTable that matches the source "
                            "and destination address pair <%s:%hu>-<%s:%hu> for the %s Connector and %s encryption\n",
                            ui16RemoteID, NOMADSUtil::InetAddr{ui32RemoteHostIP}.getIPAsString(), ui16RemotePort,
                            NOMADSUtil::InetAddr{ui32LocalHostIP}.getIPAsString(), ui16LocalPort,
                            connectorTypeToString (connectorType), encryptionTypeToString (encryptionType));
            return -1;
        }

        /* Need to acquire both the lock on the TCPConnTable and the Entry atomically to avoid a potential deadlock with the Connection::prepareForDelete()
         * method, which acquires the lock on the TCPConnTable first and then tries to acquire the lock on all Entries, one by one */
        auto * const pEntry = _rTCPConnTable.getEntry (ui32LocalHostIP, ui16LocalPort, ui32RemoteHostIP, ui16RemotePort);
        std::lock (_rTCPConnTable.getMutexRef(), pEntry->getMutexRef());
        std::unique_lock<std::mutex> ulTCPConnTable{_rTCPConnTable.getMutexRef(), std::adopt_lock};
        std::unique_lock<std::mutex> ulEntry{pEntry->getMutexRef(), std::adopt_lock};

        pEntry->prepareNewConnection();
        pEntry->ui32RemoteProxyUniqueID = ui32RemoteProxyUniqueID;
        pEntry->iaLocalInterfaceAddr = query.getLocalProxyInterfaceAddress();
        pEntry->iaRemoteProxyAddr = iaRemoteProxyAddr;
        pEntry->remoteState = TCTRS_ConnRequestReceived;
        pEntry->ui16RemoteID = ui16RemoteID;
        pEntry->i64RemoteActionTime = NOMADSUtil::getTimeInMilliseconds();

        pEntry->setProtocol (pProtocolSetting->getProxyMessageProtocol());
        pEntry->assignedPriority = pProtocolSetting->getPriorityLevel();

        auto * pConnection = query.getActiveConnectionToRemoteProxy();
        // openNewConnectionToRemoteProxy() returns nullptr unless the new connection has been established
        pConnection = (pConnection && (pConnection->isConnected() || pConnection->isConnecting())) ?
            pConnection : Connection::openNewConnectionToRemoteProxy (_rConnectionManager, _rTCPConnTable, *this, _rPacketRouter,
                                                                      _rStatisticsManager, query, false);
        // Set connection even if not yet connected
        pConnection = pConnection ? pConnection :
            Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                pEntry->iaRemoteProxyAddr, connectorType, pProtocolSetting->getEncryptionType());
        if (!pConnection || (pConnection->hasFailedConnection())) {
            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage, but it was impossible to create a new %sConnection to "
                            "connect the remote NetProxy at address %s in order to remap a new TCP Connection from <%s:%hu> to <%s:%hu>\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, connectorTypeToString (connectorType), iaRemoteProxyAddr.getIPAsString(),
                            NOMADSUtil::InetAddr{ui32RemoteHostIP}.getIPAsString(), ui16RemotePort,
                            NOMADSUtil::InetAddr{ui32LocalHostIP}.getIPAsString(), ui16LocalPort);
            pEntry->reset();
            return -2;
        }
        pEntry->setConnection (pConnection);
        checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_Info,
                        "L%hu-R%hu: the new TCP Connection opened from the remote application with address %s:%hu to the local application with address "
                        "%s:%hu will be remapped over the %s connection with local endpoint <%s:%hu> and remote endpoint <%s:%hu> (remote NetProxy with "
                        "UniqueID %u)\n", pEntry->ui16ID, pEntry->ui16RemoteID, NOMADSUtil::InetAddr{ui32RemoteHostIP}.getIPAsString(), ui16RemotePort,
                        NOMADSUtil::InetAddr{ui32LocalHostIP}.getIPAsString(), ui16LocalPort, pConnection->getConnectorTypeAsString(),
                        pConnection->getLocalInterfaceInetAddr()->getIPAsString(), pConnection->getLocalInterfaceInetAddr()->getPort(),
                        pConnection->getRemoteInterfaceLocalInetAddr()->getIPAsString(), pConnection->getRemoteInterfaceLocalInetAddr()->getPort(),
                        pConnection->getRemoteNetProxyID());

        if (pConnection->isConnecting()) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage; waiting to establish a connection via %s "
                            "to the remote NetProxy with address <%s:%hu>\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                            pEntry->getConnection()->getConnectorTypeAsString(), pEntry->iaRemoteProxyAddr.getIPAsString(),
                            pEntry->iaRemoteProxyAddr.getPort());
        }
        else if (pConnection->isConnected()) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_HighDetailDebug,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage; a connection via %s to the remote "
                            "NetProxy with address <%s:%hu> was established\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                            pEntry->getConnection()->getConnectorTypeAsString(), pEntry->iaRemoteProxyAddr.getIPAsString(),
                            pEntry->iaRemoteProxyAddr.getPort());
        }
        else {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage; a connection via %s to the "
                            "remote NetProxy with address <%s:%hu> is in state %d; resetting TCP connection\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getConnection()->getConnectorTypeAsString(),
                            pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort(),
                            pConnection->getStatus());
            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            pEntry->reset();
            return -3;
        }

        /* Ignoring TIME_WAIT might cause slow packets to be lost, but it is probably not a problem if
         * we assume that the internal network has slow latency and packet loss; ignore it only if
         * NetworkConfigurationSettings::TCP_TIME_WAIT_IGNORE_STATE is true */
        if ((pEntry->localState != TCTLS_LISTEN) && (pEntry->localState != TCTLS_CLOSED) &&
            (!NetworkConfigurationSettings::TCP_TIME_WAIT_IGNORE_STATE || (pEntry->localState != TCTLS_TIME_WAIT))) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: connection between <%s:%hu> and <%s:%hu> is in state %d: cannot open a new connection; resetting connection\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, NOMADSUtil::InetAddr (pEntry->ui32LocalIP).getIPAsString(), pEntry->ui16LocalPort,
                            NOMADSUtil::InetAddr (pEntry->ui32RemoteIP).getIPAsString(), pEntry->ui16RemotePort, pEntry->localState);
            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send an RST packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            pEntry->reset();
            return -4;
        }

        delete pEntry->setProperConnectorReader (ConnectorReader::inizializeConnectorReader (ui8CompressionTypeAndLevel));
        delete pEntry->setProperConnectorWriter (ConnectorWriter::connectorWriterFactory (pProtocolSetting ?
                                                                                          pProtocolSetting->getCompressionSetting() :
                                                                                          CompressionSettings::DefaultNOCompressionSetting));

        // Enqueue and then send a SYN packet to the host to open the connection
        pEntry->dqLocalHostOutBuf.emplace_back (pEntry->ui32OutSeqNum, 0, nullptr, NOMADSUtil::TCPHeader::TCPF_SYN);
        checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "L%hu-R%hu: enqueued a SYN packet with SEQ number %u into the output buffer\n",
                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum);

        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_SYN, pEntry->ui32OutSeqNum))) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: failed to send SYN packet to local host; rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            pEntry->reset();
            return -5;
        }

        pEntry->localState = TCTLS_SYN_SENT;
        pEntry->i64LocalActionTime = NOMADSUtil::getTimeInMilliseconds();
        pEntry->i64LastAckTime = pEntry->i64LocalActionTime;
        pEntry->dqLocalHostOutBuf.back().setLastTransmitTime (pEntry->i64LocalActionTime);
        ++pEntry->ui32OutSeqNum;
        checkAndLogMsg ("TCPManager::openTCPConnectionToHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "L%hu-R%hu: sent a SYN packet with SEQ NUM %u to open a new TCP connection with the local application; "
                        "local state moved to SYN_SENT\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum - 1);

        return 0;
    }

    int TCPManager::confirmTCPConnectionToHostOpened (uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32RemoteProxyUniqueID, uint8 ui8CompressionTypeAndLevel)
    {
        int rc;
        auto * const pEntry = _rTCPConnTable.getEntry (ui16LocalID);
        if (pEntry == nullptr) {
            checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_Warning,
                            "no entry found in the TCPConnTable with local ID %hu; cannot "
                            "update the remote state to ConnEstablished\n", ui16LocalID);
            return -1;
        }

        std::lock_guard<std::mutex> lg{pEntry->getMutexRef()};
        if ((pEntry->remoteState != TCTRS_ConnRequestSent) && (pEntry->remoteState != TCTRS_DisconnRequestSent)) {
            checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: impossible to process the TCPConnectionOpened ProxyMessage "
                            "when the remote connection state is %d; resetting connections\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteState);
            pEntry->ui16ID = ui16LocalID;
            pEntry->ui16RemoteID = ui16RemoteID;
            delete pEntry->setProperConnectorWriter (ConnectorWriter::connectorWriterFactory (CompressionSettings::DefaultNOCompressionSetting));
            pEntry->reset();
            return -2;
        }
        pEntry->ui16RemoteID = ui16RemoteID;
        pEntry->ui32RemoteProxyUniqueID = ui32RemoteProxyUniqueID;
        delete pEntry->setProperConnectorReader (ConnectorReader::inizializeConnectorReader (ui8CompressionTypeAndLevel));
        pEntry->remoteState = (pEntry->remoteState == TCTRS_ConnRequestSent) ? TCTRS_ConnEstablished : TCTRS_DisconnRequestSent;
        pEntry->i64RemoteActionTime = NOMADSUtil::getTimeInMilliseconds();

        // If the SynchronizeTCPHandshake option is enabled, NetProxy needs to send a SYN+ACK packet now
        if (NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE) {
            if (0 != (rc = sendTCPPacketToHost (pEntry, (NOMADSUtil::TCPHeader::TCPF_SYN | NOMADSUtil::TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send SYN+ACK packet after the remote NetProxy has opened the TCP connection on its "
                                "side (SynchronizeTCPHandshake option enabled); rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                pEntry->reset();
                return -3;
            }
            pEntry->ui32OutSeqNum++;
            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
            pEntry->i64LocalActionTime = pEntry->i64RemoteActionTime;
            checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_LowDetailDebug,
                            "L%hu-R%hu: sent SYN+ACK packet with ACK %u (relative %u) and SEQ num %u (relative %u) (SynchronizeTCPHandshake "
                            "option enabled)\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                            NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                            pEntry->ui32OutSeqNum - 1, NOMADSUtil::SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1,
                                                                                                pEntry->ui32StartingOutSeqNum));
        }

        if (pEntry->remoteState == TCTRS_ConnEstablished) {
            checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_Info,
                            "L%hu-R%hu: received confirmation of an ESTABLISHED connection from <%s:%hu> to <%s:%hu> from the "
                            "remote NetProxy at address <%s:%hu>; data will be deflated to %s:%hhd and inflated from %s:%hhd\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, NOMADSUtil::InetAddr{pEntry->ui32LocalIP}.getIPAsString(),
                            pEntry->ui16LocalPort, NOMADSUtil::InetAddr{pEntry->ui32RemoteIP}.getIPAsString(),
                            pEntry->ui16RemotePort, pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort(),
                            pEntry->getConnectorWriter()->getCompressionName(), pEntry->getConnectorWriter()->getCompressionLevel(),
                            pEntry->getConnectorReader()->getCompressionName(), pEntry->getConnectorReader()->getCompressionLevel());
        }
        else {
            checkAndLogMsg ("TCPManager::confirmTCPConnectionToHostOpened", NOMADSUtil::Logger::L_Info,
                            "L%hu-R%hu: received confirmation of an ESTABLISHED connection from <%s:%hu> to <%s:%hu> from the remote NetProxy at "
                            "address <%s:%hu> when the local NetProxy has already sent a CloseTCPConnection ProxyMessage to the remote NetProxy; "
                            "the remote connection state is DisconnRequestSent; data will be deflated to %s:%hhd and inflated from %s:%hhd\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, NOMADSUtil::InetAddr{pEntry->ui32LocalIP}.getIPAsString(),
                            pEntry->ui16LocalPort, NOMADSUtil::InetAddr{pEntry->ui32RemoteIP}.getIPAsString(), pEntry->ui16RemotePort,
                            pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->iaRemoteProxyAddr.getPort(),
                            pEntry->getConnectorWriter()->getCompressionName(), pEntry->getConnectorWriter()->getCompressionLevel(),
                            pEntry->getConnectorReader()->getCompressionName(), pEntry->getConnectorReader()->getCompressionLevel());
        }

        return 0;
    }

    int TCPManager::sendTCPDataToHost (uint16 ui16LocalID, uint16 ui16RemoteID, const uint8 * const pui8CompData, uint16 ui16CompDataLen,
                                       uint8 ui8Flags, LocalTCPTransmitterThread & rLocalTCPTransmitterThread)
    {
        static const uint16 MAX_SEGMENT_LENGTH = NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui16InterfaceMTU -
            static_cast<uint16> (sizeof(NOMADSUtil::IPHeader) + sizeof(NOMADSUtil::TCPHeader));

        int rc;
        auto * const pEntry = _rTCPConnTable.getEntry (ui16LocalID, ui16RemoteID);
        if (pEntry == nullptr) {
            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_Warning,
                            "no entry found in TCP Connection Table with local ID %hu and remote ID %hu; "
                            "impossible to send data to local host\n", ui16LocalID, ui16RemoteID);
            return -1;
        }

        std::unique_lock<std::mutex> ul{pEntry->getMutexRef()};
        if (pEntry->localState == TCTLS_LISTEN) {
            // Only reset the remote connection
            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: impossible to process a TCPData ProxyMessage with %hu bytes because the "
                            "local connection state is LISTEN (while the remote state is %d); NetProxy will send a "
                            "ResetTCPConnection ProxyMessage to the remote NetProxy\n", pEntry->ui16ID,
                            pEntry->ui16RemoteID, ui16CompDataLen, static_cast<int> (pEntry->remoteState));
            return -2;
        }

        pEntry->i64RemoteActionTime = NOMADSUtil::getTimeInMilliseconds();
        if ((pEntry->remoteState != TCTRS_ConnEstablished) && (pEntry->remoteState != TCTRS_DisconnRequestSent)) {
            // Reset both local and remote connections
            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: impossible to process a TCPData ProxyMessage with %hu bytes because the remote connection "
                            "state is %d (while the local state is %d); NetProxy will send an RST to the local application and a "
                            "ResetTCPConnection ProxyMessage to the remote NetProxy\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                            ui16CompDataLen, static_cast<int> (pEntry->remoteState), static_cast<int> (pEntry->localState));

            if ((pEntry->localState == TCTLS_SYN_RCVD) && NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE) {
                // The local NetProxy never sent a packet to the local application (only a SYN was received)
                pEntry->ui32StartingOutSeqNum = 0;
                if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0))) {
                    checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R0: failed to send an RST+ACK packet to the local application; rc = %d\n",
                                    pEntry->ui16ID, rc);
                }
            }
            else {
                if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send an RST packet to the local application; rc = %d\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                }
            }
            pEntry->reset();
            return -3;
        }

        bool isPSHFlagSet = false;
        if ((pEntry->localState == TCTLS_SYN_SENT) || (pEntry->localState == TCTLS_SYN_RCVD) ||
            (pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_CLOSE_WAIT)) {
            // Correct outgoing SEQ number depends on the status of the local connection and on its buffer
            uint32 ui32PcktOutSeqNum = (pEntry->dqLocalHostOutBuf.empty() ||
                                       ((pEntry->localState == TCTLS_SYN_SENT) && (pEntry->dqLocalHostOutBuf.size() == 1))) ?
                pEntry->ui32OutSeqNum :
                (pEntry->dqLocalHostOutBuf.back().getSequenceNumber() + pEntry->dqLocalHostOutBuf.back().getItemLength());

            if (pEntry->getConnectorReader()) {
                uint8 * pui8Data[1];
                uint32 ui32DataLen;
                *pui8Data = nullptr;
                // Receives the packet (through the decompressor, if there is any)
                if (0 != (rc = pEntry->getConnectorReader()->receiveTCPDataProxyMessage (pui8CompData, ui16CompDataLen,
                                                                                         pui8Data, ui32DataLen))) {
                    checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_MildError,
                                    "L%hu-R%hu: receiveTCPDataProxyMessage() failed with rc = %d; resetting connection\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);

                    if ((pEntry->localState == TCTLS_SYN_RCVD) && NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE) {
                        // The local NetProxy never sent a packet to the local application (only a SYN was received)
                        pEntry->ui32StartingOutSeqNum = 0;
                        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0))) {
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_SevereError,
                                            "L%hu-R0: failed to send an RST+ACK packet to the local application; rc = %d\n",
                                            pEntry->ui16ID, rc);
                        }
                    }
                    else {
                        if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_SevereError,
                                            "L%hu-R%hu: failed to send an RST packet to the local application; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                    }
                    pEntry->reset();
                    return -4;
                }

                // To fix problem caused by long inactivity times
                if ((pEntry->dqLocalHostOutBuf.size() == 0) && (pEntry->getOutgoingTotalBytesCount() == 0)) {
                    pEntry->i64LocalActionTime = pEntry->i64RemoteActionTime;
                }
                if (ui32DataLen == 0) {
                    // ConnectorReader returned 0 bytes (decompression needs more data?)
                    checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: ConnectorReader returned 0 bytes to append to the outgoing "
                                    "buffer queue (decompression needs more data?); doing nothing\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                    return 0;
                }

                uint32 ui32SentData = 0;
                if (!pEntry->dqLocalHostOutBuf.empty()) {
                    // Check if we can append data to the last packet
                    auto & rrdTailPacket = pEntry->dqLocalHostOutBuf.back();
                    if (rrdTailPacket.canAppendData() && (rrdTailPacket.getItemLength() < MAX_SEGMENT_LENGTH)) {
                        uint32 ui32BytesToEnqueue = std::min (ui32DataLen, MAX_SEGMENT_LENGTH - rrdTailPacket.getItemLength());
                        if (0 < (rc = rrdTailPacket.appendDataToBuffer (*pui8Data, ui32BytesToEnqueue))) {
                            ui32SentData = ui32BytesToEnqueue;
                            if (ui32BytesToEnqueue >= ui32DataLen) {
                                rrdTailPacket.addTCPFlags (ui8Flags);
                            }
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: appended %u bytes to the segment of TCP packet with SEQ number %u; "
                                            "there are now %u bytes of total data and FLAGs are %hhu\n", pEntry->ui16ID,
                                            pEntry->ui16RemoteID, ui32BytesToEnqueue, rrdTailPacket.getSequenceNumber(),
                                            rrdTailPacket.getItemLength(), rrdTailPacket.getTCPFlags());
                        }
                        else {
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_Warning,
                                            "L%hu-R%hu: error trying to append %u bytes to the segment of TCP packet with SEQ "
                                            "NUM %u; rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, ui32BytesToEnqueue,
                                            rrdTailPacket.getSequenceNumber(), rc);
                        }
                    }
                }

                // Create new packets to store remaining received data
                while (ui32SentData < ui32DataLen) {
                    uint32 ui32BytesToEnqueue = std::min (ui32DataLen - ui32SentData, (uint32) MAX_SEGMENT_LENGTH);
                    uint8 ui8FlagsToEnqueue = ((ui32SentData + ui32BytesToEnqueue) >= ui32DataLen) ?
                        ui8Flags : NOMADSUtil::TCPHeader::TCPF_ACK;
                    pEntry->dqLocalHostOutBuf.emplace_back (ui32PcktOutSeqNum + ui32SentData, ui32BytesToEnqueue,
                                                            *pui8Data + ui32SentData, ui8FlagsToEnqueue);
                    const auto & rrdNewData = pEntry->dqLocalHostOutBuf.back();
                    ui32SentData += ui32BytesToEnqueue;
                    checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: enqueued TCP packet with SEQ number %u, %hu bytes of data (%hu "
                                    "before decompression, if any) and FLAGs %hhu into the output buffer\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, rrdNewData.getSequenceNumber(),
                                    rrdNewData.getItemLength(), ui16CompDataLen, ui8FlagsToEnqueue);
                }
                isPSHFlagSet = (ui8Flags & NOMADSUtil::TCPHeader::TCPF_PSH) != 0;
            }
            else {
                checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_MildError,
                                "L%hu-R%hu: ConnectorReader not initialized; resetting connection\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID);

                if ((pEntry->localState == TCTLS_SYN_RCVD) && NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE) {
                    // The local NetProxy never sent a packet to the local application (only a SYN was received)
                    pEntry->ui32StartingOutSeqNum = 0;
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK, 0))) {
                        checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R0: failed to send an RST+ACK packet to the local application; rc = %d\n",
                                        pEntry->ui16ID, rc);
                    }
                }
                else {
                    if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send an RST packet to the local application; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    }
                }
                pEntry->reset();
                return -5;
            }
        }
        else {
            checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: cannot enqueue received data while the local connection is in state %d; NetProxy "
                            "will reset the local TCP connection and send a ResetTCPConnection ProxyMessage\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, static_cast<int> (pEntry->localState));

            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::sendTCPDataToHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send an RST packet to local host; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            pEntry->reset();
            return -6;
        }

        if (isPSHFlagSet && ((pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_CLOSE_WAIT))) {
            ul.unlock();
            rLocalTCPTransmitterThread.wakeUpThread();
        }

        return 0;
    }

    int TCPManager::closeTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID, RemoteTCPTransmitterThread & rRemoteTCPTransmitterThread)
    {
        auto * const pEntry = _rTCPConnTable.getEntry (ui16LocalID, ui16RemoteID);
        if (pEntry == nullptr) {
            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_Warning,
                            "no entry found in TCP Connection Table with local ID %hu and remote ID %hu; "
                            "impossible to close connection to local host\n", ui16LocalID, ui16RemoteID);
            return -1;
        }

        int rc;
        std::unique_lock<std::mutex> ul{pEntry->getMutexRef()};
        int64 i64CurrTime = NOMADSUtil::getTimeInMilliseconds();
        if ((pEntry->remoteState == TCTRS_DisconnRequestReceived) || (pEntry->remoteState == TCTRS_Disconnected)) {
            // Duplicate request --> ignoring
            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: remote CloseTCPConnectionRequest had already been received or connection was reset "
                            "(local state is %d, remote state is %d); request will be ignored\n", pEntry->ui16ID,
                            pEntry->ui16RemoteID, pEntry->localState, pEntry->remoteState);
            return 0;
        }

        if (pEntry->remoteState == TCTRS_ConnEstablished) {
            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_LowDetailDebug,
                            "L%hu-R%hu: CloseConnectionRequest received when the remote state was ConnEstablished; "
                            "the remote state is now DisconnRequestReceived and the local state is %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState);
            pEntry->remoteState = TCTRS_DisconnRequestReceived;
        }
        else if (pEntry->remoteState == TCTRS_DisconnRequestSent) {
            // Remote Connection terminated
            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_LowDetailDebug,
                            "L%hu-R%hu: CloseConnectionRequest received when remote state was DisconnRequestSent; remote "
                            "state is now Disconnected, local state moved to CLOSE_WAIT and outgoing ACK number has "
                            "been incremented to ACK previously received FIN\n", pEntry->ui16ID, pEntry->ui16RemoteID);
            pEntry->remoteState = TCTRS_Disconnected;
            pEntry->resetConnectors();
        }
        else {
            // Error: connection state with the remote NetProxy is out of sync --> resetting local and remote connections
            if (0 != (rc = sendTCPPacketToHost (pEntry, NOMADSUtil::TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send an RST packet as a consequence of a CloseConnection "
                                "request received while in local state %d and remote state %d; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState, pEntry->remoteState, rc);
            }

            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: could not process CloseTCPConnection request; the local state is %d "
                            "and the remote state is %d; sending back a ResetTCPConnection ProxyMessage\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localState, pEntry->remoteState);
            sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            return 0;
        }

        pEntry->i64RemoteActionTime = i64CurrTime;
        // Enqueue a zero-length ReceivedData packet, which stands for the FIN packet
        uint32 ui32PcktOutSeqNum = pEntry->dqLocalHostOutBuf.empty() ? pEntry->ui32OutSeqNum :
            (pEntry->dqLocalHostOutBuf.back().getSequenceNumber() + pEntry->dqLocalHostOutBuf.back().getItemLength());
        pEntry->dqLocalHostOutBuf.emplace_back (ui32PcktOutSeqNum, 0, nullptr, NOMADSUtil::TCPHeader::TCPF_FIN | NOMADSUtil::TCPHeader::TCPF_ACK);
        auto & rrdLatestPacket = pEntry->dqLocalHostOutBuf.back();
        if (pEntry->dqLocalHostOutBuf.size() == 1) {
            // Try to transmit the FIN packet immediately since it is the only packet in the buffer
            if (0 != (rc = sendTCPPacketToHost (pEntry, rrdLatestPacket.getTCPFlags(), rrdLatestPacket.getSequenceNumber()))) {
                checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send FIN+ACK packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
                return -2;
            }
            pEntry->ui32OutSeqNum++;
            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
            pEntry->i64LocalActionTime = i64CurrTime;
            rrdLatestPacket.setLastTransmitTime (i64CurrTime);

            if (pEntry->localState == TCTLS_ESTABLISHED) {
                pEntry->localState = TCTLS_FIN_WAIT_1;
                checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                "L%hu-R%hu: sent FIN, moving to FIN_WAIT_1; out sequence number is: %u\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rrdLatestPacket.getSequenceNumber());
            }
            else if (pEntry->localState == TCTLS_CLOSE_WAIT) {
                pEntry->localState = TCTLS_LAST_ACK;
                checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                "L%hu-R%hu: sent FIN while in CLOSE_WAIT, moving to LAST_ACK; out sequence number is: %u\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rrdLatestPacket.getSequenceNumber());
            }
        }

        if (pEntry->dqLocalHostOutBuf.size() > 1) {
            ul.unlock();
            rRemoteTCPTransmitterThread.wakeUpThread();
        }

        return 0;
    }

    int TCPManager::resetTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID)
    {
        auto * pEntry = _rTCPConnTable.getEntry (ui16LocalID, ui16RemoteID);
        if (pEntry == nullptr) {
            pEntry = _rTCPConnTable.getEntry (ui16LocalID);
            if (pEntry == nullptr) {
                checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", NOMADSUtil::Logger::L_MildError,
                                "no entry found in TCP Connection Table with local ID %hu and remote ID %hu; "
                                "impossible to reset connection to local host\n", ui16LocalID, ui16RemoteID);
                return -1;
            }
        }

        int rc;
        std::lock_guard<std::mutex> lg{pEntry->getMutexRef()};
        if ((pEntry->ui16RemoteID != ui16RemoteID) && (pEntry->ui16RemoteID != 0)) {
            checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", NOMADSUtil::Logger::L_Warning,
                            "retrieved an entry from the TCP Connection Table with local ID %hu, but the remote "
                            "ID %hu does not match: the remote ID currently assigned to the entry is %hu\n",
                            ui16LocalID, ui16RemoteID, pEntry->ui16RemoteID);
        }

        pEntry->remoteState = TCTRS_Disconnected;
        pEntry->i64RemoteActionTime = NOMADSUtil::getTimeInMilliseconds();
        if ((pEntry->localState == TCTLS_SYN_SENT) || (pEntry->localState == TCTLS_SYN_RCVD) ||
            (pEntry->localState == TCTLS_ESTABLISHED) || (pEntry->localState == TCTLS_CLOSE_WAIT) ||
            (pEntry->localState == TCTLS_TIME_WAIT) || (pEntry->localState == TCTLS_FIN_WAIT_1) ||
            (pEntry->localState == TCTLS_FIN_WAIT_2)) {
            // Directly send the RST
            uint32 ui32OutSeqNum = (pEntry->localState == TCTLS_SYN_RCVD) ? 0 : pEntry->ui32OutSeqNum;
            uint8 ui8TCPFlags = (pEntry->localState == TCTLS_SYN_RCVD) ?
                (NOMADSUtil::TCPHeader::TCPF_RST | NOMADSUtil::TCPHeader::TCPF_ACK) : NOMADSUtil::TCPHeader::TCPF_RST;
            if (0 != (rc = sendTCPPacketToHost (pEntry, ui8TCPFlags, ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", NOMADSUtil::Logger::L_SevereError,
                                "L%hu-R%hu: failed to send an RST packet as a consequence of a ResetTCPConnection "
                                "ProxyMessage; rc = %d\n", ui16LocalID, ui16RemoteID, rc);
            }
            else {
                checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", NOMADSUtil::Logger::L_Info,
                                "L%hu-R%hu: received a ResetTCPConnection ProxyMessage from the remote NetProxy with IP %s "
                                "and UniqueID %u to reset the TCP connection between <%s:%hu> (local node) and <%s:%hu> "
                                "(remote node); successfully sent an RST to the local host\n", ui16LocalID, ui16RemoteID,
                                pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->ui32RemoteProxyUniqueID,
                                NOMADSUtil::InetAddr(pEntry->ui32LocalIP).getIPAsString(), pEntry->ui16LocalPort,
                                NOMADSUtil::InetAddr(pEntry->ui32RemoteIP).getIPAsString(), pEntry->ui16RemotePort);
            }
            pEntry->reset();
        }
        else if ((pEntry->localState == TCTLS_LISTEN) || (pEntry->localState == TCTLS_CLOSED)) {
            checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: connection is already in local state %d; ignoring the reset\n",
                            ui16LocalID, ui16RemoteID, static_cast<int> (pEntry->localState));
        }
        else {
            checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: connection is in local state %d; cannot reset the local state\n",
                            ui16LocalID, ui16RemoteID, static_cast<int> (pEntry->localState));
            return -2;
        }

        return 0;
    }

    int TCPManager::flushAndSendCloseConnectionRequest (Entry * const pEntry)
    {
        // Flush any data left in compressor buffer
        int rc;
        unsigned int uiBytesToSend = 0, uiSentBytes = 0;
        unsigned char *pDest[1];
        *pDest = nullptr;
        if (0 != (rc = pEntry->getConnectorWriter()->flush (pDest, uiBytesToSend))) {
            checkAndLogMsg ("TCPManager::flushAndSendCloseConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: flushWriter() failed with rc = %d; sending RST packet to local host and clearing connection\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            return -1;
        }

        if (pEntry->getConnection() == nullptr) {
            checkAndLogMsg ("TCPManager::flushAndSendCloseConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: could not retrieve the Connection to remote proxy with address %s for protocol %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->iaRemoteProxyAddr.getIPAsString(), pEntry->getProtocol());
            return -2;
        }

        while (uiBytesToSend > uiSentBytes) {
            uint32 bytesToWriteToPacket = std::min (static_cast<uint32> (uiBytesToSend - uiSentBytes),
                static_cast<uint32> (NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE));
            uint8 ui8Flags = (uiBytesToSend <= uiSentBytes) ?
                (NOMADSUtil::TCPHeader::TCPF_ACK | NOMADSUtil::TCPHeader::TCPF_PSH) : NOMADSUtil::TCPHeader::TCPF_ACK;
            if (0 != (rc = pEntry->getConnection()->sendTCPDataToRemoteHost (pEntry, *pDest + uiSentBytes, bytesToWriteToPacket, ui8Flags))) {
                checkAndLogMsg ("TCPManager::flushAndSendCloseConnectionRequest", NOMADSUtil::Logger::L_MildError,
                                "L%hu-R%hu: sendTCPDataToRemoteHost() failed with rc = %d; couldn't send final (flushed) bytes;\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                break;
            }
            uiSentBytes += bytesToWriteToPacket;
        }
        if (uiBytesToSend > uiSentBytes) {
            // Unable to flush all data --> connection already reset
            checkAndLogMsg ("TCPManager::flushAndSendCloseConnectionRequest", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: unable to flush all data to remote host; flushed only %u/%u; connection has been reset\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, uiSentBytes, uiBytesToSend);
            return -3;
        }
        else {
            checkAndLogMsg ("TCPManager::flushAndSendCloseConnectionRequest", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: flushed all data to remote host; amount of flushed bytes is %u\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, uiSentBytes);
        }

        // The outgoing queue is empty, so send close request to the remote side
        if (0 != (rc = pEntry->getConnection()->sendCloseTCPConnectionRequest (pEntry))) {
            checkAndLogMsg ("TCPManager::flushAndSendCloseConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: could not send close connection request to remote proxy %s; rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->iaRemoteProxyAddr.getIPAsString(), rc);
            return -4;
        }

        return 0;
    }

    int TCPManager::sendRemoteResetRequestIfNeeded (Entry * const pEntry)
    {
        int rc;
        Connection * const pConnection = pEntry->getConnection();
        if (!pConnection || !pConnection->isConnected()) {
            checkAndLogMsg ("TCPManager::sendRemoteResetRequestIfNeeded", NOMADSUtil::Logger::L_Warning,
                            "L%hu-R%hu: could not send a RemoteTCPResetRequest to remote proxy with address <%s:%hu>; "
                            "impossible to retrieve Connection (Connection deleted or disconnected, or Entry already reset)\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->iaRemoteProxyAddr.getIPAsString(),
                            pEntry->iaRemoteProxyAddr.getPort());
            return -1;
        }

        // Look if we need to reset remote connection
        if (((pEntry->remoteState == TCTRS_ConnEstablished) || (pEntry->remoteState == TCTRS_ConnRequestSent) ||
            (pEntry->remoteState == TCTRS_ConnRequestReceived) || (pEntry->remoteState == TCTRS_DisconnRequestSent) ||
             (pEntry->remoteState == TCTRS_DisconnRequestReceived)) && (pEntry->ui16RemoteID != 0)) {
            if (0 != (rc = pConnection->sendResetTCPConnectionRequest (pEntry))) {
                checkAndLogMsg ("TCPManager::sendRemoteResetRequestIfNeeded", NOMADSUtil::Logger::L_MildError,
                                "L%hu-R%hu: could not send a remoteResetRequest to remote proxy with address %s; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->iaRemoteProxyAddr.getIPAsString(), rc);
                return -2;
            }
        }

        pEntry->remoteState = TCTRS_Disconnected;
        return 0;
    }
}
