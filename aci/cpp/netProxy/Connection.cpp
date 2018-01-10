/*
 * Connection.cpp
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
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "Logger.h"

#include "Connection.h"
#include "Connector.h"
#include "ConnectorAdapter.h"
#include "ConnectorReader.h"
#include "ConnectionManager.h"
#include "AutoConnectionEntry.h"
#include "ConfigurationManager.h"
#include "TCPManager.h"
#include "PacketRouter.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    Connection::IncomingMessageHandler::~IncomingMessageHandler (void)
    {
        // Assuming Connector::_mConnectionsTable is locked
        _pConnector->lockConnectionTable();
        delete _pConnection;
        _pConnector->unlockConnectionTable();
    }

    void Connection::IncomingMessageHandler::run (void)
    {
        started();

        int rc;
        uint32 ui32MsgSize, ui32BufLen = 0;

        while (!terminationRequested()) {
            if ((rc = _pConnectorAdapter->receiveMessage (_ui8InBuf, sizeof(_ui8InBuf))) < 0) {
                if (!terminationRequested()) {
                    if (_pConnectorAdapter->isConnected()) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "receive() on %s failed with rc = %d\n",
                                        _pConnectorAdapter->getConnectorTypeAsString(), rc);
                        setTerminatingResultCode (-1);
                    }
                    else {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_Info,
                                        "connection closed by the remote host; rc = %d\n", rc);
                    }
                }
                break;
            }
            else if (rc == 0) {
                continue;
            }

            if (terminationRequested()) {
                checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_Info,
                                "received termination request; avoid processing any received packet\n");
                break;
            }

            ui32MsgSize = static_cast<uint32> (rc);
            ProxyMessage *pMsg = (ProxyMessage*) _ui8InBuf;
            switch (pMsg->getMessageType()) {
                case ProxyMessage::PMT_InitializeConnection:
                {
                    InitializeConnectionProxyMessage *pICPM = (InitializeConnectionProxyMessage*) _ui8InBuf;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                    "received an InitializeConnectionProxyMessage via %s from the remote NetProxy with address %s and ProxyUniqueID %u\n",
                                    _pConnection->getConnectorTypeAsString(), _pConnectorAdapter->getRemoteInetAddr()->getIPAsString(), pICPM->_ui32ProxyUniqueID);

                    _pConnection->updateRemoteProxyInformation (pICPM->_ui32ProxyUniqueID, pICPM->_ui16LocalMocketsServerPort, pICPM->_ui16LocalTCPServerPort,
                                                                pICPM->_ui16LocalUDPServerPort, pMsg->getRemoteProxyReachability());

                    _pPacketRouter->initializeRemoteConnection (pICPM->_ui32ProxyUniqueID, _pConnection->getConnectorType(), _pConnectorAdapter->getEncryptionType());
                    break;
                }

                case ProxyMessage::PMT_ConnectionInitialized:
                {
                    ConnectionInitializedProxyMessage *pCIPM = (ConnectionInitializedProxyMessage*) _ui8InBuf;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                    "received a ConnectionInitializedProxyMessage via %s from the remote NetProxy with address %s and ProxyUniqueID %u\n",
                                    _pConnection->getConnectorTypeAsString(), _pConnectorAdapter->getRemoteInetAddr()->getIPAsString(), pCIPM->_ui32ProxyUniqueID);

                    _pConnection->updateRemoteProxyInformation (pCIPM->_ui32ProxyUniqueID, pCIPM->_ui16LocalMocketsServerPort, pCIPM->_ui16LocalTCPServerPort,
                                                                pCIPM->_ui16LocalUDPServerPort, pCIPM->getRemoteProxyReachability());
                    break;
                }

                case ProxyMessage::PMT_ICMPMessage:
                {
                    ICMPProxyMessage *pICMPPM = (ICMPProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_HighDetailDebug,
                                    "received an ICMP ProxyMessage via %s of type %hhu, code %hhu, TTL %hhu, and %hu bytes of data from "
                                    "the remote NetProxy with address %s and ProxyUniqueID %u\n", _pConnection->getConnectorTypeAsString(),
                                    pICMPPM->_ui8Type, pICMPPM->_ui8Code, pICMPPM->_ui8PacketTTL, pICMPPM->getPayloadLen(),
                                    _pConnection->getRemoteProxyInetAddr()->getIPAsString(), pICMPPM->_ui32ProxyUniqueID);
                    _pConnection->updateRemoteProxyInformation (pICMPPM->_ui32ProxyUniqueID, pICMPPM->_ui16LocalMocketsServerPort, pICMPPM->_ui16LocalTCPServerPort,
                                                                pICMPPM->_ui16LocalUDPServerPort, pICMPPM->getRemoteProxyReachability());
                    //Connection::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pICMPPM->_ui32LocalIP), pICMPPM->_ui32ProxyUniqueID);

                    if ((pICMPPM->_ui8PacketTTL == 0) || ((pICMPPM->_ui8PacketTTL == 1) && !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                        "the ICMP message contained in the ICMP ProxyMessage has reached a TTL value of %hhu "
                                        "and it needs to be discarded", pICMPPM->_ui8PacketTTL);
                        break;
                    }
                    pICMPPM->_ui8PacketTTL = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? pICMPPM->_ui8PacketTTL : pICMPPM->_ui8PacketTTL - 1;
                    if (0 != (rc = _pPacketRouter->forwardICMPMessageToHost (pICMPPM->_ui32RemoteIP, pICMPPM->_ui32LocalIP, _pConnectorAdapter->getRemoteIPAddr(),
                                                                             pICMPPM->_ui8PacketTTL, static_cast<ICMPHeader::Type> (pICMPPM->_ui8Type),
                                                                             static_cast<ICMPHeader::Code_Destination_Unreachable> (pICMPPM->_ui8Code),
                                                                             pICMPPM->_ui32RoH, pICMPPM->_aui8Data, pICMPPM->getPayloadLen()))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "forwardICMPMessageToHost() failed with rc = %d\n", rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_UDPUnicastData:
                {
                    UDPUnicastDataProxyMessage *pUDPUCDPM = (UDPUnicastDataProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_HighDetailDebug,
                                    "received a UDPUnicastData ProxyMessage via %s with TTL %hhu and %hu bytes of data from the remote NetProxy with "
                                    "address %s and ProxyUniqueID %u\n", _pConnection->getConnectorTypeAsString(), pUDPUCDPM->_ui8PacketTTL,
                                    pUDPUCDPM->getPayloadLen(), _pConnection->getRemoteProxyInetAddr()->getIPAsString(), pUDPUCDPM->_ui32ProxyUniqueID);

                    unsigned char *pucBuf[1];
                    _pConnection->updateRemoteProxyID (pUDPUCDPM->_ui32ProxyUniqueID);
                    //Connection::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pUDPUCDPM->_ui32LocalIP), pUDPUCDPM->_ui32ProxyUniqueID);
                    if ((pUDPUCDPM->_ui8PacketTTL == 0) || ((pUDPUCDPM->_ui8PacketTTL == 1) && !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                        "the UDP datagram contained in the UDPUnicastData ProxyMessage has reached a TTL value of %hhu "
                                        "and it needs to be discarded", pUDPUCDPM->_ui8PacketTTL);
                        break;
                    }
                    pUDPUCDPM->_ui8PacketTTL = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? pUDPUCDPM->_ui8PacketTTL : pUDPUCDPM->_ui8PacketTTL - 1;
                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPUCDPM->_ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pUDPUCDPM->_aui8Data, pUDPUCDPM->getPayloadLen(), pucBuf, ui32BufLen);
                    if (0 != (rc = _pPacketRouter->sendUDPUniCastPacketToHost (pUDPUCDPM->_ui32LocalIP, pUDPUCDPM->_ui32RemoteIP,
                                                                               pUDPUCDPM->_ui8PacketTTL, (UDPHeader*) pucBuf[0]))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    break;
                }

                case ProxyMessage::PMT_MultipleUDPDatagrams:
                {
                    MultipleUDPDatagramsProxyMessage *pMUDPDPM = (MultipleUDPDatagramsProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_HighDetailDebug,
                                    "received a MultipleUDPDatagrams ProxyMessage via %s containing %hhu UDP packets for a total of %hu bytes of data from the remote "
                                    "NetProxy with address %s and ProxyUniqueID %u\n", _pConnection->getConnectorTypeAsString(), pMUDPDPM->_ui8WrappedUDPDatagramsNum,
                                    pMUDPDPM->getPayloadLen(), _pConnection->getRemoteProxyInetAddr()->getIPAsString(), pMUDPDPM->_ui32ProxyUniqueID);

                    unsigned char *pucBuf[1];
                    const UDPHeader *pUDPHeader = nullptr;
                    uint16 ui16UDPDatagramOffset = 0;
                    _pConnection->updateRemoteProxyID (pMUDPDPM->_ui32ProxyUniqueID);
                    //Connection::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pMUDPDPM->_ui32LocalIP), pMUDPDPM->_ui32ProxyUniqueID);

                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pMUDPDPM->_ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pMUDPDPM->_aui8Data, pMUDPDPM->getPayloadLen(), pucBuf, ui32BufLen);
                    for (uint8 i = 0; i < pMUDPDPM->_ui8WrappedUDPDatagramsNum; ++i) {
                        pUDPHeader = (UDPHeader*) (pucBuf[0] + pMUDPDPM->_ui8WrappedUDPDatagramsNum + ui16UDPDatagramOffset);
                        ui16UDPDatagramOffset += pUDPHeader->ui16Len;

                        if ((pucBuf[0][i] == 0) || ((pucBuf[0][i] == 1) && !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
                            checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                            "the UDP datagram with index %hhu contained in the MultipleUDPDatagrams ProxyMessage has reached a "
                                            "TTL value of %hhu and it needs to be discarded", i, pucBuf[0][i]);
                            continue;
                        }
                        pucBuf[0][i] = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? pucBuf[0][i] : pucBuf[0][i] - 1;

                        if (0 != (rc = _pPacketRouter->sendUDPUniCastPacketToHost (pMUDPDPM->_ui32LocalIP, pMUDPDPM->_ui32RemoteIP, pucBuf[0][i], pUDPHeader))) {
                            checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                            "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
                        }
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MediumDetailDebug,
                                    "%hhu UDP datagrams, for a total of %u bytes, have been sent to local host\n",
                                    pMUDPDPM->_ui8WrappedUDPDatagramsNum, ui32BufLen);
                    break;
                }

                case ProxyMessage::PMT_UDPBCastMCastData:
                {
                    UDPBCastMCastDataProxyMessage *pUDPBCMCDPM = (UDPBCastMCastDataProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_HighDetailDebug,
                                    "received a UDPBroadcastMulticastData ProxyMessage via %s with %hu bytes of data from the remote NetProxy "
                                    "with address %s and ProxyUniqueID %u\n", _pConnection->getConnectorTypeAsString(), pUDPBCMCDPM->getPayloadLen(),
                                    _pConnection->getRemoteProxyInetAddr()->getIPAsString(), pUDPBCMCDPM->_ui32ProxyUniqueID);

                    _pConnection->updateRemoteProxyID (pUDPBCMCDPM->_ui32ProxyUniqueID);

                    unsigned char *pucBuf[1];
                    ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPBCMCDPM->_ui8CompressionTypeAndLevel);
                    pConnectorReader->receiveTCPDataProxyMessage (pUDPBCMCDPM->_aui8Data, pUDPBCMCDPM->getPayloadLen(), pucBuf, ui32BufLen);
                    if (0 != (rc = _pPacketRouter->sendUDPBCastMCastPacketToHost (pucBuf[0], ui32BufLen))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "sendUDPBCastMCastPacketToHost() failed with rc = %d\n", rc);
                    }
                    pConnectorReader->resetAndUnlockConnectorReader();
                    break;
                }

                case ProxyMessage::PMT_TCPOpenConnection:
                {
                    OpenConnectionProxyMessage *pOCPM = (OpenConnectionProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                    "L0-R%hu: received an OpenTCPConnection request via %s from the remote NetProxy with address %s and ProxyUniqueID %u; compression "
                                    "type code is %hhu and compression level is %hhu\n", pOCPM->_ui16LocalID, _pConnection->getConnectorTypeAsString(),
                                    _pConnection->getRemoteProxyInetAddr()->getIPAsString(), pOCPM->_ui32ProxyUniqueID,
                                    pOCPM->_ui8CompressionTypeAndLevel & COMPRESSION_TYPE_FLAGS_MASK,
                                    pOCPM->_ui8CompressionTypeAndLevel & COMPRESSION_LEVEL_FLAGS_MASK);

                    _pConnection->updateRemoteProxyInformation (pOCPM->_ui32ProxyUniqueID, pOCPM->_ui16LocalMocketsServerPort, pOCPM->_ui16LocalTCPServerPort,
                                                                pOCPM->_ui16LocalUDPServerPort, pOCPM->getRemoteProxyReachability());
                    //Connection::_pConnectionManager->updateAddressMappingBook (AddressRangeDescriptor(pOCPM->_ui32LocalIP, pOCPM->_ui16LocalPort), pOCPM->_ui32ProxyUniqueID);

                    if (0 != (rc = TCPManager::openTCPConnectionToHost (_pConnectorAdapter->getRemoteIPAddr(), pOCPM->_ui32ProxyUniqueID, pOCPM->_ui16LocalID, pOCPM->_ui32RemoteIP,
                                                                        pOCPM->_ui16RemotePort, pOCPM->_ui32LocalIP, pOCPM->_ui16LocalPort, pOCPM->_ui8CompressionTypeAndLevel))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "L0-R%hu: openTCPConnectionToHost() failed with rc = %d\n",
                                        pOCPM->_ui16LocalID, rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPConnectionOpened:
                {
                    ConnectionOpenedProxyMessage *pCOPM = (ConnectionOpenedProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a TCPConnectionOpened response via %s from the remote NetProxy with address %s and ProxyUniqueID %u; compression "
                                    "type code is %hhu and compression level is %hhu\n", pCOPM->_ui16RemoteID, pCOPM->_ui16LocalID, _pConnection->getConnectorTypeAsString(),
                                    _pConnection->getRemoteProxyInetAddr()->getIPAsString(), pCOPM->_ui32ProxyUniqueID, pCOPM->_ui8CompressionTypeAndLevel & COMPRESSION_TYPE_FLAGS_MASK,
                                    pCOPM->_ui8CompressionTypeAndLevel & COMPRESSION_LEVEL_FLAGS_MASK);

                    _pConnection->updateRemoteProxyInformation (pCOPM->_ui32ProxyUniqueID, pCOPM->_ui16LocalMocketsServerPort, pCOPM->_ui16LocalTCPServerPort,
                                                                pCOPM->_ui16LocalUDPServerPort, pCOPM->getRemoteProxyReachability());

                    if (0 != (rc = TCPManager::tcpConnectionToHostOpened (pCOPM->_ui16RemoteID, pCOPM->_ui16LocalID, pCOPM->_ui32ProxyUniqueID, pCOPM->_ui8CompressionTypeAndLevel))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "L%hu-R%hu: tcpConnectionToHostOpened() failed with rc = %d\n",
                                        pCOPM->_ui16RemoteID, pCOPM->_ui16LocalID, rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPData:
                {
                    TCPDataProxyMessage *pTCPDPM = (TCPDataProxyMessage*) pMsg;
                    if (ui32MsgSize <= sizeof(TCPDataProxyMessage)) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_Warning,
                                        "received a TCPDataProxyMessage via %s that is too short or has no data\n",
                                        _pConnection->getConnectorTypeAsString());
                        break;
                    }
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_HighDetailDebug,
                                    "L%hu-R%hu: received a TCPData ProxyMessage with %hu bytes of data\n",
                                    pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, pTCPDPM->getPayloadLen());

                    if (0 != (rc = TCPManager::sendTCPDataToHost (pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, pTCPDPM->_aui8Data,
                                                                  pTCPDPM->getPayloadLen(), pTCPDPM->_ui8TCPFlags))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "L%hu-R%hu: sendTCPDataToHost() failed with rc = %d\n",
                                        pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPCloseConnection:
                {
                    CloseConnectionProxyMessage *pCCPM = (CloseConnectionProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a CloseTCPConnection request via %s from remote proxy\n",
                                    pCCPM->_ui16RemoteID, pCCPM->_ui16LocalID, _pConnection->getConnectorTypeAsString());

                    if (0 != (rc = TCPManager::closeTCPConnectionToHost (pCCPM->_ui16RemoteID, pCCPM->_ui16LocalID))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "closeTCPConnectionToHost() failed with rc = %d\n", rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_TCPResetConnection:
                {
                    ResetConnectionProxyMessage *pRCPM = (ResetConnectionProxyMessage*) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                    "L%hu-R%hu: received a ResetTCPconnection request via %s from remote proxy\n",
                                    pRCPM->_ui16RemoteID, pRCPM->_ui16LocalID, _pConnection->getConnectorTypeAsString());

                    if (0 != (rc = TCPManager::resetTCPConnectionToHost (pRCPM->_ui16RemoteID, pRCPM->_ui16LocalID))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "resetTCPConnectionToHost() failed with rc = %d\n", rc);
                    }
                    break;
                }

                case ProxyMessage::PMT_TunnelPacket:
                {
                    TunnelPacketProxyMessage * pTPPM = (TunnelPacketProxyMessage *) pMsg;
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_LowDetailDebug,
                                    "received a TunnelPacketProxyMessage via %s from remote proxy\n",
                                    _pConnection->getConnectorTypeAsString());

                    if (0 != (rc = PacketRouter::sendTunneledPacketToHost (PacketRouter::_pInternalInterface, pTPPM->_aui8Data, pTPPM->getPayloadLen()))) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_MildError,
                                        "sendPacketToHost() failed with rc = %d\n", rc);
                    }
                    break;
                }

                default:
                {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_Warning,
                                    "received a ProxyMessage via %s of unknown type %d and %u bytes - ignoring\n",
                                    _pConnection->getConnectorTypeAsString(), (int) pMsg->getMessageType(), rc);
                }
            }
        }
        checkAndLogMsg ("Connection::IncomingMessageHandler::run", Logger::L_Info,
                        "Incoming Message Handler terminated; termination code is %d\n",
                        getTerminatingResultCode());

        terminating();
        delete this;
    }

    void Connection::BackgroundConnectionThread::run (void)
    {
        int rc;

        _pConnection->_mBCThread.lock();
        if (0 != (rc = _pConnection->doConnect())) {
            checkAndLogMsg ("MocketConnection::BackgroundConnectionThread::run", Logger::L_MildError,
                            "doConnect() failed with rc = %d\n", rc);
        }
        _pConnection->connectionThreadTerminating();
        _pConnection->_mBCThread.unlock();

        delete this;
    }

    Connection::Connection (ConnectorAdapter * const pConnectorAdapter, Connector * const pConnector) :
        _connectorType (pConnectorAdapter->getConnectorType()), _encryptionType (pConnectorAdapter->getEncryptionType()), _localHostRole (CHR_Server),
        _status ((_connectorType == CT_UDPSOCKET) ? CS_Connected : CS_NotConnected), _ui32RemoteProxyID (0), _remoteProxyInetAddr (*(pConnectorAdapter->getRemoteInetAddr())),
        _pConnectorAdapter (pConnectorAdapter), _pConnector (pConnector), _pIncomingMessageHandler (new IncomingMessageHandler (this, pConnectorAdapter, pConnector)),
        _pBCThread (nullptr), _cvBCThread (&_mBCThread), _bDeleteRequested (false)
    {
        memset (_pucMultipleUDPDatagramsBuffer, 0, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
        pConnectorAdapter->registerPeerUnreachableWarningCallback (peerUnreachableWarning, this);
    }

    Connection::Connection (ConnectorType connectorType, EncryptionType encryptionType, uint32 ui32RemoteProxyID,
                            const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, Connector * const pConnector) :
        _connectorType (connectorType), _encryptionType (encryptionType), _localHostRole (CHR_Client),
        _status ((_connectorType == CT_UDPSOCKET) ? CS_Connected : CS_NotConnected),
        _ui32RemoteProxyID (ui32RemoteProxyID), _remoteProxyInetAddr (*pRemoteProxyInetAddr),
        _pConnectorAdapter (ConnectorAdapter::ConnectorAdapterFactory (connectorType, encryptionType, pRemoteProxyInetAddr)), _pConnector (pConnector),
        _pIncomingMessageHandler ((_connectorType == CT_UDPSOCKET) ? nullptr : new IncomingMessageHandler (this, nullptr, pConnector)),
        _pBCThread (nullptr), _cvBCThread (&_mBCThread), _bDeleteRequested (false) { }

    Connection::~Connection (void)
    {
        // Assuming Connector::_mConnectionsTable is locked
        std::lock_guard<std::mutex> lg (_mConnection);

        _bDeleteRequested = true;
        setStatusAsDisconnected();
        if (getConnectorType() != CT_UDPSOCKET) {
            _pIncomingMessageHandler->requestTermination();
            if (_pConnectorAdapter) {
                _pConnectorAdapter->shutdown (true, true);
                _pConnectorAdapter->close();
                _mBCThread.lock();
                if (_pBCThread) {
                    _pBCThread->_pConnection = nullptr;
                    _pBCThread->_bDeleteConnectorAdapter = true;
                    _pConnectorAdapter = nullptr;      // This prevents the delete below to have any effect
                }
                _mBCThread.unlock();
                delete _pConnectorAdapter;
            }
            _pIncomingMessageHandler->executionTerminated();        // Avoids that ~IncomingMessageHandler invokes delete on this Connection instance

            const Connection * const pOldConnection = _pConnector->removeFromConnectionsTable (this);
            if (pOldConnection && (pOldConnection != this)) {
                checkAndLogMsg ("Connection::~Connection", Logger::L_MildError,
                                "found a different Connection instance to the remote NetProxy with address %s:%hu in the ConnectionsTable "
                                "of the %sConnector (a new instance to try and open a new connection to the remote NetProxy was created "
                                "and added to the table); this should NEVER happen\n", getRemoteProxyInetAddr()->getIPAsString(),
                                getRemoteProxyInetAddr()->getPort(), getConnectorTypeAsString());
            }
            if (!_pConnectionManager->removeActiveConnectionFromTable (this)) {
                checkAndLogMsg ("Connection::~Connection", Logger::L_LowDetailDebug,
                                "could not find any connection instance with remote NetProxy UniqueID %u and address %s:%hu in the "
                                "ActiveConnectionTable; removeActiveConnectionFromTable() failed\n", getRemoteProxyID(),
                                getRemoteProxyInetAddr()->getIPAsString(), getRemoteProxyInetAddr()->getPort());
            }
            AutoConnectionEntry * const pAutoConnectionEntry = _pConnectionManager->getAutoConnectionEntryToRemoteProxyID (getRemoteProxyID());
            if (pAutoConnectionEntry && (pAutoConnectionEntry->getConnectorType() == getConnectorType()) &&
                isEncryptionTypeInDescriptor (pAutoConnectionEntry->getConnectionEncryptionDescriptor(), getEncryptionType())) {
                pAutoConnectionEntry->resetSynch (getEncryptionType());
                checkAndLogMsg ("Connection::~Connection", Logger::L_MediumDetailDebug,
                                "successfully reset synchronization of the AutoConnectionEntry instance for the remote NetProxy with UniqueID %u and address "
                                "%s:%hu\n", getRemoteProxyID(), getRemoteProxyInetAddr()->getIPAsString(), getRemoteProxyInetAddr()->getPort());
            }
            else {
                checkAndLogMsg ("Connection::~Connection", Logger::L_HighDetailDebug,
                                "no AutoConnectionEntry instance found for the remote NetProxy with UniqueID %u and address %s:%hu\n",
                                getRemoteProxyID(), getRemoteProxyInetAddr()->getIPAsString(), getRemoteProxyInetAddr()->getPort());
            }
        }

        Entry *pEntry = _TCPEntryList.getFirst();
        while (pEntry) {
            if (pEntry->getConnection() == this) {
                pEntry->lock();
                pEntry->setConnection (nullptr);
                checkAndLogMsg ("Connection::~Connection", Logger::L_MediumDetailDebug,
                                "L%hu-R%hu: removed reference to connection instance with remote proxy at address %s from the entry in the TCPConnTable\n",
                                pEntry->ui16ID, pEntry->ui16RemoteID, getRemoteProxyInetAddr()->getIPAsString());
                pEntry->unlock();
            }
            pEntry = _TCPEntryList.getNext();
        }
        _pGUIStatsManager->deleteStatistics (_connectorType, getRemoteProxyID());

        if (_connectorType != CT_UDPSOCKET) {
            checkAndLogMsg ("Connection::~Connection", Logger::L_Info,
                            "%sConnection to remote proxy at address %s:%hu terminated\n",
                            getConnectorTypeAsString(), _remoteProxyInetAddr.getIPAsString(),
                            _remoteProxyInetAddr.getPort());
        }
    }

    int Connection::startMessageHandler (void)
    {
        if (_pIncomingMessageHandler && _pIncomingMessageHandler->isRunning()) {
            return 0;
        }

        if (!_pIncomingMessageHandler || !_pIncomingMessageHandler->isReadyToRun()) {
            return -1;
        }

        _status = CS_Connected;
        _pIncomingMessageHandler->start();

        return 0;
    }

    int Connection::connectSync (void)
    {
        _mBCThread.lock();

        if ((_status == CS_Connecting) || (_status == CS_Connected)) {
            checkAndLogMsg ("Connection::connectSync", Logger::L_MildError,
                            "cannot attempt to establish a new connection while already in status %d\n",
                            (int) _status);
            _mBCThread.unlock();
            return -1;
        }
        _status = CS_Connecting;
        int res = doConnect();

        _mBCThread.unlock();

        return res;
    }

    int Connection::connectAsync (void)
    {
        _mBCThread.lock();
        if (_pBCThread) {
            checkAndLogMsg ("Connection::connectAsync", Logger::L_MildError,
                            "background connection thread already running\n");
            _mBCThread.unlock();
            return -1;
        }
        if ((_status == CS_Connecting) || (_status == CS_Connected)) {
            checkAndLogMsg ("Connection::connectAsync", Logger::L_MildError,
                            "cannot establish connection while in %d state\n", (int) _status);
            _mBCThread.unlock();
            return -2;
        }

        _pBCThread = new BackgroundConnectionThread (this);
        _pBCThread->setName ("BackgroundConnection Thread");
        _status = CS_Connecting;
        _pBCThread->start();
        _mBCThread.unlock();

        return 0;
    }

    // Assume lock is already acquired by caller!
    int Connection::doConnect (void)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::doConnect", Logger::L_MildError,
                            "cannot establish connection: Connector not initialized\n");
            return -1;
        }

        int rc;
        const String sConfigFile (_pConnectionManager->getMocketsConfigFileForProxyWithID (getRemoteProxyID()));
        if (sConfigFile.length() > 0) {
            _pConnectorAdapter->readConfigFile (sConfigFile);
        }

        if (!_pConnectorAdapter->isConnected()) {
            InetAddr remoteProxyInetAddress (_remoteProxyInetAddr.getIPAddress(), _remoteProxyInetAddr.getPort());
            if (0 != (rc = _pConnectorAdapter->connect (remoteProxyInetAddress.getIPAsString(), remoteProxyInetAddress.getPort()))) {
                checkAndLogMsg ("Connection::doConnect", Logger::L_MildError,
                                "failed to connect to host at address %s:%hu; rc = %d\n",
                                remoteProxyInetAddress.getIPAsString(), remoteProxyInetAddress.getPort(), rc);
                if (_bDeleteRequested) {
                    // Upon return from connect(), regardless it was successful or not, check if deletion was requested
                    return 0;
                }
                _status = CS_ConnectionFailed;
                _pConnectionManager->removeActiveConnectionFromTable (this);        // Just to ensure consistency
                return -2;
            }
            if (_bDeleteRequested) {
                // Upon return from connect(), regardless it was successful or not, check if deletion was requested
                return 0;
            }
        }
        else {
            checkAndLogMsg ("Connection::doConnect", Logger::L_Warning,
                            "%s connection to remote NetProxy on host %s:%hu was found to be already established\n",
                            _pConnectorAdapter->getConnectorTypeAsString(), _remoteProxyInetAddr.getIPAsString(),
                            _remoteProxyInetAddr.getPort());
            _status = CS_Connected;
            _pConnectionManager->addNewActiveConnectionToRemoteProxy (this);
            return 0;
        }

        _status = CS_Connected;
        _pConnectorAdapter->registerPeerUnreachableWarningCallback (peerUnreachableWarning, this);
        _pIncomingMessageHandler->setConnectorAdapter (_pConnectorAdapter);
        if (0 != startMessageHandler()) {
            checkAndLogMsg ("Connection::doConnect", Logger::L_MildError,
                            "impossible to start the IncomingMessageHandler thread for opened "
                            "connection via %s to the remote NetProxy at address %s:%hu\n",
                            _pConnectorAdapter->getConnectorTypeAsString(), _remoteProxyInetAddr.getIPAsString(),
                            _remoteProxyInetAddr.getPort());
            return -3;
        }

        if (_pConnectionManager->addNewActiveConnectionToRemoteProxyIfNone (this) == this) {
            /* The remote NetProxy had already established a connection itself, and we also have already received at least one message from it
            * --> delete the new one to maintain only one connection to each NetProxy. Note that if we close the new connection without
            * sending any packets, there is no risk that the remote NetProxy will also attempt to terminate it. */
            _pIncomingMessageHandler->requestTermination();
            checkAndLogMsg ("Connection::doConnect", Logger::L_Warning,
                            "a new connection to the remote NetProxy with UniqueID %u and address %s:%hu was established via %s, but the remote "
                            "NetProxy had already established and sent data over another connection; closing the newly established connection\n",
                            getRemoteProxyID(), _remoteProxyInetAddr.getIPAsString(), _remoteProxyInetAddr.getPort(),
                            _pConnectorAdapter->getConnectorTypeAsString());
            return -4;
        }
        checkAndLogMsg ("Connection::doConnect", Logger::L_Info,
                        "established a new connection via %s to the remote NetProxy at address %s:%hu\n",
                        _pConnectorAdapter->getConnectorTypeAsString(), _remoteProxyInetAddr.getIPAsString(),
                        _remoteProxyInetAddr.getPort());
        PacketRouter::wakeUpAutoConnectionAndRemoteTransmitterThreads();

        return 0;
    }

    int Connection::waitForConnectionEstablishment (void)
    {
        if (isConnected()) {
            return 0;
        }

        // Synchronize with the BackgroundConnectionThread
        while (_status == CS_Connecting) {
            _mBCThread.lock();
            _mBCThread.unlock();
        }

        return isConnected() ? 0 : -1;
    }

    int Connection::openConnectionWithRemoteProxy (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, bool bLocalReachabilityFromRemote)
    {
        if (!_pConnectorAdapter) {
            return -1;
        }
        if (!pRemoteProxyInetAddr) {
            checkAndLogMsg ("Connection::openConnectionWithRemoteProxy", Logger::L_MildError,
                            "remoteProxyInetAddress is NULL when trying to send an InitializeConnection ProxyMessage via %s\n",
                            getConnectorTypeAsString());
            return -2;
        }
        else if (!isConnected()) {
            checkAndLogMsg ("Connection::openConnectionWithRemoteProxy", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        InitializeConnectionProxyMessage iCPM (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, NetProxyApplicationParameters::MOCKET_SERVER_PORT,
                                               NetProxyApplicationParameters::TCP_SERVER_PORT, NetProxyApplicationParameters::UDP_SERVER_PORT, bLocalReachabilityFromRemote);
        if (0 != (rc = _pConnectorAdapter->send (pRemoteProxyInetAddr, 0, true, true, &iCPM, sizeof(InitializeConnectionProxyMessage)))) {
            checkAndLogMsg ("Connection::openConnectionWithRemoteProxy", Logger::L_MildError,
                            "send() of an InitializeConnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::openConnectionWithRemoteProxy", Logger::L_HighDetailDebug,
                        "successfully sent an InitializeConnection ProxyMessage via %s to the remote NetProxy with address <%s:%hu>\n",
                        getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort());

        return 0;
    }

    int Connection::confirmOpenedConnectionWithRemoteProxy (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, bool bLocalReachabilityFromRemote)
    {
        if (!_pConnectorAdapter) {
            return -1;
        }
        if (!pRemoteProxyInetAddr) {
            checkAndLogMsg ("Connection::confirmOpenedConnectionWithRemoteProxy", Logger::L_MildError,
                            "remoteProxyInetAddress is NULL when trying to send a ConnectionInitialized ProxyMessage via %s\n",
                            getConnectorTypeAsString());
            return -2;
        }
        else if (!isConnected()) {
            checkAndLogMsg ("Connection::confirmOpenedConnectionWithRemoteProxy", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        ConnectionInitializedProxyMessage cIPM (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, NetProxyApplicationParameters::MOCKET_SERVER_PORT,
                                                NetProxyApplicationParameters::TCP_SERVER_PORT, NetProxyApplicationParameters::UDP_SERVER_PORT, bLocalReachabilityFromRemote);
        if (0 != (rc = _pConnectorAdapter->send (pRemoteProxyInetAddr, 0, true, true, &cIPM, sizeof(ConnectionInitializedProxyMessage)))) {
            checkAndLogMsg ("Connection::confirmOpenedConnectionWithRemoteProxy", Logger::L_MildError,
                            "send() of a ConnectionInitialized ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::confirmOpenedConnectionWithRemoteProxy", Logger::L_HighDetailDebug,
                        "successfully sent a ConnectionInitialized ProxyMessage via %s to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort());

        return 0;
    }

    int Connection::tunnelEthernetPacket (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32RemoteDestinationIP,
                                          const uint8 * const pui8Buf, uint16 ui16BufLen)
    {
        int rc;
        if (!_pConnectorAdapter) {
            return -1;
        }
        if (!pRemoteProxyInetAddr) {
            checkAndLogMsg ("Connection::tunnelEthernetPacket", Logger::L_MildError,
                            "remoteProxyInetAddress is NULL when trying to send a %hu bytes long packet via %s\n",
                            ui16BufLen, getConnectorTypeAsString());
            return -2;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::tunnelEthernetPacket", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), getRemoteProxyID());
            return -3;
        }

        TunnelPacketProxyMessage tPPM (ui16BufLen);
        if (0 != (rc = _pConnectorAdapter->gsend (pRemoteProxyInetAddr, ui32RemoteDestinationIP, false, false, &tPPM,
                                                  sizeof(TunnelPacketProxyMessage), pui8Buf, tPPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::tunnelEthernetPacket", Logger::L_MildError,
                            "gsend() of an TunnelPacketProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::tunnelEthernetPacket", Logger::L_HighDetailDebug,
                        "successfully sent a TunnelPacketProxyMessage via %s addressed to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort());

        return 0;
    }

    int Connection::sendICMPProxyMessageToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint8 ui8Type, uint8 ui8Code, uint32 ui32RoH,
                                                      uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, uint8 ui8PacketTTL, const uint8 * const pui8Buf,
                                                      uint16 ui16BufLen, ProxyMessage::Protocol ui8PMProtocol, bool bLocalReachabilityFromRemote)
    {
        if (!_pConnectorAdapter) {
            return -1;
        }
        if (!pRemoteProxyInetAddr) {
            checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", Logger::L_MildError,
                            "remoteProxyInetAddress is NULL when trying to send an ICMP ProxyMessage "
                            "with type %hhu, code %hhu via %s\n", ui8Type, ui8Code,
                            getConnectorTypeAsString());
            return -2;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isCommunicationReliable (ui8PMProtocol);
        bool bSequenced = isCommunicationSequenced (ui8PMProtocol);
        ICMPProxyMessage iCMPPM (ui16BufLen, ui8Type, ui8Code, ui32RoH, ui32LocalSourceIP, ui32RemoteDestinationIP, NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                 NetProxyApplicationParameters::MOCKET_SERVER_PORT, NetProxyApplicationParameters::TCP_SERVER_PORT,
                                 NetProxyApplicationParameters::UDP_SERVER_PORT, ui8PacketTTL, bLocalReachabilityFromRemote);
        if (0 != (rc = _pConnectorAdapter->gsend (pRemoteProxyInetAddr, ui32RemoteDestinationIP, bReliable, bSequenced, &iCMPPM, sizeof(ICMPProxyMessage),
                                                  pui8Buf, iCMPPM._ui16PayloadLen, nullptr))) {
            checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", Logger::L_MildError,
                            "gsend() of an ICMPProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", Logger::L_MediumDetailDebug,
                        "successfully sent an ICMP ProxyMessage via %s addressed to the remote NetProxy at address <%s:%hu> with parameters: "
                        "bReliable = %s, bSequenced = %s\n", getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(),
                        pRemoteProxyInetAddr->getPort(), bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _pGUIStatsManager->increaseTrafficIn (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                              pRemoteProxyInetAddr->getPort(), PT_ICMP, sizeof(ICMPHeader) + ui16BufLen);
        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                               pRemoteProxyInetAddr->getPort(), PT_ICMP, sizeof(ICMPProxyMessage) + iCMPPM._ui16PayloadLen);

        return 0;
    }

    int Connection::sendUDPUnicastPacketToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP,
                                                      uint8 ui8PacketTTL, const uint8 * const pPacket, uint16 ui16PacketLen, const CompressionSetting * const pCompressionSetting,
                                                      ProxyMessage::Protocol ui8PMProtocol)
    {
        if (!_pConnectorAdapter) {
            return -1;
        }
        if (!pRemoteProxyInetAddr) {
            checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", Logger::L_MildError,
                            "remoteProxyInetAddress is NULL when trying to send a UDPUnicastData ProxyMessage via %s "
                            "to the remote host with IP address %s\n", getConnectorTypeAsString(),
                            InetAddr(ui32RemoteDestinationIP).getIPAsString());
            return -2;
        }
        else if (!isConnected()) {
            checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isCommunicationReliable (ui8PMProtocol);
        bool bSequenced = isCommunicationSequenced (ui8PMProtocol);
        UDPUnicastDataProxyMessage udpUDPM (ui16PacketLen, ui32LocalSourceIP, ui32RemoteDestinationIP, NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                            ui8PacketTTL, &CompressionSetting::DefaultNOCompressionSetting);

        // Applying compression if convenient
        unsigned char *pucBuf[1], *pucBytesToSend = const_cast<unsigned char *> (pPacket);
        unsigned int uiBufLen = 0;
        ConnectorWriter * const pConnectorWriter = ConnectorWriter::getAndLockUPDConnectorWriter (pCompressionSetting);
        if (0 == pConnectorWriter->writeDataAndResetWriter (pPacket, ui16PacketLen, pucBuf, uiBufLen)) {
            if ((uiBufLen > 0) && (uiBufLen < ui16PacketLen)) {
                pucBytesToSend = pucBuf[0];
                udpUDPM._ui16PayloadLen = uiBufLen;
                udpUDPM._ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionTypeAndLevel();

                checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", Logger::L_MediumDetailDebug,
                                "the compressed datagram is %u bytes long, which is less than the original packet of %hu "
                                "bytes in size; NetProxy will send the datagram compressed through %s level $hhu\n",
                                uiBufLen, ui16PacketLen, pCompressionSetting->getCompressionTypeAsString(),
                                pCompressionSetting->getCompressionLevel());
            }
        }
        if (0 != (rc = _pConnectorAdapter->gsend (pRemoteProxyInetAddr, ui32RemoteDestinationIP, bReliable, bSequenced, &udpUDPM,
                                                  sizeof(UDPUnicastDataProxyMessage), pucBytesToSend, udpUDPM._ui16PayloadLen, nullptr))) {
            checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", Logger::L_MildError,
                            "gsend() of a UDPUnicastData ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), rc);
            pConnectorWriter->unlockConnectorWriter();
            return -4;
        }
        pConnectorWriter->unlockConnectorWriter();
        checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", Logger::L_MediumDetailDebug,
                        "successfully sent a UDPUnicastData ProxyMessage via %s with %hu bytes of data "
                        "(%hu bytes before compression, if any) to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), udpUDPM._ui16PayloadLen, ui16PacketLen,
                        pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort());

        _pGUIStatsManager->increaseTrafficIn (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                              pRemoteProxyInetAddr->getPort(), PT_UDP, ui16PacketLen);
        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                               pRemoteProxyInetAddr->getPort(), PT_UDP, sizeof(UDPUnicastDataProxyMessage) + udpUDPM._ui16PayloadLen);

        return 0;
    }

    int Connection::sendMultipleUDPDatagramsToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP,
                                                          MutexUDPQueue * const pUDPDatagramsQueue, const CompressionSetting * const pCompressionSetting, ProxyMessage::Protocol ui8PMProtocol)
    {
        if (!_pConnectorAdapter) {
            return -1;
        }
        if (!pRemoteProxyInetAddr) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", Logger::L_MildError,
                            "remoteProxyInetAddress is NULL when trying to send a MultipleUDPDatagram ProxyMessage via %s "
                            "to the remote host with IP address %s\n", getConnectorTypeAsString(),
                            InetAddr(ui32RemoteDestinationIP).getIPAsString());
            return -2;
        }
        else if (!isConnected()) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        const int iPacketsNum = pUDPDatagramsQueue->size();
        if ((sizeof(MultipleUDPDatagramsProxyMessage) + iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount()) > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", Logger::L_MildError,
                            "impossible to wrap together %d UDP Datagrams for a total of %u bytes; buffer is only %hu bytes long\n",
                            pUDPDatagramsQueue->size(), pUDPDatagramsQueue->getEnqueuedBytesCount(), NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            return -4;
        }

        bool bReliable = isCommunicationReliable (ui8PMProtocol);
        bool bSequenced = isCommunicationSequenced (ui8PMProtocol);
        MultipleUDPDatagramsProxyMessage mUDPDPM (static_cast<uint16> (iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount()), ui32LocalSourceIP, ui32RemoteDestinationIP,
                                                  NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, static_cast<uint8> (iPacketsNum), &CompressionSetting::DefaultNOCompressionSetting);

        unsigned int iTTLIndex = 0;
        uint32 ui32CopiedBytes = 0;
        pUDPDatagramsQueue->resetGet();
        while (const UDPDatagramPacket * const pUDPDatagramPacket = pUDPDatagramsQueue->getNext()) {
            _pucMultipleUDPDatagramsBuffer[iTTLIndex++] = pUDPDatagramPacket->getIPPacketTTL();
            // Leave space for the TTL value of each consolidated packet
            memcpy (_pucMultipleUDPDatagramsBuffer + iPacketsNum + ui32CopiedBytes, pUDPDatagramPacket->getUDPPacket(), pUDPDatagramPacket->getPacketLen());
            ui32CopiedBytes += pUDPDatagramPacket->getPacketLen();
        }
        if (ui32CopiedBytes != pUDPDatagramsQueue->getEnqueuedBytesCount()) {
            // Error copying data
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", Logger::L_MildError,
                            "the value of copied bytes (%u) and bytes to be copied (%u) do not match\n",
                            ui32CopiedBytes, pUDPDatagramsQueue->getEnqueuedBytesCount());
            return -5;
        }

        // Apply compression if convenient
        unsigned char *pucBuf[1], *pucBytesToSend = _pucMultipleUDPDatagramsBuffer;
        unsigned int uiBufLen = 0;
        ConnectorWriter * const pConnectorWriter = ConnectorWriter::getAndLockUPDConnectorWriter (pCompressionSetting);
        if (0 == pConnectorWriter->writeDataAndResetWriter (_pucMultipleUDPDatagramsBuffer, mUDPDPM.getPayloadLen(), pucBuf, uiBufLen)) {
            if ((uiBufLen > 0) && (uiBufLen < mUDPDPM.getPayloadLen())) {
                pucBytesToSend = pucBuf[0];
                mUDPDPM._ui16PayloadLen = uiBufLen;
                mUDPDPM._ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionTypeAndLevel();

                checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", Logger::L_MediumDetailDebug,
                                "the compressed size of %d consolidated UDP datagrams is %u bytes long, which is less than their size before "
                                "compression (%hu bytes); NetProxy will send the consolidated datagrams compressed through %s level $hhu\n",
                                iPacketsNum, uiBufLen, (iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount()),
                                pCompressionSetting->getCompressionTypeAsString(), pCompressionSetting->getCompressionLevel());
            }
        }
        if (0 != (rc = _pConnectorAdapter->gsend (pRemoteProxyInetAddr, ui32RemoteDestinationIP, bReliable, bSequenced, &mUDPDPM,
                                                  sizeof(MultipleUDPDatagramsProxyMessage), pucBytesToSend, mUDPDPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", Logger::L_MildError,
                            "gsend() of a MultipleUDPDatagram ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), rc);
            pConnectorWriter->unlockConnectorWriter();
            return -6;
        }
        pConnectorWriter->unlockConnectorWriter();
        checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", Logger::L_MediumDetailDebug,
                        "successfully sent a MultipleUDPDatagram ProxyMessage via %s with %hu bytes of data (%hu bytes before "
                        "compression, if any) to the remote NetProxy at address <%s:%hu>\n", getConnectorTypeAsString(),
                        mUDPDPM.getPayloadLen(), iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount(),
                        _remoteProxyInetAddr.getIPAsString(), _remoteProxyInetAddr.getPort());

        _pGUIStatsManager->increaseTrafficIn (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                              pRemoteProxyInetAddr->getPort(), PT_UDP, pUDPDatagramsQueue->getEnqueuedBytesCount());
        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                               pRemoteProxyInetAddr->getPort(), PT_UDP, sizeof(MultipleUDPDatagramsProxyMessage) + mUDPDPM.getPayloadLen());

        return 0;
    }

    int Connection::sendUDPBCastMCastPacketToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32RemoteDestinationIP, const uint8 * const pPacket,
                                                         uint16 ui16PacketLen, const CompressionSetting * const pCompressionSetting, ProxyMessage::Protocol ui8PMProtocol)
    {
        if (!_pConnectorAdapter) {
            return -1;
        }
        if (!pRemoteProxyInetAddr) {
            checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", Logger::L_MildError,
                            "remoteProxyInetAddress is NULL when trying to send a UDPBCastMCastData ProxyMessage via %s "
                            "to the remote host with IP address %s\n", getConnectorTypeAsString(),
                            InetAddr(ui32RemoteDestinationIP).getIPAsString());
            return -2;
        }
        else if (!isConnected()) {
            checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        unsigned int uiBufLen = 0;
        bool bReliable = isCommunicationReliable (ui8PMProtocol);
        bool bSequenced = isCommunicationSequenced (ui8PMProtocol);
        UDPBCastMCastDataProxyMessage udpBCMCPM (ui16PacketLen, NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, &CompressionSetting::DefaultNOCompressionSetting);

        // Apply compression if convenient
        unsigned char *pucBuf[1], *pucBytesToSend = const_cast <unsigned char *> (pPacket);
        ConnectorWriter * const pConnectorWriter = ConnectorWriter::getAndLockUPDConnectorWriter (pCompressionSetting);
        if (0 == pConnectorWriter->writeDataAndResetWriter (pPacket, ui16PacketLen, pucBuf, uiBufLen)) {
            if ((uiBufLen > 0) && (uiBufLen < ui16PacketLen)) {
                pucBytesToSend = pucBuf[0];
                udpBCMCPM._ui16PayloadLen = uiBufLen;
                udpBCMCPM._ui8CompressionTypeAndLevel = pCompressionSetting->getCompressionTypeAndLevel();

                checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", Logger::L_MediumDetailDebug,
                                "the compressed multi-/broad-cast datagram is %u bytes long, which is less than the original packet of %hu "
                                "bytes in size; NetProxy will send the multi-/broad-cast datagram compressed through %s level $hhu\n",
                                uiBufLen, ui16PacketLen, pCompressionSetting->getCompressionTypeAsString(),
                                pCompressionSetting->getCompressionLevel());
            }
        }
        if (0 != (rc = _pConnectorAdapter->gsend (pRemoteProxyInetAddr, ui32RemoteDestinationIP, bReliable, bSequenced, &udpBCMCPM,
                                                  sizeof(UDPBCastMCastDataProxyMessage), pucBytesToSend, udpBCMCPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", Logger::L_MildError,
                            "gsend() of a UDPBCastMCastData ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort(), rc);
            pConnectorWriter->unlockConnectorWriter();
            return -4;
        }
        pConnectorWriter->unlockConnectorWriter();
        checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", Logger::L_MediumDetailDebug,
                        "successfully sent a UDPBCastMCastData ProxyMessage via %s with %hu bytes of data "
                        "(%hu bytes before compression, if any) to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), udpBCMCPM.getPayloadLen(), ui16PacketLen,
                        pRemoteProxyInetAddr->getIPAsString(), pRemoteProxyInetAddr->getPort());

        _pGUIStatsManager->increaseTrafficIn (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                              pRemoteProxyInetAddr->getPort(), PT_UDP, ui16PacketLen);
        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), pRemoteProxyInetAddr->getIPAddress(),
                                               pRemoteProxyInetAddr->getPort(), PT_UDP, sizeof(UDPBCastMCastDataProxyMessage) + udpBCMCPM.getPayloadLen());

        return 0;
    }

    int Connection::sendOpenTCPConnectionRequest (Entry * const pEntry, bool bLocalReachabilityFromRemote)
    {
        if (!pEntry) {
            return -1;
        }

        if (!_pConnectorAdapter) {
            return -2;
        }
        else if (!isConnected()) {
            checkAndLogMsg ("Connection::sendOpenTCPConnectionRequest", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            _remoteProxyInetAddr.getIPAsString(), _remoteProxyInetAddr.getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isCommunicationReliable (pEntry->getProtocol());
        bool bSequenced = isCommunicationSequenced (pEntry->getProtocol());
        OpenConnectionProxyMessage oCPM (pEntry->ui16ID, pEntry->ui32LocalIP, pEntry->ui16LocalPort, pEntry->ui32RemoteIP, pEntry->ui16RemotePort,
                                         NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, NetProxyApplicationParameters::MOCKET_SERVER_PORT,
                                         NetProxyApplicationParameters::TCP_SERVER_PORT, NetProxyApplicationParameters::UDP_SERVER_PORT,
                                         pEntry->getConnectorWriter()->getCompressionSetting(), bLocalReachabilityFromRemote);
        if (0 != (rc = _pConnectorAdapter->send (&pEntry->remoteProxyAddr, pEntry->ui32RemoteIP, bReliable, bSequenced, &oCPM, sizeof(OpenConnectionProxyMessage)))) {
            checkAndLogMsg ("Connection::sendOpenTCPConnectionRequest", Logger::L_MildError,
                            "L%hu-R0: send() of an OpenConnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            oCPM._ui16LocalID, getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort(), rc);
            return -5;
        }
        checkAndLogMsg ("Connection::sendOpenTCPConnectionRequest", Logger::L_LowDetailDebug,
                        "L%hu-R0: successfully sent an OpenTCPConnection request via %s to the remote NetProxy at address <%s:%hu> "
                        "for virtual connection <%s:%hu --> %s:%hu> with parameters: bReliable = %s, bSequenced = %s\n",
                        oCPM._ui16LocalID, getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                        pEntry->remoteProxyAddr.getPort(), InetAddr(oCPM._ui32LocalIP).getIPAsString(), oCPM._ui16LocalPort,
                        InetAddr(oCPM._ui32RemoteIP).getIPAsString(), oCPM._ui16RemotePort,
                        bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), pEntry->ui32RemoteIP, getRemoteProxyID(), getRemoteProxyInetAddr()->getIPAddress(),
                                               getRemoteProxyInetAddr()->getPort(), PT_TCP, sizeof(OpenConnectionProxyMessage));

        return 0;
    }

    int Connection::sendTCPConnectionOpenedResponse (Entry * const pEntry, bool bLocalReachabilityFromRemote)
    {
        if (!pEntry) {
            return -1;
        }

        if (!_pConnectorAdapter) {
            return -2;
        }
        else if (!isConnected()) {
            checkAndLogMsg ("Connection::sendTCPConnectionOpenedResponse", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            _remoteProxyInetAddr.getIPAsString(), _remoteProxyInetAddr.getPort(), getRemoteProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isCommunicationReliable (pEntry->getProtocol());
        bool bSequenced = isCommunicationSequenced (pEntry->getProtocol());
        ConnectionOpenedProxyMessage cOPM (pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32LocalIP, NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                           NetProxyApplicationParameters::MOCKET_SERVER_PORT, NetProxyApplicationParameters::TCP_SERVER_PORT,
                                           NetProxyApplicationParameters::UDP_SERVER_PORT, pEntry->getConnectorWriter()->getCompressionSetting(), bLocalReachabilityFromRemote);
        if (0 != (rc = _pConnectorAdapter->send (&pEntry->remoteProxyAddr, pEntry->ui32RemoteIP, bReliable, bSequenced, &cOPM, sizeof(ConnectionOpenedProxyMessage)))) {
            checkAndLogMsg ("Connection::sendTCPConnectionOpenedResponse", Logger::L_MildError,
                            "L%hu-R%hu: send() of a TCPConnectionOpened ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            cOPM._ui16LocalID, cOPM._ui16RemoteID, getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendTCPConnectionOpenedResponse", Logger::L_LowDetailDebug,
                        "L%hu-R%hu: successfully sent a TCPConnectionOpened response via %s to the remote NetProxy at address <%s:%hu> with parameters: "
                        "bReliable = %s, bSequenced = %s\n", cOPM._ui16LocalID, cOPM._ui16RemoteID, getConnectorTypeAsString(),
                        pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort(), bReliable ? "true":"false",
                        bSequenced ? "true" : "false");

        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), pEntry->ui32RemoteIP, getRemoteProxyID(), getRemoteProxyInetAddr()->getIPAddress(),
                                               getRemoteProxyInetAddr()->getPort(), PT_TCP, sizeof(ConnectionOpenedProxyMessage));

        return 0;
    }

    int Connection::sendTCPDataToRemoteHost (const Entry * const pEntry, const uint8 * const pui8Buf, uint16 ui16BufLen, uint8 ui8TCPFlags)
    {
        if (!pEntry) {
            return -1;
        }

        Connection *pConnection = pEntry->getConnection();
        if (!pConnection) {
            return -2;
        }

        ConnectorAdapter * const _pConnectorAdapter = pConnection->getConnectorAdapter();
        if (!_pConnectorAdapter || !_pConnectorAdapter->isConnected()) {
            return -3;
        }

        int rc;
        bool bReliable = isCommunicationReliable (pEntry->getProtocol());
        bool bSequenced = isCommunicationSequenced (pEntry->getProtocol());
        TCPDataProxyMessage tDPM (ui16BufLen, pEntry->ui16ID, pEntry->ui16RemoteID, ui8TCPFlags);

		if (NetProxyApplicationParameters::PRIORITY_MECHANISM && (_pConnectorAdapter->getOutgoingBufferSize() < sizeof(TCPDataProxyMessage))) {
			checkAndLogMsg ("Connection::sendTCPDataToRemoteHost", Logger::L_Info,
				            "L%hu-R%hu: blocking gsend() of a TCPData ProxyMessage via %s to the remote NetProxy at address <%s:%hu> dropped\n",
				            tDPM._ui16LocalID, tDPM._ui16RemoteID, getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
				            pEntry->remoteProxyAddr.getPort());
			return -4;
		}

        if (0 != (rc = _pConnectorAdapter->gsend (&pEntry->remoteProxyAddr, pEntry->ui32RemoteIP, bReliable, bSequenced, &tDPM,
                                                  sizeof(TCPDataProxyMessage), pui8Buf, tDPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::sendTCPDataToRemoteHost", Logger::L_MildError,
                            "L%hu-R%hu: gsend() of a TCPData ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            tDPM._ui16LocalID, tDPM._ui16RemoteID, getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort(), rc);
            return -5;
        }
        checkAndLogMsg ("Connection::sendTCPDataToRemoteHost", Logger::L_HighDetailDebug,
                        "L%hu-R%hu: successfully sent a TCPData ProxyMessage with %hu bytes of data via %s to the remote NetProxy at address "
                        "<%s:%hu> with parameters: bReliable = %s, bSequenced = %s\n", tDPM._ui16LocalID, tDPM._ui16RemoteID,
                        tDPM._ui16PayloadLen, getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                        pEntry->remoteProxyAddr.getPort(), bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), pEntry->ui32RemoteIP, getRemoteProxyID(), getRemoteProxyInetAddr()->getIPAddress(),
                                               getRemoteProxyInetAddr()->getPort(), PT_TCP, sizeof(TCPDataProxyMessage) + tDPM.getPayloadLen());

        return 0;
    }

    int Connection::sendCloseTCPConnectionRequest (const Entry * const pEntry)
    {
        if (!pEntry) {
            return -1;
        }

        Connection *pConnection = pEntry->getConnection();
        if (!pConnection) {
            return -2;
        }

        ConnectorAdapter * const _pConnectorAdapter = pConnection->getConnectorAdapter();
        if (!_pConnectorAdapter || !_pConnectorAdapter->isConnected()) {
            return -3;
        }

        int rc;
        bool bReliable = isCommunicationReliable (pEntry->getProtocol());
        bool bSequenced = isCommunicationSequenced (pEntry->getProtocol());
        CloseConnectionProxyMessage cCPM (pEntry->ui16ID, pEntry->ui16RemoteID);
        if (0 != (rc = _pConnectorAdapter->send (&pEntry->remoteProxyAddr, pEntry->ui32RemoteIP, bReliable, bSequenced, &cCPM, sizeof(CloseConnectionProxyMessage)))) {
            checkAndLogMsg ("Connection::sendCloseTCPConnectionRequest", Logger::L_MildError,
                            "L%hu-R%hu: send() of a CloseTCPConnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            cCPM._ui16LocalID, cCPM._ui16RemoteID, getConnectorTypeAsString(), pEntry->remoteProxyAddr.getIPAsString(),
                            pEntry->remoteProxyAddr.getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendCloseTCPConnectionRequest", Logger::L_LowDetailDebug,
                        "L%hu-R%hu: successfully sent a CloseTCPConnection request via %s to the remote NetProxy at address <%s:%hu> with parameters: "
                        "bReliable = %s, bSequenced = %s\n", cCPM._ui16LocalID, cCPM._ui16RemoteID, getConnectorTypeAsString(),
                        pEntry->remoteProxyAddr.getIPAsString(), pEntry->remoteProxyAddr.getPort(),
                        bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), pEntry->ui32RemoteIP, getRemoteProxyID(), getRemoteProxyInetAddr()->getIPAddress(),
                                               getRemoteProxyInetAddr()->getPort(), PT_TCP, sizeof(CloseConnectionProxyMessage));

        return 0;
    }

    int Connection::sendResetTCPConnectionRequest (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32RemoteDestinationIP,
                                                   uint16 ui16LocalID, uint16 ui16RemoteID, ProxyMessage::Protocol ui8PMProtocol)
    {
        if (!_pConnectorAdapter) {
            return -1;
        }
        else if (!_pConnectorAdapter->isConnected()) {
            checkAndLogMsg ("Connection::sendResetTCPConnectionRequest", Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> with UniqueID %u is not connected\n",
                            getConnectorTypeAsString(), _remoteProxyInetAddr.getIPAsString(), _remoteProxyInetAddr.getPort(), getRemoteProxyID());

            return -2;
        }

        int rc;
        bool bReliable = isCommunicationReliable (ui8PMProtocol);
        bool bSequenced = isCommunicationSequenced (ui8PMProtocol);
        ResetConnectionProxyMessage rCPM (ui16LocalID, ui16RemoteID);
        if (0 != (rc = _pConnectorAdapter->send (pRemoteProxyInetAddr, ui32RemoteDestinationIP, bReliable, bSequenced, &rCPM, sizeof(ResetConnectionProxyMessage)))) {
            checkAndLogMsg ("Connection::sendResetTCPConnectionRequest", Logger::L_MildError,
                            "L%hu-R%hu: send() of a ResetTCPconnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            rCPM._ui16LocalID, rCPM._ui16RemoteID, getConnectorTypeAsString(), _remoteProxyInetAddr.getIPAsString(),
                            _remoteProxyInetAddr.getPort(), rc);
            return -3;
        }
        checkAndLogMsg ("Connection::sendResetTCPConnectionRequest", Logger::L_LowDetailDebug,
                        "L%hu-R%hu: successfully sent a ResetTCPconnection request via %s to the remote NetProxy at address <%s:%hu> with parameters: "
                        "bReliable = %s, bSequenced = %s\n", rCPM._ui16LocalID, rCPM._ui16RemoteID, getConnectorTypeAsString(),
                        _remoteProxyInetAddr.getIPAsString(), _remoteProxyInetAddr.getPort(), bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _pGUIStatsManager->increaseTrafficOut (getConnectorType(), ui32RemoteDestinationIP, getRemoteProxyID(), getRemoteProxyInetAddr()->getIPAddress(),
                                               getRemoteProxyInetAddr()->getPort(), PT_TCP, sizeof(ResetConnectionProxyMessage));

        return 0;
    }

    void Connection::updateRemoteProxyID (uint32 ui32RemoteProxyID)
    {
        if (getRemoteProxyID() == ui32RemoteProxyID) {
            // Connection has already been updated --> nothing to do
            return;
        }

        Connection::_pConnectionManager->updateRemoteProxyUniqueID (getRemoteProxyID(), ui32RemoteProxyID);
        auto * pAutoConnEntry = Connection::_pConnectionManager->getAutoConnectionEntryToRemoteProxyID (_ui32RemoteProxyID);
        if (pAutoConnEntry) {
            pAutoConnEntry->updateRemoteProxyID (ui32RemoteProxyID);
        }
        Connection * const pOldConnection = Connection::_pConnectionManager->addNewActiveConnectionToRemoteProxy (this, ui32RemoteProxyID);
        _pConnectorAdapter->readConfigFile (_pConnectionManager->getMocketsConfigFileForProxyWithID (ui32RemoteProxyID));
        enforceConnectionsConsistency (pOldConnection, ui32RemoteProxyID);
        _ui32RemoteProxyID = ui32RemoteProxyID;
    }

    void Connection::updateRemoteProxyInformation (uint32 ui32RemoteProxyID, uint16 ui16MocketsServerPort, uint16 ui16TCPServerPort,
                                                   uint16 ui16UDPServerPort, bool bRemoteProxyReachability)
    {
        if (getRemoteProxyID() == ui32RemoteProxyID) {
            // Connection has already been updated --> nothing to do
            return;
        }

        Connection * const pOldConnection = Connection::_pConnectionManager->updateRemoteProxyInfo (this, ui32RemoteProxyID, ui16MocketsServerPort, ui16TCPServerPort,
                                                                                                    ui16UDPServerPort, bRemoteProxyReachability);
        auto * pAutoConnEntry = Connection::_pConnectionManager->getAutoConnectionEntryToRemoteProxyID (_ui32RemoteProxyID);
        if (pAutoConnEntry) {
            pAutoConnEntry->updateRemoteProxyID (ui32RemoteProxyID);
        }
        _pConnectorAdapter->readConfigFile (_pConnectionManager->getMocketsConfigFileForProxyWithID (ui32RemoteProxyID));
        enforceConnectionsConsistency (pOldConnection, ui32RemoteProxyID);
        _ui32RemoteProxyID = ui32RemoteProxyID;
    }

    void Connection::enforceConnectionsConsistency (Connection * const pOldConnection, uint32 ui32RemoteProxyID)
    {
        // If pOldConnection is passed as argument to this method, then it must have same remote NetProxy UniqueID of _pConnection
        if (pOldConnection && (pOldConnection != this)) {
            checkAndLogMsg ("Connection::enforceConnectionConsistency", Logger::L_LowDetailDebug,
                            "replaced the existing %s connection to the remote NetProxy with UniqueID %u and IP address %s; "
                            "the old connection, which was in status %hu and for which the local NetProxy had the role of %s, "
                            "is going to be closed\n", pOldConnection->getConnectorTypeAsString(), ui32RemoteProxyID,
                            pOldConnection->getRemoteProxyInetAddr()->getIPAsString(), pOldConnection->getStatus(),
                            (pOldConnection->getLocalHostRole() == CHR_Client) ? "client" : "server");
            pOldConnection->requestTermination();
        }
    }

    bool Connection::peerUnreachableWarning (void *pCallbackArg, unsigned long ulTimeSinceLastContact)
    {
        Mocket * const pMocket = (Mocket*) pCallbackArg;
        InetAddr remoteInetAddress (pMocket->getRemoteAddress(), pMocket->getRemotePort());
        if (ulTimeSinceLastContact > NetProxyApplicationParameters::MOCKET_TIMEOUT) {
            checkAndLogMsg ("Connection::peerUnreachableWarning", Logger::L_Info,
                            "disconnecting %s connection with the remote NetProxy at address %s:%hu\n",
                            connectorTypeToString (CT_MOCKETS), remoteInetAddress.getIPAsString(),
                            remoteInetAddress.getPort());
            return true;
        }

        return false;
    }

}
