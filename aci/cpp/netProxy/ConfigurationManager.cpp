/*
 * ConfigurationManager.cpp
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
    #include <io.h>
#else
    #if !defined (ANDROID)
        #include <sys/io.h>
    #endif
#endif

#include <climits>
#include <string>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <utility>

#include "NetworkHeaders.h"
#include "InetAddr.h"
#include "NLFLib.h"
#include "Logger.h"

#include "ConfigurationManager.h"
#include "ConnectionManager.h"
#include "PacketRouter.h"
#include "Utilities.h"

#if defined (WIN32)
    #define access _access
    #define strdup _strdup
#endif


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    int ConfigurationManager::EndpointConfigParameters::parse (char *pszParamsEntry)
    {
        if (pszParamsEntry == nullptr) {
            return -1;
        }

        std::transform (pszParamsEntry, pszParamsEntry + strlen (pszParamsEntry), pszParamsEntry, ::tolower);
        const auto vParams = splitStringToVector (pszParamsEntry, ';');
        for (auto sKeyValuePair : vParams) {
            const auto vProtocolPortPair = splitStringToVector (sKeyValuePair, '=');
            const auto sKey = vProtocolPortPair[0], sValue = vProtocolPortPair[1];
            if (sKey.length() == 0) {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                "no key found in the line of the config file\n");
                continue;
            }
            if (sValue.length() == 0) {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                "no value has been specified for the key %s in the line of the config file\n", sKey.c_str());
                continue;
            }
            if (sKey == "icmp") {
                if (!ProtocolSetting::isProtocolNameCorrect (sValue.c_str())) {
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                    "the specified protocol %s is not correct. The default (%s) will be used\n", sValue.c_str(),
                                    _psICMPProtocolSetting.getProxyMessageProtocolAsString());
                }
                else {
                    _psICMPProtocolSetting = ProtocolSetting (sValue.c_str());
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "protocol %s setted to map ICMP protocol\n", sValue.c_str());
                }
            }
            else if (sKey == "tcp") {
                if (!ProtocolSetting::isProtocolNameCorrect (sValue.c_str())) {
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                    "the specified protocol %s is not correct. The default (%s) will be used\n", sValue.c_str(),
                                    _psTCPProtocolSetting.getProxyMessageProtocolAsString());
                }
                else {
                    _psTCPProtocolSetting = ProtocolSetting (sValue.c_str());
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "protocol %s setted to map TCP protocol\n", sValue.c_str());
                }
            }
            else if (sKey == "udp") {
                if (!ProtocolSetting::isProtocolNameCorrect (sValue.c_str())) {
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                    "the specified protocol %s is not correct. The default (%s) will be used\n", sValue.c_str(),
                                    _psUDPProtocolSetting.getProxyMessageProtocolAsString());
                }
                else {
                    _psUDPProtocolSetting = ProtocolSetting (sValue.c_str());
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "protocol %s setted to map UDP protocol\n", sValue.c_str());
                }
            }
            else if (sKey == "tcpcompression") {
                _psTCPProtocolSetting.setCompressionSetting (readCompressionConfEntry (sValue.c_str()));
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                "compression algorithm <%s> with level %hhu will be used to compress TCP streams\n",
                                _psTCPProtocolSetting.getCompressionSetting().getCompressionTypeAsString(),
                                _psTCPProtocolSetting.getCompressionSetting().getCompressionLevel());
            }
            else if (sKey == "udpcompression") {
                _psUDPProtocolSetting.setCompressionSetting (readCompressionConfEntry (sValue.c_str()));
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                "compression algorithm <%s> with level %hhu will be used to compress UDP datagrams\n",
                                _psUDPProtocolSetting.getCompressionSetting().getCompressionTypeAsString(),
                                _psUDPProtocolSetting.getCompressionSetting().getCompressionLevel());
            }
            else if (sKey == "tcppriority") {
                _psTCPProtocolSetting.setPriorityLevel (readPriorityConfEntry (sValue.c_str()));
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                "TCP priority level set to %d\n", _psTCPProtocolSetting.getPriorityLevel());
            }
            else if (sKey == "tcpencryption") {
                EncryptionType etTCP = readEncryptionValue (sValue.c_str());
                if (etTCP == ET_UNDEF) {
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                    "Unable to parse the string %s to set the encryption protocol for "
                                    "remapped TCP connections; using PLAIN\n", sValue.c_str());
                    etTCP = ET_PLAIN;
                }
                _psTCPProtocolSetting.setEncryption (etTCP);
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                "Encryption protocol for remapped TCP connections set to %s\n",
                                encryptionTypeToString (_psTCPProtocolSetting.getEncryptionType()));
            }
            else if (sKey == "udpencryption") {
                EncryptionType etUDP = readEncryptionValue (sValue.c_str());
                if (etUDP == ET_UNDEF) {
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                    "Unable to parse the string %s to set the encryption protocol for "
                                    "remapped UDP connections; using PLAIN\n", sValue.c_str());
                    etUDP = ET_PLAIN;
                }
                _psUDPProtocolSetting.setEncryption (etUDP);
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                "Encryption protocol for remapped UDP traffic set to %s\n",
                                encryptionTypeToString (_psUDPProtocolSetting.getEncryptionType()));
            }
            else if (sKey == "icmpencryption") {
                EncryptionType etICMP = readEncryptionValue (sValue.c_str());
                if (etICMP == ET_UNDEF) {
                    checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                    "Unable to parse the string %s to set the encryption protocol for "
                                    "remapped ICMP connections; using PLAIN\n", sValue.c_str());
                    etICMP = ET_PLAIN;
                }
                _psICMPProtocolSetting.setEncryption (etICMP);
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_LowDetailDebug,
                                "Encryption protocol for remapped ICMP traffic set to %s\n",
                                encryptionTypeToString (_psUDPProtocolSetting.getEncryptionType()));
            }
            else {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::parse", NOMADSUtil::Logger::L_Warning,
                                "key value %s not recognized\n", sKey.c_str());
            }

        }

        return 0;
    }

    const uint8 ConfigurationManager::EndpointConfigParameters::readPriorityConfEntry (const char * const pcPriority)
    {
        int iPriorityLevel = ProtocolSetting::getDefaultTCPProtocolSetting()->getPriorityLevel();

        if (pcPriority) {
            iPriorityLevel = std::atoi (pcPriority);
            if (iPriorityLevel < 0) {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::readPriorityConfEntry", NOMADSUtil::Logger::L_Warning,
                                "%d is not a valid priority level for the Entry; the value 0 will be used instead\n", iPriorityLevel);
                iPriorityLevel = 0;
            }
            else if (iPriorityLevel > 9) {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigParameters::readPriorityConfEntry", NOMADSUtil::Logger::L_Warning,
                                "%d is not a valid priority level for the Entry; the value 9 will be used instead\n", iPriorityLevel);
                iPriorityLevel = 9;
            }
        }

        return static_cast<uint8> (iPriorityLevel);
    }

    const CompressionSettings ConfigurationManager::EndpointConfigParameters::readCompressionConfEntry (const char * const pcCompressionAlg)
    {
        // Format is <compression_name>:<compression_level>
        CompressionSettings compressionSetting;
        int iCompressionLevel = CompressionSettings::DEFAULT_COMPRESSION_LEVEL;
        auto st = splitStringToVector (pcCompressionAlg, ':');
        std::transform (st[0].begin(), st[0].end(), st[0].begin(), tolower);
        std::string sCompressionAlgName{st[0]};
        if (st.size() == 2) {
            iCompressionLevel = std::atoi (st[1].c_str());
            if (iCompressionLevel < 0) {
                iCompressionLevel = 0;
            }
            else if (iCompressionLevel > 9) {
                iCompressionLevel = 9;
            }
        }
        if (!CompressionSettings::isSpecifiedCompressionNameCorrect (sCompressionAlgName.c_str())) {
            checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::EndpointConfigParameters::readCompressionConfEntry", NOMADSUtil::Logger::L_Warning,
                            "specified an invalid compression algorithm (%s); no compression (plain) will be used\n", sCompressionAlgName.c_str());
        }
        else if ((sCompressionAlgName != "none") && (iCompressionLevel == 0)) {
            checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::EndpointConfigParameters::readCompressionConfEntry", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "specified a compression level of 0 (or less) with compression algorithm <%s>; no compression (plain) will be used instead\n",
                            sCompressionAlgName.c_str());
        }
        compressionSetting = CompressionSettings (sCompressionAlgName.c_str(), iCompressionLevel);

        return compressionSetting;
    }

    int ConfigurationManager::NetworkInterfaceDescriptionReader::init (const std::string & sConfigFilePath)
    {
        int rc;
        if (0 != (rc = ConfigManager::init())) {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::init", NOMADSUtil::Logger::L_MildError,
                            "init() of ConfigManager failed with rc =  %d\n", rc);
            return -1;
        }

        if (0 != (rc = ConfigManager::readConfigFile (sConfigFilePath.c_str(), true))) {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::init", NOMADSUtil::Logger::L_MildError,
                            "readConfigFile() of ConfigManager failed with rc = %d\n", rc);
            return -2;
        }

        return 0;
    }

    NetworkInterfaceDescriptor ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile (const std::vector<std::string> & vComplementaryNetworkInterfacesList)
    {
        NetworkInterfaceDescriptor nid;

        if (hasValue ("InterfaceName")) {
            // The name needs to match the one of the file
            std::string sInterfaceName = getValue ("InterfaceName");
            if (sInterfaceName != _sInterfaceName) {
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                                "the name specified in the interface description file %s does not match the one in the filename (%s); "
                                "NetProxy will ignore the entry specified inside the file and use the one in the filename\n",
                                sInterfaceName.c_str(), _sInterfaceName.c_str());
            }
        }
        nid.sInterfaceName = _sInterfaceName;

        uint32 ui32IPv4Address = 0;
        if (hasValue ("IPv4Address")) {
            // IP Address of the interface
            const std::string sIPv4Address = getValue ("IPv4Address");
            if ((sIPv4Address.length() > 0) && checkIPv4AddressFormat (sIPv4Address)) {
                NOMADSUtil::InetAddr iaInterface{sIPv4Address.c_str()};
                ui32IPv4Address = iaInterface.getIPAddress();
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Info,
                                "IPv4 Address of the interface with name %s set to %s\n",
                                nid.sInterfaceName.c_str(), iaInterface.getIPAsString());
            }
            else {
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                                "configured IPv4 Address of the interface with name %s has the wrong format; "
                                "the entry is %s\n", nid.sInterfaceName.c_str(), sIPv4Address.c_str());
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                            "no value found for the IPv4 address of the interface with name %s; "
                            "NetProxy will query it from the NIC\n", nid.sInterfaceName.c_str());
        }
        nid.ui32IPv4Address = ui32IPv4Address;

        uint32 ui32IPv4Netmask = 0;
        if (hasValue ("IPv4Netmask")) {
            // Network netmask of the interface
            const std::string sIPv4Netmask = getValue ("IPv4Netmask");
            if ((sIPv4Netmask.length() > 0) && checkIPv4AddressFormat (sIPv4Netmask)) {
                NOMADSUtil::InetAddr iaNetmask {sIPv4Netmask.c_str()};
                ui32IPv4Netmask = iaNetmask.getIPAddress();
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Info,
                                "Network IPv4 netmask of the interface with name %s set to %s\n",
                                nid.sInterfaceName.c_str(), iaNetmask.getIPAsString());
            }
            else {
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                                "configured IPv4 Netmask of the interface with name %s has the wrong format; "
                                "the entry is %s\n", nid.sInterfaceName.c_str(), sIPv4Netmask.c_str());
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                            "no value found for the IPv4 netmask of the interface with name %s; "
                            "NetProxy will query it from the NIC\n", nid.sInterfaceName.c_str());
        }
        nid.ui32IPv4NetMask = ui32IPv4Netmask;

        uint32 ui32InterfaceMTU = 0;
        if (hasValue ("MTU")) {
            // MTU for the interface
            const std::string sMTU = getValue ("MTU");
            if (sMTU.length() > 0) {
                ui32InterfaceMTU = std::stoul (sMTU);
                if (ui32InterfaceMTU > USHRT_MAX) {
                    checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "configured MTU of the interface with name %s is too big; the entry is %s\n",
                                    nid.sInterfaceName.c_str(), sMTU.c_str());
                    ui32InterfaceMTU = 0;
                }
                else {
                    checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Info,
                                    "MTU of the interface with name %s set to %u\n",
                                    nid.sInterfaceName.c_str(), ui32InterfaceMTU);
                }
            }
            else {
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                                "the MTU option of the interface with name %s has an empty value; "
                                "NetProxy will query it from the NIC\n", nid.sInterfaceName.c_str());
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                            "no value found for the MTU of the interface with name %s; "
                            "NetProxy will query it from the NIC\n", nid.sInterfaceName.c_str());
        }
        nid.ui16InterfaceMTU = static_cast<uint16> (ui32InterfaceMTU);

        uint32 ui32IPv4GatewayAddress = 0;
        if (hasValue ("GatewayIPv4Address")) {
            // IPv4 address of the default network gateway
            const std::string sIPv4GatewayAddress = getValue ("GatewayIPv4Address");
            if ((sIPv4GatewayAddress.length() > 0) && checkIPv4AddressFormat (sIPv4GatewayAddress)) {
                NOMADSUtil::InetAddr iaGatewayAddress {sIPv4GatewayAddress.c_str()};
                ui32IPv4GatewayAddress = iaGatewayAddress.getIPAddress();
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Info,
                                "IPv4 address of the default network gateway for the interface with name %s "
                                "set to %s\n", nid.sInterfaceName.c_str(), iaGatewayAddress.getIPAsString());
            }
            else {
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                                "configured IPv4 address of the default network gateway for the interface with name %s "
                                "has the wrong format; the entry is %s\n", nid.sInterfaceName.c_str(), sIPv4GatewayAddress.c_str());
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                            "no value found for the IPv4 address of the default network gateway for the interface "
                            "with name %s; NetProxy will query it from the NIC\n", nid.sInterfaceName.c_str());
        }
        nid.ui32IPv4GatewayAddress = ui32IPv4GatewayAddress;

        NOMADSUtil::EtherMACAddr etherMACAddress{};
        if (hasValue ("MACAddress")) {
            // Ethernet MAC address of the interface
            const std::string sEthernetMACAddress = getValue ("MACAddress");
            if ((sEthernetMACAddress.length() > 0) &&
                ((etherMACAddress = buildEthernetMACAddressFromString (sEthernetMACAddress.c_str())) != NOMADSUtil::EtherMACAddr{})) {
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Info,
                                "Ethernet MAC address of the interface with name %s set to %s\n",
                                nid.sInterfaceName.c_str(), etherMACAddrToString (etherMACAddress).c_str());
            }
            else {
                checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                                "could not parse the MAC address for the interface with name %s from entry <%s>; NetProxy "
                                "will query it from the NIC\n", nid.sInterfaceName.c_str(), sEthernetMACAddress.c_str());
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Warning,
                            "no option found for MAC address of the interface with name %s; "
                            "NetProxy will query it from the NIC\n", nid.sInterfaceName.c_str());
        }
        nid.emaInterfaceMACAddress = etherMACAddress;

        if (hasValue ("forwardMulticastPacketsOnInterfaces")) {
            const std::string sMCastPacketForwardingInterfaces = getValue ("forwardMulticastPacketsOnInterfaces");
            const auto vInterfaceNames = splitStringToVector (sMCastPacketForwardingInterfaces, ',',
                                                              [&vComplementaryNetworkInterfacesList] (const std::string & sInterfaceName)
            {
                return std::find (vComplementaryNetworkInterfacesList.cbegin(), vComplementaryNetworkInterfacesList.cend(),
                                  sInterfaceName) != vComplementaryNetworkInterfacesList.cend();
            });

            nid.usMulticastForwardingInterfacesList = std::unordered_set<std::string> {std::make_move_iterator (vInterfaceNames.begin()),
                                                                                       std::make_move_iterator (vInterfaceNames.end())};
        }
        if (nid.usMulticastForwardingInterfacesList.size() > 0) {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Info,
                            "interface <%s> configured to forward multicast packets on the following network interfaces: %s\n",
                            _sInterfaceName.c_str(), stringSetToString (nid.usMulticastForwardingInterfacesList).c_str());
        }

        if (hasValue ("forwardBroadcastPacketsOnInterfaces")) {
            const std::string sBCastPacketForwardingInterfaces = getValue ("forwardBroadcastPacketsOnInterfaces");
            const auto vInterfaceNames = splitStringToVector (sBCastPacketForwardingInterfaces, ',',
                                                              [&vComplementaryNetworkInterfacesList] (const std::string & sInterfaceName)
            {
                return std::find (vComplementaryNetworkInterfacesList.cbegin(), vComplementaryNetworkInterfacesList.cend(),
                                  sInterfaceName) != vComplementaryNetworkInterfacesList.cend();
            });

            nid.usBroadcastForwardingInterfacesList = std::unordered_set<std::string> {std::make_move_iterator (vInterfaceNames.begin()),
                                                                                       std::make_move_iterator (vInterfaceNames.end())};
        }
        if (nid.usBroadcastForwardingInterfacesList.size() > 0) {
            checkAndLogMsg ("ConfigurationManager::NetworkInterfaceDescriptionReader::parseConfigFile", NOMADSUtil::Logger::L_Info,
                            "interface <%s> configured to forward broadcast packets on the following network interfaces: %s\n",
                            _sInterfaceName.c_str(), stringSetToString (nid.usBroadcastForwardingInterfacesList).c_str());
        }

        return nid;
    }

    int ConfigurationManager::AddressMappingConfigFileReader::parseAddressMappingConfigFile (const char * const pszPathToConfigFile)
    {
        int rc;
        char szLineBuf[MAX_LINE_LENGTH];
        FILE *file = fopen (pszPathToConfigFile, "r");
        if (file == nullptr) {
            return -1;
        }
        while (nullptr != fgets (szLineBuf, MAX_LINE_LENGTH, file)) {
            size_t len = strlen (szLineBuf);
            if (len <= 0) {
                continue;
            }

            if (szLineBuf[len-1] != '\n') {
                if (!feof (file)) {
                    // Did not find \n so the buffer was not big enough!
                    fclose (file);
                    return -2;
                }
                else {
                    // End of file found - closing file and parsing last line; next while condition will be false
                    fclose (file);
                }
            }

            // Trimming line from spaces and comments
            len = ConfigurationManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // This line needs to be parsed
            if (0 != (rc = parseAndAddEntry (szLineBuf))) {
                checkAndLogMsg ("ConfigurationManager::AddressMappingConfigFileReader::parseAddressMappingConfigFile",
                                NOMADSUtil::Logger::L_Warning, "failed to parse line <%s>; rc = %d\n", szLineBuf, rc);
                fclose (file);
                return -3;
            }
        }
        fclose (file);

        // Add all parsed entries to the ConnectionManager
        if (enforceAddressMappingConsistency() < 0) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAddressMappingConfigFile",
                            NOMADSUtil::Logger::L_MildError, "inconsistencies found in the address mapping table; "
                            "please check the configuration and restart the NetProxy\n");
            return -4;
        }
        addAllAddressMappingEntriesToConnectionManager();
        _rConnectionManager.finalizeMappingRules();

        return 0;
    }

    void ConfigurationManager::AddressMappingConfigFileReader::addAllAddressMappingEntriesToConnectionManager (void) const
    {
        for (const auto & ppARDARDui32 : _remoteHostAddressMappings) {
            _rConnectionManager.addNewAddressMappingToBook (ppARDARDui32.first, ppARDARDui32.second);
        }
        for (const auto & ppARDARDui32 : _multiBroadCastAddressMappings) {
            _rConnectionManager.addNewAddressMappingToBook (ppARDARDui32.first, ppARDARDui32.second);
        }
    }

    int ConfigurationManager::AddressMappingConfigFileReader::parseAndAddEntry (const char *pszEntry)
    {
        if (pszEntry == nullptr) {
            return -1;
        }

        int rc;
        auto vsAddressMappingConfigurationLine = splitStringToVector (pszEntry, ' ', 3);
        if (vsAddressMappingConfigurationLine.size() < 2) {
            return -2;
        }
        std::string sSourceAddressRange{vsAddressMappingConfigurationLine[0]};
        std::string sDestinationAddressRange{vsAddressMappingConfigurationLine[1]};

        std::string sRemoteProxyMapping;
        if (vsAddressMappingConfigurationLine.size () < 3) {
            // The source address part of the mapping was not specified --> assume *.*.*.*:*
            if (sDestinationAddressRange.length() == 0) {
                // Not enough info provided
                checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                "error parsing the line <%s>; at least the destination address mapping and "
                                "the remote NetProxy IP address or Unique ID must be provided\n", pszEntry);
                return -3;
            }

            sRemoteProxyMapping = sDestinationAddressRange;
            sDestinationAddressRange = sSourceAddressRange;
            sSourceAddressRange = DEFAULT_SOURCE_ADDRESS_MAPPING;
        }
        else {
            sRemoteProxyMapping = {vsAddressMappingConfigurationLine[2]};
        }

        /* The constructor parses a range of IP addresses in the format <X.Y.W.Z:P> (without <>).
         * Any of the symbols X, Y, W, Z, and P can be represented as a range in the format A-B;
         * a range A-B will include all IPs (or ports) from A to B in that network.
         * For an IP to be a match for an entry of a range of addresses, the latter needs to include
         * all bytes of said IP address within all ranges specified in such entry. */
        NetworkAddressRangeDescriptor nardSource{sSourceAddressRange.c_str()};
        if (!nardSource.isValid()) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "impossible to parse the source address range %s correctly\n",
                            sSourceAddressRange.c_str());
            return -4;
        }
        NetworkAddressRangeDescriptor nardDestination{sDestinationAddressRange.c_str()};
        if (!nardDestination.isValid()) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "impossible to parse the destination address range %s correctly\n",
                            sDestinationAddressRange.c_str());
            return -5;
        }

        // Parsing remote NetProxy UniqueID and interface IPv4 address
        uint32 ui32RemoteProxyID = 0;
        NOMADSUtil::InetAddr iaLocalInterfaceIPv4Address, iaRemoteInterfaceIPv4Address;
        const auto vRemappingRuleInterfaces = splitStringToVector (sRemoteProxyMapping, ':');
        if (vRemappingRuleInterfaces.size() == 3) {
            // String is in the format <LocalInterfaceIP>:<RemoteNetProxyUniqueID>:<RemoteInterfaceIP/NAME>
            if (!checkIPv4AddressFormat (vRemappingRuleInterfaces[0])) {
                // Wrong IPv4 format for <LocalInterfaceIP>
                checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                "error parsing the IPv4 address of the local interface from the configuration string <%s>; "
                                "only the interface name of one of the configured external interfaces is acceptable\n",
                                vRemappingRuleInterfaces[0].c_str(), sRemoteProxyMapping.c_str());
                return -6;
            }
            iaLocalInterfaceIPv4Address.setIPAddress (vRemappingRuleInterfaces[0].c_str());
            ui32RemoteProxyID = (vRemappingRuleInterfaces[1].find ('.') != std::string::npos) ?
                NOMADSUtil::InetAddr{vRemappingRuleInterfaces[1].c_str()}.getIPAddress() : std::stoul (vRemappingRuleInterfaces[1]);
            auto * const rpi = _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32RemoteProxyID);
            if (rpi == nullptr) {
                // UniqueID not configured in the proxyUniqueIDs.cfg config file
                checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                "could not retrieve information about the remote NetProxy with UniqueID %u specified "
                                "in the configuration string <%s>\n", ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                return -7;
            }

            if (!checkIPv4AddressFormat (vRemappingRuleInterfaces[2])) {
                // Check if a label was used in place of a the IPv4 address of a remote interface
                iaRemoteInterfaceIPv4Address = rpi->getIPAddressOfInterfaceWithLabel (vRemappingRuleInterfaces[2]);
                if (iaRemoteInterfaceIPv4Address == NetProxyApplicationParameters::IA_INVALID_ADDR) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "impossible to find an interface with label %s for the remote NetProxy with UniqueID %u "
                                    "when parsing the configuration string <%s>\n", vRemappingRuleInterfaces[2].c_str(),
                                    ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                    return -8;
                }
            }
            else {
                iaRemoteInterfaceIPv4Address.setIPAddress (vRemappingRuleInterfaces[2].c_str());
                if (!rpi->hasInterfaceIPv4Address (iaRemoteInterfaceIPv4Address.getIPAddress())) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "impossible to find an interface with IPv4 address %s for the remote NetProxy with UniqueID %u "
                                    "when parsing the configuration string <%s>\n", iaRemoteInterfaceIPv4Address.getIPAsString(),
                                    ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                    return -9;
                }
            }
        }
        else if (vRemappingRuleInterfaces.size() == 2) {
            // <LocalInterfaceIP>:<UniqueID>, <LocalInterfaceIP>:<RemoteInterfaceIP>, or <UniqueID>:<RemoteInterfaceIP/NAME>
            if (!checkIPv4AddressFormat (vRemappingRuleInterfaces[1])) {
                // <LocalInterfaceIP>:<UniqueID> or <UniqueID>:<RemoteInterfaceNAME>
                if (!checkIPv4AddressFormat (vRemappingRuleInterfaces[0])) {
                    // <UniqueID>:<RemoteInterfaceNAME>
                    iaLocalInterfaceIPv4Address.setIPAddress (0UL);
                    ui32RemoteProxyID = std::stoul (vRemappingRuleInterfaces[0]);
                    auto * const rpi = _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32RemoteProxyID);
                    if (rpi == nullptr) {
                        checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                        "parsed UniqueID %u from the configuration string <%s> does not match any configured UniqueID\n",
                                        ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                        return -10;
                    }
                    iaRemoteInterfaceIPv4Address = rpi->getIPAddressOfInterfaceWithLabel (vRemappingRuleInterfaces[1]);
                    if (iaRemoteInterfaceIPv4Address == NetProxyApplicationParameters::IA_INVALID_ADDR) {
                        checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                        "the remote interface with label <%s> could be found for the remote NetProxy with UniqueID %u "
                                        "when parsing the configuration string <%s>\n", vRemappingRuleInterfaces[1].c_str(),
                                        ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                        return -11;
                    }
                }
                else {
                    // <LocalInterfaceIP>:<UniqueID>
                    iaLocalInterfaceIPv4Address.setIPAddress (vRemappingRuleInterfaces[0].c_str());
                    try {
                        ui32RemoteProxyID = std::stoul (vRemappingRuleInterfaces[1]);
                    }
                    catch (const std::exception & ex) {
                        checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                        "exception raised by stoul() when parsing the UniqueID from string %s in configuration string <%s>\n",
                                        vRemappingRuleInterfaces[1].c_str(), sRemoteProxyMapping.c_str());
                        return -12;
                    }
                    const auto * const rpi = _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32RemoteProxyID);
                    if (rpi == nullptr) {
                        checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                        "parsed UniqueID %u from the configuration string <%s> does not match any configured UniqueID\n",
                                        ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                        return -13;
                    }
                    iaRemoteInterfaceIPv4Address = rpi->getRemoteProxyMainInetAddr();
                }
            }
            else if (!checkIPv4AddressFormat (vRemappingRuleInterfaces[0])) {
                // <UniqueID>:<RemoteInterfaceIP>
                iaLocalInterfaceIPv4Address.setIPAddress (0UL);
                try {
                    ui32RemoteProxyID = std::stoul (vRemappingRuleInterfaces[0]);
                }
                catch (const std::exception & ex) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "exception raised by stoul() when parsing the UniqueID from string %s in configuration string <%s>\n",
                                    vRemappingRuleInterfaces[0].c_str(), sRemoteProxyMapping.c_str());
                    return -14;
                }
                const auto * const rpi = _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32RemoteProxyID);
                if (rpi == nullptr) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "parsed UniqueID %u from the configuration string <%s> does not match any configured remote NetProxy UniqueID\n",
                                    ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                    return -15;
                }
                iaRemoteInterfaceIPv4Address.setIPAddress (vRemappingRuleInterfaces[1].c_str());
                if (!rpi->hasInterfaceIPv4Address (iaRemoteInterfaceIPv4Address.getIPAddress())) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "a remote interface with IPv4 address %s was not configured for the remote NetProxy with UniqueID %u; "
                                    "error while parsing the configuration string <%s>\n", iaRemoteInterfaceIPv4Address.getIPAsString(),
                                    ui32RemoteProxyID, sRemoteProxyMapping.c_str());
                    return -16;
                }
            }
            else {
                // <LocalInterfaceIP>:<RemoteInterfaceIP>
                iaLocalInterfaceIPv4Address.setIPAddress (vRemappingRuleInterfaces[0].c_str());
                iaRemoteInterfaceIPv4Address.setIPAddress (vRemappingRuleInterfaces[1].c_str());
                ui32RemoteProxyID = _rConnectionManager.findUniqueIDOfRemoteNetProxyWithIPAddress (iaRemoteInterfaceIPv4Address.getIPAddress());
                if (ui32RemoteProxyID == 0) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "error parsing the configuration string <%s>; impossible to identify the UniqueID "
                                    "of the remote NetProxy with IPv4 address %s\n", sRemoteProxyMapping.c_str(),
                                    vRemappingRuleInterfaces[1].c_str());
                    return -17;
                }
            }
        }
        else if (vRemappingRuleInterfaces.size() == 1) {
            // <UniqueID> or <RemoteInterfaceIP>
            iaLocalInterfaceIPv4Address.setIPAddress (0UL);
            if (checkIPv4AddressFormat (vRemappingRuleInterfaces[0])) {
                // <RemoteInterfaceIP> --> retrieve the corresponding UniqueID
                iaRemoteInterfaceIPv4Address.setIPAddress (vRemappingRuleInterfaces[0].c_str());
                ui32RemoteProxyID = iaRemoteInterfaceIPv4Address.getIPAddress();
                auto * rpi = _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32RemoteProxyID);
                if (!rpi) {
                    // The UniqueID built from the interface address is not used --> look for the right one
                    ui32RemoteProxyID = _rConnectionManager.findUniqueIDOfRemoteNetProxyWithIPAddress (ui32RemoteProxyID);
                    if (ui32RemoteProxyID == 0) {
                        checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                        "impossible to identify the UniqueID of the remote NetProxy with IPv4 address <%s>\n",
                                        vRemappingRuleInterfaces[0].c_str());
                        return -18;
                    }
                    rpi = _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32RemoteProxyID);
                }
                if (!rpi->hasInterfaceIPv4Address (iaRemoteInterfaceIPv4Address.getIPAddress())) {
                    // The remote NPUI for which the parsed address was configured does not match the UniqueID obtained from the IPv4 address
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "error parsing the configuration string <%s>; the remote interface with IPv4 address %s "
                                    "was not configured for the remote NetProxy with UniqueID %u\n", sRemoteProxyMapping.c_str(),
                                    iaRemoteInterfaceIPv4Address.getIPAsString(), ui32RemoteProxyID);
                    return -19;
                }
            }
            else {
                // Only the UniqueID was specified --> retrieve the main IP address
                try {
                    ui32RemoteProxyID = std::stoul (vRemappingRuleInterfaces[0]);
                }
                catch (const std::exception & ex) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "exception raised by stoul() when parsing the UniqueID from string %s in configuration string <%s>\n",
                                    vRemappingRuleInterfaces[0].c_str(), sRemoteProxyMapping.c_str());
                    return -20;
                }
                auto pRemoteProxyInfo = _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32RemoteProxyID);
                if (pRemoteProxyInfo == nullptr) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "the specified UniqueID %u could not be found in the remoteProxyInfoTable\n", ui32RemoteProxyID);
                    return -21;
                }
                iaRemoteInterfaceIPv4Address = pRemoteProxyInfo->getRemoteProxyMainInetAddr();
                if (iaRemoteInterfaceIPv4Address == NetProxyApplicationParameters::IA_INVALID_ADDR) {
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "Unable to retrieve the main interface address for the NetProxy with UniqueID %u\n", ui32RemoteProxyID);
                    return -22;
                }
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "impossible to parse the remote NetProxy UniqueID and interface IPv4 address correctly from string <%s>; "
                            "the configuration line is <%s>\n", sRemoteProxyMapping.c_str(), pszEntry);
            return -23;
        }

        if (ui32RemoteProxyID == 0) {
            checkAndLogMsg ("ConfigurationManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "a wrong remote NetProxy UniqueID (%u) was parsed for line %s\n", ui32RemoteProxyID, pszEntry);
            return -24;
        }

        if ((rc = addAddressMappingToList (nardSource, nardDestination, ui32RemoteProxyID, iaLocalInterfaceIPv4Address, iaRemoteInterfaceIPv4Address)) < 0) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "impossible to add the new rule <%s>-<%s>/<%u>-<%s> to the Address Mapping Table; most likely, the new entry overlaps with "
                            "an existing mapping that points to a different NetProxy\n", nardSource.getAddressRangeStringDescription().c_str(),
                            nardDestination.getAddressRangeStringDescription().c_str(), ui32RemoteProxyID, iaRemoteInterfaceIPv4Address.getIPAsString());
            return -25;
        }
        else if (rc = 0) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "the new rule <%s>-<%s>/<%u>-<%s> was not added to the Address Mapping Table; most likely, a previously existing "
                            "mapping rule made the new entry redundant\n", nardSource.getAddressRangeStringDescription().c_str(),
                            nardDestination.getAddressRangeStringDescription().c_str(), ui32RemoteProxyID,
                            iaRemoteInterfaceIPv4Address.getIPAsString());
        }

        checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Info,
                        "successfully added the new rule <%s>-<%s>/<%u>-<%s> to the Address Mapping Table\n",
                        nardSource.getAddressRangeStringDescription().c_str(),
                        nardDestination.getAddressRangeStringDescription().c_str(),
                        ui32RemoteProxyID, iaRemoteInterfaceIPv4Address.getIPAsString());


        measure::Measure mAddressMappingMeasure;
        buildMeasure (mAddressMappingMeasure, nardSource, nardDestination, ui32RemoteProxyID, iaRemoteInterfaceIPv4Address);
        _rStatisticsManager.addAddressMappingRule (std::move (mAddressMappingMeasure));

        return 0;
    }

    int ConfigurationManager::AddressMappingConfigFileReader::addAddressMappingToList (const NetworkAddressRange & sourceAddressRangeDescriptor,
                                                                                       const NetworkAddressRange & destinationAddressRangeDescriptor,
                                                                                       uint32 ui32RemoteProxyID, const NOMADSUtil::InetAddr & iaLocalInterfaceIPv4Address,
                                                                                       const NOMADSUtil::InetAddr & iaRemoteInterfaceIPv4Address)
    {
        const auto mappingRule = std::make_tuple (ui32RemoteProxyID, iaLocalInterfaceIPv4Address, iaRemoteInterfaceIPv4Address);
        if (isMulticastAddressRange (destinationAddressRangeDescriptor)) {
            bool bIsMoreGeneral = false;
            for (auto it = _multiBroadCastAddressMappings.begin(); it != _multiBroadCastAddressMappings.end(); ) {
                // Check to see if the new rule is redundant compared to any other rule already parsed
                if (!bIsMoreGeneral && ((*it).second == mappingRule) &&
                    (*it).first.first.contains (sourceAddressRangeDescriptor) &&
                    (*it).first.second.contains (destinationAddressRangeDescriptor)) {
                    // The new rule is redundant --> it will not be added to the list
                    return 0;
                }

                if (((*it).second == mappingRule) && sourceAddressRangeDescriptor.contains ((*it).first.first) &&
                    destinationAddressRangeDescriptor.contains ((*it).first.second)) {
                    // The new rule will make an existing rule redundant --> switch that rule with the new one
                    if (bIsMoreGeneral) {
                        it = _multiBroadCastAddressMappings.erase (it);
                        continue;
                    }
                    else {
                        (*it).first = std::make_pair (sourceAddressRangeDescriptor, destinationAddressRangeDescriptor);
                        bIsMoreGeneral = true;
                    }
                }
                ++it;
            }
            if (!bIsMoreGeneral) {
                // Need to add the new rule
                _multiBroadCastAddressMappings.emplace_back (std::make_pair (sourceAddressRangeDescriptor, destinationAddressRangeDescriptor),
                                                             std::move (mappingRule));
            }

            return 1;
        }

        for (auto & ppARDARDui32 : _remoteHostAddressMappings) {
            /* Check to see if mappings overlaps. The NetProxy keeps two entries with the same mapping rule <sourceIP:RNPUID:destinationIP>,
             * even when the second entry is contained in the first one, because it might be a specialization of a specialization with
             * a different mapping rule and the NetProxy has no ways to know if that is the case until all mapping rules have been added. */
            if (sourceAddressRangeDescriptor.overlaps (ppARDARDui32.first.first) &&
                destinationAddressRangeDescriptor.overlaps (ppARDARDui32.first.second) &&
                (ppARDARDui32.second != mappingRule)) {
                if (addressMappingIsAValidSpecializationOfMapping (sourceAddressRangeDescriptor, destinationAddressRangeDescriptor,
                                                                   ppARDARDui32.first.first, ppARDARDui32.first.second) ||
                    addressMappingIsAValidSpecializationOfMapping (ppARDARDui32.first.first, ppARDARDui32.first.second,
                                                                   sourceAddressRangeDescriptor, destinationAddressRangeDescriptor)) {
                    // The new entry is a specialization of this mapping, or vice-versa --> keep both
                    continue;
                }

                // The address mappings overlap, but neither is a specification of the other --> inconsistent rules!
                return -1;
            }
        }

        _remoteHostAddressMappings.emplace_back (std::make_pair (sourceAddressRangeDescriptor, destinationAddressRangeDescriptor),
                                                 std::move (mappingRule));
        return 1;
    }

    /* This method enforces consistency in the mapping rules specified in the configuration file.
     * It returns:
     *  0 in case no work was done;
     *  1 in case some work was done. This method needs to be invoked again until it returns 0;
     *  -1 in case of error.
     */
    int ConfigurationManager::AddressMappingConfigFileReader::enforceAddressMappingConsistencyImpl (void)
    {
        for (auto extIt = _remoteHostAddressMappings.begin(); extIt != _remoteHostAddressMappings.end(); ++extIt) {
            for (auto intIt = _remoteHostAddressMappings.begin(); intIt != _remoteHostAddressMappings.end(); ++intIt) {
                if (extIt == intIt) {
                    continue;
                }

                // Check for redundant rules
                if ((*extIt).second == (*intIt).second) {
                    /* If one entry contains the other and the more specialized one is not a specialization of another rule
                     * that points to a different NetProxy, then the specialized rule is redundant and it can be removed. */
                    if ((*extIt).first.first.contains ((*intIt).first.first) && (*extIt).first.second.contains ((*intIt).first.second)) {
                        // Either the entry pointed by intIt further specializes another rule, or it can be removed
                        bool bRemove = true;
                        for (auto compIt = _remoteHostAddressMappings.begin(); compIt != _remoteHostAddressMappings.end(); ++compIt) {
                            if ((*compIt).second == (*intIt).second) {
                                // This check also accounts for the cases (compIt == intIt) and (compIt == extIt)
                                continue;
                            }

                            // Check if the rule pointed by intIt specializes the rule pointed by compIt
                            if (addressMappingIsAValidSpecializationOfMapping ((*compIt).first.first, (*compIt).first.second,
                                                                               (*intIt).first.first, (*intIt).first.second)) {
                                bRemove = false;
                                break;
                            }
                        }

                        if (bRemove) {
                            // Remove redundant entry and return 1 to indicate that some work was done
                            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                            "Removed redundant address mapping rule <%s>:<%s> --> <%u:%s:%s>\n",
                                            NetworkAddressRangeDescriptor{intIt->first.first}.getAddressRangeStringDescription().c_str(),
                                            NetworkAddressRangeDescriptor{intIt->first.second}.getAddressRangeStringDescription().c_str(),
                                            std::get<0> (intIt->second), std::get<1> (intIt->second).getIPAsString(),
                                            std::get<2> (intIt->second).getIPAsString());
                            _remoteHostAddressMappings.erase (intIt);
                            return 1;
                        }
                    }
                }
            }
        }

        return 0;
    }

    bool ConfigurationManager::AddressMappingConfigFileReader::addressMappingIsAValidSpecializationOfMapping (const NetworkAddressRange & ardSourceGeneral,
                                                                                                              const NetworkAddressRange & ardDestinationGeneral,
                                                                                                              const NetworkAddressRange & ardSourceSpecialization,
                                                                                                              const NetworkAddressRange & ardDestinationSpecialization)
    {
        return ardSourceGeneral.contains (ardSourceSpecialization) && ardDestinationGeneral.contains (ardDestinationSpecialization) &&
            (ardSourceGeneral.getNumberOfAddressesInRange() >= ardSourceSpecialization.getNumberOfAddressesInRange()) &&
            (ardDestinationGeneral.getNumberOfAddressesInRange() >= ardDestinationSpecialization.getNumberOfAddressesInRange()) &&
            ((ardSourceGeneral.getNumberOfAddressesInRange() > ardSourceSpecialization.getNumberOfAddressesInRange()) ||
            (ardDestinationGeneral.getNumberOfAddressesInRange() > ardDestinationSpecialization.getNumberOfAddressesInRange()));
    }

    void ConfigurationManager::AddressMappingConfigFileReader::buildMeasure (measure::Measure & m, const NetworkAddressRangeDescriptor & nardSource,
                                                                             const NetworkAddressRangeDescriptor & nardDestination, uint32 ui32RemoteProxyUID,
                                                                             const NOMADSUtil::InetAddr & iaRemoteInterfaceIPv4Address)
    {
        m.set_subject (measure::Subject::netproxy_addr_mapping);

        m.mutable_timestamp()->set_seconds (0);
        m.mutable_timestamp()->set_nanos (0);

        auto & string_map = *m.mutable_strings();
        string_map["sensor_ip"] = NOMADSUtil::InetAddr(NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST[0].ui32IPv4Address).getIPAsString();
        string_map["np_uid"] = std::to_string (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);

        auto src_addr_mapping = splitIPAndPort (nardSource.getAddressRangeStringDescription().c_str());
        string_map["src_ip_mapping"] = src_addr_mapping.first;
        string_map["src_port_mapping"] = src_addr_mapping.second;

        auto dst_addr_mapping = splitIPAndPort (nardDestination.getAddressRangeStringDescription().c_str());
        string_map["dst_ip_mapping"] = dst_addr_mapping.first;
        string_map["dst_port_mapping"] = dst_addr_mapping.second;

        string_map["remote_np_uid"] = std::to_string (ui32RemoteProxyUID);
        string_map["remote_np_iface_ip"] = iaRemoteInterfaceIPv4Address.getIPAsString();
    }

    int ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::parseLine (const char *pszEntry)
    {
        if (pszEntry == nullptr) {
            return -1;
        }
        char *pszEntryDup = strdup (pszEntry);
        char *pszSource = pszEntryDup;
        char *pszTemp = strchr (pszEntryDup, ' ');
        if (pszTemp == nullptr) {
            return -2;
        }
        *pszTemp = '\0';
        char *pszDest = pszTemp + 1;
        pszTemp = strchr (pszDest, ' ' );
        if (pszTemp == nullptr) {
            return -3;
        }
        *pszTemp = '\0';
        char *pszConfigParams = pszTemp + 1;
        _nardSource = NetworkAddressRangeDescriptor{pszSource};
        if (!_nardSource.isValid()) {
            return -4;
        }
        checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::parseLine", NOMADSUtil::Logger::L_Info,
                        "parsed source Endpoint (source): lowest ip and port = %s, highest ip and port = %s\n",
                        _nardSource.getLowestAddressAsString().c_str(), _nardSource.getHighestAddressAsString().c_str());

        _nardDestination = NetworkAddressRangeDescriptor{pszDest};
        if (!_nardDestination.isValid()) {
            return -5;
        }
        checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::parseLine", NOMADSUtil::Logger::L_Info,
                        "parsed destination Endpoint (destination): lowest ip and port = %s, highest ip and port = %s\n",
                        _nardDestination.getLowestAddressAsString().c_str(),_nardDestination.getHighestAddressAsString().c_str());

        if (_configParams.parse (pszConfigParams)) {
            return -6;
        }
        const CompressionSettings tcpCompressionSetting = _configParams.getCompressionSetting (NOMADSUtil::IP_PROTO_TCP);
        const CompressionSettings udpCompressionSetting = _configParams.getCompressionSetting (NOMADSUtil::IP_PROTO_UDP);
        if (tcpCompressionSetting.getCompressionType() == CompressionType::PMC_UncompressedData) {
            if (udpCompressionSetting.getCompressionType() == CompressionType::PMC_UncompressedData) {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::parseLine", NOMADSUtil::Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s>; UDP compression algorithm = <%s>\n",
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), udpCompressionSetting.getCompressionTypeAsString());
            }
            else {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::parseLine", NOMADSUtil::Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s>; UDP compression algorithm = <%s> - level %hhu\n",
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), udpCompressionSetting.getCompressionTypeAsString(),
                                udpCompressionSetting.getCompressionLevel());
            }
        }
        else {
            if (udpCompressionSetting.getCompressionType() == CompressionType::PMC_UncompressedData) {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::parseLine", NOMADSUtil::Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s> - level %hhu; UDP compression algorithm = <%s>\n",
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), tcpCompressionSetting.getCompressionLevel(),
                                udpCompressionSetting.getCompressionTypeAsString());
            }
            else {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigEntry::parseLine", NOMADSUtil::Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s> - level %hhu; UDP compression algorithm = <%s> - level %hhu\n",
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (NOMADSUtil::IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), tcpCompressionSetting.getCompressionLevel(),
                                udpCompressionSetting.getCompressionTypeAsString(), udpCompressionSetting.getCompressionLevel());
            }
        }
        free (pszEntryDup);

        return 0;
    }

    int ConfigurationManager::EndpointConfigFileReader::parseEndpointConfigFile (const char * const pszPathToConfigFile)
    {
        int rc;
        char szLineBuf[MAX_LINE_LENGTH];
        FILE * file = fopen (pszPathToConfigFile, "r");
        if (file == nullptr) {
            return -1;
        }
        while (nullptr != fgets (szLineBuf, MAX_LINE_LENGTH, file)) {
            size_t len = strlen (szLineBuf);
            if (len <= 0) {
                continue;
            }

            if (szLineBuf[len-1] != '\n') {
                if (!feof (file)) {
                    // Did not find \n so the buffer was not big enough!
                    fclose (file);
                    return -2;
                }
                else {
                    // End of file found - closing file and parsing last line; next while condition will be false
                    fclose (file);
                }
            }

            // Trimming line from spaces and comments
            len = ConfigurationManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // This line needs to be parsed
            EndpointConfigEntry epce{};
            epce.parseLine (szLineBuf);
            if (0 != (rc = epce.parseLine (szLineBuf))) {
                checkAndLogMsg ("ConfigurationManager::EndpointConfigFileReader::parseConfigFile", NOMADSUtil::Logger::L_MildError,
                                "failed to parse line <%s>\n", szLineBuf);
                fclose (file);
                return -3;
            }
            _vEndpointConfigTable.push_back (epce);

            // Build protocol mapping rule measure
            measure::Measure mProtocolMappingRule;
            buildMeasure (mProtocolMappingRule, epce);
            _rStatisticsManager.addProtocolMappingRule (std::move (mProtocolMappingRule));
        }

        fclose (file);
        return 0;
    }

    void ConfigurationManager::EndpointConfigFileReader::buildMeasure (measure::Measure & m, const EndpointConfigEntry & entry)
    {
        m.set_subject (measure::Subject::netproxy_proto_mapping);

        m.mutable_timestamp()->set_seconds (0);
        m.mutable_timestamp()->set_nanos (0);

        auto & string_map = *m.mutable_strings();
        string_map["sensor_ip"] = NOMADSUtil::InetAddr(NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST[0].ui32IPv4Address).getIPAsString();
        string_map["np_uid"] = std::to_string (NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);

        auto src_addr_mapping = splitIPAndPort (entry.getNARDSource().getAddressRangeStringDescription().c_str());
        string_map["src_ip_mapping"] = src_addr_mapping.first;
        string_map["src_port_mapping"] = src_addr_mapping.second;

        auto dst_addr_mapping = splitIPAndPort (entry.getNARDDestination().getAddressRangeStringDescription().c_str());
        string_map["dst_ip_mapping"] = dst_addr_mapping.first;
        string_map["dst_port_mapping"] = dst_addr_mapping.second;

        string_map["tcp"] = entry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_TCP).getProxyMessageProtocolAsString();
        string_map["udp"] = entry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_UDP).getProxyMessageProtocolAsString();
        string_map["icmp"] = entry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_ICMP).getProxyMessageProtocolAsString();

        string_map["tcp_compression"] = entry.getConfigParams()->getCompressionSetting (NOMADSUtil::IP_PROTO_TCP).getCompressionTypeAsString();
        string_map["udp_compression"] = entry.getConfigParams()->getCompressionSetting (NOMADSUtil::IP_PROTO_UDP).getCompressionTypeAsString();

        string_map["tcp_encryption"] = encryptionTypeToString (entry.getConfigParams()->getEncryptionType (NOMADSUtil::IP_PROTO_TCP));
        string_map["udp_encryption"] = encryptionTypeToString (entry.getConfigParams()->getEncryptionType (NOMADSUtil::IP_PROTO_UDP));
        string_map["icmp_encryption"] = encryptionTypeToString (entry.getConfigParams()->getEncryptionType (NOMADSUtil::IP_PROTO_ICMP));
    }

    int ConfigurationManager::UniqueIDsConfigFileReader::parseUniqueIDsConfigFile (const char * const pszPathToConfigFile)
    {
        int rc;
        char szLineBuf[MAX_LINE_LENGTH];
        FILE * file = fopen (pszPathToConfigFile, "r");
        if (file == nullptr) {
            return -1;
        }
        while (nullptr != fgets (szLineBuf, MAX_LINE_LENGTH, file)) {
            size_t len = strlen (szLineBuf);
            if (len <= 0) {
                continue;
            }

            if (szLineBuf[len-1] != '\n') {
                if (!feof (file)) {
                    // Did not find \n so the buffer was not big enough!
                    fclose (file);
                    return -2;
                }
                else {
                    // End of file found - closing file and parsing last line; next while condition will be false
                    fclose (file);
                }
            }

            // Trimming line from spaces and comments
            len = ConfigurationManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // This line needs to be parsed
            if (0 != (rc = parseAndAddEntry (szLineBuf))) {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseUniqueIDsConfigFile", NOMADSUtil::Logger::L_Warning,
                                "failed to parse line <%s>; rc = %d\n", szLineBuf, rc);
                fclose (file);
                return -3;
            }
        }
        fclose (file);

        return 0;
    }

    int ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry (const char * pszEntry)
    {
        if (pszEntry == nullptr) {
            return -1;
        }

        uint32 ui32UniqueID{0};
        auto vsConfigurationLine = splitStringToVector (pszEntry, ' ');
        auto iConfigurationOptions = vsConfigurationLine.begin();
        std::string sProxyUniqueID{*iConfigurationOptions++};
        std::string sIPAddressesList{""};
        if ((*iConfigurationOptions).find ("=") == std::string::npos) {
            if (iConfigurationOptions != vsConfigurationLine.end()) {
                // Both the UniqueID and the list of IPv4 addresses of the remote NetProxy's interfaces are specified
                sIPAddressesList = *iConfigurationOptions++;
            }

            // Parse NetProxy UniqueID
            if (sProxyUniqueID.find (".") == std::string::npos) {
                // A NetProxy UniqueID in the integer form was specified
                try {
                    ui32UniqueID = std::stoul (sProxyUniqueID);
                }
                catch (const std::exception & ex) {
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "failed to parse the parameter <%s> in line <%s>; a remote NetProxy UniqueID or a "
                                    "list of IP addresses is expected; error message: <%s>\n", sProxyUniqueID.c_str(),
                                    pszEntry, ex.what());
                    return -2;
                }
            }
            else {
                // A NetProxy UniqueID in the IPv4 address form was specified
                ui32UniqueID = NOMADSUtil::InetAddr{sProxyUniqueID.c_str()}.getIPAddress();
            }
        }
        else {
            sIPAddressesList = sProxyUniqueID;
        }

        auto vIPAddressesStringList = splitStringToVector (sIPAddressesList, ',');
        vIPAddressesStringList.erase (std::remove_if (vIPAddressesStringList.begin(), vIPAddressesStringList.end(),
                                                      [](const std::string & s) { return !checkIPv4AddressFormat (s.substr (0, s.find ('/'))); }),
                                      vIPAddressesStringList.end());
        std::vector<NOMADSUtil::InetAddr> viaList;
        std::vector<std::string> vsInterfaceLabels;
        std::transform (vIPAddressesStringList.cbegin(), vIPAddressesStringList.cend(), std::back_inserter (viaList),
                        [](const std::string & sIPv4Address) {
                            return NOMADSUtil::InetAddr{sIPv4Address.substr (0, sIPv4Address.find ("/")).c_str()};
                        });
        std::transform (vIPAddressesStringList.cbegin(), vIPAddressesStringList.cend(), std::back_inserter (vsInterfaceLabels),
                        [](const std::string & sIPv4Address) {
                            return (sIPv4Address.find ("/") == std::string::npos) ? "" : sIPv4Address.substr (sIPv4Address.find ("/") + 1);
                        });
        if (viaList.size() == 0) {
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "failed to parse the list of IP addresses <%s> in line <%s>\n",
                            sIPAddressesList.c_str(), pszEntry);
            return -3;
        }
        else if (viaList.size() < vIPAddressesStringList.size()) {
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "failed to parse some of the IP addresses in the list <%s> in line <%s>\n",
                            sIPAddressesList.c_str(), pszEntry);
        }
        std::vector<std::string> vsInterfaceLabelsCopy{vsInterfaceLabels};
        std::sort (vsInterfaceLabelsCopy.begin(), vsInterfaceLabelsCopy.end());
        if (std::unique (vsInterfaceLabelsCopy.begin(), vsInterfaceLabelsCopy.end(), [](const std::string & s1, const std::string & s2)
                                                                                     { return (s1 == s2) && s1.length() > 0; })
            != vsInterfaceLabelsCopy.end()) {
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "error parsing line <%s>; duplicate interface labels were found\n", pszEntry);
            return -4;
        }

        ui32UniqueID = (ui32UniqueID == 0) ? viaList[0].getIPAddress() : ui32UniqueID;
        RemoteProxyInfo remoteProxyInfo{ui32UniqueID, viaList, vsInterfaceLabels, NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE.c_str()};
        // The next element in the line could be either an option (e.g., MocketsPort=8751) or nothing.
        while (iConfigurationOptions != vsConfigurationLine.end()) {
            std::transform (iConfigurationOptions->begin(), iConfigurationOptions->end(), iConfigurationOptions->begin(), tolower);
            auto stProtocolPortPair = splitStringToVector (*iConfigurationOptions, '=');
            if (stProtocolPortPair.size() == 2) {
                if (updateInfoWithKeyValuePair (remoteProxyInfo, stProtocolPortPair[0], stProtocolPortPair[1]) < 0) {
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                    "error encountered while trying to parsing line <%s> in the config file\n", pszEntry);
                    return -5;
                }
            }
            else {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                                "error encountered while trying to parse line <%s> in the config file; one option is missing '='\n", pszEntry);
                return -6;
            }
            ++iConfigurationOptions;
        }
        if (!remoteProxyInfo.isLocalProxyReachableFromRemote() && !remoteProxyInfo.isRemoteProxyReachableFromLocalHost()) {
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "both local and remote reachability options for NetProxy with ID %u are set to false!\n",
                            remoteProxyInfo.getRemoteNetProxyID());
        }

        if (_rConnectionManager.addNewRemoteProxyInfo (std::move (remoteProxyInfo)) > 0) {
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "another entry for the remote NetProxy with UniqueID %u was already in the table; "
                            "the NetProxy has overwritten the existing entry with the new one\n", ui32UniqueID);
        }

        checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Info,
                        "successfully completed the parsing of a new line for the NetProxy with UniqueID <%u>; "
                        "the corresponding RemoteProxyInfo object is the following: %s\n", ui32UniqueID,
                        _rConnectionManager.getRemoteProxyInfoForNetProxyWithID (ui32UniqueID)->toString().c_str());

        return 0;
    }

    int ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair (RemoteProxyInfo & remoteProxyInfo, const std::string & sKey,
                                                                                     const std::string & sValue)
    {
        if ((sKey.size() == 0) || (sValue.size() == 0)) {
            return -1;
        }

        if (sKey == "mocketsport") {
            int iPortNumber = std::atoi (sValue.c_str());
            if ((iPortNumber <= 0) || (iPortNumber > 65535)) {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                "impossible to correctly parse Mockets port number <%s>\n", sValue.c_str());
                return -2;
            }
            remoteProxyInfo.setRemoteServerPort (CT_MOCKETS, (uint16) iPortNumber);
        }
        else if (sKey == "tcpport") {
            int iPortNumber = std::atoi (sValue.c_str());
            if ((iPortNumber <= 0) || (iPortNumber > 65535)) {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                "impossible to correctly parse TCP port number <%s>\n", sValue.c_str());
                return -3;
            }
            remoteProxyInfo.setRemoteServerPort (CT_TCPSOCKET, (uint16) iPortNumber);
        }
        else if (sKey == "udpport") {
            int iPortNumber = std::atoi (sValue.c_str());
            if ((iPortNumber <= 0) || (iPortNumber > 65535)) {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                "impossible to correctly parse UDP port number <%s>\n", sValue.c_str());
                return -4;
            }
            remoteProxyInfo.setRemoteServerPort (CT_UDPSOCKET, (uint16) iPortNumber);
        }
        else if (sKey == "autoconnect") {
            if (!remoteProxyInfo.isRemoteProxyReachableFromLocalHost()) {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                "impossible to add an autoConnection entry for the remote NetProxy with UniqueID %u and main "
                                "address %s because its connectivity is set to PASSIVE\n", remoteProxyInfo.getRemoteNetProxyID(),
                                remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                return 0;
            }

            const auto vsPerProtocolAutoConnections = splitStringToVector (sValue, ';');
            for (const auto & sProtocolAutoConnection : vsPerProtocolAutoConnections) {
                if (sProtocolAutoConnection.length() == 0) {
                    continue;
                }
                const auto vsProtocolAndIPv4Addresses = splitStringToVector (sProtocolAutoConnection, ',');
                if (!isConnectorTypeNameCorrect (vsProtocolAndIPv4Addresses[0].c_str())) {
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                    "specified value <%s> is not a valid protocol for AutoConnections to the NetProxy "
                                    "with UniqueID %u and main address %s\n", vsProtocolAndIPv4Addresses[0].c_str(),
                                    remoteProxyInfo.getRemoteNetProxyID(), remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                    return -5;
                }

                ConnectorType connectorType = connectorTypeFromString (vsProtocolAndIPv4Addresses[0].c_str());
                if (connectorType == CT_UDPSOCKET) {
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                    "impossible to set UDP as the protocol over which performing "
                                    "the connection to the remote NetProxy; ignoring entry\n");
                    continue;
                }
                else if (connectorType == CT_UNDEF) {
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                    "%s is not a valid protocol type; ignoring entry\n", vsProtocolAndIPv4Addresses[0].c_str());
                    continue;
                }

                bool bSpecifiedAddress{false};
                for (int i = 1; i < vsProtocolAndIPv4Addresses.size(); ++i) {
                    NOMADSUtil::InetAddr iaLocalInterfaceAddress, iaRemoteInterfaceAddress;
                    const auto vsLocalRemoteInterfaceAddresses = splitStringToVector (vsProtocolAndIPv4Addresses[i], ':');
                    if (vsLocalRemoteInterfaceAddresses.size() == 2) {
                        // Both local and remote interface addresses are given
                        if (!checkIPv4AddressFormat (vsLocalRemoteInterfaceAddresses[0]) ||
                            !checkIPv4AddressFormat (vsLocalRemoteInterfaceAddresses[1])) {
                            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                            "the specified IPv4 source/destination address pair <%s>, for AutoConnections to the remote NetProxy with UniqueID %u "
                                            "and main address %s, are not expressed in a valid format; skipping address\n", vsProtocolAndIPv4Addresses[i].c_str(),
                                            remoteProxyInfo.getRemoteNetProxyID(), remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                            continue;
                        }
                        iaLocalInterfaceAddress.setIPAddress (vsLocalRemoteInterfaceAddresses[0].c_str());
                        iaRemoteInterfaceAddress.setIPAddress (vsLocalRemoteInterfaceAddresses[1].c_str());
                        iaRemoteInterfaceAddress.setPort (remoteProxyInfo.getRemoteServerInetAddrForIPv4AddressAndConnectorType (iaRemoteInterfaceAddress.getIPAddress(),
                                                                                                                                 connectorType).getPort());
                    }
                    else {
                        // Only the remote interface address is given
                        if (!checkIPv4AddressFormat (vsLocalRemoteInterfaceAddresses[0])) {
                            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                            "the specified IPv4 address <%s> for AutoConnections to the remote NetProxy with UniqueID %u and main address %s "
                                            "is not expressed in a valid format; skipping address\n", vsProtocolAndIPv4Addresses[i].c_str(),
                                            remoteProxyInfo.getRemoteNetProxyID(), remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                            continue;
                        }
                        iaLocalInterfaceAddress.setIPAddress (0UL);
                        iaRemoteInterfaceAddress.setIPAddress (vsLocalRemoteInterfaceAddresses[0].c_str());
                        iaRemoteInterfaceAddress.setPort (remoteProxyInfo.getRemoteServerInetAddrForIPv4AddressAndConnectorType (iaRemoteInterfaceAddress.getIPAddress(),
                                                                                                                                 connectorType).getPort());
                    }

                    if (iaRemoteInterfaceAddress.getPort() == 0) {
                        // Remote server port not specified
                        checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                        "the server port number for connections using the %s protocol is not specified; it is impossible to correctly "
                                        "configure the AutoConnection entry to reach the remote NetProxy with UniqueID %u and main IPv4 address %s on "
                                        "the addresses <%s:%hu>\n", connectorTypeToString (connectorType), remoteProxyInfo.getRemoteNetProxyID(),
                                        remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString(), iaRemoteInterfaceAddress.getIPAsString(),
                                        iaRemoteInterfaceAddress.getPort());
                        return -6;
                    }

                    if (remoteProxyInfo.hasInterfaceIPv4Address (iaRemoteInterfaceAddress.getIPAddress())) {
                        bSpecifiedAddress = true;
                        AutoConnectionEntry autoConnectionEntry{remoteProxyInfo.getRemoteNetProxyID(), iaLocalInterfaceAddress, iaRemoteInterfaceAddress, connectorType};
                        if (_rConnectionManager.addOrUpdateAutoConnectionToList (autoConnectionEntry) > 0) {
                            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Info,
                                            "successfully added a new entry to the AutoConnection Table for the NetProxy with UniqueID %u "
                                            "and remote IPv4 address %s using protocol %s and a connection attempt timeout of %ums\n",
                                            remoteProxyInfo.getRemoteNetProxyID(), iaRemoteInterfaceAddress.getIPAsString(),
                                            connectorTypeToString (autoConnectionEntry.getConnectorType()),
                                            autoConnectionEntry.getAutoReconnectTimeInMillis());
                        }
                        else {
                            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Info,
                                            "successfully updated the AutoConnection to the NetProxy with UniqueID %u and remote IPv4 address %s "
                                            "to use protocol %s\n", remoteProxyInfo.getRemoteNetProxyID(), iaRemoteInterfaceAddress.getIPAsString(),
                                            connectorTypeToString (autoConnectionEntry.getConnectorType()));
                        }
                    }
                    else {
                        // Specified autoConnection remote IPv4 address not configured for the remote NetProxy
                            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                            "the specified IPv4 address <%s> for AutoConnections to the remote NetProxy with UniqueID %u and main address %s "
                                            "is not configured for that NetProxy; skipping address\n", vsProtocolAndIPv4Addresses[i].c_str(),
                                            remoteProxyInfo.getRemoteNetProxyID(), remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                    }
                }

                if (!bSpecifiedAddress) {
                    // None of the configured AutoConnection entries have a remote address that matches one of the addresses assigned to the remote NetProxy
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_MildError,
                                    "impossible to configure any AutoConnection entry for the remote NetProxy with UniqueID %u and ConnectorType %s; "
                                    "none of the specified remote addresses match any of the addresses assigned to the remote NetProxy\n",
                                    remoteProxyInfo.getRemoteNetProxyID(), connectorTypeToString (connectorType));
                    return -7;
                }
            }
        }
        else if (sKey == "reconnectinterval") {
            uint32 ui32ReconnectTime = NOMADSUtil::atoui32 (sValue.c_str());
            if (ui32ReconnectTime == 0) {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                "impossible to set value <%s> as AutoConnection timeout\n", sValue.c_str());
                return 0;
            }

            const auto usAutoConnections =
                _rConnectionManager.getSetOfAutoConnectionEntriesToRemoteProxyWithID (remoteProxyInfo.getRemoteNetProxyID());
            for (auto & spAutoConnectionEntry : usAutoConnections) {
                if (spAutoConnectionEntry) {
                    spAutoConnectionEntry->setAutoReconnectTime (ui32ReconnectTime);
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Info,
                                    "successfully updated the timeout of the AutoConnection to the NetProxy with "
                                    "UniqueID %u and main address %s to %ums\n", remoteProxyInfo.getRemoteNetProxyID(),
                                    remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString(), ui32ReconnectTime);
                }
                else {
                    checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Info,
                                    "found a valid AutoConnection Timeout value, but it was not possible to find the AutoConnection "
                                    "Entry to the remote NetProxy with UniqueID %u and main address %s; ignoring option\n",
                                    remoteProxyInfo.getRemoteNetProxyID(), remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                }
            }

            return 0;
        }
        else if (sKey == "connectivity") {
            if (sValue == ACTIVE_CONNECTIVITY_CONFIG_PARAMETER) {
                remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (true);
                remoteProxyInfo.setLocalProxyReachabilityFromRemote (false);
            }
            else if (sValue == PASSIVE_CONNECTIVITY_CONFIG_PARAMETER) {
                remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (false);
                remoteProxyInfo.setLocalProxyReachabilityFromRemote (true);

                const auto usAutoConnections =
                    _rConnectionManager.getSetOfAutoConnectionEntriesToRemoteProxyWithID (remoteProxyInfo.getRemoteNetProxyID());
                for (auto & spAutoConnectionEntry : usAutoConnections) {
                    if (spAutoConnectionEntry) {
                        spAutoConnectionEntry->setInvalid (ET_UNDEF);
                        checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                        "autoConnection to the remote NetProxy with address %s is impossible because the specified "
                                        "connectivity is PASSIVE\n", remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                    }
                }
            }
            else if (sValue == BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER) {
                    remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (true);
                    remoteProxyInfo.setLocalProxyReachabilityFromRemote (true);
            }
            else {
                // Error: impossible to detect the right connectivity value; assuming BIDIRECTIONAL
                remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (true);
                remoteProxyInfo.setLocalProxyReachabilityFromRemote (true);

                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                                "could not interpret the connectivity value specified for remote NetProxy with UniqueID %u and "
                                "main address %s; specified value was %s, admissible values are: %s, %s, %s; assuming %s\n",
                                remoteProxyInfo.getRemoteNetProxyID(), remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString(),
                                sValue.c_str(), ACTIVE_CONNECTIVITY_CONFIG_PARAMETER.c_str(), PASSIVE_CONNECTIVITY_CONFIG_PARAMETER.c_str(),
                                BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER.c_str(), BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER.c_str());
                return 0;
            }

            std::string sValueCopy = sValue;
            std::transform (sValueCopy.begin(), sValueCopy.end(), sValueCopy.begin(), ::toupper);
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Info,
                            "connectivity of local NetProxy to the remote one with UniqueID %u and main address %s set to %s\n",
                            remoteProxyInfo.getRemoteNetProxyID(), remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString(),
                            sValueCopy.c_str());
        }
        else if (sKey == "mocketsconfigfile") {
            if (sValue.length() == 0) {
                checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithMocketsConfigFilePath", NOMADSUtil::Logger::L_Warning,
                                "no name specified for the Mockets config file for connections to the remote NetProxy "
                                "with UniqueID %u and main IP address %s\n", remoteProxyInfo.getRemoteNetProxyID(),
                                remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
                return -8;
            }

            remoteProxyInfo.setMocketsConfigFileName (sValue.c_str());
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithMocketsConfigFilePath", NOMADSUtil::Logger::L_Info,
                            "successfully added the Mockets config file with name <%s> for connections to the remote NetProxy with "
                            "UniqueID %u and main IP address %s\n", sValue.c_str(), remoteProxyInfo.getRemoteNetProxyID(),
                            remoteProxyInfo.getRemoteProxyMainInetAddr().getIPAsString());
        }
        else {
            checkAndLogMsg ("ConfigurationManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", NOMADSUtil::Logger::L_Warning,
                            "unrecognized key %s found\n", sKey.c_str());
            return -9;
        }

        return 0;
    }

    int ConfigurationManager::StaticARPTableConfigFileReader::parseStaticARPTableConfigFile (const char * const pszPathToConfigFile)
    {
        int rc;
        char szLineBuf[MAX_LINE_LENGTH];
        FILE *file = fopen (pszPathToConfigFile, "r");
        if (file == nullptr) {
            return -1;
        }
        while (nullptr != fgets (szLineBuf, MAX_LINE_LENGTH, file)) {
            size_t len = strlen (szLineBuf);
            if (len <= 0) {
                continue;
            }

            if (szLineBuf[len - 1] != '\n') {
                if (!feof (file)) {
                    // Did not find \n so the buffer was not big enough!
                    fclose (file);
                    return -2;
                }
                else {
                    // End of file found - closing file and parsing last line; next while condition will be false
                    fclose (file);
                }
            }

            // Trimming line from spaces and comments
            len = ConfigurationManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // This line needs to be parsed
            if (0 != (rc = parseAndAddEntry (szLineBuf))) {
                checkAndLogMsg ("ConfigurationManager::StaticARPTableConfigFileReader::parseStaticARPTableConfigFile",
                                NOMADSUtil::Logger::L_Warning, "failed to parse line <%s>; rc = %d\n", szLineBuf, rc);
                fclose (file);
                return -3;
            }
        }
        fclose (file);

        return 0;
    }

    int ConfigurationManager::StaticARPTableConfigFileReader::parseAndAddEntry (const char * const pszEntry)
    {
        if (pszEntry == nullptr) {
            return -1;
        }
        // Entries in the format <IP MAC> (without '<' and '>' symbols)
        auto vsIPv4MACAddressPair = splitStringToVector (pszEntry, ' ');
        if (vsIPv4MACAddressPair.size() != 2) {
            return -2;
        }

        // First element in the vector is the IPv4 Address
        if (!checkIPv4AddressFormat (vsIPv4MACAddressPair[0].c_str())) {
            // Wrong IPv4 address format
            checkAndLogMsg ("ConfigurationManager::StaticARPTableConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "error parsing the IPv4 address in line %s; ignoring entry\n", pszEntry);
            return -3;
        }
        NOMADSUtil::InetAddr iaIPv4Address{vsIPv4MACAddressPair[0].c_str()};

        // Second element in the vector is the MAC Address
        auto ema = buildEthernetMACAddressFromString (vsIPv4MACAddressPair[1].c_str());
        if (ema == NetProxyApplicationParameters::EMA_INVALID_ADDRESS) {
            checkAndLogMsg ("ConfigurationManager::StaticARPTableConfigFileReader::parseAndAddEntry", NOMADSUtil::Logger::L_Warning,
                            "error parsing the Ethernet MAC address in line %s; ignoring entry\n", pszEntry);
            return -4;
        }

        _rAC.insert (NOMADSUtil::EndianHelper::ntohl (static_cast<uint32> (iaIPv4Address.getIPAddress())), ema);

        return 0;
    }

    int ConfigurationManager::init (const std::string & sHomeDir, const std::string & sConfigFile)
    {
        int rc;
        _sHomeDir = sHomeDir;
        _sConfigDir = (sConfigFile.substr (0, sConfigFile.find_last_of (NOMADSUtil::getPathSepChar())) == sConfigFile) ?
            "." : sConfigFile.substr (0, sConfigFile.find_last_of (NOMADSUtil::getPathSepChar()));
        checkAndLogMsg ("ConfigurationManager::init", NOMADSUtil::Logger::L_Info,
                        "using <%s> as the home directory\n", sHomeDir.c_str());
        checkAndLogMsg ("ConfigurationManager::init", NOMADSUtil::Logger::L_Info,
                        "using <%s> as the directory for configuration files\n", _sConfigDir.c_str());
        checkAndLogMsg ("ConfigurationManager::init", NOMADSUtil::Logger::L_Info,
                        "using <%s> as the config file\n", sConfigFile.c_str());
        if (0 != (rc = ConfigManager::init())) {
            checkAndLogMsg ("ConfigurationManager::init", NOMADSUtil::Logger::L_MildError,
                            "init() of ConfigManager failed with rc =  %d\n", rc);
            return -1;
        }

        if (0 != (rc = ConfigManager::readConfigFile (sConfigFile.c_str(), true))) {
            checkAndLogMsg ("ConfigurationManager::init", NOMADSUtil::Logger::L_MildError,
                            "readConfigFile() of ConfigManager failed with rc = %d\n", rc);
            return -2;
        }

        return 0;
    }

    int ConfigurationManager::processConfigFiles (void)
    {
        int rc = 0;
        if (0 != (rc = processMainConfigFile())) {
            checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_MildError,
                            "failed to process the NetProxy config file; rc = %d\n", rc);
            return -1;
        }

        // Determine if there is an Endpoint config file specified
        const auto * const pszEndPointConfigFile = getValue ("EndPointConfigFile");
        if (pszEndPointConfigFile != nullptr) {
            std::ostringstream osConfigFile;
            if (_sConfigDir.length() > 0) {
                osConfigFile << _sConfigDir << NOMADSUtil::getPathSepCharAsString();
            }
            osConfigFile << pszEndPointConfigFile;

            const std::string sConfigFile = osConfigFile.str();
            checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Info,
                            "using <%s> as the Endpoint config file\n", sConfigFile.c_str());
            if (0 != (rc = _ecfr.parseEndpointConfigFile (sConfigFile.c_str()))) {
                checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Warning,
                                "failed to read the proxy Endpoint config file at path <%s>; "
                                "rc = %d\n", sConfigFile.c_str(), rc);
                return -2;
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_MildError,
                            "no path was specified for the proxy Endpoint config file\n");
            return -3;
        }

        // Determine if there is a NetProxy UniqueID config file specified
        const auto * const pszUniqueIDConfigFile = getValue ("ProxyUniqueIDsConfigFile");
        if (pszUniqueIDConfigFile != nullptr) {
            std::ostringstream osConfigFile;
            if (_sHomeDir.length() > 0) {
                osConfigFile << _sConfigDir << NOMADSUtil::getPathSepCharAsString();
            }
            osConfigFile << pszUniqueIDConfigFile;

            const std::string sConfigFile = osConfigFile.str();
            checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Info,
                            "using <%s> as the Unique ID config file\n", sConfigFile.c_str());
            if (0 != (rc = _uicfr.parseUniqueIDsConfigFile (sConfigFile.c_str()))) {
                checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Warning,
                                "failed to read the proxy Unique ID config file at path <%s>; "
                                "rc = %d\n", sConfigFile.c_str(), rc);
                return -4;
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_MildError,
                            "no path was specified for the proxy Unique ID config file\n");
            return -5;
        }

        // Determine if there is an Address Mapping config file specified
        const auto * const pszAddressMappingConfigFile = getValue ("ProxyAddrMappingConfigFile");
        if (pszAddressMappingConfigFile != nullptr) {
            std::ostringstream osConfigFile;
            if (_sHomeDir.length() > 0) {
                osConfigFile << _sConfigDir << NOMADSUtil::getPathSepCharAsString();
            }
            osConfigFile << pszAddressMappingConfigFile;

            const std::string sConfigFile = osConfigFile.str();
            checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Info,
                            "using <%s> as the proxy Address Mapping config file\n", sConfigFile.c_str());
            if (0 != (rc = _amcfr.parseAddressMappingConfigFile (sConfigFile.c_str()))) {
                checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Warning,
                                "failed to read the proxy Address Mapping config file at path <%s>; "
                                "rc = %d\n", sConfigFile.c_str(), rc);
                return -6;
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_MildError,
                            "no path was specified for the proxy Address Mapping config file\n");
            return -7;
        }

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            // Determine if there is a Static ARP Table config file specified
            const auto * const pszStaticARPTableConfigFile = getValue ("StaticARPTableConfigFile");
            if (pszStaticARPTableConfigFile != nullptr) {
                std::ostringstream osConfigFile;
                if (_sHomeDir.length() > 0) {
                    osConfigFile << _sConfigDir << NOMADSUtil::getPathSepCharAsString() << pszStaticARPTableConfigFile;
                }
                else {
                    osConfigFile << pszStaticARPTableConfigFile;
                }
                const std::string sConfigFile = osConfigFile.str();
                checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Info,
                                "using <%s> as the proxy Static ARP Table config file\n", sConfigFile.c_str());
                if (0 != (rc = _satcfr.parseStaticARPTableConfigFile (sConfigFile.c_str()))) {
                    checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_Warning,
                                    "failed to read the proxy Static ARP Table config file at path <%s>; "
                                    "rc = %d\n", sConfigFile.c_str(), rc);
                    return -8;
                }
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processConfigFiles", NOMADSUtil::Logger::L_MildError,
                                "no path was specified for the proxy Static ARP Table config file\n");
                return -9;
            }
        }
        consolidateConfigurationSettings();

        return 0;
    }

    int ConfigurationManager::processMainConfigFile (void)
    {
        int rc;

        // Configure logging
        bool bConsoleLogging = false;
        NOMADSUtil::pLogger->enableScreenOutput();
        if (hasValue ("ConsoleLogging")) {
            if (getValueAsBool ("ConsoleLogging")) {
                bConsoleLogging = true;
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "logging to console disabled from configuration file\n");
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                            "ConsoleLogging option not found; NetProxy will disable logging to console\n");
        }

        if (hasValue ("LogFile")) {
            // Log file
            char szLogFilePath[PATH_MAX];
            char szLogFileName[PATH_MAX];
            const char *pszLogFileName = getValue ("LogFile");
            if (0 != stricmp (pszLogFileName, "<none>")) {
                if (0 == stricmp (pszLogFileName, "<generated>")) {
                    char szTimestamp[15] = "";
                    NOMADSUtil::generateTimestamp (szTimestamp, sizeof(szTimestamp));
                    sprintf (szLogFileName, "netproxy.%s.log", szTimestamp);
                    if (_sHomeDir.length() > 0) {
                        strcpy (szLogFilePath, _sHomeDir.c_str());
                        strcat (szLogFilePath, NOMADSUtil::getPathSepCharAsString());
                        strcat (szLogFilePath, NetProxyApplicationParameters::LOGS_DIR.c_str());
                        strcat (szLogFilePath, NOMADSUtil::getPathSepCharAsString());
                        strcat (szLogFilePath, szLogFileName);
                    }
                    else {
                        strcpy (szLogFilePath, "../log/");
                        strcat (szLogFilePath, szLogFileName);
                    }
                }
                else {
                    strcpy (szLogFilePath, pszLogFileName);
                }
                if (0 != (rc = NOMADSUtil::pLogger->initLogFile (szLogFilePath, false))) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "failed to initialize logging to file %s\n", szLogFilePath);
                    NOMADSUtil::pLogger->disableFileOutput();
                }
                else {
                    NOMADSUtil::pLogger->enableFileOutput();
                }
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "logging to file disabled in the options\n");
                NOMADSUtil::pLogger->disableFileOutput();
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "LogFile options not found; logging to file will be disabled\n");
            NOMADSUtil::pLogger->disableFileOutput();
        }

        if (hasValue ("LogLevel")) {
            // Logging level
            const auto iLogLevel = getValueAsInt ("LogLevel");
            if ((iLogLevel > 0) && (iLogLevel < 9)) {
                NOMADSUtil::pLogger->setDebugLevel (static_cast<unsigned char> (iLogLevel));
            }
            else {
                if (0 != NOMADSUtil::pLogger->setDebugLevel (getValue ("LogLevel"))) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "LogLevel options not valid; logging level set to the default value (%s)\n",
                                    NetProxyApplicationParameters::S_DEFAULT_LOG_DETAIL_LEVEL);
                    NOMADSUtil::pLogger->setDebugLevel (NetProxyApplicationParameters::S_DEFAULT_LOG_DETAIL_LEVEL);
                }
            }
        }
        checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                        "logging detail level set to %hhu\n", NOMADSUtil::pLogger->getDebugLevel());

        // Log these here so that they will be written to the log file, if configured to do so
        checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                        "using <%s> as the home directory\n", _sHomeDir.c_str());
        checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                        "using <%s> as the config file\n", _pszConfigFile);

        // Check if a Unique ID has been specified
        if (hasValue ("NetProxyUniqueID")) {
            NetProxyApplicationParameters::NETPROXY_UNIQUE_ID = getValueAsUInt32 ("NetProxyUniqueID");
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "the NetProxy UniqueID is set to be %u\n",
                            NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        }

        // Check whether NetProxy should run in Gateway Mode or Host Mode
        NetProxyApplicationParameters::GATEWAY_MODE = hasValue ("GatewayMode") ?
            getValueAsBool ("GatewayMode") : NetProxyApplicationParameters::DEFAULT_GATEWAY_MODE;
        checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                        NetProxyApplicationParameters::GATEWAY_MODE ?
                        "configured to run in Gateway Mode\n" :
                        "configured to run in Host Mode using the TUN/TAP interface\n");

        // Check whether NetProxy should run in Tunnel mode or Proxy mode
        NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE = hasValue ("Level2TunnelMode") ?
            getValueAsBool ("Level2TunnelMode") : NetProxyApplicationParameters::DEFAULT_LEVEL2_TUNNEL_MODE;
        checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                        NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE ?
                        "configured to run in Tunnel Mode\n" : "configured to run in Proxy Mode\n");

        // Process configuration of internal and external interfaces
        const std::string sExternalInterfaceList = nullprtToEmptyString (getValue ("ExternalInterfaceNames"));
        const auto vExternalInterfacesList = splitStringToVector (sExternalInterfaceList, ',');
        if (vExternalInterfacesList.size() == 0) {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_SevereError,
                            "ExternalInterfaceNames option not found; a list of network interfaces names "
                            "to use as external interfaces is required; NetProxy cannot proceed\n");
            return -1;
        }

        const std::string sInternalInterfaceList = nullprtToEmptyString (getValue ("InternalInterfaceNames"));
        auto vInternalInterfacesList = splitStringToVector (sInternalInterfaceList, ',');
        if ((vInternalInterfacesList.size() == 0) && NetProxyApplicationParameters::GATEWAY_MODE) {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_SevereError,
                            "InternalInterfaceNames option not found; a list of network interfaces names to use as "
                            "internal interfaces is required when running in Gateway Mode; NetProxy cannot proceed\n");
            return -2;
        }
        if (vInternalInterfacesList.size() > 1) {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                            "option InternalInterfaceNames specifies more than one interface "
                            "name; NetProxy will only consider the first one\n");
            vInternalInterfacesList.erase (vInternalInterfacesList.cbegin() + 1, vInternalInterfacesList.cend());
        }
        for (const auto & sInterfaceName : vInternalInterfacesList) {
            if (std::find (vExternalInterfacesList.cbegin(), vExternalInterfacesList.cend(), sInterfaceName) != vExternalInterfacesList.cend()) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_SevereError,
                                "the interface <%s> appears both in the external and the internal interface list; "
                                "please, check your configuration and restart the NetProxy\n", sInterfaceName.c_str());
                return -3;
            }
        }

        // Read interface details for all external interfaces
        for (const auto & sExternalInterfaceName : vExternalInterfacesList) {
            if (std::find_if (NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cbegin(),
                              NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cend(),
                              [sExternalInterfaceName](const NetworkInterfaceDescriptor & nid)
            { return sExternalInterfaceName == nid.sInterfaceName; }) == NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.cend()) {
                auto nidExternalInterface = parseNetworkInterfaceDetails (sExternalInterfaceName, vInternalInterfacesList);
                NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST.push_back (nidExternalInterface);
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "added details about the network interface with name %s\n",
                                sExternalInterfaceName.c_str());
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "did not add details about the network interface with name %s "
                                "(duplicated entry in the ExternalInterfaceNames list?)\n",
                                sExternalInterfaceName.c_str());
            }
        }

        // Read interface details for the internal interface
        NetProxyApplicationParameters::NID_INTERNAL_INTERFACE = parseNetworkInterfaceDetails (vInternalInterfacesList[0], vExternalInterfacesList);

        // Gateway Mode
        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            // Use default MTU size for now - ideally, should query from adapter
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "MFS of the EXTERNAL network interface set to the default value of %hu bytes\n",
                            NetProxyApplicationParameters::ETHERNET_DEFAULT_MFS);
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "MFS of the INTERNAL network interface set to the default value of %hu bytes\n",
                            NetProxyApplicationParameters::ETHERNET_DEFAULT_MFS);

            // Option to run NetProxy in transparent or non-transparent Gateway Mode
            if (hasValue ("TransparentGatewayMode")) {
                bool bTransparentGatewayMode = getValueAsBool ("TransparentGatewayMode");
                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE = bTransparentGatewayMode;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "Transparent Gateway Mode set to %s: the TTL field of IP packets forwarded from one network to the other will %s\n",
                                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? "TRUE" : "FALSE",
                                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ?
                                "not be decremented" : "be decremented by one");
            }
            else {
                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_TRANSPARENT_GATEWAY_MODE;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "The Transparent Gateway Mode option was not specified; using the default value %s: "
                                "the TTL field of IP packets forwarded from one network to the other will %s\n",
                                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? "TRUE" : "FALSE",
                                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? "not be decremented" : "be decremented by one");
            }
        }

        // Connectors: Mockets, TCP, UDP
        if (!NetProxyApplicationParameters::LEVEL2_TUNNEL_MODE) {
            std::string enabledConnectors{getValue ("EnabledConnectors")};
            if (enabledConnectors.length() <= 0) {
                enabledConnectors = NetworkConfigurationSettings::DEFAULT_ENABLED_CONNECTORS;
            }
            auto vsConnectorNames = splitStringToVector (enabledConnectors, ',');
            for (auto sConnectorName = vsConnectorNames.begin(); sConnectorName != vsConnectorNames.end(); ++sConnectorName) {
                if ((*sConnectorName) == S_TCP_CONNECTOR) {
                    _enabledConnectorsSet.put (CT_TCPSOCKET);
                }
                else if ((*sConnectorName) == S_UDP_CONNECTOR) {
                    _enabledConnectorsSet.put (CT_UDPSOCKET);
                }
                else if ((*sConnectorName) == S_MOCKETS_CONNECTOR) {
                    _enabledConnectorsSet.put (CT_MOCKETS);
                }
                else if ((*sConnectorName) == S_CSR_CONNECTOR) {
                    _enabledConnectorsSet.put (CT_CSR);
                }
                else {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "impossible to recognize connector %s; ignoring entry\n",
                                    (*sConnectorName).c_str());
                }
            }
        }
        else {
            // If running in tunnel mode, NetProxy will use UDP to forward packets to the remote NetProxy
            _enabledConnectorsSet.put (CT_UDPSOCKET);
        }

        // Mockets
        if (hasValue ("MocketsListenPort")) {
            // Mockets server port number
            uint32 ui32PortNumber = getValueAsUInt32 ("MocketsListenPort");
            if ((ui32PortNumber > 0) && (ui32PortNumber <= 65535U)) {
                NetProxyApplicationParameters::MOCKET_SERVER_PORT = ui32PortNumber;
            }
            else {
                NetProxyApplicationParameters::MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
            }
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "set Mockets server port to %hu for incoming Mockets connections\n",
                            NetProxyApplicationParameters::MOCKET_SERVER_PORT);
        }
        else {
            NetProxyApplicationParameters::MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "using default Mockets server port of %hu for incoming Mockets connections\n",
                            NetProxyApplicationParameters::MOCKET_SERVER_PORT);
        }

        if (hasValue ("MocketsTimeout")) {
            // Timeout value for mockets to drop a connection
            uint32 ui32MocketsTimeout = getValueAsUInt32 ("MocketsTimeout");
            NetProxyApplicationParameters::MOCKET_TIMEOUT = ui32MocketsTimeout;
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "set Mockets timeout to %lu milliseconds\n",
                            NetProxyApplicationParameters::MOCKET_TIMEOUT);
        }
        else {
            NetProxyApplicationParameters::MOCKET_TIMEOUT = NetProxyApplicationParameters::DEFAULT_MOCKET_TIMEOUT;
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "using default Mockets timeout of %lu milliseconds\n",
                            NetProxyApplicationParameters::MOCKET_TIMEOUT);
        }

        if (hasValue ("DefaultMocketsConfigFile")) {
            // Read default path for the Mockets configuration file
            const std::string sDefaultMocketsConfigFile{nullprtToEmptyString (getValue ("DefaultMocketsConfigFile"))};
            if (sDefaultMocketsConfigFile.length() > 0) {
                NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE = sDefaultMocketsConfigFile.c_str();
            }
        }
        checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                        "default Mockets parameter settings will be loaded from file <%s>\n",
                        NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE.c_str());

        // TCP
        if (hasValue ("TCPListenPort")) {
            // TCP port number
            uint32 ui32PortNumber = getValueAsUInt32 ("TCPListenPort");
            if ((ui32PortNumber > 0) && (ui32PortNumber <= 65535U)) {
                NetProxyApplicationParameters::TCP_SERVER_PORT = ui32PortNumber;
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "listening on port %hu for new TCP connections\n",
                            NetProxyApplicationParameters::TCP_SERVER_PORT);
        }

        // UDP
        if (hasValue ("UDPListenPort")) {
            // UDP port number
            uint32 ui32PortNumber = getValueAsInt ("UDPListenPort");
            if ((ui32PortNumber > 0) && (ui32PortNumber <= 65535U)) {
                if (ui32PortNumber == NetProxyApplicationParameters::MOCKET_SERVER_PORT) {
                    if (NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT == NetProxyApplicationParameters::MOCKET_SERVER_PORT) {
                        ui32PortNumber++;
                    }
                    else {
                        ui32PortNumber = NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT;
                    }
                }
                NetProxyApplicationParameters::UDP_SERVER_PORT = ui32PortNumber;
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "listening on port %hu for any incoming UDP datagram\n",
                            NetProxyApplicationParameters::UDP_SERVER_PORT);
        }

        // Check if a timeout for the ARP Table Miss Cache has been specified
        auto * const pARPMissCacheTimeout = getValue ("ARPMissCacheTimeout");
        if (pARPMissCacheTimeout) {
            int64 i64ARPMissCacheTimeout = std::atoi (pARPMissCacheTimeout);
            if (i64ARPMissCacheTimeout <= 0) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                "entered an invalid value (%lld ms) as the ARP miss cache timeout; the "
                                "default value of %lld bytes will be used instead\n", i64ARPMissCacheTimeout,
                                NetworkConfigurationSettings::DEFAULT_ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS);
            }
            else {
                NetworkConfigurationSettings::ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS = i64ARPMissCacheTimeout;
                _rATMC.setTimeout (NetworkConfigurationSettings::ARP_TABLE_MISS_EXPIRATION_TIME_IN_MS);
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "default maximum transmit packet size set to %hu bytes\n",
                                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            }
        }

        // Check if a maximum transmit packet size has been specified
        const char *pMaxMsgLen = getValue ("MaxMsgLen");
        if (pMaxMsgLen) {
            int iMaxMsgLen = std::atoi (pMaxMsgLen);
            if (iMaxMsgLen <= 0) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                "entered an invalid value (%d bytes) as maximum transmit packet size; "
                                "the default value of %hu bytes will be used\n", iMaxMsgLen,
                                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            }
            else if (iMaxMsgLen > NetworkConfigurationSettings::PROXY_MESSAGE_MTU) {
                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = NetworkConfigurationSettings::PROXY_MESSAGE_MTU;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "entered a too high value (%d bytes) as maximum transmit packet size; "
                                "the maximum allowed value (%hu bytes) will be used\n", iMaxMsgLen,
                                NetworkConfigurationSettings::PROXY_MESSAGE_MTU);
            }
            else {
                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = iMaxMsgLen;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "default maximum transmit packet size set to %hu bytes\n",
                                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "MaxMsgLen option not found; the default value of %hu bytes will be used\n",
                            NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
        }

        // Check if a maximum transmit packet size has been specified
        const char *pVrtConnEstTimeout = getValue ("VirtualConnectionEstablishmentTimeout");
        if (pVrtConnEstTimeout) {
            int iVrtConnEstTimeout = std::atoi (pVrtConnEstTimeout);
            if (iVrtConnEstTimeout <= 0) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                "entered an invalid value (%dms) as timeout for virtual connection establishment; "
                                "the default value of %u bytes will be used\n", iVrtConnEstTimeout,
                                NetworkConfigurationSettings::DEFAULT_VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT);
            }
            else {
                NetworkConfigurationSettings::VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT = iVrtConnEstTimeout;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "timeout for virtual connection establishment set to %ums\n",
                                NetworkConfigurationSettings::VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT);
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "VirtualConnectionEstablishmentTimeout option not found; the default value of %ums will be used\n",
                            NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
        }

        // Priority Mechanism
        if (hasValue ("PriorityMechanism")) {
            NetProxyApplicationParameters::ENABLE_PRIORITIZATION_MECHANISM = getValueAsBool ("PriorityMechanism");
            if (NetProxyApplicationParameters::ENABLE_PRIORITIZATION_MECHANISM) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "Priority mechanism enabled\n");
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "Priority mechanism disabled\n");
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "PriorityMechanism option not found; the option will be set to %s, as by default\n",
                            NetProxyApplicationParameters::DEFAULT_ENABLE_PRIORITIZATION_MECHANISM ? "true" : "false");
            NetProxyApplicationParameters::ENABLE_PRIORITIZATION_MECHANISM =
                NetProxyApplicationParameters::DEFAULT_ENABLE_PRIORITIZATION_MECHANISM;
        }

        // Ignore TCP TIME_WAIT state
        if (hasValue ("SynchronizedTCPHandshake")) {
            NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE = getValueAsBool ("SynchronizedTCPHandshake");
            if (NetworkConfigurationSettings::SYNCHRONIZE_TCP_HANDSHAKE) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "NetProxy will wait to receive the corresponding TCPConnectionOpened ProxyMessage "
                                "from the remote NetProxy before replying with a SYN-ACK to the client "
                                "application to continue the TCP handshake\n");
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "NetProxy will immediately continue the TCP handshake and reply with a SYN-ACK to any "
                                "SYN received from client applications without waiting to receive the corresponding "
                                "TCPConnectionOpened ProxyMessage from the remote NetProxy\n");
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "SynchronizedTCPHandshake option not found; the option will be set to %s, as by default\n",
                            NetworkConfigurationSettings::DEFAULT_SYNCHRONIZE_TCP_HANDSHAKE ? "true" : "false");
        }

        // Ignore TCP TIME_WAIT state
        if (hasValue ("IgnoreTCPTimeWaitStatus")) {
            NetworkConfigurationSettings::TCP_TIME_WAIT_IGNORE_STATE = getValueAsBool ("IgnoreTCPTimeWaitStatus");
            if (NetworkConfigurationSettings::TCP_TIME_WAIT_IGNORE_STATE) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "the NetProxy will ignore the TCP TIME_WAIT state when trying "
                                "to open new TCP connections with local server applications\n");
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "the NetProxy will NOT ignore the TCP TIME_WAIT state when trying "
                                "to open new TCP connections with local server applications\n");
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "IgnoreTCPTimeWaitStatus option not found; the option will be set to %s, as by default\n",
                            NetworkConfigurationSettings::DEFAULT_TCP_TIME_WAIT_IGNORE_STATE ? "true" : "false");
        }

        // Check if timeout for the UDP Nagle's Algorithm has been specified
        bool bUDPNagleAlgorithmUsed = false;
        const char *pcUDPNagleAlgorithmTimeout = getValue ("UDPNagleAlgorithmTimeout");
        if (pcUDPNagleAlgorithmTimeout) {
            int iUDPNagleAlgorithmTimeout = std::atoi (pcUDPNagleAlgorithmTimeout);
            if (iUDPNagleAlgorithmTimeout < 0) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                "entered an invalid value (%d milliseconds) as the timeout for the UDP Nagle's algorithm; "
                                "the algorithm will be disabled\n", iUDPNagleAlgorithmTimeout);
            }

            const uint32 ui32UDPNagleAlgorithmTimeout = static_cast<uint32> (iUDPNagleAlgorithmTimeout);
            if (ui32UDPNagleAlgorithmTimeout > NetworkConfigurationSettings::MAX_UDP_NAGLE_ALGORITHM_TIMEOUT) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "entered a value for the timeout of the UDP Nagle's algorithm that is too high (%u milliseconds); "
                                "the proxy will use the maximum value allowed (%u milliseconds)\n", ui32UDPNagleAlgorithmTimeout,
                                NetworkConfigurationSettings::MAX_UDP_NAGLE_ALGORITHM_TIMEOUT);
            }
            else {
                bUDPNagleAlgorithmUsed = true;
                NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT = iUDPNagleAlgorithmTimeout;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "timeout for the UDP Nagle's algorithm set to %u milliseconds\n",
                                NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT);
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "UDPNagleAlgorithmTimeout option not found; NetProxy will "
                            "disable the Nagle's algorithm for UDP\n");
        }

        // Check if a threshold for the building and sending of a multiple UDP packet has been specified
        if (bUDPNagleAlgorithmUsed) {
            const char *pUDPNagleAlgorithmThreshold = getValue ("UDPNagleAlgorithmThreshold");
            if (pUDPNagleAlgorithmThreshold) {
                int iUDPNagleAlgorithmThreshold = std::atoi (pUDPNagleAlgorithmThreshold);
                if (iUDPNagleAlgorithmThreshold < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%d bytes) as the UDP Nagle's algorithm threshold; "
                                    "algorithm will use the default value of %hu bytes\n", iUDPNagleAlgorithmThreshold,
                                    NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                    iUDPNagleAlgorithmThreshold = NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;
                }

                const uint32 ui32UDPNagleAlgorithmThreshold = static_cast<uint32> (iUDPNagleAlgorithmThreshold);
                if (ui32UDPNagleAlgorithmThreshold > NetworkConfigurationSettings::PROXY_MESSAGE_MTU) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                    "entered a value for the UDP Nagle's algorithm threshold that is too high (%u bytes); "
                                    "max allowed value is %hu; algorithm will use the default value of %hu bytes\n",
                                    ui32UDPNagleAlgorithmThreshold, NetworkConfigurationSettings::PROXY_MESSAGE_MTU,
                                    NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }
                else {
                    NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD = static_cast<uint16> (ui32UDPNagleAlgorithmThreshold);
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                    "UDP Nagle's algorithm threshold set to %u bytes\n",
                                    NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "UDP Nagle's algorithm will use the default threshold value of %hu bytes\n",
                                NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
            }
        }

        // Check if a maximum transmit rate for UDPConnection class has been specified
        const char *pMaxUDPTransmitRate = getValue ("MaxUDPTransmitRate");
        if (pMaxUDPTransmitRate) {
            int iMaxUDPTransmitRate = std::atoi (pMaxUDPTransmitRate);
            if (iMaxUDPTransmitRate < 0) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                "entered an invalid value (%d bytes per second) as maximum UDP transmission rate; "
                                "no limit will be used\n", iMaxUDPTransmitRate);
            }
            else {
                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS = iMaxUDPTransmitRate;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "maximum UDP transmission rate set to %u Bps (%.2f KBps, %.2f MBps)\n",
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS,
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS / 1024.0,
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS / (1024.0 * 1024.0));
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "MaxUDPTransmitRate option not found; NetProxy will not set a "
                            "transmission rate limit for UDP connectors\n");
        }

        // Check if the buffer size for the UDPConnection class has been specified
        const char *pcUDPConnectionBufferSize = getValue ("UDPConnectionBufferSize");
        if (pcUDPConnectionBufferSize) {
            int iUDPConnectionBufferSize = std::atoi (pcUDPConnectionBufferSize);
            if (iUDPConnectionBufferSize < 0) {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                "entered an invalid value (%d bytes) as the maximum buffer size for UDP connections; ",
                                "default value (%u bytes) will be used\n", iUDPConnectionBufferSize,
                                NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_BUFFER_SIZE);
            }

            const uint32 ui32UDPConnectionBufferSize = static_cast<uint32> (iUDPConnectionBufferSize);
            if (ui32UDPConnectionBufferSize > NetworkConfigurationSettings::MAX_UDP_CONNECTION_BUFFER_SIZE) {
                NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetworkConfigurationSettings::MAX_UDP_CONNECTION_BUFFER_SIZE;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "entered a value for the buffer size of UDP connections that is too high (%u bytes); "
                                "the maximum value (%u bytes) will be used instead\n", ui32UDPConnectionBufferSize,
                                NetworkConfigurationSettings::MAX_UDP_CONNECTION_BUFFER_SIZE);
            }
            else {
                if ((NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT > 0) &&
                    (ui32UDPConnectionBufferSize < NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD)) {
                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "the specified buffer size for UDP connections is lower than the UDPNagleAlgorithmThreshold value; "
                                    "the proxy will use the same value (%u bytes) also for the buffer size of UDP connections\n",
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
                else if (ui32UDPConnectionBufferSize == 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                    "entered an invalid value (%u bytes) as the maximum buffer size for UDP connections; "
                                    "the default value of %u bytes will be used instead\n", ui32UDPConnectionBufferSize,
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
                else {
                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = ui32UDPConnectionBufferSize;
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                    "maximum UDP connection buffer size set to %u bytes\n",
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
            }
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "UDPConnectionBufferSize option not found; NetProxy will set the buffer size to the default "
                            "value of %u bytes\n", NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_BUFFER_SIZE);
        }

        // NetSensor
        if (hasValue ("ActivateNetSensor")) {
            NetProxyApplicationParameters::ACTIVATE_NETSENSOR = getValueAsBool ("ActivateNetSensor");
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "ActivateNetSensor option set to %s; NetProxy will %srun NetSensor\n",
                            NetProxyApplicationParameters::ACTIVATE_NETSENSOR? "TRUE" : "FALSE",
                            NetProxyApplicationParameters::ACTIVATE_NETSENSOR ? "" : "not ");
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "ActivateNetsensor option not found; NetProxy will %s NetSensor, as per default option\n",
                            NetProxyApplicationParameters::DEFAULT_ACTIVATE_NETSENSOR ? "enable" : "disable");
            NetProxyApplicationParameters::ACTIVATE_NETSENSOR = NetProxyApplicationParameters::DEFAULT_ACTIVATE_NETSENSOR;
        }

        if (hasValue ("NetSensorStatisticsDestination")) {
            NetProxyApplicationParameters::NETSENSOR_STATISTICS_IP_ADDRESS = getValue ("NetSensorStatisticsDestination");
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "NetSensorStatisticsDestination option set to %s\n",
                            NetProxyApplicationParameters::NETSENSOR_STATISTICS_IP_ADDRESS.c_str());
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "NetSensorStatisticsDestination option not found; statistics will be sent to %s, as per default\n",
                            NetProxyApplicationParameters::DEFAULT_NETSENSOR_STATISTICS_IP_ADDRESS);
            NetProxyApplicationParameters::NETSENSOR_STATISTICS_IP_ADDRESS =
                NetProxyApplicationParameters::DEFAULT_NETSENSOR_STATISTICS_IP_ADDRESS;
        }

        // Statistics and measures
        if (hasValue ("GenerateStatisticsUpdates")) {
            // Statistics collection enabled/diabled
            bool bRunUpdateGUIThread = getValueAsBool ("GenerateStatisticsUpdates");
            NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES = bRunUpdateGUIThread;
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "the StatisticsUpdate Thread is %s\n",
                            NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES ?
                            "enabled" : "disabled");
        }
        else {
            checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                            "GenerateStatisticsUpdates option not found; NetProxy will %s the StatisticsUpdate Thread, as per default\n",
                            NetProxyApplicationParameters::DEFAULT_GENERATE_STATISTICS_UPDATES ? "enable" : "disable");
        }

        // Timeouts for each different measure
        if (NetProxyApplicationParameters::GENERATE_STATISTICS_UPDATES) {
            // Process measure
            const char *pcProcessMI = getValue ("ProcessMeasureInterval");
            if (pcProcessMI) {
                int64 i64ProcessMI = std::stoll (pcProcessMI);
                if (i64ProcessMI < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the interval between two consecutive "
                                    "transmissions of the \"process\" measure; the default value of %lld ms will be used\n",
                                    i64ProcessMI, NetProxyApplicationParameters::I64_DEFAULT_PROCESS_MEASURE_INTERVAL_IN_MS);
                    i64ProcessMI = NetProxyApplicationParameters::I64_DEFAULT_PROCESS_MEASURE_INTERVAL_IN_MS;
                }

                NetProxyApplicationParameters::I64_PROCESS_MEASURE_INTERVAL_IN_MS = i64ProcessMI;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "interval between the transmission of two consecutive \"process\" measures set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_PROCESS_MEASURE_INTERVAL_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "ProcessMeasureInterval option not found; NetProxy will use the default value of %lld milliseconds "
                                "as the interval between two consecutive transmissions of the \"process\" measure\n",
                                NetProxyApplicationParameters::I64_DEFAULT_PROCESS_MEASURE_INTERVAL_IN_MS);
            }

            // Configuration measures
            const char *pcAddressMI = getValue ("AddressMeasureInterval");
            if (pcAddressMI) {
                int64 i64AddressMI = std::stoll (pcAddressMI);
                if (i64AddressMI < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the interval between two consecutive "
                                    "transmissions of the \"address\" measure; the default value of %lld ms will be used\n",
                                    i64AddressMI, NetProxyApplicationParameters::I64_DEFAULT_ADDRESS_MEASURE_INTERVAL_IN_MS);
                    i64AddressMI = NetProxyApplicationParameters::I64_DEFAULT_ADDRESS_MEASURE_INTERVAL_IN_MS;
                }

                NetProxyApplicationParameters::I64_ADDRESS_MEASURE_INTERVAL_IN_MS = i64AddressMI;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "interval for the transmission of two consecutive \"address\" measures set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_ADDRESS_MEASURE_INTERVAL_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "AddressMeasureInterval option not found; NetProxy will use the default value of %lld milliseconds"
                                "as the interval between two consecutive transmissions of the \"address\" measure\n",
                                NetProxyApplicationParameters::I64_DEFAULT_ADDRESS_MEASURE_INTERVAL_IN_MS);
            }

            const char *pcProtocolMI = getValue ("ProtocolMeasureInterval");
            if (pcProtocolMI) {
                int64 i64ProtocolMI = std::stoll (pcProtocolMI);
                if (i64ProtocolMI < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the interval between two consecutive "
                                    "transmissions of the \"protocol\" measure; the default value of %lld ms will be used\n",
                                    i64ProtocolMI, NetProxyApplicationParameters::I64_DEFAULT_PROTOCOL_MEASURE_INTERVAL_IN_MS);
                    i64ProtocolMI = NetProxyApplicationParameters::I64_DEFAULT_PROTOCOL_MEASURE_INTERVAL_IN_MS;
                }

                NetProxyApplicationParameters::I64_PROTOCOL_MEASURE_INTERVAL_IN_MS = i64ProtocolMI;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "interval for the transmission of two consecutive \"protocol\" measures set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_PROTOCOL_MEASURE_INTERVAL_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "ProtocolMeasureInterval option not found; NetProxy will use the default value of %lld milliseconds "
                                "as the interval between two consecutive transmissions of the \"protocol\" measure\n",
                                NetProxyApplicationParameters::I64_DEFAULT_PROTOCOL_MEASURE_INTERVAL_IN_MS);
            }

            // Topology measures
            const char *pcTopologyNodeMI = getValue ("TopologyNodeMeasureInterval");
            if (pcTopologyNodeMI) {
                int64 i64TopologyNodeMI = std::stoll (pcTopologyNodeMI);
                if (i64TopologyNodeMI < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the interval between two consecutive "
                                    "transmissions of the \"topology node\" measure; the default value of %lld ms will be used\n",
                                    i64TopologyNodeMI, NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS);
                    i64TopologyNodeMI = NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS;
                }

                NetProxyApplicationParameters::I64_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS = i64TopologyNodeMI;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "interval for the transmission of two consecutive \"topology node\" measures set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "TopologyNodeMeasureInterval option not found; NetProxy will use the default value of %lld milliseconds "
                                "as the interval between two consecutive transmissions of the \"topology node\" measure\n",
                                NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_NODE_MEASURE_INTERVAL_IN_MS);
            }

            const char *pcTopologyEdgeMI = getValue ("TopologyEdgeMeasureInterval");
            if (pcTopologyEdgeMI) {
                int64 i64TopologyEdgeMI = std::stoll (pcTopologyEdgeMI);
                if (i64TopologyEdgeMI < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the interval between two consecutive "
                                    "transmissions of the \"topology edge\" measure; the default value of %lld ms will be used\n",
                                    i64TopologyEdgeMI, NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS);
                    i64TopologyEdgeMI = NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS;
                }

                NetProxyApplicationParameters::I64_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS = i64TopologyEdgeMI;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "interval for the transmission of two consecutive \"topology edge\" measures set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "TopologyEdgeMeasureInterval option not found; NetProxy will use the default value of %lld milliseconds "
                                "as the interval between two successive transmissions of the \"topology edge\" measure\n",
                                NetProxyApplicationParameters::I64_DEFAULT_TOPOLOGY_EDGE_MEASURE_INTERVAL_IN_MS);
            }

            // Link Description measures
            const char *pcLinkDescriptionMI = getValue ("LinkDescriptionMeasureInterval");
            if (pcLinkDescriptionMI) {
                int64 i64LinkDescriptionMI = std::stoll (pcLinkDescriptionMI);
                if (i64LinkDescriptionMI < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the interval between two consecutive "
                                    "transmissions of the \"link description\" measure; the default value of %lld ms will be used\n",
                                    i64LinkDescriptionMI, NetProxyApplicationParameters::I64_DEFAULT_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS);
                    i64LinkDescriptionMI = NetProxyApplicationParameters::I64_DEFAULT_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS;
                }

                NetProxyApplicationParameters::I64_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS = i64LinkDescriptionMI;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "interval for the transmission of two consecutive \"link description\" measures set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "LinkDescriptionMeasureInterval option not found; NetProxy will use the default value of %lld milliseconds "
                                "as the interval between successive transmissions of the \"link description\" measure\n",
                                NetProxyApplicationParameters::I64_DEFAULT_LINK_DESCRIPTION_MEASURE_INTERVAL_IN_MS);
            }

            // Link Traffic measures
            const char *pcLinkTrafficMI = getValue ("LinkTrafficMeasureInterval");
            if (pcLinkTrafficMI) {
                int64 i64LinkTrafficMI = std::stoll (pcLinkTrafficMI);
                if (i64LinkTrafficMI < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the interval between two consecutive "
                                    "transmissions of the \"link traffic\" measure; the default value of %lld ms will be used\n",
                                    i64LinkTrafficMI, NetProxyApplicationParameters::I64_DEFAULT_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS);
                    i64LinkTrafficMI = NetProxyApplicationParameters::I64_DEFAULT_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS;
                }

                NetProxyApplicationParameters::I64_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS = i64LinkTrafficMI;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "interval for the transmission of two consecutive \"link traffic\" measures set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "LinkTrafficMeasureInterval option not found; NetProxy will use the default value of %lld milliseconds "
                                "as the interval between two consecutive transmissions of the \"link traffic\" measure\n",
                                NetProxyApplicationParameters::I64_DEFAULT_LINK_TRAFFIC_MEASURE_INTERVAL_IN_MS);
            }

            // Deletion of Edge measures of disconnected links
            const char *pcDisconnectedEdgeMLT = getValue ("DisconnectedEdgeMeasureLifetime");
            if (pcDisconnectedEdgeMLT) {
                int64 i64DisconnectedEdgeMLT = std::stoll (pcDisconnectedEdgeMLT);
                if (i64DisconnectedEdgeMLT < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the lifetime of \"edge\" measures of "
                                    "disconnected links; the default value of %lld ms will be used\n", i64DisconnectedEdgeMLT,
                                    NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS);
                    i64DisconnectedEdgeMLT = NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS;
                }

                NetProxyApplicationParameters::I64_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS = i64DisconnectedEdgeMLT;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "lifetime for the transmission of \"edge\" measures of disconnected links set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "DisconnectedEdgeMeasureLifetime option not found; NetProxy will use the default value "
                                "of %lld milliseconds as the lifetime of \"edge\" measures of disconnected links\n",
                                NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_EDGE_MEASURE_LIFETIME_IN_MS);
            }

            // Deletion of Link Description measures of disconnected links
            const char *pcDisconnectedLinkDescriptionMLT = getValue ("DisconnectedLinkDescriptionMeasureLifetime");
            if (pcDisconnectedLinkDescriptionMLT) {
                int64 i64DisconnectedLinkDescriptionMLT = std::stoll (pcDisconnectedLinkDescriptionMLT);
                if (i64DisconnectedLinkDescriptionMLT < 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entered an invalid value (%lld milliseconds) as the lifetime of \"link description\" measures "
                                    "of disconnected links; the default value %lld will be used\n", i64DisconnectedLinkDescriptionMLT,
                                    NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS);
                    i64DisconnectedLinkDescriptionMLT = NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS;
                }

                NetProxyApplicationParameters::I64_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS = i64DisconnectedLinkDescriptionMLT;
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "lifetime for the transmission of \"link description\" measures of disconnected links set to %lld milliseconds\n",
                                NetProxyApplicationParameters::I64_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS);
            }
            else {
                checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                "DisconnectedLinkDescriptionMeasureLifetime option not found; NetProxy will use the default value "
                                "of %lld milliseconds as the lifetime of \"link description\" measure of disconnected links\n",
                                NetProxyApplicationParameters::I64_DEFAULT_DISCONNECTED_LINK_DESCRIPTION_MEASURE_LIFETIME_IN_MS);
            }

            // Parse addresses to which measures need to be sent
            std::string sCleanAddressList, sAddresses{
                getValue ("StatusNotificationAddresses", NetProxyApplicationParameters::DEFAULT_STATUS_NOTIFICATION_ADDRESS)};
            auto vsAddressList = splitStringToVector (sAddresses, ',');
            for (auto iAddresses = vsAddressList.begin(); iAddresses != vsAddressList.end(); ++iAddresses) {
                auto vsIPv4PortPair = splitStringToVector (*iAddresses, ':');
                if (iAddresses->length() <= 0) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "found an empty entry in the StatusNotificationAddresses option\n");
                    continue;
                }

                std::string sIP{vsIPv4PortPair[0]}, sPort{(vsIPv4PortPair.size() > 1) ? vsIPv4PortPair[1] : ""};
                if (!checkIPv4AddressFormat (sIP)) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                    "entry <%s> in the StatusNotificationAddresses option does not contain a valid IP address; using the "
                                    "default value <%s>\n", iAddresses->c_str(), NetProxyApplicationParameters::DEFAULT_STATS_COLLECTOR_IP_ADDR);
                    sIP = NetProxyApplicationParameters::DEFAULT_STATS_COLLECTOR_IP_ADDR;
                }
                if ((vsIPv4PortPair.size() == 1) || (sPort.length() == 0)) {
                    checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Info,
                                    "port not specified for address <%s> in the StatusNotificationAddresses option; using the default port "
                                    "number %hu\n", sIP.c_str(), NetProxyApplicationParameters::DEFAULT_STATS_COLLECTOR_PORT);
                    sPort = std::to_string (NetProxyApplicationParameters::DEFAULT_STATS_COLLECTOR_PORT);
                }
                else {
                    // Parsing port number
                    uint32 ui32GUIPortNumber = std::stoi (sPort);
                    if (ui32GUIPortNumber > 65535U) {
                        checkAndLogMsg ("ConfigurationManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                        "<%u> is not a valid port number; the default value <%hu> will be used for notifications sent to <%s>\n",
                                        ui32GUIPortNumber, NetProxyApplicationParameters::DEFAULT_STATS_COLLECTOR_PORT, sIP.c_str());
                        sPort = std::to_string (NetProxyApplicationParameters::DEFAULT_STATS_COLLECTOR_PORT);
                    }
                }

                sCleanAddressList += ((sCleanAddressList.length () > 0) ? "," : "") + sIP + ":" + sPort;
            }

            setValue ("StatusNotificationAddresses", sCleanAddressList.c_str());
        }


        // Disable console logging if specified in the config file
        if (!bConsoleLogging) {
            NOMADSUtil::pLogger->disableScreenOutput();
        }

        return 0;
    }

    NetworkInterfaceDescriptor ConfigurationManager::parseNetworkInterfaceDetails (const std::string & sInterfaceName,
                                                                                   const std::vector<std::string> & vComplementaryNetworkInterfacesList)
    {
        NetworkInterfaceDescriptor nidRes;
        nidRes.sInterfaceName = sInterfaceName;
        const auto sInterfaceDescriptionFileName = "interface." + sInterfaceName + ".cfg";

        std::ostringstream oss;
        oss << _sConfigDir << NOMADSUtil::getPathSepCharAsString() << sInterfaceDescriptionFileName;
        const auto sInterfaceDescriptionFilePath = oss.str();
        checkAndLogMsg ("ConfigurationManager::addExternalInterfaceDetails", NOMADSUtil::Logger::L_Info,
                        "parsing network interface configuration file <%s>\n",
                        sInterfaceDescriptionFilePath.c_str());

        NetworkInterfaceDescriptionReader nidr {sInterfaceName};
        if (0 != nidr.init (sInterfaceDescriptionFilePath)) {
            checkAndLogMsg ("ConfigurationManager::addExternalInterfaceDetails", NOMADSUtil::Logger::L_Warning,
                            "NetworkInterfceDescriptorReader.init() returned an error while trying to read "
                            "file %s; an empty NetworkInterfaceDescriptor object will be returned\n",
                            sInterfaceDescriptionFilePath.c_str());
            return nidRes;
        }

        return nidr.parseConfigFile (vComplementaryNetworkInterfacesList);
    }

    void ConfigurationManager::consolidateConfigurationSettings (void)
    {
        const auto remoteHostAddressMappingBook = _amcfr.getRemoteHostMappingList();
        for (const auto & endpointConfigEntry : _ecfr.getEndpointConfigTable()) {
            for (const auto & ppARDARDtiaui32ia : remoteHostAddressMappingBook) {
                if (endpointConfigEntry.matches (ppARDARDtiaui32ia.first.first, ppARDARDtiaui32ia.first.second)) {
                    auto * const pConnectivitySolutions =
                        _rConnectionManager.findConnectivitySolutionsToNetProxyWithIDAndIPv4Address (std::get<0> (ppARDARDtiaui32ia.second),
                                                                                                     std::get<1> (ppARDARDtiaui32ia.second).getIPAddress(),
                                                                                                     std::get<2> (ppARDARDtiaui32ia.second).getIPAddress());
                    if (pConnectivitySolutions == nullptr) {
                        checkAndLogMsg ("NetProxyConfigManager::consolidateConfigurationSettings", NOMADSUtil::Logger::L_Warning,
                                        "findConnectivitySolutionsToProxyWithIDAndIPv4Address() could not find the ConnectivitySolutions object "
                                        "to reach the remote NetProxy with Unique ID %u (possible IP address: %s); the NetProxy will not "
                                        "try to consolidate the address mapping entry with filter <%s>-<%s> for this remote NetProxy\n",
                                        std::get<0> (ppARDARDtiaui32ia.second), std::get<2> (ppARDARDtiaui32ia.second).getIPAsString(),
                                        NetworkAddressRangeDescriptor{ppARDARDtiaui32ia.first.first}.getAddressRangeStringDescription().c_str(),
                                        NetworkAddressRangeDescriptor{ppARDARDtiaui32ia.first.second}.getAddressRangeStringDescription().c_str());
                        continue;
                    }

                    // TCP
                    const ProtocolSetting &tcpSettings = endpointConfigEntry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_TCP);
                    pConnectivitySolutions->setEncryptionDescription (tcpSettings.getConnectorTypeFromProtocol(), tcpSettings.getEncryptionType());
                    if (tcpSettings.getConnectorTypeFromProtocol() == CT_MOCKETS) {
                        NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED |= (tcpSettings.getEncryptionType() == ET_DTLS);
                    }

                    // UDP
                    const ProtocolSetting &udpSettings = endpointConfigEntry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_UDP);
                    pConnectivitySolutions->setEncryptionDescription (udpSettings.getConnectorTypeFromProtocol(), udpSettings.getEncryptionType());
                    if (udpSettings.getConnectorTypeFromProtocol() == CT_MOCKETS) {
                        NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED |= (udpSettings.getEncryptionType() == ET_DTLS);
                    }

                    // ICMP
                    const ProtocolSetting &icmpSettings = endpointConfigEntry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_ICMP);
                    pConnectivitySolutions->setEncryptionDescription (icmpSettings.getConnectorTypeFromProtocol(), icmpSettings.getEncryptionType());
                    if (icmpSettings.getConnectorTypeFromProtocol() == CT_MOCKETS) {
                        NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED |= (icmpSettings.getEncryptionType() == ET_DTLS);
                    }
                }
            }
        }

        const auto multicastAddressMappingBook = _amcfr.getMulticastMappingList();
        for (const auto & endpointConfigEntry : _ecfr.getEndpointConfigTable()) {
            for (const auto & ppARDARDpui32ia : multicastAddressMappingBook) {
                if (endpointConfigEntry.matches (ppARDARDpui32ia.first.first, ppARDARDpui32ia.first.second)) {
                    auto * const pConnectivitySolutions =
                        _rConnectionManager.findConnectivitySolutionsToNetProxyWithIDAndIPv4Address (std::get<0> (ppARDARDpui32ia.second),
                                                                                                     std::get<1> (ppARDARDpui32ia.second).getIPAddress(),
                                                                                                     std::get<2> (ppARDARDpui32ia.second).getIPAddress());
                    if (pConnectivitySolutions == nullptr) {
                        checkAndLogMsg ("NetProxyConfigManager::processMainConfigFile", NOMADSUtil::Logger::L_Warning,
                                        "findConnectivitySolutionsToProxyWithIDAndIPv4Address() could not find the ConnectivitySolutions instance for "
                                        "the remote NetProxy with ID %u (possible IP address: %s); make sure that the remote NetProxy UniqueIDs specified "
                                        "in the proxyAddressMapping configuration file match the ones defined in the proxyUniqueID configuration file\n",
                                        std::get<0> (ppARDARDpui32ia.second), std::get<2> (ppARDARDpui32ia.second).getIPAsString());
                        continue;
                    }

                    // TCP
                    const ProtocolSetting &tcpSettings = endpointConfigEntry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_TCP);
                    pConnectivitySolutions->setEncryptionDescription (tcpSettings.getConnectorTypeFromProtocol(), tcpSettings.getEncryptionType());
                    if (tcpSettings.getConnectorTypeFromProtocol() == CT_MOCKETS) {
                        NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED |= (tcpSettings.getEncryptionType() == ET_DTLS);
                    }

                    // UDP
                    const ProtocolSetting &udpSettings = endpointConfigEntry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_UDP);
                    pConnectivitySolutions->setEncryptionDescription (udpSettings.getConnectorTypeFromProtocol(), udpSettings.getEncryptionType());
                    if (udpSettings.getConnectorTypeFromProtocol() == CT_MOCKETS) {
                        NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED |= (udpSettings.getEncryptionType() == ET_DTLS);
                    }

                    // ICMP
                    const ProtocolSetting &icmpSettings = endpointConfigEntry.getConfigParams()->getProtocolSetting (NOMADSUtil::IP_PROTO_ICMP);
                    pConnectivitySolutions->setEncryptionDescription (icmpSettings.getConnectorTypeFromProtocol(), icmpSettings.getEncryptionType());
                    if (icmpSettings.getConnectorTypeFromProtocol() == CT_MOCKETS) {
                        NetProxyApplicationParameters::MOCKETS_DTLS_ENABLED |= (icmpSettings.getEncryptionType() == ET_DTLS);
                    }
                }
            }
        }

        // Update encryption descriptor of AutoConnectionEntries with information contained in the ConnectivitySolution instances
        _rConnectionManager.updateAutoConnectionEntries();
    }

    unsigned int ConfigurationManager::trimConfigLine (char * const pConfigLine)
    {
        auto len = strlen (pConfigLine);

        // Perform a dos2unix eol conversion if necessary
        if ((len >= 2) && (pConfigLine[len-2] == '\r') && (pConfigLine[len-1] == '\n')) {
            pConfigLine[len-2] = '\0';
            len -= 2;
        }

        if ((len >= 1) && (pConfigLine[len-1] == '\n')) {
            pConfigLine[len-1] = '\0';
            len--;
        }

        char * pcCommentSymbol;
        if ((pcCommentSymbol = strchr (pConfigLine, '#'))) {
            *pcCommentSymbol = '\0';
        }
        len = strlen (pConfigLine);

        // Trim final spaces
        while (len > 0) {
            if (isspace (pConfigLine[len - 1])) {
                pConfigLine[len - 1] = '\0';
                len--;
            }
            else {
                break;
            }
        }

        return len;
    }

    const NetworkInterfaceDescriptor & findNIDWithName (const std::string & sInterfaceName)
    {
        for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            if (nid.sInterfaceName == sInterfaceName) {
                return nid;
            }
        }
        if (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.sInterfaceName == sInterfaceName) {
            return NetProxyApplicationParameters::NID_INTERNAL_INTERFACE;
        }

        return NetProxyApplicationParameters::NID_INVALID;
    }

    const NetworkInterfaceDescriptor & findNIDWithIPv4Address (uint32 ui32IPv4Address)
    {
        for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            if (nid.ui32IPv4Address == ui32IPv4Address) {
                return nid;
            }
        }
        if (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4Address == ui32IPv4Address) {
            return NetProxyApplicationParameters::NID_INTERNAL_INTERFACE;
        }

        return NetProxyApplicationParameters::NID_INVALID;
    }

    const NetworkInterfaceDescriptor & findNIDWithDefaultGatewayIPv4Address (uint32 ui32IPv4DefaultGatewayAddress)
    {
        for (const auto & nid : NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            if (nid.ui32IPv4GatewayAddress == ui32IPv4DefaultGatewayAddress) {
                return nid;
            }
        }
        if (NetProxyApplicationParameters::NID_INTERNAL_INTERFACE.ui32IPv4GatewayAddress == ui32IPv4DefaultGatewayAddress) {
            return NetProxyApplicationParameters::NID_INTERNAL_INTERFACE;
        }

        return NetProxyApplicationParameters::NID_INVALID;
    }


    const std::string ConfigurationManager::S_TCP_CONNECTOR{"TCP"};
    const std::string ConfigurationManager::S_UDP_CONNECTOR{"UDP"};
    const std::string ConfigurationManager::S_MOCKETS_CONNECTOR{"Mockets"};
    const std::string ConfigurationManager::S_CSR_CONNECTOR{"CSR"};
    const std::string ConfigurationManager::AddressMappingConfigFileReader::DEFAULT_SOURCE_ADDRESS_MAPPING{"*.*.*.*"};
    const std::string ConfigurationManager::UniqueIDsConfigFileReader::ACTIVE_CONNECTIVITY_CONFIG_PARAMETER{"active"};
    const std::string ConfigurationManager::UniqueIDsConfigFileReader::PASSIVE_CONNECTIVITY_CONFIG_PARAMETER{"passive"};
    const std::string ConfigurationManager::UniqueIDsConfigFileReader::BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER{"bidirectional"};
}
