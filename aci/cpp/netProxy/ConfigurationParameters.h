#ifndef INCL_CONFIGURATION_PARAMETERS_H
#define INCL_CONFIGURATION_PARAMETERS_H

/*
 * ConfigurationParameters.h
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
 * This file defines two structs: NetworkConfigurationSettings and
 * NetProxyApplicationParameters.
 * The NetworkConfigurationSettings struct defines global parameters
 * related to the configuration of connections to remote NetProxies.
 * The NetProxyApplicationParameters struct defines global parameters
 * used for the general configuration of the NetProxy component and
 * for the configuration of connections to local nodes (if running in
 * gateway mode) or to local applications (if running in host mode).
 */

#include <string>
#include <vector>

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "InetAddr.h"

#include "ProxyMessages.h"
#include "NetworkInterfaceDescriptor.h"


namespace ACMNetProxy
{
    namespace NetworkConfigurationSettings
    {
        constexpr unsigned int PCAP_SEND_PACKET_ATTEMPTS = 3;                                               // Total number of attempts before failing transmission of a packet over the network with PCAP
        constexpr unsigned int PCAP_SEND_ATTEMPT_INTERVAL_IN_MS = 15;                                       // Wait interval before trying a new packet send with PCAP if the previous attempt failed

        constexpr int64 DEFAULT_ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS = 3000;                                // Configured default expiration time (in ms) after which entries in the ARP Table Miss Cache are deleted
        constexpr uint16 MIN_IP_HEADER_SIZE = 20U;                                                          // Minimum size of the IP Header
        constexpr uint16 TCP_WINDOW_SIZE = 65535U;                                                          // Window size for incoming TCP connections (in bytes)
        constexpr uint16 DEFAULT_MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = 1024;                            // Default maximum payload size (in bytes) of TCPData Proxy Messages that will be sent to remote proxies
        constexpr uint16 DEFAULT_MAX_TCP_UNACKED_DATA = 536U;                                               // Default maximum segment size that TCP can skip ACKing
        constexpr uint32 IDLE_TCP_CONNECTION_NOTIFICATION_TIME = 5000U;                                     // Time after which a TCP connection is considered idle (amount of time without any actual activity)
        constexpr uint32 IDLE_TCP_FAST_RETRANSMIT_TRIGGER_TIMEOUT = 1000U;                                  // Time after which a TCP connection is considered idle (amount of time without any actual activity)
        constexpr int64 SYN_SENT_RETRANSMISSION_TIMEOUTS[] =
            {100LL, 250LL, 500LL, 1000LL, 2000LL, 4000LL, 8000LL, 16000LL, 32000LL, 0x7FFFFFFFFFFFFFFFLL};  // TCP SYN retransmission timeouts
        constexpr uint32 SYN_SENT_RETRANSMISSION_TIMEOUT_ENTRIES =
            sizeof(SYN_SENT_RETRANSMISSION_TIMEOUTS) / sizeof(int64);                                       // Max number of SYN retransmissions when trying to establish a new TCP connection with a local application
        constexpr int64 SYN_SENT_FAILED_TIMEOUT = 60000U;                                                   // Timeout after which the attempt to establish a new TCP connection with a local application fails
        constexpr bool DEFAULT_SYNCHRONIZE_TCP_HANDSHAKE = true;                                            // Determines whether the NetProxy running at client-side waits to receive a TCPConnectionOpened ProxyMessage from the NetProxy at server-side before sending a SYN-ACK to the client application
        constexpr bool DEFAULT_TCP_TIME_WAIT_IGNORE_STATE = false;                                          // Determines whether the NetProxy running at server-side checks if the TCPConnTable Entry is in state TIME_WAIT before sending a SYN
        constexpr double DEFAULT_UPDATE_TCP_WINDOW_ACK_THRESHOLD = 30.0;                                    // Threshold of the TCP window free size ratio that, when passed, triggers a TCP window update notification to be sent to the local application
        constexpr uint32 MAX_UDP_CONNECTION_BUFFER_SIZE = 262144U;                                          // Maximum allowed size of the buffer which stores outgoing data in the UDP connection thread
        constexpr uint32 DEFAULT_UDP_NAGLE_ALGORITHM_TIMEOUT = 0U;                                          // 0 implies that Nagle's Algorithm is disabled
        constexpr uint32 MAX_UDP_NAGLE_ALGORITHM_TIMEOUT = 60000U;                                          // Maximum allowed value for the timeout of the UDP Nagle's Algorithm
        constexpr uint16 DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD = 1024U;                           // Size (in bytes) beyond which a MultipleUDPDatagrams ProxyMessage is sent without any timeout to expire
        constexpr uint32 DEFAULT_UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS = 0U;                               // 0 implies that no throughput limits are applied to the data sent via UDP
        constexpr uint32 DEFAULT_UDP_CONNECTION_BUFFER_SIZE = 16384U;                                       // Default size of the buffer which stores outgoing data in the UDP connection thread
        constexpr int32 DEFAULT_AUTO_RECONNECT_TIME = 30000U;                                               // Default interval of time (in millisecond) to wait before attempting a new AutoConnection to remote proxies
        constexpr uint32 DEFAULT_DUPLICATE_ACK_TIMEOUT = 1000U;                                             // Timeout which manages the sending of duplicate ACKs to local hosts in FIN-WAIT-1 to prevent TCP to expire

