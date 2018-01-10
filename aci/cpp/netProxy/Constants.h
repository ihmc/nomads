#ifndef INCL_CONSTANTS_H
#define INCL_CONSTANTS_H

/*
 * Constants.h
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
 * File responsible for initializing all static variables in the program.
 */

#include "DArray.h"
#include "Mutex.h"
#include "ConditionVariable.h"
#include "StrClass.h"
#include "net/NetworkHeaders.h"

#include "ARPCache.h"
#include "ARPTableMissCache.h"
#include "ConfigurationManager.h"
#include "ConnectionManager.h"
#include "ConnectivitySolutions.h"
#include "ConnectorReader.h"
#include "PacketBufferManager.h"
#include "PacketRouter.h"
#include "RemoteProxyInfo.h"
#include "TCPConnTable.h"
#include "TCPManager.h"


namespace ACMNetProxy
{
    const char * const NetworkConfigurationSettings::DEFAULT_ENABLED_CONNECTORS = "Mockets";
    int64 NetworkConfigurationSettings::ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS = NetworkConfigurationSettings::DEFAULT_ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS;
    uint16 NetworkConfigurationSettings::MAX_TCP_UNACKED_DATA = NetworkConfigurationSettings::DEFAULT_MAX_TCP_UNACKED_DATA;
    uint16 NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = NetworkConfigurationSettings::DEFAULT_MAX_TCP_DATA_PAYLOAD_SIZE;
    uint32 NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT = NetworkConfigurationSettings::DEFAULT_UDP_NAGLE_ALGORITHM_TIMEOUT;
    uint16 NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD = NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;
    uint32 NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS = NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS;
    uint32 NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_BUFFER_SIZE;

