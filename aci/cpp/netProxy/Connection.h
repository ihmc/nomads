#ifndef INCL_CONNECTION_H
#define INCL_CONNECTION_H

/*
 * Connection.h
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

#include <mutex>

#include "StrClass.h"
#include "InetAddr.h"
#include "UInt32Hashset.h"
#include "UInt64Hashtable.h"
#include "FIFOBuffer.h"
#include "PtrLList.h"
#include "Thread.h"
#include "ManageableThread.h"
#include "Mutex.h"
#include "ConditionVariable.h"

#include "ConfigurationParameters.h"
#include "Utilities.h"
#include "Entry.h"
#include "ConnectorAdapter.h"
#include "GUIUpdateMessage.h"


namespace ACMNetProxy
{
    class AutoConnectionEntry;
    class MutexUDPQueue;
    class Connector;
    class ConnectionManager;
    class TCPConnTable;
    class TCPManager;
    class PacketRouter;
    class NetProxyConfigManager;

    class Connection
    {
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
                    IncomingMessageHandler (Connection * const pConnection, ConnectorAdapter * const pConnectorAdapter,
                                            Connector * const pConnector);
                    virtual ~IncomingMessageHandler (void);

                    void run (void);
                    void executionTerminated (void);

                    bool isReadyToRun (void) const;
                    bool isTerminationRequested (void);
                    void setConnectorAdapter (ConnectorAdapter * const pConnectorAdapter);

                private:
                    Connection * _pConnection;
                    ConnectorAdapter * _pConnectorAdapter;
                    Connector * const _pConnector;

                    uint8 _ui8InBuf[NetProxyApplicationParameters::PROXY_MESSAGE_MTU];
            };


        private:
            class BackgroundConnectionThread : public NOMADSUtil::Thread
            {
                public:
                    BackgroundConnectionThread (Connection * const pConnection);
                    ~BackgroundConnectionThread (void);

                    void run (void);


                private:
                    friend class Connection;

                    Connection * _pConnection;
                    ConnectorAdapter * _pConnectorAdapter;

                    bool _bDeleteConnectorAdapter;
            };


        public:
            // Constructor used when a connection has been accepted (HostRole server)
            Connection (ConnectorAdapter * const pConnectorAdapter, Connector * const pConnector);
            // Constructor used when a connection should be established (HostRole client)
            Connection (ConnectorType connectorType, EncryptionType encryptionType, uint32 ui32RemoteProxyID,
                        const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, Connector * const pConnector);
            ~Connection (void);

            int startMessageHandler (void);
            void requestTermination (void);
            void connectionThreadTerminating (void);

            int connectSync (void);
            int connectAsync (void);
            int waitForConnectionEstablishment (void);

            bool isConnected (void) const;
            bool isConnecting (void) const;
            bool hasFailedConnecting (void) const;
            bool isDeleteRequested (void) const;
            EncryptionType getEncryptionType (void) const;

            int openConnectionWithRemoteProxy (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, bool bLocalReachabilityFromRemote);
            int confirmOpenedConnectionWithRemoteProxy (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, bool bLocalReachabilityFromRemote);
            int sendICMPProxyMessageToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint8 ui8Type, uint8 ui8Code,
                                                  uint32 ui32RoH, uint32 ui32LocalOriginationIP, uint32 ui32RemoteDestinationIP, uint8 ui8PacketTTL,
                                                  const uint8 * const pui8Buf, uint16 ui16BufLen, ProxyMessage::Protocol ui8PMProtocol,
                                                  bool bLocalReachabilityFromRemote);
            int sendUDPUnicastPacketToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32LocalSourceIP,
                                                  uint32 ui32RemoteDestinationIP, uint8 ui8PacketTTL, const uint8 * const pPacket, uint16 ui16PacketLen,
                                                  const CompressionSetting * const pCompressionSetting, ProxyMessage::Protocol ui8PMProtocol);
            int sendMultipleUDPDatagramsToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32LocalSourceIP,
                                                      uint32 ui32RemoteDestinationIP, MutexUDPQueue * const pUDPDatagramsQueue,
                                                      const CompressionSetting * const pCompressionSetting, ProxyMessage::Protocol ui8PMProtocol);
            int sendUDPBCastMCastPacketToRemoteHost (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32RemoteDestinationIP,
                                                     const uint8 * const pPacket, uint16 ui16PacketLen, const CompressionSetting * const pCompressionSetting,
                                                     ProxyMessage::Protocol ui8PMProtocol);
            int sendOpenTCPConnectionRequest (Entry * const pEntry, bool bLocalReachabilityFromRemote);
            int sendTCPConnectionOpenedResponse (Entry * const pEntry, bool bLocalReachabilityFromRemote);
            int sendTCPDataToRemoteHost (const Entry * const pEntry, const uint8 * const pui8Buf, uint16 ui16BufLen, uint8 ui8TCPFlags);
            int sendCloseTCPConnectionRequest (const Entry * const pEntry);
            int sendResetTCPConnectionRequest (const Entry * const pEntry);
            int sendResetTCPConnectionRequest (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32RemoteDestinationIP,
                                               uint16 ui16LocalID, uint16 ui16RemoteID, ProxyMessage::Protocol ui8PMProtocol);
            int tunnelEthernetPacket (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr, uint32 ui32RemoteDestinationIP,
                                      const uint8 * const pui8Buf, uint16 ui16BufLen);

            void setStatusAsDisconnected (void);
            int addTCPEntry (Entry * const pEntry);
            int removeTCPEntry (Entry * const pEntry);

            ConnectorType getConnectorType (void) const;
            HostRole getLocalHostRole (void) const;
            Status getStatus (void) const;
            const uint32 getRemoteProxyID (void) const;
            const char * const getConnectorTypeAsString (void) const;
            ConnectorAdapter * const getConnectorAdapter (void) const;
            const NOMADSUtil::InetAddr * const getRemoteProxyInetAddr (void) const;
            bool isTCPEntryInTable (Entry * const pEntry) const;

            void lock (void) const;
            bool tryLock (void) const;
            void unlock (void) const;


        private:
            friend class IncomingMessageHandler;

            int doConnect (void);

            void updateRemoteProxyID (uint32 ui32RemoteProxyID);
            void updateRemoteProxyInformation (uint32 ui32RemoteProxyID, uint16 ui16MocketsServerPort, uint16 ui16TCPServerPort, uint16 ui16UDPServerPort, bool bRemoteProxyReachability);
            void enforceConnectionsConsistency (Connection * const pOldConnection, uint32 ui32RemoteProxyID);

            static bool peerUnreachableWarning (void *pCallbackArg, unsigned long ulTimeSinceLastContact);

            static bool isCommunicationReliable (ProxyMessage::Protocol pmProtocol);
            static bool isCommunicationSequenced (ProxyMessage::Protocol pmProtocol);

            const ConnectorType _connectorType;
            const EncryptionType _encryptionType;
            const HostRole _localHostRole;
            Status _status;
            uint32 _ui32RemoteProxyID;
            const NOMADSUtil::InetAddr _remoteProxyInetAddr;
            ConnectorAdapter * _pConnectorAdapter;
            Connector * const _pConnector;

            NOMADSUtil::PtrLList<Entry> _TCPEntryList;
            unsigned char _pucMultipleUDPDatagramsBuffer[NetProxyApplicationParameters::PROXY_MESSAGE_MTU];

            IncomingMessageHandler* _pIncomingMessageHandler;
            BackgroundConnectionThread *_pBCThread;

            mutable std::mutex _mConnection;
            mutable NOMADSUtil::Mutex _mBCThread;
            mutable NOMADSUtil::ConditionVariable _cvBCThread;

            bool _bDeleteRequested;

            static ConnectionManager * const _pConnectionManager;
            static NetProxyConfigManager * const _pConfigurationManager;
            static GUIStatsManager * const _pGUIStatsManager;
            static PacketRouter * const _pPacketRouter;
    };


    inline Connection::IncomingMessageHandler::IncomingMessageHandler (Connection * const pConnection, ConnectorAdapter * const pConnectorAdapter,
                                                                       Connector * const pConnector) :
        _pConnection (pConnection), _pConnectorAdapter (pConnectorAdapter), _pConnector (pConnector)
    {
        memset (_ui8InBuf, 0, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);

        String threadName(pConnection ? pConnection->getConnectorTypeAsString() : "NULL");
        setName (threadName + " IncomingMessageHandler Thread");
    }

    inline void Connection::IncomingMessageHandler::executionTerminated (void)
    {
        _pConnection = nullptr;
        _pConnectorAdapter = nullptr;
    }

    inline bool Connection::IncomingMessageHandler::isReadyToRun (void) const
    {
        return _pConnection && _pConnectorAdapter;
    }

    inline bool Connection::IncomingMessageHandler::isTerminationRequested (void)
    {
        return terminationRequested();
    }

    inline void Connection::IncomingMessageHandler::setConnectorAdapter (ConnectorAdapter * const pConnectorAdapter)
    {
        _pConnectorAdapter = pConnectorAdapter;
    }

    inline Connection::BackgroundConnectionThread::BackgroundConnectionThread (Connection * const pConnection) :
        _pConnection(pConnection), _pConnectorAdapter(pConnection->getConnectorAdapter()), _bDeleteConnectorAdapter(false) { }

    inline Connection::BackgroundConnectionThread::~BackgroundConnectionThread (void)
    {
        if (_bDeleteConnectorAdapter) {
            delete _pConnectorAdapter;
        }
    }

    inline void Connection::requestTermination (void)
    {
        std::lock_guard<std::mutex> lg (_mConnection);

        if (_pIncomingMessageHandler) {
            _pIncomingMessageHandler->requestTermination();
        }
        if (_pConnectorAdapter) {
            _pConnectorAdapter->shutdown (true, true);
        }
    }

    inline void Connection::connectionThreadTerminating (void)
    {
        _pBCThread = nullptr;                  // Thread will delete itself
    }

    inline ConnectorType Connection::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline Connection::HostRole Connection::getLocalHostRole (void) const
    {
        return _localHostRole;
    }

    inline Connection::Status Connection::getStatus (void) const
    {
        return _status;
    }

    inline const uint32 Connection::getRemoteProxyID (void) const
    {
        return _ui32RemoteProxyID;
    }

    inline const char * const Connection::getConnectorTypeAsString (void) const
    {
        return connectorTypeToString (_connectorType);
    }

    inline ConnectorAdapter * const Connection::getConnectorAdapter (void) const
    {
        return _pConnectorAdapter;
    }

    inline const NOMADSUtil::InetAddr * const Connection::getRemoteProxyInetAddr (void) const
    {
        return &_remoteProxyInetAddr;
    }

    inline bool Connection::isTCPEntryInTable (Entry * const pEntry) const
    {
        return _TCPEntryList.search (pEntry) != nullptr;
    }

    inline bool Connection::isConnected (void) const
    {
        return (_status == CS_Connected) && _pConnectorAdapter && _pConnectorAdapter->isConnected();
    }

    inline bool Connection::isConnecting (void) const
    {
        return (_status == CS_Connecting) && _pConnectorAdapter;
    }

    inline bool Connection::hasFailedConnecting (void) const
    {
        return (_status == CS_ConnectionFailed) || ((_status == CS_Connected) && (!_pConnectorAdapter || !_pConnectorAdapter->isConnected()));
    }

    inline bool Connection::isDeleteRequested (void) const
    {
        return _bDeleteRequested || !_pIncomingMessageHandler || _pIncomingMessageHandler->isTerminationRequested();
    }

    inline EncryptionType Connection::getEncryptionType (void) const
    {
        return _encryptionType;
    }

    inline int Connection::sendResetTCPConnectionRequest (const Entry * const pEntry)
    {
        if (!pEntry) {
            return -1;
        }

        return sendResetTCPConnectionRequest (&pEntry->remoteProxyAddr, pEntry->ui32RemoteIP, pEntry->ui16ID, pEntry->ui16RemoteID, pEntry->getProtocol());
    }

    inline void Connection::setStatusAsDisconnected (void)
    {
        _status = CS_NotConnected;
    }

    inline int Connection::addTCPEntry (Entry * const pEntry)
    {
        if (_TCPEntryList.search (pEntry)) {
            return 0;
        }
        _TCPEntryList.append (pEntry);

        return 0;
    }

    inline int Connection::removeTCPEntry (Entry * const pEntry)
    {
        if (_TCPEntryList.remove (pEntry)) {
            return 0;
        }

        return -1;
    }

    inline void Connection::lock (void) const
    {
        _mConnection.lock();
    }

    inline bool Connection::tryLock (void) const
    {
        return _mConnection.try_lock();
    }

    inline void Connection::unlock (void) const
    {
        _mConnection.unlock();
    }

    inline bool Connection::isCommunicationReliable (ProxyMessage::Protocol pmProtocol)
    {
        return (pmProtocol == ProxyMessage::PMP_MocketsRS) || (pmProtocol == ProxyMessage::PMP_MocketsRU) ||
               (pmProtocol == ProxyMessage::PMP_UNDEF_MOCKETS) || (pmProtocol == ProxyMessage::PMP_CSRRS) ||
               (pmProtocol == ProxyMessage::PMP_CSRRU) || (pmProtocol == ProxyMessage::PMP_UNDEF_CSR);
    }

    inline bool Connection::isCommunicationSequenced (ProxyMessage::Protocol pmProtocol)
    {
        return (pmProtocol == ProxyMessage::PMP_MocketsRS) || (pmProtocol == ProxyMessage::PMP_MocketsUS) ||
               (pmProtocol == ProxyMessage::PMP_UNDEF_MOCKETS) || (pmProtocol == ProxyMessage::PMP_CSRRS) ||
               (pmProtocol == ProxyMessage::PMP_CSRUS) || (pmProtocol == ProxyMessage::PMP_UNDEF_CSR);
    }

}

#endif  // INCL_CONNECTION_H