        constexpr uint16 PROXY_MESSAGE_MTU = 8192U + sizeof(TCPDataProxyMessage);                           // MTU (in bytes) for packets addressed to remote proxies
        constexpr uint32 DEFAULT_VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT = 30000U;                               // Default inactivity time after which a local TCP connection is reset
        constexpr bool DEFAULT_LOCAL_PROXY_REACHABILITY_FROM_REMOTE = true;

        constexpr auto DEFAULT_ENABLED_CONNECTORS = "Mockets";                                              // By default, the NetProxy only initializes Mocket Connectors

        extern int64 ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS;
        extern uint16 MAX_TCP_UNACKED_DATA;
        extern uint16 MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE;
        extern double UPDATE_TCP_WINDOW_ACK_THRESHOLD;
        extern bool SYNCHRONIZE_TCP_HANDSHAKE;                                                              // Determines whether the NetProxy running at client-side waits to receive a TCPConnectionOpened ProxyMessage from the NetProxy at server-side before sending a SYN-ACK to the client application
        extern bool TCP_TIME_WAIT_IGNORE_STATE;                                                             // Determines whether the NetProxy running at the server side checks if the TCPConnTable Entry is in state TIME_WAIT before sending a SYN
        extern uint32 UDP_NAGLE_ALGORITHM_TIMEOUT;
        extern uint16 MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;
        extern uint32 UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS;
        extern uint32 UDP_CONNECTION_BUFFER_SIZE;

        extern uint32 VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT;
    };


    namespace NetProxyApplicationParameters
    {
        // Default values
        constexpr bool DEFAULT_GATEWAY_MODE = false;                                                        // Default gateway mode is false
        constexpr bool DEFAULT_TRANSPARENT_GATEWAY_MODE = true;                                             // By default, the NetProxy operates in transparent Gateway Mode
        constexpr bool DEFAULT_LEVEL2_TUNNEL_MODE = false;                                                  // Default level 2 tunnel mode is false

        constexpr auto S_DEFAULT_MAIN_CONFIGURATION_FILE_NAME = "netproxy.cfg";                             // Default main config file name
        constexpr auto S_DEFAULT_LOG_DETAIL_LEVEL = "L_Info";                                               // Default logging detail level

        constexpr bool DEFAULT_ENABLE_PRIORITIZATION_MECHANISM = false;
        constexpr uint32 MIN_ENTRY_BUF_SIZE = 8192U;
        constexpr uint32 MAX_ENTRY_BUF_SIZE = 32768U;
        constexpr uint8 TCP_CONN_TABLE_ENTRIES_POOL_SIZE = 10U;

