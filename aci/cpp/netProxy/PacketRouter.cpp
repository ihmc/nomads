/*
* PacketRouter.cpp
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

#if defined (WIN32)
#define NOMINMAX    // For interference with std::min and std::max
#endif

#include <ctime>
#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "FTypes.h"
#include "SequentialArithmetic.h"
#include "net/NetUtils.h"
#include "InetAddr.h"
#include "NetSensor.h"
#include "Logger.h"

#include "PacketRouter.h"
#include "MutexCounter.h"
#include "MutexUDPQueue.h"
#include "ARPCache.h"
#include "ARPTableMissCache.h"
#include "UDPDatagramPacket.h"
#include "QueryResult.h"
#include "PacketBufferManager.h"
#include "ConnectorWriter.h"
#include "TapInterface.h"
#include "PCapInterface.h"
#include "PacketReceiver.h"
#include "Entry.h"
#include "TCPConnTable.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "AutoConnectionManager.h"
#include "Connector.h"
#include "TCPManager.h"
#include "ConfigurationManager.h"
#include "StatisticsManager.h"
#include "Utilities.h"


#if defined (USE_DISSERVICE)
using namespace IHMC_ACI;
#endif

#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    int PacketRouter::init (const std::string & sHomeDir, const std::string & sConfigFilePath)
    {
        int rc;
        NetProxyApplicationParameters::UI64_STARTUP_TIME_IN_MILLISECONDS = NOMADSUtil::getTimeInMilliseconds();

        // Read the config file
        if (0 != (rc = _configurationManager.init (sHomeDir, sConfigFilePath))) {
            checkAndLogMsg ("main", NOMADSUtil::Logger::L_MildError,
                            "failed to call init() on ConfigurationManager with config file path <%s>; rc = %d\n",
                            (const char*) sConfigFilePath.c_str(), rc);
            return -1;
        }

        // Process the config file
        if (0 != (rc = _configurationManager.processConfigFiles())) {
            checkAndLogMsg ("main", NOMADSUtil::Logger::L_SevereError,
                            "config files processing failed with rc = %d\n", rc);
            return -2;
        }

        if (0 != (rc = setupNetworkInterfaces())) {
            checkAndLogMsg ("PacketRouter::init", NOMADSUtil::Logger::L_SevereError,
                            "Error initializing network intefaces; setupNetworkInterfaces() returned rc = %d\n", rc);
            return -3;
        }

        // Check if the NPUID was set and, if not, set it to the 32-bits value of the IP address of the external/TAP interface
        if (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID == 0U) {
            NetProxyApplicationParameters::NETPROXY_UNIQUE_ID = NetProxyApplicationParameters::GATEWAY_MODE ?
                NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST[0].ui32IPv4Address :
                NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address;
            checkAndLogMsg ("PacketRouter::init", NOMADSUtil::Logger::L_Info,
                            "no UniqueID was specified in the config file; using the IP address of the %s interface as UniqueID (%u)\n",
                            NetProxyApplicationParameters::GATEWAY_MODE ? "main external" : "TAP",
                            NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        }

        // If running in Host Mode, remove all references to NetworkInterface instances that refer to external interfaces
        if (!NetProxyApplicationParameters::GATEWAY_MODE) {
            _umExternalInterfaces.clear();
        }
        else {
            // Update vectors of network interfaces on which to forward broadcast and multicast packets
            for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
                updateMulticastBroadcastPacketForwardingRulesForNID (nid);
            }
            updateMulticastBroadcastPacketForwardingRulesForNID (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE);
        }

        if (NetProxyApplicationParameters::ACTIVATE_NETSENSOR && NetProxyApplicationParameters::GATEWAY_MODE &&
            !NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE) {
            if (0 != (rc = setupNetSensor())) {
                checkAndLogMsg ("PacketRouter::init", NOMADSUtil::Logger::L_SevereError,
                                "Error initializing NetSensor; setupNetSensor() returned rc = %d\n", rc);
                return -4;
            }
        }


    #if defined (USE_DISSERVICE)
        // Initialize DisService
        //_pDisService = new DisseminationService {_pConfigurationManager};  /*!!*/ // Need to fix
        if (0 != (rc = _pDisService->init())) {
            checkAndLogMsg ("PacketRouter::init", NOMADSUtil::Logger::L_MildError,
                            "failed to initialize DisseminationService; rc = %d\n", rc);
            return -3;
        }
        _pDisService->registerDisseminationServiceListener (0, this);
        _pDisService->subscribe (0, "netproxy.reliable", 0, true, true, true);
        _pDisService->subscribe (0, "netproxy.unreliable", 0, false, true, true);
    #endif

        addMACToIntHostsSet (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress);
        for (const auto & nidExternalInterface : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            addMACToExtHostsSet (nidExternalInterface.emaInterfaceMACAddress);
        }

        // Interface-specific connectors
        NOMADSUtil::UInt32Hashset * const hsEnabledConnectors = _configurationManager.getEnabledConnectorsSet();
        for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            for (NOMADSUtil::UInt32Hashset::Iterator iter{hsEnabledConnectors}; !iter.end(); iter.nextElement()) {
                ConnectorType ct = static_cast<ConnectorType> (iter.getKey());
                const uint16 ui16AcceptServerPort = Connector::getAcceptServerPortForConnector (ct);
                std::shared_ptr<Connector> spConnector{Connector::connectorFactoryMethod (ct, _connectionManager, _TCPConnTable,
                                                                                          _TCPManager, *this, _statisticsManager)};
                if (0 != (rc = spConnector->init (ui16AcceptServerPort, nid.ui32IPv4Address))) {
                    checkAndLogMsg ("PacketRouter::init", NOMADSUtil::Logger::L_MildError,
                                    "failed to initialize the %sConnector bound to address %s and port %hu; rc = %d\n",
                                    spConnector->getConnectorTypeAsString(), NOMADSUtil::InetAddr{nid.ui32IPv4Address}.getIPAsString(),
                                    ui16AcceptServerPort, rc);
                    return -5;
                }

                checkAndLogMsg ("PacketRouter::init", NOMADSUtil::Logger::L_Info,
                                "successfully created and initialized a %sConnector listening on address <%s:%hu>\n",
                                spConnector->getConnectorTypeAsString(), NOMADSUtil::InetAddr{nid.ui32IPv4Address}.getIPAsString(),
                                ui16AcceptServerPort);
                // The following instruction also updates all entries in the AutoConnection Table that make use of the specified connector
                _connectionManager.registerConnector (std::move (spConnector));
            }
        }

        _statisticsManager.init();
        _statisticsManager.addNode (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);

        _bInitSuccessful = true;
        return 0;
    }

    int PacketRouter::startThreads (void)
    {
        Connector *pConnector = nullptr;
        NOMADSUtil::UInt32Hashset * const hsEnabledConnectors = _configurationManager.getEnabledConnectorsSet();
        for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            for (NOMADSUtil::UInt32Hashset::Iterator iter{hsEnabledConnectors}; !iter.end(); iter.nextElement()) {
                ConnectorType ct = static_cast<ConnectorType> (iter.getKey());
                pConnector = _connectionManager.getConnectorBoundToAddressForType (nid.ui32IPv4Address, ct);
                if (!pConnector) {
                    checkAndLogMsg ("PacketRouter::startThreads", NOMADSUtil::Logger::L_Warning,
                                    "found null pointer in the ConnectorSet for type %s; ignoring connector\n",
                                    connectorTypeToString (ct));
                    continue;
                }

                NOMADSUtil::ManageableThread *pMT = dynamic_cast<NOMADSUtil::ManageableThread *> (pConnector);
                if (pMT != nullptr) {
                    std::string sThreadName{pConnector->getConnectorTypeAsString()};
                    sThreadName += "Connector Thread";
                    pMT->setName (sThreadName.c_str());
                    pMT->start();
                    checkAndLogMsg ("PacketRouter::startThreads", NOMADSUtil::Logger::L_Info,
                                    "%sConnector started\n", pConnector->getConnectorTypeAsString());
                }
                else {
                    checkAndLogMsg ("PacketRouter::startThreads", NOMADSUtil::Logger::L_SevereError,
                                    "unable to start connectorThread for connector of type %s %s\n",
                                    pConnector->getConnectorTypeAsString());
                    return -1;
                }
            }
        }

        auto fInternalInterfacePacketHandler =
            [this] (uint8 * const ui8Packet, uint16 ui16PacketLen, NetworkInterface * const pNI) {
                return handlePacketFromInternalInterface (ui8Packet, ui16PacketLen, pNI);
        };
        _internalPacketReceiverThread = PacketReceiver{"Internal PacketReceiver Thread", PacketRouter::_spInternalInterface,
                                                       fInternalInterfacePacketHandler};
        if (0 != _internalPacketReceiverThread.start()) {
            return -2;
        }

        if (NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE) {
            // No other threads are required
            return 0;
        }

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            _vExternalPacketReceiverThreads.reserve (NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.size());
            for (const auto & nidExternalInterface : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
                auto fExternalInterfacePacketHandler =
                    [this] (uint8 * const ui8Packet, uint16 ui16PacketLen, NetworkInterface * const pNI) {
                        return handlePacketFromExternalInterface (ui8Packet, ui16PacketLen, pNI);
                };
                _vExternalPacketReceiverThreads.emplace_back ("External PacketReceiver Thread (" + nidExternalInterface.sInterfaceName + ")",
                                                              PacketRouter::getExternalNetworkInterfaceWithIP (nidExternalInterface.ui32IPv4Address),
                                                              fExternalInterfacePacketHandler);
                if (0 != _vExternalPacketReceiverThreads.back().start()) {
                    return -3;
                }
            }
        }

        if (0 != _localUDPDatagramsManagerThread.start (false)) {
            return -5;
        }
        _localUDPDatagramsManagerThread.setName ("LocalUDPDatagramsManager Thread");

        if (0 != _localTCPTransmitterThread.start (false)) {
            return -6;
        }
        _localTCPTransmitterThread.setName ("LocalTCPTransmitter Thread");

        if (0 != _remoteTCPTransmitterThread.start (false)) {
            return -7;
        }
        _remoteTCPTransmitterThread.setName ("RemoteTCPTransmitter Thread");

        if (_connectionManager.getNumberOfValidAutoConnectionEntries() > 0) {
            if (0 != _autoConnectionManagerThread.start (false)) {
                return -8;
            }
            _autoConnectionManagerThread.setName ("AutoConnectionManager Thread");
        }

        if (0 != _memoryCleanerManagerThread.start (false)) {
            return -9;
        }
        _memoryCleanerManagerThread.setName ("MemoryCleanerManager Thread");

        if (NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES) {
            _statisticsUpdateManagerThread.init (_configurationManager.getValue ("StatusNotificationAddresses"));
            if (0 != _statisticsUpdateManagerThread.start (false)) {
                return -10;
            }
            _statisticsUpdateManagerThread.setName ("StatisticsUpdateManager Thread");
        }

        while (!_localUDPDatagramsManagerThread.isRunning()) {
            // Wait for the UDPDatagramsManager thread to actually start the execution
            NOMADSUtil::sleepForMilliseconds (10);
        }

        return 0;
    }

    int PacketRouter::joinThreads (void)
    {
        int rc;
        if (0 != _internalPacketReceiverThread.join()) {
            return -1;
        }

        if (NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE) {
            // All active threads have been joined
            return 0;
        }

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            for (auto & prExternalInterfaceThread : _vExternalPacketReceiverThreads) {
                if (0 != (rc = prExternalInterfaceThread.join())) {
                    checkAndLogMsg ("PacketRouter::joinThreads", NOMADSUtil::Logger::L_Warning,
                                    "error while trying to join() externalPacketReceiverThread with name %s; "
                                    "rc = %d\n", prExternalInterfaceThread.getThreadName().c_str(), rc);
                    return -2;
                }
            }
        }

        if (0 != _localUDPDatagramsManagerThread.join()) {
            return -3;
        }
        if (0 != _localTCPTransmitterThread.join()) {
            return -4;
        }
        if (0 != _remoteTCPTransmitterThread.join()) {
            return -5;
        }
        if (0 != _memoryCleanerManagerThread.join()) {
            return -6;
        }
        _autoConnectionManagerThread.join();
        _statisticsUpdateManagerThread.join();

        int iNumOpenConn = 0;
        while ((iNumOpenConn = _connectionManager.getNumberOfOpenConnections()) > 0) {
            checkAndLogMsg ("PacketRouter::joinThreads", NOMADSUtil::Logger::L_LowDetailDebug,
                            "at least %d connections are still open; waiting 500ms...\n", iNumOpenConn);
            NOMADSUtil::sleepForMilliseconds (500);
        }

        while (_connectionManager.getNumberOfConnectors() > 0) {
            checkAndLogMsg ("PacketRouter::joinThreads", NOMADSUtil::Logger::L_LowDetailDebug,
                            "Connectors are still closing; waiting...\n");
            NOMADSUtil::sleepForMilliseconds (200);
        }

        return 0;
    }

    void PacketRouter::requestTermination (void)
    {
        if (PacketRouter::isTerminationRequested()) {
            return;
        }
        _bTerminationRequested = true;

        // Terminate the internal receiver thread
        _internalPacketReceiverThread.requestTermination();
        if (_spInternalInterface) {
            _spInternalInterface->requestTermination();
        }

        // Terminate all external receiver threads
        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            for (auto & prExternalInterfaceThread : _vExternalPacketReceiverThreads) {
                prExternalInterfaceThread.requestTermination();
            }
        }
        for (auto pui32spni : _umExternalInterfaces) {
            pui32spni.second->requestTermination();
        }

        // Terminate and notify all other threads
        _autoConnectionManagerThread.requestTermination();
        _autoConnectionManagerThread.notify();
        _localUDPDatagramsManagerThread.requestTermination();
        _localUDPDatagramsManagerThread.notify();
        _localTCPTransmitterThread.requestTermination();
        _localTCPTransmitterThread.notify();
        _remoteTCPTransmitterThread.requestTermination();
        _remoteTCPTransmitterThread.notify();
        _memoryCleanerManagerThread.requestTermination();
        _memoryCleanerManagerThread.notify();
        _statisticsUpdateManagerThread.requestTermination();
        _statisticsUpdateManagerThread.notify();

        // Terminate NetSensor, if running
        if (_upNetSensor) {
            _upNetSensor->requestTermination();
        }

        // Clear all connection-related caches, tables, and lists from memory
        _connectionManager.clearAutoConnectionTable();
        _connectionManager.clearAllConnectionMappings();

        // Clear all entries in the TCP Connections Table
        _TCPConnTable.clearTable();

        // Terminate all Connectors and Connections
        if (_bInitSuccessful) {
            NOMADSUtil::UInt32Hashset * const hsEnabledConnectors = _configurationManager.getEnabledConnectorsSet();
            for (NOMADSUtil::UInt32Hashset::Iterator iter{hsEnabledConnectors}; !iter.end(); iter.nextElement()) {
                for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
                    auto * const pBoundConnector = _connectionManager.getConnectorBoundToAddressForType (nid.ui32IPv4Address,
                                                                                                         static_cast<ConnectorType> (iter.getKey()));
                    if (pBoundConnector) {
                        pBoundConnector->terminateExecution();
                    }
                }
            }
            Connection::closeAllConnections (true);
        }
    }

    std::shared_ptr<NetworkInterface> PacketRouter::selectMainExternalInterfaceInTheSameNetwork (uint32 ui32IPv4DestinationAddress)
    {
        NetworkInterfaceDescriptor nidNetmaskMatchingInterface{},
            nidDefaultGatewayInterface = NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST[0];
        for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            if ((nidDefaultGatewayInterface.ui32IPv4GatewayAddress == 0) && (nid.ui32IPv4GatewayAddress != 0)) {
                nidDefaultGatewayInterface = nid;
            }
            if ((nid.ui32IPv4Address & nid.ui32IPv4NetMask) == (ui32IPv4DestinationAddress & nid.ui32IPv4NetMask)) {
                // Look for the longest matching prefix
                if (nid.ui32IPv4NetMask > nidNetmaskMatchingInterface.ui32IPv4NetMask) {
                    nidNetmaskMatchingInterface = nid;
                }
            }
        }

        return nidNetmaskMatchingInterface.ui32IPv4Address != 0 ? _umExternalInterfaces.at (nidNetmaskMatchingInterface.ui32IPv4Address) :
            _umExternalInterfaces.at (nidDefaultGatewayInterface.ui32IPv4Address);
    }

    void PacketRouter::updateMulticastBroadcastPacketForwardingRulesForNID (const NetworkInterfaceDescriptor & nid)
    {
        std::transform (nid.usMulticastForwardingInterfacesList.cbegin(), nid.usMulticastForwardingInterfacesList.cend(),
                        std::inserter (_umInterfacesMulticastPacketsForwardingRules[nid.sInterfaceName],
                                       _umInterfacesMulticastPacketsForwardingRules[nid.sInterfaceName].begin()),
                        [this] (const std::string & sInterfaceName)
        {
            const auto & nid = findNIDWithName (sInterfaceName);
            assert (nid != NetProxyApplicationParameters::NID_INVALID);

            if (_umExternalInterfaces.count (nid.ui32IPv4Address) == 1) {
                return _umExternalInterfaces.at (nid.ui32IPv4Address);
            }
            else if (_spInternalInterface->getIPv4Addr()->ui32Addr == nid.ui32IPv4Address) {
                return _spInternalInterface;
            }

            throw std::invalid_argument{"Could not find interface with the specified IPv4 address"};
        });

        std::transform (nid.usBroadcastForwardingInterfacesList.cbegin(), nid.usBroadcastForwardingInterfacesList.cend(),
                        std::inserter (_umInterfacesMulticastPacketsForwardingRules[nid.sInterfaceName],
                                       _umInterfacesMulticastPacketsForwardingRules[nid.sInterfaceName].begin()),
                        [this] (const std::string & sInterfaceName)
        {
            const auto & nid = findNIDWithName (sInterfaceName);
            assert (nid != NetProxyApplicationParameters::NID_INVALID);

            if (_umExternalInterfaces.count (nid.ui32IPv4Address) == 1) {
                return _umExternalInterfaces.at (nid.ui32IPv4Address);
            }
            else if (_spInternalInterface->getIPv4Addr()->ui32Addr == nid.ui32IPv4Address) {
                return _spInternalInterface;
            }

            throw std::invalid_argument{"Could not find interface with the specified IPv4 address"};
        });
    }

    void PacketRouter::wakeUpAutoConnectionAndRemoteTransmitterThreads (void)
    {
        {
            if (_connectionManager.getNumberOfValidAutoConnectionEntries() > 0) {
                _autoConnectionManagerThread.notify();
                checkAndLogMsg ("PacketRouter::wakeUpAutoConnectionAndRemoteTransmitterThreads", NOMADSUtil::Logger::L_HighDetailDebug,
                                "AutoConnectionManager thread notified\n");
            }
        }

        uint16 ui16ActiveConnectionsNum = 0;
        {
            std::lock_guard<std::mutex> lgTCPConnTable{_TCPConnTable.getMutexRef()};
            ui16ActiveConnectionsNum = _TCPConnTable.getActiveLocalConnectionsCount();
        }
        if (ui16ActiveConnectionsNum > 0) {
            _remoteTCPTransmitterThread.wakeUpThread();
            checkAndLogMsg ("PacketRouter::wakeUpAutoConnectionAndRemoteTransmitterThreads", NOMADSUtil::Logger::L_HighDetailDebug,
                            "RemoteTCPTransmitter thread notified; there are %hu active TCP connections\n",
                            ui16ActiveConnectionsNum);
        }
    }

    int PacketRouter::handlePacketFromInternalInterface (uint8 * const pPacket, uint16 ui16PacketLen,
                                                         NetworkInterface * const pReceivingNetworkInterface)
    {
        static int rc = 0;
        NOMADSUtil::EtherFrameHeader * const pEthHeader = reinterpret_cast<NOMADSUtil::EtherFrameHeader *> (pPacket);
        ntoh (pEthHeader);

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            if (pEthHeader->src == NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress &&
                !NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE) {
                // Packet generated by the internal interface
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "ignoring packet generated by this host on the internal interface\n");
                return 0;
            }

            if (hostBelongsToTheExternalNetwork (pEthHeader->src)) {
                // Packet originated in the external network and reinjected on the internal one --> ignore it
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "ignoring ethernet packet originated on the external network and "
                                "previously forwarded on the internal one by the NetProxy\n");
                return 0;
            }

            if (!hostBelongsToTheInternalNetwork (pEthHeader->src)) {
                // Add new host to the list of hosts in the internal network
                addMACToIntHostsSet (pEthHeader->src);
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "added MAC address %02X:%02X:%02X:%02X:%02X:%02X to the set of hosts in the internal network\n",
                                pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3,
                                pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6);
            }
        }

        uint16 ui16EtherType = getEtherTypeFromEthernetFrame (pEthHeader);
        if (NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE) {
            // Tunnel Mode
            uint32 ui32LocalHostIPAddress = 0, ui32RemoteHostIPAddress = 0;
            if (ui16EtherType == NOMADSUtil::ET_ARP) {
                register NOMADSUtil::ARPPacket * pARPPacket = reinterpret_cast<NOMADSUtil::ARPPacket *> (getPacketWithinEthernetFrame (pEthHeader));
                ui32LocalHostIPAddress = pARPPacket->spa.ui32Addr;          // bytes in network order
                ui32RemoteHostIPAddress = pARPPacket->tpa.ui32Addr;         // bytes in network order
            }
            else if (ui16EtherType == NOMADSUtil::ET_IP) {
                register NOMADSUtil::IPHeader * pIPHeader = reinterpret_cast <NOMADSUtil::IPHeader *> (getPacketWithinEthernetFrame (pEthHeader));
                ui32LocalHostIPAddress = pIPHeader->srcAddr.ui32Addr;       // bytes in network order
                ui32RemoteHostIPAddress = pIPHeader->destAddr.ui32Addr;     // bytes in network order
            }
            else {
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                "tunneling a non-ARP, non-IPv4 packet: EtherType is %hu\n", ui16EtherType);
            }

            if ((ui32RemoteHostIPAddress == 0) || _connectionManager.isFlowPotentiallyMapped (ui32LocalHostIPAddress, ui32RemoteHostIPAddress)) {
                // Forward the Ethernet packet over the tunnel
                hton (pEthHeader);
                if (0 != sendPacketOverTheTunnel (pPacket, ui16PacketLen, ui32LocalHostIPAddress, ui32RemoteHostIPAddress)) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                    "sendPacketOverTheTunnel() of an Ethernet packet with remapped source-destination address "
                                    "pair <%s-%s> and %hu bytes long failed with rc = %d; NetProxy will discard the packet\n",
                                    NOMADSUtil::InetAddr{ui32LocalHostIPAddress}.getIPAsString(),
                                    NOMADSUtil::InetAddr{ui32RemoteHostIPAddress}.getIPAsString(), ui16PacketLen, rc);
                    return -1;
                }
            }

            // Running in Tunnel Mode --> NetProxy will not process the packet
            return 0;
        }

        // Proxy Mode or unknown EtherType
        if (ui16EtherType == NOMADSUtil::ET_ARP) {
            register NOMADSUtil::ARPPacket * pARPPacket = reinterpret_cast<NOMADSUtil::ARPPacket *> (getPacketWithinEthernetFrame (pEthHeader));
            pARPPacket->ntoh();
            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                            "ARP Packet: operation %hu - SPA %s - TPA %s\n", pARPPacket->ui16Oper,
                            NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)}.getIPAsString(),
                            NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)}.getIPAsString());

            if (pARPPacket->spa.ui32Addr != 0) {
                if (pARPPacket->sha == NOMADSUtil::EtherMACAddr{0}) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "received an ill-formed ARP packet with operation %hu, SHA set to zero, SPA set to %s, "
                                    "THA set to %s, and TPA set to %s on the internal network; ignoring it\n", pARPPacket->ui16Oper,
                                    NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)}.getIPAsString(),
                                    etherMACAddrToString (pARPPacket->tha).c_str(),
                                    NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)}.getIPAsString());
                    return 0;
                }

                // This is not an ARP probe --> process it
                if (isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)) ||
                    isMACAddressAssignedToExternalInterface (pARPPacket->sha)) {
                    // Packets with SHA or SPA of the external interface should not appear in the internal network --> ignoring the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "an ARP packet operation %hu (THA set to %s, TPA set to %s) with SHA set to %s and SPA set to %s "
                                    "that match the MAC and/or the IP address of the external (!!!) interface detected in the internal "
                                    "network; ignoring the packet\n", pARPPacket->ui16Oper, etherMACAddrToString (pARPPacket->tha).c_str(),
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString(),
                                    etherMACAddrToString (pARPPacket->sha).c_str(),
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                    return 0;
                }

                if (!isIPv4AddressAssignedToInternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)) &&
                    !isMACAddressAssignedToInternalInterface (pARPPacket->sha)) {
                    // The sender is not this machine --> opportunistically cache the MAC address of the sender and continue processing
                    _ARPCache.insert (pARPPacket->spa.ui32Addr, pARPPacket->sha);
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                    "cached MAC address %02X:%02X:%02X:%02X:%02X:%02X for node with IPv4 address %s in the internal network\n",
                                    pARPPacket->sha.ui8Byte1, pARPPacket->sha.ui8Byte2, pARPPacket->sha.ui8Byte3,
                                    pARPPacket->sha.ui8Byte4, pARPPacket->sha.ui8Byte5, pARPPacket->sha.ui8Byte6,
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());

                    // Send any cached packets addressed to the IP for which the corresponding MAC address was just received
                    int iSentPackets = sendCachedPacketsToDestination (pARPPacket->spa.ui32Addr);
                    if (iSentPackets > 0) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "successfully sent %d cached packets to host with IP address %s\n", iSentPackets,
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                    }
                }
                else {
                    // Packet generated by the internal interface of this machine --> ignore it
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "received an ARP packet with operation %hu and SPA set to %s that was "
                                    "generated by this machine; NetProxy will ignore it\n", pARPPacket->ui16Oper,
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                    return 0;
                }
            }
            else if (NetProxyApplicationParameters::GATEWAY_MODE) {
                // Received an ARP Probe (spa == 0.0.0.0) while running in Gateway Mode
                if (!isIPv4AddressAssignedToInternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)) &&
                    !isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    // The TPA is not one of this machine's internal interfaces --> forward ARP probe onto all external networks to which the TPA belongs
                    std::vector<std::shared_ptr<NetworkInterface>> vspExternalInterfaceList;
                    vspExternalInterfaceList.reserve (_umExternalInterfaces.size() - 1);
                    MapToVec_if (_umExternalInterfaces, vspExternalInterfaceList,
                                 [pReceivingNetworkInterface, pARPPacket] (const std::pair<decltype(_umExternalInterfaces)::key_type,
                                                                           decltype(_umExternalInterfaces)::mapped_type> & pui32sp)
                    {
                        return NOMADSUtil::NetUtils::areInSameNetwork (pui32sp.second->getIPv4Addr()->ui32Addr, pui32sp.second->getNetmask()->ui32Addr,
                                                                       pARPPacket->tpa.ui32Addr, pReceivingNetworkInterface->getNetmask()->ui32Addr);
                    });
                    std::vector<NetworkInterface *> vExternalInterfaceList;
                    vExternalInterfaceList.reserve (vspExternalInterfaceList.size());
                    std::transform (vspExternalInterfaceList.cbegin(), vspExternalInterfaceList.cend(), std::back_inserter (vExternalInterfaceList),
                                    [] (const std::shared_ptr<NetworkInterface> & spNI) { return spNI.get(); });

                    if (vExternalInterfaceList.size() > 0) {
                        pARPPacket->hton();
                        hton (pEthHeader);
                        if (0 != (rc = sendPacketToHost (vExternalInterfaceList, pPacket, ui16PacketLen))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                            "sendPacketToHost() of an ARP Probe packet (Source Protocol Address: 0.0.0.0 - "
                                            "Target Protocol Address: %s) of %hu bytes long failed with rc = %d\n",
                                            NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                            return -2;
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "successfully forwarded an ARP Probe packet (Source Protocol Address: 0.0.0.0 - Target Protocol "
                                        "Address: %s) of %hu bytes long onto all external networks to which the TPA belongs\n",
                                        NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "an ARP Probe packet (Source Protocol Address: 0.0.0.0 - Target Protocol Address: %s) of %hu "
                                        "bytes long was not forwarded because the TPA did not belong to any of the external networks\n",
                                        NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
                    }
                }
                else if (isIPv4AddressAssignedToInternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    // Another host is trying to use the IP of the internal interface! --> reply with a gratuitous ARP announcement
                    if (0 != (rc = sendARPAnnouncement (_spInternalInterface.get(), pARPPacket,
                                                        NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address,
                                                        NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                        "sendARPAnnouncement() on the internal network interface failed with rc = %d\n", rc);
                        return -3;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "successfully sent a gratuitous ARP Announcement on the internal network in response to "
                                    "an ARP Probe that had the IP of the internal interface as the Target Protocol Address\n");
                }
                else {
                    // Another host is trying to use the IP address of the external interface! --> reply with a gratuitous ARP announcement on both interfaces
                    if (0 != (rc = sendARPAnnouncement (_spInternalInterface.get(), pARPPacket, NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr),
                                                        NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                        "sendARPAnnouncement() on the internal network interface failed with rc = %d\n", rc);
                        return -4;
                    }
                    if (0 != (rc = sendARPAnnouncement (_umExternalInterfaces, pARPPacket, NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                        "sendARPAnnouncement() on all external network interfaces failed with rc = %d\n", rc);
                        return -5;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "successfully sent a gratuitous ARP Announcement packet on all network interfaces in response to an ARP "
                                    "Probe that had the IP address of one of the external interfaces as the Target Protocol Address\n");
                }

                return 0;
            }

            // SPA and SHA != 0 and != MAC or IP addresses of the internal interface of this machine
            if (pARPPacket->ui16Oper == 1) {
                // ARP Request
                if (isIPv4AddressAssignedToInternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    // A host in the internal network wants to send packets to the internal interface --> the OS will take care of that
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "received an ARP Request with the IP of the internal interface as the Target Protocol "
                                    "Address; NetProxy will ignore the packet, as the OS will reply to it\n");
                    return 0;
                }
                if (isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    // A host in the internal network wants to send packets to the external interface --> ignore the ARP Request
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "received an ARP Request with the IP of the external interface as the "
                                    "Target Protocol Address; NetProxy will ignore the packet, but the OS "
                                    "might still reply to it, depending on the configuration\n");
                    return 0;
                }

                if (_connectionManager.isFlowPotentiallyMapped (NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr),
                                                                NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    /* If we end up here, regardless of whether the NetProxy is running in Gateway Mode or in Host Mode,
                     * the NetProxy needs to reply to the ARP Request because it is configured to remap this specific TPA. */

                    const NOMADSUtil::EtherMACAddr sourceMACAddr = NetProxyApplicationParameters::GATEWAY_MODE ?
                        NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress :
                        buildVirtualNetProxyEthernetMACAddress (pARPPacket->tpa.ui8Byte3, pARPPacket->tpa.ui8Byte4);
                    if (0 != (rc = sendARPReplyToHost (_spInternalInterface.get(), pARPPacket, sourceMACAddr))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                        "sendARPReplyToHost() on the internal network interface failed with rc = %d\n", rc);
                        return -6;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                    "successfully sent an ARP Response onto the internal network in response to an "
                                    "ARP Request for the TPA %s that the NetProxy is configured to remap\n",
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                    return 0;
                }
                else if (!NetProxyApplicationParameters::GATEWAY_MODE) {
                    // NetProxy running in Host Mode and IP address not mapped --> ignore the ARP packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                    "received an ARP Request with Target Protocol Address %s, which NetProxy cannot "
                                    "remap to any remote NetProxy address; the packet will be ignored\n",
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                    return 0;
                }

                // ARP Request needs to be forwarded onto the external network --> will be done below
            }
            else if (pARPPacket->ui16Oper == 2) {
                if (NetProxyApplicationParameters::GATEWAY_MODE) {
                    // Received an ARP Response while running in Gateway Mode
                    if (areIPv4AndMACAddressAssignedToInternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr), pARPPacket->tha)) {
                        // ARP Response addressed to this host --> it is not necessary to forward it on the external network
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "received an ARP Response with Source Protocol Address %s addressed to this host (Target "
                                        "Protocol Address: %s); it is not necessary to forward it onto the external network\n",
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl(pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl(pARPPacket->tpa.ui32Addr)).getIPAsString());
                        return 0;
                    }
                    else if (isMACAddressAssignedToInternalInterface (pARPPacket->tha)) {
                        /* The THA is the MAC address of this machine's internal interface, but the TPA does not
                         * match the IP address of that interface. This should never happen, as the NetProxy
                         * does not change the source MAC address or the SHA of Ethernet and ARP packets. */
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                        "received an ARP Response with Source Protocol Address %s, Target Protocol Address "
                                        "%s, and Target Hardware Address the MAC address of the internal interface, which "
                                        "however does not match the TPA; NetProxy will discard the packet\n",
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                        return 0;
                    }
                    else if (isIPv4AddressAssignedToInternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                        // TPA of this machine's internal interface, but the THA does not match --> discard the packet
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                        "received an ARP Response with Source Protocol Address %s and the IPv4 address of the internal interface "
                                        "as the Target Protocol Address, but the Target Hardware Address (%s) does not match the MAC address "
                                        "of the internal interface; this should NEVER happen and NetProxy will discard the packet\n",
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        etherMACAddrToString (pARPPacket->tha).c_str());
                        return 0;
                    }
                }
                else {
                    // NetProxy running in Host Mode --> ignore response
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "received an ARP Response with Source Protocol Address %s and Target Protocol Address %s "
                                    "while the NetProxy is running in Host Mode; the NetProxy will ignore the packet\n",
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                }
            }

            std::unordered_set<std::shared_ptr<NetworkInterface>> usTargetExternalInterfaces{};
            for (const auto & pui32sp : _umExternalInterfaces) {
                if (NOMADSUtil::NetUtils::areInSameNetwork (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr), pui32sp.second->getNetmask()->ui32Addr,
                                                pui32sp.second->getIPv4Addr()->ui32Addr, pui32sp.second->getNetmask()->ui32Addr)) {
                    usTargetExternalInterfaces.insert (pui32sp.second);
                }
            }
            if (usTargetExternalInterfaces.size() == 0) {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                "an ARP packet with operation %hu and TPA set to %s (SPA set to %s) will not be forwarded onto any "
                                "external network because the TPA belongs to a different network than that of any external interface\n",
                                pARPPacket->ui16Oper, NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString(),
                                NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                return 0;
            }

            // Forward the ARP packet onto all external networks
            pARPPacket->hton();
            hton (pEthHeader);
            if (0 != (rc = sendPacketToHost (usTargetExternalInterfaces, pPacket, ui16PacketLen))) {
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                "sendPacketToHost() of an ARP packet (Opcode: %hu - Source Protocol Address: %s - Target Protocol "
                                "Address: %s) of %hu bytes long on the external network interface failed with rc = %d\n",
                                NOMADSUtil::EndianHelper::ntohs (pARPPacket->ui16Oper), NOMADSUtil::InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(),
                                NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                return -7;
            }
            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "successfully forwarded an ARP packet (Opcode: %hu - Source Protocol Address: %s - "
                            "Target Protocol Address: %s) of %hu bytes long onto the external network interface\n",
                            NOMADSUtil::EndianHelper::ntohs (pARPPacket->ui16Oper), NOMADSUtil::InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(),
                            NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
            return 0;
        }
        else if (ui16EtherType == NOMADSUtil::ET_IP) {
            register NOMADSUtil::IPHeader *pIPHeader = reinterpret_cast <NOMADSUtil::IPHeader *> (getPacketWithinEthernetFrame (pEthHeader));
            uint32 ui32SrcAddr = pIPHeader->srcAddr.ui32Addr;
            uint32 ui32DestAddr = pIPHeader->destAddr.ui32Addr;
            pIPHeader->ntoh();
            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;      // Bytes in network order
            pIPHeader->destAddr.ui32Addr = ui32DestAddr;    // Bytes in network order
            uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;

            // Check frame completeness
            if (ui16PacketLen < (NetworkConfigurationSettings::MIN_IP_HEADER_SIZE + getEthernetHeaderLength (pEthHeader))) {
                // IP Header is incomplete --> BAD error: it could cause NetProxy to crash while parsing the next packet!
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                "retrieved an incomplete IPv4 packet (IPv4 Header is also incomplete); only %hu bytes were read "
                                "from the internal network interface; NetProxy will discard the packet\n", ui16PacketLen);
                return -8;
            }

            if (ui16PacketLen < (pIPHeader->ui16TLen + getEthernetHeaderLength (pEthHeader))) {
                uint16 ui16CompleteFrameSize = pIPHeader->ui16TLen + getEthernetHeaderLength (pEthHeader);
                int iMissingBytes = static_cast<int> (ui16CompleteFrameSize) - ui16PacketLen;
                // Could not read the entire Ethernet frame
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                "retrieved an incomplete IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) "
                                "of %hu bytes long (complete frame size in bytes: %hu - missing bytes: %d) from the internal "
                                "interface; NetProxy will discard the packet\n", pIPHeader->ui8Proto,
                                NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                ui16PacketLen, ui16CompleteFrameSize, iMissingBytes);
                return -9;
            }

            // Complete frame
            if (NetProxyApplicationParameters::GATEWAY_MODE) {
                // NetProxy running in Gateway Mode
                if (areIPv4AndMACAddressAssignedToInternalInterface (ui32SrcAddr, pEthHeader->src)) {
                    // The packet was generated by this host and sent via the internal interface --> ignore the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "detected an IPv4 packet sent by this host via the internal interface (IPv4 protocol type: "
                                    "%hhu - source: %s - destination: %s); the NetProxy will ignore the packet as the "
                                    "destination should not be in the external network\n", pIPHeader->ui8Proto,
                                    NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                    return 0;
                }
                if (isMACAddressAssignedToInternalInterface (pEthHeader->src)) {
                    // The packet has the source MAC address of the internal interface but the source IP address does not match --> ignore the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "detected an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) with the source MAC "
                                    "address of the internal interface of this host, but a different source IPv4 address; the NetProxy "
                                    "will ignore the packet as the destination should not be in the external network\n", pIPHeader->ui8Proto,
                                    NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                    return 0;
                }
                if (isIPv4AddressAssignedToInternalInterface (ui32SrcAddr)) {
                    // The packet has the source IPv4 address of the internal interface but the source MAC address does not match --> ignore the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "detected an IPv4 packet (IPv4 protocol type: %hhu - source IP: %s - destination IP: %s) with "
                                    "the source IPv4 address of the internal interface of this host, but a different source MAC "
                                    "address (%s); the NetProxy will ignore the packet\n", pIPHeader->ui8Proto,
                                    NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                    etherMACAddrToString (pEthHeader->src).c_str());
                    return 0;
                }

                if (areIPv4AndMACAddressAssignedToInternalInterface (ui32DestAddr, pEthHeader->dest)) {
                    // Packet addressed to the internal IP address of this host: the kernel will take care of it -- just ignore the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "received a packet addressed to this host (IPv4 protocol type: %hhu - source IP: %s - destination IP: %s); "
                                    "the NetProxy will ignore the packet as the kernel should take care of it\n", pIPHeader->ui8Proto,
                                    NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                    return 0;
                }

                // The packet is not addressed to or generated by this host : check if the TTL needs to be decremented and if forwarding is necessary
                if (!NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE) {
                    // Non-transparent Gateway Mode
                    if (pIPHeader->ui8TTL > 1) {
                        // Decrement the TTL
                        --(pIPHeader->ui8TTL);
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu "
                                        "bytes long; the value of the TTL field was decremented to %hhu\n", pIPHeader->ui8Proto,
                                        NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                        ui16PacketLen, pIPHeader->ui8TTL);
                    }
                    else {
                        // TTL reached 0, throw away the packet!
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                        "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu bytes long "
                                        "with a TTL value of %hhu; dropping it\n", pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(),
                                        NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, pIPHeader->ui8TTL);
                        return 0;
                    }
                }

                if ((pEthHeader->dest != NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress) &&
                    hostBelongsToTheInternalNetwork (pEthHeader->dest)) {
                    // Destination MAC address belongs to a node in the internal network --> ignore the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu bytes long that "
                                    "belongs to a node in the internal network; NetProxy will ignore the packet\n", pIPHeader->ui8Proto,
                                    NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen);
                    return 0;
                }

                if (isMACAddrBroadcast (pEthHeader->dest) || isMACAddrMulticast (pEthHeader->dest)) {
                    // Received a packet with a broadcast or a Multicast Ethernet address
                    if ((isMACAddrBroadcast (pEthHeader->dest) &&
                        (_umInterfacesBroadcastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()].size() > 0)) ||
                        (isMACAddrMulticast (pEthHeader->dest) &&
                        (_umInterfacesMulticastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()].size() > 0))) {

                        // Check if broadcast traffic belongs to the same network of the internal interfaces
                        std::unordered_set<std::shared_ptr<NetworkInterface>> usTargetInterfaces;
                        if (isMACAddrBroadcast (pEthHeader->dest)) {
                            const auto & usBroadcastForwardingInterfaces =
                                _umInterfacesBroadcastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()];
                            for (const auto & spNI : usBroadcastForwardingInterfaces) {
                                if (ui32DestAddr == 0xFFFFFFFFU) {
                                    usTargetInterfaces.insert (spNI);
                                }
                                else if (NOMADSUtil::NetUtils::areInSameNetwork (ui32DestAddr, pReceivingNetworkInterface->getNetmask()->ui32Addr,
                                                                                 spNI->getIPv4Addr()->ui32Addr, spNI->getNetmask()->ui32Addr)) {
                                    usTargetInterfaces.insert (spNI);
                                }
                            }
                        }
                        else {
                            usTargetInterfaces = _umInterfacesMulticastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()];
                        }

                        // Forward multicast/broadcast packets on all external interfaces
                        pIPHeader->srcAddr.ui32Addr = ntohl (ui32SrcAddr);
                        pIPHeader->destAddr.ui32Addr = ntohl (ui32DestAddr);
                        pIPHeader->computeChecksum();
                        pIPHeader->hton();
                        hton (pEthHeader);
                        if (0 != (rc = sendPacketToHost (usTargetInterfaces, pPacket, ui16PacketLen))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                            "sendPacketToHost() of an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu "
                                            "bytes long and with a %s destination MAC address on all external interfaces failed with rc = %d\n",
                                            pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                            ui16PacketLen, isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast", rc);
                            return -10;
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "successfully forwarded an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu bytes "
                                        "long and with a %s destination MAC address on all external interfaces configured for forwarding of %s packets\n",
                                        pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                        ui16PacketLen, isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast",
                                        isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast");

                        // Re-establish the host format for both the Ethernet frame and the IP datagram in case further processing is required
                        ntoh (pEthHeader);
                        pIPHeader->ntoh();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                    }

                    if (!_connectionManager.isFlowPotentiallyMapped (pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr)) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "no remapping configuration found for the %s IPv4 packet (protocol type: %hhu "
                                        "- source: %s - destination: %s); NetProxy will drop the packet\n",
                                        isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast",
                                        pIPHeader->ui8Proto, NOMADSUtil::InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(),
                                        NOMADSUtil::InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString());
                        return 0;
                    }

                    // If we reach this point, the packet needs further processing; it will be done below
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "found a remapping configuration for the %s IPv4 packet (IPv4 protocol type: "
                                    "%hhu - source: %s - destination: %s); NetProxy will process the packet\n",
                                    isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast",
                                    pIPHeader->ui8Proto, NOMADSUtil::InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(),
                                    NOMADSUtil::InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString());
                }
                else if (!_connectionManager.isFlowPotentiallyMapped (pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr)) {
                    // NetProxy is not configured to remap unicast packets with this <Source_IP>:<Destination_IP> pair --> forward it onto the external interface
                    const auto smExternalInterface = selectMainExternalInterfaceInTheSameNetwork (ui32DestAddr);
                    if (pEthHeader->dest == NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress) {
                        /* The packet has the destination MAC address of the internal interface of this host; the NetProxy will change the
                         * destination MAC address to be the same of the main external network's default gateway and then forward the packet. */
                        const auto emaDefaultGWMACAddr = buildEthernetMACAddressFromArray (smExternalInterface->getMACAddr());
                        if (emaDefaultGWMACAddr == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                            "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) with the same "
                                            "destination MAC address of the internal interface of this host, but the destination IPv4 cannot be "
                                            "remapped to any remote NetProxy; the NetProxy could not change the destination MAC address to be "
                                            "the one of the default network gateway of the main external interface; the packet will be dropped\n",
                                            pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                        }
                        pEthHeader->dest = buildEthernetMACAddressFromArray (smExternalInterface->getMACAddr());
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) with the same "
                                        "destination MAC address of the internal interface of this host, but the destination IPv4 cannot "
                                        "be remapped to any remote NetProxy; the NetProxy will change the destination MAC address to be "
                                        "the one of the default network gateway of the interface with name %s and then forward the packet\n",
                                        pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                        smExternalInterface->getUserFriendlyInterfaceName().c_str());
                    }

                    pIPHeader->srcAddr.ui32Addr = ntohl (ui32SrcAddr);
                    pIPHeader->destAddr.ui32Addr = ntohl (ui32DestAddr);
                    pIPHeader->computeChecksum();
                    pIPHeader->hton();
                    hton (pEthHeader);
                    /* Find the external network whose netmask is the longest prefix match for the current destination address.
                     * If none matches, use the default gateway of the main external interface. */
                    if (0 != (rc = sendPacketToHost (smExternalInterface.get(), pPacket, ui16PacketLen))) {
                        if ((ui16PacketLen > smExternalInterface->getMTUSize()) &&
                            (NOMADSUtil::EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                            // Send back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the host in the internal network
                            ntoh (pEthHeader);
                            pIPHeader->ntoh();
                            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                            pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                            if (0 != (rc = buildAndSendICMPMessageToHost (_spInternalInterface.get(), NOMADSUtil::ICMPHeader::T_Destination_Unreachable,
                                                                          NOMADSUtil::ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                          _spInternalInterface->getIPv4Addr()->ui32Addr,
                                                                          pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                                "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                return -11;
                            }
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                            "IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu bytes long could not "
                                            "be forwarded on the external network %s because its size exceeds the MTU (%hu bytes); successfully "
                                            "sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                            pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                            ui16PacketLen, smExternalInterface->getUserFriendlyInterfaceName().c_str(),
                                            smExternalInterface->getMTUSize());
                            return 0;
                        }
                        else if (ui16PacketLen > smExternalInterface->getMTUSize()) {
                            // Fragmentation of IP packets not yet implemented --> drop packet!
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                            "Fragmentation of IPv4 packets not yet implemented: packet will be dropped\n");
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                        "sendPacketToHost() of an IPv4 packet (IP protocol type: %hhu - source: %s - destination: %s) "
                                        "of %hu bytes long on the external interface %s failed with rc = %d\n", pIPHeader->ui8Proto,
                                        NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen,
                                        smExternalInterface->getUserFriendlyInterfaceName().c_str(), rc);
                        return -12;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "successfully forwarded an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) "
                                    "of %hu bytes long and destination MAC address %02X:%02X:%02X:%02X:%02X:%02X that did not require "
                                    "remapping onto the external interface %s\n", pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(),
                                    NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2,
                                    pEthHeader->dest.ui8Byte3, pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6,
                                    smExternalInterface->getUserFriendlyInterfaceName().c_str());
                    return 0;
                }

                // If we reach this point, the packet needs further processing; it will be done below
            }
            else {
                // NetProxy is running in Host Mode
                if (!isIPv4AddressAssignedToInternalInterface (pIPHeader->srcAddr.ui32Addr)) {
                    // Packet does not come from the TAP interface --> ignore it
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "ignoring IPv4 packet whose source IPv4 address (%s) does not match the IPv4 address "
                                    "of the virtual TAP interface (%s) while the NetProxy is running in Host Mode\n",
                                    NOMADSUtil::InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(),
                                    NOMADSUtil::InetAddr(NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address).getIPAsString());
                    return 0;
                }

                if (!_connectionManager.isFlowPotentiallyMapped (pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr)) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "ignoring IPv4 packet whose destination IPv4 address (%s) does not match any "
                                    "entry in the remapping table while the NetProxy is running in Host Mode\n",
                                    NOMADSUtil::InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(),
                                    NOMADSUtil::InetAddr(NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address).getIPAsString());
                    return 0;
                }

                // The received packet needs to be remapped --> it will be done below
            }

            // If we reach this point, the NetProxy needs to remap the packet
            if (pIPHeader->ui8Proto == NOMADSUtil::IP_PROTO_ICMP) {
                auto * pICMPHeader = reinterpret_cast<NOMADSUtil::ICMPHeader *> ((reinterpret_cast<uint8 *> (pIPHeader)) + ui16IPHeaderLen);
                auto * pICMPData = reinterpret_cast<uint8 *> (reinterpret_cast<uint8 *> (pICMPHeader) + sizeof(NOMADSUtil::ICMPHeader));
                uint16 ui16ICMPDataLen = ui16PacketLen - (getEthernetHeaderLength (pEthHeader) + ui16IPHeaderLen + sizeof(NOMADSUtil::ICMPHeader));
                pICMPHeader->ntoh();
                const auto * pProtocolSetting = _configurationManager.mapAddrToProtocol (pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr,
                                                                                         NOMADSUtil::IP_PROTO_ICMP);
                if (!pProtocolSetting) {
                    pProtocolSetting = ProtocolSetting::getDefaultICMPProtocolSetting();
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "received an ICMP packet destined to the IPv4 address %s that could not be mapped to any specific protocol; "
                                    "using the default protocol (%s)", NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(),
                                    pProtocolSetting->getProxyMessageProtocolAsString());
                }

                const auto connectorType = pProtocolSetting->getConnectorTypeFromProtocol();
                QueryResult query{_connectionManager.queryConnectionToRemoteHostForConnectorType (pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr,
                                                                                                  connectorType, pProtocolSetting->getEncryptionType())};
                const auto iaRemoteProxyAddr = query.getBestConnectionAddressSolution();
                if ((iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) || !query.isValid()) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                    "received an ICMP message with type %hhu and code %hhu with source IPv4 address %s and destination IPv4 address %s "
                                    "which could not be mapped to a remote NetProxy; is a general remapping rule for this <Source_IP:Destination_IP> "
                                    "pair missing? NetProxy will reply with a destination unreachable:host unreachable ICMP message\n",
                                    pICMPHeader->ui8Type, pICMPHeader->ui8Code, NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(),
                                    NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString());
                    if (0 != (rc = buildAndSendICMPMessageToHost (_spInternalInterface.get(), NOMADSUtil::ICMPHeader::T_Destination_Unreachable,
                                                                  NOMADSUtil::ICMPHeader::CDU_Host_Unreachable, _spInternalInterface->getIPv4Addr()->ui32Addr,
                                                                  pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                        "buildAndSendICMPMessageToHost() failed with rc = %d\n", rc);
                        return -13;
                    }

                    return 0;
                }
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "received an ICMP message with source IPv4 address %s, type %hhu, code %hhu, and %hu bytes addressed "
                                "to the node with IPv4 address %s that will remapped over %s; rest of header is %hhu|%hhu|%hhu|%hhu\n",
                                NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(), pICMPHeader->ui8Type,
                                pICMPHeader->ui8Code, ui16ICMPDataLen, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(),
                                connectorTypeToString (connectorType), ((uint8*) &pICMPHeader->ui32RoH)[3], ((uint8*) &pICMPHeader->ui32RoH)[2],
                                ((uint8*) &pICMPHeader->ui32RoH)[1], ((uint8*) &pICMPHeader->ui32RoH)[0]);

                auto * pConnection = query.getActiveConnectionToRemoteProxy();
                if (!pConnection) {
                    pConnection = Connection::openNewConnectionToRemoteProxy (_connectionManager, _TCPConnTable, _TCPManager,
                                                                              *this, _statisticsManager, query, false);
                    if (!pConnection) {
                        pConnection = Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                                          iaRemoteProxyAddr, connectorType, pProtocolSetting->getEncryptionType());
                        if (!pConnection) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                            "could not create a new %s Connection to send an ICMP packet to the remote NetProxy\n",
                                            connectorTypeToString (connectorType));
                            return -14;
                        }
                    }
                }
                if (!pConnection->isEnqueueingAllowed()) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                    "not enough space available in the %sConnection buffer; discarding the received ICMP packet\n",
                                    pConnection->getConnectorTypeAsString());
                    return 0;
                }
                if (pConnection->isConnecting()) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                    "cannot send the ICMP ProxyMessage with type %hhu and code %hhu to the remote NetProxy with "
                                    "address %s via %s: the Connection instance still has to finish the connection process\n",
                                    pICMPHeader->ui8Type, pICMPHeader->ui8Code, iaRemoteProxyAddr.getIPAsString(),
                                    pConnection->getConnectorTypeAsString());
                    return 0;
                }

                bool bReachable =
                    _connectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress().getIPAddress(),
                                                                                           iaRemoteProxyAddr.getIPAddress());
                if (0 != (rc = pConnection->sendICMPProxyMessageToRemoteHost (pICMPHeader->ui8Type, pICMPHeader->ui8Code, pICMPHeader->ui32RoH,
                                                                              pIPHeader->srcAddr.ui32Addr, pIPHeader->destAddr.ui32Addr, pIPHeader->ui8TTL,
                                                                              const_cast<const uint8 * const> (pICMPData), ui16ICMPDataLen,
                                                                              pProtocolSetting->getProxyMessageProtocol(), bReachable))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                    "could not send the ICMP ProxyMessage with type %hhu and code %hhu to the remote NetProxy "
                                    "with address %s via %s; rc = %d\n", pICMPHeader->ui8Type, pICMPHeader->ui8Code,
                                    iaRemoteProxyAddr.getIPAsString(), pConnection->getConnectorTypeAsString(), rc);
                    return -15;
                }
            }
            else if (pIPHeader->ui8Proto == NOMADSUtil::IP_PROTO_UDP) {
                auto * pUDPHeader = reinterpret_cast<NOMADSUtil::UDPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
                pUDPHeader->ntoh();
                if (!_connectionManager.isFlowMapped (pIPHeader->srcAddr.ui32Addr, pUDPHeader->ui16SPort,
                                                      pIPHeader->destAddr.ui32Addr, pUDPHeader->ui16DPort)) {
                    if (NetProxyApplicationParameters::GATEWAY_MODE) {
                        /* UDP packet with an IP address that is mapped to a remote NetProxy, but port numbers do not
                         * match --> change destination Ethernet MAC address and forward it to the network gateway. */
                        auto spSelectedNetworkInterface = selectMainExternalInterfaceInTheSameNetwork (ui32DestAddr);
                        if (!(isMACAddrBroadcast (pEthHeader->dest) || isMACAddrMulticast (pEthHeader->dest))) {
                            if (NOMADSUtil::NetUtils::areInSameNetwork (ui32DestAddr, spSelectedNetworkInterface->getNetmask()->ui32Addr,
                                                                        spSelectedNetworkInterface->getIPv4Addr()->ui32Addr,
                                                                        spSelectedNetworkInterface->getNetmask()->ui32Addr)) {
                                /* The destination IPv4 address is in the same network of the selected external interface.
                                 * NetProxy will retrieve the MAC address of the destination, update the Ethernet header,
                                 * and send the packet to the destination host via the selected interface. */
                                pEthHeader->dest = _ARPCache.lookup (NOMADSUtil::EndianHelper::ntohl (ui32DestAddr));
                                if (pEthHeader->dest == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                                    "could not forward a UDP datagram with source %s:%hu and destination %s:%hu located in "
                                                    "the same network of the external interface with name %s because the NetProxy does not "
                                                    "have the MAC address of the destination; the datagram will be cached and an ARP request "
                                                    "will be sent to the destination IPv4 address\n", NOMADSUtil::InetAddr{ui32SrcAddr}.getIPAsString(),
                                                    pUDPHeader->ui16SPort, NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), pUDPHeader->ui16DPort,
                                                    spSelectedNetworkInterface->getUserFriendlyInterfaceName().c_str());

                                    // The packet needs to be cached before sending the ARP request, or the send might change the data stored in the buffer
                                    _ARPTableMissCache.insert (ui32DestAddr, spSelectedNetworkInterface.get(), pPacket, ui16PacketLen);
                                    if (0 != (rc = sendARPRequest (spSelectedNetworkInterface.get(), ui32DestAddr))) {
                                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                                        "could not send an ARP request for the IPv4 address %s; sendARPRequest() "
                                                        "failed with rc = %d; the packet will not be cached\n",
                                                        NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), rc);
                                        _ARPTableMissCache.remove (ui32DestAddr);
                                        return -16;
                                    }

                                    return 0;
                                }
                            }
                            else {
                                /* The destination IPv4 address is NOT in the same network of the selected external interface.
                                * The NetProxy will retrieve the MAC address of the network's default gateway and set it as the
                                * destination MAC address in the Ethernet frame header, change the source MAC address of the packet
                                * to be the same of the selected external interface, and finally transmit the packet. */
                                pEthHeader->src = buildEthernetMACAddressFromArray (spSelectedNetworkInterface->getMACAddr());
                                pEthHeader->dest = _ARPCache.lookup (spSelectedNetworkInterface->getDefaultGateway()->ui32Addr);
                                if (pEthHeader->dest == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                                    "could not forward a UDP datagram with source %s:%hu and destination %s:%hu via "
                                                    "the external interface with name %s because the NetProxy does not have the MAC "
                                                    "address of the network's default gateway with the IPv4 address %s; the datagram "
                                                    "will be cached and an ARP request will be sent to the default gateway\n",
                                                    NOMADSUtil::InetAddr{ui32SrcAddr}.getIPAsString(), pUDPHeader->ui16SPort,
                                                    NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), pUDPHeader->ui16DPort,
                                                    spSelectedNetworkInterface->getUserFriendlyInterfaceName().c_str(),
                                                    NOMADSUtil::InetAddr{spSelectedNetworkInterface->getDefaultGateway()->ui32Addr}.getIPAsString());

                                    // The packet needs to be cached before sending the ARP request, or the send might change the data stored in the buffer
                                    _ARPTableMissCache.insert (ui32DestAddr, spSelectedNetworkInterface.get(), pPacket, ui16PacketLen);
                                    if (0 != (rc = sendARPRequest (spSelectedNetworkInterface.get(), ui32DestAddr))) {
                                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                                        "could not send an ARP request for the IPv4 address %s; sendARPRequest() "
                                                        "failed with rc = %d; the packet will not be cached\n",
                                                        NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), rc);
                                        _ARPTableMissCache.remove (ui32DestAddr);
                                        return -17;
                                    }

                                    return 0;
                                }
                            }
                        }

                        // Prepare the frame to be sent over the network
                        pUDPHeader->hton();
                        pIPHeader->srcAddr.ui32Addr = ntohl (ui32SrcAddr);
                        pIPHeader->destAddr.ui32Addr = ntohl (ui32DestAddr);
                        pIPHeader->computeChecksum();
                        pIPHeader->hton();
                        hton (pEthHeader);
                        if (0 != (rc = sendPacketToHost (spSelectedNetworkInterface.get(), pPacket, ui16PacketLen))) {
                            pIPHeader->ntoh();
                            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                            pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                            pUDPHeader->ntoh();
                            if ((ui16PacketLen > spSelectedNetworkInterface->getMTUSize()) &&
                                (NOMADSUtil::EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                                // Send back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the host in the internal network
                                if (0 != (rc = buildAndSendICMPMessageToHost (_spInternalInterface.get(), NOMADSUtil::ICMPHeader::T_Destination_Unreachable,
                                                                              NOMADSUtil::ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                              _spInternalInterface->getIPv4Addr()->ui32Addr, ui32SrcAddr, pIPHeader))) {
                                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                                    "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                    "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                    return -18;
                                }
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                                "UDP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long could not be forwarded "
                                                "via the external network interface with name %s because its size exceeds the MTU of %hu bytes; "
                                                "successfully sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                                NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), pUDPHeader->ui16SPort, NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                                pUDPHeader->ui16DPort, ui16PacketLen, spSelectedNetworkInterface->getUserFriendlyInterfaceName().c_str(),
                                                spSelectedNetworkInterface->getMTUSize());
                                return 0;
                            }
                            else if (ui16PacketLen > spSelectedNetworkInterface->getMTUSize()) {
                                // Fragmentation of IP packets not yet implemented --> drop packet!
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                                "Fragmentation of IPv4 packets not yet implemented: packet will be dropped!\n");
                            }
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                            "sendPacketToHost() of a UDP packet (source: %s:%hu - destination: %s:%hu) "
                                            "of %hu bytes long on the external interface failed with rc = %d\n",
                                            NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), pUDPHeader->ui16SPort,
                                            NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), pUDPHeader->ui16DPort,
                                            ui16PacketLen, rc);
                            return -19;
                        }
                        ntoh (pEthHeader);
                        pUDPHeader->ntoh();
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "successfully forwarded a UDP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long "
                                        "whose port did not match the entry in the Address Mapping Table on the external network "
                                        "interface; the source and destination Ethernet MAC addresses of the transmitted Ethernet packet "
                                        "were set to %02X:%02X:%02X:%02X:%02X:%02X and %02X:%02X:%02X:%02X:%02X:%02X, respectively\n",
                                        NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), pUDPHeader->ui16SPort, NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                        pUDPHeader->ui16DPort, ui16PacketLen, pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2,
                                        pEthHeader->src.ui8Byte3, pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6,
                                        pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3,
                                        pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
                        return 0;
                    }
                    else {
                        // NetProxy is running in Host Mode
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "received a UDP packet (source: %s:%hu - destination: %s:%hu) that could not be mapped to a remote "
                                        "NetProxy; NetProxy will reply with a <Destination Unreachable:Host Unreachable> ICMP message\n",
                                        NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), pUDPHeader->ui16SPort,
                                        NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), pUDPHeader->ui16DPort);
                        if (0 != (rc = buildAndSendICMPMessageToHost (_spInternalInterface.get(), NOMADSUtil::ICMPHeader::T_Destination_Unreachable,
                                                                      NOMADSUtil::ICMPHeader::CDU_Host_Unreachable, _spInternalInterface->getIPv4Addr()->ui32Addr,
                                                                      pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                            "buildAndSendICMPMessageToHost() failed with rc = %d\n", rc);
                            return -20;
                        }

                        return 0;
                    }
                }

                // The datagram matches a remapping rules --> the NetProxy will process the packet
                uint16 ui16UDPPacketLen = pIPHeader->ui16TLen - ui16IPHeaderLen;
                const auto * pProtocolSetting = _configurationManager.mapAddrToProtocol (pIPHeader->srcAddr.ui32Addr, pUDPHeader->ui16SPort,
                                                                                         pIPHeader->destAddr.ui32Addr, pUDPHeader->ui16DPort,
                                                                                         NOMADSUtil::IP_PROTO_UDP);
                if (!pProtocolSetting) {
                    pProtocolSetting = ProtocolSetting::getDefaultUDPProtocolSetting();
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "received a UDP packet destined to address <%s:%hu> which could not be mapped to any specific protocol; "
                                    "using the default protocol %s", NOMADSUtil::InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(),
                                    pUDPHeader->ui16DPort, pProtocolSetting->getProxyMessageProtocolAsString());
                }
                const auto connectorType = pProtocolSetting->getConnectorTypeFromProtocol();

                if (isMACAddrBroadcast (pEthHeader->dest)) {
                    // For now, do not use DisService for Broadcast and Multicast traffic
                    //if (0 != (rc = sendBCastMCastPacketToDisService ((const uint8*) pEthHeader, getEthernetHeaderLength (pEthHeader) + pIPHeader->ui16TLen))) {
                    //    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                    //                    "sendUDPPacketToDisService() failed with rc = %d\n", rc);
                    //    return -2;
                    //}

                    // For the moment, sendBroadcastPacket() method forwards the packets only to remote NetProxies specified in the configuration file
                    if (0 != (rc = sendBroadcastPacket ((const uint8*) pEthHeader, getEthernetHeaderLength (pEthHeader) + pIPHeader->ui16TLen,
                                                        pIPHeader->srcAddr.ui32Addr, pUDPHeader->ui16SPort, pIPHeader->destAddr.ui32Addr,
                                                        pUDPHeader->ui16DPort, pProtocolSetting->getCompressionSetting()))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                        "sendBroadcastPacket() failed with rc = %d\n", rc);
                        return -21;
                    }
                    return 0;
                }
                else if (isMACAddrMulticast (pEthHeader->dest)) {
                    if (0 != (rc = sendMulticastPacket ((const uint8*) pEthHeader, getEthernetHeaderLength (pEthHeader) + pIPHeader->ui16TLen,
                                                        pIPHeader->srcAddr.ui32Addr, pUDPHeader->ui16SPort, pIPHeader->destAddr.ui32Addr,
                                                        pUDPHeader->ui16DPort, pProtocolSetting->getCompressionSetting()))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                        "sendMulticastPacket() failed with rc = %d\n", rc);
                        return -22;
                    }
                }
                else {
                    // This query is always valid, following the check for isFlowMapped() done above
                    QueryResult query{_connectionManager.queryConnectionToRemoteHostForConnectorType (pIPHeader->srcAddr.ui32Addr, pUDPHeader->ui16SPort,
                                                                                                      pIPHeader->destAddr.ui32Addr, pUDPHeader->ui16DPort,
                                                                                                      connectorType, pProtocolSetting->getEncryptionType())};
                    const auto iaRemoteProxyAddr = query.getBestConnectionAddressSolution();
                    if (iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) {
                        // The mapping matches, but there are no solutions to connect to the remote NetProxy (remote reachability from the local NetProxy is false)
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "received a UDP packet with source <%s:%hu> and destination <%s:%hu> that could not be mapped to a remote "
                                        "NetProxy using a %s connection and %s encryption; remote NetProxy reachability from the local NetProxy "
                                        "set to false?; replying with a <Destination Unreachable:Host Unreachable> ICMP message\n",
                                        NOMADSUtil::InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16SPort,
                                        NOMADSUtil::InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16DPort,
                                        connectorTypeToString (connectorType), encryptionTypeToString (pProtocolSetting->getEncryptionType()));
                        if (0 != (rc = buildAndSendICMPMessageToHost (_spInternalInterface.get(), NOMADSUtil::ICMPHeader::T_Destination_Unreachable,
                                                                      NOMADSUtil::ICMPHeader::CDU_Host_Unreachable,
                                                                      NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address,
                                                                      pIPHeader->srcAddr.ui32Addr, pIPHeader))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                            "buildAndSendICMPMessageToHost() failed with rc = %d\n", rc);
                            return -23;
                        }
                        return 0;
                    }

                    auto * pConnection = query.getActiveConnectionToRemoteProxy();
                    if (!pConnection) {
                        pConnection = Connection::openNewConnectionToRemoteProxy (_connectionManager, _TCPConnTable, _TCPManager,
                                                                                  *this, _statisticsManager, query, false);
                        if (!pConnection) {
                            pConnection = Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                                              iaRemoteProxyAddr, connectorType, pProtocolSetting->getEncryptionType());
                            if (!pConnection) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                                "could not create a new %sConnection to send a UDP packet to the remote NetProxy\n",
                                                connectorTypeToString (connectorType));
                                return -24;
                            }
                        }
                    }

                    if ((pIPHeader->ui16FlagsAndFragOff & (IP_MF_FLAG_FILTER | IP_OFFSET_FILTER)) == 0) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "UDP Unicast packet received: payload size %u; source address: %s:%hu - destination address: %s:%hu\n",
                                        static_cast<unsigned int> (ui16UDPPacketLen) - sizeof(NOMADSUtil::UDPHeader),
                                        NOMADSUtil::InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16SPort,
                                        NOMADSUtil::InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16DPort);
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "UDP Unicast fragmented packet received: IPIdent %hu - offset %hu - payload size %hu; "
                                        "source address: %s:%hu - destination address: %s:%hu\n", pIPHeader->ui16Ident,
                                        (pIPHeader->ui16FlagsAndFragOff & IP_OFFSET_FILTER) * 8, ui16UDPPacketLen,
                                        NOMADSUtil::InetAddr(pIPHeader->srcAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16SPort,
                                        NOMADSUtil::InetAddr(pIPHeader->destAddr.ui32Addr).getIPAsString(), pUDPHeader->ui16DPort);
                    }

                    if (0 > (rc = _localUDPDatagramsManagerThread.addDatagramToOutgoingQueue (pConnection, pProtocolSetting->getCompressionSetting(),
                                                                                              pProtocolSetting->getProxyMessageProtocol(),
                                                                                              pIPHeader, pUDPHeader))) {
                        if (rc == -2) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                            "impossible to reassemble received UDP fragment; dropping fragment\n", rc);
                        }
                        else {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                            "addDatagramToOutgoingQueue() failed with rc = %d\n", rc);
                            return -25;
                        }
                    }
                }
            }
            else if (pIPHeader->ui8Proto == NOMADSUtil::IP_PROTO_TCP) {
                auto * pTCPHeader = reinterpret_cast<NOMADSUtil::TCPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
                pTCPHeader->ntoh();
                if (!_connectionManager.isFlowMapped (pIPHeader->srcAddr.ui32Addr, pTCPHeader->ui16SPort,
                                                      pIPHeader->destAddr.ui32Addr, pTCPHeader->ui16DPort)) {
                    /* TCP packet with an IP address that is mapped to a remote NetProxy, but port numbers do not
                    * match --> change destination Ethernet MAC address and forward it to the network gateway. */
                    /* Find the external network whose netmask is the longest prefix match for the current destination address.
                    * If none matches, use the default gateway of the main external interface. */
                    const auto spSelectedNetworkInterface = selectMainExternalInterfaceInTheSameNetwork (ui32DestAddr);
                    pEthHeader->dest = _ARPCache.lookup (NOMADSUtil::EndianHelper::ntohl (pIPHeader->destAddr.ui32Addr));
                    if (NOMADSUtil::NetUtils::areInSameNetwork (ui32DestAddr, spSelectedNetworkInterface->getNetmask()->ui32Addr,
                                                                spSelectedNetworkInterface->getIPv4Addr()->ui32Addr,
                                                                spSelectedNetworkInterface->getNetmask()->ui32Addr)) {
                        /* The destination IPv4 address is in the same network of the selected external interface.
                        * NetProxy will retrieve the MAC address of the destination, update the Ethernet header,
                        * and send the packet to the destination host via the selected interface. */
                        pEthHeader->dest = _ARPCache.lookup (NOMADSUtil::EndianHelper::ntohl (ui32DestAddr));
                        if (pEthHeader->dest == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                            "could not forward a TCP packet with source %s:%hu and destination %s:%hu located in "
                                            "the same network of the external interface with name %s because the NetProxy does not "
                                            "have the MAC address of the destination; the packet will be cached and an ARP request "
                                            "will be sent to the destination IPv4 address\n", NOMADSUtil::InetAddr{ui32SrcAddr}.getIPAsString(),
                                            pTCPHeader->ui16SPort, NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), pTCPHeader->ui16DPort,
                                            spSelectedNetworkInterface->getUserFriendlyInterfaceName().c_str());

                            // The packet needs to be cached before sending the ARP request, or the send might change the data stored in the buffer
                            _ARPTableMissCache.insert (ui32DestAddr, spSelectedNetworkInterface.get(), pPacket, ui16PacketLen);
                            if (0 != (rc = sendARPRequest (spSelectedNetworkInterface.get(), ui32DestAddr))) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                                "could not send an ARP request for the IPv4 address %s; sendARPRequest() "
                                                "failed with rc = %d; the packet will not be cached\n",
                                                NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), rc);
                                _ARPTableMissCache.remove (ui32DestAddr);
                                return -26;
                            }

                            return 0;
                        }
                    }
                    else {
                        /* The destination IPv4 address is NOT in the same network of the selected external interface.
                        * The NetProxy will retrieve the MAC address of the network's default gateway and set it as the
                        * destination MAC address in the Ethernet frame header, change the source MAC address of the packet
                        * to be the same of the selected external interface, and finally transmit the packet. */
                        pEthHeader->src = buildEthernetMACAddressFromArray (spSelectedNetworkInterface->getMACAddr());
                        pEthHeader->dest = _ARPCache.lookup (spSelectedNetworkInterface->getDefaultGateway()->ui32Addr);
                        if (pEthHeader->dest == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                            "could not forward a TCP packet with source %s:%hu and destination %s:%hu via "
                                            "the external interface with name %s because the NetProxy does not have the MAC "
                                            "address of the network's default gateway with the IPv4 address %s; the packet "
                                            "will be cached and an ARP request will be sent to the default gateway\n",
                                            NOMADSUtil::InetAddr{ui32SrcAddr}.getIPAsString(), pTCPHeader->ui16SPort,
                                            NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), pTCPHeader->ui16DPort,
                                            spSelectedNetworkInterface->getUserFriendlyInterfaceName().c_str(),
                                            NOMADSUtil::InetAddr{spSelectedNetworkInterface->getDefaultGateway()->ui32Addr}.getIPAsString());

                            // The packet needs to be cached before sending the ARP request, or the send might change the data stored in the buffer
                            _ARPTableMissCache.insert (ui32DestAddr, spSelectedNetworkInterface.get(), pPacket, ui16PacketLen);
                            if (0 != (rc = sendARPRequest (spSelectedNetworkInterface.get(), ui32DestAddr))) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                                "could not send an ARP request for the IPv4 address %s; sendARPRequest() "
                                                "failed with rc = %d; the packet will not be cached\n",
                                                NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), rc);
                                _ARPTableMissCache.remove (ui32DestAddr);
                                return -27;
                            }

                            return 0;
                        }
                    }

                    // Prepare the frame to be sent over the network
                    pTCPHeader->hton();
                    pIPHeader->srcAddr.ui32Addr = ntohl (ui32SrcAddr);
                    pIPHeader->destAddr.ui32Addr = ntohl (ui32DestAddr);
                    pIPHeader->computeChecksum();
                    pIPHeader->hton();
                    hton (pEthHeader);
                    if (0 != (rc = sendPacketToHost (spSelectedNetworkInterface.get(), pPacket, ui16PacketLen))) {
                        pIPHeader->ntoh();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                        pTCPHeader->ntoh();
                        if ((ui16PacketLen > spSelectedNetworkInterface->getMTUSize()) &&
                            (NOMADSUtil::EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                            // Send back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the host in the internal network
                            if (0 != (rc = buildAndSendICMPMessageToHost (_spInternalInterface.get(), NOMADSUtil::ICMPHeader::T_Destination_Unreachable,
                                                                          NOMADSUtil::ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                          _spInternalInterface->getIPv4Addr()->ui32Addr, ui32SrcAddr, pIPHeader))) {
                                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                                "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation "
                                                "Needed (Type 3, Code 4) message to the internal network; rc = %d\n", rc);
                                return -28;
                            }
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                            "a TCP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long could not be forwarded "
                                            "on the external network interface with name %s because its size exceeds the MTU of %hu bytes; "
                                            "successfully sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                            NOMADSUtil::InetAddr{ui32SrcAddr}.getIPAsString(), pTCPHeader->ui16SPort,
                                            NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), pTCPHeader->ui16DPort, ui16PacketLen,
                                            spSelectedNetworkInterface->getUserFriendlyInterfaceName().c_str(), spSelectedNetworkInterface->getMTUSize());
                            return 0;
                        }
                        else if (ui16PacketLen > spSelectedNetworkInterface->getMTUSize()) {
                            // Fragmentation of IP packets not yet implemented --> drop packet!
                            checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_Warning,
                                            "Fragmentation of IPv4 packets not yet implemented: packet will be dropped!\n");
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                        "sendPacketToHost() of a TCP packet (source: %s:%hu - destination: %s:%hu) of "
                                        "%hu bytes long on the external interface with name %s failed with rc = %d\n",
                                        NOMADSUtil::InetAddr{ui32SrcAddr}.getIPAsString(), pTCPHeader->ui16SPort,
                                        NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), pTCPHeader->ui16DPort, ui16PacketLen,
                                        spSelectedNetworkInterface->getUserFriendlyInterfaceName().c_str(), rc);
                        return -29;
                    }
                    ntoh (pEthHeader);
                    pTCPHeader->ntoh();
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "successfully forwarded a TCP packet (source: %s:%hu - destination: %s:%hu) of %hu bytes long "
                                    "whose port did not match the entry in the Address Mapping Table on the external network "
                                    "interface; the source and destination Ethernet MAC addresses of the transmitted Ethernet packet "
                                    "were set to %02X:%02X:%02X:%02X:%02X:%02X and %02X:%02X:%02X:%02X:%02X:%02X, respectively\n",
                                    NOMADSUtil::InetAddr{ui32SrcAddr}.getIPAsString(), pTCPHeader->ui16SPort,
                                    NOMADSUtil::InetAddr{ui32DestAddr}.getIPAsString(), pTCPHeader->ui16DPort, ui16PacketLen,
                                    pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3, pEthHeader->src.ui8Byte4,
                                    pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6, pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2,
                                    pEthHeader->dest.ui8Byte3, pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);

                    return 0;
                }
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "received a TCP Packet with SEQ number %u, ACK number %u, and flags %hhu from %s:%hu to %s:%hu "
                                "that matches a remapping rule; NetProxy will process it\n", pTCPHeader->ui32SeqNum,
                                pTCPHeader->ui32AckNum, pTCPHeader->ui8Flags, NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(),
                                pTCPHeader->ui16SPort, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pTCPHeader->ui16DPort);
                if (0 != (rc = _TCPManager.handleTCPPacketFromHost (reinterpret_cast<uint8*> (pIPHeader), pIPHeader->ui16TLen,
                                                                    _localTCPTransmitterThread, _remoteTCPTransmitterThread))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_MildError,
                                    "handleTCPPacketFromHost() failed with rc = %d\n", rc);
                    return -30;
                }
            }
            else {
                checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                "received a packet with IP protocol type %hhu - ignoring it\n", pIPHeader->ui8Proto);
            }
        }
        else {
            // Non-IP, Non-ARP ethernet packet received
            if (NetProxyApplicationParameters::GATEWAY_MODE) {
                // Running in Gateway Mode --> forward it onto the external interface
                if (pEthHeader->src == buildEthernetMACAddressFromArray (_spInternalInterface->getMACAddr())) {
                    // Packet generated by this host --> NetProxy will not forward it onto the external network
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "received a non-IPv4 packet (Ethernet protocol type %hu) of %hu bytes long with destination "
                                    "MAC address %02X:%02X:%02X:%02X:%02X:%02X and source MAC address that of the internal "
                                    "interface of this host; NetProxy will ignore it\n", ui16EtherType, ui16PacketLen,
                                    pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3,
                                    pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
                    return 0;
                }

                hton (pEthHeader);
                if (0 != (rc = sendPacketToHost (_umExternalInterfaces, pPacket, ui16PacketLen))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_SevereError,
                                    "sendPacketToHost() of a non-IPv4 Ethernet packet (ethernet protocol type %hu) "
                                    "of %hu bytes long onto the external interface failed with rc = %d\n",
                                    ui16EtherType, ui16PacketLen, rc);
                    return -31;
                }

                if (ui16EtherType == NOMADSUtil::ET_IP_v6) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "successfully forwarded an IPv6 packet of %hu bytes long onto all external networks "
                                    "(source MAC address %02X:%02X:%02X:%02X:%02X:%02X - destination MAC address "
                                    "%02X:%02X:%02X:%02X:%02X:%02X)\n", ui16PacketLen, pEthHeader->src.ui8Byte1,
                                    pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3, pEthHeader->src.ui8Byte4,
                                    pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6, pEthHeader->dest.ui8Byte1,
                                    pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3, pEthHeader->dest.ui8Byte4,
                                    pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
                }
                else {
                    checkAndLogMsg ("PacketRouter::handlePacketFromInternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "successfully forwarded an unknown Ethernet packet (Ethernet protocol type %hu) of %hu bytes long "
                                    "onto the internal network (source MAC address %02X:%02X:%02X:%02X:%02X:%02X - destination MAC "
                                    "address %02X:%02X:%02X:%02X:%02X:%02X)\n", ui16EtherType, ui16PacketLen, pEthHeader->src.ui8Byte1,
                                    pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3, pEthHeader->src.ui8Byte4,
                                    pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6, pEthHeader->dest.ui8Byte1,
                                    pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3, pEthHeader->dest.ui8Byte4,
                                    pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
                }
            }
        }

        return 0;
    }

    int PacketRouter::handlePacketFromExternalInterface (uint8 * const pPacket, uint16 ui16PacketLen,
                                                         NetworkInterface * const pReceivingNetworkInterface)
    {
        int rc;
        auto * const pEthHeader = reinterpret_cast<NOMADSUtil::EtherFrameHeader *> (pPacket);
        ntoh (pEthHeader);

        if (hostBelongsToTheInternalNetwork (pEthHeader->src)) {
            // Packet originated on the internal network was just reinjected on the external one --> ignore it
            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                            "ignoring ethernet packet originated on the internal network and "
                            "previously forwarded on the external one by the NetProxy\n");
            return 0;
        }

        if (!hostBelongsToTheExternalNetwork (pEthHeader->src)) {
            // Add new host to the list of hosts in the external network
            addMACToExtHostsSet (pEthHeader->src);
            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                            "added MAC address %02X:%02X:%02X:%02X:%02X:%02X to the set of hosts in the external network\n",
                            pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3,
                            pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6);
        }

        uint16 ui16EtherType = getEtherTypeFromEthernetFrame (pEthHeader);
        if (ui16EtherType == NOMADSUtil::ET_ARP) {
            register auto * pARPPacket = reinterpret_cast<NOMADSUtil::ARPPacket *> (getPacketWithinEthernetFrame (pEthHeader));
            pARPPacket->ntoh();
            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                            "ARP Packet: operation %hu - SPA %s - TPA %s\n",
                            pARPPacket->ui16Oper, NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                            NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());

            if (pARPPacket->spa.ui32Addr != 0) {
                if (pARPPacket->sha == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                    // SHA set to 0
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "received an ill-formed ARP packet with operation %hu, SHA set to zero, SPA set to "
                                    "%s, THA set to %s, and TPA set to %s from the external network; ignoring it\n",
                                    pARPPacket->ui16Oper, NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                    etherMACAddrToString (pARPPacket->tha).c_str(),
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                    return 0;
                }

                if (pARPPacket->sha == buildEthernetMACAddressFromArray (_spInternalInterface->getMACAddr())) {
                    // Packets with SHA or SPA of the internal interface should not appear in the external network --> ignoring the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_Warning,
                                    "an ARP packet operation %hu (THA set to %s, TPA set to %s, SPA set to %s) with SHA set to %s "
                                    "that matches the MAC address of the internal interface %s was detected in the external network; "
                                    "ignoring the packet\n", pARPPacket->ui16Oper, etherMACAddrToString (pARPPacket->tha).c_str(),
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString(),
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                    etherMACAddrToString (pARPPacket->sha).c_str(),
                                    _spInternalInterface->getUserFriendlyInterfaceName().c_str());
                    return 0;
                }

                // This is not an ARP probe --> cache the SHA
                if (!isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)) &&
                    !isMACAddressAssignedToExternalInterface (pARPPacket->sha)) {
                    // The sender of the ARP message is not this machine --> opportunistically cache the MAC address of the sender
                    _ARPCache.insert (pARPPacket->spa.ui32Addr, pARPPacket->sha);
                    if (updateMACAddressForDefaultGatewayWithIPv4Address (NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr), pARPPacket->sha) > 0) {
                        // Cached MAC address of the gateway
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "the default gateway of the external interface with name %s has "
                                        "IPv4 address %s and MAC address %02X:%02X:%02X:%02X:%02X:%02X\n",
                                        findNIDWithDefaultGatewayIPv4Address (NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).sInterfaceName.c_str(),
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        pARPPacket->sha.ui8Byte1, pARPPacket->sha.ui8Byte2, pARPPacket->sha.ui8Byte3,
                                        pARPPacket->sha.ui8Byte4, pARPPacket->sha.ui8Byte5, pARPPacket->sha.ui8Byte6);
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "cached MAC address %02X:%02X:%02X:%02X:%02X:%02X for node with IP address %s "
                                        "located in the same network of the external interface with name %s\n",
                                        pARPPacket->sha.ui8Byte1, pARPPacket->sha.ui8Byte2, pARPPacket->sha.ui8Byte3,
                                        pARPPacket->sha.ui8Byte4, pARPPacket->sha.ui8Byte5, pARPPacket->sha.ui8Byte6,
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        pReceivingNetworkInterface->getUserFriendlyInterfaceName().c_str());
                    }

                    // Send any cached packets addressed to the IP for which the corresponding MAC address was just received
                    int iSentPackets = sendCachedPacketsToDestination (pARPPacket->spa.ui32Addr);
                    if (iSentPackets > 0) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "successfully sent %d cached packets to host with IPv4 address %s\n", iSentPackets,
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                    }
                }
                else {
                    // Packet generated by the external interface of this machine --> ignore it
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "received an ARP packet with operation %hu and SPA set to %s that was "
                                    "generated by this machine; NetProxy will ignore it\n", pARPPacket->ui16Oper,
                                    NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                    return 0;
                }

                if (pARPPacket->ui16Oper == 1) {
                    // ARP Request
                    if (isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                        // ARP Request addressed to this machine --> ignore it
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "received an ARP Request with Source Protocol Address %s "
                                        "addressed to this host; NetProxy will ignore the packet\n",
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                        return 0;
                    }
                }

                if (pARPPacket->ui16Oper == 2) {
                    // ARP Response --> check if addressed to this host
                    if (areIPv4AndMACAddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr), pARPPacket->tha)) {
                        // ARP Response addressed to this host --> ignore it
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "received an ARP Response with Source Protocol Address %s "
                                        "addressed to this host; NetProxy will ignore the packet\n",
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString());
                        return 0;
                    }
                    else if (isMACAddressAssignedToExternalInterface (pARPPacket->tha)) {
                        /* The Target Hardware Address is the MAC address of this machine's external interface,
                        * but the IP address does not match. This should never happen, as the NetProxy does
                        * not change the source MAC address or the SHA of Ethernet and ARP packets. */
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_Warning,
                                        "received an ARP Response with Source Protocol Address %s, Target Protocol Address %s, "
                                        "and Target Hardware Address the MAC address of the external interface, which however "
                                        "does not match the TPA; NetProxy will discard the packet\n",
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString());
                        return 0;
                    }
                    else if (isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                        // ARP Response with the TPA of the external interface but a different THA --> this should never happen
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_Warning,
                                        "ARP Response with the Target Protocol Address of the external interface of this machine and "
                                        "Source Protocol Address %s, but the Target Hardware Address %s does not match the MAC address "
                                        "of the external interface; this should never happen and NetProxy will ignore the packet\n",
                                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString(),
                                        etherMACAddrToString (pARPPacket->sha).c_str(), etherMACAddrToString (pARPPacket->tha).c_str());
                        return 0;
                    }
                }

                // The ARP packet was not generated by this machine and it was not addressed to it --> forward the packet onto the internal network
            }
            else {
                // ARP Probe (SPA is 0.0.0.0)
                if (!isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)) &&
                    !isIPv4AddressAssignedToInternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    // NetProxy will forward the ARP probe onto the internal network only if the TPA belongs to the internal network
                    if (NOMADSUtil::NetUtils::areInSameNetwork (pARPPacket->tpa.ui32Addr, pReceivingNetworkInterface->getNetmask()->ui32Addr,
                                                                _spInternalInterface->getIPv4Addr()->ui32Addr,
                                                                _spInternalInterface->getNetmask()->ui32Addr)) {
                        pARPPacket->hton();
                        hton (pEthHeader);
                        if (0 != (rc = sendPacketToHost (_spInternalInterface.get(), pPacket, ui16PacketLen))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                            "sendPacketToHost() of an ARP Probe packet (Target Protocol Address: %s) "
                                            "of %hu bytes long on the internal interface failed with rc = %d\n",
                                            NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                            return -1;
                        }

                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                                        "successfully forwarded an ARP Probe packet (Target Protocol "
                                        "Address: %s) of %hu bytes long onto the internal network\n",
                                        NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
                    }
                    else {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                        "an ARP Probe packet (Target Protocol Address: %s) of %hu bytes long was not forwarded "
                                        "onto the internal network because it does not belong to the network %s\n",
                                        NOMADSUtil::InetAddr{pARPPacket->tpa.ui32Addr}.getIPAsString(), ui16PacketLen,
                                        NOMADSUtil::InetAddr{_spInternalInterface->getIPv4Addr()->ui32Addr &
                                                             _spInternalInterface->getNetmask()->ui32Addr}.getIPAsString());
                    }
                }
                else if (isIPv4AddressAssignedToExternalInterface (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr))) {
                    // Another host is trying to use the IP of one of the external interfaces! --> reply with a gratuitous ARP announcement
                    if (0 != (rc = sendARPAnnouncement (pReceivingNetworkInterface, pARPPacket, pReceivingNetworkInterface->getIPv4Addr()->ui32Addr,
                                                        buildEthernetMACAddressFromArray (pReceivingNetworkInterface->getMACAddr())))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                        "sendARPAnnouncement() on the external network interface failed with rc = %d\n", rc);
                        return -3;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "successfully sent a gratuitous ARP Announcement onto the external interface %s in "
                                    "response to an ARP Probe with the same IPv4 address of that interface (%s) as the TPA\n",
                                    pReceivingNetworkInterface->getUserFriendlyInterfaceName().c_str(),
                                    NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString());
                }
                else {
                    // Another host is trying to use the IP of the internal interface! --> reply with a gratuitous ARP announcement on both interfaces
                    if (0 != (rc = sendARPAnnouncement (pReceivingNetworkInterface, pARPPacket, _spInternalInterface->getIPv4Addr()->ui32Addr,
                                                        buildEthernetMACAddressFromArray (pReceivingNetworkInterface->getMACAddr())))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                        "sendARPAnnouncement() on the external network interface with name %s failed with rc = %d\n",
                                        pReceivingNetworkInterface->getUserFriendlyInterfaceName().c_str(), rc);
                        return -4;
                    }
                    if (0 != (rc = sendARPAnnouncement (_spInternalInterface.get(), pARPPacket, _spInternalInterface->getIPv4Addr()->ui32Addr,
                                                        buildEthernetMACAddressFromArray (_spInternalInterface->getMACAddr())))) {
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                        "sendARPAnnouncement() on the internal network interface failed with rc = %d\n", rc);
                        return -5;
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "successfully sent a gratuitous ARP Announcement on both networks in response to "
                                    "an ARP Probe with the IPv4 address of the internal interface with name %s as TPA\n",
                                    _spInternalInterface->getUserFriendlyInterfaceName().c_str());
                }

                return 0;
            }

            if (!NOMADSUtil::NetUtils::areInSameNetwork (NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr), _spInternalInterface->getNetmask()->ui32Addr,
                                                         _spInternalInterface->getIPv4Addr()->ui32Addr, _spInternalInterface->getNetmask()->ui32Addr)) {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                "an ARP packet with operation %hu and TPA set to %s (SPA set to %s) will not be forwarded onto the "
                                "internal interface %s because the TPA belongs to a different network than that of the interface\n",
                                pARPPacket->ui16Oper, NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->tpa.ui32Addr)).getIPAsString(),
                                NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPPacket->spa.ui32Addr)).getIPAsString(),
                                _spInternalInterface->getUserFriendlyInterfaceName().c_str());
                return 0;
            }

            // ARP packet needs to be forwarded onto the internal network
            pARPPacket->hton();
            hton (pEthHeader);
            if (0 != (rc = sendPacketToHost (_spInternalInterface.get(), pPacket, ui16PacketLen))) {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                "sendPacketToHost() of an ARP packet (opcode: %hu - Source Protocol Address: %s - Target Protocol "
                                "Address: %s) of %hu bytes long on the internal network interface failed with rc = %d\n",
                                NOMADSUtil::EndianHelper::ntohs (pARPPacket->ui16Oper), NOMADSUtil::InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(),
                                NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen, rc);
                return -6;
            }

            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "successfully forwarded an ARP packet (opcode: %hu - Source Protocol Address: %s - "
                            "Target Protocol Address: %s) of %hu bytes long on the internal network\n",
                            NOMADSUtil::EndianHelper::ntohs (pARPPacket->ui16Oper), NOMADSUtil::InetAddr(pARPPacket->spa.ui32Addr).getIPAsString(),
                            NOMADSUtil::InetAddr(pARPPacket->tpa.ui32Addr).getIPAsString(), ui16PacketLen);
            return 0;
        }
        else if (ui16EtherType == NOMADSUtil::ET_IP) {
            register auto * pIPHeader = reinterpret_cast<NOMADSUtil::IPHeader *> (getPacketWithinEthernetFrame (pEthHeader));
            uint32 ui32SrcAddr = pIPHeader->srcAddr.ui32Addr;
            uint32 ui32DestAddr = pIPHeader->destAddr.ui32Addr;
            pIPHeader->ntoh();
            pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
            pIPHeader->destAddr.ui32Addr = ui32DestAddr;
            uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;

            // Check frame completeness
            if (ui16PacketLen < (NetworkConfigurationSettings::MIN_IP_HEADER_SIZE + getEthernetHeaderLength (pEthHeader))) {
                // IP Header is incomplete --> BAD error: it could cause NetProxy to crash while parsing the next packet!
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                "retrieved an incomplete IPv4 packet (IPv4 Header is also incomplete): only %hu bytes were "
                                "read from the external network interface; NetProxy will discard the packet\n", ui16PacketLen);
                return -7;
            }

            if (ui16PacketLen < (pIPHeader->ui16TLen + getEthernetHeaderLength (pEthHeader))) {
                uint16 ui16CompleteFrameSize = pIPHeader->ui16TLen + getEthernetHeaderLength (pEthHeader);
                int iMissingBytes = static_cast<int> (ui16CompleteFrameSize) - ui16PacketLen;
                // Could not read the entire Ethernet frame (probably due to segmentation offload) --> return and retrieve the complete packet
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                "retrieved an incomplete IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: "
                                "%s) of %hu bytes long (complete frame size in bytes: %hu - missing bytes: %d) from the "
                                "external interface; NetProxy will discard the packet\n", pIPHeader->ui8Proto,
                                NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                ui16PacketLen, ui16CompleteFrameSize, iMissingBytes);
                return -8;
            }

            // Complete frame
            if (areIPv4AndMACAddressAssignedToExternalInterface (ui32SrcAddr, pEthHeader->src)) {
                // The packet was generated by this host and sent via the external interface -- just ignore the packet
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "detected an IPv4 packet sent by this host via the external interface (IPv4 protocol "
                                "type: %hhu - source: %s - destination: %s); this method will ignore the packet\n",
                                pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(),
                                NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                return 0;
            }
            else if (isMACAddressAssignedToExternalInterface (pEthHeader->src)) {
                // The packet was sent by this node, but the source IPv4 address did not match; this should never happen --> drop the packet
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "detected an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) "
                                "with the source MAC address of the external interface of this host, but the source "
                                "IPv4 address does not match; NetProxy will ignore the packet\n", pIPHeader->ui8Proto,
                                NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                return 0;
            }

            if (areIPv4AndMACAddressAssignedToExternalInterface (ui32DestAddr, pEthHeader->dest)) {
                // The packet is addressed to the external IPv4 address of this host: the kernel will take care of it -- just ignore the packet
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "received an IPv4 packet addressed to this host (IPv4 protocol type: %hhu - source: %s - "
                                "destination: %s); this method will ignore the packet as the kernel will take care of it\n",
                                pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                return 0;
            }
            else if (isMACAddressAssignedToExternalInterface (pEthHeader->dest)) {
                if (isIPv4AddressAssignedToInternalInterface (ui32DestAddr)) {
                    // The destination IPv4 of the packet is that of the internal interface -- the OS will take care of the packet
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "received an IPv4 packet on the external interface with the destination address of the internal interface "
                                    "(source: %s - destination: %s); NetProxy will ignore the packet, but the OS might process it, if "
                                    "configured accordingly\n", NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                    return 0;
                }

                // The packet was sent to this node, but the destination IPv4 address did not match; this should never happen --> drop the packet
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "received an IPv4 packet with the destination MAC address of the external interface "
                                "of this host, but the destination IPv4 address does not match (IPv4 protocol type: "
                                "%hhu - source: %s - destination: %s); NetProxy will discard the packet\n",
                                pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(),
                                NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString());
                return 0;
            }

            // The packet is not addressed to this host: check if the TTL needs to be decremented and if forwarding is necessary
            if (!NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE) {
                // Non-transparent Gateway Mode
                if (pIPHeader->ui8TTL > 1) {
                    // Decrement the TTL
                    --(pIPHeader->ui8TTL);
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                    "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) "
                                    "of %hu bytes long; the value of the TTL field was decremented to %hhu;\n",
                                    pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(),
                                    NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, pIPHeader->ui8TTL);
                }
                else {
                    // TTL reached 0, throw away the packet!
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) "
                                    "of %hu bytes long with a TTL value of %hhu; dropping it\n", pIPHeader->ui8Proto,
                                    NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                    ui16PacketLen, pIPHeader->ui8TTL);
                    return 0;
                }
            }

            if ((isMACAddrBroadcast (pEthHeader->dest) &&
                (_umInterfacesBroadcastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()].size() > 0)) ||
                (isMACAddrMulticast (pEthHeader->dest) &&
                (_umInterfacesMulticastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()].size() > 0))) {

                // Check if broadcast traffic belongs to the same network of the internal interfaces
                std::unordered_set<std::shared_ptr<NetworkInterface>> usTargetInterfaces{};
                if (isMACAddrBroadcast (pEthHeader->dest)) {
                    const auto & usBroadcastForwardingInterfaces =
                        _umInterfacesBroadcastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()];
                    for (const auto & spNI : usBroadcastForwardingInterfaces) {
                        if (ui32DestAddr == 0xFFFFFFFFU) {
                            usTargetInterfaces.insert (spNI);
                        }
                        else if (NOMADSUtil::NetUtils::areInSameNetwork (ui32DestAddr, pReceivingNetworkInterface->getNetmask()->ui32Addr,
                                                                         spNI->getIPv4Addr()->ui32Addr, spNI->getNetmask()->ui32Addr)) {
                            usTargetInterfaces.insert (spNI);
                        }
                    }
                }
                else {
                    usTargetInterfaces = _umInterfacesMulticastPacketsForwardingRules[pReceivingNetworkInterface->getUserFriendlyInterfaceName()];
                }

                // Forward multicast/broadcast packets onto the internal network
                pIPHeader->srcAddr.ui32Addr = ntohl (ui32SrcAddr);
                pIPHeader->destAddr.ui32Addr = ntohl (ui32DestAddr);
                pIPHeader->computeChecksum();
                pIPHeader->hton();
                hton (pEthHeader);
                if (0 != (rc = sendPacketToHost (usTargetInterfaces, pPacket, ui16PacketLen))) {
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                    "sendPacketToHost() of an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of "
                                    "%hu bytes long and with a %s destination MAC address on the internal interface failed with rc = %d\n",
                                    pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                    ui16PacketLen, isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast", rc);
                    return -9;
                }
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "successfully forwarded an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu bytes "
                                "long and with a %s destination MAC address on all internal interfaces configured for forwarding of %s packets\n",
                                pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                ui16PacketLen, isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast",
                                isMACAddrBroadcast (pEthHeader->dest) ? "broadcast" : "multicast");
                return 0;
            }
            else if (!hostBelongsToTheExternalNetwork (pEthHeader->dest)) {
                // The destination MAC address does not belong to any node in the external network --> forward the packet
                pIPHeader->srcAddr.ui32Addr = ntohl (ui32SrcAddr);
                pIPHeader->destAddr.ui32Addr = ntohl (ui32DestAddr);
                pIPHeader->computeChecksum();
                pIPHeader->hton();
                hton (pEthHeader);
                if (0 != (rc = sendPacketToHost (_spInternalInterface.get(), pPacket, ui16PacketLen))) {
                    if ((ui16PacketLen > _spInternalInterface->getMTUSize()) &&
                        (NOMADSUtil::EndianHelper::ntohs (pIPHeader->ui16FlagsAndFragOff) & IP_DF_FLAG_FILTER)) {
                        // Send back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the host in the external network
                        ntoh (pEthHeader);
                        pIPHeader->ntoh();
                        pIPHeader->srcAddr.ui32Addr = ui32SrcAddr;
                        pIPHeader->destAddr.ui32Addr = ui32DestAddr;
                        if (0 != (rc = buildAndSendICMPMessageToHost (pReceivingNetworkInterface, NOMADSUtil::ICMPHeader::T_Destination_Unreachable,
                                                                      NOMADSUtil::ICMPHeader::CDU_Fragmentation_needed_and_DF_set,
                                                                      pReceivingNetworkInterface->getIPv4Addr()->ui32Addr, ui32SrcAddr, pIPHeader))) {
                            checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                            "buildAndSendICMPMessageToHost() failed when sending back an ICMP Packet Fragmentation Needed "
                                            "(Type %hhu, Code %hhu) message because an IPv4 packet (IPv4 protocol type: %hhu - source: %s "
                                            "- destination: %s) of %hu bytes long was received; rc = %d\n",
                                            static_cast<uint8> (NOMADSUtil::ICMPHeader::T_Destination_Unreachable),
                                            static_cast<uint8> (NOMADSUtil::ICMPHeader::CDU_Fragmentation_needed_and_DF_set),
                                            pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(),
                                            NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                            return -10;
                        }
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_NetDetailDebug,
                                        "IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu bytes long could not "
                                        "be forwarded to the internal network because its size exceeds the MTU of %hu bytes; successfully "
                                        "sent back an ICMP Packet Fragmentation Needed (Type 3, Code 4) message to the sender\n",
                                        pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                        ui16PacketLen, _spInternalInterface->getMTUSize());
                        return 0;
                    }
                    else if (ui16PacketLen > _spInternalInterface->getMTUSize()) {
                        // Fragmentation of IPv4 packets not yet implemented --> drop packet!
                        checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_Warning,
                                        "Fragmentation of IPv4 packets not yet implemented: packet will be dropped!\n");
                    }
                    checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                    "sendPacketToHost() of an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) "
                                    "of %hu bytes long on the internal interface failed with rc = %d\n", pIPHeader->ui8Proto,
                                    NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen, rc);
                    return -11;
                }

                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_LowDetailDebug,
                                "successfully forwarded an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) "
                                "of %hu bytes long and destination MAC address that does not belong to any node in the external "
                                "network to the internal network\n", pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(),
                                NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(), ui16PacketLen);
                return 0;
            }
            else {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "received an IPv4 packet (IPv4 protocol type: %hhu - source: %s - destination: %s) of %hu bytes long, source "
                                "MAC address %02X:%02X:%02X:%02X:%02X:%02X, and destination MAC address %02X:%02X:%02X:%02X:%02X:%02X that does "
                                "not need to be processed or forwarded onto the internal network (destination node in the external network)\n",
                                pIPHeader->ui8Proto, NOMADSUtil::InetAddr(ui32SrcAddr).getIPAsString(), NOMADSUtil::InetAddr(ui32DestAddr).getIPAsString(),
                                ui16PacketLen, pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3,
                                pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6,
                                pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3,
                                pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
            }
        }
        else {
            // Non-IP, Non-ARP ethernet packet received --> forward it onto the internal network
            if (isMACAddressAssignedToExternalInterface (pEthHeader->src)) {
                // Packet generated by this host --> NetProxy will not forward it onto the internal network
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "received a non-IPv4 packet (Ethernet protocol type %hu) of %hu bytes long with destination "
                                "MAC address %02X:%02X:%02X:%02X:%02X:%02X and source MAC address that of the external "
                                "interface of this host; NetProxy will ignore it\n", ui16EtherType, ui16PacketLen,
                                pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3,
                                pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
                return 0;
            }

            hton (pEthHeader);
            if (0 != (rc = sendPacketToHost (_spInternalInterface.get(), pPacket, ui16PacketLen))) {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_SevereError,
                                "sendPacketToHost() of an unknown Ethernet packet (ethernet protocol type %hu) of "
                                "%hu bytes long onto the internal interface failed with rc = %d\n",
                                ui16EtherType, ui16PacketLen, rc);
                return -12;
            }

            ntoh (pEthHeader);
            if (ui16EtherType == NOMADSUtil::ET_IP_v6) {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "successfully forwarded an IPv6 packet of %hu bytes long onto the internal network "
                                "(source MAC address %02X:%02X:%02X:%02X:%02X:%02X - destination MAC address "
                                "%02X:%02X:%02X:%02X:%02X:%02X)\n", ui16PacketLen, pEthHeader->src.ui8Byte1,
                                pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3, pEthHeader->src.ui8Byte4,
                                pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6, pEthHeader->dest.ui8Byte1,
                                pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3, pEthHeader->dest.ui8Byte4,
                                pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
            }
            else {
                checkAndLogMsg ("PacketRouter::handlePacketFromExternalInterface", NOMADSUtil::Logger::L_HighDetailDebug,
                                "successfully forwarded an unknown Ethernet packet (Ethernet protocol type %hu) of %hu "
                                "bytes long onto the internal network (source MAC address %02X:%02X:%02X:%02X:%02X:%02X - "
                                "destination MAC address %02X:%02X:%02X:%02X:%02X:%02X)\n", ui16EtherType, ui16PacketLen,
                                pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3,
                                pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6,
                                pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3,
                                pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
            }
        }

        return 0;
    }

    int PacketRouter::initializeRemoteConnection (uint32 ui32RemoteProxyID, uint32 ui32LocalInterfaceIPv4Address, uint32 ui32RemoteInterfaceIPv4Address,
                                                  ConnectorType connectorType, EncryptionType encryptionType)
    {
        if (ui32RemoteProxyID == 0) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", NOMADSUtil::Logger::L_MildError,
                            "impossible to initialize connection to remote proxy: remote NetProxy UniqueID not valid\n");
            return -1;
        }
        if (connectorType == CT_UNDEF) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", NOMADSUtil::Logger::L_MildError,
                            "impossible to initialize connection to remote proxy: ConnectorType unspecified\n");
            return -2;
        }

        int rc;
        const QueryResult query{_connectionManager.
            queryConnectionToRemoteProxyIDAndInterfaceIPv4AddressForConnectorTypeAndEncryptionType (ui32RemoteProxyID, ui32LocalInterfaceIPv4Address,
                                                                                                    ui32RemoteInterfaceIPv4Address, connectorType, encryptionType)};
        const auto iaRemoteProxyAddr = query.getBestConnectionAddressSolution();
        if (iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", NOMADSUtil::Logger::L_MildError,
                            "impossible to find the necessary information to connect to the remote NetProxy with UniqueID %u\n",
                            ui32RemoteProxyID);
            return -3;
        }

        auto * pConnection = query.getActiveConnectionToRemoteProxy();
        if (!pConnection) {
            pConnection = Connection::openNewConnectionToRemoteProxy (_connectionManager, _TCPConnTable, _TCPManager,
                                                                      *this, _statisticsManager, query, false);
            if (!pConnection) {
                pConnection = Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                                  iaRemoteProxyAddr, connectorType, encryptionType);
                if (!pConnection) {
                    checkAndLogMsg ("PacketRouter::initializeRemoteConnection", NOMADSUtil::Logger::L_MildError,
                                    "impossible to retrieve the Connection to remote NetProxy at address <%s:%hu>\n",
                                    iaRemoteProxyAddr.getIPAsString(), iaRemoteProxyAddr.getPort());
                    return -4;
                }
            }
        }

        bool bReachable = _connectionManager.getReachabilityFromRemoteProxyWithIDAndIPv4Address (ui32RemoteProxyID, query.getLocalProxyInterfaceAddress().getIPAddress(),
                                                                                                 ui32RemoteInterfaceIPv4Address);
        if ((rc = pConnection->sendConnectionInitializedProxyMessage (ConnectionManager::getListOfLocalInterfacesIPv4Addresses(), bReachable)) != 0) {
            checkAndLogMsg ("PacketRouter::initializeRemoteConnection", NOMADSUtil::Logger::L_MildError,
                            "sendConnectionInitializedProxyMessage() failed with rc = %d\n", rc);
            return -5;
        }

        checkAndLogMsg ("PacketRouter::initializeRemoteConnection", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "successfully initialized connection with the remote NetProxy at address <%s:%hu> and UniqueID %u\n",
                        iaRemoteProxyAddr.getIPAsString(), iaRemoteProxyAddr.getPort(), ui32RemoteProxyID);

        return 0;
    }

    int PacketRouter::wrapEthernetIPFrameAndSendToHost (NetworkInterface * const pNI, uint8 * pui8Buf, uint16 ui16PacketLen,
                                                        NOMADSUtil::EtherFrameHeader const * const pEtherFrameHeaderPckt)
    {
        int rc;
        auto * pEthHeader = reinterpret_cast<NOMADSUtil::EtherFrameHeader *> (pui8Buf);
        auto * pIPHeader = reinterpret_cast<NOMADSUtil::IPHeader *> (pui8Buf + sizeof(NOMADSUtil::EtherFrameHeader));
        uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            auto ema = (pIPHeader->destAddr.ui32Addr == NOMADSUtil::EndianHelper::ntohl (pNI->getIPv4Addr()->ui32Addr)) ?
                buildEthernetMACAddressFromArray (pNI->getMACAddr()) : _ARPCache.lookup (pIPHeader->destAddr.ui32Addr);
            if (ema == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                // NetProxy is missing the destination's MAC address --> send an ARP Request and cache the packet or forward it to the network gateway
                if (NOMADSUtil::NetUtils::areInSameNetwork (pNI->getIPv4Addr()->ui32Addr, pNI->getNetmask()->ui32Addr,
                                                            NOMADSUtil::EndianHelper::htonl (pIPHeader->destAddr.ui32Addr), pNI->getNetmask()->ui32Addr)) {
                    checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_NetDetailDebug,
                                    "do not have the MAC address associated to the IP address %s located in the %s network; "
                                    "NetProxy will send an ARP Request to retrieve it and cache this packet to send it later\n",
                                    NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)}.getIPAsString(),
                                    isIPv4AddressAssignedToInternalInterface (pNI->getIPv4Addr()->ui32Addr) ? "internal" : "external");

                    // The packet needs to be cached before sending the ARP request, or the send might change the data stored in the buffer
                    _ARPTableMissCache.insert (pIPHeader->destAddr.ui32Addr, pNI, pui8Buf, ui16PacketLen);
                    if (0 != (rc = sendARPRequest (pNI, NOMADSUtil::EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)))) {
                        checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_MildError,
                                        "could not send ARP request for IP address %s; sendARPRequest() failed "
                                        "with rc = %d; packet will be discarded and deleted from cache\n",
                                        NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)}.getIPAsString(), rc);
                        _ARPTableMissCache.remove (pIPHeader->destAddr.ui32Addr);
                        return -1;
                    }

                    return 0;
                }
                else {
                    // Destination not in the same network of the interface --> use the default gateway's MAC address
                    if (!pNI->getDefaultGateway() || pNI->getDefaultGateway()->ui32Addr == 0) {
                        // IP of the network gateway unknown, or no network gateway available
                        checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_Warning,
                                        "cannot send a packet to the node with IP %s via the network interface with name %s"
                                        "because the IP of the network gateway is unknown; NetProxy will discard this packet\n",
                                        NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)}.getIPAsString(),
                                        pNI->getUserFriendlyInterfaceName().c_str());
                        return 0;
                    }

                    ema = _ARPCache.lookup (pNI->getDefaultGateway()->ui32Addr);
                    if (ema == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
                        checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_NetDetailDebug,
                                        "do not have the MAC address of the network gateway with IPv4 %s, necessary to reach the IPv4 address %s; "
                                        "sending an ARP request to retrieve the gateway MAC address; NetProxy will cache this packet and try "
                                        "to send it later\n", NOMADSUtil::InetAddr{pNI->getDefaultGateway()->ui32Addr}.getIPAsString(),
                                        NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (pIPHeader->destAddr.ui32Addr)}.getIPAsString());

                        // The packet needs to be cached before sending the ARP request, or the send might change the data stored in the buffer
                        _ARPTableMissCache.insert (NOMADSUtil::EndianHelper::ntohl (pNI->getDefaultGateway()->ui32Addr), pNI, pui8Buf, ui16PacketLen);
                        if (0 != (rc = sendARPRequestForGatewayMACAddress (pNI, getNetworkInterfaceDescriptorWithIP (pNI->getIPv4Addr()->ui32Addr)))) {
                            checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_MildError,
                                            "sendARPRequestForGatewayMACAddress() failed with rc = %d\n", rc);
                            _ARPTableMissCache.remove (NOMADSUtil::EndianHelper::ntohl (pNI->getDefaultGateway()->ui32Addr));
                            return -2;
                        }

                        return 0;
                    }
                    checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                    "IPv4 address %s does not belong to the same network of this machine's %s interface; "
                                    "packet will be forwarded to the network gateway using its MAC address\n",
                                    NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl(pIPHeader->destAddr.ui32Addr)}.getIPAsString(),
                                    isIPv4AddressAssignedToInternalInterface (pNI->getIPv4Addr()->ui32Addr) ? "internal" : "external");
                }
            }

            pEthHeader->src = buildEthernetMACAddressFromArray (pNI->getMACAddr());
            pEthHeader->dest = ema;
        }
        else {
            // Running in Host Mode
            pEthHeader->src = buildVirtualNetProxyEthernetMACAddress (pIPHeader->srcAddr.ui8Byte3, pIPHeader->srcAddr.ui8Byte4);
            pEthHeader->dest = buildVirtualNetProxyEthernetMACAddress (pIPHeader->destAddr.ui8Byte3, pIPHeader->destAddr.ui8Byte4);
        }
        pEthHeader->ui16EtherType = NOMADSUtil::ET_IP;

        switch (pIPHeader->ui8Proto) {
        case NOMADSUtil::IP_PROTO_ICMP:
        {
            auto * pICMPHeader = reinterpret_cast<NOMADSUtil::ICMPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
            pICMPHeader->hton();
            break;
        }
        case NOMADSUtil::IP_PROTO_TCP:
        {
            auto * pTCPHeader = reinterpret_cast<NOMADSUtil::TCPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
            pTCPHeader->hton();
            break;
        }
        case NOMADSUtil::IP_PROTO_UDP:
        {
            auto * pUDPHeader = reinterpret_cast<NOMADSUtil::UDPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
            pUDPHeader->hton();
            break;
        }
        default:
        {
            checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_MildError,
                            "received a non-ICMP/non-TCP/non-UDP packet; protocol = %hhu\n",
                            pIPHeader->ui8Proto);
            return -3;
        }
        }
        pIPHeader->hton();
        hton (pEthHeader);

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, ui16PacketLen))) {
            checkAndLogMsg ("PacketRouter::wrapEthernetFrameAndSendToHost", NOMADSUtil::Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            return -4;
        }

        // Converting the packet's byte order back to host format
        ntoh (pEthHeader);
        pIPHeader->ntoh();
        switch (pIPHeader->ui8Proto) {
        case NOMADSUtil::IP_PROTO_ICMP:
        {
            auto * pICMPHeader = reinterpret_cast<NOMADSUtil::ICMPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
            pICMPHeader->ntoh();
            break;
        }
        case NOMADSUtil::IP_PROTO_TCP:
        {
            auto * pTCPHeader = reinterpret_cast<NOMADSUtil::TCPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
            pTCPHeader->ntoh();
            break;
        }
        case NOMADSUtil::IP_PROTO_UDP:
        {
            auto * pUDPHeader = reinterpret_cast<NOMADSUtil::UDPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
            pUDPHeader->ntoh();
            break;
        }
        }

        return 0;
    }

    int PacketRouter::sendTunneledPacketToLocalHost (NetworkInterface * const pNI, const uint8 * const pPacket, int iSize)
    {
        int rc;

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            auto srcEther = reinterpret_cast<const NOMADSUtil::EtherFrameHeader *> (pPacket)->src;
            srcEther.ntoh();

            if (!hostBelongsToTheExternalNetwork (srcEther)) {
                // Add new host to the list of hosts in the internal network
                addMACToExtHostsSet (srcEther);
                checkAndLogMsg ("PacketRouter::sendTunneledPacketToLocalHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "added MAC address %02X:%02X:%02X:%02X:%02X:%02X to the set of hosts in the "
                                "external network\n", srcEther.ui8Byte1, srcEther.ui8Byte2, srcEther.ui8Byte3,
                                srcEther.ui8Byte4, srcEther.ui8Byte5, srcEther.ui8Byte6);
            }
        }

        if ((rc = pNI->writePacket (pPacket, iSize)) != iSize) {
            checkAndLogMsg ("PacketRouter::sendTunneledPacketToLocalHost", NOMADSUtil::Logger::L_MildError,
                            "writePacket() failed with rc = %d\n", rc);
            return -1;
        }

        return 0;
    }

    int PacketRouter::sendARPRequest (NetworkInterface * const pNI, uint32 ui32TargetProtocolAddress)
    {
        int rc;
        if (!NetProxyApplicationParameters::GATEWAY_MODE) {
            // ARP Request is only supported in Gateway mode
            checkAndLogMsg ("PacketRouter::sendARPRequest", NOMADSUtil::Logger::L_MildError,
                            "sendARPRequest() is only supported in Gateway mode\n");
            return -1;
        }

        auto * pui8Buf = reinterpret_cast<uint8 *> (_packetBufferManager.getAndLockWriteBuf());
        auto * pEthHeader = reinterpret_cast<NOMADSUtil::EtherFrameHeader *> (pui8Buf);
        auto * pARPPacket = reinterpret_cast<NOMADSUtil::ARPPacket *> (pui8Buf + sizeof(NOMADSUtil::EtherFrameHeader));
        uint32 ui32SourceProtocolAddress = pNI->getIPv4Addr()->ui32Addr;
        auto srcMACAddr = buildEthernetMACAddressFromArray (pNI->getMACAddr());
        pEthHeader->src = srcMACAddr;
        pEthHeader->dest = NetProxyApplicationParameters::EMA_BROADCAST_ADDR;
        pEthHeader->ui16EtherType = NOMADSUtil::ET_ARP;
        pARPPacket->ui16HType = 1;
        pARPPacket->ui16PType = NOMADSUtil::ET_IP;
        pARPPacket->ui8HLen = 6;
        pARPPacket->ui8PLen = 4;
        pARPPacket->sha = srcMACAddr;
        pARPPacket->spa.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32SourceProtocolAddress);
        pARPPacket->tha.ui8Byte1 = 0;
        pARPPacket->tha.ui8Byte2 = 0;
        pARPPacket->tha.ui8Byte3 = 0;
        pARPPacket->tha.ui8Byte4 = 0;
        pARPPacket->tha.ui8Byte5 = 0;
        pARPPacket->tha.ui8Byte6 = 0;
        pARPPacket->tpa.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32TargetProtocolAddress);
        pARPPacket->ui16Oper = 1;
        pARPPacket->hton();
        hton (pEthHeader);

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, sizeof(NOMADSUtil::EtherFrameHeader) + sizeof(NOMADSUtil::ARPPacket)))) {
            checkAndLogMsg ("PacketRouter::sendARPRequest", NOMADSUtil::Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Buf)) {
                checkAndLogMsg ("PacketRouter::sendARPRequest", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -3;
        }

        if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Buf)) {
            checkAndLogMsg ("PacketRouter::sendARPRequest", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -4;
        }
        checkAndLogMsg ("PacketRouter::sendARPRequest", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully sent ARP request to host with IP address %s\n",
                        NOMADSUtil::InetAddr(ui32TargetProtocolAddress).getIPAsString());

        return 0;
    }

    int PacketRouter::sendARPReplyToHost (NetworkInterface * const pNI, const NOMADSUtil::ARPPacket * const pARPReqPacket,
                                          const NOMADSUtil::EtherMACAddr & rSourceHardwareAddress)
    {
        int rc;
        if (!pARPReqPacket) {
            return -1;
        }
        if (pARPReqPacket->ui16Oper != 1) {
            checkAndLogMsg ("PacketRouter::sendARPReplyToHost", NOMADSUtil::Logger::L_Warning,
                            "sendARPReplyToHost() called passing an ARP packet that is not a "
                            "Request (ARP Operation code is %hu)\n", pARPReqPacket->ui16Oper);
            return -2;
        }

        auto * pui8Buf = reinterpret_cast<uint8 *> (_packetBufferManager.getAndLockWriteBuf());
        auto * pEthHeader = reinterpret_cast<NOMADSUtil::EtherFrameHeader *> (pui8Buf);
        auto * pARPPacket = reinterpret_cast<NOMADSUtil::ARPPacket *> (pui8Buf + sizeof(NOMADSUtil::EtherFrameHeader));
        pEthHeader->dest = pARPReqPacket->sha;
        pEthHeader->src = rSourceHardwareAddress;
        pEthHeader->ui16EtherType = NOMADSUtil::ET_ARP;
        pARPPacket->ui16HType = pARPReqPacket->ui16HType;
        pARPPacket->ui16PType = pARPReqPacket->ui16PType;
        pARPPacket->ui8HLen = pARPReqPacket->ui8HLen;
        pARPPacket->ui8PLen = pARPReqPacket->ui8PLen;
        pARPPacket->sha = rSourceHardwareAddress;
        pARPPacket->spa = pARPReqPacket->tpa;
        pARPPacket->tha = pARPReqPacket->sha;
        pARPPacket->tpa = pARPReqPacket->spa;
        pARPPacket->ui16Oper = 2;
        pARPPacket->hton();
        hton (pEthHeader);

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, sizeof(NOMADSUtil::EtherFrameHeader) + sizeof(NOMADSUtil::ARPPacket)))) {
            checkAndLogMsg ("PacketRouter::sendARPReplyToHost", NOMADSUtil::Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Buf)) {
                checkAndLogMsg ("PacketRouter::sendARPReplyToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -3;
        }

        if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Buf)) {
            checkAndLogMsg ("PacketRouter::sendARPReplyToHost", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -4;
        }
        checkAndLogMsg ("PacketRouter::sendARPRequest", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully sent an ARP reply to host with IP address %s\n",
                        NOMADSUtil::InetAddr(NOMADSUtil::EndianHelper::htonl (pARPReqPacket->spa.ui32Addr)).getIPAsString());

        return 0;
    }

    int PacketRouter::sendARPAnnouncement (NetworkInterface * const pNI, const NOMADSUtil::ARPPacket * const pARPReqPacket,
                                           uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr & rMACAddr)
    {
        if (!pARPReqPacket) {
            return -1;
        }

        int rc;
        auto * pui8Buf = reinterpret_cast<uint8 *> (_packetBufferManager.getAndLockWriteBuf());
        auto * pEthHeader = reinterpret_cast<NOMADSUtil::EtherFrameHeader *> (pui8Buf);
        auto * pARPPacket = reinterpret_cast<NOMADSUtil::ARPPacket *> (pui8Buf + sizeof(NOMADSUtil::EtherFrameHeader));
        pEthHeader->dest = NetProxyApplicationParameters::EMA_BROADCAST_ADDR;
        pEthHeader->src = rMACAddr;
        pEthHeader->ui16EtherType = NOMADSUtil::ET_ARP;
        pARPPacket->ui16HType = pARPReqPacket->ui16HType;
        pARPPacket->ui16PType = pARPReqPacket->ui16PType;
        pARPPacket->ui8HLen = pARPReqPacket->ui8HLen;
        pARPPacket->ui8PLen = pARPReqPacket->ui8PLen;
        pARPPacket->ui16Oper = 2;
        pARPPacket->sha = rMACAddr;
        pARPPacket->spa.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32IPAddr);
        pARPPacket->tha = rMACAddr;
        pARPPacket->tpa = pARPPacket->spa;
        pARPPacket->hton();
        hton (pEthHeader);

        if (0 != (rc = sendPacketToHost (pNI, pui8Buf, sizeof(NOMADSUtil::EtherFrameHeader) + sizeof(NOMADSUtil::ARPPacket)))) {
            checkAndLogMsg ("PacketRouter::sendARPAnnouncement", NOMADSUtil::Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Buf)) {
                checkAndLogMsg ("PacketRouter::sendARPAnnouncement", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }

        if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Buf)) {
            checkAndLogMsg ("PacketRouter::sendARPAnnouncement", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -3;
        }

        return 0;
    }

    int PacketRouter::sendARPAnnouncement (std::unordered_map<uint32, std::shared_ptr<NetworkInterface>> & umExternalInterfaces,
                                           const NOMADSUtil::ARPPacket * const pARPReqPacket, uint32 ui32IPAddr)
    {
        if (!pARPReqPacket) {
            return -1;
        }

        int rc;
        for (auto & pui32spni : umExternalInterfaces) {
            if ((rc = sendARPAnnouncement (pui32spni.second.get(), pARPReqPacket, ui32IPAddr,
                                           buildEthernetMACAddressFromArray (pui32spni.second->getMACAddr()))) != 0) {
                checkAndLogMsg ("PacketRouter::sendARPAnnouncement", NOMADSUtil::Logger::L_Warning,
                                "sendARPAnnouncement() failed when trying to send an ARP announcement "
                                "packet over the network interface with IPv4 address %s\n",
                                NOMADSUtil::InetAddr{pui32spni.second->getIPv4Addr()->ui32Addr}.getIPAsString());
            }
        }

        return 0;
    }

    int PacketRouter::buildAndSendICMPMessageToHost (NetworkInterface * const pNI, NOMADSUtil::ICMPHeader::Type ICMPType,
                                                     NOMADSUtil::ICMPHeader::Code_Destination_Unreachable ICMPCode, uint32 ui32SourceIP,
                                                     uint32 ui32DestinationIP, NOMADSUtil::IPHeader * const pRcvdIPPacket)
    {
        int rc;
        auto * pui8Packet = reinterpret_cast<uint8 *> (_packetBufferManager.getAndLockWriteBuf());
        auto * pIPHeader = reinterpret_cast<NOMADSUtil::IPHeader *> (pui8Packet + sizeof(NOMADSUtil::EtherFrameHeader));
        size_t uiICMPDataLen = ((pRcvdIPPacket->ui8VerAndHdrLen & 0x0F) * 4) + 8;
        uint16 ui16IPPacketLen = sizeof(NOMADSUtil::IPHeader) + sizeof(NOMADSUtil::ICMPHeader) + uiICMPDataLen;
        uint32 ui32IPSourceAddr = pRcvdIPPacket->srcAddr.ui32Addr;
        uint32 ui32IPDestinationAddr = pRcvdIPPacket->destAddr.ui32Addr;
        if (ui16IPPacketLen > NetworkConfigurationSettings::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_MildError,
                            "ICMP packet length with %hu bytes of data exceeds maximum packet size (%hu)\n",
                            ui16IPPacketLen, NetworkConfigurationSettings::PROXY_MESSAGE_MTU);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf(pui8Packet)) {
                checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }

        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof(NOMADSUtil::IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16TLen = ui16IPPacketLen;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui16FlagsAndFragOff = 0;
        pIPHeader->ui8TTL = 128;
        pIPHeader->ui8Proto = NOMADSUtil::IP_PROTO_ICMP;
        pIPHeader->srcAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32SourceIP);
        pIPHeader->destAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32DestinationIP);
        pIPHeader->computeChecksum();

        auto * pICMPHeader = reinterpret_cast<NOMADSUtil::ICMPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + sizeof(NOMADSUtil::IPHeader));
        auto * pICMPData = reinterpret_cast<uint8 *> (pICMPHeader) + sizeof(NOMADSUtil::ICMPHeader);
        pICMPHeader->ui8Type = (uint8) ICMPType;
        pICMPHeader->ui8Code = ICMPCode;
        pRcvdIPPacket->hton();
        pRcvdIPPacket->srcAddr.ui32Addr = ui32IPSourceAddr;
        pRcvdIPPacket->destAddr.ui32Addr = ui32IPDestinationAddr;

        switch (ICMPType) {
        case NOMADSUtil::ICMPHeader::T_Destination_Unreachable:
        {
            switch (ICMPCode) {
            case NOMADSUtil::ICMPHeader::CDU_Host_Unreachable:
            {
                // Rest of Header is unused
                pICMPHeader->ui32RoH = 0;
                memcpy (reinterpret_cast<void *> (pICMPData), reinterpret_cast<const void *> (pRcvdIPPacket), uiICMPDataLen);
                pICMPHeader->computeChecksum (sizeof(NOMADSUtil::ICMPHeader) + sizeof(NOMADSUtil::IPHeader) + 8);
                break;
            }
            case NOMADSUtil::ICMPHeader::CDU_Port_Unreachable:
            {
                // Rest of Header is unused
                pICMPHeader->ui32RoH = 0;
                memcpy (reinterpret_cast<void *> (pICMPData), reinterpret_cast<const void *> (pRcvdIPPacket), uiICMPDataLen);
                pICMPHeader->computeChecksum (sizeof(NOMADSUtil::ICMPHeader) + sizeof(NOMADSUtil::IPHeader) + 8);
                break;
            }
            case NOMADSUtil::ICMPHeader::CDU_Fragmentation_needed_and_DF_set:
            {
                // First word of Rest of Header is unused, the second word is the Next-hop MTU
                pICMPHeader->ui16RoHWord1 = 0;
                pICMPHeader->ui16RoHWord2 = NetProxyApplicationParameters::ETHERNET_DEFAULT_MTU;        // Next-hop MTU
                memcpy (reinterpret_cast<void *> (pICMPData), reinterpret_cast<const void *> (pRcvdIPPacket), uiICMPDataLen);
                pICMPHeader->computeChecksum (sizeof(NOMADSUtil::ICMPHeader) + sizeof(NOMADSUtil::IPHeader) + 8);
                break;
            }
            default:
            {
                checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_Warning,
                                "could not find an entry for ICMP Message Type Destination Unreachable "
                                "with Code %d\n", ICMPCode);
                if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
                    checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                                    "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                }
                return -3;
            }
            }
            break;
        }
        default:
        {
            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_Warning,
                            "could not find an entry for ICMP Message Type %d\n", ICMPType);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }
        }

        if (0 != (rc = wrapEthernetIPFrameAndSendToHost (pNI, pui8Packet, sizeof(NOMADSUtil::EtherFrameHeader) + ui16IPPacketLen))) {
            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_MildError,
                            "wrapEthernetFrameAndSendToHost() failed with rc = %d - could not send an ICMP message to host\n", rc);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -4;
        }
        if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::buildAndSendICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -5;
        }

        return 0;
    }

    int PacketRouter::forwardICMPMessageToHost (uint32 ui32LocalTargetIP, uint32 ui32RemoteOriginationIP, uint32 ui32RemoteProxyIP, uint8 ui8PacketTTL,
                                                NOMADSUtil::ICMPHeader::Type ICMPType, NOMADSUtil::ICMPHeader::Code_Destination_Unreachable ICMPCode,
                                                uint32 ui32RoH, const uint8 * const pICMPData, uint16 ui16PayloadLen)
    {
        (void) ui32RemoteProxyIP;

        int rc;
        auto * pui8Packet = reinterpret_cast<uint8 *> (_packetBufferManager.getAndLockWriteBuf());
        auto * pIPHeader = reinterpret_cast<NOMADSUtil::IPHeader *> (pui8Packet + sizeof(NOMADSUtil::EtherFrameHeader));
        uint16 ui16IPPacketLen = sizeof(NOMADSUtil::IPHeader) + sizeof(NOMADSUtil::ICMPHeader) + ui16PayloadLen;
        if (ui16IPPacketLen > NetworkConfigurationSettings::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", NOMADSUtil::Logger::L_MildError,
                            "ICMP packet length with %hu bytes of data exceeds maximum packet size (%hu)\n",
                            ui16IPPacketLen, NetworkConfigurationSettings::PROXY_MESSAGE_MTU);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf(pui8Packet)) {
                checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }

        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof(NOMADSUtil::IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16TLen = ui16IPPacketLen;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui16FlagsAndFragOff = 0;
        pIPHeader->ui8TTL = ui8PacketTTL;
        pIPHeader->ui8Proto = NOMADSUtil::IP_PROTO_ICMP;
        pIPHeader->srcAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32RemoteOriginationIP);
        pIPHeader->destAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32LocalTargetIP);
        pIPHeader->computeChecksum();

        auto * pICMPHeader = reinterpret_cast<NOMADSUtil::ICMPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + sizeof(NOMADSUtil::IPHeader));
        pICMPHeader->ui8Type = static_cast<uint8> (ICMPType);
        pICMPHeader->ui8Code = ICMPCode;
        pICMPHeader->ui32RoH = ui32RoH;
        memcpy (reinterpret_cast<void *> (reinterpret_cast<uint8 *> (pICMPHeader) + sizeof(NOMADSUtil::ICMPHeader)),
                reinterpret_cast<const void *> (pICMPData), ui16PayloadLen);
        pICMPHeader->computeChecksum (sizeof(NOMADSUtil::ICMPHeader) + ui16PayloadLen);

        if (0 != (rc = wrapEthernetIPFrameAndSendToHost (_spInternalInterface.get(), pui8Packet, sizeof(NOMADSUtil::EtherFrameHeader) + ui16IPPacketLen))) {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", NOMADSUtil::Logger::L_MildError,
                            "wrapEthernetFrameAndSendToHost() failed trying to send an ICMP message to host with "
                            "IP address %s; rc = %d\n", NOMADSUtil::InetAddr(ui32LocalTargetIP).getIPAsString(), rc);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -2;
        }
        else {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", NOMADSUtil::Logger::L_HighDetailDebug,
                            "successfully sent to the host with IP address %s an ICMP message of type %d, code %d and %hu bytes of "
                            "data coming from address %s; RoH is %hhu|%hhu|%hhu|%hhu\n", NOMADSUtil::InetAddr(ui32LocalTargetIP).getIPAsString(),
                            ICMPType, ICMPCode, ui16PayloadLen, NOMADSUtil::InetAddr(ui32RemoteOriginationIP).getIPAsString(),
                            ((uint8*) &pICMPHeader->ui32RoH)[3], ((uint8*) &pICMPHeader->ui32RoH)[2],
                            ((uint8*) &pICMPHeader->ui32RoH)[1], ((uint8*) &pICMPHeader->ui32RoH)[0]);
        }
        if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::forwardICMPMessageToHost", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -3;
        }

        return 0;
    }

    int PacketRouter::sendUDPUniCastPacketToHost (uint32 ui32RemoteOriginationIP, uint32 ui32LocalTargetIP, uint8 ui8PacketTTL,
                                                  const NOMADSUtil::UDPHeader * const pUDPPacket, const NOMADSUtil::IPHeader * pIPHeaderPckt,
                                                  const NOMADSUtil::EtherFrameHeader * pEtherFrameHeaderPckt)
    {
        static const uint16 MAX_UDP_PACKET_LENGTH =
            NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui16InterfaceMTU - static_cast<uint16> (sizeof(NOMADSUtil::IPHeader));

        int rc;
        auto * pui8Packet = reinterpret_cast<uint8 *> (_packetBufferManager.getAndLockWriteBuf());
        auto * pIPHeader = reinterpret_cast<NOMADSUtil::IPHeader *> (pui8Packet + sizeof(NOMADSUtil::EtherFrameHeader));
        auto * pUDPHeader = reinterpret_cast<NOMADSUtil::UDPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + sizeof(NOMADSUtil::IPHeader));

        pIPHeader->ui8VerAndHdrLen = 0x40 | (sizeof(NOMADSUtil::IPHeader) / 4);
        pIPHeader->ui8TOS = 0;
        pIPHeader->ui16Ident = PacketRouter::getMutexCounter()->tick();
        pIPHeader->ui8TTL = ui8PacketTTL;
        pIPHeader->ui8Proto = NOMADSUtil::IP_PROTO_UDP;
        pIPHeader->srcAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32RemoteOriginationIP);
        pIPHeader->destAddr.ui32Addr = NOMADSUtil::EndianHelper::ntohl (ui32LocalTargetIP);

        // It might be necessary to split UDP Packet at the level of the IP protocol
        uint16 ui16WrittenBytes = 0;
        while (ui16WrittenBytes < pUDPPacket->ui16Len) {
            if ((pUDPPacket->ui16Len - ui16WrittenBytes) > MAX_UDP_PACKET_LENGTH) {
                pIPHeader->ui16TLen = ((MAX_UDP_PACKET_LENGTH / 8) * 8) + sizeof(NOMADSUtil::IPHeader);
                pIPHeader->ui16FlagsAndFragOff = IP_MF_FLAG_FILTER | (((ui16WrittenBytes / 8) & IP_OFFSET_FILTER));
            }
            else {
                pIPHeader->ui16TLen = (pUDPPacket->ui16Len - ui16WrittenBytes) + sizeof(NOMADSUtil::IPHeader);
                pIPHeader->ui16FlagsAndFragOff = ((ui16WrittenBytes / 8) & IP_OFFSET_FILTER);
            }
            pIPHeader->computeChecksum();
            memcpy (pUDPHeader, reinterpret_cast<const uint8 *> (pUDPPacket) + ui16WrittenBytes, pIPHeader->ui16TLen - sizeof(NOMADSUtil::IPHeader));
            if (ui16WrittenBytes == 0) {
                // Set the checksum field of the UDP header to zero to avoid the UDP checksum check at the receiver
                pUDPHeader->ui16CRC = 0U;
            }
            if (0 != (rc = wrapEthernetIPFrameAndSendToHost (_spInternalInterface.get(), pui8Packet,
                                                             sizeof(NOMADSUtil::EtherFrameHeader) + pIPHeader->ui16TLen))) {
                checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", NOMADSUtil::Logger::L_MildError,
                                "wrapEthernetFrameAndSendToHost() failed with rc = %d - "
                                "failed to send a UDP message to host\n", rc);
                if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
                    checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", NOMADSUtil::Logger::L_SevereError,
                                    "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
                }
                return -1;
            }
            ui16WrittenBytes += pIPHeader->ui16TLen - sizeof(NOMADSUtil::IPHeader);
        }
        checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "successfully forwarded UDP Packet of %hu bytes from %s:%hu to %s:%hu\n",
                        pUDPPacket->ui16Len, NOMADSUtil::InetAddr(ui32RemoteOriginationIP).getIPAsString(), pUDPPacket->ui16SPort,
                        NOMADSUtil::InetAddr(ui32LocalTargetIP).getIPAsString(), pUDPPacket->ui16DPort);
        if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::sendUDPUniCastPacketToHost", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -2;
        }

        return 0;
    }

    int PacketRouter::sendUDPBCastMCastPacketToHost (const uint8 * const pPacket, uint16 ui16PacketLen)
    {
        int rc;
        auto * pui8Packet = reinterpret_cast<uint8 *> (_packetBufferManager.getAndLockWriteBuf());
        memcpy (pui8Packet, pPacket, ui16PacketLen);
        auto * pEthHeader = reinterpret_cast<NOMADSUtil::EtherFrameHeader *> (pui8Packet);
        pEthHeader->src = NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress;
        auto * pIPHeader = reinterpret_cast<NOMADSUtil::IPHeader *> (pui8Packet + sizeof(NOMADSUtil::EtherFrameHeader));
        uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        auto * pUDPHeader = reinterpret_cast<NOMADSUtil::UDPHeader *> (reinterpret_cast<uint8 *> (pIPHeader) + ui16IPHeaderLen);
        checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "Sending a broadcast/multicast UDP Packet of %hu bytes from %s:%hu to %s:%hu\n",
                        pUDPHeader->ui16Len - sizeof(NOMADSUtil::UDPHeader), NOMADSUtil::InetAddr{pIPHeader->srcAddr.ui32Addr}.getIPAsString(),
                        pUDPHeader->ui16SPort, NOMADSUtil::InetAddr{pIPHeader->destAddr.ui32Addr}.getIPAsString(), pUDPHeader->ui16DPort);
        pIPHeader->srcAddr.ntoh();
        pIPHeader->destAddr.ntoh();
        pUDPHeader->hton();
        pIPHeader->hton();
        hton (pEthHeader);

        if (0 != (rc = sendPacketToHost (_spInternalInterface.get(), const_cast<const uint8 * const> (pui8Packet), ui16PacketLen))) {
            checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", NOMADSUtil::Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
                checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", NOMADSUtil::Logger::L_SevereError,
                                "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            }
            return -1;
        }
        if (0 != _packetBufferManager.findAndUnlockWriteBuf (pui8Packet)) {
            checkAndLogMsg ("PacketRouter::sendUDPBCastMCastPacketToHost", NOMADSUtil::Logger::L_SevereError,
                            "findAndUnlockWriteBuf() failed; impossible to find pointed buffer\n");
            return -2;
        }

        return 0;
    }

    int PacketRouter::sendBroadcastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32BroadcastSrcIP, uint16 ui16SrcPort,
                                           uint32 ui32BroadcastDestIP, uint16 ui16DestPort, const CompressionSettings & compressionSettings)
    {
        int rc;
        const NOMADSUtil::InetAddr broadcastSourceIP{ui32BroadcastSrcIP}, broadcastDestinationIP{ui32BroadcastDestIP};
        const auto * pProtocolSetting = _configurationManager.mapAddrToProtocol (ui32BroadcastSrcIP, ui32BroadcastDestIP, NOMADSUtil::IP_PROTO_UDP);
        if (!pProtocolSetting) {
            pProtocolSetting = ProtocolSetting::getDefaultUDPProtocolSetting();
            checkAndLogMsg ("PacketRouter::sendBroadcastPacket", NOMADSUtil::Logger::L_LowDetailDebug,
                            "received a UDP broadcast message with source address %s and addressed to %s:%hu that could not be mapped "
                            "to any specific protocol; using the standard protocol %s", broadcastSourceIP.getIPAsString(),
                            broadcastDestinationIP.getIPAsString(), ui16DestPort, pProtocolSetting->getProxyMessageProtocolAsString());
        }
        const auto connectorType = pProtocolSetting->getConnectorTypeFromProtocol();
        const auto encryptionType = pProtocolSetting->getEncryptionType();

        const auto vQueryList =
            _connectionManager.queryAllConnectivitySolutionsToMulticastAddressForConnectorType (ui32BroadcastSrcIP, ui16SrcPort, ui32BroadcastDestIP,
                                                                                                ui16DestPort, connectorType, encryptionType);
        for (const auto & query : vQueryList) {
            if (!query.isValid()) {
                continue;
            }

            const auto iaRemoteProxyAddr = query.getBestConnectionAddressSolution();
            if (iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) {
                continue;
            }

            auto * pConnection = query.getActiveConnectionToRemoteProxy();
            if (!pConnection) {
                pConnection = Connection::openNewConnectionToRemoteProxy (_connectionManager, _TCPConnTable, _TCPManager,
                                                                          *this, _statisticsManager, query, false);
                if (!pConnection) {
                    pConnection = Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                                      query.getRemoteProxyServerAddress(), connectorType,
                                                                                      pProtocolSetting->getEncryptionType());
                    if (!pConnection) {
                        checkAndLogMsg ("PacketRouter::sendBroadcastPacket", NOMADSUtil::Logger::L_MildError,
                                        "impossible to retrieve the Connection to remote NetProxy at address <%s:%hu>\n",
                                        iaRemoteProxyAddr.getIPAsString(), iaRemoteProxyAddr.getPort());
                    }
                    else {
                        // Still establishing connection --> skip sending for now
                        checkAndLogMsg ("PacketRouter::sendBroadcastPacket", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "the connection to the remote NetProxy at address <%s:%hu> is not established, yet. "
                                        "Skip sending to this address for now.\n", iaRemoteProxyAddr.getIPAsString(),
                                        iaRemoteProxyAddr.getPort());
                    }

                    // Move on to the next address in the query result list
                    continue;
                }
            }
            if (!pConnection->isEnqueueingAllowed()) {
                checkAndLogMsg ("PacketRouter::sendBroadcastPacket", NOMADSUtil::Logger::L_Warning,
                                "could not enqueue packet in the %s Connection to the remote NetProxy with UniqueID %u and "
                                "%s encryption; the NetProxy will not forward the Broadcast UDP packet over this Connection\n",
                                pConnection->getConnectorTypeAsString(), pConnection->getRemoteNetProxyID(),
                                pConnection->getEncryptionTypeAsString());
                continue;
            }

            if (0 != (rc = pConnection->sendUDPBCastMCastPacketToRemoteHost (ui32BroadcastSrcIP, ui32BroadcastDestIP, pPacket, ui16PacketLen,
                                                                             compressionSettings, pProtocolSetting->getProxyMessageProtocol()))) {
                checkAndLogMsg ("PacketRouter::sendBroadcastPacket", NOMADSUtil::Logger::L_MildError,
                                "sendUDPBCastMCastPacketToRemoteHost() failed when trying to send a message to the "
                                "remote proxy with address %s; rc = %d\n", iaRemoteProxyAddr.getIPAsString(), rc);
                continue;
            }

            checkAndLogMsg ("PacketRouter::sendBroadcastPacket", NOMADSUtil::Logger::L_HighDetailDebug,
                            "UDP broadcast packet of size %hu coming from the host with IP address %s and with destination "
                            "address %s was successfully forwarded to the remote NetProxy at address %s\n",
                            ui16PacketLen, broadcastSourceIP.getIPAsString(), broadcastDestinationIP.getIPAsString(),
                            iaRemoteProxyAddr.getIPAsString());
        }

        return 0;
    }

    int PacketRouter::sendMulticastPacket (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32MulticastSrcIP, uint16 ui16SrcPort,
                                           uint32 ui32MulticastDestIP, uint16 ui16DestPort, const CompressionSettings & compressionSettings)
    {
        int rc;
        const NOMADSUtil::InetAddr multicastSourceIP{ui32MulticastSrcIP}, multicastDestinationIP{ui32MulticastDestIP};
        const auto * pProtocolSetting = _configurationManager.mapAddrToProtocol (ui32MulticastSrcIP, ui32MulticastDestIP, NOMADSUtil::IP_PROTO_UDP);
        if (!pProtocolSetting) {
            pProtocolSetting = ProtocolSetting::getDefaultUDPProtocolSetting();
            checkAndLogMsg ("PacketRouter::sendMulticastPacket", NOMADSUtil::Logger::L_LowDetailDebug,
                            "received a UDP multicast message with source address %s and addressed to %s:%hu that could not be mapped "
                            "to any specific protocol; using the standard protocol %s", multicastSourceIP.getIPAsString(),
                            multicastDestinationIP.getIPAsString(), ui16DestPort, pProtocolSetting->getProxyMessageProtocolAsString());
        }
        const auto connectorType = pProtocolSetting->getConnectorTypeFromProtocol();
        const auto encryptionType = pProtocolSetting->getEncryptionType();

        const auto vQueryList =
            _connectionManager.queryAllConnectivitySolutionsToMulticastAddressForConnectorType (ui32MulticastSrcIP, ui16SrcPort, ui32MulticastDestIP,
                                                                                                ui16DestPort, connectorType, encryptionType);
        for (const auto & query : vQueryList) {
            if (!query.isValid()) {
                continue;
            }

            const auto iaRemoteProxyAddr = query.getBestConnectionAddressSolution();
            if (iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) {
                continue;
            }

            auto * pConnection = query.getActiveConnectionToRemoteProxy();
            if (!pConnection) {
                pConnection = Connection::openNewConnectionToRemoteProxy (_connectionManager, _TCPConnTable, _TCPManager,
                                                                          *this, _statisticsManager, query, false);
                if (!pConnection) {
                    pConnection = Connection::getAvailableConnectionToRemoteNetProxy (query.getRemoteProxyUniqueID(), query.getLocalProxyInterfaceAddress(),
                                                                                      query.getRemoteProxyServerAddress(), connectorType,
                                                                                      pProtocolSetting->getEncryptionType());
                    if (!pConnection) {
                        checkAndLogMsg ("PacketRouter::sendMulticastPacket", NOMADSUtil::Logger::L_Warning,
                                        "impossible to retrieve the Connection to remote NetProxy at address <%s:%hu>\n",
                                        iaRemoteProxyAddr.getIPAsString(), iaRemoteProxyAddr.getPort());
                    }
                    else {
                        // Still establishing connection --> skip sending for now
                        checkAndLogMsg ("PacketRouter::sendMulticastPacket", NOMADSUtil::Logger::L_LowDetailDebug,
                                        "the connection to the remote NetProxy at address <%s:%hu> is not established, yet. Skip sending for now.\n",
                                        iaRemoteProxyAddr.getIPAsString(), iaRemoteProxyAddr.getPort());
                    }

                    // Move on to the next address in the query result list
                    continue;
                }
            }
            if (!pConnection->isEnqueueingAllowed()) {
                checkAndLogMsg ("PacketRouter::sendMulticastPacket", NOMADSUtil::Logger::L_Warning,
                                "could not enqueue packet in the %s Connection to the remote NetProxy with UniqueID %u and "
                                "%s encryption; the NetProxy will not forward the Multicast UDP packet over this Connection\n",
                                pConnection->getConnectorTypeAsString(), pConnection->getRemoteNetProxyID(),
                                pConnection->getEncryptionTypeAsString());
                continue;
            }

            if (0 != (rc = pConnection->sendUDPBCastMCastPacketToRemoteHost (ui32MulticastSrcIP, ui32MulticastDestIP, pPacket, ui16PacketLen,
                                                                             compressionSettings, pProtocolSetting->getProxyMessageProtocol()))) {
                checkAndLogMsg ("PacketRouter::sendMulticastPacket", NOMADSUtil::Logger::L_MildError,
                                "sendUDPBCastMCastPacketToRemoteHost() failed when trying to send a message to the "
                                "remote proxy with address %s; rc = %d\n", iaRemoteProxyAddr.getIPAsString(), rc);
                continue;
            }
            checkAndLogMsg ("PacketRouter::sendMulticastPacket", NOMADSUtil::Logger::L_HighDetailDebug,
                            "UDP multicast packet of size %hu coming from the host with IP address %s with destination "
                            "address %s was successfully forwarded to the remote NetProxy at address %s\n",
                            ui16PacketLen, multicastSourceIP.getIPAsString(), multicastDestinationIP.getIPAsString(),
                            iaRemoteProxyAddr.getIPAsString());
        }

        return 0;
    }

    int PacketRouter::sendBCastMCastPacketToDisService (const uint8 * const pPacket, uint16 ui16PacketLen)
    {
    #if defined (USE_DISSERVICE)
        int rc;
        if (_pDisService == nullptr) {
            checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", NOMADSUtil::Logger::L_MildError,
                            "ignoring BCastMCast packet since DisService has not been initialized\n");
            return -2;
        }
        if (0 != (rc = _pDisService->push (0, "netproxy.unreliable", "", 1, pPacket, ui16PacketLen, 0, 0, 0, 0, nullptr, 0))) {
            checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", NOMADSUtil::Logger::L_MildError,
                            "push() on DisService failed with rc = %d\n", rc);
            return -3;
        }
        else {
            checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "sent a packet of size %d\n", (int) ui16PacketLen);
        }

        return 0;
    #else
        (void) pPacket;
        (void) ui16PacketLen;
        checkAndLogMsg ("PacketRouter::sendBCastMCastPacketToDisService", NOMADSUtil::Logger::L_MildError,
                        "DisService has not been included in build\n");

        return -1;
    #endif
    }

    const NetworkInterfaceDescriptor & PacketRouter::getNetworkInterfaceDescriptorWithIP (uint32 ui32IPv4Address)
    {
        for (const auto & nidInterface : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            if (nidInterface.ui32IPv4Address == ui32IPv4Address) {
                return nidInterface;
            }
        }

        if (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address == ui32IPv4Address) {
            return NetProxyApplicationParameters::NID_INTERNAL_INTERFACE;
        }

        return NetProxyApplicationParameters::NID_INVALID;
    }

    int PacketRouter::sendPacketToHost (NetworkInterface * const pNI, const uint8 * const pPacket, int iSize)
    {
        int rc;

        if ((rc = pNI->writePacket (pPacket, iSize)) != iSize) {
            checkAndLogMsg ("PacketRouter::sendPacketToHost", NOMADSUtil::Logger::L_MildError,
                            "writePacket() failed with rc = %d\n", rc);
            return -1;
        }

        return 0;
    }

    int PacketRouter::sendPacketToHost (const std::vector<NetworkInterface *> & vExternalInterfaces,
                                        const uint8 * const pPacket, int iSize)
    {
        int rc;
        for (auto & pNI : vExternalInterfaces) {
            if ((rc = sendPacketToHost (pNI, pPacket, iSize)) != 0) {
                checkAndLogMsg ("PacketRouter::sendPacketToHost", NOMADSUtil::Logger::L_MildError,
                                "sendPacketToHost() failed with rc = %d when trying to send a packet of %d "
                                "bytes from the network interface with IPv4 address %s\n", rc, iSize,
                                NOMADSUtil::InetAddr{pNI->getIPv4Addr()->ui32Addr}.getIPAsString());
                return -1;
            }
        }

        return 0;
    }

    int PacketRouter::sendPacketToHost (const std::vector<std::shared_ptr<NetworkInterface>> & vspNI,
                                        const uint8 * const pPacket, int iSize)
    {
        int rc;
        for (auto & pNI : vspNI) {
            if ((rc = sendPacketToHost (pNI.get(), pPacket, iSize)) != 0) {
                checkAndLogMsg ("PacketRouter::sendPacketToHost", NOMADSUtil::Logger::L_MildError,
                                "sendPacketToHost() failed with rc = %d when trying to send a packet of %d "
                                "bytes from the network interface with IPv4 address %s\n", rc, iSize,
                                NOMADSUtil::InetAddr{pNI->getIPv4Addr()->ui32Addr}.getIPAsString());
                return -1;
            }
        }

        return 0;
    }

    int PacketRouter::sendPacketToHost (const std::unordered_set<std::shared_ptr<NetworkInterface>> & usspNI,
                                        const uint8 * const pPacket, int iSize)
    {

        int rc;
        for (auto & pNI : usspNI) {
            if ((rc = sendPacketToHost (pNI.get(), pPacket, iSize)) != 0) {
                checkAndLogMsg ("PacketRouter::sendPacketToHost", NOMADSUtil::Logger::L_MildError,
                                "sendPacketToHost() failed with rc = %d when trying to send a packet of %d "
                                "bytes from the network interface with IPv4 address %s\n", rc, iSize,
                                NOMADSUtil::InetAddr{pNI->getIPv4Addr()->ui32Addr}.getIPAsString());
                return -1;
            }
        }

        return 0;
    }

    int PacketRouter::sendPacketToHost (std::unordered_map<uint32, std::shared_ptr<NetworkInterface>> & umExternalInterfaces,
                                        const uint8 * const pPacket, int iSize)
    {
        int rc;
        for (auto & pui32ni : umExternalInterfaces) {
            if ((rc = sendPacketToHost (pui32ni.second.get(), pPacket, iSize)) != 0) {
                checkAndLogMsg ("PacketRouter::sendPacketToHost", NOMADSUtil::Logger::L_MildError,
                                "sendPacketToHost() failed with rc = %d when trying to send a packet of %d "
                                "bytes from the network interface with IPv4 address %s\n", rc, iSize,
                                NOMADSUtil::InetAddr{pui32ni.second->getIPv4Addr()->ui32Addr}.getIPAsString());
                return -1;
            }
        }

        return 0;
    }

    int PacketRouter::sendARPRequestForGatewayMACAddress (NetworkInterface * const pNI, const NetworkInterfaceDescriptor & nid)
    {
        int rc = 0;
        if (nid.ui32IPv4Address == 0U) {
            checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", NOMADSUtil::Logger::L_MildError,
                            "unable to generate an ARP Request packet because the local node's IP address is unknown\n");
            return -1;
        }
        if (nid.ui32IPv4GatewayAddress == 0U) {
            checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", NOMADSUtil::Logger::L_Warning,
                            "unable to generate an ARP Request packet because the default gateway's IP address is unknown\n");
            return -2;
        }
        if (0 != (rc = sendARPRequest (pNI, nid.ui32IPv4GatewayAddress))) {
            checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", NOMADSUtil::Logger::L_MildError,
                            "could not send an ARP Request for the IP address %s from the "
                            "interface with name %s; sendARPRequest() failed with rc = %d\n",
                            NOMADSUtil::InetAddr{nid.ui32IPv4GatewayAddress}.getIPAsString(),
                            nid.sInterfaceName.c_str(), rc);
            return -3;
        }
        checkAndLogMsg ("PacketRouter::sendARPRequestForGatewayMACAddress", NOMADSUtil::Logger::L_HighDetailDebug,
                        "successfully sent an ARP request to the gateway with IP address %s\n",
                        NOMADSUtil::InetAddr{nid.ui32IPv4GatewayAddress}.getIPAsString());

        return 0;
    }

    int PacketRouter::setupNetworkInterfaces (void)
    {
        int rc;
        /*
        * The internal network interface will be instantiated only if running in Gateway Mode,
        * while the external ones need to be accessed anyway (for statistics and for the NPUID)
        */
        for (auto & nidExternalInterface : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            std::shared_ptr<NetworkInterface> spExternalNetworkInterface {PCapInterface::getPCapInterface (nidExternalInterface.sInterfaceName.c_str())};
            if (spExternalNetworkInterface == nullptr) {
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                                "could not access the external network interface with name <%s>\n",
                                nidExternalInterface.sInterfaceName.c_str());
                return -1;
            }

            if ((rc = updateDescriptorFromNetworkInterface (nidExternalInterface, spExternalNetworkInterface)) != 0) {
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                                "an error was encountered when trying to update the NetworkInterfaceDescriptor of the network "
                                "interface with name %s; rc = %d\n", nidExternalInterface.sInterfaceName.c_str(), rc);
                return -2;
            }

            _umExternalInterfaces[nidExternalInterface.ui32IPv4Address] = spExternalNetworkInterface;
        }

        std::shared_ptr<NetworkInterface> spInternalNetworkInterface {NetProxyApplicationParameters::GATEWAY_MODE ?
            static_cast<NetworkInterface *> (PCapInterface::getPCapInterface (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.sInterfaceName.c_str())) :
            static_cast<NetworkInterface *> (TapInterface::createAndInitTAPInterface())};
        if (!spInternalNetworkInterface) {
            checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                            "an invalid (nullptr) handler returned from the call to %s\n",
                            NetProxyApplicationParameters::GATEWAY_MODE ? "PCapInterface::getPCapInterface()" :
                            "TapInterface::createAndInitTAPInterface()");
            return -3;
        }
        if ((rc = updateDescriptorFromNetworkInterface (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE, spInternalNetworkInterface)) != 0) {
            checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                            "an error was encountered when trying to update the NetworkInterfaceDescriptor of the network interface with "
                            "name %s; rc = %d\n", NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.sInterfaceName.c_str(), rc);
            return -4;
        }
        _spInternalInterface = spInternalNetworkInterface;

        return 0;
    }

    int PacketRouter::setupNetSensor (void)
    {
        checkAndLogMsg ("PacketRouter::init", NOMADSUtil::Logger::L_Info,
                        "Initializing NetSensor...\n");

        _upNetSensor = make_unique<IHMC_NETSENSOR::NetSensor> (IHMC_NETSENSOR::Mode::EM_NETPROXY);

        _upNetSensor->addMonitoringInterface (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.sInterfaceName.c_str());
        for (const auto & nidExternalInterface : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            _upNetSensor->addMonitoringInterface (nidExternalInterface.sInterfaceName.c_str());
        }

        // Pass info about the internal interface to NetSensor
        _upNetSensor->setInterfaceInfo ({NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.sInterfaceName.c_str(),
                                         NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.emaInterfaceMACAddress,
                                         NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address,
                                         NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4NetMask,
                                         NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4GatewayAddress,
                                         true, "", ""});

        _upNetSensor->initAsComponent (NetProxyApplicationParameters::NETSENSOR_STATISTICS_IP_ADDRESS.c_str());
        _upNetSensor->start();

        return 0;
    }

    int PacketRouter::sendCachedPacketsToDestination (uint32 ui32DestinationIPAddress)
    {
        int rc, counter = 0;
        if (!_ARPTableMissCache.hasCachedPacketsWithDestination (ui32DestinationIPAddress)) {
            checkAndLogMsg ("PacketRouter::sendCachedPacketsToDestination", NOMADSUtil::Logger::L_HighDetailDebug,
                            "there are no cached packets with destination IPv4 address %s (key is %u)\n",
                            NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (ui32DestinationIPAddress)}.getIPAsString(),
                            ui32DestinationIPAddress);
            return 0;
        }

        auto & rdeqCachedPackets = _ARPTableMissCache.lookup (ui32DestinationIPAddress);
        for (auto & rATMP : rdeqCachedPackets) {
            if (0 != (rc = wrapEthernetIPFrameAndSendToHost (rATMP.getNetworkInterface(), rATMP.getPacket(), rATMP.getPacketLen()))) {
                checkAndLogMsg ("PacketRouter::sendCachedPacketsToDestination", NOMADSUtil::Logger::L_MildError,
                                "failed trying to resend a cached packet to host with IP %s; "
                                "wrapEthernetFrameAndSendToHost() failed with rc = %d\n",
                                NOMADSUtil::InetAddr{NOMADSUtil::EndianHelper::htonl (ui32DestinationIPAddress)}.getIPAsString(), rc);
                continue;
            }
            ++counter;
        }
        _ARPTableMissCache.remove (ui32DestinationIPAddress);

        return counter;
    }

    int PacketRouter::sendPacketOverTheTunnel (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32SourceIP, uint32 ui32DestinationIP)
    {
        static const auto tunnelCT = CT_UDPSOCKET;
        static const auto tunnelET = ET_PLAIN;

        if (ui32DestinationIP == 0) {
            // Transmit the packet to all remote hosts in the address mapping book
            auto vConnSol = _connectionManager.getAllConnectivitySolutions();
            for (auto pConnSol : vConnSol) {
                int rc;
                auto query = pConnSol->getBestConnectionSolutionForConnectorAndEncryptionType (tunnelCT, tunnelET);
                if (0 != (rc = sendPacketOverTheTunnelImpl (pPacket, ui16PacketLen, ui32SourceIP, ui32DestinationIP, query, tunnelCT, tunnelET))) {
                    checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnel", NOMADSUtil::Logger::L_SevereError,
                                    "sendPacketOverTheTunnelImpl() for the remote NetProxy with IP %s failed with rc = %d\n",
                                    query.getRemoteProxyServerAddress().getIPAsString(), rc);
                    continue;
                }
                checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnel", NOMADSUtil::Logger::L_MediumDetailDebug,
                                "successfully tunneled a packet of %hu bytes long to the remote NetProxy with IP %s\n",
                                ui16PacketLen, query.getRemoteProxyServerAddress().getIPAsString());
            }

            return 0;
        }

        auto query = _connectionManager.queryConnectionToRemoteHostForConnectorType (ui32SourceIP, 0, ui32DestinationIP, 0, tunnelCT, tunnelET);
        return sendPacketOverTheTunnelImpl (pPacket, ui16PacketLen, ui32SourceIP, ui32DestinationIP, query, tunnelCT, tunnelET);
    }

    int PacketRouter::sendPacketOverTheTunnelImpl (const uint8 * const pPacket, uint16 ui16PacketLen, uint32 ui32SourceIP, uint32 ui32DestinationIP,
                                                   const QueryResult & qrQuery, ConnectorType ct, EncryptionType et) {
        int rc = 0;

        if (!qrQuery.isValid()) {
            checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnelImpl", NOMADSUtil::Logger::L_SevereError,
                            "queryConnectionToRemoteHostForConnectorType() for the "
                            "remapped IP %s did not return a valid query result\n",
                            NOMADSUtil::InetAddr{ui32DestinationIP}.getIPAsString());
            return -1;
        }

        const auto iaRemoteProxyAddr = qrQuery.getBestConnectionAddressSolution();
        if (iaRemoteProxyAddr == NetProxyApplicationParameters::IA_INVALID_ADDR) {
            checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnelImpl", NOMADSUtil::Logger::L_SevereError,
                            "getBestConnectionSolution() when querying for a connection solution "
                            "for remapped IP address %s returned an invalid InetAddr instance\n",
                            NOMADSUtil::InetAddr{ui32DestinationIP}.getIPAsString());
            return -2;
        }
        checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnelImpl", NOMADSUtil::Logger::L_HighDetailDebug,
                        "received a %hu bytes long packet with target IP %s that needs "
                        "to be tunnelled to the remote NetProxy with address %s\n",
                        ui16PacketLen, NOMADSUtil::InetAddr{ui32DestinationIP}.getIPAsString(),
                        iaRemoteProxyAddr.getIPAsString());

        auto * pConnection = qrQuery.getActiveConnectionToRemoteProxy();
        if (!pConnection) {
            pConnection = Connection::openNewConnectionToRemoteProxy (_connectionManager, _TCPConnTable, _TCPManager,
                                                                      *this, _statisticsManager, qrQuery, false);
            if (!pConnection) {
                pConnection = Connection::getAvailableConnectionToRemoteNetProxy (qrQuery.getRemoteProxyUniqueID(),
                                                                                  qrQuery.getLocalProxyInterfaceAddress(),
                                                                                  iaRemoteProxyAddr, ct, et);
                if (!pConnection) {
                    checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnelImpl", NOMADSUtil::Logger::L_SevereError,
                                    "could not create a new %sConnection to tunnel a packet to the remote NetProxy\n",
                                    connectorTypeToString (ct));
                    return -3;
                }
            }
        }
        if (!pConnection->isEnqueueingAllowed()) {
            checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnelImpl", NOMADSUtil::Logger::L_Warning,
                            "could not enqueue packet in the %s connector; ignoring received packet\n",
                            pConnection->getConnectorTypeAsString());
            return 0;
        }

        if (0 != (rc = pConnection->sendTunneledEthernetPacket (ui32SourceIP, ui32DestinationIP, pPacket, ui16PacketLen))) {
            checkAndLogMsg ("PacketRouter::sendPacketOverTheTunnelImpl", NOMADSUtil::Logger::L_MildError,
                            "could not send a packet of %hu bytes long to the remote proxy with address <%s:%hu> via %s; rc = %d\n",
                            ui16PacketLen, iaRemoteProxyAddr.getIPAsString(), iaRemoteProxyAddr.getPort(),
                            pConnection->getConnectorTypeAsString(), rc);
            return -4;
        }

        return 0;
    }

    int PacketRouter::updateDescriptorFromNetworkInterface (NetworkInterfaceDescriptor & nid, std::shared_ptr<NetworkInterface> spNetworkInterface)
    {
        {
            // Try to retrieve the IPv4 address of the external network interface by querying the device itself
            uint32 ui32IPv4QueriedAddress = spNetworkInterface->getIPv4Addr() ?
                spNetworkInterface->getIPv4Addr()->ui32Addr : 0;
            if ((ui32IPv4QueriedAddress == 0) && (nid.ui32IPv4Address == 0)) {
                // Error
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                                "could not determine the IPv4 address of the external network "
                                "interface with name %s\n", nid.sInterfaceName.c_str());
                return -2;
            }

            if (nid.ui32IPv4Address == 0) {
                // Successfully queried the NIC, nothing specified via configuration
                nid.ui32IPv4Address = ui32IPv4QueriedAddress;
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Info,
                                "retrieved the IPv4 address %s for the external network interface with name %s\n",
                                NOMADSUtil::InetAddr(nid.ui32IPv4Address).getIPAsString(), nid.sInterfaceName.c_str());
            }
            else if (ui32IPv4QueriedAddress == 0) {
                // Use the value read from the configuration file
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Info,
                                "could not query the network interface with name %s for the IPv4 address; "
                                "NetProxy will use the address %s, specified in the configuration file\n",
                                nid.sInterfaceName.c_str(), NOMADSUtil::InetAddr(nid.ui32IPv4Address).getIPAsString());
                spNetworkInterface->setIPv4Address (nid.ui32IPv4Address);
            }
            else if (ui32IPv4QueriedAddress != nid.ui32IPv4Address) {
                // The IP address queried from the NIC does not match the IP address provided in the configuration
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Warning,
                                "the IPv4 address retrieved from the external network interface with name %s differs from "
                                "the one specified in the configuration file: retrieved IP address is %s - configured IPv4 "
                                "address is %s; NetProxy will use the IPv4 address retrieved from the network interface\n",
                                nid.sInterfaceName.c_str(), NOMADSUtil::InetAddr(ui32IPv4QueriedAddress).getIPAsString(),
                                NOMADSUtil::InetAddr(nid.ui32IPv4Address).getIPAsString());
                nid.ui32IPv4Address = ui32IPv4QueriedAddress;
            }
        }

        {
            // Try to retrieve the IPv4 netmask of the external network interface by querying the device itself
            uint32 ui32IPv4QueriedNetmask = spNetworkInterface->getNetmask() ?
                spNetworkInterface->getNetmask()->ui32Addr : 0;
            if ((ui32IPv4QueriedNetmask == 0) && (nid.ui32IPv4NetMask == 0)) {
                // Error
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                                "could not determine the IPv4 netmask of the external network "
                                "interface with name %s\n", nid.sInterfaceName.c_str());
                return -3;
            }

            if (nid.ui32IPv4NetMask == 0) {
                // Successfully queried the NIC, nothing specified via configuration
                nid.ui32IPv4NetMask = ui32IPv4QueriedNetmask;
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Info,
                                "retrieved IPv4 netmask %s for the external network interface with name %s\n",
                                NOMADSUtil::InetAddr(nid.ui32IPv4NetMask).getIPAsString(), nid.sInterfaceName.c_str());
            }
            else if (ui32IPv4QueriedNetmask == 0) {
                // Use the value read from the configuration file
                checkAndLogMsg ("main", NOMADSUtil::Logger::L_Info,
                                "could not query the network interface with name %s for the IPv4 netmask; "
                                "NetProxy will use the address %s, specified in the configuration file\n",
                                nid.sInterfaceName.c_str(), NOMADSUtil::InetAddr(nid.ui32IPv4NetMask).getIPAsString());
                spNetworkInterface->setIPv4Netmask (nid.ui32IPv4NetMask);
            }
            else if (ui32IPv4QueriedNetmask != nid.ui32IPv4NetMask) {
                // The IPv4 netmask queried from the NIC does not match the IPv4 netmask provided in the configuration
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Warning,
                                "the IPv4 netmask retrieved from the external network interface with name %s differs from "
                                "the one specified in the configuration file: retrieved IPv4 netmask is %s - configured IPv4 "
                                "netmask is %s; NetProxy will use the IPv4 netmask retrieved from the network interface\n",
                                nid.sInterfaceName.c_str(), NOMADSUtil::InetAddr(ui32IPv4QueriedNetmask).getIPAsString(),
                                NOMADSUtil::InetAddr(nid.ui32IPv4NetMask).getIPAsString());
                nid.ui32IPv4NetMask = ui32IPv4QueriedNetmask;
            }
        }

        {
            // Try to retrieve the MTU of the external network interface by querying the device itself
            uint16 ui16MTU = (spNetworkInterface->getMTUSize() < NetProxyApplicationParameters::ETHERNET_MAX_MTU) ?
                spNetworkInterface->getMTUSize() : 0;
            if ((ui16MTU == 0) && (nid.ui16InterfaceMTU == 0)) {
                // Error
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                                "could not determine the MTU of the external network interface "
                                "with name %s\n", nid.sInterfaceName.c_str());
                return -4;
            }

            if (nid.ui16InterfaceMTU == 0) {
                // Successfully queried the NIC, nothing specified via configuration
                nid.ui16InterfaceMTU = ui16MTU;
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Info,
                                "retrieved the MTU value %hu for the external network interface with name %s\n",
                                ui16MTU, nid.sInterfaceName.c_str());
            }
            else if (ui16MTU == 0) {
                // Use the value read from the configuration file
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Info,
                                "could not query the network interface with name %s for the MTU; NetProxy will use the value "
                                "%hu, specified in the configuration file\n", nid.sInterfaceName.c_str(), nid.ui16InterfaceMTU);
                spNetworkInterface->setMTU (nid.ui16InterfaceMTU);
            }
            else if (ui16MTU != nid.ui16InterfaceMTU) {
                // The MTU queried from the NIC does not match the MTU provided in the configuration
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Warning,
                                "the MTU retrieved from the external network interface with name %s differs from the one specified "
                                "in the configuration file: retrieved MTU is %hu - configured MTU is %hu; NetProxy will use the MTU "
                                "retrieved from the network interface\n", nid.sInterfaceName.c_str(), ui16MTU, nid.ui16InterfaceMTU);
                nid.ui16InterfaceMTU = ui16MTU;
            }
        }

        {
            // Try to retrieve the MAC address of the external network interface by querying the device itself
            const auto pszExternalMACAddress = spNetworkInterface->getMACAddr();
            if ((pszExternalMACAddress == nullptr) && (nid.emaInterfaceMACAddress == NOMADSUtil::EtherMACAddr{0})) {
                // Error
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_SevereError,
                                "could not determine the MAC address of the external network "
                                "interface with name %s\n", nid.sInterfaceName.c_str());
                return -5;
            }

            if (nid.emaInterfaceMACAddress == NOMADSUtil::EtherMACAddr{0}) {
                // Successfully queried the NIC, nothing specified via configuration
                nid.emaInterfaceMACAddress = buildEthernetMACAddressFromArray (pszExternalMACAddress);
                checkAndLogMsg ("main", NOMADSUtil::Logger::L_Info,
                                "retrieved the MAC address %s for the external network interface with name %s\n",
                                etherMACAddrToString (nid.emaInterfaceMACAddress).c_str(), nid.sInterfaceName.c_str());
            }
            else if (pszExternalMACAddress == 0) {
                // Use the value read from the configuration file
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Info,
                                "could not query the network interface with name %s for the MAC address; NetProxy will "
                                "use the address %s, specified in the configuration file\n", nid.sInterfaceName.c_str(),
                                etherMACAddrToString (nid.emaInterfaceMACAddress).c_str());
                spNetworkInterface->setMACAddress (nid.emaInterfaceMACAddress);
            }
            else if (buildEthernetMACAddressFromArray (pszExternalMACAddress) != nid.emaInterfaceMACAddress) {
                // The MAC address queried from the NIC does not match the MAC address provided in the configuration
                checkAndLogMsg ("PacketRouter::setupNetworkInterfaces", NOMADSUtil::Logger::L_Warning,
                                "the MAC address retrieved from the external network interface with name %s differs from "
                                "the one specified in the configuration file: retrieved MAC address is %s - configured "
                                "MAC address is %s; NetProxy will use the MTU retrieved from the network interface\n",
                                nid.sInterfaceName.c_str(), etherMACAddrToString (pszExternalMACAddress).c_str(),
                                etherMACAddrToString (nid.emaInterfaceMACAddress).c_str());
                nid.emaInterfaceMACAddress = buildEthernetMACAddressFromArray (pszExternalMACAddress);
            }
        }

        return 0;
    }

