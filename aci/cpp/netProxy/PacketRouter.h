#ifndef INCL_PACKET_ROUTER_H
#define INCL_PACKET_ROUTER_H

/*
 * PacketRouter.h
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
 * Central class of the NetProxy component.
 * It starts all threads, reads the configuration files,
 * and handles incoming and outgoing network packets.
 */

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <atomic>

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "ISAACRand.h"
#include "Mutex.h"
#include "Thread.h"

#if defined (USE_DISSERVICE)
    #include "DisseminationService.h"
    #include "DisseminationServiceListener.h"
#endif

#include "MutexCounter.h"
#include "ARPCache.h"
#include "ARPTableMissCache.h"
#include "PacketBufferManager.h"
#include "PacketReceiver.h"
#include "TCPConnTable.h"
#include "ConnectionManager.h"
#include "StatisticsManager.h"
#include "ConfigurationManager.h"
#include "TCPManager.h"
#include "LocalUDPDatagramsManager.h"
#include "LocalTCPTransmitter.h"
#include "RemoteTCPTransmitter.h"
#include "AutoConnectionManager.h"
#include "MemoryCleanerManager.h"
#include "StatisticsUpdateManager.h"
#include "NetSensor.h"


#if defined (USE_DISSERVICE)
namespace IHMC_ACI
{
    class DisseminationService;
}
#endif


namespace NOMADSUtil
{
    struct ARPPacket;
    struct EtherMACAddr;
}


namespace ACMNetProxy
{
    struct QueryResult;

    class NetworkInterface;