        constexpr bool DEFAULT_GENERATE_STATISTICS_UPDATES = true;                                          // Default behavior is to enable UpdateGUIThread
        constexpr uint64 I64_DEFAULT_PROCESS_MEASURE_INTERVAL_IN_MS = 4 * 1000U;
        constexpr uint64 I64_DEFAULT_ADDRESS_MEASURE_INTERVAL_IN_MS = 4 * 1000U;
        constexpr uint64 I64_DEFAULT_PROTOCOL_MEASURE_INTERVAL_IN_MS = 4 * 1000U;
        constexpr uint64 I64_DEFAULT_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS = 4 * 1000U;
        constexpr uint64 I64_DEFAULT_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS = 4 * 1000U;
        constexpr uint64 I64_DEFAULT_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS = 4 * 1000U;
        constexpr uint64 I64_DEFAULT_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS = 4 * 1000U;
        constexpr uint64 I64_DEFAULT_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS =
            5 * I64_DEFAULT_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS;
        constexpr uint64 I64_DEFAULT_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS =
            5 * I64_DEFAULT_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS;

        constexpr bool DEFAULT_ACTIVATE_NETSENSOR = false;
        constexpr auto DEFAULT_NETSENSOR_STATISTICS_IP_ADDRESS = "127.0.0.1";
        constexpr auto DEFAULT_STATS_COLLECTOR_IP_ADDR = "127.0.0.1";
        constexpr uint16 DEFAULT_STATS_COLLECTOR_PORT = 8755U;
        constexpr auto DEFAULT_STATUS_NOTIFICATION_ADDRESS = "127.0.0.1:8755";

        // TAP Interface configuration values
        constexpr uint16 TAP_INTERFACE_DEFAULT_MTU = 1500U;                                                 // Deafult value of the MTU of the Virtual TAP interface
        constexpr uint32 TAP_INTERFACE_READ_TIMEOUT = 500U;                                                 // Timeout for the call to WaitForSingleObject() on the TAP handle
        // First 4 bytes of the MAC Address of the virtual TAP interface
        constexpr uint8 VIRT_MAC_ADDR_BYTE1 = 0x02;
        constexpr uint8 VIRT_MAC_ADDR_BYTE2 = 0x0A;
        constexpr uint8 VIRT_MAC_ADDR_BYTE3 = 0x0C;
        constexpr uint8 VIRT_MAC_ADDR_BYTE4 = 0x00;

        // Network interface configuration values
        constexpr uint16 ETHERNET_DEFAULT_MTU = 1500U;                                                      // Default MTU (in bytes) for packets transmitted within ethernet network frames
        constexpr uint16 ETHERNET_DEFAULT_MFS = 1538U;                                                      // Default MFS (in bytes) for frames written to/read from the ethernet network interface
        constexpr uint16 ETHERNET_MIN_MTU = 68U;                                                            // Min allowed value as MTU of the Virtual TUN/TAP interface
        constexpr uint16 ETHERNET_MAX_MTU = 9000U;                                                          // Max allowed MTU (in bytes) for frames written to/read from the ethernet network interface
        constexpr uint16 ETHERNET_MAX_MFS = 9038U;                                                          // Max allowed MFS (in bytes) for frames written to/read from the ethernet network interface
        const NOMADSUtil::InetAddr IA_INVALID_ADDR{};                                                       // Invalid InetAddr (0.0.0.0)
        constexpr NOMADSUtil::EtherMACAddr EMA_INVALID_ADDRESS{ 0x0000U, 0x0000U, 0x0000U };                // Invalid MAC address (00:00:00:00:00:00)
        constexpr NOMADSUtil::EtherMACAddr EMA_BROADCAST_ADDR{ 0xFFFFU, 0xFFFFU, 0xFFFFU };                 // Broadcast MAC address (FF:FF:FF:FF:FF:FF)
        const NetworkInterfaceDescriptor NID_INVALID{};                                                     // NetworkInterfaceDescriptor of the internal interface
        constexpr auto pszNetworkAdapterNamePrefix =
            #if defined (WIN32)
                "\\Device\\NPF_";
            #else
                "";
            #endif