#if defined (USE_DISSERVICE)
    bool PacketRouter::dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pData,
                                    uint32 ui32Length, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
    {
        int rc;
        checkAndLogMsg ("PacketRouter::dataArrived", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "dataArrived: length = %d\n", (int) ui32Length);
        if (ui32Length < (sizeof(EtherFrameHeader) + sizeof(IPHeader) + sizeof(UDPHeader))) {
            checkAndLogMsg ("PacketRouter::dataArrived", NOMADSUtil::Logger::L_MildError,
                            "received a message that is smaller than a UDP packet - ignoring; size of message = %u; size must be at least %u\n",
                            ui32Length, sizeof(EtherFrameHeader) + sizeof(IPHeader) + sizeof(UDPHeader));
            return false;
        }
        else if (ui32Length > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
            checkAndLogMsg ("PacketRouter::dataArrived", NOMADSUtil::Logger::L_MildError,
                            "received a message of size %lu that is too large - maximum MTU is %lu\n",
                            ui32Length, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
        }
        uint8 ui8Buf[NetProxyApplicationParameters::PROXY_MESSAGE_MTU];
        memcpy (ui8Buf, pData, ui32Length);
        EtherFrameHeader *pEthHeader = (EtherFrameHeader*) ui8Buf;
        IPHeader *pIPHeader = (IPHeader*) (ui8Buf + getEthernetHeaderLength (pEthHeader));      /*!!*/ // Check if header for TUN/TAP is the size of an Ethernet header
        uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
        UDPHeader *pUDPHeader = (UDPHeader*) (((uint8*) pIPHeader) + ui16IPHeaderLen);
        checkAndLogMsg ("PacketRouter::dataArrived", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "sending UDP Packet: size %d from %d.%d.%d.%d:%d to %d.%d.%d.%d:%d\n",
                        (int) pUDPHeader->ui16Len - sizeof(UDPHeader), (int) pIPHeader->srcAddr.ui8Byte1,
                        (int) pIPHeader->srcAddr.ui8Byte2, (int) pIPHeader->srcAddr.ui8Byte3, (int) pIPHeader->srcAddr.ui8Byte4,
                        (int) pUDPHeader->ui16SPort, (int) pIPHeader->destAddr.ui8Byte1, (int) pIPHeader->destAddr.ui8Byte2,
                        (int) pIPHeader->destAddr.ui8Byte3, (int) pIPHeader->destAddr.ui8Byte4, (int) pUDPHeader->ui16DPort);
        hton (pEthHeader);
        pIPHeader->hton();
        pUDPHeader->hton();
        if (0 != (rc = sendPacketToHost (ui8Buf, ui32Length + sizeof(EtherFrameHeader)))) {
            checkAndLogMsg ("PacketRouter::dataArrived", NOMADSUtil::Logger::L_MildError,
                            "sendPacketToHost() failed with rc = %d\n", rc);
            return false;
        }

        return true;
    }

    bool PacketRouter::chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pChunk,
                                     uint32 ui32Length, uint8 ui8NChunks, uint8 ui8TotNChunks, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
    {
        return true;
    }

    bool PacketRouter::metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const void *pMetadata,
                                        uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
    {
        return true;
    }

    bool PacketRouter::dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName, uint32 ui32SeqId, const char * pszId,
                                      const void *pMetadata, uint32 ui32MetadataLength, uint16 ui16HistoryWindow, uint16 ui16Tag, uint8 ui8Priority)
    {
        return true;
    }

#endif

#if defined (USE_DISSERVICE)
    IHMC_ACI::DisseminationService * const PacketRouter::_pDisService = nullptr;
#endif
}