    #if defined (USE_DISSERVICE)
        class PacketRouter : public IHMC_ACI::DisseminationServiceListener
    #else
        class PacketRouter
    #endif
    {
    public:
        PacketRouter (void);
        explicit PacketRouter (const PacketRouter & rPacketRouter) = delete;
        ~PacketRouter (void);

        // Initialize the PacketRouter
        int init (const std::string & sHomeDir, const std::string & sConfigFilePath);
        int startThreads (void);
        int joinThreads (void);
        void requestTermination (void);

        bool isTerminationRequested (void);
        std::shared_ptr<NetworkInterface> & getInternalNetworkInterface (void);
        std::shared_ptr<NetworkInterface> & getExternalNetworkInterfaceWithIP (uint32 ui32IPv4Address);
        std::shared_ptr<NetworkInterface> selectMainExternalInterfaceInTheSameNetwork (uint32 ui32IPv4DestinationAddress);

        LocalTCPTransmitterThread & getLocalTCPTransmitterThread (void);
        RemoteTCPTransmitterThread & getRemoteTCPTransmitterThread (void);

        bool hostBelongsToTheInternalNetwork (const NOMADSUtil::EtherMACAddr & emaHost);
        bool hostBelongsToTheExternalNetwork (const NOMADSUtil::EtherMACAddr & emaHost);
        bool isIPv4AddressAssignedToInternalInterface (uint32 ui32IPv4Address);
        bool isIPv4AddressAssignedToExternalInterface (uint32 ui32IPv4Address);
        bool isMACAddressAssignedToInternalInterface (const NOMADSUtil::EtherMACAddr & ema);
        bool isMACAddressAssignedToExternalInterface (const NOMADSUtil::EtherMACAddr & ema);
        bool areIPv4AndMACAddressAssignedToInternalInterface (uint32 ui32IPv4Address, const NOMADSUtil::EtherMACAddr & ema);
        bool areIPv4AndMACAddressAssignedToExternalInterface (uint32 ui32IPv4Address, const NOMADSUtil::EtherMACAddr & ema);

        void addMACToIntHostsSet (const NOMADSUtil::EtherMACAddr & macAddr);
        void addMACToExtHostsSet (const NOMADSUtil::EtherMACAddr & macAddr);
        void updateMulticastBroadcastPacketForwardingRulesForNID (const NetworkInterfaceDescriptor & nid);

        // The following method is useful whenever a new Connection has been established, to reduce latency in case there are enqued packets/requests
        void wakeUpAutoConnectionAndRemoteTransmitterThreads (void);

        int handlePacketFromInternalInterface (uint8 * const pPacket, uint16 ui16PacketLen, NetworkInterface * const pReceivingNetworkInterface);
        int handlePacketFromExternalInterface (uint8 * const pPacket, uint16 ui16PacketLen, NetworkInterface * const pReceivingNetworkInterface);

        int initializeRemoteConnection (uint32 ui32RemoteProxyID, uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                                        ConnectorType connectorType, EncryptionType encryptionType);

        // Ethernet
        int wrapEthernetIPFrameAndSendToHost (NetworkInterface * const pNI, uint8 * ui8Buf, uint16 ui16PacketLen,
                                              NOMADSUtil::EtherFrameHeader const * const pEtherFrameHeaderPckt = nullptr);
        int sendTunneledPacketToLocalHost (NetworkInterface * const pNI, const uint8 * const pPacket, int iSize);

        // ARP
        int sendARPRequest (NetworkInterface * const pNI, uint32 ui32TargetProtocolAddress);
        int sendARPReplyToHost (NetworkInterface * const pNI, const NOMADSUtil::ARPPacket * const pARPReqPacket,
                                const NOMADSUtil::EtherMACAddr & rSourceHardwreAddress);
        int sendARPAnnouncement (std::unordered_map<uint32, std::shared_ptr<NetworkInterface>> & umExternalInterfaces,
                                 const NOMADSUtil::ARPPacket * const pARPReqPacket, uint32 ui32IPAddr);
        int sendARPAnnouncement (NetworkInterface * const pNI, const NOMADSUtil::ARPPacket * const pARPReqPacket,
                                 uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr & rMACAddr);
        int sendARPRequestForGatewayMACAddress (NetworkInterface * const pNI, const NetworkInterfaceDescriptor & nid);

        // ICMP
        int buildAndSendICMPMessageToHost (NetworkInterface * const pNI, NOMADSUtil::ICMPHeader::Type ICMPType,
                                           NOMADSUtil::ICMPHeader::Code_Destination_Unreachable ICMPCode, uint32 ui32SourceIP,
                                           uint32 ui32DestinationIP, NOMADSUtil::IPHeader * const pRcvdIPPacket);
        int forwardICMPMessageToHost (uint32 ui32LocalTargetIP, uint32 ui32RemoteOriginationIP, uint32 ui32RemoteProxyIP, uint8 ui8PacketTTL,
                                      NOMADSUtil::ICMPHeader::Type ICMPType, NOMADSUtil::ICMPHeader::Code_Destination_Unreachable ICMPCode,
                                      uint32 ui32RoH, const uint8 * const pICMPData, uint16 ui16PayloadLen);

        // UDP
        int sendUDPUniCastPacketToHost (uint32 ui32RemoteOriginationIP, uint32 ui32LocalTargetIP, uint8 ui8PacketTTL,
                                        const NOMADSUtil::UDPHeader * const pUDPPacket, const NOMADSUtil::IPHeader * const pIPHeaderPckt = nullptr,
                                        NOMADSUtil::EtherFrameHeader const * const pEtherFrameHeaderPckt = nullptr);
        int sendUDPBCastMCastPacketToHost (const uint8 * const pPacket, uint16 ui16PacketLen);
        int sendBroadcastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32BroadcastSrcIP, uint16 ui16SrcPort,
                                 uint32 ui32BroadcastDestIP, uint16 ui16DestPort, const CompressionSettings & compressionSettings);
        int sendMulticastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32MulticastSrcIP, uint16 ui16SrcPort,
                                 uint32 ui32MulticastDestIP, uint16 ui16DestPort, const CompressionSettings & compressionSettings);
        int sendBCastMCastPacketToDisService (const uint8 * const pPacket, uint16 ui16PacketLen);

        static MutexCounter<uint16> * const getMutexCounter (void);
        static const NetworkInterfaceDescriptor & getNetworkInterfaceDescriptorWithIP (uint32 ui32IPv4Address);

        // Ethernet
        static int sendPacketToHost (NetworkInterface * const pNI, const uint8 * const pPacket, int iSize);
        static int sendPacketToHost (const std::vector<NetworkInterface *> & vExternalInterfaces, const uint8 * const pPacket, int iSize);
        static int sendPacketToHost (const std::vector<std::shared_ptr<NetworkInterface>> & vspNI, const uint8 * const pPacket, int iSize);
        static int sendPacketToHost (const std::unordered_set<std::shared_ptr<NetworkInterface>> & usspNI, const uint8 * const pPacket, int iSize);
        static int sendPacketToHost (std::unordered_map<uint32, std::shared_ptr<NetworkInterface>> & umExternalInterfaces,
                                     const uint8 * const pPacket, int iSize);


    private:
        int setupNetworkInterfaces (void);
        int setupNetSensor (void);

        int sendCachedPacketsToDestination (uint32 ui32DestinationIPAddress);

        int sendPacketOverTheTunnel (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32SourceIP, uint32 ui32DestinationIP);
        int sendPacketOverTheTunnelImpl (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32SourceIP, uint32 ui32DestinationIP,
                                         const QueryResult & qrQuery, ConnectorType ct, EncryptionType et);

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

        static bool isMACAddrBroadcast (const NOMADSUtil::EtherMACAddr & macAddr);
        static bool isMACAddrMulticast (const NOMADSUtil::EtherMACAddr & macAddr);

        static void hton (NOMADSUtil::EtherFrameHeader * const pEtherFrame);
        static void ntoh (NOMADSUtil::EtherFrameHeader * const pEtherFrame);
        static uint16 getEthernetHeaderLength (NOMADSUtil::EtherFrameHeader * const pEtherFrame);
        static uint16 getEtherTypeFromEthernetFrame (NOMADSUtil::EtherFrameHeader * const pEtherFrame);
        static void * getPacketWithinEthernetFrame (NOMADSUtil::EtherFrameHeader * const pEtherFrame);

        static int updateMACAddressForDefaultGatewayWithIPv4Address (uint32 ui32IPv4Address, const NOMADSUtil::EtherMACAddr & ema);
        static int updateDescriptorFromNetworkInterface (NetworkInterfaceDescriptor & nid, std::shared_ptr<NetworkInterface> spNetworkInterface);


        std::atomic<bool> _bInitSuccessful;
        std::atomic<bool> _bTerminationRequested;

        std::unordered_set<uint64> _usInternalHosts;
        std::unordered_set<uint64> _usExternalHosts;
        std::shared_ptr<NetworkInterface> _spInternalInterface;
        std::unordered_map<uint32, std::shared_ptr<NetworkInterface>> _umExternalInterfaces;
        std::unordered_map<std::string, std::unordered_set<std::shared_ptr<NetworkInterface>>>
            _umInterfacesMulticastPacketsForwardingRules;
        std::unordered_map<std::string, std::unordered_set<std::shared_ptr<NetworkInterface>>>
            _umInterfacesBroadcastPacketsForwardingRules;
        PacketBufferManager _packetBufferManager;

        ARPCache _ARPCache;
        ARPTableMissCache _ARPTableMissCache;
        TCPConnTable _TCPConnTable;
        ConnectionManager _connectionManager;
        ConfigurationManager _configurationManager;
        StatisticsManager _statisticsManager;
        TCPManager _TCPManager;
        std::unique_ptr<IHMC_NETSENSOR::NetSensor> _upNetSensor;

        #if defined (USE_DISSERVICE)
            static IHMC_ACI::DisseminationService * const _pDisService;
        #endif

        // Handles receiving data from host's virtual interface (in host mode) or from the internal network (in gateway mode)
        PacketReceiver _internalPacketReceiverThread;
        // Keeps track of all handlers that receive data from an external interface (in gateway mode)
        std::vector<PacketReceiver> _vExternalPacketReceiverThreads;
        // Handles storing, buffering, wrapping, and subsequent forwarding of received UDP datagram packets
        LocalUDPDatagramsManagerThread _localUDPDatagramsManagerThread;
        // Handles transmission of data to host's virtual interface
        LocalTCPTransmitterThread _localTCPTransmitterThread;
        // Handles transmission of buffered data to remote proxies
        RemoteTCPTransmitterThread _remoteTCPTransmitterThread;
        // Handles sending AutoConnection requests to remote proxies
        AutoConnectionManager _autoConnectionManagerThread;
        // Handles cleaning up of connections
        MemoryCleanerManagerThread _memoryCleanerManagerThread;
        // Handles sending of statsUpdate messages to listeners
        StatisticsUpdateManagerThread _statisticsUpdateManagerThread;

        std::mutex _mtxIntHostsSet;
        std::mutex _mtxExtHostsSet;
    };


    inline PacketRouter::PacketRouter (void) :
        _bInitSuccessful{false}, _bTerminationRequested{false}, _ARPCache{}, _packetBufferManager{}, _spInternalInterface{nullptr},
        _ARPTableMissCache{NetworkConfigurationSettings::DEFAULT_ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS},
        _configurationManager{_ARPCache, _ARPTableMissCache, _connectionManager, _statisticsManager},
        _TCPManager{_packetBufferManager, _TCPConnTable, _connectionManager, _configurationManager, _statisticsManager, *this},
        _upNetSensor{nullptr}, _autoConnectionManagerThread{_connectionManager, _TCPConnTable, _TCPManager, *this, _statisticsManager},
        _localTCPTransmitterThread{_TCPConnTable, _TCPManager}, _remoteTCPTransmitterThread{_TCPConnTable, _connectionManager, _TCPManager},
        _memoryCleanerManagerThread{_ARPTableMissCache, _TCPConnTable, _TCPManager}, _statisticsUpdateManagerThread{_statisticsManager}
    { }

    inline PacketRouter::~PacketRouter (void)
    {
        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            if (NetProxyApplicationParameters::ACTIVATE_NETSENSOR) {
                if (_upNetSensor) {
                    _upNetSensor->requestTerminationAndWait();
                }
            }
        }

        requestTermination();
    }

    inline bool PacketRouter::isTerminationRequested (void)
    {
        return _bTerminationRequested;
    }

    inline std::shared_ptr<NetworkInterface> & PacketRouter::getInternalNetworkInterface (void)
    {
        return _spInternalInterface;
    }

    inline std::shared_ptr<NetworkInterface> & PacketRouter::getExternalNetworkInterfaceWithIP (uint32 ui32IPv4Address)
    {
        return _umExternalInterfaces.at (ui32IPv4Address);
    }

    inline LocalTCPTransmitterThread & PacketRouter::getLocalTCPTransmitterThread (void)
    {
        return _localTCPTransmitterThread;
    }

    inline RemoteTCPTransmitterThread & PacketRouter::getRemoteTCPTransmitterThread (void)
    {
        return _remoteTCPTransmitterThread;
    }

    inline bool PacketRouter::hostBelongsToTheInternalNetwork (const NOMADSUtil::EtherMACAddr & emaHost)
    {
        std::lock_guard<std::mutex> lg{_mtxIntHostsSet};
        return _usInternalHosts.count (etherMACAddrTouint64 (emaHost)) == 1;
    }

    inline bool PacketRouter::hostBelongsToTheExternalNetwork (const NOMADSUtil::EtherMACAddr & emaHost)
    {
        std::lock_guard<std::mutex> lg{_mtxExtHostsSet};
        return _usExternalHosts.count (etherMACAddrTouint64 (emaHost)) == 1;
    }

    inline bool PacketRouter::isIPv4AddressAssignedToInternalInterface (uint32 ui32IPv4Address)
    {
        return NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address == ui32IPv4Address;
    }

    inline bool PacketRouter::isIPv4AddressAssignedToExternalInterface (uint32 ui32IPv4Address)
    {
        return _umExternalInterfaces.count (ui32IPv4Address) == 1;
    }

    inline bool PacketRouter::isMACAddressAssignedToInternalInterface (const NOMADSUtil::EtherMACAddr & ema)
    {
        return NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress == ema;
    }

    inline bool PacketRouter::isMACAddressAssignedToExternalInterface (const NOMADSUtil::EtherMACAddr & ema)
    {
        return std::find_if (NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cbegin(),
                             NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cend(),
                             [ema](const NetworkInterfaceDescriptor & nid)
        { return nid.emaInterfaceMACAddress == ema; } ) != NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cend();
    }

    inline bool PacketRouter::areIPv4AndMACAddressAssignedToInternalInterface (uint32 ui32IPv4Address, const NOMADSUtil::EtherMACAddr & ema)
    {
        return (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address == ui32IPv4Address) &&
            (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress == ema);
    }

    inline void PacketRouter::addMACToIntHostsSet (const NOMADSUtil::EtherMACAddr & macAddr)
    {
        std::lock_guard<std::mutex> lg{_mtxIntHostsSet};
        _usInternalHosts.emplace (etherMACAddrTouint64 (macAddr));
    }

    inline void PacketRouter::addMACToExtHostsSet (const NOMADSUtil::EtherMACAddr & macAddr)
    {
        std::lock_guard<std::mutex> lg{_mtxExtHostsSet};
        _usExternalHosts.emplace (etherMACAddrTouint64 (macAddr));
    }

    inline MutexCounter<uint16> * const PacketRouter::getMutexCounter (void)
    {
        static MutexCounter<uint16>
            gIPIdentProvider{static_cast<uint16> (NOMADSUtil::ISAACRand::getRnd (static_cast<uint32> (NOMADSUtil::getTimeInMilliseconds())))};

        return &gIPIdentProvider;
    }

    inline bool PacketRouter::isMACAddrBroadcast (const NOMADSUtil::EtherMACAddr & macAddr)
    {
        return (macAddr.ui16Word1 == 0xFFFF) && (macAddr.ui16Word2 == 0xFFFF) && (macAddr.ui16Word3 == 0xFFFF);
    }

    inline bool PacketRouter::isMACAddrMulticast (const NOMADSUtil::EtherMACAddr & macAddr)
    {
        return (macAddr.ui8Byte1 == 0x01) && (macAddr.ui8Byte2 == 0x00) && (macAddr.ui8Byte3 == 0x5E);
    }

    inline bool PacketRouter::areIPv4AndMACAddressAssignedToExternalInterface (uint32 ui32IPv4Address, const NOMADSUtil::EtherMACAddr & ema)
    {
        return std::find_if (NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cbegin(),
                             NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cend(),
                             [ui32IPv4Address, ema](const NetworkInterfaceDescriptor & nid)
        { return (nid.ui32IPv4Address == ui32IPv4Address) && (nid.emaInterfaceMACAddress == ema); } ) !=
            NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cend();
    }

    inline void PacketRouter::hton (NOMADSUtil::EtherFrameHeader * const pEtherFrame)
    {
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1Q) {
            return reinterpret_cast<NOMADSUtil::EtherFrameHeader802_1Q * const> (pEtherFrame)->hton();
        }
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1AD) {
            return reinterpret_cast<NOMADSUtil::EtherFrameHeader802_1AD * const> (pEtherFrame)->hton();
        }

        return pEtherFrame->hton();
    }

    inline void PacketRouter::ntoh (NOMADSUtil::EtherFrameHeader * const pEtherFrame)
    {
        if (ntohs (pEtherFrame->ui16EtherType) == NOMADSUtil::ET_802_1Q) {
            return reinterpret_cast<NOMADSUtil::EtherFrameHeader802_1Q * const> (pEtherFrame)->ntoh();
        }
        if (ntohs (pEtherFrame->ui16EtherType) == NOMADSUtil::ET_802_1AD) {
            return reinterpret_cast<NOMADSUtil::EtherFrameHeader802_1AD * const> (pEtherFrame)->ntoh();
        }

        return pEtherFrame->ntoh();
    }

    inline uint16 PacketRouter::getEthernetHeaderLength (NOMADSUtil::EtherFrameHeader * const pEtherFrame)
    {
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1Q) {
            return sizeof(NOMADSUtil::EtherFrameHeader802_1Q);
        }
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1AD) {
            return sizeof(NOMADSUtil::EtherFrameHeader802_1AD);
        }

        return sizeof(NOMADSUtil::EtherFrameHeader);
    }

    inline uint16 PacketRouter::getEtherTypeFromEthernetFrame (NOMADSUtil::EtherFrameHeader * const pEtherFrame)
    {
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1Q) {
            return reinterpret_cast<NOMADSUtil::EtherFrameHeader802_1Q * const> (pEtherFrame)->ui16EtherType;
        }
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1AD) {
            return reinterpret_cast<NOMADSUtil::EtherFrameHeader802_1AD * const> (pEtherFrame)->ui16EtherType;
        }

        return pEtherFrame->ui16EtherType;
    }

    inline void * PacketRouter::getPacketWithinEthernetFrame (NOMADSUtil::EtherFrameHeader * const pEtherFrame)
    {
        char * pPacket = reinterpret_cast<char *> (pEtherFrame);
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1Q) {
            return pPacket + sizeof(NOMADSUtil::EtherFrameHeader802_1Q);
        }
        if (pEtherFrame->ui16EtherType == NOMADSUtil::ET_802_1AD) {
            return pPacket + sizeof(NOMADSUtil::EtherFrameHeader802_1AD);
        }

        return pPacket + sizeof(NOMADSUtil::EtherFrameHeader);
    }

    inline int PacketRouter::updateMACAddressForDefaultGatewayWithIPv4Address (uint32 ui32IPv4Address, const NOMADSUtil::EtherMACAddr & ema) {
        for (auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            if (nid.ui32IPv4GatewayAddress == ui32IPv4Address) {
                nid.emaNetworkGatewayMACAddress = ema;
                return 1;
            }
        }

        return 0;
    }
}

#endif   // #ifndef INCL_PACKET_ROUTER_H
