/*
 * Connection.cpp
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

#include <type_traits>

#include "UDPRawDatagramSocket.h"
#include "Mocket.h"
#include "Logger.h"

#include "Connection.h"
#include "ProxyMessages.h"
#include "QueryResult.h"
#include "MutexUDPQueue.h"
#include "ConnectorAdapter.h"
#include "MocketAdapter.h"
#include "UDPSocketAdapter.h"
#include "ConnectorReader.h"
#include "ConnectorWriter.h"
#include "Entry.h"
#include "ConnectionManager.h"
#include "TCPConnTable.h"
#include "TCPManager.h"
#include "PacketRouter.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    void Connection::IncomingMessageHandler::run (void)
    {
        started();

        int rc;
        uint32 ui32MsgSize;
        auto * const pConnectorAdapter = _spConnection->getConnectorAdapter();
        if (_spConnection->getRemoteNetProxyID() != 0) {
            // Add Node, Edge, and LinkDescription measures
            _spConnection->registerConnectionWithStatisticsManager();
            checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_HighDetailDebug,
                            "client-side: added Node, Edge, and Link measures for the %s connection "
                            "with the remote NetProxy with UniqueID %u and address %s:%hu\n",
                            _spConnection->getConnectorTypeAsString(), _spConnection->getRemoteNetProxyID(),
                            _spConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                            _spConnection->getRemoteNetProxyInetAddr()->getPort());
        }

        while (!terminationRequested()) {
            if ((rc = pConnectorAdapter->receiveMessage (_ui8InBuf, sizeof(_ui8InBuf))) < 0) {
                if (!terminationRequested()) {
                    if (pConnectorAdapter->isConnected()) {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_MildError,
                                        "receive() on %s failed with rc = %d\n",
                                        _spConnection->getConnectorTypeAsString(), rc);
                        setTerminatingResultCode (-1);
                    }
                    else {
                        checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Info,
                                        "connection closed by the remote host; rc = %d\n", rc);
                    }
                }
                break;
            }
            else if (rc == 0) {
                continue;
            }

            if (terminationRequested()) {
                checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Info,
                                "received termination request; avoid processing any received packet\n");
                break;
            }

            ui32MsgSize = static_cast<uint32> (rc);
            const auto * const pProxyMessage = reinterpret_cast<ProxyMessage *> (_ui8InBuf);
            switch (pProxyMessage->getMessageType()) {
            case PacketType::PMT_InitializeConnection:
            {
                const auto * const pICPM = reinterpret_cast<const InitializeConnectionProxyMessage * > (pProxyMessage);
                if ((rc = _spConnection->receiveInitializeConnectionProxyMessage (pICPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveInitializeConnectionProxyMessage returned with rc = %d; "
                                    "terminating the IncomingMessageHandler thread\n", rc);
                    setTerminatingResultCode (-2);
                }
                break;
            }

            case PacketType::PMT_ConnectionInitialized:
            {
                const auto * const pCIPM = reinterpret_cast<const ConnectionInitializedProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveConnectionInitializedProxyMessage (pCIPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveConnectionInitializedProxyMessage returned with rc = %d; "
                                    "terminating the IncomingMessageHandler thread\n", rc);
                    setTerminatingResultCode (-3);
                }
                break;
            }

            case PacketType::PMT_ICMPMessage:
            {
                const auto * const pICMPPM = reinterpret_cast<const ICMPProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveICMPProxyMessageToRemoteHost (pICMPPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveICMPProxyMessageToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_UDPUnicastData:
            {
                const auto * const pUDPUcDPM = reinterpret_cast<const UDPUnicastDataProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveUDPUnicastPacketToRemoteHost (pUDPUcDPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveUDPUnicastPacketToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_MultipleUDPDatagrams:
            {
                const auto * const pMUDPDPM = reinterpret_cast<const MultipleUDPDatagramsProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveMultipleUDPDatagramsToRemoteHost (pMUDPDPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveMultipleUDPDatagramsToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_UDPBCastMCastData:
            {
                const auto * const pUDPBcMcDPM = reinterpret_cast<const UDPBCastMCastDataProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveUDPBCastMCastPacketToRemoteHost (pUDPBcMcDPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveUDPBCastMCastPacketToRemoteHost returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_TCPOpenConnection:
            {
                const auto * const pOTCPCPM = reinterpret_cast<const OpenTCPConnectionProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveOpenTCPConnectionRequest (pOTCPCPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveOpenTCPConnectionRequest returned with rc = %d\n", rc);
                    _spConnection->sendResetTCPConnectionRequest (pOTCPCPM->_ui32RemoteIP, pOTCPCPM->_ui32LocalIP, 0,
                                                                  pOTCPCPM->_ui16LocalID, true, true);
                }
                break;
            }

            case PacketType::PMT_TCPConnectionOpened:
            {
                const auto * const pTCPCOPM = reinterpret_cast<const TCPConnectionOpenedProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveTCPConnectionOpenedResponse (pTCPCOPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveTCPConnectionOpenedResponse returned with rc = %d\n", rc);
                    if (rc == -1) {
                        _spConnection->sendResetTCPConnectionRequest (0, 0, pTCPCOPM->_ui16RemoteID,
                                                                      pTCPCOPM->_ui16LocalID, true, true);
                    }
                }
                break;
            }

            case PacketType::PMT_TCPData:
            {
                if (ui32MsgSize <= sizeof(TCPDataProxyMessage)) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "received a TCPDataProxyMessage via %s that is too short or has no data\n",
                                    _spConnection->getConnectorTypeAsString());
                    break;
                }

                const auto * const pTCPDPM = reinterpret_cast<const TCPDataProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveTCPDataAndSendToLocalHost (pTCPDPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveTCPDataAndSendToLocalHost returned with rc = %d\n", rc);
                    if (rc == -1) {
                        _spConnection->sendResetTCPConnectionRequest (0, 0, pTCPDPM->_ui16RemoteID,
                                                                      pTCPDPM->_ui16LocalID, true, true);
                    }
                }
                break;
            }

            case PacketType::PMT_TCPCloseConnection:
            {
                const auto * const pCTCPCPM = reinterpret_cast<const CloseTCPConnectionProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveCloseTCPConnectionRequest (pCTCPCPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveCloseTCPConnectionRequest returned with rc = %d\n", rc);
                    if (rc == -1) {
                        _spConnection->sendResetTCPConnectionRequest (0, 0, pCTCPCPM->_ui16RemoteID,
                                                                      pCTCPCPM->_ui16LocalID, true, true);
                    }
                }
                break;
            }

            case PacketType::PMT_TCPResetConnection:
            {
                const auto * const pRTCPCPM = reinterpret_cast<const ResetTCPConnectionProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveResetTCPConnectionRequest (pRTCPCPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveResetTCPConnectionRequest returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_TunnelPacket:
            {
                const auto * const pTPPM = reinterpret_cast<const TunnelPacketProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveTunneledEthernetPacket (pTPPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveTunneledEthernetPacket returned with rc = %d\n", rc);
                }
                break;
            }

            case PacketType::PMT_ConnectionError:
            {
                auto * const pCEPM = reinterpret_cast<const ConnectionErrorProxyMessage *> (pProxyMessage);
                if ((rc = _spConnection->receiveConnectionErrorProxyMessage (pCEPM)) < 0) {
                    checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Warning,
                                    "receiveConnectionErrorProxyMessage returned with rc = %d\n", rc);
                }
                break;
            }

            default:
            {
                checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_MildError,
                                "received a ProxyMessage via %s of %u bytes and unknown type %hhu; the NetProxy will ignore it\n",
                                _spConnection->getConnectorTypeAsString(), static_cast<uint8> (pProxyMessage->getMessageType()), rc);
            }
            }
        }
        checkAndLogMsg ("Connection::IncomingMessageHandler::run", NOMADSUtil::Logger::L_Info,
                        "Incoming Message Handler terminated; termination code is %d\n",
                        getTerminatingResultCode());

        // Clean-up actions before terminating
        if (_bPrepareConnectionForDelete) {
            _spConnection->prepareForDelete();
        }
        _spConnection->deregisterConnectionFromStatisticsManager();

        terminating();
        delete this;
    }

    void Connection::BackgroundConnectionThread::run (void)
    {
        int rc;
        {
            std::lock (_spConnection->_mtxConnection, _spConnection->_mtxBCThread);
            std::unique_lock<std::mutex> ulConnection{_spConnection->_mtxConnection, std::adopt_lock};
            std::unique_lock<std::mutex> ulBCThread{_spConnection->_mtxBCThread, std::adopt_lock};
            rc = _spConnection->doConnect (_spConnection);
        }

        if (rc == 0) {
            // Successful connection --> wake up interested threads
            _spConnection->_rPacketRouter.wakeUpAutoConnectionAndRemoteTransmitterThreads();
        }
        else {
            // Error
            checkAndLogMsg ("MocketConnection::BackgroundConnectionThread::run", NOMADSUtil::Logger::L_MildError,
                            "doConnect() failed with rc = %d\n", rc);
        }

        delete this;
    }

    // Passive (CHR_Server) constructor
    Connection::Connection (const NOMADSUtil::InetAddr & iaLocalInterfaceInetAddr, const NOMADSUtil::InetAddr & iaRemoteInterfaceInetAddr,
                            ConnectorAdapter * const pConnectorAdapter, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                            TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        _connectorType{pConnectorAdapter->getConnectorType()}, _encryptionType{pConnectorAdapter->getEncryptionType()}, _localHostRole{CHR_Server},
        _status{CS_NotConnected}, _ui32RemoteNetProxyUID{0}, _iaLocalInterfaceAddress{iaLocalInterfaceInetAddr},
        _iaRemoteProxyAddress{iaRemoteInterfaceInetAddr}, _sEdgeUUID{""}, _bTerminationRequested{false}, _pConnectorAdapter{pConnectorAdapter},
        _pBCThread{nullptr}, _pIncomingMessageHandler{nullptr}, _rConnectionManager{rConnectionManager}, _rTCPConnTable{rTCPConnTable},
        _rTCPManager{rTCPManager}, _rPacketRouter{rPacketRouter}, _rStatisticsManager{rStatisticsManager}, _pucMultipleUDPDatagramsBuffer{0, }
    { }

    // Active (CHR_Client) constructor
    Connection::Connection (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr & iaLocalInterfaceAddress,
                            const NOMADSUtil::InetAddr & iaRemoteInterfaceAddress, ConnectorAdapter * const pConnectorAdapter,
                            ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                            PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager) :
        _connectorType{pConnectorAdapter->getConnectorType()}, _encryptionType{pConnectorAdapter->getEncryptionType()}, _localHostRole{CHR_Client},
        _status{CS_NotConnected}, _ui32RemoteNetProxyUID{ui32RemoteNetProxyUniqueID}, _iaLocalInterfaceAddress{iaLocalInterfaceAddress},
        _iaRemoteProxyAddress{iaRemoteInterfaceAddress}, _iaRemoteInterfaceLocalAddress{iaRemoteInterfaceAddress}, _sEdgeUUID{""},
        _bTerminationRequested{false}, _pConnectorAdapter{pConnectorAdapter}, _pBCThread{nullptr}, _pIncomingMessageHandler{nullptr},
        _rConnectionManager{rConnectionManager}, _rTCPConnTable{rTCPConnTable}, _rTCPManager{rTCPManager}, _rPacketRouter{rPacketRouter},
        _rStatisticsManager{rStatisticsManager}, _pucMultipleUDPDatagramsBuffer{0, }
    {
        _iaLocalInterfaceAddress.setPort (_pConnectorAdapter->getLocalPort());
    }

    Connection::~Connection (void)
    {
        if (_connectorType != CT_UDPSOCKET) {
            std::lock (_mtxBCThread, _mtxConnection);
            std::lock_guard<std::mutex> lgBTC{_mtxBCThread, std::adopt_lock};
            std::lock_guard<std::mutex> lgConnection{_mtxConnection, std::adopt_lock};

            delete _pConnectorAdapter;
        }
        else {
            // Deregistration from the StatisticsManager for non-UDP Connection instances is done by the IMHT
            deregisterConnectionFromStatisticsManager();
        }
        checkAndLogMsg ("Connection::~Connection", NOMADSUtil::Logger::L_Info,
                        "deleted the %sConnection object to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort());
    }

    int Connection::createAndStartIncomingMessageHandler (const std::shared_ptr<Connection> & spConnection)
    {
        if (getConnectorType() == CT_UDPSOCKET) {
            return 0;
        }

        if (!_pIncomingMessageHandler) {
            _pIncomingMessageHandler = new IncomingMessageHandler{spConnection};
            if (!_pIncomingMessageHandler) {
                checkAndLogMsg ("Connection::startIncomingMessageHandler", NOMADSUtil::Logger::L_Info,
                                "could not create the IncomingMessageHandler object to handle messages sent via %s by "
                                "the remote NetProxy with address <%s:%hu>; rc = %d\n", getConnectorTypeAsString(),
                                getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteInterfaceLocalInetAddr()->getPort());
                return -1;
            }
        }

        int rc;
        if ((rc = _pIncomingMessageHandler->start()) != 0) {
            checkAndLogMsg ("Connection::startIncomingMessageHandler", NOMADSUtil::Logger::L_Info,
                            "could not start the IncomingMessageHandler thread to handle messages sent via %s by "
                            "the remote NetProxy with address <%s:%hu>; rc = %d\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteInterfaceLocalInetAddr()->getPort());
            delete _pIncomingMessageHandler;
            _pIncomingMessageHandler = nullptr;
            return -2;
        }

        return 0;
    }

    int Connection::connectSync (const std::shared_ptr<Connection> & spConnection)
    {
        std::lock_guard<std::mutex> lg{_mtxBCThread};

        if ((_status == CS_Connecting) || (_status == CS_Connected)) {
            checkAndLogMsg ("Connection::connectSync", NOMADSUtil::Logger::L_MildError,
                            "cannot attempt to establish a new connection while already in status %d\n",
                            static_cast<int> (_status));
            return -1;
        }

        int rc;
        setStatus (CS_Connecting);
        if ((rc = doConnect (spConnection)) < 0) {
            checkAndLogMsg ("Connection::connectSync", NOMADSUtil::Logger::L_Warning,
                            "doConnect() failed with rc = %d\n", rc);
            return -2;
        }
        // Successful connection --> wake up interested threads
        _rPacketRouter.wakeUpAutoConnectionAndRemoteTransmitterThreads();

        return 0;
    }

    int Connection::connectAsync (const std::shared_ptr<Connection> & spConnection)
    {
        std::lock_guard<std::mutex> lg{_mtxBCThread};

        if (_status == CS_Connected) {
            checkAndLogMsg ("Connection::connectAsync", NOMADSUtil::Logger::L_LowDetailDebug,
                            "connection already in the Connected state\n");
            return 0;
        }

        if (_pBCThread) {
            checkAndLogMsg ("Connection::connectAsync", NOMADSUtil::Logger::L_MildError,
                            "BackgroundConnectionThread instance already exists! This should never happen\n");
            return -1;
        }
        if (_status == CS_Connecting) {
            checkAndLogMsg ("Connection::connectAsync", NOMADSUtil::Logger::L_MildError,
                            "Connection already in the Connecting state\n");
            return -2;
        }

        setStatus (CS_Connecting);
        _pBCThread = new BackgroundConnectionThread (spConnection);
        if (_pBCThread == nullptr) {
            checkAndLogMsg ("Connection::connectAsync", NOMADSUtil::Logger::L_MildError,
                            "Impossible to create a new BackgroundConnectionThread object\n");
            return -3;
        }
        _pBCThread->setName ("Background Connection Thread");
        _pBCThread->start();

        return 0;
    }

    int Connection::waitForConnectionEstablishment (void)
    {
        if (isConnected()) {
            return 0;
        }

        // Synchronize with the BackgroundConnectionThread
        while (_status == CS_Connecting) {
            _mtxBCThread.lock();
            _mtxBCThread.unlock();
        }

        return isConnected() ? 0 : -1;
    }

    bool Connection::isEnqueueingAllowed (void) const
    {
        if (_connectorType == CT_UDPSOCKET) {
            auto * const pUDPSocketAdapter = dynamic_cast<UDPSocketAdapter *> (_pConnectorAdapter);
            return pUDPSocketAdapter->isEnqueueingAllowed();
        }

        return true;
    }
    int Connection::receiveInitializeConnectionProxyMessage (const InitializeConnectionProxyMessage * const pICPM)
    {
        NOMADSUtil::InetAddr iaRemoteInterfaceLocalAddress{pICPM->_ui32LocalInterfaceIPv4Address,
                                                           getRemoteNetProxyInetAddr()->getPort()};
        checkAndLogMsg ("Connection::receiveInitializeConnectionProxyMessage", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "received an InitializeConnection ProxyMessage via %s from the remote NetProxy at address <%s:%hu>, "
                        "with remote interface local address %s and ProxyUniqueID %u\n", getConnectorTypeAsString(),
                        getRemoteNetProxyInetAddr()->getIPAsString(), getRemoteNetProxyInetAddr()->getPort(),
                        iaRemoteInterfaceLocalAddress.getIPAsString(), pICPM->_ui32ProxyUniqueID);

        std::lock_guard<std::mutex> lg{_mtxConnection};
        setStatus (CS_Connected);
        setRemoteInterfaceLocalAddress (iaRemoteInterfaceLocalAddress);

        // Extract interfaces information
        std::vector<uint32> vInterfaceIPv4Addresses;
        for (int i = 0; i < pICPM->_ui8NumberOfInterfaces; ++i) {
            uint32 ui32InterfaceIPv4Address;
            std::copy (static_cast<const char *> (static_cast<const void *> (&pICPM->_aui8Data[i * sizeof(uint32)])),
                       static_cast<const char *> (static_cast<const void *> (&pICPM->_aui8Data[(i+1) * sizeof(uint32)])),
                       static_cast<char *> (static_cast<void *> (&ui32InterfaceIPv4Address)));
            vInterfaceIPv4Addresses.push_back (ui32InterfaceIPv4Address);
        }

        // Initialize local Connection instance and update info about the remote NetProxy
        if (updateUninitializedConnection (_rConnectionManager, _rTCPConnTable, this, pICPM->_ui32ProxyUniqueID) < 0) {
            // An error was detected --> we need to terminate this Connection
            requestTermination_NoLock (false);      // Lock acquired above; ungraceful close because this NetProxy acts as the server
            return -1;
        }
        if (updateRemoteProxyInformation (pICPM->_ui32ProxyUniqueID, iaRemoteInterfaceLocalAddress, vInterfaceIPv4Addresses,
                                          pICPM->_ui16LocalMocketsServerPort, pICPM->_ui16LocalTCPServerPort,
                                          pICPM->_ui16LocalUDPServerPort, pICPM->getRemoteProxyReachability()) < 0) {
            // An error was detected --> we need to terminate this Connection
            requestTermination_NoLock (false);      // Lock acquired above; ungraceful close because this NetProxy acts as the server
            return -2;
        }

        // Answer with a ConnectionInitialized ProxyMessage
        if (_rPacketRouter.initializeRemoteConnection (pICPM->_ui32ProxyUniqueID, getLocalInterfaceInetAddr()->getIPAddress(),
                                                       getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                       getConnectorType(), getEncryptionType()) < 0) {
            // An error was detected --> we need to terminate this Connection
            requestTermination_NoLock (false);      // Lock acquired above; ungraceful close because this NetProxy acts as the server
            return -3;
        }

        checkAndLogMsg ("Connection::receiveInitializeConnectionProxyMessage", NOMADSUtil::Logger::L_LowDetailDebug,
                        "successfully replied with a ConnectionInitialized ProxyMessage to the remote NetProxy "
                        "with UniqueID %u at address <%s:%hu> from the local interface with address %s\n",
                        getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAsString());

        checkAndLogMsg ("Connection::receiveInitializeConnectionProxyMessage", NOMADSUtil::Logger::L_Info,
                        "successfully initialized the connection opened by the remote NetProxy with "
                        "UniqueID %u from the remote address <%s:%hu> to the local address <%s:%hu>\n",
                        getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAsString(),
                        getLocalInterfaceInetAddr()->getPort());
        return 0;
    }

    int Connection::receiveConnectionInitializedProxyMessage (const ConnectionInitializedProxyMessage * const pCIPM)
    {
        NOMADSUtil::InetAddr iaRemoteInterfaceLocalAddress{pCIPM->_ui32LocalInterfaceIPv4Address,
                                                           getRemoteNetProxyInetAddr()->getPort()};
        checkAndLogMsg ("Connection::receiveConnectionInitializedProxyMessage", NOMADSUtil::Logger::L_LowDetailDebug,
                        "received a ConnectionInitialized ProxyMessage via %s from the remote NetProxy at the address <%s:%hu> "
                        "with UniqueID %u\n", getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort(), pCIPM->_ui32ProxyUniqueID);

        if (iaRemoteInterfaceLocalAddress.getIPAddress() != getRemoteInterfaceLocalInetAddr()->getIPAddress()) {
            checkAndLogMsg ("Connection::receiveConnectionInitializedProxyMessage", NOMADSUtil::Logger::L_Info,
                            "received a ConnectionInitializedProxyMessage from the remote NetProxy with address %s and "
                            "ProxyUniqueID %u that contains a different IPv4 local address for the remote interface: "
                            "configured address is %s, signaled address is %s; NetProxy will update it\n",
                            getRemoteNetProxyInetAddr()->getIPAsString(), pCIPM->_ui32ProxyUniqueID,
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            iaRemoteInterfaceLocalAddress.getIPAsString());
        }
        setRemoteInterfaceLocalAddress (iaRemoteInterfaceLocalAddress);

        // Extract interfaces information
        std::vector<uint32> vInterfaceIPv4Addresses;
        for (int i = 0; i < pCIPM->_ui8NumberOfInterfaces; ++i) {
            uint32 ui32InterfaceIPv4Address;
            std::copy (static_cast<const char *> (static_cast<const void *> (&pCIPM->_aui8Data[i * sizeof(uint32)])),
                       static_cast<const char *> (static_cast<const void *> (&pCIPM->_aui8Data[(i+1) * sizeof(uint32)])),
                       static_cast<char *> (static_cast<void *> (&ui32InterfaceIPv4Address)));
            vInterfaceIPv4Addresses.push_back (ui32InterfaceIPv4Address);
        }

        // Update local Connection instance and info about the remote NetProxy
        if (getRemoteNetProxyID() != pCIPM->_ui32ProxyUniqueID) {
            // The configured UniqueID for the remote NetProxy is wrong; the InitializedConnectionsTable needs to be updated
            int rc;
            if ((rc = updateInitializedConnection (this, getRemoteNetProxyID(), pCIPM->_ui32ProxyUniqueID)) < 0) {
                checkAndLogMsg ("Connection::receiveConnectionInitializedProxyMessage", NOMADSUtil::Logger::L_Info,
                                "updateInitializedConnection() failed with rc = %d when updating an initialized "
                                "Connection to the remote NetProxy with address %s and UniqueID %u\n", rc,
                                getRemoteInterfaceLocalInetAddr()->getIPAsString(), pCIPM->_ui32ProxyUniqueID);
                // An error was detected --> we need to terminate this Connection
                requestTermination (true);      // Need to lock this Connection instance
                return -1;
            }
        }
        if (updateRemoteProxyInformation (pCIPM->_ui32ProxyUniqueID, iaRemoteInterfaceLocalAddress, vInterfaceIPv4Addresses,
                                          pCIPM->_ui16LocalMocketsServerPort, pCIPM->_ui16LocalTCPServerPort,
                                          pCIPM->_ui16LocalUDPServerPort, pCIPM->getRemoteProxyReachability()) < 0) {
            // An error was detected --> we need to terminate this Connection
            requestTermination (true);      // Need to lock this Connection instance
            return -2;
        }

        return 0;
    }

    int Connection::receiveICMPProxyMessageToRemoteHost (const ICMPProxyMessage * const pICMPPM)
    {
        checkAndLogMsg ("Connection::receiveICMPProxyMessageToRemoteHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "received an ICMP ProxyMessage via %s of type %hhu, code %hhu, TTL %hhu, and %hu bytes of "
                        "data from the remote NetProxy with address %s and UniqueID %u\n", getConnectorTypeAsString(),
                        pICMPPM->_ui8Type, pICMPPM->_ui8Code, pICMPPM->_ui8PacketTTL, pICMPPM->getPayloadLen(),
                        getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteNetProxyID());

        if ((pICMPPM->_ui8PacketTTL == 0) || ((pICMPPM->_ui8PacketTTL == 1) &&
            !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
            checkAndLogMsg ("Connection::receiveICMPProxyMessageToRemoteHost", NOMADSUtil::Logger::L_LowDetailDebug,
                            "the ICMP message contained in the ICMP ProxyMessage has reached a TTL "
                            "value of %hhu and it needs to be discarded", pICMPPM->_ui8PacketTTL);
            return 0;
        }

        int rc;
        const uint8 ui8PacketTTL = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ?
            pICMPPM->_ui8PacketTTL : (pICMPPM->_ui8PacketTTL - 1);
        if (0 != (rc = _rPacketRouter.forwardICMPMessageToHost (pICMPPM->_ui32RemoteIP, pICMPPM->_ui32LocalIP,
                                                                getRemoteNetProxyInetAddr()->getIPAddress(), ui8PacketTTL,
                                                                static_cast<NOMADSUtil::ICMPHeader::Type> (pICMPPM->_ui8Type),
                                                                static_cast<NOMADSUtil::ICMPHeader::Code_Destination_Unreachable> (pICMPPM->_ui8Code),
                                                                pICMPPM->_ui32RoH, pICMPPM->_aui8Data, pICMPPM->getPayloadLen()))) {
            checkAndLogMsg ("Connection::receiveICMPProxyMessageToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "forwardICMPMessageToHost() failed with rc = %d\n", rc);
            return -1;
        }

        return 0;
    }

    int Connection::receiveUDPUnicastPacketToRemoteHost (const UDPUnicastDataProxyMessage * const pUDPUcDPM)
    {
        checkAndLogMsg ("Connection::receiveUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "received a UDPUnicastData ProxyMessage via %s with TTL %hhu and %hu bytes of data from the remote "
                        "NetProxy with address %s and UniqueID %u\n", getConnectorTypeAsString(), pUDPUcDPM->_ui8PacketTTL,
                        pUDPUcDPM->getPayloadLen(), getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteNetProxyID());

        if ((pUDPUcDPM->_ui8PacketTTL == 0) || ((pUDPUcDPM->_ui8PacketTTL == 1) &&
            !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
            checkAndLogMsg ("Connection::receiveUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_LowDetailDebug,
                            "the UDP datagram contained in the UDPUnicastData ProxyMessage has reached a TTL value of %hhu "
                            "and it needs to be discarded", pUDPUcDPM->_ui8PacketTTL);
            return 0;
        }

        int rc;
        unsigned char * pucBuf[1];
        uint32 ui32BufLen;
        const uint8 ui8PacketTTL = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ?
            pUDPUcDPM->_ui8PacketTTL : (pUDPUcDPM->_ui8PacketTTL - 1);
        auto * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPUcDPM->_ui8CompressionTypeAndLevel);
        pConnectorReader->receiveTCPDataProxyMessage (pUDPUcDPM->_aui8Data, pUDPUcDPM->getPayloadLen(), pucBuf, ui32BufLen);
        if (0 != (rc = _rPacketRouter.sendUDPUniCastPacketToHost (pUDPUcDPM->_ui32LocalIP, pUDPUcDPM->_ui32RemoteIP,
                                                                  ui8PacketTTL, reinterpret_cast<NOMADSUtil::UDPHeader *> (pucBuf[0])))) {
            checkAndLogMsg ("Connection::receiveUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
            pConnectorReader->resetAndUnlockConnectorReader();
            return -1;
        }
        pConnectorReader->resetAndUnlockConnectorReader();

        return 0;
    }

    int Connection::receiveMultipleUDPDatagramsToRemoteHost (const MultipleUDPDatagramsProxyMessage * const pMUDPDPM)
    {
        checkAndLogMsg ("Connection::receiveMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "received a MultipleUDPDatagrams ProxyMessage via %s containing %hhu UDP packets for a "
                        "total of %hu bytes of data from the remote NetProxy with address %s and UniqueID %u\n",
                        getConnectorTypeAsString(), pMUDPDPM->_ui8WrappedUDPDatagramsNum, pMUDPDPM->getPayloadLen(),
                        getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteNetProxyID());

        int rc;
        uint16 ui16UDPDatagramOffset = 0;
        uint32 ui32BufLen;
        unsigned char * pucBuf[1];
        const NOMADSUtil::UDPHeader * pUDPHeader = nullptr;
        auto * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pMUDPDPM->_ui8CompressionTypeAndLevel);
        pConnectorReader->receiveTCPDataProxyMessage (pMUDPDPM->_aui8Data, pMUDPDPM->getPayloadLen(), pucBuf, ui32BufLen);
        for (uint8 i = 0; i < pMUDPDPM->_ui8WrappedUDPDatagramsNum; ++i) {
            pUDPHeader = reinterpret_cast<NOMADSUtil::UDPHeader *> (pucBuf[0] + pMUDPDPM->_ui8WrappedUDPDatagramsNum + ui16UDPDatagramOffset);
            ui16UDPDatagramOffset += pUDPHeader->ui16Len;

            if ((pucBuf[0][i] == 0) || ((pucBuf[0][i] == 1) && !NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE)) {
                checkAndLogMsg ("Connection::receiveMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_LowDetailDebug,
                                "the UDP datagram with index %hhu contained in the MultipleUDPDatagrams ProxyMessage has reached a "
                                "TTL value of %hhu and it needs to be discarded", i, pucBuf[0][i]);
                continue;
            }
            pucBuf[0][i] = NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? pucBuf[0][i] : pucBuf[0][i] - 1;

            if (0 != (rc = _rPacketRouter.sendUDPUniCastPacketToHost (pMUDPDPM->_ui32LocalIP, pMUDPDPM->_ui32RemoteIP,
                                                                      pucBuf[0][i], pUDPHeader))) {
                checkAndLogMsg ("Connection::receiveMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MildError,
                                "sendUDPUniCastPacketToHost() failed with rc = %d\n", rc);
                pConnectorReader->resetAndUnlockConnectorReader();
                return -1;
            }
        }
        pConnectorReader->resetAndUnlockConnectorReader();
        checkAndLogMsg ("Connection::receiveMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "%hhu UDP datagrams, for a total of %u bytes, have been sent to local host\n",
                        pMUDPDPM->_ui8WrappedUDPDatagramsNum, ui32BufLen);

        return 0;
    }

    int Connection::receiveUDPBCastMCastPacketToRemoteHost (const UDPBCastMCastDataProxyMessage * const pUDPBcMcDPM)
    {
        checkAndLogMsg ("Connection::receiveUDPBCastMCastPacketToRemoteHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "received a UDPBroadcastMulticastData ProxyMessage via %s with %hu bytes of data from "
                        "the remote NetProxy with address %s and UniqueID %u\n", getConnectorTypeAsString(),
                        pUDPBcMcDPM->getPayloadLen(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteNetProxyID());

        int rc;
        uint32 ui32BufLen;
        unsigned char *pucBuf[1];
        ConnectorReader * const pConnectorReader = ConnectorReader::getAndLockUDPConnectorReader (pUDPBcMcDPM->_ui8CompressionTypeAndLevel);
        pConnectorReader->receiveTCPDataProxyMessage (pUDPBcMcDPM->_aui8Data, pUDPBcMcDPM->getPayloadLen(), pucBuf, ui32BufLen);
        if (0 != (rc = _rPacketRouter.sendUDPBCastMCastPacketToHost (pucBuf[0], ui32BufLen))) {
            checkAndLogMsg ("Connection::receiveUDPBCastMCastPacketToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "sendUDPBCastMCastPacketToHost() failed with rc = %d\n", rc);
            pConnectorReader->resetAndUnlockConnectorReader();
            return -1;
        }
        pConnectorReader->resetAndUnlockConnectorReader();

        return 0;
    }

    int Connection::receiveOpenTCPConnectionRequest (const OpenTCPConnectionProxyMessage * const pOTCPCPM)
    {
        checkAndLogMsg ("Connection::receiveOpenTCPConnectionRequest", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L0-R%hu: received an OpenTCPConnection ProxyMessage via %s from the remote NetProxy with "
                        "address %s and UniqueID %u; compression type code is %hhu and compression level is %hhu\n",
                        pOTCPCPM->_ui16LocalID, getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteNetProxyID(), (pOTCPCPM->_ui8CompressionTypeAndLevel & ProxyMessage::COMPRESSION_TYPE_FLAGS_MASK),
                        (pOTCPCPM->_ui8CompressionTypeAndLevel & ProxyMessage::COMPRESSION_LEVEL_FLAGS_MASK));

        int rc;
        if (0 != (rc = _rTCPManager.openTCPConnectionToHost (getRemoteNetProxyInetAddr()->getIPAddress(), getRemoteNetProxyID(),
                                                             pOTCPCPM->_ui16LocalID, pOTCPCPM->_ui32RemoteIP, pOTCPCPM->_ui16RemotePort,
                                                             pOTCPCPM->_ui32LocalIP, pOTCPCPM->_ui16LocalPort,
                                                             pOTCPCPM->_ui8CompressionTypeAndLevel))) {
            checkAndLogMsg ("Connection::receiveOpenTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "L0-R%hu: openTCPConnectionToHost() failed with rc = %d\n",
                            pOTCPCPM->_ui16LocalID, rc);
            sendResetTCPConnectionRequest (pOTCPCPM->_ui32RemoteIP, pOTCPCPM->_ui32LocalIP, 0, pOTCPCPM->_ui16LocalID, true, false);
            return rc;
        }

        return 0;
    }

    int Connection::receiveTCPConnectionOpenedResponse (const TCPConnectionOpenedProxyMessage * const pTCPCOPM)
    {
        checkAndLogMsg ("Connection::receiveTCPConnectionOpenedResponse", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L%hu-R%hu: received a TCPConnectionOpened ProxyMessage via %s from the remote NetProxy with address %s "
                        "and UniqueID %u; compression type code is %hhu and compression level is %hhu\n", pTCPCOPM->_ui16RemoteID,
                        pTCPCOPM->_ui16LocalID, getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteNetProxyID(), (pTCPCOPM->_ui8CompressionTypeAndLevel & ProxyMessage::COMPRESSION_TYPE_FLAGS_MASK),
                        (pTCPCOPM->_ui8CompressionTypeAndLevel & ProxyMessage::COMPRESSION_LEVEL_FLAGS_MASK));

        int rc;
        if (0 != (rc = _rTCPManager.confirmTCPConnectionToHostOpened (pTCPCOPM->_ui16RemoteID, pTCPCOPM->_ui16LocalID,
                                                                      getRemoteNetProxyID(), pTCPCOPM->_ui8CompressionTypeAndLevel))) {
            checkAndLogMsg ("Connection::receiveTCPConnectionOpenedResponse", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: confirmTCPConnectionToHostOpened() failed with rc = %d\n",
                            pTCPCOPM->_ui16RemoteID, pTCPCOPM->_ui16LocalID, rc);
            sendResetTCPConnectionRequest (0, 0, pTCPCOPM->_ui16RemoteID, pTCPCOPM->_ui16LocalID, true, false);
            return rc;
        }

        return 0;
    }

    int Connection::receiveTCPDataAndSendToLocalHost (const TCPDataProxyMessage * const pTCPDPM)
    {
        checkAndLogMsg ("Connection::receiveTCPDataAndSendToLocalHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "L%hu-R%hu: received a TCPData ProxyMessage with %hu bytes of data\n",
                        pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, pTCPDPM->getPayloadLen());

        int rc;
        if (0 != (rc = _rTCPManager.sendTCPDataToHost (pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, pTCPDPM->_aui8Data, pTCPDPM->getPayloadLen(),
                                                       pTCPDPM->_ui8TCPFlags, _rPacketRouter.getLocalTCPTransmitterThread()))) {
            checkAndLogMsg ("Connection::receiveTCPDataAndSendToLocalHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: sendTCPDataToHost() failed with rc = %d\n",
                            pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, rc);
            sendResetTCPConnectionRequest (0, 0, pTCPDPM->_ui16RemoteID, pTCPDPM->_ui16LocalID, true, false);
            return rc;
        }

        return 0;
    }

    int Connection::receiveCloseTCPConnectionRequest (const CloseTCPConnectionProxyMessage * const pCTCPCPM)
    {
        checkAndLogMsg ("Connection::receiveCloseTCPConnectionRequest", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L%hu-R%hu: received a CloseTCPConnection ProxyMessage via %s from remote proxy\n",
                        pCTCPCPM->_ui16RemoteID, pCTCPCPM->_ui16LocalID, getConnectorTypeAsString());

        int rc;
        if (0 != (rc = _rTCPManager.closeTCPConnectionToHost (pCTCPCPM->_ui16RemoteID, pCTCPCPM->_ui16LocalID,
                                                              _rPacketRouter.getRemoteTCPTransmitterThread()))) {
            checkAndLogMsg ("Connection::receiveCloseTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "closeTCPConnectionToHost() failed with rc = %d\n", rc);
            return rc;
        }

        return 0;
    }

    int Connection::receiveResetTCPConnectionRequest (const ResetTCPConnectionProxyMessage * const pRTCPCPM)
    {
        checkAndLogMsg ("Connection::receiveResetTCPConnectionRequest", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L%hu-R%hu: received a ResetTCPconnection ProxyMessage via %s from remote proxy\n",
                        pRTCPCPM->_ui16RemoteID, pRTCPCPM->_ui16LocalID, getConnectorTypeAsString());

        int rc;
        if (0 != (rc = _rTCPManager.resetTCPConnectionToHost (pRTCPCPM->_ui16RemoteID, pRTCPCPM->_ui16LocalID))) {
            checkAndLogMsg ("Connection::receiveResetTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "resetTCPConnectionToHost() failed with rc = %d\n", rc);
            return -1;
        }
        removeTCPTypePacketsFromTransmissionQueue (pRTCPCPM->_ui16RemoteID, pRTCPCPM->_ui16LocalID);

        return 0;
    }

    int Connection::receiveTunneledEthernetPacket (const TunnelPacketProxyMessage * const pTPPM)
    {
        checkAndLogMsg ("Connection::receiveTunneledEthernetPacket", NOMADSUtil::Logger::L_LowDetailDebug,
                        "received a TunnelPacketProxyMessage via %s from remote proxy\n",
                        getConnectorTypeAsString());

        int rc;
        if (0 != (rc = _rPacketRouter.sendTunneledPacketToLocalHost (_rPacketRouter.getInternalNetworkInterface().get(),
                                                                     pTPPM->_aui8Data, pTPPM->getPayloadLen()))) {
            checkAndLogMsg ("Connection::receiveTunneledEthernetPacket", NOMADSUtil::Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            return -1;
        }

        return 0;
    }

    int Connection::receiveConnectionErrorProxyMessage (const ConnectionErrorProxyMessage * const pCEPM)
    {
        checkAndLogMsg ("Connection::receiveConnectionErrorProxyMessage", NOMADSUtil::Logger::L_Warning,
                        "received a ConnectionError ProxyMessage from the remote NetProxy; this message "
                        "is ignored for connections over %s\n", getConnectorTypeAsString());

        return 0;
    }

    int Connection::sendInitializeConnectionProxyMessage (const std::vector<uint32> & vui32InterfaceIPv4Address, bool bLocalReachabilityFromRemote)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendInitializeConnectionProxyMessage", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnecting()) {
            checkAndLogMsg ("Connection::sendInitializeConnectionProxyMessage", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -2;
        }


        int rc;
        InitializeConnectionProxyMessage iCPM{NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, getLocalInterfaceInetAddr()->getIPAddress(),
                                              NetProxyApplicationParameters::MOCKET_SERVER_PORT, NetProxyApplicationParameters::TCP_SERVER_PORT,
                                              NetProxyApplicationParameters::UDP_SERVER_PORT, static_cast<uint8> (vui32InterfaceIPv4Address.size()),
                                              bLocalReachabilityFromRemote};
        const size_t stVectorElementSize = sizeof(std::remove_reference<decltype (vui32InterfaceIPv4Address)>::type::value_type);
        const size_t stPayloadBytes = iCPM._ui8NumberOfInterfaces * stVectorElementSize;
        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), 0, 0, true, true, &iCPM, sizeof(InitializeConnectionProxyMessage),
                                                  vui32InterfaceIPv4Address.data(), stPayloadBytes, nullptr))) {
            checkAndLogMsg ("Connection::sendInitializeConnectionProxyMessage", NOMADSUtil::Logger::L_MildError,
                            "send() of an InitializeConnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> "
                            "failed with rc = %d\n", getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), rc);
            return -3;
        }
        checkAndLogMsg ("Connection::sendInitializeConnectionProxyMessage", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully sent an InitializeConnection ProxyMessage via %s to the remote NetProxy with address "
                        "<%s:%hu>\n", getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort());

        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_ICMP, sizeof(InitializeConnectionProxyMessage) + stPayloadBytes);
        return 0;
    }

    int Connection::sendConnectionInitializedProxyMessage (const std::vector<uint32> & vui32InterfaceIPv4Address, bool bLocalReachabilityFromRemote)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendConnectionInitializedProxyMessage", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendConnectionInitializedProxyMessage", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -2;
        }

        int rc;
        ConnectionInitializedProxyMessage cIPM{NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, getLocalInterfaceInetAddr()->getIPAddress(),
                                               NetProxyApplicationParameters::MOCKET_SERVER_PORT, NetProxyApplicationParameters::TCP_SERVER_PORT,
                                               NetProxyApplicationParameters::UDP_SERVER_PORT, static_cast<uint8> (vui32InterfaceIPv4Address.size()),
                                               bLocalReachabilityFromRemote};
        const size_t stVectorElementSize = sizeof(std::remove_reference<decltype (vui32InterfaceIPv4Address)>::type::value_type);
        const size_t stPayloadBytes = cIPM._ui8NumberOfInterfaces * stVectorElementSize;
        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), 0, 0, true, true, &cIPM, sizeof(ConnectionInitializedProxyMessage),
                                                  vui32InterfaceIPv4Address.data(), stPayloadBytes, nullptr))) {
            checkAndLogMsg ("Connection::sendConnectionInitializedProxyMessage", NOMADSUtil::Logger::L_MildError,
                            "send() of a ConnectionInitialized ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteInterfaceLocalInetAddr()->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendConnectionInitializedProxyMessage", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully sent a ConnectionInitialized ProxyMessage via %s to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort());

        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_ICMP, sizeof(ConnectionInitializedProxyMessage) + stPayloadBytes);
        return 0;
    }

    int Connection::sendICMPProxyMessageToRemoteHost (uint8 ui8Type, uint8 ui8Code, uint32 ui32RoH, uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP,
                                                      uint8 ui8PacketTTL, const uint8 * const pui8Buf, uint16 ui16BufLen, Protocol ui8Protocol,
                                                      bool bLocalReachabilityFromRemote)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -2;
        }

        int rc;
        bool bReliable = isProtocolReliable (ui8Protocol);
        bool bSequenced = isProtocolSequenced (ui8Protocol);
        ICMPProxyMessage iCMPPM{ui16BufLen, ui8Type, ui8Code, ui32RoH, ui32LocalSourceIP, ui32RemoteDestinationIP, ui8PacketTTL, bLocalReachabilityFromRemote};
        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), ui32LocalSourceIP, ui32RemoteDestinationIP, bReliable, bSequenced, &iCMPPM,
                                                  sizeof(ICMPProxyMessage), pui8Buf, iCMPPM._ui16PayloadLen, nullptr))) {
            checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "gsend() of an ICMPProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with "
                            "rc = %d\n", getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), rc);
            return -3;
        }
        checkAndLogMsg ("Connection::sendICMPProxyMessageToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "successfully sent an ICMP ProxyMessage via %s addressed to the remote NetProxy at address <%s:%hu> with parameters: "
                        "bReliable = %s, bSequenced = %s\n", getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort(), bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _rStatisticsManager.increaseInboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                    getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                    getLocalInterfaceInetAddr()->getPort(), PT_ICMP, sizeof(NOMADSUtil::ICMPHeader) + ui16BufLen);
        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_ICMP, sizeof(ICMPProxyMessage) + iCMPPM._ui16PayloadLen);

        return 0;
    }

    int Connection::sendUDPUnicastPacketToRemoteHost (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, uint8 ui8PacketTTL, const uint8 * const pPacket,
                                                      uint16 ui16PacketLen, const CompressionSettings & compressionSettings, Protocol ui8Protocol)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -2;
        }

        int rc;
        bool bReliable = isProtocolReliable (ui8Protocol);
        bool bSequenced = isProtocolSequenced (ui8Protocol);
        UDPUnicastDataProxyMessage udpUDPM{ui16PacketLen, ui32LocalSourceIP, ui32RemoteDestinationIP, ui8PacketTTL,
                                           CompressionSettings::DefaultNOCompressionSetting};

        // Applying compression if convenient
        unsigned char *pucBuf[1], *pucBytesToSend = const_cast<unsigned char *> (pPacket);
        unsigned int uiBufLen = 0;
        ConnectorWriter * const pConnectorWriter = ConnectorWriter::getAndLockUPDConnectorWriter (compressionSettings);
        if (0 == pConnectorWriter->writeDataAndResetWriter (pPacket, ui16PacketLen, pucBuf, uiBufLen)) {
            if ((uiBufLen > 0) && (uiBufLen < ui16PacketLen)) {
                pucBytesToSend = pucBuf[0];
                udpUDPM._ui16PayloadLen = uiBufLen;
                udpUDPM._ui8CompressionTypeAndLevel = compressionSettings.getCompressionTypeAndLevel();

                checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "the compressed datagram is %u bytes long, which is less than the original packet of %hu "
                                "bytes in size; NetProxy will send the datagram compressed through %s level $hhu\n",
                                uiBufLen, ui16PacketLen, compressionSettings.getCompressionTypeAsString(),
                                compressionSettings.getCompressionLevel());
            }
        }
        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), ui32LocalSourceIP, ui32RemoteDestinationIP, bReliable, bSequenced,
                                                  &udpUDPM, sizeof(UDPUnicastDataProxyMessage), pucBytesToSend, udpUDPM._ui16PayloadLen, nullptr))) {
            checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "gsend() of a UDPUnicastData ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed "
                            "with rc = %d\n", getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), rc);
            pConnectorWriter->unlockConnectorWriter();
            return -3;
        }
        pConnectorWriter->unlockConnectorWriter();
        checkAndLogMsg ("Connection::sendUDPUnicastPacketToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "successfully sent a UDPUnicastData ProxyMessage via %s with %hu bytes of data "
                        "(%hu bytes before compression, if any) to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), udpUDPM._ui16PayloadLen, ui16PacketLen,
                        getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteInterfaceLocalInetAddr()->getPort());

        _rStatisticsManager.increaseInboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                    getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                    getLocalInterfaceInetAddr()->getPort(), PT_UDP, ui16PacketLen);
        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_UDP,
                                                     sizeof(UDPUnicastDataProxyMessage) + udpUDPM._ui16PayloadLen);

        return 0;
    }

    int Connection::sendMultipleUDPDatagramsToRemoteHost (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, MutexUDPQueue * const pUDPDatagramsQueue,
                                                          const CompressionSettings & compressionSettings, Protocol ui8Protocol)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteNetProxyInetAddr()->getIPAsString(), getRemoteNetProxyInetAddr()->getPort(),
                            getRemoteNetProxyID());
            return -2;
        }

        int rc;
        const int iPacketsNum = pUDPDatagramsQueue->size();
        if ((sizeof(MultipleUDPDatagramsProxyMessage) + iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount()) >
            NetworkConfigurationSettings::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "impossible to wrap together %d UDP Datagrams for a total of %u bytes; buffer is only %hu bytes long\n",
                            pUDPDatagramsQueue->size(), pUDPDatagramsQueue->getEnqueuedBytesCount(),
                            NetworkConfigurationSettings::PROXY_MESSAGE_MTU);
            return -3;
        }

        bool bReliable = isProtocolReliable (ui8Protocol);
        bool bSequenced = isProtocolSequenced (ui8Protocol);
        MultipleUDPDatagramsProxyMessage mUDPDPM{static_cast<uint16> (iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount()),
                                                 ui32LocalSourceIP, ui32RemoteDestinationIP, static_cast<uint8> (iPacketsNum),
                                                 CompressionSettings::DefaultNOCompressionSetting};

        unsigned int iTTLIndex = 0;
        uint32 ui32CopiedBytes = 0;
        pUDPDatagramsQueue->resetGet();
        while (const UDPDatagramPacket * const pUDPDatagramPacket = pUDPDatagramsQueue->getNext()) {
            _pucMultipleUDPDatagramsBuffer[iTTLIndex++] = pUDPDatagramPacket->getIPPacketTTL();
            // Leave space for the TTL value of each consolidated packet
            memcpy (_pucMultipleUDPDatagramsBuffer + iPacketsNum + ui32CopiedBytes, pUDPDatagramPacket->getUDPPacket(),
                    pUDPDatagramPacket->getPacketLen());
            ui32CopiedBytes += pUDPDatagramPacket->getPacketLen();
        }
        if (ui32CopiedBytes != pUDPDatagramsQueue->getEnqueuedBytesCount()) {
            // Error copying data
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "the value of copied bytes (%u) and bytes to be copied (%u) do not match\n",
                            ui32CopiedBytes, pUDPDatagramsQueue->getEnqueuedBytesCount());
            return -4;
        }

        // Apply compression if convenient
        unsigned char *pucBuf[1], *pucBytesToSend = _pucMultipleUDPDatagramsBuffer;
        unsigned int uiBufLen = 0;
        ConnectorWriter * const pConnectorWriter = ConnectorWriter::getAndLockUPDConnectorWriter (compressionSettings);
        if (0 == pConnectorWriter->writeDataAndResetWriter (_pucMultipleUDPDatagramsBuffer, mUDPDPM.getPayloadLen(), pucBuf, uiBufLen)) {
            if ((uiBufLen > 0) && (uiBufLen < mUDPDPM.getPayloadLen())) {
                pucBytesToSend = pucBuf[0];
                mUDPDPM._ui16PayloadLen = uiBufLen;
                mUDPDPM._ui8CompressionTypeAndLevel = compressionSettings.getCompressionTypeAndLevel();

                checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "the compressed size of %d consolidated UDP datagrams is %u bytes long, which is less than their size before "
                                "compression (%hu bytes); NetProxy will send the consolidated datagrams compressed through %s level $hhu\n",
                                iPacketsNum, uiBufLen, (iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount()),
                                compressionSettings.getCompressionTypeAsString(), compressionSettings.getCompressionLevel());
            }
        }
        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), ui32LocalSourceIP, ui32RemoteDestinationIP, bReliable, bSequenced, &mUDPDPM,
                                                  sizeof(MultipleUDPDatagramsProxyMessage), pucBytesToSend, mUDPDPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "gsend() of a MultipleUDPDatagram ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteInterfaceLocalInetAddr()->getPort(), rc);
            pConnectorWriter->unlockConnectorWriter();
            return -5;
        }
        pConnectorWriter->unlockConnectorWriter();
        checkAndLogMsg ("Connection::sendMultipleUDPDatagramsToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "successfully sent a MultipleUDPDatagram ProxyMessage via %s with %hu bytes of data (%hu bytes before "
                        "compression, if any) to the remote NetProxy at address <%s:%hu>\n", getConnectorTypeAsString(),
                        mUDPDPM.getPayloadLen(), iPacketsNum + pUDPDatagramsQueue->getEnqueuedBytesCount(),
                        getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteInterfaceLocalInetAddr()->getPort());

        _rStatisticsManager.increaseInboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                    getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                    getLocalInterfaceInetAddr()->getPort(), PT_UDP, pUDPDatagramsQueue->getEnqueuedBytesCount());
        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_UDP,
                                                     sizeof(MultipleUDPDatagramsProxyMessage) + mUDPDPM.getPayloadLen());

        return 0;
    }

    int Connection::sendUDPBCastMCastPacketToRemoteHost (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, const uint8 * const pPacket,
                                                         uint16 ui16PacketLen, const CompressionSettings & compressionSettings,
                                                         Protocol ui8Protocol)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -2;
        }

        int rc;
        unsigned int uiBufLen = 0;
        bool bReliable = isProtocolReliable (ui8Protocol);
        bool bSequenced = isProtocolSequenced (ui8Protocol);
        UDPBCastMCastDataProxyMessage udpBCMCPM{ui16PacketLen, CompressionSettings::DefaultNOCompressionSetting};

        // Apply compression if convenient
        unsigned char *pucBuf[1], *pucBytesToSend = const_cast <unsigned char *> (pPacket);
        ConnectorWriter * const pConnectorWriter = ConnectorWriter::getAndLockUPDConnectorWriter (compressionSettings);
        if (0 == pConnectorWriter->writeDataAndResetWriter (pPacket, ui16PacketLen, pucBuf, uiBufLen)) {
            if ((uiBufLen > 0) && (uiBufLen < ui16PacketLen)) {
                pucBytesToSend = pucBuf[0];
                udpBCMCPM._ui16PayloadLen = uiBufLen;
                udpBCMCPM._ui8CompressionTypeAndLevel = compressionSettings.getCompressionTypeAndLevel();

                checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "the compressed multi-/broad-cast datagram is %u bytes long, which is less than the original packet of %hu "
                                "bytes in size; NetProxy will send the multi-/broad-cast datagram compressed through %s level $hhu\n",
                                uiBufLen, ui16PacketLen, compressionSettings.getCompressionTypeAsString(),
                                compressionSettings.getCompressionLevel());
            }
        }
        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), ui32LocalSourceIP, ui32RemoteDestinationIP, bReliable, bSequenced, &udpBCMCPM,
                                                  sizeof(UDPBCastMCastDataProxyMessage), pucBytesToSend, udpBCMCPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "gsend() of a UDPBCastMCastData ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            getConnectorTypeAsString(), getRemoteNetProxyInetAddr()->getIPAsString(), getRemoteNetProxyInetAddr()->getPort(), rc);
            pConnectorWriter->unlockConnectorWriter();
            return -3;
        }
        pConnectorWriter->unlockConnectorWriter();
        checkAndLogMsg ("Connection::sendUDPBCastMCastPacketToRemoteHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "successfully sent a UDPBCastMCastData ProxyMessage via %s with %hu bytes of data "
                        "(%hu bytes before compression, if any) to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), udpBCMCPM.getPayloadLen(), ui16PacketLen,
                        getRemoteNetProxyInetAddr()->getIPAsString(), getRemoteNetProxyInetAddr()->getPort());

        _rStatisticsManager.increaseInboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteNetProxyInetAddr()->getIPAddress(),
                                                    getRemoteNetProxyInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                    getLocalInterfaceInetAddr()->getPort(), PT_UDP, ui16PacketLen);
        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteNetProxyInetAddr()->getIPAddress(),
                                                     getRemoteNetProxyInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_UDP,
                                                     sizeof(UDPBCastMCastDataProxyMessage) + udpBCMCPM.getPayloadLen());

        return 0;
    }

    int Connection::sendOpenTCPConnectionRequest (const Entry * const pEntry, bool bLocalReachabilityFromRemote)
    {
        if (!pEntry) {
            return -1;
        }
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendOpenTCPConnectionRequest", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -2;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendOpenTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isProtocolReliable (pEntry->getProtocol());
        bool bSequenced = isProtocolSequenced (pEntry->getProtocol());
        OpenTCPConnectionProxyMessage oCPM{NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, pEntry->ui16ID, pEntry->ui32LocalIP,
                                           pEntry->ui16LocalPort, pEntry->ui32RemoteIP, pEntry->ui16RemotePort,
                                           pEntry->getConnectorWriter()->getCompressionSetting(), bLocalReachabilityFromRemote};
        if (0 != (rc = _pConnectorAdapter->send (getRemoteNetProxyInetAddr(), pEntry->ui32LocalIP, pEntry->ui32RemoteIP,
                                                 bReliable, bSequenced, &oCPM, sizeof(OpenTCPConnectionProxyMessage)))) {
            checkAndLogMsg ("Connection::sendOpenTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R0: send() of an OpenConnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            oCPM._ui16LocalID, getConnectorTypeAsString(), getRemoteNetProxyInetAddr()->getIPAsString(),
                getRemoteNetProxyInetAddr()->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendOpenTCPConnectionRequest", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L%hu-R0: successfully sent an OpenTCPConnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> "
                        "for virtual connection <%s:%hu --> %s:%hu> with parameters: bReliable = %s, bSequenced = %s\n",
                        oCPM._ui16LocalID, getConnectorTypeAsString(), getRemoteNetProxyInetAddr()->getIPAsString(),
                        getRemoteNetProxyInetAddr()->getPort(), NOMADSUtil::InetAddr{oCPM._ui32LocalIP}.getIPAsString(), oCPM._ui16LocalPort,
                        NOMADSUtil::InetAddr{oCPM._ui32RemoteIP}.getIPAsString(), oCPM._ui16RemotePort,
                        bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_TCP, sizeof(OpenTCPConnectionProxyMessage));

        return 0;
    }

    int Connection::sendTCPConnectionOpenedResponse (const Entry * const pEntry, bool bLocalReachabilityFromRemote)
    {
        if (!pEntry) {
            return -1;
        }

        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendTCPConnectionOpenedResponse", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -2;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendTCPConnectionOpenedResponse", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isProtocolReliable (pEntry->getProtocol());
        bool bSequenced = isProtocolSequenced (pEntry->getProtocol());
        TCPConnectionOpenedProxyMessage cOPM{NetProxyApplicationParameters::NETPROXY_UNIQUE_ID, pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->ui32LocalIP,
                                             pEntry->getConnectorWriter()->getCompressionSetting(), bLocalReachabilityFromRemote};
        if (0 != (rc = _pConnectorAdapter->send (getRemoteNetProxyInetAddr(), pEntry->ui32LocalIP, pEntry->ui32RemoteIP,
                                                 bReliable, bSequenced, &cOPM, sizeof(TCPConnectionOpenedProxyMessage)))) {
            checkAndLogMsg ("Connection::sendTCPConnectionOpenedResponse", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: send() of a TCPConnectionOpened ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            cOPM._ui16LocalID, cOPM._ui16RemoteID, getConnectorTypeAsString(), getRemoteNetProxyInetAddr()->getIPAsString(),
                            getRemoteNetProxyInetAddr()->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendTCPConnectionOpenedResponse", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L%hu-R%hu: successfully sent a TCPConnectionOpened ProxyMessage via %s to the remote NetProxy at address <%s:%hu> with parameters: "
                        "bReliable = %s, bSequenced = %s\n", cOPM._ui16LocalID, cOPM._ui16RemoteID, getConnectorTypeAsString(),
                        getRemoteNetProxyInetAddr()->getIPAsString(), getRemoteNetProxyInetAddr()->getPort(), bReliable ? "true":"false",
                        bSequenced ? "true" : "false");

        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_TCP, sizeof(TCPConnectionOpenedProxyMessage));

        return 0;
    }

    int Connection::sendTCPDataToRemoteHost (const Entry * const pEntry, const uint8 * const pui8Buf, uint16 ui16BufLen, uint8 ui8TCPFlags)
    {
        if (!pEntry) {
            return -1;
        }

        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendTCPDataToRemoteHost", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -2;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendTCPDataToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isProtocolReliable (pEntry->getProtocol());
        bool bSequenced = isProtocolSequenced (pEntry->getProtocol());
        TCPDataProxyMessage tDPM{ui16BufLen, pEntry->ui16ID, pEntry->ui16RemoteID, ui8TCPFlags};

        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), pEntry->ui32LocalIP, pEntry->ui32RemoteIP, bReliable, bSequenced,
                                                  &tDPM, sizeof(TCPDataProxyMessage), pui8Buf, tDPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::sendTCPDataToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: gsend() of a TCPData ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            tDPM._ui16LocalID, tDPM._ui16RemoteID, getConnectorTypeAsString(), getRemoteNetProxyInetAddr()->getIPAsString(),
                            getRemoteNetProxyInetAddr()->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendTCPDataToRemoteHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "L%hu-R%hu: successfully sent a TCPData ProxyMessage with %hu bytes of data via %s to the remote NetProxy at address "
                        "<%s:%hu> with parameters: bReliable = %s, bSequenced = %s\n", tDPM._ui16LocalID, tDPM._ui16RemoteID,
                        tDPM._ui16PayloadLen, getConnectorTypeAsString(), getRemoteNetProxyInetAddr()->getIPAsString(),
                        getRemoteNetProxyInetAddr()->getPort(), bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_TCP, sizeof(TCPDataProxyMessage) + tDPM.getPayloadLen());

        return 0;
    }

    int Connection::sendCloseTCPConnectionRequest (const Entry * const pEntry)
    {
        if (!pEntry) {
            return -1;
        }

        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendCloseTCPConnectionRequest", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -2;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendCloseTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -3;
        }

        int rc;
        bool bReliable = isProtocolReliable (pEntry->getProtocol());
        bool bSequenced = isProtocolSequenced (pEntry->getProtocol());
        CloseTCPConnectionProxyMessage cCPM{pEntry->ui16ID, pEntry->ui16RemoteID};
        if (0 != (rc = _pConnectorAdapter->send (getRemoteNetProxyInetAddr(), pEntry->ui32LocalIP, pEntry->ui32RemoteIP,
                                                 bReliable, bSequenced, &cCPM, sizeof(CloseTCPConnectionProxyMessage)))) {
            checkAndLogMsg ("Connection::sendCloseTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: send() of a CloseTCPConnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> "
                            "failed with rc = %d\n", cCPM._ui16LocalID, cCPM._ui16RemoteID, getConnectorTypeAsString(),
                            getRemoteNetProxyInetAddr()->getIPAsString(), getRemoteNetProxyInetAddr()->getPort(), rc);
            return -4;
        }
        checkAndLogMsg ("Connection::sendCloseTCPConnectionRequest", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L%hu-R%hu: successfully sent a CloseTCPConnection ProxyMessage via %s to the remote NetProxy at address "
                        "<%s:%hu> with parameters: bReliable = %s, bSequenced = %s\n", cCPM._ui16LocalID, cCPM._ui16RemoteID,
                        getConnectorTypeAsString(), getRemoteNetProxyInetAddr()->getIPAsString(),
                        getRemoteNetProxyInetAddr()->getPort(), bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_TCP, sizeof(CloseTCPConnectionProxyMessage));

        return 0;
    }

    int Connection::sendResetTCPConnectionRequest (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, uint16 ui16LocalID,
                                                   uint16 ui16RemoteID, bool bReliable, bool bSequenced)
    {
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendResetTCPConnectionRequest", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendResetTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "with UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());

            return -2;
        }

        int rc;
        ResetTCPConnectionProxyMessage rCPM{ui16LocalID, ui16RemoteID};
        if (0 != (rc = _pConnectorAdapter->send (getRemoteNetProxyInetAddr(), ui32LocalSourceIP, ui32RemoteDestinationIP,
                                                 bReliable, bSequenced, &rCPM, sizeof(ResetTCPConnectionProxyMessage)))) {
            checkAndLogMsg ("Connection::sendResetTCPConnectionRequest", NOMADSUtil::Logger::L_MildError,
                            "L%hu-R%hu: send() of a ResetTCPconnection ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed with rc = %d\n",
                            rCPM._ui16LocalID, rCPM._ui16RemoteID, getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), rc);
            return -3;
        }
        checkAndLogMsg ("Connection::sendResetTCPConnectionRequest", NOMADSUtil::Logger::L_LowDetailDebug,
                        "L%hu-R%hu: successfully sent a ResetTCPconnection ProxyMessages via %s to the remote NetProxy at address "
                        "<%s:%hu> with parameters: bReliable = %s, bSequenced = %s\n", rCPM._ui16LocalID, rCPM._ui16RemoteID,
                        getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort(), bReliable ? "true" : "false", bSequenced ? "true" : "false");

        _rStatisticsManager.increaseOutboundTraffic (getConnectorType(), getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                     getRemoteInterfaceLocalInetAddr()->getPort(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                     getLocalInterfaceInetAddr()->getPort(), PT_TCP, sizeof(ResetTCPConnectionProxyMessage));

        return 0;
    }

    int Connection::sendTunneledEthernetPacket (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, const uint8 * const pui8Buf, uint16 ui16BufLen)
    {
        int rc;
        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendTunneledEthernetPacket", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send an InitializeConnection ProxyMessage\n");
            return -1;
        }
        if (!isConnected()) {
            checkAndLogMsg ("Connection::sendTunneledEthernetPacket", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> "
                            "and NetProxy UniqueID %u is not connected\n", getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), getRemoteNetProxyID());
            return -2;
        }

        TunnelPacketProxyMessage tPPM{ui16BufLen};
        if (0 != (rc = _pConnectorAdapter->gsend (getRemoteNetProxyInetAddr(), ui32LocalSourceIP, ui32RemoteDestinationIP, false, false,
                                                  &tPPM, sizeof(TunnelPacketProxyMessage), pui8Buf, tPPM.getPayloadLen(), nullptr))) {
            checkAndLogMsg ("Connection::sendTunneledEthernetPacket", NOMADSUtil::Logger::L_MildError,
                            "gsend() of an TunnelPacket ProxyMessage via %s to the remote NetProxy at address <%s:%hu> failed "
                            "with rc = %d\n", getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort(), rc);
            return -3;
        }
        checkAndLogMsg ("Connection::sendTunneledEthernetPacket", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully sent a TunnelPacket ProxyMessage via %s to the remote NetProxy at address <%s:%hu>\n",
                        getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort());

        return 0;
    }

    bool Connection::areThereTCPTypePacketsInUDPTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID) const
    {
        if (_connectorType != CT_UDPSOCKET) {
            return 0;
        }

        auto * const pUDPSocketAdapter = dynamic_cast<UDPSocketAdapter *> (_pConnectorAdapter);
        if (!pUDPSocketAdapter) {
            checkAndLogMsg ("Connection::removeTCPTypePacketsFromTransmissionQueue", NOMADSUtil::Logger::L_MildError,
                            "dynamic_cast to UDPSocketAdapter failed on ConnectorAdapter\n");
            return -1;
        }

        return pUDPSocketAdapter->areThereTCPTypePacketsInUDPTransmissionQueue (uiLocalID, uiRemoteID);
    }

    int Connection::removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID)
    {
        if (_connectorType != CT_UDPSOCKET) {
            return 0;
        }

        auto * const pUDPSocketAdapter = dynamic_cast<UDPSocketAdapter *> (_pConnectorAdapter);
        if (!pUDPSocketAdapter) {
            checkAndLogMsg ("Connection::removeTCPTypePacketsFromTransmissionQueue", NOMADSUtil::Logger::L_MildError,
                            "dynamic_cast to UDPSocketAdapter failed on ConnectorAdapter\n");
            return -1;
        }

        return pUDPSocketAdapter->removeTCPTypePacketsFromTransmissionQueue (uiLocalID, uiRemoteID);
    }

    Connection * const Connection::openNewConnectionToRemoteProxy (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                                                                   PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager,
                                                                   const QueryResult & query, bool bBlocking)
    {
        if ((query.getRemoteProxyUniqueID() == 0) || (query.getQueriedConnectorType() == CT_UNDEF) ||
            (query.getQueriedEncryptionType() == ET_UNDEF)) {
            return nullptr;
        }

        if (query.getActiveConnectionToRemoteProxy()) {
            return query.getActiveConnectionToRemoteProxy();
        }

        int rc;
        const auto pucConnectorTypeName = connectorTypeToString (query.getQueriedConnectorType());
        const NOMADSUtil::InetAddr iaRemoteInterfaceAddress = query.getRemoteProxyServerAddress();
        const NOMADSUtil::InetAddr iaLocalInterfaceAddress = (query.getLocalProxyInterfaceAddress() != 0) ? query.getLocalProxyInterfaceAddress() :
            NOMADSUtil::UDPRawDatagramSocket::getLocalIPv4AddressToReachRemoteIPv4Address (iaRemoteInterfaceAddress).getIPAddress();
        const auto ui64HashKey = generateConnectionsTableKey (iaRemoteInterfaceAddress, query.getQueriedConnectorType(), query.getQueriedEncryptionType());
        std::shared_ptr<Connection> spConnection;
        std::unique_lock<std::mutex> ul;
        {
            std::lock_guard<std::mutex> lg{_mtxConnectionsTable};
            spConnection = _umConnectionsTable[query.getRemoteProxyUniqueID()][iaLocalInterfaceAddress.getIPAddress()][ui64HashKey];
            if (!spConnection) {
                auto * const pConnectionAdapter =
                    ConnectorAdapter::ConnectorAdapterFactory (rConnectionManager, query.getQueriedConnectorType(), query.getQueriedEncryptionType(),
                                                               iaLocalInterfaceAddress, iaRemoteInterfaceAddress);
                spConnection =
                    std::make_shared<Connection> (query.getRemoteProxyUniqueID(), iaLocalInterfaceAddress, iaRemoteInterfaceAddress, pConnectionAdapter,
                                                  rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager);
                if (!spConnection) {
                    checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_MildError,
                                    "impossible to instantiate a new %sConnection object for connection "
                                    "to the remote NetProxy with address <%s:%hu>\n", pucConnectorTypeName,
                                    iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort());
                    return nullptr;
                }
                checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_LowDetailDebug,
                                "created a %sConnection object using %s encryption to connect to the remote "
                                "NetProxy at address <%s:%hu>\n", spConnection->getConnectorTypeAsString(),
                                encryptionTypeToString (query.getQueriedEncryptionType()),
                                iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort());
                _umConnectionsTable[query.getRemoteProxyUniqueID()][iaLocalInterfaceAddress.getIPAddress()][ui64HashKey] = spConnection;
            }

            ul = std::unique_lock<std::mutex>{spConnection->_mtxConnection};
            if (spConnection->isTerminationRequested()) {
                // Connection marked for termination --> create new Connection and add it to the Connections Table
                ul.unlock();    // unlock old Connection
                auto * const pConnectionAdapter =
                    ConnectorAdapter::ConnectorAdapterFactory (rConnectionManager, query.getQueriedConnectorType(), query.getQueriedEncryptionType(),
                                                               iaLocalInterfaceAddress, iaRemoteInterfaceAddress);
                spConnection =
                    std::make_shared<Connection>(query.getRemoteProxyUniqueID(), iaLocalInterfaceAddress, iaRemoteInterfaceAddress, pConnectionAdapter,
                                                 rConnectionManager, rTCPConnTable, rTCPManager, rPacketRouter, rStatisticsManager);
                if (!spConnection) {
                    checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_MildError,
                                    "impossible to instantiate a new %sConnection object for connection "
                                    "to the remote NetProxy with address <%s:%hu>\n", pucConnectorTypeName,
                                    iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort());
                    return nullptr;
                }
                checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_Warning,
                                "retrieved a %sConnection object for the remote NetProxy with address <%s:%hu>, but "
                                "the object was marked for deletion; created a new %sConnection object using %s "
                                "encryption to connect to the remote NetProxy\n", pucConnectorTypeName,
                                iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort(),
                                pucConnectorTypeName, encryptionTypeToString (query.getQueriedEncryptionType()));
                _umConnectionsTable[query.getRemoteProxyUniqueID()][iaLocalInterfaceAddress.getIPAddress()][ui64HashKey] = spConnection;
                ul = std::unique_lock<std::mutex>{spConnection->_mtxConnection};     // Lock the newly created Connection instance
            }
        }

        if (spConnection->isConnected()) {
            // The status is CS_Connected and the ConnectorAdapter is connected --> add Connection to the RemoteProxyConnectivity
            checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_Warning,
                            "retrieved an active %sConnection object for the remote NetProxy with address "
                            "<%s:%hu> even if the query did not return any active Connection; %sConnection is "
                            "connected to the address <%s:%hu> using %s encryption\n", pucConnectorTypeName,
                            iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort(), pucConnectorTypeName,
                            spConnection->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            spConnection->getRemoteInterfaceLocalInetAddr()->getPort(),
                            encryptionTypeToString (query.getQueriedEncryptionType()));
            auto * const pOldConnection = rConnectionManager.addNewActiveConnectionToRemoteProxy (spConnection.get());
            if (pOldConnection && (pOldConnection != spConnection.get())) {
                checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_MildError,
                                "addNewActiveConnectionToRemoteProxy() for a new %sConnection with remote NetProxy UniqueID %u "
                                "and remote address <%s:%hu> returned an old %sConnection instance connected to the address "
                                "<%s:%hu>; this should NEVER happen\n", spConnection->getConnectorTypeAsString(),
                                spConnection->getRemoteNetProxyID(), iaRemoteInterfaceAddress.getIPAsString(),
                                iaRemoteInterfaceAddress.getPort(), pOldConnection->getConnectorTypeAsString(),
                                pOldConnection->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                pOldConnection->getRemoteInterfaceLocalInetAddr()->getPort());
                pOldConnection->requestTermination (true);
            }
            return spConnection.get();
        }
        if (spConnection->isConnecting()) {
            checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_LowDetailDebug,
                            "the %sConnection object to reach the remote NetProxy at address <%s:%hu> "
                            "using %s encryption is still connecting\n", pucConnectorTypeName,
                            iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort(),
                            encryptionTypeToString (query.getQueriedEncryptionType()));
            return nullptr;
        }
        rConnectionManager.removeActiveConnectionFromRemoteProxyConnectivityTable (spConnection->getRemoteNetProxyID(),
                                                                                   spConnection->getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                   spConnection->getRemoteInterfaceLocalInetAddr()->getIPAddress(),
                                                                                   spConnection.get());
        spConnection->setStatus (Connection::CS_NotConnected);

        // Do nothing if status of the Connection instance is not CS_NotConnected
        if (bBlocking) {
            if (0 != (rc = spConnection->connectSync (spConnection))) {
                checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_MildError,
                                "failed to establish a %s connection to remote NetProxy at address %s; "
                                "rc = %d\n", pucConnectorTypeName, iaRemoteInterfaceAddress.getIPAsString(), rc);
                return nullptr;
            }
        }
        else {
            checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_LowDetailDebug,
                            "starting a %sConnection background thread to connect asynchronously "
                            "to the remote NetProxy at address <%s:%hu>\n", pucConnectorTypeName,
                            iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort());

            if (0 != (rc = spConnection->connectAsync (spConnection))) {
                checkAndLogMsg ("Connection::openNewConnectionToRemoteProxy", NOMADSUtil::Logger::L_MildError,
                                "failed to start background %sConnection thread to connect to the "
                                "remote NetProxy at address <%s:%hu>; rc = %d\n", pucConnectorTypeName,
                                iaRemoteInterfaceAddress.getIPAsString(), iaRemoteInterfaceAddress.getPort(), rc);
                return nullptr;
            }
        }

        return spConnection->isConnected() ? spConnection.get() : nullptr;
    }

    void Connection::closeAllConnections (const bool bGracefulClose)
    {
        std::lock_guard<std::mutex> lg{_mtxConnectionsTable};
        for (auto & pui64Connection : _umUninitializedConnectionsTable) {
            if (pui64Connection.second) {
                pui64Connection.second->requestTermination_NoLock (
                    bGracefulClose && (pui64Connection.second->getStatus() == Connection::CS_Connected));
            }
        }
        _umUninitializedConnectionsTable.clear();

        for (auto & pmmui64Connection : _umConnectionsTable) {
            for (auto & pmui64Connection : pmmui64Connection.second) {
                for (auto & pui64Connection : pmui64Connection.second) {
                    if (pui64Connection.second) {
                        pui64Connection.second->requestTermination_NoLock (
                            bGracefulClose && (pui64Connection.second->getStatus() == Connection::CS_Connected));
                    }
                }
            }
        }
        _umConnectionsTable.clear();
    }

    int Connection::addNewInitializedConnectionToTable (const std::shared_ptr<Connection> & spInitializedConnection)
    {
        if (!spInitializedConnection) {
            return -1;
        }

        int res = 0;
        const uint32 ui32RemoteNetProxyUniqueID = spInitializedConnection->getRemoteNetProxyID(),
            ui32LocalInterfaceIPv4Address = spInitializedConnection->getLocalInterfaceInetAddr()->getIPAddress();
        const auto ui64Key = generateConnectionsTableKey (spInitializedConnection->getRemoteInterfaceLocalInetAddr(),
                                                          spInitializedConnection->getConnectorType(),
                                                          spInitializedConnection->getEncryptionType());

        std::shared_ptr<Connection> spOldConnection;
        {
            std::lock_guard<std::mutex> lg{_mtxConnectionsTable};
            spOldConnection = _umConnectionsTable[ui32RemoteNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64Key];
            _umConnectionsTable[ui32RemoteNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64Key] = spInitializedConnection;
        }
        if (spOldConnection) {
            res = 1;
            checkAndLogMsg ("Connection::addNewInitializedConnectionToTable", NOMADSUtil::Logger::L_Warning,
                            "replaced an existing %sConnection in status %hu to reach the remote NetProxy at address <%s:%hu> "
                            "with a new instance\n", spOldConnection->getConnectorTypeAsString(), spOldConnection->getStatus(),
                            spOldConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                            spOldConnection->getRemoteNetProxyInetAddr()->getPort());
            const bool bIsUDP = spOldConnection->getConnectorType() == CT_UDPSOCKET;
            requestTerminationOfConnection (spOldConnection.get(), true, bIsUDP, !bIsUDP);
        }
        else {
            checkAndLogMsg ("Connection::addNewInitializedConnectionToTable", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "added a new initialized %s connection to the remote NetProxy with UniqueID %u and address <%s:%hu>\n",
                            spInitializedConnection->getConnectorTypeAsString(), ui32RemoteNetProxyUniqueID,
                            spInitializedConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                            spInitializedConnection->getRemoteNetProxyInetAddr()->getPort());
        }

        return res;
    }

    int Connection::addNewUninitializedConnectionToTable (const std::shared_ptr<Connection> & spUninitializedConnection)
    {
        const auto ui64Key = generateUninitializedConnectionsTableKey (*spUninitializedConnection->getRemoteNetProxyInetAddr(),
                                                                       spUninitializedConnection->getConnectorType(),
                                                                       spUninitializedConnection->getEncryptionType());

        int res = 0;
        std::shared_ptr<Connection> spOldConnection;
        {
            std::lock_guard<std::mutex> lg{_mtxConnectionsTable};
            spOldConnection = _umUninitializedConnectionsTable[ui64Key];

            // Create and start the IncomingMessageHandler thread for the new Connection
            if (0 != spUninitializedConnection->createAndStartIncomingMessageHandler (spUninitializedConnection)) {
                spUninitializedConnection->setStatus (CS_ConnectionFailed);
                res = -1;
            }
            else {
                // IMHT started successfully --> add uninitialized connection to the table
                spUninitializedConnection->setStatus (CS_Connecting);
                _umUninitializedConnectionsTable[ui64Key] = spUninitializedConnection;
            }
        }

        // Check for errors and old connections
        if (res < 0) {
            /* Request graceful termination of the new uninitialized connection; no
             * preparation for deletion is required, since connection is uninitialized.
             * Note that, if there was already an entry in the table, we leave it there. */
            checkAndLogMsg ("Connection::addNewUninitializedConnectionToTable", NOMADSUtil::Logger::L_SevereError,
                            "could not start the IncomingMessageHandler for the new %s "
                            "connection opened by the remote NetProxy with address <%s:%hu>\n",
                            spUninitializedConnection->getConnectorTypeAsString(),
                            spUninitializedConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                            spUninitializedConnection->getRemoteNetProxyInetAddr()->getPort());
            requestTerminationOfConnection (spUninitializedConnection.get(), true, false, false);
            // Connection instance will be deleted when the last shared_ptr gets destructed
        }
        else if (spOldConnection) {
            /* A Connection instance that matches the generated ui64Key was already in the table and the
             * IMHT of the new Connection started fine --> request termination of the old Connection */
            checkAndLogMsg ("Connection::addNewUninitializedConnectionToTable", NOMADSUtil::Logger::L_Info,
                            "replaced an existing %s connection to <%s:%hu> in status %hu with a new instance\n",
                            spUninitializedConnection->getConnectorTypeAsString(),
                            spUninitializedConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                            spUninitializedConnection->getRemoteNetProxyInetAddr()->getPort(),
                            spOldConnection->getStatus());
            spOldConnection->setStatus (CS_ConnectionFailed);
            requestTerminationOfConnection (spOldConnection.get(), false, false, false);
            // Connection instance will be deleted when the last shared_ptr gets destructed
        }
        else {
            checkAndLogMsg ("Connection::addNewUninitializedConnectionToTable", NOMADSUtil::Logger::L_Info,
                            "accepted a new %s connection from the remote NetProxy with address <%s:%hu>\n",
                            spUninitializedConnection->getConnectorTypeAsString(),
                            spUninitializedConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                            spUninitializedConnection->getRemoteNetProxyInetAddr()->getPort());
        }

        return res;
    }

    Connection * const Connection::getAvailableConnectionToRemoteNetProxy (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr & rLocalInterfaceAddress,
                                                                           const NOMADSUtil::InetAddr & rRemoteProxyAddr, ConnectorType connectorType,
                                                                           EncryptionType encryptionType)
    {
        if ((rRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) || (connectorType == CT_UNDEF) || (encryptionType == ET_UNDEF)) {
            return nullptr;
        }

        const auto localInterfaceAddress = (rLocalInterfaceAddress.getIPAddress() != 0) ?
            rLocalInterfaceAddress : NOMADSUtil::UDPRawDatagramSocket::getLocalIPv4AddressToReachRemoteIPv4Address (rRemoteProxyAddr);
        if (localInterfaceAddress == NetProxyApplicationParameters::IA_INVALID_ADDR) {
            // No route to destination exists!
            checkAndLogMsg ("Connection::getAvailableConnectionToRemoteNetProxy", NOMADSUtil::Logger::L_Warning,
                            "impossible to find a local address to reach the remote address %s\n",
                            rRemoteProxyAddr.getIPAsString());
            return nullptr;
        }

        auto & spConnection = getConnectionFromTable (ui32RemoteNetProxyUniqueID, localInterfaceAddress, rRemoteProxyAddr, connectorType, encryptionType);
        if (!spConnection) {
            // Could not find a matching connection
            checkAndLogMsg ("Connection::getAvailableConnectionToRemoteNetProxy", NOMADSUtil::Logger::L_LowDetailDebug,
                            "no %s connection to reach the remote NetProxy with UniqueID %u and "
                            "address %s from the local interface with address %s is available\n",
                            connectorTypeToString (connectorType), ui32RemoteNetProxyUniqueID,
                            rRemoteProxyAddr.getIPAsString(), localInterfaceAddress.getIPAsString());
        }

        return spConnection.get();
    }

    bool Connection::isConnectedToRemoteNetProxyOnAddress (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr * const pLocalInterfaceAddr,
                                                           const NOMADSUtil::InetAddr * const pRemoteProxyAddr, ConnectorType connectorType,
                                                           EncryptionType encryptionType)
    {
        if (!pLocalInterfaceAddr || !pRemoteProxyAddr) {
            return false;
        }

        auto & spConnection = getConnectionFromTable (ui32RemoteNetProxyUniqueID, *pLocalInterfaceAddr, *pRemoteProxyAddr, connectorType, encryptionType);
        return spConnection ? spConnection->isConnected() : false;
    }

    bool Connection::isConnectingToRemoteNetProxyOnAddress (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr * const pLocalInterfaceAddr,
                                                            const NOMADSUtil::InetAddr * const pRemoteProxyAddr, ConnectorType connectorType,
                                                            EncryptionType encryptionType)
    {
        if (!pLocalInterfaceAddr || !pRemoteProxyAddr) {
            return false;
        }

        auto & spConnection = getConnectionFromTable (ui32RemoteNetProxyUniqueID, *pLocalInterfaceAddr, *pRemoteProxyAddr, connectorType, encryptionType);
        return spConnection ? spConnection->isConnecting() : false;
    }

    bool Connection::hasFailedConnectionToRemoteNetProxyOnAddress (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr * const pLocalInterfaceAddr,
                                                                   const NOMADSUtil::InetAddr * const pRemoteProxyAddr, ConnectorType connectorType,
                                                                   EncryptionType encryptionType)
    {
        if (!pLocalInterfaceAddr || !pRemoteProxyAddr) {
            return false;
        }

        auto & spConnection = getConnectionFromTable (ui32RemoteNetProxyUniqueID, *pLocalInterfaceAddr, *pRemoteProxyAddr, connectorType, encryptionType);
        return spConnection ? spConnection->hasFailedConnection() : false;
    }

    // Assume the lock on the Connection is already acquired by the caller
    int Connection::doConnect (const std::shared_ptr<Connection> & spConnection)
    {
        if (isTerminationRequested()) {
            checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_Info,
                            "termination was requested during connection process; connection will not proceed!\n");
            return 0;
        }

        if (!_pConnectorAdapter) {
            checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_MildError,
                            "cannot establish connection: Connector not initialized\n");
            return -1;
        }

        int rc;
        const std::string sConfigFile {_rConnectionManager.getMocketsConfigFileForProxyWithIDAndIPv4Address (getRemoteNetProxyID(),
                                                                                                             getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                                             getRemoteInterfaceLocalInetAddr()->getIPAddress())};
        if (sConfigFile.length() > 0) {
            _pConnectorAdapter->readConfigFile (sConfigFile.c_str());
        }
        const bool bReachableFromRemote =
            _rConnectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (getRemoteNetProxyID(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                    getRemoteInterfaceLocalInetAddr()->getIPAddress());

        // Check if the connection needs to be established
        if (!_pConnectorAdapter->isConnected()) {
            // Connect to the remote NetProxy
            if (0 != (rc = _pConnectorAdapter->connect (getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                                        getRemoteInterfaceLocalInetAddr()->getPort()))) {
                checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_MildError,
                                "failed to connect using %s to the remote NetProxy at address <%s:%hu>; rc = %d\n",
                                getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                getRemoteInterfaceLocalInetAddr()->getPort(), rc);
                setStatus (CS_ConnectionFailed);
                return -2;
            }
        }
        else if (getConnectorType() != CT_UDPSOCKET) {
            // Not UDP and connection already connected: weird!
            checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_Warning,
                            "%s connection to remote NetProxy on host %s:%hu was found to be already established!\n",
                            getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                            getRemoteInterfaceLocalInetAddr()->getPort());
        }
        else {
            // UDP Connector
            std::lock_guard<std::mutex> lg{_rConnectionManager._mtx};
            if (_rConnectionManager.addNewActiveConnectionToRemoteProxyIfNone (this) != this) {
                // Another ActiveConnection is already in the table --> request termination of new Connection
                setStatus (CS_ConnectionFailed);
                requestTermination_NoLock (true);
                return -4;
            }
            if (0 != (rc = sendInitializeConnectionProxyMessage (ConnectionManager::getListOfLocalInterfacesIPv4Addresses(), bReachableFromRemote))) {
                _rConnectionManager.removeActiveConnectionFromRemoteProxyConnectivityTable_NoLock (getRemoteNetProxyID(),
                                                                                                   getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                                   getRemoteInterfaceLocalInetAddr()->getIPAddress(), this);
                setStatus (CS_ConnectionFailed);
                requestTermination_NoLock (true);
                checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_Warning,
                                "failed to send an InitializeConneciton ProxyMessage via %s to the remote NetProxy at address <%s:%hu>; "
                                "sendInitializeConnectionProxyMessage() returned %d; NetProxy will close the newly established connection\n",
                                getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                getRemoteInterfaceLocalInetAddr()->getPort(), rc);
                return -5;
            }

            setStatus (CS_Connected);
            _iaLocalInterfaceAddress.setPort (_pConnectorAdapter->getLocalPort());                                              // TO-DO: check if this is necessary for UDP
            updateEdgeUUID (StatisticsManager::buildEdgeUUID (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,                // Update the EdgeUUID
                                                              getLocalInterfaceInetAddr()->getIPAsString(), getLocalInterfaceInetAddr()->getPort(),
                                                              getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                                              getRemoteInterfaceLocalInetAddr()->getPort(), getConnectorType()));

            return 0;
        }
        // If processing reaches this point of the code, the Adapter is not UDP

        // Update the EdgeUUID
        _iaLocalInterfaceAddress.setPort (_pConnectorAdapter->getLocalPort());
        updateEdgeUUID (StatisticsManager::buildEdgeUUID (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                                          getLocalInterfaceInetAddr()->getIPAsString(), getLocalInterfaceInetAddr()->getPort(),
                                                          getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                                          getRemoteInterfaceLocalInetAddr()->getPort(), getConnectorType()));

        // Register PeerUnreachableWarning, create and start the IncomingMessageHandler thread
        if ((getConnectorType() == CT_MOCKETS) || (getConnectorType() == CT_CSR)) {
            _pConnectorAdapter->registerPeerUnreachableWarningCallback (peerUnreachableWarning, _pConnectorAdapter);
        }
        if (0 != createAndStartIncomingMessageHandler (spConnection)) {
            checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_MildError,
                            "impossible to create and start the IncomingMessageHandler thread for the %s connection to "
                            "the remote NetProxy at address <%s:%hu>\n", _pConnectorAdapter->getConnectorTypeAsString(),
                            getRemoteInterfaceLocalInetAddr()->getIPAsString(), getRemoteInterfaceLocalInetAddr()->getPort());
            setStatus (CS_ConnectionFailed);
            Connection::removeConnectionFromTable (this);
            requestTermination_NoLock (true);
            return -6;
        }
        checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_Info,
                        "established a new connection via %s to the remote NetProxy at address <%s:%hu>; the local NetProxy will now add "
                        "it to the RemoteProxyConnectivityTable and send an InitializeConnection ProxyMessage to the remote NetProxy\n",
                        _pConnectorAdapter->getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort());

        // Add new Connection to the RemoteProxyConnectivityTable
        {
            std::lock_guard<std::mutex> lg{_rConnectionManager._mtx};
            if (_rConnectionManager.addNewActiveConnectionToRemoteProxyIfNone (this) != this) {
                /* The remote NetProxy has already established another connection and sent us an InitializeConnection ProxyMessage;
                * this NetProxy will delete this Connection instance to maintain only one connection to the remote NetProxy.
                * Note that by closing the connection without sending any packets, the remote NetProxy will not try to terminate it. */
                setStatus (CS_ConnectionFailed);
                Connection::removeConnectionFromTable (this);
                _rConnectionManager.removeActiveConnectionFromRemoteProxyConnectivityTable_NoLock (getRemoteNetProxyID(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                                   getRemoteInterfaceLocalInetAddr()->getIPAddress(), this);
                _pIncomingMessageHandler->setPrepareConnectionForDelete (false);
                _pIncomingMessageHandler->setDeleteConnection (false);
                requestTermination_NoLock (true);
                checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_Warning,
                                "a new connection to the remote NetProxy with UniqueID %u and address <%s:%hu> has been established via %s, "
                                "but an error occurred while trying to add it to the ConnectivityTable; common causes include: the remote "
                                "NetProxy having already initialized another connection and the NPUID associated to the remote NetProxy "
                                "being updated while the new connection was being established; NetProxy will close the new connection\n",
                                getRemoteNetProxyID(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                getRemoteInterfaceLocalInetAddr()->getPort(), _pConnectorAdapter->getConnectorTypeAsString());
                return -7;
            }

            // Initialize connection with the remote NetProxy
            if (0 != (rc = sendInitializeConnectionProxyMessage (ConnectionManager::getListOfLocalInterfacesIPv4Addresses(), bReachableFromRemote))) {
                setStatus (CS_ConnectionFailed);
                Connection::removeConnectionFromTable (this);
                _rConnectionManager.removeActiveConnectionFromRemoteProxyConnectivityTable_NoLock (getRemoteNetProxyID(), getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                                   getRemoteInterfaceLocalInetAddr()->getIPAddress(), this);
                _pIncomingMessageHandler->setPrepareConnectionForDelete (false);
                _pIncomingMessageHandler->setDeleteConnection (false);
                requestTermination_NoLock (true);
                checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_Warning,
                                "failed to send an InitializeConneciton ProxyMessage via %s to the remote NetProxy at address <%s:%hu>; "
                                "sendInitializeConnectionProxyMessage() returned %d; NetProxy will close the newly established connection\n",
                                _pConnectorAdapter->getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                getRemoteInterfaceLocalInetAddr()->getPort(), rc);
                return -8;
            }
        }

        setStatus (CS_Connected);
        checkAndLogMsg ("Connection::doConnect", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "successfully sent an InitializeConneciton ProxyMessage via %s to the remote NetProxy at address <%s:%hu>\n",
                        _pConnectorAdapter->getConnectorTypeAsString(), getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        getRemoteInterfaceLocalInetAddr()->getPort(), rc);

        return 0;
    }

    void Connection::requestTermination_NoLock (const bool bGracefulClose)
    {
        if (_bTerminationRequested) {
            return;
        }

        _bTerminationRequested = true;
        if (_pIncomingMessageHandler) {
            _pIncomingMessageHandler->requestTermination();
        }

        if ((getConnectorType() != CT_UDPSOCKET) && _pConnectorAdapter && bGracefulClose) {
            _pConnectorAdapter->close();
        }
        else if ((getConnectorType() != CT_UDPSOCKET) && _pConnectorAdapter) {
            _pConnectorAdapter->shutdown (true, true);
        }
    }

    void Connection::prepareForDelete (Connection * const pConnectionToSubstitute)
    {
        std::lock (_mtxConnection, _rConnectionManager._mtxAutoConnectionTable, _mtxConnectionsTable,
                   _rConnectionManager._mtx, _rTCPConnTable.getMutexRef());
        std::lock_guard<std::mutex> lgConnection{_mtxConnection, std::adopt_lock};
        std::lock_guard<std::mutex> lgTCPConnTable{_rTCPConnTable.getMutexRef(), std::adopt_lock};
        std::lock_guard<std::mutex> lgRemoteProxyConnectivityTable{_rConnectionManager._mtx, std::adopt_lock};
        std::lock_guard<std::mutex> lgConnectionsTable{_mtxConnectionsTable, std::adopt_lock};
        std::lock_guard<std::mutex> lgAutoConnectionsTable{_rConnectionManager._mtxAutoConnectionTable, std::adopt_lock};

        _rConnectionManager.prepareForDelete (_rTCPConnTable, this, pConnectionToSubstitute);
    }

    void Connection::registerConnectionWithStatisticsManager (void)
    {
        if (getRemoteNetProxyID() != 0) {
            // Add Node measure
            _rStatisticsManager.addNode (getRemoteNetProxyID());

            // Add Edge and Link Description measures
            _rStatisticsManager.addEdgeAndActivateLink (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                                        getLocalInterfaceInetAddr()->getIPAsString(),
                                                        getLocalInterfaceInetAddr()->getPort(), getRemoteNetProxyID(),
                                                        getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                                                        getRemoteInterfaceLocalInetAddr()->getPort(),
                                                        getConnectorType());
        }
    }

    void Connection::deregisterConnectionFromStatisticsManager (void)
    {
        _rStatisticsManager.deactivateEdgeAndLink (getEdgeUUID());
        _rStatisticsManager.deleteStats (getConnectorType(), getRemoteNetProxyID());
    }

    int Connection::updateRemoteProxyInformation (uint32 ui32RemoteNetProxyUID, const NOMADSUtil::InetAddr & iaRemoteInterfaceLocalAddress,
                                                  const std::vector<uint32> & vui32InterfaceIPv4AddressList, uint16 ui16MocketsServerPort,
                                                  uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability)
    {
        int rc;
        auto ui32CurrentRemoteNetProxyID = getRemoteNetProxyID();
        if (ui32CurrentRemoteNetProxyID == 0) {
            bool bClashingLocalConfiguration{false};
            ui32CurrentRemoteNetProxyID =
                _rConnectionManager.findBestMatchingUniqueIDOfRemoteNetProxyWithInterfaceIPs (vui32InterfaceIPv4AddressList, bClashingLocalConfiguration);
            if (bClashingLocalConfiguration) {
                checkAndLogMsg ("Connection::updateRemoteProxyInformation", NOMADSUtil::Logger::L_MildError,
                                "findBestMatchingUniqueIDOfRemoteNetProxyWithInterfaceIPs() failed; "
                                "check the configuration and restart the local NetProxy\n");
                return -1;
            }
        }

        // Update the information available about the remote NetProxy and the relevant configuration settings
        if ((rc = _rConnectionManager.updateRemoteProxyInfo (this, ui32RemoteNetProxyUID, vui32InterfaceIPv4AddressList, ui16MocketsServerPort,
                                                             ui16TCPServerPort, ui16UDPServerPort, bRemoteProxyReachability)) < 0) {
            checkAndLogMsg ("Connection::updateRemoteProxyInformation", NOMADSUtil::Logger::L_MildError,
                            "updateRemoteProxyInfo() failed with rc = %d\n", rc);
            return -2;
        }
        _rConnectionManager.updateRemoteProxyIDForAutoConnectionWithID (ui32CurrentRemoteNetProxyID, ui32RemoteNetProxyUID);
        _pConnectorAdapter->readConfigFile (_rConnectionManager.getMocketsConfigFileForProxyWithIDAndIPv4Address (ui32RemoteNetProxyUID,
                                                                                                                  getLocalInterfaceInetAddr()->getIPAddress(),
                                                                                                                  getRemoteInterfaceLocalInetAddr()->getIPAddress()));

        const auto sUpdatedEdgeUUID = StatisticsManager::buildEdgeUUID (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID,
                                                                        getLocalInterfaceInetAddr()->getIPAsString(), getLocalInterfaceInetAddr()->getPort(),
                                                                        ui32RemoteNetProxyUID, iaRemoteInterfaceLocalAddress.getIPAsString(),
                                                                        iaRemoteInterfaceLocalAddress.getPort(), getConnectorType());
        _rStatisticsManager.updateEdgeUUIDAndRemoteNetProxyUIDInMeasures (_ui32RemoteNetProxyUID, ui32RemoteNetProxyUID, getEdgeUUID(), sUpdatedEdgeUUID,
                                                                          getLocalInterfaceInetAddr()->getIPAsString(), iaRemoteInterfaceLocalAddress.getIPAsString(),
                                                                          iaRemoteInterfaceLocalAddress.getPort(), getConnectorType());
        updateEdgeUUID (sUpdatedEdgeUUID);
        setRemoteNetProxyUID (ui32RemoteNetProxyUID);

        return 0;
    }

    int Connection::updateInitializedConnection (const Connection * const pInitializedConnection, uint32 ui32CurrentNetProxyUniqueID,
                                                 uint32 ui32NewNetProxyUniqueID)
    {
        if (!pInitializedConnection) {
            return -1;
        }
        if ((ui32CurrentNetProxyUniqueID == 0) || (ui32NewNetProxyUniqueID == 0)) {
            return -2;
        }

        if ((ui32CurrentNetProxyUniqueID == ui32NewNetProxyUniqueID) &&
            (*pInitializedConnection->getRemoteNetProxyInetAddr() == *pInitializedConnection->getRemoteInterfaceLocalInetAddr())) {
            return 0;
        }

        const auto ui32LocalInterfaceIPv4Address = pInitializedConnection->getLocalInterfaceInetAddr()->getIPAddress();
        const auto ui64CurrentConnectionKey = generateConnectionsTableKey (pInitializedConnection->getRemoteNetProxyInetAddr(),
                                                                           pInitializedConnection->getConnectorType(),
                                                                           pInitializedConnection->getEncryptionType());
        const auto ui64UpdatedConnectionKey = generateConnectionsTableKey (pInitializedConnection->getRemoteInterfaceLocalInetAddr(),
                                                                           pInitializedConnection->getConnectorType(),
                                                                           pInitializedConnection->getEncryptionType());

        {
            std::lock_guard<std::mutex> lg{_mtxConnectionsTable};
            const auto * const pConnectionFromTable =
                _umConnectionsTable[ui32CurrentNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64CurrentConnectionKey].get();
            if (pConnectionFromTable && (pConnectionFromTable != pInitializedConnection)) {
                checkAndLogMsg ("Connection::updateInitializedConnection", NOMADSUtil::Logger::L_MildError,
                                "the retrieved Connection object from the ConnectionsTable is different from the Connection that needs to be updated; the "
                                "Connection in the table has the remote NetProxy UniqueID %u, local address <%s:%hu>, remote address <%s:%hu>, type %s, "
                                "and encryption type %s; the Connection that needs to be updated has the remote NetProxy UniqueID %u, local address <%s:%hu>, "
                                "remote address <%s:%hu>, type %s, and encryption type %s; the NetProxy will not update the Connection and return an error\n",
                                pConnectionFromTable->getRemoteNetProxyID(), pConnectionFromTable->getLocalInterfaceInetAddr()->getIPAsString(),
                                pConnectionFromTable->getLocalInterfaceInetAddr()->getPort(), pConnectionFromTable->getRemoteNetProxyInetAddr()->getIPAsString(),
                                pConnectionFromTable->getRemoteNetProxyInetAddr()->getPort(), pConnectionFromTable->getConnectorTypeAsString(),
                                pConnectionFromTable->getEncryptionType(), pInitializedConnection->getRemoteNetProxyID(),
                                pInitializedConnection->getLocalInterfaceInetAddr()->getIPAsString(), pInitializedConnection->getLocalInterfaceInetAddr()->getPort(),
                                pInitializedConnection->getRemoteNetProxyInetAddr()->getIPAsString(), pInitializedConnection->getRemoteNetProxyInetAddr()->getPort(),
                                pInitializedConnection->getConnectorTypeAsString(), pInitializedConnection->getEncryptionType());
                return -3;
            }
            else if (!pConnectionFromTable) {
                checkAndLogMsg ("Connection::updateInitializedConnection", NOMADSUtil::Logger::L_MildError,
                                "no Connection object exists in the table with UniqueID %u, local IPv4 address %s, and key in table %llu; the "
                                "NetProxy will not update the %sConnection with updated remote NetProxy UniqueID %u and return an error\n",
                                ui32CurrentNetProxyUniqueID, pInitializedConnection->getLocalInterfaceInetAddr()->getIPAsString(),
                                ui64CurrentConnectionKey, pInitializedConnection->getConnectorTypeAsString(), ui32NewNetProxyUniqueID);
                return -4;
            }

            std::swap (_umConnectionsTable[ui32CurrentNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64CurrentConnectionKey],
                       _umConnectionsTable[ui32NewNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64UpdatedConnectionKey]);
            _umConnectionsTable[ui32CurrentNetProxyUniqueID][ui32LocalInterfaceIPv4Address].erase (ui64CurrentConnectionKey);
            if (_umConnectionsTable[ui32CurrentNetProxyUniqueID][ui32LocalInterfaceIPv4Address].size() == 0) {
                _umConnectionsTable[ui32CurrentNetProxyUniqueID].erase (ui32LocalInterfaceIPv4Address);
                if (_umConnectionsTable[ui32CurrentNetProxyUniqueID].size() == 0) {
                    _umConnectionsTable.erase (ui32CurrentNetProxyUniqueID);
                }
            }
        }

        checkAndLogMsg ("Connection::updateInitializedConnection", NOMADSUtil::Logger::L_Info,
                        "successfully updated the connection to the remote NetProxy with UniqueID %u (the old NPUID was: %u); "
                        "the address of the interface on the remote node is <%s:%hu>, which appears locally as <%s:%hu>)\n",
                        ui32NewNetProxyUniqueID, ui32CurrentNetProxyUniqueID,
                        pInitializedConnection->getRemoteInterfaceLocalInetAddr()->getIPAsString(),
                        pInitializedConnection->getRemoteInterfaceLocalInetAddr()->getPort(),
                        pInitializedConnection->getRemoteNetProxyInetAddr()->getIPAsString(),
                        pInitializedConnection->getRemoteNetProxyInetAddr()->getPort());

        return 0;
    }

    // Assume the lock on pUninitializedConnection is held by the caller
    int Connection::updateUninitializedConnection (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                                                   Connection * const pUninitializedConnection, uint32 ui32RemoteNetProxyUniqueID)
    {
        if (!pUninitializedConnection) {
            return -1;
        }
        if (pUninitializedConnection->getRemoteNetProxyID() != 0) {
            // Connection already initialized!
            return -2;
        }

        const auto * const piaRemoteInterfaceLocalAddress = pUninitializedConnection->getRemoteInterfaceLocalInetAddr();
        if (!piaRemoteInterfaceLocalAddress || (piaRemoteInterfaceLocalAddress->getIPAddress() == 0)) {
            return -3;
        }

        std::unique_lock<std::mutex> ul{_mtxConnectionsTable};
        const auto ui32LocalInterfaceIPv4Address = pUninitializedConnection->getLocalInterfaceInetAddr()->getIPAddress();
        const auto ui64UninitializedConnectionKey = generateUninitializedConnectionsTableKey (*pUninitializedConnection->getRemoteNetProxyInetAddr(),
                                                                                              pUninitializedConnection->getConnectorType(),
                                                                                              pUninitializedConnection->getEncryptionType());
        {
            const auto & pConnectionFromTable = _umUninitializedConnectionsTable[ui64UninitializedConnectionKey];
            if (pUninitializedConnection != pConnectionFromTable.get()) {
                return -4;
            }
        }

        // Check if there is already a Connection instance in the ConnectionTable; if yes, this NetProxy must have been the client
        int res;
        const auto ui64ConnectionKey = generateConnectionsTableKey (piaRemoteInterfaceLocalAddress, pUninitializedConnection->getConnectorType(),
                                                                    pUninitializedConnection->getEncryptionType());
        auto spOldConnection = _umConnectionsTable[ui32RemoteNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64ConnectionKey];
        if (spOldConnection && ((spOldConnection->getLocalHostRole() == Connection::CHR_Server) ||
            ((spOldConnection->getLocalHostRole() == Connection::CHR_Client) &&
            (spOldConnection->getRemoteNetProxyID() < NetProxyApplicationParameters::NETPROXY_UNIQUE_ID)))) {
            /* If the remote NetProxy opened another Connection to the local NetProxy or this NetProxy had also opened a
             * Connection to the remote one and that NetProxy has a lower UniqueID (which corresponds to higher priority),
             * then the local NetProxy needs to remove the old Connection. */
            spOldConnection->setIMHTToPrepareConnectionForDelete (false);    // No need for the IMHT to take care of preparing the Connection for the delete
            spOldConnection->requestTermination_NoLock (spOldConnection->getLocalHostRole() == Connection::CHR_Client);        // The client triggers graceful termination
            ul.unlock();
            spOldConnection->prepareForDelete (pUninitializedConnection);
            ul.lock();
            std::swap (_umConnectionsTable[ui32RemoteNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64ConnectionKey],
                       _umUninitializedConnectionsTable[ui64UninitializedConnectionKey]);
            res = 1;
        }
        else if (spOldConnection) {
            // Keep the Connection that was already in the table (with CHR_Client) and return an error
            res = -5;
        }
        else {
            // Connection initialization can proceed normally
            std::swap (_umConnectionsTable[ui32RemoteNetProxyUniqueID][ui32LocalInterfaceIPv4Address][ui64ConnectionKey],
                       _umUninitializedConnectionsTable[ui64UninitializedConnectionKey]);
            res = 0;
        }
        _umUninitializedConnectionsTable.erase (ui64UninitializedConnectionKey);

        return res;
    }

    void Connection::requestTerminationOfConnection (Connection * const pConnectionToTerminate, bool bGracefulClose,
                                                     bool bPrepareForDelete, bool bIMHTPrepareForDelete)
    {
        if (!pConnectionToTerminate) {
            // Nothing to do
            return;
        }

        /* If bPrepareForDelete == true, preparation is done by this thread, otherwise,
         * and if bIMHTPrepareForDelete == true, preparation is entrusted to the IMHT. */
        {
            std::lock_guard<std::mutex> lg{pConnectionToTerminate->_mtxConnection};
            pConnectionToTerminate->setIMHTToPrepareConnectionForDelete (!bPrepareForDelete && bIMHTPrepareForDelete);
            pConnectionToTerminate->requestTermination_NoLock (bGracefulClose);
        }
        if (bPrepareForDelete && !bIMHTPrepareForDelete) {
            pConnectionToTerminate->prepareForDelete();
        }
    }

    // This method assumes that the lock on _umConnectionsTable is acquired by the caller
    const Connection * const Connection::removeConnectionFromTable_NoLock (const Connection * const pConnectionToRemove)
    {
        const auto ui32RemoteNetProxyUniqueID = pConnectionToRemove->getRemoteNetProxyID();
        const auto ui32LocalInterfaceIPv4Address = pConnectionToRemove->getLocalInterfaceInetAddr()->getIPAddress();
        const auto ui64Key = generateConnectionsTableKey (pConnectionToRemove->getRemoteInterfaceLocalInetAddr(),
                                                          pConnectionToRemove->getConnectorType(),
                                                          pConnectionToRemove->getEncryptionType());

        if ((_umConnectionsTable.count (ui32RemoteNetProxyUniqueID) == 1) &&
            (_umConnectionsTable.at (ui32RemoteNetProxyUniqueID).count (ui32LocalInterfaceIPv4Address) == 1) &&
            (_umConnectionsTable.at (ui32RemoteNetProxyUniqueID).at (ui32LocalInterfaceIPv4Address).count (ui64Key) == 1)) {
            auto * const pConnection =
                _umConnectionsTable.at (ui32RemoteNetProxyUniqueID).at (ui32LocalInterfaceIPv4Address).at (ui64Key).get();
            if (pConnection == pConnectionToRemove) {
                _umConnectionsTable.at (ui32RemoteNetProxyUniqueID).at (ui32LocalInterfaceIPv4Address).erase (ui64Key);
                if (_umConnectionsTable.at (ui32RemoteNetProxyUniqueID).at (ui32LocalInterfaceIPv4Address).size() == 0) {
                    _umConnectionsTable.at (ui32RemoteNetProxyUniqueID).erase (ui32LocalInterfaceIPv4Address);
                    if (_umConnectionsTable.at (ui32RemoteNetProxyUniqueID).size() == 0) {
                        _umConnectionsTable.erase (ui32RemoteNetProxyUniqueID);
                    }
                }
            }

            return pConnection;
        }

        return nullptr;
    }

    int Connection::sendConnectionErrorProxyMessageToRemoteHost (ConnectorAdapter * const pConnectorAdapter, const NOMADSUtil::InetAddr & iaRemoteNetProxyAddr,
                                                                 uint32 ui32LocalNetProxyUniqueID, uint32 ui32RemoteNetProxyUniqueID,
                                                                 uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address, ConnectorType ct)
    {
        int rc;
        if (!pConnectorAdapter) {
            checkAndLogMsg ("Connection::sendConnectionErrorProxyMessageToRemoteHost", NOMADSUtil::Logger::L_SevereError,
                            "ConnectorAdapter instance is NULL when trying to send a ConnectionError ProxyMessage\n");
            return -1;
        }
        if (!pConnectorAdapter->isConnected()) {
            checkAndLogMsg ("Connection::sendConnectionErrorProxyMessageToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "the connector of type %s used to reach the remote NetProxy at address <%s:%hu> and "
                            "UniqueID %u is not connected\n", pConnectorAdapter->getConnectorTypeAsString(),
                            iaRemoteNetProxyAddr.getIPAsString(), iaRemoteNetProxyAddr.getPort(), ui32RemoteNetProxyUniqueID);
            return -2;
        }

        ConnectionErrorProxyMessage cEPM{ui32LocalNetProxyUniqueID, ui32RemoteNetProxyUniqueID,
                                         ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address, ct};
        if (0 != (rc = pConnectorAdapter->gsend (&iaRemoteNetProxyAddr, 0, 0, false, false, &cEPM, sizeof(ConnectionErrorProxyMessage), nullptr))) {
            checkAndLogMsg ("Connection::sendConnectionErrorProxyMessageToRemoteHost", NOMADSUtil::Logger::L_MildError,
                            "gsend() of an ConnectionError ProxyMessage via %s to the remote NetProxy at address "
                            "<%s:%hu> failed with rc = %d\n", pConnectorAdapter->getConnectorTypeAsString(),
                            iaRemoteNetProxyAddr.getIPAsString(), iaRemoteNetProxyAddr.getPort(), rc);
            return -3;
        }
        checkAndLogMsg ("Connection::sendConnectionErrorProxyMessageToRemoteHost", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully sent a ConnectionError ProxyMessage via %s to the remote NetProxy at address <%s:%hu>\n",
                        pConnectorAdapter->getConnectorTypeAsString(), iaRemoteNetProxyAddr.getIPAsString(), iaRemoteNetProxyAddr.getPort());

        return 0;
    }


    std::unordered_map<uint64, std::shared_ptr<Connection>> Connection::_umUninitializedConnectionsTable;
    std::unordered_map<uint32, std::unordered_map<uint32, std::unordered_map<uint64, std::shared_ptr<Connection>>>> Connection::_umConnectionsTable;
    std::mutex Connection::_mtxConnectionsTable;
}
