#ifndef INCL_CONFIGURATION_PARAMETERS_H
#define INCL_CONFIGURATION_PARAMETERS_H

/*
 * ConfigurationParameters.h
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
 * This file defines two structs: NetworkConfigurationSettings and
 * NetProxyApplicationParameters.
 * The NetworkConfigurationSettings struct defines global parameters
 * related to the configuration of connections to remote NetProxies.
 * The NetProxyApplicationParameters struct defines global parameters
 * used for the general configuration of the NetProxy component and
 * for the configuration of connections to local nodes (if running in
 * gateway mode) or to local applications (if running in host mode).
 */

#include "FTypes.h"
#include "StrClass.h"
#include "net/NetworkHeaders.h"

#include "ProxyMessages.h"

namespace ACMNetProxy
{
    struct NetworkConfigurationSettings
    {
        static const char * const DEFAULT_ENABLED_CONNECTORS;
        static uint16 MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE;                           // Configured maximum payload size (in bytes) of TCPData Proxy Messages that will be sent to remote proxies
        static uint32 UDP_NAGLE_ALGORITHM_TIMEOUT;                                       // Configured UDP Nagle's Algorthm timeout
        static uint16 MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;                           // Configured size (in bytes) beyond which a MultipleUDPDatagrams proxy message is sent without any timeout to expire
        static uint32 UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS;                            // Configured throughput limit for UDP Connections
        static uint32 UDP_CONNECTION_BUFFER_SIZE;                                        // Configured size of the buffer which stores outgoing data in the UDP connection thread

        static const uint16 DEFAULT_MAX_TCP_DATA_PAYLOAD_SIZE = 1024;                    // Default maximum payload size (in bytes) of TCPData Proxy Messages that will be sent to remote proxies
        static const uint16 DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD = 2048U;     // Size (in bytes) beyond which a MultipleUDPDatagrams proxy message is sent without any timeout to expire
        static const uint32 DEFAULT_UDP_NAGLE_ALGORITHM_TIMEOUT = 0U;                    // 0 implies that Nagle's Algorithm is disabled
        static const uint32 DEFAULT_UDP_CONNECTION_BUFFER_SIZE = 16384U;                 // Default size of the buffer which stores outgoing data in the UDP connection thread
        static const uint32 DEFAULT_UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS = 0U;         // 0 implies that no throughput limits are applied to the data sent via UDP
        static const uint32 DEFAULT_AUTO_RECONNECT_TIME = 30000U;                        // Default interval of time (in millisecond) to wait before attempting a new AutoConnection to remote proxies
        static const uint32 DEFAULT_DUPLICATE_ACK_TIMEOUT = 1000U;                       // Timeout which manages the sending of duplicate ACKs to local hosts in FIN-WAIT-1 to prevent TCP to expire
    };


    struct NetProxyApplicationParameters
    {
        static uint32 NETPROXY_UNIQUE_ID;                                                           // The 32bits unique ID of this NetProxy instance
        static bool GATEWAY_MODE;                                                                   // true if the NetProxy runs in gateway mode
        static const bool DEFAULT_GATEWAY_MODE = false;                                             // Default gateway mode is false
        static uint32 NETPROXY_IP_ADDR;                                                             // Local IP address (of the TAP interface if running in HM, of the external network interface otherwise)
        static uint32 NETPROXY_EXTERNAL_IP_ADDR;                                                    // IP address of the external network interface (same as the above if running in GM)
        static uint32 NETPROXY_NETWORK_NETMASK;                                                     // Netmask of the physical network to which the NetProxy and the configured gateway belong
        static NOMADSUtil::EtherMACAddr NETPROXY_INTERNAL_INTERFACE_MAC_ADDR;                       // MAC address of the internal network interface
        static NOMADSUtil::EtherMACAddr NETPROXY_EXTERNAL_INTERFACE_MAC_ADDR;                       // MAC address of the external network interface
        static uint32 NETWORK_GATEWAY_NODE_IP_ADDR;                                                 // IP address of the physical network gateway
        static NOMADSUtil::EtherMACAddr NETWORK_GATEWAY_NODE_MAC_ADDR;                              // MAC address of the physical network gateway
        static const NOMADSUtil::EtherMACAddr INVALID_MAC_ADDR;                                     // Invalid MAC address (00:00:00:00:00:00)
        static const NOMADSUtil::EtherMACAddr BROADCAST_MAC_ADDR;                                   // Broadcast MAC address

        // TAP Interface configuration values
        static uint16 TAP_INTERFACE_MTU;                                                            // MTU of the Virtual TAP Interface
        static const uint16 TAP_INTERFACE_DEFAULT_MTU = 1500U;                                      // Deafult value of the MTU of the Virtual TAP interface
        static const uint16 TAP_INTERFACE_MIN_MTU = 68U;                                            // Min allowed value as MTU of the Virtual TUN/TAP interface
        static const uint16 TAP_INTERFACE_MAX_MTU = 9000U;                                          // Max allowed value as MTU of the Virtual TUN/TAP interface
        static const uint32 TAP_READ_TIMEOUT = 500U;                                                // Timeout for the call to WaitForSingleObject() on the TAP handle
        // First 4 bytes of the MAC Address of the virtual TAP interface
        static const uint8 VIRT_MAC_ADDR_BYTE1 = 0x02;
        static const uint8 VIRT_MAC_ADDR_BYTE2 = 0x0A;
        static const uint8 VIRT_MAC_ADDR_BYTE3 = 0x0C;
        static const uint8 VIRT_MAC_ADDR_BYTE4 = 0x00;

