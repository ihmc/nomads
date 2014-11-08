/*
 * UDPConnector.cpp
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
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pICPM->ui32ProxyUniqueID, &remoteProxyAddress, pICPM->ui16LocalMocketsServerPort,
                                                                           pICPM->ui16LocalTCPServerPort, pICPM->ui16LocalUDPServerPort,
                                                                           pICPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "received an InitializeConnectionProxyMessage from remote NetProxy at address %s\n",
                                    remoteProxyAddress.getIPAsString());

                    _pPacketRouter->initializeRemoteConnection (pICPM->ui32ProxyUniqueID, pConnection->getConnectorType());
                    break;
                }

                case ProxyMessage::PMT_ConnectionInitialized:
                {
                    ConnectionInitializedProxyMessage *pCIPM = (ConnectionInitializedProxyMessage*) _pucInBuf;
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pCIPM->ui32ProxyUniqueID, &remoteProxyAddress, pCIPM->ui16LocalMocketsServerPort,
                                                                           pCIPM->ui16LocalTCPServerPort, pCIPM->ui16LocalUDPServerPort,
                                                                           pCIPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "received a ConnectionInitializedProxyMessage from remote NetProxy at address %s\n",
                                    remoteProxyAddress.getIPAsString());
                    break;
                }

                case ProxyMessage::PMT_ICMPMessage:
                {
                    ICMPProxyMessage *pICMPPM = (ICMPProxyMessage*) _pucInBuf;
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pICMPPM->ui32ProxyUniqueID, &remoteProxyAddress, pICMPPM->ui16LocalMocketsServerPort,
                                                                           pICMPPM->ui16LocalTCPServerPort, pICMPPM->ui16LocalUDPServerPort,
                                                                           pICMPPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_MediumDetailDebug,
                                    "received an ICMP ProxyMessage via %s of type %hhu, code %hhu and %hu bytes of data from "
                                    "remote proxy at address %s\n", pConnection->getConnectorTypeAsString(), pICMPPM->ui8Type,
                                    pICMPPM->ui8Code, pICMPPM->ui16PayloadLen, remoteProxyAddress.getIPAsString());

                    if (0 != (rc = _pPacketRouter->forwardICMPMessageToHost (pICMPPM->ui32RemoteIP, pICMPPM->ui32LocalIP, remoteProxyAddress.getIPAddress(),
                                                                             static_cast<ICMPHeader::Type> (pICMPPM->ui8Type),
                                                                             static_cast<ICMPHeader::Code_Destination_Unreachable> (pICMPPM->ui8Code),
                                                                             pICMPPM->ui32RoH, pICMPPM->aui8Data, pICMPPM->ui16PayloadLen))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "forwardICMPMessageToHost() failed with rc = %d\n", rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_UDPUnicastData:
                {
                    UDPUnicastDataProxyMessage *pUDPUCDPM = (UDPUnicastDataProxyMessage*) _pucInBuf;
                    unsigned char *pucBuf[1];

                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPUCDPM->ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pUDPUCDPM->aui8Data, pUDPUCDPM->ui16PayloadLen, pucBuf, ui32BufLen);
                    if (0 != (rc = _pPacketRouter->sendUDPUniCastPacketToHost (pUDPUCDPM->ui32LocalIP, pUDPUCDPM->ui32RemoteIP, (UDPHeader*) pucBuf[0]))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    break;
                }

                case ProxyMessage::PMT_MultipleUDPDatagrams:
                {
                    MultipleUDPDatagramsProxyMessage *pMUDPDPM = (MultipleUDPDatagramsProxyMessage*) _pucInBuf;
                    uint16 ui16UDPDatagramOffset = 0;
                    unsigned char *pucBuf[1];
                    const UDPHeader *pUDPHeader = NULL;

                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pMUDPDPM->ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pMUDPDPM->aui8Data, pMUDPDPM->ui16PayloadLen, pucBuf, ui32BufLen);
                    for (int i = 0; i < pMUDPDPM->ui8WrappedUDPDatagramsNum; i++) {
                        pUDPHeader = (UDPHeader*) (pucBuf[0] + ui16UDPDatagramOffset);
                        if (0 != (rc = _pPacketRouter->sendUDPUniCastPacketToHost (pMUDPDPM->ui32LocalIP, pMUDPDPM->ui32RemoteIP, pUDPHeader))) {
                            checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                            "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
                        }
                        ui16UDPDatagramOffset += pUDPHeader->ui16Len;
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    checkAndLogMsg ("UDPConnector::run", Logger::L_MediumDetailDebug,
                                    "%hhu UDP datagrams, for a total of %u bytes, sent to local host\n",
                                    pMUDPDPM->ui8WrappedUDPDatagramsNum, ui32BufLen);
                    break;
                }

                case ProxyMessage::PMT_UDPBCastMCastData:
                {
                    unsigned char *pucBuf[1];
                    UDPBCastMCastDataProxyMessage *pUDPBCMCDPM = (UDPBCastMCastDataProxyMessage*) _pucInBuf;
                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPBCMCDPM->ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pUDPBCMCDPM->aui8Data, pUDPBCMCDPM->ui16PayloadLen, pucBuf, ui32BufLen);
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
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pOCPM->ui32ProxyUniqueID, &remoteProxyAddress, pOCPM->ui16LocalMocketsServerPort,
                                                                           pOCPM->ui16LocalTCPServerPort, pOCPM->ui16LocalUDPServerPort,
                                                                           pOCPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L0-R%hu: received an OpenTCPConnection request from remote proxy; compression type code is %hhu and compression level is %hhu\n",
                                    pOCPM->ui16LocalID, pOCPM->ui8CompressionTypeAndLevel & COMPRESSION_TYPE_FLAGS_MASK,
                                    pOCPM->ui8CompressionTypeAndLevel & COMPRESSION_LEVEL_FLAGS_MASK);

                    if (0 != (rc = TCPManager::openTCPConnectionToHost (remoteProxyAddress.getIPAddress(), pOCPM->ui32ProxyUniqueID, pOCPM->ui16LocalID, pOCPM->ui32RemoteIP,
                                                                        pOCPM->ui16RemotePort, pOCPM->ui32LocalIP, pOCPM->ui16LocalPort, pOCPM->ui8CompressionTypeAndLevel))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "L0-R%hu: openTCPConnectionToHost() failed with rc = %d\n",
                                        pOCPM->ui16LocalID, rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPConnectionOpened:
                {
                    ConnectionOpenedProxyMessage *pCOPM = (ConnectionOpenedProxyMessage*) _pucInBuf;
                    Connector::_pConnectionManager->updateRemoteProxyInfo (pCOPM->ui32ProxyUniqueID, &remoteProxyAddress, pCOPM->ui16LocalMocketsServerPort,
                                                                           pCOPM->ui16LocalTCPServerPort, pCOPM->ui16LocalUDPServerPort,
                                                                           pCOPM->getRemoteProxyReachability());
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a TCPConnectionOpened response from remote proxy; compression type code is %hhu and compression level is %hhu\n",
                                    pCOPM->ui16RemoteID, pCOPM->ui16LocalID, pCOPM->ui8CompressionTypeAndLevel & COMPRESSION_TYPE_FLAGS_MASK,
                                    pCOPM->ui8CompressionTypeAndLevel & COMPRESSION_LEVEL_FLAGS_MASK);

                    if (0 != (rc = TCPManager::tcpConnectionToHostOpened (pCOPM->ui16RemoteID, pCOPM->ui16LocalID, pCOPM->ui32ProxyUniqueID, pCOPM->ui8CompressionTypeAndLevel))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "L%hu-R%hu: tcpConnectionToHostOpened() failed with rc = %d\n",
                                        pCOPM->ui16RemoteID, pCOPM->ui16LocalID, rc);
                        if (rc == -1) {
                            // Entry not found (has proxy been reset?) --> sending back a ResetTCPConnectionRequest

                            pConnection->sendResetTCPConnectionRequest (&remoteProxyAddress, 0, pCOPM->ui16RemoteID, pCOPM->ui16LocalID, ProxyMessage::PMP_UDP);
                        }
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPData:
                {
                    TCPDataProxyMessage *pTCPDPM = (TCPDataProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a TCPData packet; packet has %hu data bytes and there are %d bytes in the buffer\n",
                                    pTCPDPM->ui16RemoteID, pTCPDPM->ui16LocalID, pTCPDPM->ui16PayloadLen, _i32BytesInBuffer);
                    if (0 != (rc = TCPManager::sendTCPDataToHost (pTCPDPM->ui16RemoteID, pTCPDPM->ui16LocalID, pTCPDPM->aui8Data,
                                                                     pTCPDPM->ui16PayloadLen, pTCPDPM->ui8TCPFlags))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_MildError,
                                        "L%hu-R%hu: sendTCPDataToHost() failed with rc = %d\n",
                                        pTCPDPM->ui16RemoteID, pTCPDPM->ui16LocalID, rc);
                        if (rc == -1) {
                            // Entry not found (has proxy been reset?) --> sending back a ResetTCPConnectionRequest
                            pConnection->sendResetTCPConnectionRequest (&remoteProxyAddress, 0, pTCPDPM->ui16RemoteID, pTCPDPM->ui16LocalID, ProxyMessage::PMP_UDP);
                        }
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPCloseConnection:
                {
                    CloseConnectionProxyMessage *pCCPM = (CloseConnectionProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a CloseTCPConnection request from remote proxy\n",
                                    pCCPM->ui16RemoteID, pCCPM->ui16LocalID);
                    if (0 != (rc = TCPManager::closeTCPConnectionToHost (pCCPM->ui16RemoteID, pCCPM->ui16LocalID))) {
                        checkAndLogMsg ("UDPConnector::run", Logger::L_Warning,
                                        "L%hu-R%hu: closeTCPConnectionToHost() failed with rc = %d\n",
                                        pCCPM->ui16RemoteID, pCCPM->ui16LocalID, rc);
                        if (rc == -1) {
                            // Entry not found (has proxy been reset?) --> sending back a ResetTCPConnectionRequest
                            pConnection->sendResetTCPConnectionRequest (&remoteProxyAddress, 0, pCCPM->ui16RemoteID, pCCPM->ui16LocalID, ProxyMessage::PMP_UDP);
                        }
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPResetConnection:
                {
                    ResetConnectionProxyMessage *pRCPM = (ResetConnectionProxyMessage*) _pucInBuf;
                    checkAndLogMsg ("UDPConnector::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a ResetTCPconnection request from remote proxy\n",
                                    pRCPM->ui16RemoteID, pRCPM->ui16LocalID);
                    if (0 != (rc = TCPManager::resetTCPConnectionToHost (pRCPM->ui16RemoteID, pRCPM->ui16LocalID))) {
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