    uint32 NetProxyApplicationParameters::NETPROXY_UNIQUE_ID = 0U;
    bool NetProxyApplicationParameters::GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_GATEWAY_MODE;
    bool NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE = NetProxyApplicationParameters::DEFAULT_LEVEL2_TUNNEL_MODE;
    NOMADSUtil::String NetProxyApplicationParameters::EXTERNAL_INTERFACE_NAME;
    NOMADSUtil::String NetProxyApplicationParameters::INTERNAL_INTERFACE_NAME;
    uint32 NetProxyApplicationParameters::EXTERNAL_IP_ADDR = 0U;
    uint32 NetProxyApplicationParameters::INTERNAL_IP_ADDR = 0U;
    uint32 NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK = 0U;
    uint32 NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK = 0U;
    NOMADSUtil::EtherMACAddr NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR = { 0x0000U, 0x0000U, 0x0000U };
    NOMADSUtil::EtherMACAddr NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR = { 0x0000U, 0x0000U, 0x0000U };
    uint32 NetProxyApplicationParameters::NETWORK_GATEWAY_IP_ADDR = 0U;
    NOMADSUtil::EtherMACAddr NetProxyApplicationParameters::NETWORK_GATEWAY_MAC_ADDR = { 0x0000U, 0x0000U, 0x0000U };
    const NOMADSUtil::EtherMACAddr NetProxyApplicationParameters::INVALID_MAC_ADDR = { 0x0000U, 0x0000U, 0x0000U };
    const NOMADSUtil::EtherMACAddr NetProxyApplicationParameters::BROADCAST_MAC_ADDR = { 0xFFFFU, 0xFFFFU, 0xFFFFU };
    uint16 NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF = 0U;
    uint16 NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF = 0U;
    uint16 NetProxyApplicationParameters::MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
    uint32 NetProxyApplicationParameters::MOCKET_TIMEOUT = NetProxyApplicationParameters::DEFAULT_MOCKET_TIMEOUT;
    uint16 NetProxyApplicationParameters::TCP_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT;
    uint16 NetProxyApplicationParameters::UDP_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT;
    uint16 NetProxyApplicationParameters::CSR_MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
    #if defined (WIN32)
        const char * const NetProxyApplicationParameters::pszNetworkAdapterNamePrefix = "\\Device\\NPF_";
    #else
        const char * const NetProxyApplicationParameters::pszNetworkAdapterNamePrefix = "";
    #endif
    const uint16 NetProxyApplicationParameters::CSR_PROXY_SERVER_PORT = 7878;
    const char * const NetProxyApplicationParameters::CSR_PROXY_SERVER_ADDR = "127.0.0.1";
    const char * const NetProxyApplicationParameters::DEFAULT_GUI_IP_ADDR = "127.0.0.1";
    bool NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED = false;
    bool NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_TRANSPARENT_GATEWAY_MODE;
    bool NetProxyApplicationParameters::MULTICAST_PACKETS_FORWARDING_ON_EXTERNAL_NETWORK =
        NetProxyApplicationParameters::DEFAULT_MULTICAST_PACKETS_FORWARDING_ON_EXTERNAL_NETWORK;
    bool NetProxyApplicationParameters::MULTICAST_PACKETS_FORWARDING_ON_INTERNAL_NETWORK =
        NetProxyApplicationParameters::DEFAULT_MULTICAST_PACKETS_FORWARDING_ON_INTERNAL_NETWORK;
    bool NetProxyApplicationParameters::BROADCAST_PACKETS_FORWARDING_ON_EXTERNAL_NETWORK =
        NetProxyApplicationParameters::DEFAULT_BROADCAST_PACKETS_FORWARDING_ON_EXTERNAL_NETWORK;
    bool NetProxyApplicationParameters::BROADCAST_PACKETS_FORWARDING_ON_INTERNAL_NETWORK =
        NetProxyApplicationParameters::DEFAULT_BROADCAST_PACKETS_FORWARDING_ON_INTERNAL_NETWORK;
    const int64 NetProxyApplicationParameters::SYN_SENT_RETRANSMISSION_TIMEOUTS[] = {1000LL, 3000LL, 7000LL, 15000LL, 31000LL, 0x7FFFFFFFFFFFFFFFLL};
	uint32 NetProxyApplicationParameters::VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT = NetProxyApplicationParameters::DEFAULT_VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT;
    const double NetProxyApplicationParameters::UPDATE_TCP_WINDOW_ACK_THRESHOLD = 30.0;
    bool NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED = NetProxyApplicationParameters::DEFAULT_UPDATE_GUI_THREAD_ENABLED;
    const char * const NetProxyApplicationParameters::DEFAULT_GUI_UPDATE_MESSAGE_HEADER = "ACMNP";
    #if defined (WIN32)
        NOMADSUtil::String NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE ("c:\\temp\\mockets.conf");
    #else
        NOMADSUtil::String NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE ("/tmp/mockets.conf");
    #endif
    const char * const NetProxyApplicationParameters::LOGS_DIR = "log";

    NOMADSUtil::String NetProxyApplicationParameters::NETSENSOR_STAT_DEST_IP = "localhost";
	bool NetProxyApplicationParameters::ACTIVATE_NETSENSOR = true;
	bool NetProxyApplicationParameters::PRIORITY_MECHANISM = false;

	const NOMADSUtil::String NetProxyConfigManager::UniqueIDsConfigFileReader::ACTIVE_CONNECTIVITY_CONFIG_PARAMETER("active");
	const NOMADSUtil::String NetProxyConfigManager::UniqueIDsConfigFileReader::PASSIVE_CONNECTIVITY_CONFIG_PARAMETER("passive");
	const NOMADSUtil::String NetProxyConfigManager::UniqueIDsConfigFileReader::BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER("bidirectional");

    const CompressionSetting CompressionSetting::DefaultNOCompressionSetting;

    const ProtocolSetting ProtocolSetting::INVALID_PROTOCOL_SETTING (ProxyMessage::PMP_UNDEF_PROTOCOL);
    const ProtocolSetting ProtocolSetting::DEFAULT_ICMP_PROTOCOL_SETTING (ProxyMessage::PMP_UDP);
    const ProtocolSetting ProtocolSetting::DEFAULT_TCP_PROTOCOL_SETTING (ProxyMessage::PMP_MocketsRS);
    const ProtocolSetting ProtocolSetting::DEFAULT_UDP_PROTOCOL_SETTING (ProxyMessage::PMP_MocketsUU);

