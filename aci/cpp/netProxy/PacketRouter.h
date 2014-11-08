#ifndef INCL_PACKET_ROUTER_H
#define INCL_PACKET_ROUTER_H

/*
 * PacketRouter.h
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
 *
 * Central class of the NetProxy component.
 * It starts all threads, reads the configuration files, 
 * and handles incoming and outgoing network packets.
 */
 
#include "DArray.h"
#include "UInt32Hashtable.h"
#include "ManageableThread.h"
#include "net/NetworkHeaders.h"

#if defined (USE_DISSERVICE)
    #include "DisseminationService.h"
    #include "DisseminationServiceListener.h"
#endif

#include "ProxyMessages.h"
#include "PacketBufferManager.h"
#include "MutexCounter.h"
#include "MutexUDPQueue.h"
#include "NetworkInterface.h"
#include "TapInterface.h"
#include "MocketConnector.h"
#include "SocketConnector.h"
#include "UDPConnector.h"
#include "GUIUpdateMessage.h"


#if defined (USE_DISSERVICE)
namespace IHMC_ACI
{
    class DisseminationService;
}
#endif

namespace NOMADSUtil
{
    struct ARPPacket;
    class ConfigManager;
    struct EtherMACAddr;
    class InetAddr;
}

namespace ACMNetProxy
{
    class ARPCache;
    class ConnectorReader;
    class EndpointConfigFileReader;
    class NetProxyConfigManager;
    class TCPManager;
    class UDPDatagramPacket;

