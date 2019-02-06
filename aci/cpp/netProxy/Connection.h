#ifndef INCL_CONNECTION_H
#define INCL_CONNECTION_H

/*
 * Connection.h
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
 *
 * This file defines the Connection, the IncomingMessageHandler, and the
 * BackgroundConnectionThread classes.
 * The Connection class is responsible for sending messages using an
 * already established connection to a remote NetProxy.
 * The IncomingMessageHandler class is responsible for receiving messages
 * over a connection with a remote NetProxy and for dispatching them
 * to other components to be processed.
 * The BackgroundConnectionThread is responsible for establishing a new
 * connection to a remote NetProxy.
 */

#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "InetAddr.h"
#include "ManageableThread.h"

#include "ConfigurationParameters.h"
#include "ProxyMessages.h"
#include "Entry.h"
#include "ConnectorAdapter.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    struct QueryResult;

    class MutexUDPQueue;
    class ConnectionManager;
    class TCPConnTable;
    class TCPManager;
    class PacketRouter;
    class StatisticsManager;


    class Connection
    {
    private:
        class BackgroundConnectionThread : public NOMADSUtil::Thread
        {
        public:
            BackgroundConnectionThread (const std::shared_ptr<Connection> & spConnection);
            ~BackgroundConnectionThread (void);

            void run (void);


        private:
            std::shared_ptr<Connection> _spConnection;
        };


    public:
        enum Status
        {
            CS_NotConnected,            // No active connection
            CS_ConnectionFailed,        // No active connection - connection attempt failed
            CS_Connecting,              // Connection is being established
            CS_Connected                // Connected
        };


        enum HostRole
        {
            CHR_Client,                 // Connection request was generated on this host
            CHR_Server                  // Connection was generated on a remote host
        };


        class IncomingMessageHandler : public NOMADSUtil::ManageableThread
        {
        public:
            IncomingMessageHandler (const std::shared_ptr<Connection> & spConnection, bool bPrepareConnectionForDelete = true,
                                    bool bDeleteConnection = true);
            virtual ~IncomingMessageHandler (void);

            void run (void);

            void setPrepareConnectionForDelete (bool bPrepareConnectionForDelete);
            void setDeleteConnection (bool bDeleteConnection);

            bool isTerminationRequested (void);


        private:
            std::shared_ptr<Connection> _spConnection;

            std::atomic<bool> _bPrepareConnectionForDelete;
            std::atomic<bool> _bDeleteConnection;

            uint8 _ui8InBuf[NetworkConfigurationSettings::PROXY_MESSAGE_MTU];
        };


        // Constructor used when a connection has been accepted (CHR_Server)
        Connection (const NOMADSUtil::InetAddr & iaLocalInterfaceAddress, const NOMADSUtil::InetAddr & iaRemoteInterfaceAddress,
                    ConnectorAdapter * const pConnectorAdapter, ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                    TCPManager & rTCPManager, PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);
        // Constructor used when a connection should be established (CHR_Client)
        Connection (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr & iaLocalInterfaceAddress,
                    const NOMADSUtil::InetAddr & iaRemoteInterfaceAddress, ConnectorAdapter * const pConnectorAdapter,
                    ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                    PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager);
        ~Connection (void);

        int createAndStartIncomingMessageHandler (const std::shared_ptr<Connection> & spConnection);
        void requestTermination (const bool bGracefulClose);
        void backgroundConnectionThreadTerminated (void);
        void incomingMessageHandlerThreadTerminated (void);

        int connectSync (const std::shared_ptr<Connection> & spConnection);
        int connectAsync (const std::shared_ptr<Connection> & spConnection);
        int waitForConnectionEstablishment (void);

        bool isConnected (void) const;
        bool isConnecting (void) const;
        bool hasFailedConnection (void) const;
        bool isTerminationRequested (void) const;
        bool isEnqueueingAllowed (void) const;

        int receiveInitializeConnectionProxyMessage (const InitializeConnectionProxyMessage * const pICPM);
        int receiveConnectionInitializedProxyMessage (const ConnectionInitializedProxyMessage * const pCIPM);
        int receiveICMPProxyMessageToRemoteHost (const ICMPProxyMessage * const pICMPPM);
        int receiveUDPUnicastPacketToRemoteHost (const UDPUnicastDataProxyMessage * const pUDPUDPM);
        int receiveMultipleUDPDatagramsToRemoteHost (const MultipleUDPDatagramsProxyMessage * const pMUDPDPM);
        int receiveUDPBCastMCastPacketToRemoteHost (const UDPBCastMCastDataProxyMessage * const pUDPBcMcDPM);
        int receiveOpenTCPConnectionRequest (const OpenTCPConnectionProxyMessage * const pOTCPCPM);
        int receiveTCPConnectionOpenedResponse (const TCPConnectionOpenedProxyMessage * const pTCPCOPM);
        int receiveTCPDataAndSendToLocalHost (const TCPDataProxyMessage * const pTCPDPM);
        int receiveCloseTCPConnectionRequest (const CloseTCPConnectionProxyMessage * const pCTCPCPM);
        int receiveResetTCPConnectionRequest (const ResetTCPConnectionProxyMessage * const pRTCPCPM);
        int receiveTunneledEthernetPacket (const TunnelPacketProxyMessage * const pTPPM);
        int receiveConnectionErrorProxyMessage (const ConnectionErrorProxyMessage * const pCEPM);

        int sendInitializeConnectionProxyMessage (const std::vector<uint32> & vui32InterfaceIPv4Address, bool bLocalReachabilityFromRemote);
        int sendConnectionInitializedProxyMessage (const std::vector<uint32> & vui32InterfaceIPv4Address, bool bLocalReachabilityFromRemote);
        int sendICMPProxyMessageToRemoteHost (uint8 ui8Type, uint8 ui8Code, uint32 ui32RoH, uint32 ui32LocalOriginationIP,
                                              uint32 ui32RemoteDestinationIP, uint8 ui8PacketTTL, const uint8 * const pui8Buf,
                                              uint16 ui16BufLen, Protocol ui8Protocol, bool bLocalReachabilityFromRemote);
        int sendUDPUnicastPacketToRemoteHost (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, uint8 ui8PacketTTL, const uint8 * const pPacket,
                                              uint16 ui16PacketLen, const CompressionSettings & compressionSettings, Protocol ui8Protocol);
        int sendMultipleUDPDatagramsToRemoteHost (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, MutexUDPQueue * const pUDPDatagramsQueue,
                                                  const CompressionSettings & compressionSettings, Protocol ui8Protocol);
        int sendUDPBCastMCastPacketToRemoteHost (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, const uint8 * const pPacket,
                                                 uint16 ui16PacketLen, const CompressionSettings & compressionSettings, Protocol ui8Protocol);
        int sendOpenTCPConnectionRequest (const Entry * const pEntry, bool bLocalReachabilityFromRemote);
        int sendTCPConnectionOpenedResponse (const Entry * const pEntry, bool bLocalReachabilityFromRemote);
        int sendTCPDataToRemoteHost (const Entry * const pEntry, const uint8 * const pui8Buf, uint16 ui16BufLen, uint8 ui8TCPFlags);
        int sendCloseTCPConnectionRequest (const Entry * const pEntry);
        int sendResetTCPConnectionRequest (const Entry * const pEntry);
        int sendResetTCPConnectionRequest (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, uint16 ui16LocalID,
                                           uint16 ui16RemoteID, bool bRealiable, bool bSequenced);
        int sendTunneledEthernetPacket (uint32 ui32LocalSourceIP, uint32 ui32RemoteDestinationIP, const uint8 * const pui8Buf, uint16 ui16BufLen);
        int sendConnectionErrorProxyMessageToRemoteHost (uint32 ui32LocalNetProxyUniqueID, uint32 ui32RemoteNetProxyUniqueID,
                                                         uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address, ConnectorType ct);

        bool areThereTCPTypePacketsInUDPTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID) const;
        int removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID);

        ConnectorType getConnectorType (void) const;
        const char * const getConnectorTypeAsString (void) const;
        EncryptionType getEncryptionType (void) const;
        const char * const getEncryptionTypeAsString (void) const;
        HostRole getLocalHostRole (void) const;
        Status getStatus (void) const;
        const uint32 getRemoteNetProxyID (void) const;
        const std::string getEdgeUUID (void) const;
        ConnectorAdapter * const getConnectorAdapter (void) const;
        const NOMADSUtil::InetAddr * const getLocalInterfaceInetAddr (void) const;
        const NOMADSUtil::InetAddr * const getRemoteNetProxyInetAddr (void) const;
        const NOMADSUtil::InetAddr * const getRemoteInterfaceLocalInetAddr (void) const;

        static Connection * const openNewConnectionToRemoteProxy (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable, TCPManager & rTCPManager,
                                                                  PacketRouter & rPacketRouter, StatisticsManager & rStatisticsManager,
                                                                  const QueryResult & query, bool bBlocking);
        static const Connection * const removeConnectionFromTable (const Connection * const pConnectionToRemove);
        static void closeAllConnections (const bool bGracefulClose);

        static int addNewInitializedConnectionToTable (const std::shared_ptr<Connection> & spInitializedConnection);
        static int addNewUninitializedConnectionToTable (const std::shared_ptr<Connection> & spUninitializedConnection);
        static Connection * const getAvailableConnectionToRemoteNetProxy (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr & rLocalInterfaceAddress,
                                                                          const NOMADSUtil::InetAddr & rRemoteProxyAddr, ConnectorType connectorType,
                                                                          EncryptionType encryptionType);

        static bool isConnectedToRemoteNetProxyOnAddress (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr * const pLocalInterfaceAddr,
                                                          const NOMADSUtil::InetAddr * const pRemoteProxyAddr, ConnectorType connectorType,
                                                          EncryptionType encryptionType);
        static bool isConnectingToRemoteNetProxyOnAddress (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr * const pLocalInterfaceAddr,
                                                           const NOMADSUtil::InetAddr * const pRemoteProxyAddr, ConnectorType connectorType,
                                                           EncryptionType encryptionType);
        static bool hasFailedConnectionToRemoteNetProxyOnAddress (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr * const pLocalInterfaceAddr,
                                                                  const NOMADSUtil::InetAddr * const pRemoteProxyAddr, ConnectorType connectorType,
                                                                  EncryptionType encryptionType);


    private:
        friend class IncomingMessageHandler;
        friend class ConnectionManager;
        friend class UDPConnector;


        int doConnect (const std::shared_ptr<Connection> & spConnection);
        void requestTermination_NoLock (const bool bGracefulClose);
        void setStatus (Status sConnectionStatus);

        void prepareForDelete (Connection * const pConnectionToSubstitute = nullptr);
        void setIMHTToPrepareConnectionForDelete (bool bIMHTToPrepareConnectionForDelete);
        void registerConnectionWithStatisticsManager (void);
        void deregisterConnectionFromStatisticsManager (void);

        int updateRemoteProxyInformation (uint32 ui32RemoteNetProxyUID, const NOMADSUtil::InetAddr & iaRemoteInterfaceLocalAddress,
                                          const std::vector<uint32> & vui32InterfaceIPv4AddressList, uint16 ui16MocketsServerPort,
                                          uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability);
        void updateEdgeUUID (const std::string & sEdgeUUID);
        void setRemoteNetProxyUID (uint32 ui32RemoteNetProxyUID);
        void setLocalInterfaceAddress (const NOMADSUtil::InetAddr & iaLocalInterface);
        void setRemoteInterfaceLocalAddress (const NOMADSUtil::InetAddr & iaRemoteInterfaceLocalAddress);


        static std::shared_ptr<Connection> & getConnectionFromTable (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr & rLocalInterfaceAddr,
                                                                     const NOMADSUtil::InetAddr & rRemoteProxyAddr, ConnectorType connectorType,
                                                                     EncryptionType encryptionType);

        static int updateInitializedConnection (const Connection * const pInitializedConnection, uint32 ui32CurrentNetProxyUniqueID,
                                                uint32 ui32NewNetProxyUniqueID);
        static int updateUninitializedConnection (ConnectionManager & rConnectionManager, TCPConnTable & rTCPConnTable,
                                                  Connection * const pUninitializedConnection, uint32 ui32RemoteNetProxyUniqueID);

        static void requestTerminationOfConnection (Connection * const pConnectionToTerminate, bool bGracefulClose,
                                                    bool bPrepareForDelete, bool bIMHTPrepareForDelete);
        static const Connection * const removeConnectionFromTable_NoLock (const Connection * const pConnectionToRemove);

        static int sendConnectionErrorProxyMessageToRemoteHost (ConnectorAdapter * const pConnectorAdapter, const NOMADSUtil::InetAddr & iaRemoteNetProxyAddr,
                                                                uint32 ui32LocalNetProxyUniqueID, uint32 ui32RemoteNetProxyUniqueID,
                                                                uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address, ConnectorType ct);

        static uint64 generateConnectionsTableKey (const NOMADSUtil::InetAddr & rInetAddr, const ConnectorType ct, const EncryptionType et);
        static uint64 generateConnectionsTableKey (const NOMADSUtil::InetAddr * const pInetAddr, const ConnectorType ct, const EncryptionType et);
        static uint64 generateUninitializedConnectionsTableKey (const NOMADSUtil::InetAddr & rInetAddr, const ConnectorType ct, const EncryptionType et);


        const ConnectorType _connectorType;
        const EncryptionType _encryptionType;
        const HostRole _localHostRole;
        Status _status;
        uint32 _ui32RemoteNetProxyUID;
        NOMADSUtil::InetAddr _iaLocalInterfaceAddress;
        const NOMADSUtil::InetAddr _iaRemoteProxyAddress;
        NOMADSUtil::InetAddr _iaRemoteInterfaceLocalAddress;
        std::string _sEdgeUUID;

        std::atomic<bool> _bTerminationRequested;

        ConnectorAdapter * const _pConnectorAdapter;
        BackgroundConnectionThread * _pBCThread;
        IncomingMessageHandler * _pIncomingMessageHandler;

        ConnectionManager & _rConnectionManager;
        TCPConnTable & _rTCPConnTable;
        TCPManager & _rTCPManager;
        PacketRouter & _rPacketRouter;
        StatisticsManager & _rStatisticsManager;

        unsigned char _pucMultipleUDPDatagramsBuffer[NetworkConfigurationSettings::PROXY_MESSAGE_MTU];

        mutable std::mutex _mtxConnection;
        mutable std::mutex _mtxBCThread;

        static std::unordered_map<uint64, std::shared_ptr<Connection>> _umUninitializedConnectionsTable;
        static std::unordered_map<uint32, std::unordered_map<uint32, std::unordered_map<uint64, std::shared_ptr<Connection>>>> _umConnectionsTable;
        static std::mutex _mtxConnectionsTable;
    };


    inline Connection::IncomingMessageHandler::IncomingMessageHandler (const std::shared_ptr<Connection> & spConnection,
                                                                       bool bPrepareConnectionForDelete, bool bDeleteConnection) :
        _spConnection{spConnection}, _bPrepareConnectionForDelete{true}, _bDeleteConnection{bDeleteConnection}, _ui8InBuf{0, }
    {
        std::string sThreadName{spConnection ? spConnection->getConnectorTypeAsString() : "NULL"};
        setName ((sThreadName + " IncomingMessageHandler Thread").c_str());
    }

    inline Connection::IncomingMessageHandler::~IncomingMessageHandler (void)
    {
        _spConnection->incomingMessageHandlerThreadTerminated();
    }

    inline bool Connection::IncomingMessageHandler::isTerminationRequested (void)
    {
        return terminationRequested();
    }

    inline void Connection::IncomingMessageHandler::setPrepareConnectionForDelete (bool bPrepareConnectionForDelete)
    {
        _bPrepareConnectionForDelete = bPrepareConnectionForDelete;
    }

    inline void Connection::IncomingMessageHandler::setDeleteConnection (bool bDeleteConnection)
    {
        _bDeleteConnection = bDeleteConnection;
    }

    inline Connection::BackgroundConnectionThread::BackgroundConnectionThread (const std::shared_ptr<Connection> & spConnection) :
        _spConnection{spConnection} { }

    inline Connection::BackgroundConnectionThread::~BackgroundConnectionThread (void)
    {
        _spConnection->backgroundConnectionThreadTerminated();
    }

    inline void Connection::requestTermination (const bool bGracefulClose)
    {
        std::lock_guard<std::mutex> lg{_mtxConnection};
        requestTermination_NoLock (bGracefulClose);
    }

    inline void Connection::backgroundConnectionThreadTerminated (void)
    {
        _pBCThread = nullptr;                       // BCT will delete itself
    }

    inline void Connection::incomingMessageHandlerThreadTerminated (void)
    {
        _pIncomingMessageHandler = nullptr;         // IMHT will delete itself
    }

    inline int Connection::sendConnectionErrorProxyMessageToRemoteHost (uint32 ui32LocalNetProxyUniqueID, uint32 ui32RemoteNetProxyUniqueID,
                                                                        uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                                                                        ConnectorType ct)
    {
        return sendConnectionErrorProxyMessageToRemoteHost (_pConnectorAdapter, *getRemoteNetProxyInetAddr(), ui32LocalNetProxyUniqueID,
                                                            ui32RemoteNetProxyUniqueID, ui32LocalInterfaceIPv4Address, ui32RemoteInterfaceIPv4Address, ct);
    }

    inline ConnectorType Connection::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline const char * const Connection::getConnectorTypeAsString (void) const
    {
        return connectorTypeToString (_connectorType);
    }

    inline EncryptionType Connection::getEncryptionType (void) const
    {
        return _encryptionType;
    }

    inline const char * const Connection::getEncryptionTypeAsString (void) const
    {
        return encryptionTypeToString (_encryptionType);
    }

    inline Connection::HostRole Connection::getLocalHostRole (void) const
    {
        return _localHostRole;
    }

    inline Connection::Status Connection::getStatus (void) const
    {
        return _status;
    }

    inline const uint32 Connection::getRemoteNetProxyID (void) const
    {
        return _ui32RemoteNetProxyUID;
    }

    inline const std::string Connection::getEdgeUUID (void) const
    {
        return _sEdgeUUID;
    }

    inline ConnectorAdapter * const Connection::getConnectorAdapter (void) const
    {
        return _pConnectorAdapter;
    }

    inline const NOMADSUtil::InetAddr * const Connection::getLocalInterfaceInetAddr (void) const
    {
        return &_iaLocalInterfaceAddress;
    }

    inline const NOMADSUtil::InetAddr * const Connection::getRemoteNetProxyInetAddr (void) const
    {
        return &_iaRemoteProxyAddress;
    }

    inline const NOMADSUtil::InetAddr * const Connection::getRemoteInterfaceLocalInetAddr (void) const
    {
        return &_iaRemoteInterfaceLocalAddress;
    }

    inline const Connection * const Connection::removeConnectionFromTable (const Connection * const pConnectionToRemove)
    {
        if (!pConnectionToRemove) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lg{_mtxConnectionsTable};
        return removeConnectionFromTable_NoLock (pConnectionToRemove);
    }

    inline bool Connection::isConnected (void) const
    {
        return (_status == CS_Connected) && _pConnectorAdapter && _pConnectorAdapter->isConnected();
    }

    inline bool Connection::isConnecting (void) const
    {
        return (_status == CS_Connecting) && _pConnectorAdapter;
    }

    inline bool Connection::hasFailedConnection (void) const
    {
        return (_status == CS_ConnectionFailed) || !_pConnectorAdapter;
    }

    inline bool Connection::isTerminationRequested (void) const
    {
        return _bTerminationRequested;
    }

    inline int Connection::sendResetTCPConnectionRequest (const Entry * const pEntry)
    {
        if (!pEntry) {
            return -1;
        }

        return sendResetTCPConnectionRequest (pEntry->ui32LocalIP, pEntry->ui32RemoteIP, pEntry->ui16ID, pEntry->ui16RemoteID,
                                              isProtocolReliable (pEntry->getProtocol()), isProtocolSequenced (pEntry->getProtocol()));
    }

    inline void Connection::setStatus (Status sConnectionStatus)
    {
        _status = sConnectionStatus;
    }

    inline void Connection::setIMHTToPrepareConnectionForDelete (bool bIMHTToPrepareConnectionForDelete)
    {
        if (_pIncomingMessageHandler) {
            _pIncomingMessageHandler->setPrepareConnectionForDelete (bIMHTToPrepareConnectionForDelete);
        }
    }

    inline void Connection::updateEdgeUUID (const std::string & sEdgeUUID)
    {
        _sEdgeUUID = sEdgeUUID;
    }

    inline void Connection::setRemoteNetProxyUID (uint32 ui32RemoteNetProxyUID)
    {
        _ui32RemoteNetProxyUID = ui32RemoteNetProxyUID;
    }

    inline void Connection::setLocalInterfaceAddress (const NOMADSUtil::InetAddr & iaLocalInterface)
    {
        _iaLocalInterfaceAddress = iaLocalInterface;
    }

    inline void Connection::setRemoteInterfaceLocalAddress (const NOMADSUtil::InetAddr & iaRemoteInterfaceLocalAddress)
    {
        _iaRemoteInterfaceLocalAddress = iaRemoteInterfaceLocalAddress;
    }

    inline std::shared_ptr<Connection> & Connection::getConnectionFromTable (uint32 ui32RemoteNetProxyUniqueID, const NOMADSUtil::InetAddr & rLocalInterfaceAddr,
                                                                             const NOMADSUtil::InetAddr & rRemoteProxyAddr, ConnectorType connectorType,
                                                                             EncryptionType encryptionType)
    {
        const auto ui64Key = generateConnectionsTableKey (rRemoteProxyAddr, connectorType, encryptionType);

        std::lock_guard<std::mutex> lg{_mtxConnectionsTable};
        return _umConnectionsTable[ui32RemoteNetProxyUniqueID][rLocalInterfaceAddr.getIPAddress()][ui64Key];
    }

    inline uint64 Connection::generateConnectionsTableKey (const NOMADSUtil::InetAddr & rInetAddr, const ConnectorType ct, const EncryptionType et)
    {
        return generateUInt64Key (rInetAddr.getIPAddress(), 0, ct, et);
    }

    inline uint64 Connection::generateConnectionsTableKey (const NOMADSUtil::InetAddr * const pInetAddr, const ConnectorType ct, const EncryptionType et)
    {
        return generateUInt64Key (pInetAddr->getIPAddress(), 0, ct, et);
    }

    inline uint64 Connection::generateUninitializedConnectionsTableKey (const NOMADSUtil::InetAddr & rInetAddr, const ConnectorType ct, const EncryptionType et)
    {
        return generateUInt64Key (rInetAddr.getIPAddress(), rInetAddr.getPort(), ct, et);
    }

}

#endif  // INCL_CONNECTION_H
