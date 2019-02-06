/*
 * UDPConnector.cpp
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

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "Logger.h"

#include "UDPConnector.h"
#include "UDPSocketAdapter.h"
#include "Connection.h"
#include "ConnectionManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    UDPConnector::~UDPConnector (void)
    {
        requestTermination();

        if (_upUDPSocketAdapter) {
            _upUDPSocketAdapter->requestUDPConnectionThreadTermination();
            _upUDPSocketAdapter->shutdown (true, true);
            _upUDPSocketAdapter->close();
            _upUDPSocketAdapter->requestUDPConnectionThreadTerminationAndWait();
        }
    }

    int UDPConnector::init (uint16 ui16AcceptServerPort, uint32 ui32LocalIPv4Address)
    {
        int rc;

        _upUDPSocketAdapter = ::make_unique<UDPSocketAdapter> (::make_unique<NOMADSUtil::UDPDatagramSocket>(),
                                                               NOMADSUtil::InetAddr{ui32LocalIPv4Address, ui16AcceptServerPort});
        if ((rc = _upUDPSocketAdapter->initUDPSocket (ui16AcceptServerPort, ui32LocalIPv4Address)) < 0) {
            checkAndLogMsg ("UPDConnector::init", NOMADSUtil::Logger::L_MildError,
                            "listen() on ServerSocket failed - could not bind to address <%s:%hu>; rc = %d\n",
                            NOMADSUtil::InetAddr{ui32LocalIPv4Address}.getIPAsString(), ui16AcceptServerPort, rc);
            return -1;
        }
        Connector::init (ui16AcceptServerPort, ui32LocalIPv4Address);

        if (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS > 0) {
            if (0 != (rc = _upUDPSocketAdapter->setUDPSocketTransmissionRateLimit (NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS))) {
                checkAndLogMsg ("UPDConnector::init", NOMADSUtil::Logger::L_Warning,
                                "setTransmitRateLimit() on UDP Socket failed with rc = %d; specified value was %u; no limit will be set\n",
                                rc, NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS);
                _upUDPSocketAdapter->setUDPSocketTransmissionRateLimit (0);
            }
        }
        if ((rc = _upUDPSocketAdapter->startUDPConnectionThread()) < 0) {
            checkAndLogMsg ("UPDConnector::init", NOMADSUtil::Logger::L_MildError,
                            "start() on the UDPConnection thread failed with rc = %d\n", rc);
            return -2;
        }

        return 0;
    }

    void UDPConnector::terminateExecution (void)
    {
        requestTermination();

        if (_upUDPSocketAdapter) {
            _upUDPSocketAdapter->requestUDPConnectionThreadTermination();
            _upUDPSocketAdapter->shutdown (true, true);
            _upUDPSocketAdapter->close();
        }
    }

    void UDPConnector::run (void)
    {
        started();

        int rc;
        uint32 ui32BufLen;
        NOMADSUtil::InetAddr iaRemoteProxyAddress, iaLocalProxyAddress;

        while (!terminationRequested()) {
            if ((rc = _upUDPSocketAdapter->receiveMessage (_pucInBuf, NetworkConfigurationSettings::PROXY_MESSAGE_MTU,
                                                           iaLocalProxyAddress, iaRemoteProxyAddress)) < 0) {
                if (!terminationRequested()) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_MildError,
                                    "receiveMessage() on UDPSocket failed with rc = %d\n", rc);
                    setTerminatingResultCode (-1);
                }
                break;
            }
            else if (rc == 0) {
                continue;
            }

            auto * const pProxyMessage = reinterpret_cast<ProxyMessage * const> (_pucInBuf);
            checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_HighDetailDebug,
                            "received a packet of %d bytes with ProxyMessage flag = %d from address %s\n",
                            rc, pProxyMessage->getMessageType(), iaRemoteProxyAddress.getIPAsString());

            const auto ui64UDPConnectionKey = generateUDPConnectionsTableKey (iaRemoteProxyAddress);
            auto & spConnection = _umUDPConnections[ui64UDPConnectionKey];
            if (spConnection && (spConnection->isTerminationRequested() ||
                (pProxyMessage->getMessageType() == PacketType::PMT_InitializeConnection))) {
                // If termination has been requested, remove the Connection instance from the table
                spConnection->prepareForDelete();
                spConnection.reset();
            }

            if (pProxyMessage->getMessageType() == PacketType::PMT_InitializeConnection) {
                // Server-side initialization: add new UDP Connection to the UninitializedConnectionsTable (initialization will be done below)
                spConnection = std::make_shared<Connection> (iaLocalProxyAddress, iaRemoteProxyAddress, _upUDPSocketAdapter.get(),
                                                             _rConnectionManager, _rTCPConnTable, _rTCPManager,
                                                             _rPacketRouter, _rStatisticsManager);
                Connection::addNewUninitializedConnectionToTable (spConnection);
            }
            else if (!spConnection) {
                if (pProxyMessage->getMessageType() == PacketType::PMT_ConnectionError) {
                    // Ignore a ConnectionError ProxyMessage since there is not a Connection in the table
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "received a ConnectionError ProxyMessage from the remote NetProxy with the address <%s:%hu>, "
                                    "but no UDP Connection instances were found connected to that NetProxy; ignoring the message\n",
                                    iaRemoteProxyAddress.getIPAsString(), iaRemoteProxyAddress.getPort());
                    continue;
                }

                // Connection not found in the table --> create a new one if it cannot be retrieved
                uint32 ui32RemoteNetProxyUniqueID =
                    _rConnectionManager.findUniqueIDOfRemoteNetProxyWithIPAddress (iaRemoteProxyAddress.getIPAddress());
                if (ui32RemoteNetProxyUniqueID == 0) {
                    // Send a ConnectionError ProxyMessage back to the remote NetProxy
                    Connection::sendConnectionErrorProxyMessageToRemoteHost (_upUDPSocketAdapter.get(), iaRemoteProxyAddress,
                                                                             NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                                                             0, iaLocalProxyAddress.getIPAddress(),
                                                                             iaRemoteProxyAddress.getIPAddress(), CT_UDPSOCKET);
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "received a packet of %d bytes with a ProxyMessage of PakcetType %hhu from address <%s:%hu>, but the "
                                    "UDP connection is not initialized and the local configuration files contain no information about "
                                    "remote NetProxies with that address. NetProxy has replied with a ConnectionError ProxyMessage "
                                    "to the remote NetProxy\n", rc, static_cast<uint8> (pProxyMessage->getMessageType()),
                                    iaRemoteProxyAddress.getIPAsString(), iaRemoteProxyAddress.getPort());
                    continue;
                }

                // If the processing gets here, ui32RemoteNetProxyUniqueID contains the NPUID of the remote NetProxy
                auto & spConnectionInTable = Connection::getConnectionFromTable (ui32RemoteNetProxyUniqueID, iaLocalProxyAddress,
                                                                                 iaRemoteProxyAddress, CT_UDPSOCKET,
                                                                                 _upUDPSocketAdapter->getEncryptionType());
                if (!spConnectionInTable) {
                    // Local and remote NetProxies out of sync --> send a ConnectionError ProxyMessage back to the remote NetProxy
                    Connection::sendConnectionErrorProxyMessageToRemoteHost (_upUDPSocketAdapter.get(), iaRemoteProxyAddress,
                                                                             NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                                                             0, iaLocalProxyAddress.getIPAddress(),
                                                                             iaRemoteProxyAddress.getIPAddress(), CT_UDPSOCKET);
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "NetProxy could not find an existing UDP Connection to the remote NetProxy with UniqueID %u and "
                                    "address <%s:%hu> in the ConnectionsTable when it received a ProxyMessage of PacketType %hhu; "
                                    "NetProxy replied with a ConnectionError ProxyMessage and will discard the message\n",
                                    ui32RemoteNetProxyUniqueID, iaRemoteProxyAddress.getIPAsString(), iaRemoteProxyAddress.getPort(),
                                    static_cast<uint8> (pProxyMessage->getMessageType()));
                    continue;
                }
                if (pProxyMessage->getMessageType() != PacketType::PMT_ConnectionInitialized) {
                    // Local and remote NetProxies out of sync --> send a ConnectionError ProxyMessage back to the remote NetProxy
                    spConnectionInTable->prepareForDelete();
                    Connection::sendConnectionErrorProxyMessageToRemoteHost (_upUDPSocketAdapter.get(), iaRemoteProxyAddress,
                                                                             NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                                                             0, iaLocalProxyAddress.getIPAddress(),
                                                                             iaRemoteProxyAddress.getIPAddress(), CT_UDPSOCKET);
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "found an existing UDP Connection to the remote NetProxy with UniqueID %u and address <%s:%hu> "
                                    "in the ConnectionsTable, but the received ProxyMessage is of PacketType %hhu; NetProxy replied "
                                    "with a ConnectionError ProxyMessage and will discard the message\n", ui32RemoteNetProxyUniqueID,
                                    iaRemoteProxyAddress.getIPAsString(), iaRemoteProxyAddress.getPort(),
                                    static_cast<uint8> (pProxyMessage->getMessageType()));
                    continue;
                }

                // Synchronize the _umUDPConnections table with the ConnectionsTable
                _umUDPConnections[ui64UDPConnectionKey] = spConnectionInTable;
                checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_LowDetailDebug,
                                "added an existing UDP Connection to the remote NetProxy with UniqueID %u and "
                                "address <%s:%hu> to the UDPConnections table\n", ui32RemoteNetProxyUniqueID,
                                iaRemoteProxyAddress.getIPAsString(), iaRemoteProxyAddress.getPort());
            }

            switch (pProxyMessage->getMessageType()) {
            case PacketType::PMT_InitializeConnection:
            {
                auto * const pICPM = reinterpret_cast<InitializeConnectionProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveInitializeConnectionProxyMessage (pICPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveInitializeConnectionProxyMessage returned with rc = %d\n", rc);
                    _umUDPConnections[ui64UDPConnectionKey].reset();
                }
                break;
            }

            case PacketType::PMT_ConnectionInitialized:
            {
                if (spConnection->getLocalHostRole() == Connection::CHR_Server) {
                    // Simultaneous connection initialization --> ignore packet
                    break;
                }

                auto * const pCIPM  = reinterpret_cast<ConnectionInitializedProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveConnectionInitializedProxyMessage (pCIPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveConnectionInitializedProxyMessage returned with rc = %d\n", rc);
                    _umUDPConnections[ui64UDPConnectionKey].reset();
                }
                break;
            }

            case PacketType::PMT_ICMPMessage:
            {
                auto * const pICMPPM = reinterpret_cast<ICMPProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveICMPProxyMessageToRemoteHost (pICMPPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveICMPProxyMessageToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_UDPUnicastData:
            {
                auto * const pUDPUCDPM = reinterpret_cast<UDPUnicastDataProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveUDPUnicastPacketToRemoteHost (pUDPUCDPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveUDPUnicastPacketToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_MultipleUDPDatagrams:
            {
                auto * const pMUDPDPM = reinterpret_cast<MultipleUDPDatagramsProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveMultipleUDPDatagramsToRemoteHost (pMUDPDPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveMultipleUDPDatagramsToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_UDPBCastMCastData:
            {
                auto * const pUDPBCMCDPM = reinterpret_cast<UDPBCastMCastDataProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveUDPBCastMCastPacketToRemoteHost (pUDPBCMCDPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveUDPBCastMCastPacketToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_TCPOpenConnection:
            {
                auto * const pOCPM = reinterpret_cast<OpenTCPConnectionProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveOpenTCPConnectionRequest (pOCPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveOpenTCPConnectionRequest returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_TCPConnectionOpened:
            {
                auto * const pCOPM = reinterpret_cast<TCPConnectionOpenedProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveTCPConnectionOpenedResponse (pCOPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveTCPConnectionOpenedResponse returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_TCPData:
            {
                auto * const pTCPDPM = reinterpret_cast<TCPDataProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveTCPDataAndSendToLocalHost (pTCPDPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveTCPDataAndSendToLocalHost returned with rc = %d\n", rc);
                    if (rc == -1) {
                        spConnection->sendResetTCPConnectionRequest (0, 0, pTCPDPM->_ui16RemoteID,
                                                                     pTCPDPM->_ui16LocalID, true, true);
                    }
                }
                break;
            }

            case PacketType::PMT_TCPCloseConnection:
            {
                auto * const pCCPM = reinterpret_cast<CloseTCPConnectionProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveCloseTCPConnectionRequest (pCCPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveCloseTCPConnectionRequest returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_TCPResetConnection:
            {
                auto * const pRCPM = reinterpret_cast<ResetTCPConnectionProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveResetTCPConnectionRequest (pRCPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveResetTCPConnectionRequest returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_TunnelPacket:
            {
                auto * const pTPPM = reinterpret_cast<TunnelPacketProxyMessage *> (_pucInBuf);
                if ((rc = spConnection->receiveTunneledEthernetPacket (pTPPM)) < 0) {
                    checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveResetTCPConnectionRequest returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_ConnectionError:
            {
                auto * const pCEPM = reinterpret_cast<ConnectionErrorProxyMessage *> (_pucInBuf);
                checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Warning,
                                "received a ConnectionError ProxyMessage from the remote NetProxy; "
                                "preparing UDP Connection for delete\n");
                spConnection->prepareForDelete();
                // Delete the Connection instance by resetting the shared_ptr that owns it
                _umUDPConnections[ui64UDPConnectionKey].reset();
                break;
            }

            default:
            {
                checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_MildError,
                                "received a ProxyMessage via %s of %u bytes and unknown type %hhu; the NetProxy will ignore it\n",
                                spConnection->getConnectorTypeAsString(), static_cast<uint8> (pProxyMessage->getMessageType()), rc);
            }
            }

            // Packet here has already been discarded or processed
            _i32BytesInBuffer = 0;
        }
        checkAndLogMsg ("UDPConnector::run", NOMADSUtil::Logger::L_Info,
                        "UDPConnector terminated; termination code is %d\n",
                        getTerminatingResultCode());
        setTerminatingResultCode (0);
        terminating();

        _rConnectionManager.deregisterConnector (getBoundIPv4Address(), _connectorType);
    }
}