    #if defined (USE_DISSERVICE)
        class PacketRouter : public IHMC_ACI::DisseminationServiceListener
    #else
        class PacketRouter
    #endif
    {
        public:
            static PacketRouter *getPacketRouter (void);
            ~PacketRouter (void);

            // Initialize the PacketRouter
            int init (NetworkInterface * const pInternalInterface, NetworkInterface * const pExternalInterface);
            int startThreads (void);
            int joinThreads (void);

            static MutexCounter<uint16> * const getMutexCounter (void);
            static NetworkInterface * const getInternalNetworkInterface (void);
            static NetworkInterface * const getExternalNetworkInterface (void);

            static int sendARPRequestForGatewayMACAddress (void);

            // The following method is useful whenever a new Connection has been established, to reduce latency in case there are enqued packets/requests
            static void wakeUpAutoConnectionAndRemoteTransmitterThreads (void);

            static void requestTermination (void);
            static bool isRunning (void);
            static bool isTerminationRequested (void);

        private:
            friend class Connection::IncomingMessageHandler;
            friend class UDPConnector;
            friend class TCPManager;

            // Receiver thread is responsible for receiving packets from the virtual ethernet interface
            class InternalReceiverThread : public NOMADSUtil::Thread
            {
                public:
                    InternalReceiverThread (void);
                    void run (void);
            };

            // Receiver thread is responsible for receiving packets from the virtual ethernet interface
            class ExternalReceiverThread : public NOMADSUtil::Thread
            {
                public:
                    ExternalReceiverThread (void);
                    void run (void);
            };


            // LocalUDPDatagramsWrapper thread is responsible for sending received UDP datagrams from the proxy to the virtual ethernet interface
            class LocalUDPDatagramsManagerThread : public NOMADSUtil::Thread
            {
                public:
                    LocalUDPDatagramsManagerThread (void);
                    ~LocalUDPDatagramsManagerThread (void);

                    void run (void);

                    int addDatagramToOutgoingQueue (const NOMADSUtil::InetAddr * const pRemoteProxyAddr, Connection * const pConnection, Connector * const pConnector,
                                                    const CompressionSetting * const pCompressionSetting, const ProxyMessage::Protocol pmProtocol,
                                                    const NOMADSUtil::IPHeader * const pIPHeader, const NOMADSUtil::UDPHeader * const pUDPHeader);

                private:
                    int sendEnqueuedDatagramsToRemoteProxy (MutexUDPQueue * const pUDPDatagramsQueue) const;

                    MutexUDPQueue * const _pUDPReassembledDatagramsQueue;
                    NOMADSUtil::UInt32Hashtable<MutexUDPQueue> _ui32UDPDatagramsQueueHashTable;
                    NOMADSUtil::Mutex _mUDPDatagramsQueueHashTable;

                    static const uint32 LDMT_TIME_BETWEEN_ITERATIONS = 500;          // Time between each iterations for LTT
            };


            // LocalTCPTransmitter thread is responsible for sending data received from remote proxies to the local virtual ethernet interface
            class LocalTCPTransmitterThread : public NOMADSUtil::Thread
            {
                public:
                    LocalTCPTransmitterThread (void);
                    void run (void);

                private:
                    static const uint32 LTT_TIME_BETWEEN_ITERATIONS = 250;          // Time between each iterations for LTT
            };

            // RemoteTCPTransmitter thread is responsible for sending data received from the local virtual ethernet interface to remote proxies
            class RemoteTCPTransmitterThread : public NOMADSUtil::Thread
            {
                public:
                    RemoteTCPTransmitterThread (void);
                    void run (void);

                private:
                    static const uint32 RTT_TIME_BETWEEN_ITERATIONS = 250;          // Time between each iterations for RTT
            };


            // AutoConnectionManager thread is responsible for establishing and verifying status of any configured autoconnection
            class AutoConnectionManager : public NOMADSUtil::Thread
            {
                public:
                    AutoConnectionManager (void);
                    void run (void);

                private:
                    static const uint32 ACM_TIME_BETWEEN_ITERATIONS = 10000;          // Time between each iterations for ACM
            };

            // CleanerThread is responsible for cleaning up connections and memory
            class CleanerThread : public NOMADSUtil::Thread
            {
                public:
                    CleanerThread (void);
                    void run (void);

                private:
                    static const uint32 CT_TIME_BETWEEN_ITERATIONS = 5000;          // Time between each iterations for CT
                    static const uint32 CT_MEMORY_CLEANING_INTERVAL = 10000U;       // Time between each memory cleanup performed by CleanerThread
            };


            // CleanerThread is responsible for cleaning up connections and memory
            class UpdateGUIThread : public NOMADSUtil::Thread
            {
                public:
                    UpdateGUIThread (void);
                    int init (const char *pszNotificationAddress);

                    void run (void);
                    void setupSocket (void);

                private:
                    static const uint32 UGT_TIME_BETWEEN_ITERATIONS = 5000;          // Time between each iterations for UGT

                    GUIStatsManager * const _pGUIStatsManager;
                    NOMADSUtil::UDPDatagramSocket _udpSocket;
                    NOMADSUtil::String _notificationAddress;
            };

            PacketRouter (void);
            explicit PacketRouter (const PacketRouter& rPacketRouter);

            static int handlePacketFromInternalInterface (const uint8 * const pPacket, uint16 ui16PacketLen);
            static int handlePacketFromExternalInterface (const uint8 * const pPacket, uint16 ui16PacketLen);

            // Ethernet
            static int sendPacketToHost (NetworkInterface * const pNI, const uint8 * const pPacket, int iSize);
            static int wrapEtherAndSendPacketToHost (NetworkInterface * const pNI, uint8 *ui8Buf, uint16 ui16PacketLen);
            // ARP
            static int sendARPRequest (NetworkInterface * const pNI, uint32 ui32OriginatingIPAddr, uint32 ui32TargetIPAddr);
            static int sendARPReplyToHost (NetworkInterface * const pNI, const NOMADSUtil::ARPPacket * const pARPReqPacket, const NOMADSUtil::EtherMACAddr &rTargetMACAddr);
            static int sendARPAnnouncement (NetworkInterface * const pNI, const NOMADSUtil::ARPPacket * const pARPReqPacket,
                                            uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr &rMACAddr);
            // ICMP
            static int buildAndSendICMPMessageToHost (NetworkInterface * const pNI, NOMADSUtil::ICMPHeader::Type ICMPType,
                                                      NOMADSUtil::ICMPHeader::Code_Destination_Unreachable ICMPCode, uint32 ui32SourceIP,
                                                      uint32 ui32DestinationIP, NOMADSUtil::IPHeader * const pRcvdIPPacket);
            static int forwardICMPMessageToHost (uint32 ui32LocalTargetIP, uint32 ui32RemoteOriginationIP, uint32 ui32RemoteProxyIP,
                                                 NOMADSUtil::ICMPHeader::Type ICMPType, NOMADSUtil::ICMPHeader::Code_Destination_Unreachable ICMPCode,
                                                 uint32 ui32RoH, const uint8 * const pICMPData, uint16 ui16PayloadLen);

            static int initializeRemoteConnection (uint32 ui32RemoteProxyID, ConnectorType connectorType);
            static int sendUDPUniCastPacketToHost (uint32 ui32RemoteOriginationIP, uint32 ui32LocalTargetIP, const NOMADSUtil::UDPHeader * const pUDPPacket);
            static int sendUDPBCastMCastPacketToHost (const uint8 * const pPacket, uint16 ui16PacketLen);

            static int sendRemoteResetRequestIfNeeded (Entry * const pEntry);
            static int flushAndSendCloseConnectionRequest (Entry * const pEntry);

            static int sendBroadcastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32BroadcastSrcIP, uint32 ui32BroadcastDestIP,
                                             uint16 ui16DestPort, const CompressionSetting * const pCompressionSetting);
            static int sendMulticastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32MulticastSrcIP, uint32 ui32MulticastDestIP,
                                             uint16 ui16DestPort, const CompressionSetting * const pCompressionSetting);
            static int sendBCastMCastPacketToDisService (const uint8 * const pPacket, uint16 ui16PacketLen);

            // Callback methods related to DisService
            #if defined (USE_DISSERVICE)
                bool dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pData,
                                  uint32 ui32Length, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority);
                bool chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pChunk,
                                   uint32 ui32Length, uint8 ui8NChunks, uint8 ui8TotNChunks, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority);
                bool metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId,
                                        const void *pMetadata, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority);
                bool dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const char * pszId,
                                    const void *pMetadata, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority);
            #endif
            
            static bool isMACAddrBroadcast (NOMADSUtil::EtherMACAddr macAddr);
            static bool isMACAddrMulticast (NOMADSUtil::EtherMACAddr macAddr);

            static bool hostBelongsToTheInternalNetwork (const NOMADSUtil::EtherMACAddr & emaHost);
            static bool hostBelongsToTheExternalNetwork (const NOMADSUtil::EtherMACAddr & emaHost);
            static bool isKnownHost (const NOMADSUtil::EtherMACAddr & emaHost);

            static PacketBufferManager & pbm;

            static bool _bTerminationRequested;
            static bool _bConnectorsDeleted;
            static bool _bInternalReceiverThreadRunning;
            static bool _bExternalReceiverThreadRunning;
            static bool _bLocalUDPDatagramsManagerThreadRunning;
            static bool _bLocalTCPTransmitterThreadRunning;
            static bool _bRemoteTCPTransmitterThreadRunning;
            static bool _bAutoConnectionManagerThreadRunning;
            static bool _bCleanerThreadRunning;
            static bool _bUpdateGUIThreadRunning;
            static ARPCache _pARPCache;
            static NOMADSUtil::DArray<NOMADSUtil::EtherMACAddr> _daInternalHosts;
            static NOMADSUtil::DArray<NOMADSUtil::EtherMACAddr> _daExternalHosts;
            static NetworkInterface *_pInternalInterface;
            static NetworkInterface *_pExternalInterface;
            static TCPConnTable * const _pTCPConnTable;
            static ConnectionManager * const _pConnectionManager;
            static NetProxyConfigManager * const _pConfigurationManager;
            #if defined (USE_DISSERVICE)
                static IHMC_ACI::DisseminationService * const _pDisService;
            #endif

            // Handles receiving data from host's virtual interface (in host mode) or from the internal network (in gateway mode)
            static InternalReceiverThread _internalReceiverThread;
            // Handles receiving data from the external network (in gateway mode)
            static ExternalReceiverThread _externalReceiverThread;
            // Handles storing, buffering, wrapping, and subsequent forwarding of received UDP datagram packets
            static LocalUDPDatagramsManagerThread _localUDPDatagramsManagerThread;
            // Handles transmission of data to host's virtual interface
            static LocalTCPTransmitterThread _localTCPTransmitterThread;
            // Handles transmission of buffered data to remote proxies
            static RemoteTCPTransmitterThread _remoteTCPTransmitterThread;
            // Handles sending AutoConnection requests to remote proxies
            static AutoConnectionManager _autoConnectionManagerThread;
            // Handles cleaning up of connections
            static CleanerThread _cleanerThread;
            // Handles sending of UpdateGUI messages to the GUI on localhost
            static UpdateGUIThread _updateGUIThread;

            static NOMADSUtil::Mutex _mLocalUDPDatagramsManager;
            static NOMADSUtil::Mutex _mLocalTCPTransmitter;
            static NOMADSUtil::Mutex _mRemoteTCPTransmitter;
            static NOMADSUtil::Mutex _mAutoConnectionManager;
            static NOMADSUtil::Mutex _mCleaner;
            static NOMADSUtil::Mutex _mGUIUpdater;
            static NOMADSUtil::ConditionVariable _cvLocalUDPDatagramsManager;
            static NOMADSUtil::ConditionVariable _cvLocalTCPTransmitter;
            static NOMADSUtil::ConditionVariable _cvRemoteTCPTransmitter;
            static NOMADSUtil::ConditionVariable _cvAutoConnectionManager;
            static NOMADSUtil::ConditionVariable _cvCleaner;
            static NOMADSUtil::ConditionVariable _cvGUIUpdater;
    };
    

    inline PacketRouter * PacketRouter::getPacketRouter (void) 
    {
        static PacketRouter packetRouter;

        return &packetRouter;
    }

    inline PacketRouter::~PacketRouter (void)
    {
        requestTermination();

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            delete _pInternalInterface;
            delete _pExternalInterface;
        }
    }

    inline PacketRouter::InternalReceiverThread::InternalReceiverThread (void) {}

    inline PacketRouter::ExternalReceiverThread::ExternalReceiverThread (void) {}

    inline PacketRouter::LocalUDPDatagramsManagerThread::LocalUDPDatagramsManagerThread (void)
        : _pUDPReassembledDatagramsQueue (new MutexUDPQueue()) {}

    inline PacketRouter::LocalUDPDatagramsManagerThread::~LocalUDPDatagramsManagerThread (void)
    {
        delete _pUDPReassembledDatagramsQueue;
    }

    inline PacketRouter::LocalTCPTransmitterThread::LocalTCPTransmitterThread (void) {}

    inline PacketRouter::RemoteTCPTransmitterThread::RemoteTCPTransmitterThread (void) {}

    inline PacketRouter::AutoConnectionManager::AutoConnectionManager (void) {}

    inline PacketRouter::CleanerThread::CleanerThread (void) {}

    inline PacketRouter::UpdateGUIThread::UpdateGUIThread (void)
        : _pGUIStatsManager (GUIStatsManager::getGUIStatsManager()) 
    {
        setupSocket();
    }

    inline void PacketRouter::UpdateGUIThread::setupSocket (void)
    {
        _udpSocket.init();
    }

    inline MutexCounter<uint16> * const PacketRouter::getMutexCounter (void)
    {
        static MutexCounter<uint16> gIPIdentProvider (0);

        return &gIPIdentProvider;
    }

    inline NetworkInterface * const PacketRouter::getInternalNetworkInterface (void)
    {
        return _pInternalInterface;
    }

    inline NetworkInterface * const PacketRouter::getExternalNetworkInterface (void)
    {
        return _pExternalInterface;
    }

    inline bool PacketRouter::isRunning (void)
    {
        return (_bInternalReceiverThreadRunning || _bExternalReceiverThreadRunning || _bLocalUDPDatagramsManagerThreadRunning ||
                _bLocalTCPTransmitterThreadRunning || _bRemoteTCPTransmitterThreadRunning ||
                _bCleanerThreadRunning || _bUpdateGUIThreadRunning || !_bConnectorsDeleted);
    }

    inline bool PacketRouter::isTerminationRequested (void)
    {
        return _bTerminationRequested;
    }

    inline PacketRouter::PacketRouter (void) { }

    inline bool PacketRouter::isMACAddrBroadcast (NOMADSUtil::EtherMACAddr macAddr)
    {
        if ((macAddr.ui16Word1 == 0xFFFF) && (macAddr.ui16Word2 == 0xFFFF) && (macAddr.ui16Word3 == 0xFFFF)) {
            return true;
        }
        return false;
    }

    inline bool PacketRouter::isMACAddrMulticast (NOMADSUtil::EtherMACAddr macAddr)
    {
        if ((macAddr.ui8Byte1 == 0x01) && (macAddr.ui8Byte2 == 0x00) && (macAddr.ui8Byte3 == 0x5E)) {
            return true;
        }
        return false;
    }
    
    inline bool PacketRouter::isKnownHost (const NOMADSUtil::EtherMACAddr & emaHost)
    {
        return hostBelongsToTheInternalNetwork (emaHost) || hostBelongsToTheExternalNetwork (emaHost);
    }
}

#endif   // #ifndef INCL_PACKET_ROUTER_H