    TCPConnTable * const TCPManager::_pTCPConnTable = TCPConnTable::getTCPConnTable();
    ConnectionManager * const TCPManager::_pConnectionManager = ConnectionManager::getConnectionManagerInstance();
    NetProxyConfigManager * const TCPManager::_pConfigurationManager = NetProxyConfigManager::getNetProxyConfigManager();
    PacketRouter * const TCPManager::_pPacketRouter = PacketRouter::getPacketRouter();

    const uint32 TCPConnTable::STANDARD_MSL = 2*60*1000;                // Maximum Segment Lifetime (2 minutes in RFC 793)
    const uint16 TCPConnTable::LB_RTO = 1*100;                          // Retransmission TimeOut Lower Bound of 100 milliseconds (in RFC 793 is 1 second)
    const uint16 TCPConnTable::UB_RTO = 3*1000;                         // Retransmission TimeOut Upper Bound of 60 seconds (RFC 793)
    const uint16 TCPConnTable::RTO_RECALCULATION_TIME = 1000;           // Time that has to pass before recalculating RTO (RFC 793)
    const double TCPConnTable::ALPHA_RTO = 0.85;                        // Alpha constant for RTO calculation (RFC 793)
    const double TCPConnTable::BETA_RTO = 0.85;                         // Beta constant for RTO calculation (RFC 793)

    ConnectionManager * const Connection::_pConnectionManager = ConnectionManager::getConnectionManagerInstance();
    NetProxyConfigManager * const Connection::_pConfigurationManager = NetProxyConfigManager::getNetProxyConfigManager();
    GUIStatsManager * const Connection::_pGUIStatsManager = GUIStatsManager::getGUIStatsManager();
    PacketRouter * const Connection::_pPacketRouter = PacketRouter::getPacketRouter();

    ConnectionManager * const Connector::_pConnectionManager = ConnectionManager::getConnectionManagerInstance();
    GUIStatsManager * const Connector::_pGUIStatsManager = GUIStatsManager::getGUIStatsManager();
    PacketRouter * const Connector::_pPacketRouter = PacketRouter::getPacketRouter();

    UDPSocketAdapter * const UDPConnector::_pUDPSocketAdapter = new UDPSocketAdapter (new NOMADSUtil::UDPDatagramSocket());
    ConnectionManager * const UDPSocketAdapter::P_CONNECTION_MANAGER = ConnectionManager::getConnectionManagerInstance();

    ConnectorReader *ConnectorReader::_pUDPConnectorReader = nullptr;
    ZLibConnectorReader *ConnectorReader::_pUDPZLibConnectorReader = nullptr;
    LzmaConnectorReader *ConnectorReader::_pUDPLzmaConnectorReader = nullptr;

    NOMADSUtil::DArray<ConnectorWriter*> ConnectorWriter::_UDPConnectorWriters (nullptr, CompressionSetting::MAX_COMPRESSION_TYPE_AND_LEVEL);

    ConnectionManager * const AutoConnectionEntry::P_CONNECTION_MANAGER = ConnectionManager::getConnectionManagerInstance();

    const NOMADSUtil::String NetProxyConfigManager::S_TCP_CONNECTOR ("TCP");
    const NOMADSUtil::String NetProxyConfigManager::S_UDP_CONNECTOR ("UDP");
    const NOMADSUtil::String NetProxyConfigManager::S_MOCKETS_CONNECTOR ("Mockets");
    const NOMADSUtil::String NetProxyConfigManager::S_CSR_CONNECTOR ("CSR");

    ConnectionManager * const NetProxyConfigManager::P_CONNECTION_MANAGER = ConnectionManager::getConnectionManagerInstance();