        // Network interface configuration values (for gateway mode)
        static uint16 ETHERNET_MTU_INTERNAL_IF;                                                     // MFS (in bytes) for frames written to/read from the internal ethernet network interface
        static uint16 ETHERNET_MTU_EXTERNAL_IF;                                                     // MFS (in bytes) for frames written to/read from the external ethernet network interface
        static const uint16 ETHERNET_DEFAULT_MTU = 1500U;                                           // Default MTU (in bytes) for packets transmitted within ethernet network frames
        static const uint16 ETHERNET_DEFAULT_MFS = 1538U;                                           // Default MFS (in bytes) for frames written to/read from the ethernet network interface
        static const uint16 ETHERNET_MAXIMUM_MTU = 9000U;                                           // Max allowed MTU (in bytes) for frames written to/read from the ethernet network interface
        static const uint16 ETHERNET_MAXIMUM_MFS = 9038U;                                           // Max allowed MFS (in bytes) for frames written to/read from the ethernet network interface
        static const char * const pszNetworkAdapterNamePrefix;

        static const uint8 WRITE_PACKET_BUFFERS = 3U;                                              // Number of buffers available to write on the TUN/TAP (or libpcap internal) interface

        // Port and timeouts for Mockets, TCP and UDP connections, and for the GUI
        static uint16 MOCKET_SERVER_PORT;
        static uint32 MOCKET_TIMEOUT;
        static uint16 TCP_SERVER_PORT;
        static uint16 UDP_SERVER_PORT;
        static uint16 CSR_MOCKET_SERVER_PORT;
        static const uint16 CSR_PROXY_SERVER_PORT;
        static uint16 GUI_LOCAL_PORT;
        static const char * const CSR_PROXY_SERVER_ADDR;
        static const uint16 DEFAULT_MOCKET_SERVER_PORT = 8751;
        static const uint16 DEFAULT_TCP_SERVER_PORT = 8751;
        static const uint16 DEFAULT_UDP_SERVER_PORT = 8752;
        static const uint16 DEFAULT_GUI_LOCAL_PORT = 8755;
        static const uint32 DEFAULT_MOCKET_TIMEOUT = 30000;
        static const uint32 DEFAULT_SOCKET_TIMEOUT = 30000;

        static const bool DEFAULT_LOCAL_PROXY_REACHABILITY_FROM_REMOTE = true;

        static const uint16 PROXY_MESSAGE_MTU = 8192U + sizeof (TCPDataProxyMessage);               // MTU (in bytes) for packets addressed to remote proxies
        static const uint16 TCP_WINDOW_SIZE = 65535U;                                               // Window size for incoming TCP connections (in bytes)
        static const uint32 MAX_UDP_CONNECTION_BUFFER_SIZE = 262144U;                               // Maximum allowed size of the buffer which stores outgoing data in the UDP connection thread
        static const uint32 MAX_UDP_NAGLE_ALGORITHM_TIMEOUT = 60000U;                               // Maximum allowed value for the timeout of the UDP Nagle's Algorithm
        static const uint32 IDLE_TCP_CONNECTION_NOTIFICATION_TIME = 5000U;                          // Time after which a TCP connection is considered idle (amount of time without any actual activity)
        static const uint32 IDLE_TCP_FAST_RETRANSMIT_TRIGGER_TIMEOUT = 1000U;                       // Time after which a TCP connection is considered idle (amount of time without any actual activity)
        // Variables to handle TCP SYN_SENT retransmissions and connection attempt timeout
        static const uint32 SYN_SENT_RETRANSMISSION_TIMEOUTS_ENTRIES = 6;
        static const int64 SYN_SENT_RETRANSMISSION_TIMEOUTS[SYN_SENT_RETRANSMISSION_TIMEOUTS_ENTRIES];
        static const uint32 SYN_SENT_FAILED_TIMEOUT = 60000U;
        // Time after which an attempt to instantiate a new remote connection is considered failed
        static const uint32 DEFAULT_REMOTE_CONN_ESTABLISHMENT_TIMEOUT = (DEFAULT_MOCKET_TIMEOUT >= DEFAULT_SOCKET_TIMEOUT) ? DEFAULT_MOCKET_TIMEOUT : DEFAULT_SOCKET_TIMEOUT;
        static const double UPDATE_TCP_WINDOW_ACK_THRESHOLD;                                        // Threshold above which a TCP window update notification is sent to the local application

        static const uint32 MIN_ENTRY_BUF_SIZE = 8192U;
        static const uint32 MAX_ENTRY_BUF_SIZE = 32768U;
        static const uint8 ENTRIES_POOL_SIZE = 10U;

        static bool UPDATE_GUI_THREAD_ENABLED;                                                      // Bool that states whether to start the UpdateGUIThread or not
        static const bool DEFAULT_UPDATE_GUI_THREAD_ENABLED = true;                                 // Default behavior is to enable UpdateGUIThread
        static const int64 DEFAULT_UDP_CONNECTOR_INACTIVITY_TIME = 30000;                           // Time above which the statistics for the UDP Connector are not included in the GUI Update Message

        static const char * const DEFAULT_GUI_UPDATE_MESSAGE_HEADER;                                // Standard Mockets config file name
        static NOMADSUtil::String DEFAULT_MOCKETS_CONFIG_FILE;                                      // Default Mockets config file name when no default Mockets config file is specified in netProxy.cfg
        static const char * const LOGS_DIR;                                                         // Name of the directory where log files are stored
    };

}

#endif      // INCL_CONFIGURATION_PARAMETERS_H
