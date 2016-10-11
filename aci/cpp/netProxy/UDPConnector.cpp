/*
 * UDPConnector.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "Logger.h"

#include "UDPConnector.h"
#include "AutoConnectionEntry.h"
#include "Connection.h"
#include "ConnectorReader.h"
#include "TCPManager.h"
#include "ConnectionManager.h"
#include "NetProxyConfigManager.h"
#include "PacketRouter.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    UDPConnector::UDPConnector (void) : Connector (CT_UDP), _i32BytesInBuffer (0)
    {
        memset (_pucInBuf, 0, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
    }

    UDPConnector::~UDPConnector (void)
    {
        requestTermination();

        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pUDPSocketAdapter) {
                _pUDPSocketAdapter->_udpConnectionThread.requestTermination();
                _pUDPSocketAdapter->shutdown (true, true);
                _pUDPSocketAdapter->close();
                _pUDPSocketAdapter->_udpConnectionThread.requestTerminationAndWait();
            }
            _mConnector.unlock();
        }

        delete _pUDPSocketAdapter;
    }

    int UDPConnector::init (uint16 ui16SocketPort)
    {
        int rc;

        if ((rc = _pUDPSocketAdapter->_pUDPSocket->init (ui16SocketPort)) < 0) {
            checkAndLogMsg ("UPDConnector::init", Logger::L_MildError,
                            "listen() on ServerSocket failed - could not initialize to use port %d; rc = %d\n",
                            (int) ui16SocketPort, rc);
            return -1;
        }
        if (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS > 0) {
            if (0 != (rc = _pUDPSocketAdapter->_pUDPSocket->setTransmitRateLimit (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS))) {
                checkAndLogMsg ("UPDConnector::init", Logger::L_Warning,
                                "setTransmitRateLimit() on UDP Socket failed with rc = %d; specified value was %u; no limit will be set\n",
                                rc, NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS);
                _pUDPSocketAdapter->_pUDPSocket->setTransmitRateLimit (0);
            }
        }

        if ((rc = _pUDPSocketAdapter->_udpConnectionThread.start()) < 0) {
            checkAndLogMsg ("UPDConnector::init", Logger::L_MildError,
                            "start() on of UDPConnection failed - could not start Thread; rc = %d\n", rc);
            return -2;
        }

        return 0;
    }

    void UDPConnector::terminateExecution (void)
    {
        requestTermination();

        if (Mutex::RC_Ok == _mConnector.lock()) {
            if (_pUDPSocketAdapter) {
                _pUDPSocketAdapter->_udpConnectionThread.requestTermination();
                _pUDPSocketAdapter->shutdown (true, true);
                _pUDPSocketAdapter->close();
            }
            _mConnector.unlock();
        }
    }

    void UDPConnector::run (void)
    {
        started();
        static Connection * const pConnection = UDPConnector::getUDPConnection();

        int rc;
        uint32 ui32BufLen;
        NOMADSUtil::InetAddr remoteProxyAddress;

        while (!terminationRequested()) {
            if ((rc = _pUDPSocketAdapter->receiveMessage (_pucInBuf, NetProxyApplicationParameters::PROXY_MESSAGE_MTU, &remoteProxyAddress)) < 0) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                    "receiveMessage() on UDPSocket failed with rc = %d\n", rc);
                    setTerminatingResultCode (-1);
                }
                break;
            }
            else if (rc == 0) {
                continue;
            }

            ProxyMessage * const pProxyMessage = (ProxyMessage * const) _pucInBuf;
            checkAndLogMsg ("UDPConnector::run", Logger::L_HighDetailDebug,
                            "received a packet of %d bytes with proxy message flag = %d from address %s\n",
                            rc, pProxyMessage->getMessageType(), remoteProxyAddress.getIPAsString());

            switch (pProxyMessage->getMessageType()) {
                case ProxyMessage::PMT_InitializeConnection:
                {
                    InitializeConnectionProxyMessage *pICPM = (InitializeConnectionProxyMessage*) _pucInBuf;
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pICPM->_ui32ProxyUniqueID, &remoteProxyAddress, pICPM->_ui16LocalMocketsServerPort,
                                                                           pICPM->_ui16LocalTCPServerPort, pICPM->_ui16LocalUDPServerPort,
                                                                           pICPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "received an InitializeConnectionProxyMessage from remote NetProxy at address %s\n",
                                    remoteProxyAddress.getIPAsString());

                    _pPacketRouter->initializeRemoteConnection (pICPM->_ui32ProxyUniqueID, pConnection->getConnectorType());
                    break;
                }

                case ProxyMessage::PMT_ConnectionInitialized:
                {
                    ConnectionInitializedProxyMessage *pCIPM = (ConnectionInitializedProxyMessage*) _pucInBuf;
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pCIPM->_ui32ProxyUniqueID, &remoteProxyAddress, pCIPM->_ui16LocalMocketsServerPort,
                                                                           pCIPM->_ui16LocalTCPServerPort, pCIPM->_ui16LocalUDPServerPort,
                                                                           pCIPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "received a ConnectionInitializedProxyMessage from remote NetProxy at address %s\n",
                                    remoteProxyAddress.getIPAsString());
                    break;
                }

                case ProxyMessage::PMT_ICMPMessage:
                {
                    ICMPProxyMessage *pICMPPM = (ICMPProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_MediumDetailDebug,
                                    "received an ICMP ProxyMessage via %s of type %hhu, code %hhu, TTL %hhu, and %hu bytes of data from "
                                    "the remote NetProxy with address %s\n", pConnection->getConnectorTypeAsString(), pICMPPM->_ui8Type,
                                    pICMPPM->_ui8Code, pICMPPM->_ui8PacketTTL, pICMPPM->getPayloadLen(), remoteProxyAddress.getIPAsString());
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pICMPPM->_ui32ProxyUniqueID, &remoteProxyAddress, pICMPPM->_ui16LocalMocketsServerPort,
                                                                           pICMPPM->_ui16LocalTCPServerPort, pICMPPM->_ui16LocalUDPServerPort,
                                                                           pICMPPM->getRemoteProxyReachability());
                    //Connector::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pICMPPM->_ui32LocalIP), pICMPPM->_ui32ProxyUniqueID);

                    if ((pICMPPM->_ui8PacketTTL == 0) || ((pICMPPM->_ui8PacketTTL == 1) && !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                        "the ICMP message contained in the ICMP ProxyMessage has reached a TTL value of %hhu "
                                        "and it needs to be discarded", pICMPPM->_ui8PacketTTL);
                        break;
                    }
                    pICMPPM->_ui8PacketTTL = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? pICMPPM->_ui8PacketTTL : pICMPPM->_ui8PacketTTL - 1;
                    if (0 != (rc = _pPacketRouter->forwardICMPMessageToHost (pICMPPM->_ui32RemoteIP, pICMPPM->_ui32LocalIP, remoteProxyAddress.getIPAddress(),
                                                                             pICMPPM->_ui8PacketTTL, static_cast<ICMPHeader::Type> (pICMPPM->_ui8Type),
                                                                             static_cast<ICMPHeader::Code_Destination_Unreachable> (pICMPPM->_ui8Code),
                                                                             pICMPPM->_ui32RoH, pICMPPM->_aui8Data, pICMPPM->getPayloadLen()))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "forwardICMPMessageToHost() failed with rc = %d\n", rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_UDPUnicastData:
                {
                    UDPUnicastDataProxyMessage *pUDPUCDPM = (UDPUnicastDataProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_HighDetailDebug,
                                    "received a UDPUnicastData ProxyMessage via %s with TTL %hhu and %hu bytes of data from the remote "
                                    "NetProxy with address %s\n", pConnection->getConnectorTypeAsString(), pUDPUCDPM->_ui8PacketTTL,
                                    pUDPUCDPM->getPayloadLen(), remoteProxyAddress.getIPAsString());

                    unsigned char *pucBuf[1];
                    //Connector::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pUDPUCDPM->_ui32LocalIP), pUDPUCDPM->_ui32ProxyUniqueID);
                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPUCDPM->_ui8CompressionTypeAndLevel);
                    if ((pUDPUCDPM->_ui8PacketTTL == 0) || ((pUDPUCDPM->_ui8PacketTTL == 1) && !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                        "the UDP datagram contained in the UDPUnicastData ProxyMessage has reached a TTL value of %hhu "
                                        "and it needs to be discarded", pUDPUCDPM->_ui8PacketTTL);
                        break;
                    }
                    pUDPUCDPM->_ui8PacketTTL = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? pUDPUCDPM->_ui8PacketTTL : pUDPUCDPM->_ui8PacketTTL - 1;
                    pConnectorReader->receiveTCPDataProxyMessage (pUDPUCDPM->_aui8Data, pUDPUCDPM->getPayloadLen(), pucBuf, ui32BufLen);
                    if (0 != (rc = _pPacketRouter->sendUDPUniCastPacketToHost (pUDPUCDPM->_ui32LocalIP, pUDPUCDPM->_ui32RemoteIP,
                                                                               pUDPUCDPM->_ui8PacketTTL, (UDPHeader*) pucBuf[0]))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    break;
                }

                case ProxyMessage::PMT_MultipleUDPDatagrams:
                {
                    MultipleUDPDatagramsProxyMessage *pMUDPDPM = (MultipleUDPDatagramsProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_HighDetailDebug,
                                    "received a MultipleUDPDatagrams ProxyMessage via %s containing %hhu UDP packets for a total of %hu bytes of data from "
                                    "the remote NetProxy with address %s\n", pConnection->getConnectorTypeAsString(), pMUDPDPM->_ui8WrappedUDPDatagramsNum,
                                    pMUDPDPM->getPayloadLen(), pConnection->getRemoteProxyInetAddr()->getIPAsString());

                    unsigned char *pucBuf[1];
                    const UDPHeader *pUDPHeader = NULL;
                    uint16 ui16UDPDatagramOffset = 0;
                    //Connector::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pMUDPDPM->_ui32LocalIP), pMUDPDPM->_ui32ProxyUniqueID);

                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pMUDPDPM->_ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pMUDPDPM->_aui8Data, pMUDPDPM->getPayloadLen(), pucBuf, ui32BufLen);
                    for (uint8 i = 0; i < pMUDPDPM->_ui8WrappedUDPDatagramsNum; ++i) {
                        pUDPHeader = (UDPHeader*) (pucBuf[0] + pMUDPDPM->_ui8WrappedUDPDatagramsNum + ui16UDPDatagramOffset);
                        ui16UDPDatagramOffset += pUDPHeader->ui16Len;

                        if ((pucBuf[0][i] == 0) || ((pucBuf[0][i] == 1) && !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
                            checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                            "the UDP datagram with index %hhu contained in the MultipleUDPDatagrams ProxyMessage has reached a "
                                            "TTL value of %hhu and it needs to be discarded", i, pucBuf[0][i]);
                            continue;
                        }
                        pucBuf[0][i] = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? pucBuf[0][i] : pucBuf[0][i] - 1;

                        if (0 != (rc = _pPacketRouter->sendUDPUniCastPacketToHost (pMUDPDPM->_ui32LocalIP, pMUDPDPM->_ui32RemoteIP, pucBuf[0][i], pUDPHeader))) {
                            checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                            "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
                        }
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    checkAndLogMsg ("UDPConnector::run", Logger::L_MediumDetailDebug,
                                    "%hhu UDP datagrams, for a total of %u bytes, sent to local host\n",
                                    pMUDPDPM->_ui8WrappedUDPDatagramsNum, ui32BufLen);
                    break;
                }

                case ProxyMessage::PMT_UDPBCastMCastData:
                {
                    unsigned char *pucBuf[1];
                    UDPBCastMCastDataProxyMessage *pUDPBCMCDPM = (UDPBCastMCastDataProxyMessage*) _pucInBuf;
                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPBCMCDPM->_ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pUDPBCMCDPM->_aui8Data, pUDPBCMCDPM->getPayloadLen(), pucBuf, ui32BufLen);
                    if (0 != (rc = _pPacketRouter->sendUDPBCastMCastPacketToHost (pucBuf[0], ui32BufLen))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "sendUDPBCastMCastPacketToHost() failed with rc = %d\n", rc);
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    break;
                }

                case ProxyMessage::PMT_TCPOpenConnection:
                {
                    OpenConnectionProxyMessage *pOCPM = (OpenConnectionProxyMessage*) _pucInBuf;
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pOCPM->_ui32ProxyUniqueID, &remoteProxyAddress, pOCPM->_ui16LocalMocketsServerPort,
                                                                           pOCPM->_ui16LocalTCPServerPort, pOCPM->_ui16LocalUDPServerPort,
                                                                           pOCPM->getRemoteProxyReachability());
                    //Connector::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pOCPM->_ui32LocalIP, pOCPM->_ui16LocalPort), pOCPM->_ui32ProxyUniqueID);

                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L0-R%hu: received an OpenTCPConnection request from remote proxy; compression type code is %hhu and compression level is %hhu\n",
                                    pOCPM->_ui16LocalID, pOCPM->_ui8CompressionTypeAndLevel & COMPRESSION_TYPE_FLAGS_MASK,
                                    pOCPM->_ui8CompressionTypeAndLevel & COMPRESSION_LEVEL_FLAGS_MASK);

                    if (0 != (rc = TCPManager::openTCPConnectionToHost (remoteProxyAddress.getIPAddress(), pOCPM->_ui32ProxyUniqueID, pOCPM->_ui16LocalID, pOCPM->_ui32RemoteIP,
                                                                        pOCPM->_ui16RemotePort, pOCPM->_ui32LocalIP, pOCPM->_ui16LocalPort, pOCPM->_ui8CompressionTypeAndLevel))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "L0-R%hu: openTCPConnectionToHost() failed with rc = %d\n",
                                        pOCPM->_ui16LocalID, rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPConnectionOpened:
                {
                    ConnectionOpenedProxyMessage *pCOPM = (ConnectionOpenedProxyMessage*) _pucInBuf;
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pCOPM->_ui32ProxyUniqueID, &remoteProxyAddress, pCOPM->_ui16LocalMocketsServerPort,
                                                                           pCOPM->_ui16LocalTCPServerPort, pCOPM->_ui16LocalUDPServerPort,
                                                                           pCOPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a TCPConnectionOpened response from remote proxy; compression type code is %hhu and compression level is %hhu\n",
                                    pCOPM->_ui16RemoteID, pCOPM->_ui16LocalID, pCOPM->_ui8CompressionTypeAndLevel & COMPRESSION_TYPE_FLAGS_MASK,
                                    pCOPM->_ui8CompressionTypeAndLevel & COMPRESSION_LEVEL_FLAGS_MASK);

                    if (0 != (rc = TCPManager::tcpConnectionToHostOpened (pCOPM->_ui16RemoteID, pCOPM->_ui16LocalID, pCOPM->_ui32ProxyUniqueID, pCOPM->_ui8CompressionTypeAndLevel))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "L%hu-R%hu: tcpConnectionToHostOpened() failed with rc = %d\n",
                                        pCOPM->_ui16RemoteID, pCOPM->_ui16LocalID, rc);
                        if (rc == -1) {
                            // Entry not found (has proxy been reset?) --> sending back a ResetTCPConnectionRequest

                            pConnection->sendResetTCPConnectionRequest (&remoteProxyAddress, 0, pCOPM->_ui16RemoteID, pCOPM->_ui16LocalID, ProxyMessage::PMP_UDP);
                        }
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPData:
                {
                    TCPDataProxyMessage *pTCPDPM = (TCPDataProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a TCPData packet; packet has %hu data bytes and there are %d bytes in the buffer\n",
                                    pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, pTCPDPM->getPayloadLen(), _i32BytesInBuffer);
                    if (0 != (rc = TCPManager::sendTCPDataToHost (pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, pTCPDPM->_aui8Data,
                                                                     pTCPDPM->getPayloadLen(), pTCPDPM->_ui8TCPFlags))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "L%hu-R%hu: sendTCPDataToHost() failed with rc = %d\n",
                                        pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, rc);
                        if (rc == -1) {
                            // Entry not found (has proxy been reset?) --> sending back a ResetTCPConnectionRequest
                            pConnection->sendResetTCPConnectionRequest (&remoteProxyAddress, 0, pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, ProxyMessage::PMP_UDP);
                        }
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPCloseConnection:
                {
                    CloseConnectionProxyMessage *pCCPM = (CloseConnectionProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a CloseTCPConnection request from remote proxy\n",
                                    pCCPM->_ui16RemoteID, pCCPM->_ui16LocalID);
                    if (0 != (rc = TCPManager::closeTCPConnectionToHost (pCCPM->_ui16RemoteID, pCCPM->_ui16LocalID))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_Warning,
                                        "L%hu-R%hu: closeTCPConnectionToHost() failed with rc = %d\n",
                                        pCCPM->_ui16RemoteID, pCCPM->_ui16LocalID, rc);
                        if (rc == -1) {
                            // Entry not found (has proxy been reset?) --> sending back a ResetTCPConnectionRequest
                            pConnection->sendResetTCPConnectionRequest (&remoteProxyAddress, 0, pCCPM->_ui16RemoteID, pCCPM->_ui16LocalID, ProxyMessage::PMP_UDP);
                        }
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPResetConnection:
                {
                    ResetConnectionProxyMessage *pRCPM = (ResetConnectionProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a ResetTCPconnection request from remote proxy\n",
                                    pRCPM->_ui16RemoteID, pRCPM->_ui16LocalID);
                    if (0 != (rc = TCPManager::resetTCPConnectionToHost (pRCPM->_ui16RemoteID, pRCPM->_ui16LocalID))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_Warning,
                                        "resetTCPConnectionToHost() failed with rc = %d\n", rc);
                    }
                    break;
                }
            }

            // Packet here has already been discarded or processed
            _i32BytesInBuffer = 0;
        }
        checkAndLogMsg ("UDPConnector::run", Logger::L_Info,
                        "UDPConnector terminated; termination code is %d\n",
                        getTerminatingResultCode());

        Connector::_pConnectionManager->deregisterConnector (_connectorType);

        terminating();
        delete this;
    }

}