        constexpr uint8 WRITE_PACKET_BUFFERS = 3U;                                                          // Number of buffers available to write on the TUN/TAP (or libpcap internal) interface

        // Default port numbers and timeouts for Mocket, TCP, and UDP servers
        constexpr uint16 DEFAULT_MOCKET_SERVER_PORT = 8751U;
        constexpr uint16 DEFAULT_TCP_SERVER_PORT = 8751U;
        constexpr uint16 DEFAULT_UDP_SERVER_PORT = 8752U;
        constexpr auto DEFAULT_CSR_PROXY_SERVER_ADDR = "127.0.0.1";
        constexpr uint16 DEFAULT_CSR_PROXY_SERVER_PORT = 7878U;
        constexpr uint32 DEFAULT_MOCKET_TIMEOUT = 30000U;

        constexpr bool DEFAULT_MOCKETS_DTLS_ENABLED = false;


        // Configuration parameters
        extern uint32 NETPROXY_UNIQUE_ID;                                                                   // The 32bits unique ID of this NetProxy instance
        extern bool GATEWAY_MODE;                                                                           // True if the NetProxy runs in gateway mode
        extern bool TRANSPARENT_GATEWAY_MODE;                                                               // When false, the TTL field of IP packets forwarded from the internal to the external network is decremented by 1
        extern bool LEVEL2_TUNNEL_MODE;                                                                     // True if the NetProxy runs in level 2 tunnel mode
        extern uint64 UI64_STARTUP_TIME_IN_MILLISECONDS;                                                    // Timestamp at startup

        extern bool ENABLE_PRIORITIZATION_MECHANISM;                                                        // Enables the use of the prioritization mechanism in the RTT

        extern bool GENERATE_STATISTICS_UPDATES;                                                            // Bool that states whether to start the UpdateGUIThread or not
        extern int64 I64_PROCESS_MEASURE_INTERVAL_IN_MS;
        extern int64 I64_ADDRESS_MEASURE_INTERVAL_IN_MS;
        extern int64 I64_PROTOCOL_MEASURE_INTERVAL_IN_MS;
        extern int64 I64_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS;
        extern int64 I64_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS;
        extern int64 I64_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS;
        extern int64 I64_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS;
        extern int64 I64_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS;
        extern int64 I64_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS;

        extern bool ACTIVATE_NETSENSOR;
        extern std::string NETSENSOR_STATISTICS_IP_ADDRESS;

        extern std::vector<NetworkInterfaceDescriptor> V_NID_EXTERNAL_INTERFACE_LIST;
        extern NetworkInterfaceDescriptor NID_INTERNAL_INTERFACE;                                           // NetworkInterfaceDescriptor of the internal interface

        extern uint16 MOCKET_SERVER_PORT;
        extern uint16 TCP_SERVER_PORT;
        extern uint16 UDP_SERVER_PORT;
        extern uint16 CSR_MOCKET_SERVER_PORT;
        extern const char * const CSR_PROXY_SERVER_ADDR;
        extern const uint16 CSR_PROXY_SERVER_PORT;
        extern uint32 MOCKET_TIMEOUT;

        extern bool MOCKETS_DTLS_ENABLED;                                                                   // If true, Mockets uses DTLS to secure messages
        extern std::string DEFAULT_MOCKETS_CONFIG_FILE;                                                     // Path to the default configuration files for Mockets
        extern std::string LOGS_DIR;                                                                        // Name of the directory where log files are stored

    };

}

#endif      // INCL_CONFIGURATION_PARAMETERS_H
