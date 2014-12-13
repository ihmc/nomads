/*
 * TCPManager.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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
#include "NetworkInterface.h"
#include "PacketBufferManager.h"
#include "ConnectorReader.h"
#include "Entry.h"
#include "TCPConnTable.h"
#include "ConnectionManager.h"
#include "NetProxyConfigManager.h"
#include "PacketRouter.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    int TCPManager::handleTCPPacketFromHost (const uint8 *pPacket, uint16 ui16PacketLen)
    {
        static int rc;
        static GUIStatsManager * const pGUIStatsManager = GUIStatsManager::getGUIStatsManager();

        // Assumptions: pPacket does not include EtherFrameHeader (and therefore points to IPHeader)
        //              pPacket is in Host byte order
        const IPHeader *pIPHeader = (const IPHeader*) pPacket;
        uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        const TCPHeader *pTCPHeader = (const TCPHeader*) (pPacket + ui16IPHeaderLen);
        uint16 ui16TCPHeaderLen = (pTCPHeader->ui8Offset >> 4) * 4;
        uint16 ui16DataSize = ui16PacketLen - (ui16IPHeaderLen + ui16TCPHeaderLen);
        uint32 ui32ACKedPackets = 0;
        uint64 packetTime = getTimeInMilliseconds();

        register Entry *pEntry = NULL;
        pEntry = _pTCPConnTable->getEntry (pIPHeader->srcAddr.ui32Addr, pTCPHeader->ui16SPort, pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort);
        pEntry->lock();
        uint64 oldWinRecSize = pEntry->ui16ReceiverWindowSize;

        if (pEntry->localStatus == TCTLS_CLOSED) {
            // Check if it is necessary to send an RST, and then drop the packet
            if (!(pTCPHeader->ui8Flags & TCPHeader::TCPF_RST)) {
                uint32 ui32OutSeqNum = (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) ? pTCPHeader->ui32AckNum : 0;
                uint8 ui8Flags = (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) ? TCPHeader::TCPF_RST : (TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK);
                pEntry->ui32NextExpectedInSeqNum = (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) ? 0 : (pTCPHeader->ui32SeqNum + ui16DataSize);
                if (0 != (rc = sendTCPPacketToHost (pEntry, ui8Flags, ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                                    "L%hu-R0: failed to send RST packet; rc = %d\n",
                                    pEntry->ui16ID, rc);
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                    "L%hu-R0: received a packet with flag %hhu but connection is not established (status is %d); sent an RST\n",
                                    pEntry->ui16ID, pTCPHeader->ui8Flags, pEntry->localStatus);
                }
            }
            pEntry->reset();
            pEntry->unlock();
            return 0;
        }

        if ((pEntry->localStatus == TCTLS_Unused) || (pEntry->localStatus == TCTLS_LISTEN)) {
            if (pTCPHeader->ui8Flags == TCPHeader::TCPF_SYN) {       // NOTE - this is an equality test on purpose - should only be true for the SYN and not for the SYN+ACK packet
                // Since this is a SYN, lookup the proxy address for the destination address
                // This is the only time we should need to look up the proxy address
                const ProtocolSetting * const pProtocolSetting = _pConfigurationManager->mapAddrToProtocol (pIPHeader->srcAddr.ui32Addr, pTCPHeader->ui16SPort,
                                                                                                            pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort, IP_PROTO_TCP);
                ProxyMessage::Protocol currentProtocol = pProtocolSetting ? pProtocolSetting->getProxyMessageProtocol() : ProtocolSetting::DEFAULT_TCP_MAPPING_PROTOCOL;
                ConnectorType connectorType = ProtocolSetting::protocolToConnectorType (currentProtocol);
                if (!pProtocolSetting) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                    "the protocol specification for this address was not found; using default protocol (id = %d)\n",
                                    ProtocolSetting::DEFAULT_TCP_MAPPING_PROTOCOL);
                }
                QueryResult query (_pConnectionManager->queryConnectionToRemoteHostForConnectorType (connectorType, pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort));
                const InetAddr * const pProxyAddr = query.getBestConnectionSolution();
                if (!pProxyAddr) {
                    pEntry->ui32StartingInSeqNum = pTCPHeader->ui32SeqNum;                      // Starting SEQ Number of the communication used by the local host
                    pEntry->setNextExpectedInSeqNum (pTCPHeader->ui32SeqNum + 1);               // Next expected sequence number sent by the host, ie the next ACK the host will send to local host
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                    "received a SYN packet destined for %s:%hu which could not be mapped to a "
                                    "proxy address - sending back an RST and ignoring connect request\n",
                                    InetAddr (pIPHeader->destAddr.ui32Addr).getIPAsString(), pTCPHeader->ui16DPort);
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK, 0))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                                        "L%hu-R0: failed to send RST+ACK packet; rc = %d\n",
                                        pEntry->ui16ID, rc);
                    }
                    pEntry->reset();
                    pEntry->unlock();
                    return 0;
                }
                pEntry->ui32RemoteProxyUniqueID = query.getRemoteProxyUniqueID();
                pEntry->remoteProxyAddr = *pProxyAddr;
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                "L%hu-R0: received a SYN packet with SEQ number %u from application at address %s:%hu, addressed to %s:%hu\n",
                                pEntry->ui16ID, pTCPHeader->ui32SeqNum, InetAddr (pIPHeader->srcAddr.ui32Addr).getIPAsString(),
                                pTCPHeader->ui16SPort, InetAddr (pIPHeader->destAddr.ui32Addr).getIPAsString(), pTCPHeader->ui16DPort);

                // Received a SYN --> moving to SYN_RCVD
                pEntry->localStatus = TCTLS_SYN_RCVD;
                pEntry->ui32StartingInSeqNum = pTCPHeader->ui32SeqNum;                      // Starting SEQ Number of the communication used by the local host
                pEntry->setNextExpectedInSeqNum (pTCPHeader->ui32SeqNum + 1);               // Next expected sequence number sent by the host, ie the next ACK the host will send to local host
                pEntry->ui32ReceivedDataSeqNum = pTCPHeader->ui32SeqNum;
                pEntry->ui16ReceiverWindowSize = pTCPHeader->ui16Window;
                pEntry->i64LocalActionTime = packetTime;
                pEntry->i64LastAckTime = packetTime;

                pEntry->setProtocol (currentProtocol);
                pEntry->setMocketConfFile (_pConnectionManager->getMocketsConfigFileForConnectionsToRemoteHost (pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort));

                const CompressionSetting * const pCompressionSetting = &pProtocolSetting->getCompressionSetting();
                delete pEntry->setProperConnectorWriter (pCompressionSetting);

                Connector * const pConnector = _pConnectionManager->getConnectorForType (connectorType);
                if (!pConnector) {
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK, 0))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R0: failed to send RST+ACK packet; rc = %d\n",
                                        pEntry->ui16ID, rc);
                    }
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                                    "L%hu-R%hu: could not retrieve the connector to remote proxy with address %s for protocol %d; "
                                    "sending RST to local application\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                    pEntry->remoteProxyAddr.getIPAsString(), pEntry->getProtocol());
                    pEntry->reset();
                    pEntry->unlock();
                    return -1;
                }
                pEntry->setConnector (pConnector);

                Connection *pConnection = query.getActiveConnectionToRemoteProxy();
                pConnection = (pConnection && (pConnection->isConnected() || pConnection->isConnecting())) ?
                               pConnection : pEntry->getConnector()->openNewConnectionToRemoteProxy (query, false);
                pConnection = pConnection ? pConnection : pEntry->getConnector()->getAvailableConnectionToRemoteProxy (&pEntry->remoteProxyAddr);       // Set connection even if not yet connected
                if (!pConnection || pConnection->hasFailedConnecting()) {
                    pEntry->ui32StartingOutSeqNum = 0;
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK, 0))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R0: failed to send RST+ACK packet; rc = %d\n",
                                        pEntry->ui16ID, rc);
                    }
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                                    "L%hu-R0: received a SYN, but it was impossible to create a new %sConnection to connect the "
                                    "remote NetProxy at address %s in order to remap a new TCP Connection from %s:%hu to %s:%hu\n",
                                    pEntry->ui16ID, pConnector->getConnectorTypeAsString(), pProxyAddr->getIPAsString(),
                                    InetAddr (pIPHeader->srcAddr.ui32Addr).getIPAsString(), pTCPHeader->ui16SPort,
                                    InetAddr (pIPHeader->destAddr.ui32Addr).getIPAsString(), pTCPHeader->ui16DPort);
                    pEntry->reset();
                    pEntry->unlock();
                    return -2;
                }
                pEntry->setConnection (pConnection);

                // Check if is necessary to send an OpenTCPConnectionRequest to avoid sending multiple openTCPConnection requests
                if (pEntry->remoteStatus == TCTRS_Unknown) {
                    if (pEntry->getConnection() && pEntry->getConnection()->isConnected()) {
                        bool bReachable = _pConnectionManager->getReachabilityFromRemoteHost (pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort);
                        if (0 != (rc = pEntry->getConnection()->sendOpenTCPConnectionRequest (pEntry, bReachable))) {
                            pEntry->ui32StartingOutSeqNum = 0;
                            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK, 0))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                                "L%hu-R0: failed to send RST+ACK packet; rc = %d\n",
                                                pEntry->ui16ID, rc);
                            }
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                            "L%hu-R0: received a SYN, but it was impossible to send an OpenTCPConnection request to the remote NetProxy: "
                                            "sendOpenTCPConnectionRequest() failed with rc = %d; sent back an RST to local application\n",
                                            pEntry->ui16ID, rc);
                            pEntry->reset();
                            pEntry->unlock();
                            return -3;
                        }
                        else {
                            pEntry->remoteStatus = TCTRS_ConnRequested;
                            pEntry->i64RemoteActionTime = packetTime;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                            "L%hu-R0: received a SYN packet; sent a OpenTCPConnection request to remote proxy at address %s:%hu\n",
                                            pEntry->ui16ID, pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                        }
                    }
                    else if (pEntry->getConnection()) {
                        pEntry->remoteStatus = TCTRS_WaitingConnEstablishment;
                        pEntry->i64RemoteActionTime = packetTime;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                        "L%hu-R0: received a SYN packet when remote connection is in status %d; waiting to establish a connection "
                                        "via %s with the remote NetProxy at address %s:%hu\n", pEntry->getConnection()->getStatus(),
                                        pEntry->ui16ID, pEntry->getConnector()->getConnectorTypeAsString(),
                                        pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                    }
                    else {
                        pEntry->ui32StartingOutSeqNum = 0;
                        if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK, 0))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                            "L%hu-R0: failed to send RST+ACK packet; rc = %d\n",
                                            pEntry->ui16ID, rc);
                        }
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                        "L%hu-R0: received a SYN, but it was impossible to open a new Connection to the remote NetProxy "
                                        "with address %s:%hu; sent back an RST to local application\n", pEntry->ui16ID,
                                        pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                        pEntry->reset();
                        pEntry->unlock();
                        return -4;
                    }
                }
                /*
                // This code is commented out because we do not want to reply with a SYN+ACK right away, but we wait for a TCPConnectionOpened message first
                if (0 != (rc = sendTCPPacketToHost (pEntry, (TCPHeader::TCPF_SYN | TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                                    "L%hu-R0: failed to send SYN+ACK packet after having moved from SYN_SENT to SYN_RCVD; rc = %d\n",
                                    pEntry->ui16ID, rc);
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->clear();
                    pEntry->unlock();
                    return -3;
                }
                pEntry->ui32OutSeqNum++;
                pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                "L%hu-R0: received a SYN packet; sent a SYN+ACK packet with ACK %u (relative %u) and SEQ num %u (relative %u); moved to SYN_RCVD\n",
                                pEntry->ui16ID, pEntry->ui32NextExpectedInSeqNum, SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                pEntry->ui32OutSeqNum - 1, SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1, pEntry->ui32StartingOutSeqNum));
                */
                pEntry->unlock();
                return 0;
            }
            // Connection is not open --> sending back an RST if it's not been received an RST itself
            else if (!(pTCPHeader->ui8Flags & TCPHeader::TCPF_RST)) {
                uint32 ui32OutSeqNum = (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) ? pTCPHeader->ui32AckNum : 0;
                uint8 ui8Flags = (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) ? TCPHeader::TCPF_RST : (TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK);
                pEntry->ui32NextExpectedInSeqNum = (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) ? 0 : (pTCPHeader->ui32SeqNum + ui16DataSize);
                if (0 != (rc = sendTCPPacketToHost (pEntry, ui8Flags, ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                    "L%hu-R0: failed to send RST packet; rc = %d\n",
                                    pEntry->ui16ID, rc);
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                    "L%hu-R0: received a packet with flag %hhu but connection is not established (status is %d); sent an RST\n",
                                    pEntry->ui16ID, pTCPHeader->ui8Flags, pEntry->localStatus);
                }
            }

            if (pTCPHeader->ui8Flags != TCPHeader::TCPF_SYN) {
                // If it's not a SYN, either we have sent an RST or received one --> resetting connection
                pEntry->reset();
            }

            pEntry->unlock();
            return 0;
        }
        else if (pEntry->localStatus == TCTLS_SYN_SENT) {
            if (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) {
                if (pTCPHeader->ui32AckNum != pEntry->ui32OutSeqNum) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a SYN+ACK but with the wrong ack num - expected %u, got %u; sending RST\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum, pTCPHeader->ui32AckNum);
                    // Probably an OLD SYN request arrived to local host. Sending an RST
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pTCPHeader->ui32AckNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    }

                    // Don't modify neither local status nor pEntry. Wait for right SYN+ACK response
                    pEntry->unlock();
                    return  0;
                }
            }
            // If we get here, either the ACK is acceptable or there is no ACK flag set
            if (pTCPHeader->ui8Flags & TCPHeader::TCPF_RST) {
                if (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                    "L%hu-R%hu: received an RST while in status SYN_SENT; resetted connection\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                    "L%hu-R%hu: received an RST without ACK flag set while in status SYN_SENT; ignoring\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                }
                pEntry->unlock();
                return 0;
            }

            // HERE THERE SHOULD BE A CHECK FOR SECURITY AND PRECEDENCE <--- RFC 793
            if (pTCPHeader->ui8Flags & TCPHeader::TCPF_SYN) {
                // Move to established state and send an ACK
                pEntry->ui32StartingInSeqNum = pTCPHeader->ui32SeqNum;                  // Starting Sequence Number of the communication used by the local host.
                pEntry->setNextExpectedInSeqNum (pTCPHeader->ui32SeqNum + 1);           // Next expected sequence number sent by the host, ie the next ACK the host will send to local host
                pEntry->ui16ReceiverWindowSize = pTCPHeader->ui16Window;
                pEntry->i64LocalActionTime = packetTime;
                delete pEntry->outBuf.dequeue();                                        // Deleting SYN packet in output buffer (both if we are moving to ESTABLISHED or to SYN_RCVD)

                if (pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) {
                    // Received a SYN+ACK with the correct ACK number --> connection ESTABLISHED
                    pEntry->ui32ReceivedDataSeqNum = pTCPHeader->ui32SeqNum;            // last SEQNUM received
                    pEntry->ui32LastAckSeqNum = pTCPHeader->ui32AckNum;                 // last ACKNUM received
                    pEntry->i64LastAckTime = packetTime;                                // last ACK received time
                    pEntry->ui8RetransmissionAttempts = 0;                              // Reset retransmission attempts
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK packet to finish establishing the connection; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send RST packet to local host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        pEntry->unlock();
                        return -5;
                    }

                    pEntry->localStatus = TCTLS_ESTABLISHED;
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                    "L%hu-R%hu: local connection ESTABLISHED. Proxy packets with SEQ %u, host packets with SEQ %u\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum, pEntry->ui32NextExpectedInSeqNum);

                    if (pEntry->remoteStatus == TCTRS_ConnRequested) {
                        // Sending a TCPConnectionOpened Response to remote proxy
                        bool bReachable = _pConnectionManager->getReachabilityFromRemoteHost (pEntry->ui32RemoteIP, pTCPHeader->ui16DPort);
                        if (pEntry->getConnection() && pEntry->getConnection()->isConnected()) {
                            if (0 != (rc = pEntry->getConnection()->sendTCPConnectionOpenedResponse (pEntry, bReachable))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                                                "L%hu-R%hu: could not send connection opened response to remote proxy %s; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID,
                                                pEntry->remoteProxyAddr.getIPAsString(), rc);
                                if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                                    "L%hu-R%hu: failed to send RST packet to local host; rc = %d\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                }
                                else {
                                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                                    "L%hu-R%hu: sent an RST packet to local host because of an error in the remote connection with proxy %s\n",
                                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString());
                                }
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                pEntry->unlock();
                                return -6;
                            }
                        }
                        else if (pEntry->getConnection() && pEntry->getConnection()->isConnecting()) {
                            // Local connection status is TCTLS_ESTABLISHED, remote connection status is TCTRS_ConnRequested
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                            "L%hu-R%hu: received a SYN+ACK packet when remote connection is in status %d; waiting to establish a connection "
                                            "via %s with the remote NetProxy at address %s:%hu\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pEntry->getConnection()->getStatus(), pEntry->getConnector()->getConnectorTypeAsString(),
                                            pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                            pEntry->unlock();
                            return 0;
                        }
                        else {
                            pEntry->ui32StartingOutSeqNum = 0;
                            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK, 0))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send RST+ACK packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            else {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                                "L%hu-R%hu: received a SYN+ACK, but it was impossible to open a new Connection to the remote NetProxy at "
                                                "address %s:%hu; sent back an RST to local application\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                                pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                            }
                            pEntry->reset();
                            pEntry->unlock();
                            return -7;
                        }

                        pEntry->remoteStatus = TCTRS_ConnEstablished;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                        "L%hu-R%hu: successfully sent a TCPConnectionOpened response for the connection from %s:%hu "
                                        "to %s:%hu to proxy at address %s; data will be coded to %s:%hu and decoded from %s:%hu\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, InetAddr(pEntry->ui32LocalIP).getIPAsString(), pEntry->ui16LocalPort,
                                        InetAddr(pEntry->ui32RemoteIP).getIPAsString(), pEntry->ui16RemotePort, pEntry->remoteProxyAddr.getIPAsString(),
                                        pEntry->getConnectorWriter()->getCompressionName(), (unsigned short) pEntry->getConnectorWriter()->getCompressionLevel(),
                                        pEntry->getConnectorReader()->getCompressionName(), (unsigned short) pEntry->getConnectorReader()->getCompressionLevel());
                    }
                    else if (pEntry->remoteStatus == TCTRS_ConnEstablished) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received first SYN+ACK from local application when the connection with the remote NetProxy "
                                        "at address %s:%hu was already established\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                        pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort());
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                        "L%hu-R%hu: received first SYN+ACK from local application, but remote connection with proxy "
                                        "at address %s is in status %d; local and remote connections are out of sync; resetting\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteStatus);
                        if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                            "L%hu-R%hu: failed to send RST packet to local host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        }
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        pEntry->unlock();
                        return -8;
                    }
                }
                else {
                    // ******* IN THIS CASE THE CONNECTION TO REMOTE PROXY IS ALREADY ESTABLISHED --> NO NEED TO DO ANYTHING ********
                    pEntry->localStatus = TCTLS_SYN_RCVD;
                    pEntry->prepareNewConnection();
                    if (0 != (rc = sendTCPPacketToHost (pEntry, (TCPHeader::TCPF_SYN | TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send SYN+ACK packet after having moved from SYN_SENT to SYN_RCVD; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        pEntry->unlock();
                        return -9;
                    }
                    pEntry->ui32OutSeqNum++;
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                    "L%hu-R%hu: received a SYN packet while in status SYN_SENT; sent a SYN+ACK packet with SEQ number %u "
                                    "and moved to SYN_RCVD\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum - 1);
                }
            }

            // If there is already data in the buffer, wake up the lTT
            bool isLTTDataReady = pEntry->outBuf.size() > 0;
            pEntry->unlock();
            if (isLTTDataReady) {
                if (_pPacketRouter->_mLocalTCPTransmitter.tryLock() == Mutex::RC_Ok) {
                    _pPacketRouter->_cvLocalTCPTransmitter.notify();
                    _pPacketRouter->_mLocalTCPTransmitter.unlock();
                }
            }
            // Returning after having moved to status ESTABLISHED or SYN_RCVD
            return 0;
        }
        else if ((pEntry->localStatus == TCTLS_SYN_RCVD) && (pTCPHeader->ui8Flags == TCPHeader::TCPF_SYN)) {
            // Duplicate SYN packet
            if ((pEntry->remoteStatus == TCTRS_ConnEstablished) && (pTCPHeader->ui32SeqNum == (pEntry->ui32NextExpectedInSeqNum - 1))) {
                // SYN+ACK was lost
                if (0 != (rc = sendTCPPacketToHost (pEntry, (TCPHeader::TCPF_SYN | TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::tcpConnectionToHostOpened", Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send a SYN+ACK packet having received a duplicate SYN in status SYN_RCVD; "
                                    "rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    pEntry->unlock();
                    return -10;
                }
                pEntry->i64LocalActionTime = packetTime;
                pEntry->i64LastAckTime = packetTime;
                checkAndLogMsg ("TCPManager::tcpConnectionToHostOpened", Logger::L_LowDetailDebug,
                                "L%hu-R%hu: sent SYN+ACK packet with ACK %u (relative %u) and SEQ num %u (relative %u) to reply "
                                "to a duplicate SYN\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                pEntry->ui32OutSeqNum - 1, SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1, pEntry->ui32StartingOutSeqNum));
            }

            // If the previous equality does not hold true, then we are still trying to connect to the remote NetProxy --> we simply ignore the SYN
            pEntry->unlock();
            return 0;
        }

        // Following checkings obey to RFC 793 directives
        if ((pEntry->getOutgoingBufferRemainingSpace() == 0) && (ui16DataSize == 0)) {
            if (!(pEntry->ui32NextExpectedInSeqNum == pTCPHeader->ui32SeqNum)) {
                if (!(pTCPHeader->ui8Flags & TCPHeader::TCPF_RST)) {
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK in answer to a packet that couldn't be saved because buffer is full; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received a packet with a wrong SEQ number (received %u, expecting %u) "
                                        "and flags %hhu while buffer window is full; sent back an ACK\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                        pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui8Flags);
                    }
                }
                pEntry->unlock();
                return 0;
            }
            else {
                if (!(pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK) && !(pTCPHeader->ui8Flags & TCPHeader::TCPF_URG) &&
                    !(pTCPHeader->ui8Flags & TCPHeader::TCPF_RST)) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with FLAGs %hhu that cannot be processed because buffer window is full; dropped\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui8Flags);
                    pEntry->unlock();
                    return 0;
                }
            }
        }
        else if ((pEntry->getOutgoingBufferRemainingSpace() > 0) && (ui16DataSize == 0)) {
            if ((pTCPHeader->ui32SeqNum == pEntry->ui32StartingInSeqNum) && (pEntry->localStatus == TCTLS_SYN_RCVD)) {
                if (pEntry->remoteStatus == TCTRS_ConnEstablished) {
                    // SYN+ACK was lost, re-sending back a SYN+ACK
                    if (0 != (rc = sendTCPPacketToHost (pEntry, (TCPHeader::TCPF_SYN | TCPHeader::TCPF_ACK), (pEntry->ui32OutSeqNum - 1)))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R0: failed to send SYN+ACK packet after having moved from SYN_SENT to SYN_RCVD; rc = %d\n",
                                        pEntry->ui16ID, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                    }
                    else {
                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                        "L%hu-R0: received a duplicate SYN packet (likely loss of SYN+ACK packet); "
                                        "sent a SYN+ACK packet with ACK %u (relative %u) and SEQ num %u (relative %u); moved to SYN_RCVD\n",
                                        pEntry->ui16ID, pEntry->ui32NextExpectedInSeqNum,
                                        SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                        pEntry->ui32OutSeqNum - 1, SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1, pEntry->ui32StartingOutSeqNum));
                    }
                }
                pEntry->unlock();
                return 0;
            }
            else if (!SequentialArithmetic::lessThanOrEqual (pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui32SeqNum) ||
                !SequentialArithmetic::lessThan (pTCPHeader->ui32SeqNum, (pEntry->ui32NextExpectedInSeqNum + pEntry->getOutgoingBufferRemainingSpace()))) {
                // Packet not acceptable --> sending back an ACK with right SEQ num and window size
                if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                    "failed to send ACK in answer to a packet that couldn't be saved in window buffer; rc = %d\n", rc);
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                }
                else {
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with SEQ %u (relative %u), 0 bytes of data and flags %hhu while in STATUS %d; "
                                    "expecting SEQ number %u (relative %u); window size is %u; sent back an ACK with correct values\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    pTCPHeader->ui8Flags, pEntry->localStatus, pEntry->ui32NextExpectedInSeqNum,
                                    SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                    pEntry->getOutgoingBufferRemainingSpace());
                }
                pEntry->unlock();
                return 0;
            }
        }
        else if ((pEntry->getOutgoingBufferRemainingSpace() == 0) && (ui16DataSize > 0)) {
            // Impossible to store a packet with data since window free space is zero --> sending back an ACK with window status and correct ACK and SEQ numbers
            if (!(pTCPHeader->ui8Flags & TCPHeader::TCPF_RST)) {
                if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send ACK in answer to a packet that couldn't be saved in window buffer; rc = %d\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                }
                else {
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with with SEQ %u (relative %u), flags %hhu and %hu bytes"
                                    " of data that couldn't be stored in the full buffer window; sent back an ACK\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    pTCPHeader->ui8Flags, ui16DataSize);
                }
            }
            // Invalid RST received or sent ACK with correct values --> drop packet and return
            pEntry->unlock();
            return 0;
        }
        else if ((pEntry->getOutgoingBufferRemainingSpace() > 0) && (ui16DataSize > 0)) {
            if (!((SequentialArithmetic::lessThanOrEqual (pEntry->ui32NextExpectedInSeqNum, pTCPHeader->ui32SeqNum) &&
                SequentialArithmetic::lessThan (pTCPHeader->ui32SeqNum, (pEntry->ui32NextExpectedInSeqNum + pEntry->getOutgoingBufferRemainingSpace()))) ||
                (SequentialArithmetic::lessThanOrEqual (pEntry->ui32NextExpectedInSeqNum, (pTCPHeader->ui32SeqNum + ui16DataSize - 1)) &&
                SequentialArithmetic::lessThan ((pTCPHeader->ui32SeqNum + ui16DataSize - 1), (pEntry->ui32NextExpectedInSeqNum + pEntry->getOutgoingBufferRemainingSpace()))))) {
                if (!(pTCPHeader->ui8Flags & TCPHeader::TCPF_RST)) {
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK in answer to a packet that couldn't be saved in window buffer; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                    }
                    else {
                        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received a packet with with SEQ %u (relative %u), flags %hhu and %hu bytes of data "
                                        "that did not belong to the buffer window; sent back an ACK\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                        pTCPHeader->ui32SeqNum, SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                        pTCPHeader->ui8Flags, ui16DataSize);
                    }
                }
                // Invalid RST received or sent ACK with correct values --> drop packet and return
                pEntry->unlock();
                return 0;
            }
        }

        if (pTCPHeader->ui8Flags & TCPHeader::TCPF_RST) {
            // Received an acceptable RST packet
            if (pEntry->localStatus == TCTLS_SYN_RCVD) {
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                "L%hu-R0: Received an RST while in status SYN_RCVD; connection resetted\n",
                                pEntry->ui16ID);
                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
            }
            else if ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_FIN_WAIT_1) ||
                (pEntry->localStatus == TCTLS_FIN_WAIT_2) || (pEntry->localStatus == TCTLS_CLOSE_WAIT)) {
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                "L%hu-R%hu: Received an RST while in status %d; connection resetted\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus);
                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
            }
            else {
                if (((pEntry->localStatus == TCTLS_CLOSING) || (pEntry->localStatus == TCTLS_TIME_WAIT)) &&
                    (pEntry->remoteStatus == TCTRS_DisconnRequestReceived)) {
                    if (pEntry->getConnection() == NULL) {
                        checkAndLogMsg ("PacketRouter::remoteTransmitterThreadRun", Logger::L_MildError,
                                        "L%hu-R%hu: getConnection() returned a NULL pointer; impossible to send a CloseTCPConnection request "
                                        "to the remote NetProxy\n", pEntry->ui16ID, pEntry->ui16RemoteID);
                    }
                    else if (0 != (rc = pEntry->getConnection()->sendCloseTCPConnectionRequest (pEntry))) {
                        checkAndLogMsg ("PacketRouter::handleTCPPacketFromHost", Logger::L_MildError,
                                        "L%hu-R%hu: sendCloseTCPConnectionRequest() failed with rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    }
                }
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: Received an RST while in status %d; connection closed normally\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus);
                pEntry->reset();
            }
            pEntry->unlock();
            return 0;
        }

        if (pTCPHeader->ui8Flags & TCPHeader::TCPF_SYN) {
            // Receiving a SYN here is an error --> sending back an RST
            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                "L%hu-R%hu: failed to send ACK to an unexpected packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            pEntry->unlock();
            return 0;
        }

        if (!(pTCPHeader->ui8Flags & TCPHeader::TCPF_ACK)) {
            // If there is no ACK flag, elaboration is over --> unlock() table and return
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                            "L%hu-R%hu: received a packet without ACK flag set; flags are %hhu; ignoring\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui8Flags);
            pEntry->unlock();
            return 0;
        }

        // ACK Flag is on --> continuing elaboration
        if (pEntry->localStatus == TCTLS_SYN_RCVD) {
            if (pTCPHeader->ui32AckNum == pEntry->ui32OutSeqNum) {
                // Received an ACK while in status SIN_RCVD --> moving to established and continue elaboration
                pEntry->localStatus = TCTLS_ESTABLISHED;
                pEntry->ui32LastAckSeqNum = pTCPHeader->ui32AckNum;
                pEntry->i64LocalActionTime = packetTime;
            }
            else {
                // Received an out of order ACK num while in status SYN_RCVD --> sending back an RST
                if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pTCPHeader->ui32AckNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                    "L%hu-R0: failed to send RST packet to local host; rc = %d\n",
                                    pEntry->ui16ID, rc);
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                    "L%hu-R0: received an ACK with a wrong number while in SYN_RCVD status; sent an RST\n",
                                    pEntry->ui16ID);
                }
                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
                pEntry->unlock();
                return 0;
            }
        }
        if ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_FIN_WAIT_1) ||
            (pEntry->localStatus == TCTLS_FIN_WAIT_2) || (pEntry->localStatus == TCTLS_CLOSE_WAIT) ||
            (pEntry->localStatus == TCTLS_CLOSING) || (pEntry->localStatus == TCTLS_LAST_ACK) ||
            (pEntry->localStatus == TCTLS_TIME_WAIT)) {
            if (SequentialArithmetic::lessThanOrEqual (pEntry->ui32LastAckSeqNum, pTCPHeader->ui32AckNum) &&
                SequentialArithmetic::lessThanOrEqual (pTCPHeader->ui32AckNum, pEntry->ui32OutSeqNum)) {
                // Acceptable ACK
                if ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_CLOSE_WAIT)) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received an ACK with number %u (relative %u); first unacknowledged packet in the output buffer had SEQ number %u; "
                                    "next sent packet will have SEQ number %u (relative %u); receiver window size has been updated from %hu to %hu\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32AckNum,
                                    SequentialArithmetic::delta (pTCPHeader->ui32AckNum, pEntry->ui32StartingOutSeqNum), pEntry->ui32LastAckSeqNum,
                                    pEntry->ui32OutSeqNum, SequentialArithmetic::delta (pEntry->ui32OutSeqNum, pEntry->ui32StartingOutSeqNum),
                                    pEntry->ui16ReceiverWindowSize, pTCPHeader->ui16Window);
                }
                pEntry->updateOutgoingWindow (pTCPHeader);                                          // Updates variables of pEntry with the data in the received packet
                ui32ACKedPackets = pEntry->ackOutgoingDataUpto (pTCPHeader->ui32AckNum);            // Removes ACKed packets from memory

                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                "L%hu-R%hu: %u packets have been ACKnowledged; there are %u packets left in the outBuf and there are %hu free bytes in the receiver window buffer\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, ui32ACKedPackets, pEntry->outBuf.size(), pTCPHeader->ui16Window);

                if (pTCPHeader->ui32AckNum == pEntry->ui32OutSeqNum) {
                    // Received ACK is up-to-date: check if local status needs to be changed
                    if (pEntry->localStatus == TCTLS_FIN_WAIT_1) {
                        pEntry->localStatus = TCTLS_FIN_WAIT_2;
                        pEntry->i64LocalActionTime = packetTime;
                        delete pEntry->outBuf.dequeue();
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: received an ACK while in FIN_WAIT_1 - moved to FIN_WAIT_2\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                    }
                    else if (pEntry->localStatus == TCTLS_FIN_WAIT_2) {
                        if (pEntry->outBuf.size() == 0) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: received an updated ACK while in status FIN_WAIT_2 and with an empty retransmission queue: all sent packets have been ACKed\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }
                    else if (pEntry->localStatus == TCTLS_CLOSING) {
                        pEntry->localStatus = TCTLS_TIME_WAIT;
                        pEntry->i64LocalActionTime = packetTime;
                        delete pEntry->outBuf.dequeue();
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                        "L%hu-R%hu: received an updated ACK while in status CLOSING; moved to TIME_WAIT\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                    }
                    else if (pEntry->localStatus == TCTLS_LAST_ACK) {
                        pEntry->i64LocalActionTime = packetTime;
                        delete pEntry->outBuf.dequeue();
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                        "L%hu-R%hu: received an ACK while in status LAST_ACK; moving to CLOSED\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID);
                        pEntry->reset();
                    }
                    if (pEntry->localStatus == TCTLS_TIME_WAIT) {
                        if (pTCPHeader->ui8Flags & TCPHeader::TCPF_FIN) {
                            // Received a duplicate FIN+ACK --> send back an ACK and update localActionTime
                            pEntry->i64LocalActionTime = packetTime;
                            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send ACK to a duplicate FIN+ACK while in status TIME_WAIT; "
                                                "rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                                pEntry->reset();
                                pEntry->unlock();
                                return 0;
                            }
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: received a duplicate ACK while in status TIME_WAIT; sent back an ACK\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }
                }
            }
            else if (SequentialArithmetic::lessThan (pTCPHeader->ui32AckNum, pEntry->ui32LastAckSeqNum)) {
                // Duplicate ACK --> just drop it
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: received an ACK with number %u while in status %d, but first unacknowledged "
                                "packet in buffer has SEQ number %u; dropping packet\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                pTCPHeader->ui32AckNum, pEntry->localStatus, pEntry->ui32LastAckSeqNum);
                pEntry->unlock();
                return 0;
            }
            else {
                // Received an ACK that is greater than the sent SEQ number --> sending back an ACK with right values and returning
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                "L%hu-R%hu: received an ACK with number %u while in status %d; expecting ACK with number %u; "
                                "sending back an ACK with the expected value\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                pTCPHeader->ui32AckNum, pEntry->localStatus, pEntry->ui32OutSeqNum);
                if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send ACK to a packet with an ACK number greater than expected; "
                                    "rc = %d; resetting remote connection\n", pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                }
                pEntry->unlock();
                return 0;
            }
        }

        if (pTCPHeader->ui8Flags & TCPHeader::TCPF_URG) {
            // URG flag not yet handled
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                            "L%hu-R%hu: received a packet with the URGent flag set; FUNCTION NOT YET IMPLEMENTED\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
        }

        // Check if there is any data in this packet (segment text)
        if (ui16DataSize > 0) {
            if ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_FIN_WAIT_1) ||
                (pEntry->localStatus == TCTLS_FIN_WAIT_2)) {
                int enqueuedDataSize = 0;
                if (pTCPHeader->ui32SeqNum == pEntry->ui32NextExpectedInSeqNum) {
                    // TO-DO:   check what happens if this packets fills a hole and the FIN had already been enqueued (we should ACK the FIN, too!)
                    //          As of now, the RTT takes care of this. The present solution could increase latency
                    // Received data with correct SEQ number --> correct ordered data
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                    "L%hu-R%hu: received a TCP packet with SEQ number %u (relative %u), %hu bytes of data and FLAGs %hhu\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    ui16DataSize, pTCPHeader->ui8Flags);
                }
                else {
                    // Received data with a higher SEQ number than expected --> out-of-order data
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                    "L%hu-R%hu: received a TCP packet with %hu bytes of data in the wrong order and FLAGs %hhu - enqueuing; "
                                    "SEQ number is %u (relative %u) and ACK num is %u; expected seq num is: %u (relative %u)\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, ui16DataSize, pTCPHeader->ui8Flags, pTCPHeader->ui32SeqNum,
                                    SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum), pTCPHeader->ui32AckNum,
                                    pEntry->ui32NextExpectedInSeqNum, SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                }

                bool wereThereHolesInBuffer = pEntry->areThereHolesInOutgoingDataBuffer();      // Check if there are any holes in the incoming buffer before inserting the new packet
                if ((enqueuedDataSize = pEntry->insertTCPSegmentIntoOutgoingBuffer (new TCPSegment (pTCPHeader->ui32SeqNum, ui16DataSize,
                                                                                                    (unsigned char*) pTCPHeader + ui16TCPHeaderLen,
                                                                                                    (pTCPHeader->ui8Flags & TCP_DATA_FLAG_FILTER)))) < 0) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                    "L%hu-R%hu: failed to enqueue segment with SEQ number %u (relative %u), ACK num %u and %hu bytes of data; "
                                    "next expected SEQ num is: %u (relative %u); rc = %d\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                    pTCPHeader->ui32SeqNum, SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    pTCPHeader->ui32AckNum, ui16DataSize, pEntry->ui32NextExpectedInSeqNum,
                                    SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum), enqueuedDataSize);
                }
                else {
                    if (enqueuedDataSize == ui16DataSize) {
                        // Correctly enqueued all data
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                        "L%hu-R%hu: successfully enqueued %d bytes in the buffer\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, enqueuedDataSize);
                    }
                    else {
                        // Correctly enqueued necessary data only
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: enqueued only %d necessary bytes over %hu bytes received\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, enqueuedDataSize, ui16DataSize);
                    }

                    pGUIStatsManager->increaseTrafficIn (pEntry->getConnector()->getConnectorType(), pIPHeader->destAddr.ui32Addr, pEntry->ui32RemoteProxyUniqueID,
                                                         pEntry->remoteProxyAddr.getIPAddress(), pEntry->remoteProxyAddr.getPort(), PT_TCP, ui16DataSize);

                    // Sending ACK only if we received a PSH or the previous packet wasn't ACKed and FIN flag is not set --> reducing amount of ACKs to about 1/2
                    if ((enqueuedDataSize >= 0) && !(pTCPHeader->ui8Flags & TCPHeader::TCPF_FIN) &&
                        (SequentialArithmetic::greaterThan (pTCPHeader->ui32SeqNum, pEntry->ui32LastACKedSeqNum) ||
                        (pTCPHeader->ui8Flags & TCPHeader::TCPF_PSH) || wereThereHolesInBuffer)) {
                        if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                            "L%hu-R%hu: failed to send ACK packet to host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            return 0;
                        }
                        else {
                            // ACK sent --> last ACKed SEQ number has to be updated
                            pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: sent ACK to local host; last ACKed SEQ number and next expected SEQ number are %u (relative %u)\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pEntry->ui32LastACKedSeqNum, SequentialArithmetic::delta (pEntry->ui32LastACKedSeqNum, pEntry->ui32StartingInSeqNum));
                        }
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                        "L%hu-R%hu: ACK to local host skipped; last ACKed SEQ num is %u (relative %u), next expected SEQ num is %u (relative %u)\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID,
                                        pEntry->ui32LastACKedSeqNum, SequentialArithmetic::delta (pEntry->ui32LastACKedSeqNum, pEntry->ui32StartingInSeqNum),
                                        pEntry->ui32NextExpectedInSeqNum, SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                    }
                }
            }
            else if ((pEntry->localStatus == TCTLS_CLOSE_WAIT) || (pEntry->localStatus == TCTLS_CLOSING) ||
                    (pEntry->localStatus == TCTLS_LAST_ACK) || (pEntry->localStatus == TCTLS_TIME_WAIT)) {
                    // Received a packet with data when local host has already closed his side of the connection --> discarding the packet
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a packet with %hu bytes of data while already in status %d; ignored\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, ui16DataSize, pEntry->localStatus);
            }
        }

        // Handling FIN packet
        if (pTCPHeader->ui8Flags & TCPHeader::TCPF_FIN) {
            if ((pEntry->localStatus == TCTLS_SYN_SENT) || (pEntry->localStatus == TCTLS_CLOSED)) {
                // Segment cannot be processed --> dropping it
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: received a FIN packet while in status SYN_SENT or CLOSED (state is %d); ignoring\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus);
                pEntry->unlock();
                return 0;
            }
            else if ((pEntry->remoteStatus == TCTRS_DisconnRequestSent) || (pEntry->remoteStatus == TCTRS_Disconnected)) {
                checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                "L%hu-R%hu: received a duplicate FIN (local status is %d, remote status is %d); ignoring\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus, pEntry->remoteStatus);
                pEntry->unlock();
                return 0;
            }
            else if ((pEntry->localStatus == TCTLS_SYN_RCVD) || (pEntry->localStatus == TCTLS_ESTABLISHED) ||
                (pEntry->localStatus == TCTLS_FIN_WAIT_1) || (pEntry->localStatus == TCTLS_FIN_WAIT_2)) {
                // Ready to receive and acknowledge FIN packet
                if ((pTCPHeader->ui32SeqNum + ui16DataSize) == pEntry->ui32NextExpectedInSeqNum) {
                    // All previous packets have been received <--> SEQ num is consistent with the expected one
                    if (!pEntry->isOutgoingBufferEmpty()) {
                        // Outgoing buffer is not empty --> enqueuing TCP packet with FIN+ACK flags for later processing
                        if ((rc = pEntry->insertTCPSegmentIntoOutgoingBuffer (new TCPSegment (pTCPHeader->ui32SeqNum + ui16DataSize, 0, NULL,
                                                                                              TCPHeader::TCPF_FIN | TCPHeader::TCPF_ACK))) < 0) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                            "L%hu-R%hu: failed to enqueue FIN packet received from host; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            return 0;
                        }
                    }

                    // ACK the FIN
                    pEntry->ui32NextExpectedInSeqNum++;
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a FIN packet with SEQ number %u (relative %u) and %hu bytes of data in the correct order "
                                    "(present state is %d); a packet with ACK number %u (relative %u) will be sent back to local host\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pTCPHeader->ui32SeqNum,
                                    SequentialArithmetic::delta (pTCPHeader->ui32SeqNum, pEntry->ui32StartingInSeqNum),
                                    ui16DataSize, pEntry->localStatus, pEntry->ui32NextExpectedInSeqNum,
                                    SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum));
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send ACK packet to answer a received FIN packet; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        pEntry->unlock();
                        return 0;
                    }
                    pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                    pEntry->i64LocalActionTime = packetTime;

                    // Check if we can send a CloseTCPConnection packet right away
                    if (pEntry->isOutgoingBufferEmpty()) {
                        // Outgoing buffer is empty --> sending a CloseConnection request and moving remote status to Disconnected
                        if (0 != (rc = PacketRouter::flushAndSendCloseConnectionRequest (pEntry))) {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                            "L%hu-R%hu: failed to flush and send a CloseConnection response to remote proxy; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            return 0;
                        }
                        if (((pEntry->localStatus == TCTLS_SYN_RCVD) || (pEntry->localStatus == TCTLS_ESTABLISHED)) &&
                            (pEntry->remoteStatus == TCTRS_ConnEstablished)) {
                            pEntry->remoteStatus = TCTRS_DisconnRequestSent;
                        }
                        else if (((pEntry->localStatus == TCTLS_FIN_WAIT_1) || (pEntry->localStatus == TCTLS_FIN_WAIT_2)) &&
                            (pEntry->remoteStatus == TCTRS_DisconnRequestReceived)) {
                            pEntry->remoteStatus = TCTRS_Disconnected;
                            pEntry->resetConnectors();
                        }
                        else {
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                            "L%hu-R%hu: local connection status (%d) and remote connection status (%d) are out of sync; "
                                            "impossible to handle received FIN packet correctly; resetting the TCP connection",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus, pEntry->remoteStatus);
                            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            return 0;
                        }
                        pEntry->i64RemoteActionTime = packetTime;
                    }

                    // All previous packets have been received --> local status can be changed
                    if ((pEntry->localStatus == TCTLS_SYN_RCVD) || (pEntry->localStatus == TCTLS_ESTABLISHED)) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                        "L%hu-R%hu: received FIN+ACK while in status %d, moved to CLOSE_WAIT and sent ACK %u (relative %u) with sequence number %u\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus, pEntry->ui32NextExpectedInSeqNum,
                                        SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum), pEntry->ui32OutSeqNum);
                        pEntry->localStatus = TCTLS_CLOSE_WAIT;
                    }
                    else if (pEntry->localStatus == TCTLS_FIN_WAIT_1) {
                        if (pTCPHeader->ui32AckNum == pEntry->ui32OutSeqNum) {
                            // FIN_WAIT_1 ACKed --> FIN_WAIT_2; received a FIN --> TIME_WAIT
                            pEntry->localStatus = TCTLS_TIME_WAIT;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                            "L%hu-R%hu: received an updated FIN+ACK while in status FIN_WAIT_1, moved to TIME_WAIT and sent ACK %u (relative %u) with sequence number %u\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                            (pEntry->ui32NextExpectedInSeqNum - pEntry->ui32StartingInSeqNum), pEntry->ui32OutSeqNum);
                        }
                        else {
                            // Simultaneous Close Sequence
                            pEntry->localStatus = TCTLS_CLOSING;
                            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_LowDetailDebug,
                                            "L%hu-R%hu: received a FIN+ACK while in FIN_WAIT_1 - moved to CLOSING (Simultaneous Close Sequence)\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID);
                        }
                    }
                    else if (pEntry->localStatus == TCTLS_FIN_WAIT_2) {
                        // Moving to TIME_WAIT
                        pEntry->localStatus = TCTLS_TIME_WAIT;
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Info,
                                        "L%hu-R%hu: received FIN+ACK while in status FIN_WAIT_2, moved to TIME_WAIT and sent ACK %u (relative %u) with sequence number %u\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                                        SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                                        pEntry->ui32OutSeqNum);
                    }
                }
                else {
                    // Enqueueing received out-of-order FIN packet for later processing
                    if ((rc = pEntry->insertTCPSegmentIntoOutgoingBuffer (new TCPSegment (pTCPHeader->ui32SeqNum + ui16DataSize, 0, NULL,
                                                                            TCPHeader::TCPF_FIN | TCPHeader::TCPF_ACK))) < 0) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                        "L%hu-R%hu: failed to enqueue an out-of-order FIN packet received from local host; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        pEntry->unlock();
                        return 0;
                    }

                    // Sending a DUP ACK to trigger fast retransmission
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                        "L%hu-R%hu: failed to ACK a FIN packet containing %hu bytes of data; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, ui16DataSize, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->reset();
                        pEntry->unlock();
                        return 0;
                    }

                    if (ui16DataSize > 0) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: received an out-of-order FIN packet (local status is still %d, remote status is %d); "
                                        "%hu bytes of data with the FIN flag have been enqueued for later processing\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus,
                                        pEntry->remoteStatus, ui16DataSize);
                    }
                    else {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MediumDetailDebug,
                                        "L%hu-R%hu: received an out-of-order FIN packet (local status is still %d, remote status is %d) carrying no data; "
                                        "FIN has been enqueued for later processing\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                                        pEntry->localStatus, pEntry->remoteStatus);
                    }
                }
            }
            else if ((pEntry->localStatus == TCTLS_CLOSE_WAIT) || (pEntry->localStatus == TCTLS_CLOSING) ||
                (pEntry->localStatus == TCTLS_LAST_ACK) || (pEntry->localStatus == TCTLS_TIME_WAIT)) {
                // Duplicate FIN+ACK packet
                pEntry->i64LocalActionTime = packetTime;
                if (pEntry->localStatus == TCTLS_TIME_WAIT) {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a duplicate FIN+ACK (already in status TCTLS_TIME_WAIT); ignoring\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID);
                }
                else {
                    checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a duplicate FIN+ACK (already in status %d); sending back an updated ACK\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus);
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_ACK, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                                        "L%hu-R%hu: failed to send ACK packet to answer a duplicate FIN+ACK packet; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                        PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                        pEntry->clear();
                        pEntry->unlock();
                        return 0;
                    }
                }
            }
        }

        bool isRTTDataReady = pEntry->isOutgoingDataReady() && ((pEntry->remoteStatus == TCTRS_ConnEstablished) || (pEntry->remoteStatus == TCTRS_DisconnRequestReceived));
        bool isLTTDataReady = pEntry->outBuf.size() > 0 && ((pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_CLOSE_WAIT));
        bool rttShouldWork = isRTTDataReady &&          // There is data ready in the buffer and...
                            // ... EITHER some data and a PSH flag have been received...
                            (((ui16DataSize > 0) && (pTCPHeader->ui8Flags & TCPHeader::TCPF_PSH)) ||
                            // ... OR we have just received a FIN+ACK from localhost
                            ((pTCPHeader->ui8Flags & (TCPHeader::TCPF_FIN | TCPHeader::TCPF_ACK)) == (TCPHeader::TCPF_FIN | TCPHeader::TCPF_ACK)));
        bool lttShouldWork = (pEntry->ui16ReceiverWindowSize > oldWinRecSize) && isLTTDataReady;
        pEntry->unlock();

        if (rttShouldWork || (!lttShouldWork && isRTTDataReady)) {
            if (_pPacketRouter->_mRemoteTCPTransmitter.tryLock() == Mutex::RC_Ok) {
                _pPacketRouter->_cvRemoteTCPTransmitter.notify();
                _pPacketRouter->_mRemoteTCPTransmitter.unlock();
            }
        }
        else if (isLTTDataReady) {
            if (_pPacketRouter->_mLocalTCPTransmitter.tryLock() == Mutex::RC_Ok) {
                _pPacketRouter->_cvLocalTCPTransmitter.notify();
                _pPacketRouter->_mLocalTCPTransmitter.unlock();
            }
        }

        return 0;
    }

    int TCPManager::sendTCPPacketToHost (Entry * const pEntry, uint8 ui8TCPFlags, uint32 ui32SeqNum, const uint8 * const pui8Payload, uint16 ui16PayloadLen)
    {
        static PacketBufferManager * const pPBM (PacketBufferManager::getPacketBufferManagerInstance());
        static const uint16 ui16TCPHeaderLen = sizeof (TCPHeader);

        int rc;
        uint8* pui8Packet = (uint8*) pPBM->getAndLockWriteBuf();
        const uint16 ui16PacketLen = sizeof (IPHeader) + ui16TCPHeaderLen + ui16PayloadLen;
        if (ui16PacketLen > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("TCPManager::sendTCPPacketToHost", Logger::L_MildError,
                            "L%hu-R%hu: packet length with data (%hu - ack %u) exceeds maximum packet size (%hu)\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, ui16PacketLen,
                            pEntry->ui32NextExpectedInSeqNum, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            if (0 != pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("TCPManager::sendTCPPacketToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }

        IPHeader *pIPHeader = (IPHeader*) (pui8Packet + sizeof (EtherFrameHeader));
        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof (IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16TLen = sizeof (IPHeader) + ui16TCPHeaderLen + ui16PayloadLen;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui16FlagsAndFragOff = 0;
        pIPHeader->ui8TTL = 8;
        pIPHeader->ui8Proto = IP_PROTO_TCP;
        pIPHeader->srcAddr.ui32Addr = EndianHelper::ntohl (pEntry->ui32RemoteIP);
        pIPHeader->destAddr.ui32Addr = EndianHelper::ntohl (pEntry->ui32LocalIP);
        pIPHeader->computeChecksum();

        TCPHeader *pTCPHeader = (TCPHeader*) ((uint8*) pIPHeader + sizeof (IPHeader));
        pTCPHeader->ui16SPort = pEntry->ui16RemotePort;
        pTCPHeader->ui16DPort = pEntry->ui16LocalPort;
        pTCPHeader->ui32SeqNum = ui32SeqNum;
        pTCPHeader->ui32AckNum = (ui8TCPFlags & TCPHeader::TCPF_ACK) ? pEntry->ui32NextExpectedInSeqNum : 0;
        pTCPHeader->ui8Offset = ((ui16TCPHeaderLen / 4) << 4);
        pTCPHeader->ui8Flags = ui8TCPFlags;
        pTCPHeader->ui16Window = pEntry->getOutgoingBufferRemainingSpace();
        pTCPHeader->ui16Urgent = 0;
        memcpy ((uint8*) pTCPHeader + ui16TCPHeaderLen, pui8Payload, ui16PayloadLen);
        TCPHeader::computeChecksum ((uint8*) pIPHeader);
        checkAndLogMsg ("TCPManager::sendTCPPacketToHost", Logger::L_MediumDetailDebug,
                        "L%hu-R%hu: sending a packet with %hu bytes of data with sequence number %u (relative %u) and ack %u (relative %u) to local host %s:%hu\n",
                        pEntry->ui16ID, pEntry->ui16RemoteID, ui16PayloadLen, ui32SeqNum, SequentialArithmetic::delta (ui32SeqNum, pEntry->ui32StartingOutSeqNum),
                        pEntry->ui32NextExpectedInSeqNum, SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                        InetAddr(pEntry->ui32LocalIP).getIPAsString(), pTCPHeader->ui16DPort);

        if (0 != (rc = _pPacketRouter->wrapEtherAndSendPacketToHost (PacketRouter::getInternalNetworkInterface(), pui8Packet, ui16PacketLen))) {
            checkAndLogMsg ("TCPManager::sendTCPPacketToHost", Logger::L_SevereError,
                            "L%hu-R%hu: wrapEtherAndSendPacketToHost() failed with rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            if (0 != pPBM->findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("TCPManager::sendTCPPacketToHost", Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }

        if (0 != pPBM->findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("TCPManager::sendTCPPacketToHost", Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -3;
        }

        return 0;
    }

    int TCPManager::openTCPConnectionToHost (uint32 ui32RemoteProxyIP, uint32 ui32RemoteProxyUniqueID, uint16 ui16RemoteID, uint32 ui32LocalHostIP, uint16 ui16LocalPort,
                                             uint32 ui32RemoteHostIP, uint16 ui16RemotePort, uint8 ui8CompressionTypeAndLevel)
    {
        int rc;
        // First check if we can find the proxy address for the remote IP
        const ProtocolSetting *pProtocolSetting = _pConfigurationManager->mapAddrToProtocol (ui32LocalHostIP, ui16LocalPort, ui32RemoteHostIP, ui16RemotePort, IP_PROTO_TCP);
        ProxyMessage::Protocol currentProtocol = pProtocolSetting ? pProtocolSetting->getProxyMessageProtocol() : ProtocolSetting::DEFAULT_TCP_MAPPING_PROTOCOL;
        ConnectorType connectorType = ProtocolSetting::protocolToConnectorType (currentProtocol);
        if (!pProtocolSetting) {
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_Warning,
                            "the protocol specification for this address was not found; using default protocol (id = %d)\n",
                            ProtocolSetting::DEFAULT_TCP_MAPPING_PROTOCOL);
        }

        QueryResult query (_pConnectionManager->queryConnectionToRemoteHostForConnectorType (connectorType, ui32RemoteHostIP, ui16RemotePort));
        const InetAddr * const pRemoteProxyAddr = query.getBestConnectionSolution();
        if (!pRemoteProxyAddr) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                            "Impossible to find an entry in the Address Mapping Table that matches destination address %s:%hu for the Connector %s\n",
                            InetAddr (ui32RemoteHostIP).getIPAsString(), ui16RemotePort, connectorTypeToString (connectorType));
            return -1;
        }

        register Entry *pEntry = _pTCPConnTable->getEntry (ui32LocalHostIP, ui16LocalPort, ui32RemoteHostIP, ui16RemotePort);
        pEntry->lock();
        pEntry->ui32RemoteProxyUniqueID = ui32RemoteProxyUniqueID;
        pEntry->remoteProxyAddr = *pRemoteProxyAddr;
        pEntry->remoteStatus = TCTRS_ConnRequested;
        pEntry->ui16RemoteID = ui16RemoteID;
        pEntry->i64RemoteActionTime = getTimeInMilliseconds();
        pEntry->setProtocol (currentProtocol);
        pEntry->setMocketConfFile (_pConnectionManager->getMocketsConfigFileForConnectionsToRemoteHost (ui32RemoteHostIP, ui16RemotePort));
        pEntry->prepareNewConnection();

        pEntry->setConnector (_pConnectionManager->getConnectorForType (connectorType));
        if (!pEntry->getConnector()) {
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                            "L%hu-R%hu: could not retrieve the Connector object to the remote proxy with address %s:%hu for protocol %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort(), pEntry->getProtocol());
            pEntry->reset();
            pEntry->unlock();
            return -2;
        }

        Connection *pConnection = query.getActiveConnectionToRemoteProxy();
        pConnection = (pConnection && (pConnection->isConnected() || pConnection->isConnecting())) ?
                       pConnection : pEntry->getConnector()->openNewConnectionToRemoteProxy (query, false);
        pConnection = pConnection ? pConnection : pEntry->getConnector()->getAvailableConnectionToRemoteProxy (&pEntry->remoteProxyAddr);       // Set connection even if not yet connected
        if (!pConnection || (pConnection->hasFailedConnecting())) {
            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_SevereError,
                                "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            checkAndLogMsg ("TCPManager::handleTCPPacketFromHost", Logger::L_MildError,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage, but it was impossible to create a new %sConnection "
                            "to connect the remote NetProxy at address %s in order to remap a new TCP Connection from %s:%hu to %s:%hu\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getConnector()->getConnectorTypeAsString(),
                            pRemoteProxyAddr->getIPAsString(), InetAddr (ui32RemoteHostIP).getIPAsString(), ui16RemotePort,
                            InetAddr (ui32LocalHostIP).getIPAsString(), ui16LocalPort);
            pEntry->reset();
            pEntry->unlock();
            return -3;
        }
        pEntry->setConnection (pConnection);
        if (pConnection->isConnecting()) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage; waiting to establish a connection via %s "
                            "to the remote NetProxy with address %s:%hu\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                            pEntry->getConnector()->getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort());
        }
        else if (pConnection->isConnected()) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_HighDetailDebug,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage; a connection via %s to the remote "
                            "NetProxy with address %s:%hu was established\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                            pEntry->getConnector()->getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort());
        }
        else {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_Warning,
                            "L%hu-R%hu: received an OpenTCPConnection ProxyMessage; a connection via %s to the "
                            "remote NetProxy with address %s:%hu is in status %d; resetting TCP connection\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getConnector()->getConnectorTypeAsString(),
                            pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort(),
                            pConnection->getStatus());
            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_SevereError,
                                "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            pEntry->unlock();
            return -4;
        }

        if ((pEntry->localStatus != TCTLS_Unused) && (pEntry->localStatus != TCTLS_LISTEN) &&
            (pEntry->localStatus != TCTLS_CLOSED)) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_Warning,
                            "L%hu-R%hu: connection between <%s:%hu> and <%s:%hu> is in state %d: cannot open a new connection; resetting connection\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, InetAddr (pEntry->ui32LocalIP).getIPAsString(), pEntry->ui16LocalPort,
                            InetAddr (pEntry->ui32RemoteIP).getIPAsString(), pEntry->ui16RemotePort, pEntry->localStatus);
            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_SevereError,
                                "L%hu-R%hu: failed to send RST packet; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            pEntry->unlock();
            return -5;
        }

        const CompressionSetting compressionReaderSetting (ui8CompressionTypeAndLevel);
        delete pEntry->setProperConnectorReader (ConnectorReader::inizializeConnectorReader (&compressionReaderSetting));
        if (pProtocolSetting) {
            const CompressionSetting compressionWriterSetting (pProtocolSetting->getCompressionSetting());            // Default copy constructor
            delete pEntry->setProperConnectorWriter (&compressionWriterSetting);
        }
        else {
            const CompressionSetting compressionWriterSetting (ProxyMessage::PMC_UncompressedData, CompressionSetting::NO_COMPRESSION_LEVEL);
            delete pEntry->setProperConnectorWriter (&compressionWriterSetting);
        }

        // Enqueue and then send a SYN packet to the host to open the connection
        ReceivedData *pData = new ReceivedData (pEntry->ui32OutSeqNum, 0, NULL, TCPHeader::TCPF_SYN);
        if (0 != (rc = pEntry->outBuf.enqueue (pData))) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                            "L%hu-R%hu: failed to enqueue SYN packet to outBuf; rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            pEntry->unlock();
            return -6;
        }
        checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_HighDetailDebug,
                        "L%hu-R%hu: enqueued a SYN packet with SEQ number %u into the output buffer\n",
                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32OutSeqNum);

        if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_SYN, pEntry->ui32OutSeqNum))) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                            "L%hu-R%hu: failed to send SYN packet to local host; rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            pEntry->unlock();
            return -7;
        }

        pEntry->localStatus = TCTLS_SYN_SENT;
        pEntry->i64LocalActionTime = getTimeInMilliseconds();
        pEntry->i64LastAckTime = pEntry->i64LocalActionTime;
        pData->_i64LastTransmitTime = pEntry->i64LocalActionTime;
        pEntry->ui32OutSeqNum++;
        checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MediumDetailDebug,
                        "L%hu-R%hu: sent a SYN packet to local host to open a new connection; local status moved to SYN_SENT\n",
                        pEntry->ui16ID, pEntry->ui16RemoteID);

        /*
        // Send the local ID to remote side - Asynchronous Connection establishment
        bool bReachable = _pConfigurationManager->isProxyReachableFromRemoteAddress (ui32RemoteHostIP);
        if (0 != (rc = pEntry->getConnector()->sendTCPConnectionOpenedResponse (&pEntry->remoteProxyAddr, ui32RemoteHostIP, pEntry->ui16ID, pEntry->ui16RemoteID,
                                                                                ui32LocalVirtualIP, pEntry->getConnectorWriter()->getCompressionSetting(),
                                                                                pEntry->getMocketConfFile(), pEntry->getProtocol(), bReachable))) {
            checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                            "L%hu-R%hu: could not send connection opened response to remote proxy %s; rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID,
                            pEntry->remoteProxyAddr.getIPAsString(), rc);
            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MildError,
                                "L%hu-R%hu: failed to send RST packet to local host; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                pEntry->clear();
            }
            else {
                checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_LowDetailDebug,
                                "L%hu-R%hu: sent an RST packet to local host because of an error in the remote connection with proxy %s\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteProxyAddr.getIPAsString());
                pEntry->reset();
            }
            pEntry->unlock();
            return -7;
        }
        checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_Info,
                        "L%hu-R%hu: sent a TCPConnectionOpenedResponse for the connection from %s:%hu to %s:%hu to proxy at address %s; "
                        "data will be coded to %s:%hhd and decoded from %s:%hhd\n",
                        pEntry->ui16ID, pEntry->ui16RemoteID, InetAddr(pEntry->ui32LocalIP).getIPAsString(), pEntry->ui16LocalPort,
                        InetAddr(pEntry->ui32RemoteIP).getIPAsString(), pEntry->ui16RemotePort, pEntry->remoteProxyAddr.getIPAsString(),
                        pEntry->getConnectorWriter()->getCompressionName(), pEntry->getConnectorWriter()->getCompressionLevel(),
                        pEntry->getConnectorReader()->getCompressionName(), pEntry->getConnectorReader()->getCompressionLevel());

        pEntry->remoteStatus = TCTRS_ConnEstablished;
        checkAndLogMsg ("TCPManager::openTCPConnectionToHost", Logger::L_MediumDetailDebug,
                        "L%hu-R%hu: sent a SYN packet to local host to open a new connection; local status moved to SYN_SENT\n",
                        pEntry->ui16ID, pEntry->ui16RemoteID);
        */

        pEntry->unlock();
        return 0;
    }

    int TCPManager::tcpConnectionToHostOpened (uint16 ui16LocalID, uint16 ui16RemoteID, uint32 ui32RemoteProxyUniqueID, uint8 ui8CompressionTypeAndLevel)
    {
        int rc;
        Entry *pEntry = _pTCPConnTable->getEntry (ui16LocalID);
        if (pEntry == NULL) {
            checkAndLogMsg ("TCPManager::tcpConnectionToHostOpened", Logger::L_Warning,
                            "no entry found in TCP Connection Table with local ID %hu; "
                            "cannot update remote status to ConnEstablished\n", ui16LocalID);
            return -1;
        }

        pEntry->lock();
        if (pEntry->remoteStatus != TCTRS_ConnRequested) {
            checkAndLogMsg ("TCPManager::tcpConnectionToHostOpened", Logger::L_MildError,
                            "L%hu-R%hu: state of remote connection is not ConnRequested, but %d; resetting connection\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID,pEntry->remoteStatus);
            pEntry->ui16ID = ui16LocalID;
            pEntry->ui16RemoteID = ui16RemoteID;
            const CompressionSetting * pCompressionSetting = new CompressionSetting (ProxyMessage::PMC_UncompressedData, CompressionSetting::NO_COMPRESSION_LEVEL);
            delete pEntry->setProperConnectorWriter (pCompressionSetting);
            delete pCompressionSetting;
            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            pEntry->unlock();
            return -2;
        }
        pEntry->ui16RemoteID = ui16RemoteID;
        pEntry->ui32RemoteProxyUniqueID = ui32RemoteProxyUniqueID;
        delete pEntry->setProperConnectorReader (ConnectorReader::inizializeConnectorReader (new CompressionSetting (ui8CompressionTypeAndLevel)));
        pEntry->remoteStatus = TCTRS_ConnEstablished;
        pEntry->i64RemoteActionTime = getTimeInMilliseconds();

        if (0 != (rc = sendTCPPacketToHost (pEntry, (TCPHeader::TCPF_SYN | TCPHeader::TCPF_ACK), pEntry->ui32OutSeqNum))) {
            checkAndLogMsg ("TCPManager::tcpConnectionToHostOpened", Logger::L_SevereError,
                            "L%hu-R%hu: failed to send SYN+ACK packet after the remote NetProxy has opened the TCP connection on its side; rc = %d\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
            pEntry->reset();
            pEntry->unlock();
            return -3;
        }
        pEntry->ui32OutSeqNum++;
        pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
        checkAndLogMsg ("TCPManager::tcpConnectionToHostOpened", Logger::L_LowDetailDebug,
                        "L%hu-R%hu: sent SYN+ACK packet with ACK %u (relative %u) and SEQ num %u (relative %u)\n",
                        pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32NextExpectedInSeqNum,
                        SequentialArithmetic::delta (pEntry->ui32NextExpectedInSeqNum, pEntry->ui32StartingInSeqNum),
                        pEntry->ui32OutSeqNum - 1, SequentialArithmetic::delta (pEntry->ui32OutSeqNum - 1, pEntry->ui32StartingOutSeqNum));

        checkAndLogMsg ("TCPManager::tcpConnectionToHostOpened", Logger::L_Info,
                        "L%hu-R%hu: received confirmation of an ESTABLISHED connection from %s:%hu to %s:%hu from proxy at address %s:%hu; "
                        "data will be coded to %s:%hhd and decoded from %s:%hhd\n", pEntry->ui16ID, pEntry->ui16RemoteID,
                        InetAddr(pEntry->ui32LocalIP).getIPAsString(), pEntry->ui16LocalPort, InetAddr(pEntry->ui32RemoteIP).getIPAsString(),
                        pEntry->ui16RemotePort, pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort(),
                        pEntry->getConnectorWriter()->getCompressionName(), pEntry->getConnectorWriter()->getCompressionLevel(),
                        pEntry->getConnectorReader()->getCompressionName(), pEntry->getConnectorReader()->getCompressionLevel());
        /*
        if (pEntry->isOutgoingDataReady()) {
            pEntry->unlock();
            if (_pPacketRouter->_mRemoteTCPTransmitter.tryLock() == Mutex::RC_Ok) {
                _pPacketRouter->_cvRemoteTCPTransmitter.notify();
                _pPacketRouter->_mRemoteTCPTransmitter.unlock();
            }
        }
        else {
            pEntry->unlock();
        }
        */
        pEntry->unlock();
        return 0;
    }

    int TCPManager::sendTCPDataToHost (uint16 ui16LocalID, uint16 ui16RemoteID, const uint8 * const pui8CompData, uint16 ui16CompDataLen, uint8 ui8Flags)
    {
        static const uint16 MAX_SEGMENT_LENGTH = NetProxyApplicationParameters::GATEWAY_MODE ?
            NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF - static_cast<uint16> (sizeof (EtherFrameHeader) + sizeof (IPHeader) + sizeof (TCPHeader)) :
            NetProxyApplicationParameters::TAP_INTERFACE_MTU - static_cast<uint16> (sizeof (IPHeader) + sizeof (TCPHeader));

        int rc;
        Entry *pEntry = _pTCPConnTable->getEntry (ui16LocalID, ui16RemoteID);
        if (pEntry == NULL) {
            checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_Warning,
                            "no entry found in TCP Connection Table with local ID %hu and remote ID %hu; "
                            "impossible to send data to local host\n", ui16LocalID, ui16RemoteID);
            return -1;
        }

        pEntry->lock();
        bool isPSHFlagSet = false;
        if ((pEntry->localStatus == TCTLS_SYN_SENT) || (pEntry->localStatus == TCTLS_SYN_RCVD) ||
            (pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_CLOSE_WAIT)) {
            // Correct outgoing SEQ number depends on the status of the local connection and on its buffer
            uint32 ui32PcktOutSeqNum = (pEntry->outBuf.isEmpty() || (pEntry->localStatus == TCTLS_SYN_SENT) || (pEntry->localStatus == TCTLS_SYN_RCVD)) ?
                pEntry->ui32OutSeqNum : (pEntry->outBuf.peekTail()->getSequenceNumber() + pEntry->outBuf.peekTail()->getItemLength());
            if (pEntry->getConnectorReader()) {
                uint8 *pui8Data[1];
                uint32 ui32DataLen;
                *pui8Data = NULL;
                // Receives the packet (through the decompressor, if there is any)
                if (0 != (rc = pEntry->getConnectorReader()->receiveTCPDataProxyMessage (pui8CompData, ui16CompDataLen, pui8Data, ui32DataLen))) {
                    checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_MildError,
                                    "L%hu-R%hu: receiveTCPDataProxyMessage() failed with rc = %d; resetting connection\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                        checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_SevereError,
                                        "L%hu-R%hu: failed to send RST packet to local host; rc = %d\n",
                                        pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    }
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    pEntry->unlock();
                    return -2;
                }
                else {
                    // To fix problem caused by long inactivity times
                    if ((pEntry->outBuf.size() == 0) && (pEntry->getOutgoingTotalBytesCount() == 0)) {
                        pEntry->i64LastAckTime = getTimeInMilliseconds();
                    }

                    uint32 ui32SentData = 0;
                    ReceivedData *pTailPacket = pEntry->outBuf.peekTail();

                    // Check if we can append data to the last packet
                    if (pTailPacket && pTailPacket->canAppendData() && (pTailPacket->getItemLength() < MAX_SEGMENT_LENGTH)) {
                        uint32 ui32BytesToEnqueue = std::min (ui32DataLen, MAX_SEGMENT_LENGTH - pTailPacket->getItemLength());
                        if (0 < (rc = pTailPacket->appendDataToBuffer (*pui8Data, ui32BytesToEnqueue))) {
                            ui32SentData = ui32BytesToEnqueue;
                            if (ui32BytesToEnqueue >= ui32DataLen) {
                                pTailPacket->addTCPFlags (ui8Flags);
                            }
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: appended %u bytes to the segment of TCP packet with SEQ number %u; "
                                            "there are now %u bytes of total data and FLAGs are %hhu\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, ui32BytesToEnqueue,
                                            pTailPacket->getSequenceNumber(), pTailPacket->getItemLength(), pTailPacket->getTCPFlags());
                        }
                        else {
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_Warning,
                                            "L%hu-R%hu: error trying to append %u bytes to the segment of TCP packet with SEQ number %u; rc = %d\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, ui32BytesToEnqueue,
                                            pTailPacket->getSequenceNumber(), rc);
                        }
                    }

                    // Create new packets to store remaining received data
                    while (ui32SentData < ui32DataLen) {
                        uint32 ui32BytesToEnqueue = std::min (ui32DataLen - ui32SentData, (uint32) MAX_SEGMENT_LENGTH);
                        uint8 ui8FlagsToEnqueue = ((ui32SentData + ui32BytesToEnqueue) >= ui32DataLen) ? ui8Flags : TCPHeader::TCPF_ACK;
                        ReceivedData *pNewData = new ReceivedData (ui32PcktOutSeqNum + ui32SentData, ui32BytesToEnqueue,
                                                                    *pui8Data + ui32SentData, ui8FlagsToEnqueue);
                        if (0 != (rc = pEntry->outBuf.enqueue (pNewData))) {
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_Warning,
                                            "L%hu-R%hu: error trying to enqueue new packet with SEQ num %u and %hu bytes of data\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID,
                                            pNewData->getSequenceNumber(), pNewData->getItemLength());
                            if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                                checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_SevereError,
                                                "L%hu-R%hu: failed to send RST packet to local host; rc = %d\n",
                                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                            }
                            PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                            pEntry->reset();
                            pEntry->unlock();
                            return -3;
                        }
                        else {
                            ui32SentData += ui32BytesToEnqueue;
                            checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_HighDetailDebug,
                                            "L%hu-R%hu: enqueued TCP packet with SEQ number %u, %hu bytes of data (%hu "
                                            "before decompression, if any) and FLAGs %hhu into the output buffer\n",
                                            pEntry->ui16ID, pEntry->ui16RemoteID, pNewData->getSequenceNumber(),
                                            pNewData->getItemLength(), ui16CompDataLen, ui8FlagsToEnqueue);
                        }
                    }
                }
                isPSHFlagSet = (ui8Flags & TCPHeader::TCPF_PSH) != 0;
            }
            else {
                checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_MildError,
                                "L%hu-R%hu: ConnectorReader not initialized (NULL pointer); resetting connection\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID);
                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
                pEntry->unlock();
                return -4;
            }
        }
        else if (pEntry->localStatus == TCTLS_CLOSED) {
            checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: local connection CLOSED; ignoring packet from remote proxy\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID);
        }
        else {
            checkAndLogMsg ("TCPManager::sendTCPDataToHost", Logger::L_MildError,
                            "L%hu-R%hu: cannot enqueue received data while connection is in state %hu\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, (unsigned short) pEntry->localStatus);
            pEntry->unlock();
            return -5;
        }

        if (isPSHFlagSet && ((pEntry->localStatus == TCTLS_ESTABLISHED) ||
            (pEntry->localStatus == TCTLS_CLOSE_WAIT))) {
            pEntry->unlock();
            if (_pPacketRouter->_mLocalTCPTransmitter.tryLock() == Mutex::RC_Ok) {
                _pPacketRouter->_cvLocalTCPTransmitter.notify();
                _pPacketRouter->_mLocalTCPTransmitter.unlock();
            }
        }
        else {
            pEntry->unlock();
        }

        return 0;
    }

    int TCPManager::closeTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID)
    {
        Entry *pEntry = _pTCPConnTable->getEntry (ui16LocalID, ui16RemoteID);
        if (pEntry == NULL) {
            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_Warning,
                            "no entry found in TCP Connection Table with local ID %hu and remote ID %hu; "
                            "impossible to close connection to local host\n", ui16LocalID, ui16RemoteID);
            return -1;
        }
        pEntry->lock();

        int rc;
        int64 i64CurrTime = getTimeInMilliseconds();
        if ((pEntry->remoteStatus == TCTRS_DisconnRequestReceived) || (pEntry->remoteStatus == TCTRS_Disconnected)) {
            // Duplicate request --> ignoring
            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_Warning,
                            "L%hu-R%hu: remote CloseTCPConnectionRequest had already been received or connection was reset (local state is %d, remote state is %d); "
                            "request will be ignored\n", pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus, pEntry->remoteStatus);
            pEntry->unlock();
            return 0;
        }
        else if ((pEntry->remoteStatus == TCTRS_ConnEstablished) || (pEntry->remoteStatus == TCTRS_DisconnRequestSent)) {
            if ((pEntry->remoteStatus == TCTRS_ConnEstablished) && ((pEntry->localStatus == TCTLS_ESTABLISHED) ||
                ((pEntry->localStatus == TCTLS_SYN_SENT) && (pEntry->outBuf.size() >= 1)) ||
                ((pEntry->localStatus == TCTLS_CLOSE_WAIT) && !pEntry->isRemoteConnectionFlushed()))) {
                checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: CloseConnectionRequest received when remote state was ConnEstablished; "
                                "remote state is now DisconnRequestReceived\n", pEntry->ui16ID, pEntry->ui16RemoteID);
                pEntry->remoteStatus = TCTRS_DisconnRequestReceived;
            }
            else if (pEntry->remoteStatus == TCTRS_DisconnRequestSent) {
                // Connection terminated; Incrementing NextExpectedInSEQNum to ACK received FIN (at this moment, local status would be CLOSE_WAIT)
                checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: CloseConnectionRequest received when remote state was DisconnRequestSent; remote state is now Disconnected, "
                                "local status moved to CLOSE_WAIT and outgoing ACK number has been incremented to ACK previously received FIN\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID);
                //pEntry->ui32NextExpectedInSeqNum++;
                pEntry->localStatus = TCTLS_CLOSE_WAIT;         // Next packet sent wiil ACK received FIN --> move to CLOSE_WAIT
                pEntry->remoteStatus = TCTRS_Disconnected;
                pEntry->resetConnectors();
            }
            else {
                // Error: connection with remote proxy out of sync --> resetting connections
                if (0 != (rc = sendTCPPacketToHost (pEntry, TCPHeader::TCPF_RST, pEntry->ui32OutSeqNum))) {
                    checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send RST packet as a consequence of a CloseConnection request while in status %d; rc = %d\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus, rc);
                }
                else {
                    checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_Warning,
                                    "L%hu-R%hu: could not process CloseTCPConnection request; local and remote connections are out of sync: "
                                    "local status is %d and remote status is %d; sending back a ResetTCPConnection request\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->localStatus, pEntry->remoteStatus);
                }

                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
                pEntry->unlock();
                return 0;
            }

            pEntry->i64RemoteActionTime = i64CurrTime;
            // Enqueue a zero-length ReceivedData packet, which signifies the FIN
            uint32 ui32PcktOutSeqNum = pEntry->outBuf.isEmpty() ? pEntry->ui32OutSeqNum : (pEntry->outBuf.peekTail()->getSequenceNumber() + pEntry->outBuf.peekTail()->getItemLength());
            ReceivedData *pReceivedData = new ReceivedData (ui32PcktOutSeqNum, 0, NULL, TCPHeader::TCPF_FIN | TCPHeader::TCPF_ACK);
            if (0 != (rc = pEntry->outBuf.enqueue (pReceivedData))) {
                checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_MildError,
                                "L%hu-R%hu: failed to enqueue FIN+ACK packet to outBuf; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                pEntry->reset();
                pEntry->unlock();
                return -3;
            }

            // Try to transmit the FIN packet immediately if it is the only packet in the buffer
            if (pEntry->outBuf.size() == 1) {
                if (0 != (rc = sendTCPPacketToHost (pEntry, pReceivedData->getTCPFlags(), pReceivedData->getSequenceNumber()))) {
                    checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_SevereError,
                                    "L%hu-R%hu: failed to send FIN+ACK packet; rc = %d\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, rc);
                    PacketRouter::sendRemoteResetRequestIfNeeded (pEntry);
                    pEntry->reset();
                    pEntry->unlock();
                    return -4;
                }
                pEntry->ui32OutSeqNum++;
                pEntry->ui32LastACKedSeqNum = pEntry->ui32NextExpectedInSeqNum;
                pEntry->i64LocalActionTime = i64CurrTime;
                pReceivedData->_i64LastTransmitTime = i64CurrTime;

                if (pEntry->localStatus == TCTLS_ESTABLISHED) {
                    pEntry->localStatus = TCTLS_FIN_WAIT_1;
                    checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: sent FIN, moving to FIN_WAIT_1; out sequence number is: %u\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pReceivedData->getSequenceNumber());
                }
                else if (pEntry->localStatus == TCTLS_CLOSE_WAIT) {
                    pEntry->localStatus = TCTLS_LAST_ACK;
                    checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: sent FIN while in CLOSE_WAIT, moving to LAST_ACK; out sequence number is: %u\n",
                                    pEntry->ui16ID, pEntry->ui16RemoteID, pReceivedData->getSequenceNumber());
                }
            }
        }
        else {
            checkAndLogMsg ("TCPManager::closeTCPConnectionToHost", Logger::L_Warning,
                            "L%hu-R%hu: connection in remote state %d; cannot elaborate CloseTCPConnection request\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->remoteStatus);
            pEntry->unlock();
            return -5;
        }

        if (pEntry->outBuf.size() > 1) {
            pEntry->unlock();
            _pPacketRouter->_mLocalTCPTransmitter.lock();
            _pPacketRouter->_cvLocalTCPTransmitter.notify();
            _pPacketRouter->_mLocalTCPTransmitter.unlock();
        }
        else {
            pEntry->unlock();
        }

        return 0;
    }

    int TCPManager::resetTCPConnectionToHost (uint16 ui16LocalID, uint16 ui16RemoteID)
    {
        Entry *pEntry = _pTCPConnTable->getEntry (ui16LocalID, ui16RemoteID);
        if (pEntry == NULL) {
            checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_Warning,
                            "no entry found in TCP Connection Table with local ID %d and remote ID %d; "
                            "impossible to reset connection to local host\n", ui16LocalID, ui16RemoteID);
            Connector *pConnector = _pConnectionManager->getConnectorForType (CT_UDP);
            UDPConnector *pUDPConnector = NULL;
            if (pConnector) {
                pUDPConnector = dynamic_cast<UDPConnector*> (pConnector);
                if (!pUDPConnector) {
                    checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_Warning,
                                    "L%hu-R%hu: error performing dynamic cast on connector %s to UDPConnector\n",
                                    ui16LocalID, ui16RemoteID, pConnector->getConnectorTypeAsString());
                    return -1;
                }
            }
            else {
                checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_MildError,
                                "L%hu-R%hu: impossible to retrieve connector\n",
                                ui16LocalID, ui16RemoteID);
                return -2;
            }

            unsigned int uiRemovedPacketsFromUDPQueue = pUDPConnector->removeTCPTypePacketsFromTransmissionQueue (ui16LocalID, ui16RemoteID);
            checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_LowDetailDebug,
                            "L%hu-R%hu: %u packets could be deleted from the UDP Connection queue\n",
                            ui16LocalID, ui16RemoteID, uiRemovedPacketsFromUDPQueue);
            return -3;
        }

        pEntry->lock();
        int rc;
        if ((pEntry->localStatus == TCTLS_SYN_SENT) || (pEntry->localStatus == TCTLS_SYN_RCVD) ||
            (pEntry->localStatus == TCTLS_ESTABLISHED) || (pEntry->localStatus == TCTLS_CLOSE_WAIT) ||
            (pEntry->localStatus == TCTLS_TIME_WAIT) || (pEntry->localStatus == TCTLS_FIN_WAIT_1) ||
            (pEntry->localStatus == TCTLS_FIN_WAIT_2)) {
            // Directly send the RST
            pEntry->remoteStatus = TCTRS_Disconnected;
            pEntry->i64RemoteActionTime = getTimeInMilliseconds();
            uint32 ui32OutSeqNum = (pEntry->localStatus == TCTLS_SYN_RCVD) ? 0 : pEntry->ui32OutSeqNum;
            uint8 ui8TCPFlags = (pEntry->localStatus == TCTLS_SYN_RCVD) ? (TCPHeader::TCPF_RST | TCPHeader::TCPF_ACK) : TCPHeader::TCPF_RST;
            if (0 != (rc = sendTCPPacketToHost (pEntry, ui8TCPFlags, ui32OutSeqNum))) {
                checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_SevereError,
                                "L%hu-R%hu: failed to send RST packet as a consequence of a remoteResetRequest; rc = %d\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, rc);
            }
            else {
                checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_Info,
                                "L%hu-R%hu: received a remoteResetRequest from remote host; successfully sent an RST to local host\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID);
            }
            pEntry->reset();
        }
        else if ((pEntry->localStatus == TCTLS_Unused) || (pEntry->localStatus == TCTLS_LISTEN) ||
            (pEntry->localStatus == TCTLS_TIME_WAIT) || (pEntry->localStatus == TCTLS_CLOSED)) {
            checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_MediumDetailDebug,
                            "L%hu-R%hu: connection already in status %hu; ignoring the reset\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, (unsigned short) pEntry->localStatus);
        }
        else {
            checkAndLogMsg ("TCPManager::resetTCPConnectionToHost", Logger::L_Warning,
                            "L%hu-R%hu: connection in state %hu; cannot do a reset\n",
                            pEntry->ui16ID, pEntry->ui16RemoteID, (unsigned short) pEntry->localStatus);
            pEntry->unlock();
            return -4;
        }

        pEntry->unlock();
        return 0;
    }

}
