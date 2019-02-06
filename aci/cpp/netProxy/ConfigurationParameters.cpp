/*
 * ConfigurationParameters.cpp
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

#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    // NetworkConfigurationSettings
    int64 NetworkConfigurationSettings::ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS = NetworkConfigurationSettings::DEFAULT_ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS;
    uint16 NetworkConfigurationSettings::MAX_TCP_UNACKED_DATA = NetworkConfigurationSettings::DEFAULT_MAX_TCP_UNACKED_DATA;
    uint16 NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = NetworkConfigurationSettings::DEFAULT_MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE;
    double NetworkConfigurationSettings::UPDATE_TCP_WINDOW_ACK_THRESHOLD = NetworkConfigurationSettings::DEFAULT_UPDATE_TCP_WINDOW_ACK_THRESHOLD;
    bool NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE = NetworkConfigurationSettings::DEFAULT_SYNCHRONIZE_TCP_HANDSHAKE;
    bool NetworkConfigurationSettings::TCP_TIME_WAIT_IGNORE_STATE = NetworkConfigurationSettings::DEFAULT_TCP_TIME_WAIT_IGNORE_STATE;
    uint32 NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT = NetworkConfigurationSettings::DEFAULT_UDP_NAGLE_ALGORITHM_TIMEOUT;
    uint16 NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD = NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;
    uint32 NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS = NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS;
    uint32 NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_BUFFER_SIZE;

    uint32 NetworkConfigurationSettings::VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT = NetworkConfigurationSettings::DEFAULT_VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT;


    // NetProxyApplicationParameters
    uint32 NetProxyApplicationParameters::NETPROXY_UNIQUE_ID = 0U;
    bool NetProxyApplicationParameters::GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_GATEWAY_MODE;
    bool NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_TRANSPARENT_GATEWAY_MODE;
    bool NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE = NetProxyApplicationParameters::DEFAULT_LEVEL2_TUNNEL_MODE;
    uint64 NetProxyApplicationParameters::UI64_STARTUP_TIME_IN_MILLISECONDS = 0U;

    bool NetProxyApplicationParameters::ENABLE_PRIORITIZATION_MECHANISM = NetProxyApplicationParameters::DEFAULT_ENABLE_PRIORITIZATION_MECHANISM;

    bool NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES = NetProxyApplicationParameters::DEFAULT_GENERATE_STATISTICS_UPDATES;
    int64 NetProxyApplicationParameters::I64_PROCESS_MEASURE_INTERVAL_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_PROCESS_MEASURE_INTERVAL_IN_MS;
    int64 NetProxyApplicationParameters::I64_ADDRESS_MEASURE_INTERVAL_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_ADDRESS_MEASURE_INTERVAL_IN_MS;
    int64 NetProxyApplicationParameters::I64_PROTOCOL_MEASURE_INTERVAL_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_PROTOCOL_MEASURE_INTERVAL_IN_MS;
    int64 NetProxyApplicationParameters::I64_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS;
    int64 NetProxyApplicationParameters::I64_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS;
    int64 NetProxyApplicationParameters::I64_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS;
    int64 NetProxyApplicationParameters::I64_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS;
    int64 NetProxyApplicationParameters::I64_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS;
    int64 NetProxyApplicationParameters::I64_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS =
        NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS;

    bool NetProxyApplicationParameters::ACTIVATE_NETSENSOR = NetProxyApplicationParameters::DEFAULT_ACTIVATE_NETSENSOR;
    std::string NetProxyApplicationParameters::NETSENSOR_STATISTICS_IP_ADDRESS = NetProxyApplicationParameters::DEFAULT_NETSENSOR_STATISTICS_IP_ADDRESS;

    std::vector<NetworkInterfaceDescriptor> NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST;
    NetworkInterfaceDescriptor NetProxyApplicationParameters::NID_INTERNAL_INTERFACE;

    uint16 NetProxyApplicationParameters::MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
    uint16 NetProxyApplicationParameters::TCP_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT;
    uint16 NetProxyApplicationParameters::UDP_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT;
    uint16 NetProxyApplicationParameters::CSR_MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
    const char * const NetProxyApplicationParameters::CSR_PROXY_SERVER_ADDR = NetProxyApplicationParameters::DEFAULT_CSR_PROXY_SERVER_ADDR;
    const uint16 NetProxyApplicationParameters::CSR_PROXY_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_CSR_PROXY_SERVER_PORT;
    uint32 NetProxyApplicationParameters::MOCKET_TIMEOUT = NetProxyApplicationParameters::DEFAULT_MOCKET_TIMEOUT;

    bool NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED = NetProxyApplicationParameters::DEFAULT_MOCKETS_DTLS_ENABLED;
    #if defined (WIN32)
        std::string NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE {"c:\\temp\\mockets.conf"};
    #else
        std::string NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE {"/tmp/mockets.conf"};
    #endif
    std::string NetProxyApplicationParameters::LOGS_DIR = "log";
}