    PacketBufferManager * const PacketRouter::_pPBM (PacketBufferManager::getPacketBufferManagerInstance());
    bool PacketRouter::_bTerminationRequested = false;
    bool PacketRouter::_bConnectorsDeleted = false;
    bool PacketRouter::_bInternalReceiverThreadRunning = false;
    bool PacketRouter::_bExternalReceiverThreadRunning = false;
    bool PacketRouter::_bLocalUDPDatagramsManagerThreadRunning = false;
    bool PacketRouter::_bLocalTCPTransmitterThreadRunning = false;
    bool PacketRouter::_bRemoteTCPTransmitterThreadRunning = false;
    bool PacketRouter::_bAutoConnectionManagerThreadRunning = false;
    bool PacketRouter::_bCleanerThreadRunning = false;
    bool PacketRouter::_bUpdateGUIThreadRunning = false;
    ARPCache PacketRouter::_ARPCache;
    ARPTableMissCache PacketRouter::_ARPTableMissCache (NetworkConfigurationSettings::ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS);
    NOMADSUtil::DArray<NOMADSUtil::EtherMACAddr> PacketRouter::_daInternalHosts;
    NOMADSUtil::DArray<NOMADSUtil::EtherMACAddr> PacketRouter::_daExternalHosts;
    NetworkInterface * PacketRouter::_pInternalInterface = nullptr;
    NetworkInterface * PacketRouter::_pExternalInterface = nullptr;
    IHMC_NETSENSOR::NetSensor * PacketRouter::_pnsNetSensor = nullptr;
    TCPConnTable * const PacketRouter::_pTCPConnTable = TCPConnTable::getTCPConnTable();
    ConnectionManager * const PacketRouter::_pConnectionManager = ConnectionManager::getConnectionManagerInstance();
    NetProxyConfigManager * const PacketRouter::_pConfigurationManager = NetProxyConfigManager::getNetProxyConfigManager();
    #if defined (USE_DISSERVICE)
        IHMC_ACI::DisseminationService * const PacketRouter::_pDisService = nullptr;
    #endif
    PacketRouter::InternalReceiverThread PacketRouter::_internalReceiverThread;
    PacketRouter::ExternalReceiverThread PacketRouter::_externalReceiverThread;
    PacketRouter::LocalUDPDatagramsManagerThread PacketRouter::_localUDPDatagramsManagerThread;
    PacketRouter::LocalTCPTransmitterThread PacketRouter::_localTCPTransmitterThread;
    PacketRouter::RemoteTCPTransmitterThread PacketRouter::_remoteTCPTransmitterThread;
    PacketRouter::AutoConnectionManager PacketRouter::_autoConnectionManagerThread;
    PacketRouter::CleanerThread PacketRouter::_cleanerThread;
    PacketRouter::UpdateGUIThread PacketRouter::_updateGUIThread;
    NOMADSUtil::Mutex PacketRouter::_mLocalUDPDatagramsManager;
    NOMADSUtil::Mutex PacketRouter::_mLocalTCPTransmitter;
    NOMADSUtil::Mutex PacketRouter::_mRemoteTCPTransmitter;
    NOMADSUtil::Mutex PacketRouter::_mAutoConnectionManager;
    NOMADSUtil::Mutex PacketRouter::_mCleaner;
    NOMADSUtil::Mutex PacketRouter::_mGUIUpdater;
    NOMADSUtil::ConditionVariable PacketRouter::_cvLocalUDPDatagramsManager (&PacketRouter::_mLocalUDPDatagramsManager);
    NOMADSUtil::ConditionVariable PacketRouter::_cvLocalTCPTransmitter (&PacketRouter::_mLocalTCPTransmitter);
    NOMADSUtil::ConditionVariable PacketRouter::_cvRemoteTCPTransmitter (&PacketRouter::_mRemoteTCPTransmitter);
    NOMADSUtil::ConditionVariable PacketRouter::_cvAutoConnectionManager (&PacketRouter::_mAutoConnectionManager);
    NOMADSUtil::ConditionVariable PacketRouter::_cvCleaner (&PacketRouter::_mCleaner);
    NOMADSUtil::ConditionVariable PacketRouter::_cvGUIUpdater (&PacketRouter::_mGUIUpdater);
}

#endif      // INCL_CONSTANTS_H
