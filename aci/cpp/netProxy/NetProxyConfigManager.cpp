/*
 * NetProxyConfigManager.cpp
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
 */

#if defined (WIN32)
    #include <io.h>
#else
    #if !defined (ANDROID)
        #include <sys/io.h>
    #endif
    #include <limits.h>
#endif

#include "NLFLib.h"
#include "StringTokenizer.h"
#include "Logger.h"

#include "NetProxyConfigManager.h"
#include "ConnectionManager.h"

#if defined (WIN32)
    #define access _access
    #define strdup _strdup
#endif


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    int NetProxyConfigManager::EndpointConfigParameters::parse (char *pszParamsEntry)
    {
        if (pszParamsEntry == NULL) {
            return -1;
        }

        StringTokenizer st (pszParamsEntry, ';');
        String sKeyValuePair, sKey, sValue;
        while ((sKeyValuePair = st.getNextToken()).length() > 0) {
            sKeyValuePair.convertToLowerCase();
            StringTokenizer stProtocolPortPair (sKeyValuePair, '=');
            sKey = stProtocolPortPair.getNextToken();
            sValue = stProtocolPortPair.getNextToken();
            if (sKey.length() <= 0) {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_Warning,
                                "no key found in the line of the config file\n");
                continue;
            }
            if (sValue.length() <= 0) {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_Warning,
                                "no value has been specified for the key %s in the line of the config file\n", sKey.c_str());
                continue;
            }
            if (sKey == "icmp") {
                if (!ProtocolSetting::isProtocolNameCorrect (sValue)) {
                    checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_Warning,
                                    "the specified protocol %s is not correct. The default (%s) will be used\n", sValue.c_str(),
                                    _psICMPProtocolSetting.getProxyMessageProtocolAsString());
                }
                else {
                    _psICMPProtocolSetting = ProtocolSetting (sValue);
                    checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_LowDetailDebug,
                                    "protocol %s setted to map ICMP protocol\n", sValue.c_str());
                }
            }
            else if (sKey == "tcp") {
                if (!ProtocolSetting::isProtocolNameCorrect (sValue)) {
                    checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_Warning,
                                    "the specified protocol %s is not correct. The default (%s) will be used\n", sValue.c_str(),
                                    _psTCPProtocolSetting.getProxyMessageProtocolAsString());
                }
                else {
                    _psTCPProtocolSetting = ProtocolSetting (sValue);
                    checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_LowDetailDebug,
                                    "protocol %s setted to map TCP protocol\n", sValue.c_str());
                }
            }
            else if (sKey == "udp") {
                if (!ProtocolSetting::isProtocolNameCorrect (sValue)) {
                    checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_Warning,
                                    "the specified protocol %s is not correct. The default (%s) will be used\n", sValue.c_str(),
                                    _psUDPProtocolSetting.getProxyMessageProtocolAsString());
                }
                else {
                    _psUDPProtocolSetting = ProtocolSetting (sValue);
                    checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_LowDetailDebug,
                                    "protocol %s setted to map UDP protocol\n", sValue.c_str());
                }
            }
            else if (sKey == "tcpcompression") {
                _psTCPProtocolSetting.setCompressionSetting (readCompressionConfEntry (sValue));
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::readCompressionConfEntry", Logger::L_LowDetailDebug,
                                "compression algorithm <%s> with level %hhu will be used to compress TCP streams\n",
                                _psTCPProtocolSetting.getCompressionSetting().getCompressionTypeAsString(),
                                _psTCPProtocolSetting.getCompressionSetting().getCompressionLevel());
            }
            else if (sKey == "udpcompression") {
                _psUDPProtocolSetting.setCompressionSetting (readCompressionConfEntry (sValue));
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::readCompressionConfEntry", Logger::L_LowDetailDebug,
                                "compression algorithm <%s> with level %hhu will be used to compress UDP datagrams\n",
                                _psUDPProtocolSetting.getCompressionSetting().getCompressionTypeAsString(),
                                _psUDPProtocolSetting.getCompressionSetting().getCompressionLevel());
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_Warning,
                                "key value %s not recognized\n", sKey.c_str());
            }
        }

        return 0;
    }

    const CompressionSetting NetProxyConfigManager::EndpointConfigParameters::readCompressionConfEntry (const char * const pcCompressionAlg)
    {
        // Format is <compression_name>:<compression_level>
        String sCompressionAlgName;
        CompressionSetting compressionSetting;
        StringTokenizer st (pcCompressionAlg, ':');
        sCompressionAlgName = st.getNextToken();
        sCompressionAlgName.convertToLowerCase();
        String sCompressionLevel = st.getNextToken();

        int iCompressionLevel = CompressionSetting::DEFAULT_COMPRESSION_LEVEL;
        if (sCompressionLevel) {
            iCompressionLevel = atoi (sCompressionLevel);
            if (iCompressionLevel < 0) {
                iCompressionLevel = 0;
            }
            else if (iCompressionLevel > 9) {
                iCompressionLevel = 9;
            }
        }
        if (!CompressionSetting::isSpecifiedCompressionNameCorrect (sCompressionAlgName)) {
            checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::readCompressionConfEntry", Logger::L_Warning,
                            "specified an invalid compression algorithm (%s); no compression (plain) will be used\n", sCompressionAlgName.c_str());
        }
        else if ((sCompressionAlgName != "none") && (iCompressionLevel == 0)) {
            checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::readCompressionConfEntry", Logger::L_MediumDetailDebug,
                            "specified a compression level of 0 (or less) with compression algorithm <%s>; no compression (plain) will be used instead\n",
                            sCompressionAlgName.c_str());
        }
        compressionSetting = CompressionSetting (sCompressionAlgName, iCompressionLevel);

        return compressionSetting;
    }

    int NetProxyConfigManager::AddressMappingConfigFileReader::parseAddressMappingConfigFile (const char * const pszPathToConfigFile)
    {
        int rc;
        char szLineBuf[MAX_LINE_LENGTH];
        FILE *file = fopen (pszPathToConfigFile, "r");
        if (file == NULL) {
            return -1;
        }
        while (NULL != fgets (szLineBuf, MAX_LINE_LENGTH, file)) {
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
            len = NetProxyConfigManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // Have a line that needs to be parsed
            if (0 != (rc = parseAndAddEntry (szLineBuf))) {
                checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAddressMappingConfigFile",
                                Logger::L_Warning, "failed to parse entry <%s>; rc = %d\n", szLineBuf, rc);
            }
        }

        return 0;
    }

    int NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry (const char *pszEntry)
    {
        if (pszEntry == NULL) {
            return -1;
        }
        static ConnectionManager * const pConnectionManager = ConnectionManager::getConnectionManagerInstance();

        StringTokenizer st (pszEntry, ' ');
        const char * const pVirtualAddrRange = st.getNextToken();
        if (pVirtualAddrRange == NULL) {
            return -2;
        }
        
        /* The constructor parses a range of IP addresses in the format <X.Y.W.Z:P> (without <>).
         * Any of the symbols X, Y, W, Z, and P can be represented as a range in the format A-B;
         * a range A-B will include all IPs (or ports) from A to B in that network.
         * For an IP to be a match for an entry of a range of addresses, the latter needs to include
         * all bytes of said IP address within all ranges specified in such entry. */
        AddressRangeDescriptor rangeDescriptor (pVirtualAddrRange);
        if (!rangeDescriptor.isValid()) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                            "impossible to parse the address range %s correctly\n", pVirtualAddrRange);
            return -3;
        }

        // Parsing a single remote NetProxy address
        uint32 ui32RemoteProxyID = 0;
        const String sRemoteProxyID (st.getNextToken());
        if (sRemoteProxyID.length() <= 0) {
            return -4;
        }
        else {
            ui32RemoteProxyID = sRemoteProxyID.contains (".") ? InetAddr (sRemoteProxyID).getIPAddress() : atoi (sRemoteProxyID);
        }
        if ((ui32RemoteProxyID == 0) || (ui32RemoteProxyID == 0xFFFFFFFFU)) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                            "impossible to parse the remote NetProxy UniqueID for line %s\n", pszEntry);
            return -5;
        }

        if (pConnectionManager->addNewAddressMappingToBook (rangeDescriptor, ui32RemoteProxyID) < 0) {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                            "impossible to add the new pair <%s>:<%u> to the Address Mapping Table\n",
                            rangeDescriptor.operator const char *const(), ui32RemoteProxyID);
            return -5;
        }
        checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::parseAndAddEntry", Logger::L_Info,
                        "successfully added the new pair <%s>:<%u> to the Address Mapping Table\n",
                        rangeDescriptor.operator const char *const(), ui32RemoteProxyID);

        return 0;
    }

    int NetProxyConfigManager::UniqueIDsConfigFileReader::parseUniqueIDsConfigFile (const char * const pszPathToConfigFile)
    {
        int rc;
        char szLineBuf[MAX_LINE_LENGTH];
        FILE *file = fopen (pszPathToConfigFile, "r");
        if (file == NULL) {
            return -1;
        }
        while (NULL != fgets (szLineBuf, MAX_LINE_LENGTH, file)) {
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
            len = NetProxyConfigManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // Have a line that needs to be parsed
            if (0 != (rc = parseAndAddEntry (szLineBuf))) {
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::parseUniqueIDsConfigFile", Logger::L_Warning,
                                "failed to parse entry <%s>; rc = %d\n", szLineBuf, rc);
            }
        }

        return 0;
    }

    int NetProxyConfigManager::UniqueIDsConfigFileReader::parseAndAddEntry (const char *pszEntry)
    {
        if (pszEntry == NULL) {
            return -1;
        }

        StringTokenizer st (pszEntry, ' ');
        const char * const pRemoteProxyIP = st.getNextToken();
        if (pRemoteProxyIP == NULL) {
            return -2;
        }
        InetAddr remoteProxyIP (pRemoteProxyIP);
        if (remoteProxyIP.getIPAddress() == 0) {
            return -3;
        }
        
        /* The next element in the line could be either the UniqueID, the first option, or nothing. */
        uint32 ui32UniqueID = 0;
        String sUniqueID = st.getNextToken();
        ui32UniqueID = sUniqueID.contains ("=") ? remoteProxyIP.getIPAddress() : (atoi (sUniqueID) ? atoi (sUniqueID) : remoteProxyIP.getIPAddress());
        RemoteProxyInfo remoteProxyInfo (ui32UniqueID, static_cast<uint32> (remoteProxyIP.getIPAddress ()));
        if (P_CONNECTION_MANAGER->addNewRemoteProxyInfo (remoteProxyInfo) < 0) {
            checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                            "error returned by call to addNewRemoteProxyInfo() after parsing "
                            "line <%s> in the config file\n", pszEntry);
            return -4;
        }

        RemoteProxyInfo * const pRemoteProxyInfo = P_CONNECTION_MANAGER->getRemoteProxyInfoForProxyWithID (ui32UniqueID);        
        // Parsing a list of port numbers per protocol (e.g., MocketsPort=8751;TCPPort=8080)
        String sKeyValuePair = sUniqueID.contains ("=") ? sUniqueID : String(st.getNextToken()), sKey, sValue;
        while (sKeyValuePair.length() >= 0) {
            sKeyValuePair.convertToLowerCase();
            StringTokenizer stProtocolPortPair (sKeyValuePair, '=');
            sKey = stProtocolPortPair.getNextToken();
            sValue = stProtocolPortPair.getNextToken();

            if (sValue != NULL) {
                // <Key=value> pair
                if (updateInfoWithKeyValuePair (*pRemoteProxyInfo, sKey, sValue) < 0) {
                    checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                                    "error encountered while trying to parsing line <%s> in the config file\n", pszEntry);
                    return -5;
                }
            }
            else {
                // Mockets Configuration file
                if (updateInfoWithMocketsConfigFilePath (*pRemoteProxyInfo, sKey) < 0) {
                    checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                                    "error encountered while trying to parse Mockets configuration file "
                                    "from line <%s> in the config file\n", pszEntry);
                    return -6;
                }
            }

            sKeyValuePair = st.getNextToken();
        }

        if (!pRemoteProxyInfo->isLocalProxyReachableFromRemote() && !pRemoteProxyInfo->isRemoteProxyReachableFromLocalHost()) {
            checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                            "both local and remote reachability options for NetProxy with ID %u are set to false!\n",
                            pRemoteProxyInfo->getRemoteProxyID());
        }
        checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::parseAndAddEntry", Logger::L_Info,
                        "successfully completed the parsing of a new line for the NetProxy with UniqueID:Address pair <%u>:<%s>; "
                        "MocketServerPort=%hu - SocketServerPort=%hu - UDPServerPort=%hu\n", pRemoteProxyInfo->getRemoteProxyID(),
                        pRemoteProxyInfo->getRemoteProxyIPAddressAsString(), pRemoteProxyInfo->getRemoteProxyInetAddr (CT_MOCKETS)->getPort(),
                        pRemoteProxyInfo->getRemoteProxyInetAddr (CT_SOCKET)->getPort(), pRemoteProxyInfo->getRemoteProxyInetAddr (CT_UDP)->getPort());

        return 0;
    }

    int NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair (RemoteProxyInfo & remoteProxyInfo, const String & sKey, const String & sValue)
    {
        if ((sKey == NULL) || (sValue == NULL)) {
            return -1;
        }

        if (sKey == "mocketsport") {
            int iPortNumber = atoi (sValue);
            if ((iPortNumber <= 0) || (iPortNumber > 65535)) {
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Warning,
                                "impossible to correctly parse Mockets port number <%s>\n", sValue.c_str());
                return -2;
            }
            remoteProxyInfo.setRemoteServerPort (CT_MOCKETS, (uint16) iPortNumber);
        }
        else if (sKey == "tcpport") {
            int iPortNumber = atoi (sValue);
            if ((iPortNumber <= 0) || (iPortNumber > 65535)) {
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Warning,
                                "impossible to correctly parse TCP port number <%s>\n", sValue.c_str());
                return -3;
            }
            remoteProxyInfo.setRemoteServerPort (CT_SOCKET, (uint16) iPortNumber);
        }
        else if (sKey == "udpport") {
            int iPortNumber = atoi (sValue);
            if ((iPortNumber <= 0) || (iPortNumber > 65535)) {
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Warning,
                                "impossible to correctly parse UDP port number <%s>\n", sValue.c_str());
                return -4;
            }
            remoteProxyInfo.setRemoteServerPort (CT_UDP, (uint16) iPortNumber);
        }
        else if (sKey == "autoconnect") {
            AutoConnectionEntry *pAutoConnectionEntry = NULL;
            if (isConnectorTypeNameCorrect (sValue)) {
                ConnectorType connectorType = connectorTypeFromString (sValue);
                if (connectorType == CT_UDP) {
                    checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Warning,
                                    "impossible to set UDP as the protocol over which performing the connection to remote proxy; ignoring entry\n");
                    return 0;
                }

                AutoConnectionEntry autoConnectionEntry (remoteProxyInfo.getRemoteProxyID(), connectorType);
                if (P_CONNECTION_MANAGER->addOrUpdateAutoConnectionToList (autoConnectionEntry) > 0) {
                    checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Info,
                                    "successfully added a new entry to the AutoConnection Table for the NetProxy with UniqueID %u and address %s using "
                                    "protocol %s and a connection attempt timeout of %ums\n", remoteProxyInfo.getRemoteProxyID(),
                                    autoConnectionEntry.getRemoteProxyInetAddress()->getIPAsString(), connectorTypeToString (autoConnectionEntry.getConnectorType()),
                                    autoConnectionEntry.getAutoReconnectTimeInMillis());
                }
                else {
                    checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Info,
                                    "successfully updated the AutoConnection to the NetProxy at address %s to use protocol %s\n",
                                    autoConnectionEntry.getRemoteProxyInetAddress()->getIPAsString(),
                                    connectorTypeToString (autoConnectionEntry.getConnectorType()));
                }
                
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Warning,
                                "specified value %s is not a valid protocol to perform AutoConnections to the NetProxy at address %s\n",
                                sValue.c_str(), remoteProxyInfo.getRemoteProxyIPAddressAsString());
                return -5;
            }
        }
        else if (sKey == "reconnectinterval") {
            uint32 ui32ReconnectTime = atoui32 (sValue);
            if (ui32ReconnectTime == 0) {
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Warning,
                                "impossible to set value <%s> as AutoConnection timeout\n", sValue.c_str());
                return 0;
            }

            AutoConnectionEntry * const pAutoConnectionEntry = P_CONNECTION_MANAGER->getAutoConnectionEntryForProxyWithID (remoteProxyInfo.getRemoteProxyID());
            if (pAutoConnectionEntry) {
                pAutoConnectionEntry->setAutoReconnectTime (ui32ReconnectTime);
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Info,
                                "successfully updated the timeout of the AutoConnection to the NetProxy with UniqueID %u and address %s to %ums\n",
                                remoteProxyInfo.getRemoteProxyID(), pAutoConnectionEntry->getRemoteProxyInetAddress()->getIPAsString(), ui32ReconnectTime);
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Info,
                                "found a valid AutoConnection Timeout value, but it was not possible to find the "
                                "AutoConnection Entry to the remote NetProxy with UniqueID %u; ignoring option\n",
                                remoteProxyInfo.getRemoteProxyID());
            }

            return 0;
        }
        else if (sKey == "localreachability") {
            bool bIsReachable = (sValue == "true") || (sValue == "on") || (sValue == "yes") || (sValue == "1");
            remoteProxyInfo.setLocalProxyReachabilityFromRemote (bIsReachable);

            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::processKeyValuePairInConfigFileEntry", Logger::L_Info,
                            "reachability of local NetProxy from the remote one at address %s set to %s\n",
                            remoteProxyInfo.getRemoteProxyIPAddressAsString(), bIsReachable ? "TRUE" : "FALSE");
        }
        else if (sKey == "remotereachability") {
            bool bIsReachable = (sValue == "true") || (sValue == "on") || (sValue == "yes") || (sValue == "1");
            remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (bIsReachable);

            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::processKeyValuePairInConfigFileEntry", Logger::L_Info,
                            "reachability of remote NetProxy at address %s from local host set to %s\n",
                            remoteProxyInfo.getRemoteProxyIPAddressAsString(), bIsReachable ? "TRUE" : "FALSE");
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::processKeyValuePairInConfigFileEntry", Logger::L_Warning,
                            "unrecognized key %s found\n", sKey.c_str());
            return -6;
        }

        return 0;
    }

    int NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithMocketsConfigFilePath (RemoteProxyInfo & remoteProxyInfo, const String & sMocketsConfigFilePath)
    {
        if (sMocketsConfigFilePath.length() <= 0) {
            checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithMocketsConfigFilePath", Logger::L_Warning,
                            "no path specified for the Mockets config file for connections to remote NetProxy with IP address %s\n",
                            sMocketsConfigFilePath.c_str());
            return -1;
        }
        remoteProxyInfo.setMocketsConfFileName (sMocketsConfigFilePath);
        checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithMocketsConfigFilePath", Logger::L_Info,
                        "successfully added Mockets config file <%s> for connections to remote NetProxy with IP address %s\n",
                        sMocketsConfigFilePath.c_str(), remoteProxyInfo.getRemoteProxyIPAddressAsString());

        return 0;
    }

    int NetProxyConfigManager::EndpointConfigFileReader::EndpointConfigEntry::parseLine (const char *pszEntry)
    {
        if (pszEntry == NULL) {
            return -1;
        }
        char *pszEntryDup = strdup (pszEntry);
        char *pszSource = pszEntryDup;
        char *pszTemp = strchr (pszEntryDup, ' ');
        if (pszTemp == NULL) {
            return -2;
        }
        *pszTemp = '\0';
        char *pszDest = pszTemp + 1;
        pszTemp = strchr (pszDest, ' ' );
        if (pszTemp == NULL) {
            return -3;
        }
        *pszTemp = '\0';
        char *pszConfigParams = pszTemp + 1;
        _source = AddressRangeDescriptor (pszSource);
        if (!_source.isValid()) {
            return -4;
        }
        checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::parseLine", Logger::L_Info,
                        "parsed source Endpoint (source): lowest ip and port = %s, highest ip and port = %s\n",
                        _source.getLowestAddressAsString().c_str(), _source.getHighestAddressAsString().c_str());

        _destination = AddressRangeDescriptor (pszDest);
        if (!_destination.isValid()) {
            return -5;
        }
        checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::parseLine", Logger::L_Info,
                        "parsed destination Endpoint (destination): lowest ip and port = %s, highest ip and port = %s\n",
                        _destination.getLowestAddressAsString().c_str(),_destination.getHighestAddressAsString().c_str());

        if (_configParams.parse (pszConfigParams)) {
            return -6;
        }
        const CompressionSetting tcpCompressionSetting = _configParams.getCompressionSetting (IP_PROTO_TCP);
        const CompressionSetting udpCompressionSetting = _configParams.getCompressionSetting (IP_PROTO_UDP);
        if (tcpCompressionSetting.getCompressionType() == ProxyMessage::PMC_UncompressedData) {
            if (udpCompressionSetting.getCompressionType() == ProxyMessage::PMC_UncompressedData) {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::parseLine", Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s>; UDP compression algorithm = <%s>\n",
                                _configParams.getProtocolSetting (IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), udpCompressionSetting.getCompressionTypeAsString());
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::parseLine", Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s>; UDP compression algorithm = <%s> - level %hhu\n",
                                _configParams.getProtocolSetting (IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), udpCompressionSetting.getCompressionTypeAsString(),
                                udpCompressionSetting.getCompressionLevel());
            }
        }
        else {
            if (udpCompressionSetting.getCompressionType() == ProxyMessage::PMC_UncompressedData) {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::parseLine", Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s> - level %hhu; UDP compression algorithm = <%s>\n",
                                _configParams.getProtocolSetting (IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), tcpCompressionSetting.getCompressionLevel(),
                                udpCompressionSetting.getCompressionTypeAsString());
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::parseLine", Logger::L_Info,
                                "Parsed new Endpoint (config params): ICMP ==> %s, TCP ==> %s, UDP ==> %s; "
                                "TCP compression algorithm = <%s> - level %hhu; UDP compression algorithm = <%s> - level %hhu\n",
                                _configParams.getProtocolSetting (IP_PROTO_ICMP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_TCP).getProxyMessageProtocolAsString(),
                                _configParams.getProtocolSetting (IP_PROTO_UDP).getProxyMessageProtocolAsString(),
                                tcpCompressionSetting.getCompressionTypeAsString(), tcpCompressionSetting.getCompressionLevel(),
                                udpCompressionSetting.getCompressionTypeAsString(), udpCompressionSetting.getCompressionLevel());
            }
        }
        free (pszEntryDup);

        return 0;
    }

    int NetProxyConfigManager::EndpointConfigFileReader::parseEndpointConfigFile (const char * const pszPathToConfigFile)
    {
        int rc;
        char szLineBuf[MAX_LINE_LENGTH];
        FILE *file = fopen (pszPathToConfigFile, "r");
        if (file == NULL) {
            return -1;
        }
        while (NULL != fgets (szLineBuf, MAX_LINE_LENGTH, file)) {
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
            len = NetProxyConfigManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // Have a line that needs to be parsed
            if (0 != (rc = _endpointConfigTable[_endpointConfigTable.getHighestIndex() + 1].parseLine (szLineBuf))) {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigFileReader::parseConfigFile", Logger::L_MildError,
                                "failed to parse line <%s>\n", szLineBuf);
                fclose (file);
                return -3;
            }
        }

        fclose (file);
        return 0;
    }

    int NetProxyConfigManager::init (const char *pszHomeDir, const char *pszConfigFile)
    {
        int rc;
        _homeDir = pszHomeDir;
        checkAndLogMsg ("NetProxyConfigManager::init", Logger::L_Info,
                        "using <%s> as the home directory\n", pszHomeDir);
        checkAndLogMsg ("NetProxyConfigManager::init", Logger::L_Info,
                        "using <%s> as the config file\n", pszConfigFile);

        if (0 != (rc = ConfigManager::init())) {
            checkAndLogMsg ("NetProxyConfigManager::init", Logger::L_MildError,
                            "init() of ConfigManager failed with rc =  %d\n", rc);
            return -1;
        }

        if (0 != (rc = ConfigManager::readConfigFile (pszConfigFile, true))) {
            checkAndLogMsg ("NetProxyConfigManager::init", Logger::L_MildError,
                            "readConfigFile() of ConfigManager failed with rc = %d\n", rc);
            return -2;
        }

        return 0;
    }

    int NetProxyConfigManager::processConfigFiles (void)
    {
        int rc = 0;
        if (0 != (rc = processNetProxyConfigFile())) {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                            "failed to process the NetProxy config file; rc = %d\n", rc);
            return -1;
        }

        // Determine if there is an EndpointConfigFile specified
        const char *pszEndPointConfigFile = getValue ("EndPointConfigFile");
        if (pszEndPointConfigFile != NULL) {
            char szFile [PATH_MAX];
            if (_homeDir.length() > 0) {
                strcpy (szFile, _homeDir);
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, "conf");
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, pszEndPointConfigFile);
            }
            else {
                strcpy (szFile, pszEndPointConfigFile);
            }
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Info,
                            "using <%s> as the Endpoint config file\n", szFile);
            if (0 != (rc = _ecfr.parseEndpointConfigFile (szFile))) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Warning,
                                "failed to read Endpoint config file <%s>; rc = %d\n", szFile, rc);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                            "no path was specified for the Endpoint config file\n");
            return -2;
        }

        // Determine if there is a ProxyUniqueIDsConfigFile specified
        const char *pszUniqueIDConfigFile = getValue ("ProxyUniqueIDsConfigFile");
        if (pszUniqueIDConfigFile != NULL) {
            char szFile [PATH_MAX];
            if (_homeDir.length() > 0) {
                strcpy (szFile, _homeDir);
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, "conf");
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, pszUniqueIDConfigFile);
            }
            else {
                strcpy (szFile, pszUniqueIDConfigFile);
            }
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Info,
                            "using <%s> as the Unique ID config file\n", szFile);
            if (0 != (rc = _uicfr.parseUniqueIDsConfigFile (szFile))) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Warning,
                                "failed to read Unique ID config file <%s>; rc = %d\n", szFile, rc);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                            "no path was specified for the Unique ID config file\n");
            return -3;
        }

        // Determine if there is a AddressMappingConfigFileReaderFile specified
        const char *pszAddressMappingConfigFileReaderFile = getValue ("ProxyAddrMappingConfigFile");
        if (pszAddressMappingConfigFileReaderFile != NULL) {
            char szFile[PATH_MAX];
            if (_homeDir.length() > 0) {
                strcpy (szFile, _homeDir);
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, "conf");
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, pszAddressMappingConfigFileReaderFile);
            }
            else {
                strcpy (szFile, pszAddressMappingConfigFileReaderFile);
            }
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Info,
                            "using <%s> as the proxy Address Mapping config file\n", szFile);
            if (0 != (rc = _pamc.parseAddressMappingConfigFile (szFile))) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Warning,
                                "failed to read proxy Address Mapping config file <%s>; rc = %d\n", szFile, rc);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                            "no path was specified for the Address Mapping config file\n");
            return -4;
        }

        return 0;
    }

    int NetProxyConfigManager::processNetProxyConfigFile (void)
    {
        int rc;

        // Configure logging
        bool bConsoleLogging = false;
        pLogger->enableScreenOutput();
        if (hasValue ("consoleLogging")) {
            if (getValueAsBool ("consoleLogging")) {
                bConsoleLogging = true;
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "Logging to console disabled in the options\n");
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Warning,
                            "Logging to console was not specified in the options and it will be disabled\n");
        }

        if (hasValue ("logFile")) {
            // Log file
            char szLogFilePath[PATH_MAX];
            char szLogFileName[PATH_MAX];
            const char *pszLogFileName = getValue ("logFile");
            if (0 != stricmp (pszLogFileName, "<none>")) {
                if (0 == stricmp (pszLogFileName, "<generated>")) {
                    char szTimestamp[15] = "";
                    generateTimestamp (szTimestamp, sizeof (szTimestamp));
                    sprintf (szLogFileName, "netproxy.%s.log", szTimestamp);
                    if (_homeDir.length () > 0) {
                        strcpy (szLogFilePath, _homeDir);
                        strcat (szLogFilePath, getPathSepCharAsString());
                        strcat (szLogFilePath, NetProxyApplicationParameters::LOGS_DIR);
                        strcat (szLogFilePath, getPathSepCharAsString());
                        strcat (szLogFilePath, szLogFileName);
                    }
                }
                else {
                    strcpy (szLogFilePath, pszLogFileName);
                }
                if (0 != (rc = pLogger->initLogFile (szLogFilePath, false))) {
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Warning,
                                    "failed to initialize logging to file %s\n", szLogFilePath);
                    pLogger->disableFileOutput();
                }
                else {
                    pLogger->enableFileOutput();
                }
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "Logging to file disabled in the options\n");
                pLogger->disableFileOutput();
            }
        }

        if (hasValue ("logLevel")) {
            // Logging level
            int iLogLevel = getValueAsInt ("logLevel");
            if ((iLogLevel > 0) && (iLogLevel < 9)) {
                pLogger->setDebugLevel ((unsigned char) getValueAsInt ("logLevel"));
            }
            else {
                if (0 != pLogger->setDebugLevel (getValue ("logLevel", "L_Info"))) {
                    pLogger->setDebugLevel ("L_Info");
                }
            }
        }
        // Log these here so that they will be logged to the file if one has been setup
        checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                        "using <%s> as the home directory\n", (const char*) _homeDir);
        checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                        "using <%s> as the config file\n", _pszConfigFile);
        
        // Check if a Unique ID has been specified
        if (hasValue ("NetProxyUniqueID")) {
            NetProxyApplicationParameters::NETPROXY_UNIQUE_ID = getValueAsUInt32 ("NetProxyUniqueID");
        }

        // Check whether NetProxy is running in Gateway Mode
        if (hasValue ("GatewayMode")) {
            NetProxyApplicationParameters::GATEWAY_MODE = getValueAsBool ("GatewayMode");
        }
        else {
            NetProxyApplicationParameters::GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_GATEWAY_MODE;
        }
        checkAndLogMsg ("NetProxyConfigManager::processConfigFile",Logger::L_Info,
                        NetProxyApplicationParameters::GATEWAY_MODE ? "configured to run in gateway mode\n" : "configured to run in host mode with the TUN/TAP interface\n");

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            // Use default MTU size for now - ideally, should query from adapter
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "MFS of the INTERNAL network interface set to the default value of %hu bytes\n",
                            NetProxyApplicationParameters::ETHERNET_DEFAULT_MFS);
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "MFS of the EXTERNAL network interface set to the default value of %hu bytes\n",
                            NetProxyApplicationParameters::ETHERNET_DEFAULT_MFS);

            if (hasValue ("IPAddress")) {
                // IP Address of the external interface
                String sExternalInterfaceIPAddress = getValue ("IPAddress");
                if (sExternalInterfaceIPAddress.length() > 0) {
                    InetAddr externalInterfaceInetAddr (sExternalInterfaceIPAddress.c_str());
                    NetProxyApplicationParameters::NETPROXY_IP_ADDR = externalInterfaceInetAddr.getIPAddress();
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                    "IP Address of the external interface set to %s\n",
                                    externalInterfaceInetAddr.getIPAsString());
                }
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Warning,
                                "Impossible to set any value for the IP address of the external network interface; "
                                "NetProxy will try and query it from the adapter.\n");
            }

            if (hasValue ("Netmask")) {
                NetProxyApplicationParameters::NETPROXY_NETWORK_NETMASK = InetAddr(getValue ("Netmask")).getIPAddress();
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "IP address of the network netmask set to %s\n",
                                InetAddr(NetProxyApplicationParameters::NETPROXY_NETWORK_NETMASK).getIPAsString());
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Warning,
                                "Impossible to set any value for the external network netmask; "
                                "NetProxy will try and query it from the adapter.\n");
            }
            
            // Use configured value for the gateway
            if (hasValue ("GatewayAddress")) {
                NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR = InetAddr(getValue ("GatewayAddress")).getIPAddress();
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "IP address of the default network gateway set to %s\n",
                                InetAddr(NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR).getIPAsString());
            }
            else {
                // implement query from the adapter
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Warning,
                                "Impossible to set any value for the IP Address of the network gateway; "
                                "NetProxy will try and query it from the adapter.\n");
            }
        }
        else {
            // Check if the IP Address of the TAP Interface has not been retrieved, yet
            if (NetProxyApplicationParameters::NETPROXY_IP_ADDR == 0) {
                if (hasValue ("IPAddress")) {
                    // Value of the IP Address of the TUN/TAP Interface
                    String sTAPInterfaceIPAddress = getValue ("IPAddress");
                    if (sTAPInterfaceIPAddress.length() > 0) {
                        InetAddr tapInterfaceInetAddr (sTAPInterfaceIPAddress.c_str());
                        NetProxyApplicationParameters::NETPROXY_IP_ADDR = tapInterfaceInetAddr.getIPAddress();
                        checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                        "IP Address of the TUN/TAP interface set to %s\n",
                                        tapInterfaceInetAddr.getIPAsString());
                    }
                }
                else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Warning,
                                "Impossible to set any value for the IP address of the TUN/TAP interface; "
                                "NetProxy will try and query it from the adapter.\n");
                }
            }

            // Check if the MTU of the TAP Interface has not been retrieved, yet
            if (NetProxyApplicationParameters::TAP_INTERFACE_MTU == 0) {
                if (hasValue ("TAPInterfaceMTU")) {
                    // Value of the MTU of the TUN/TAP Interface
                    int iTAPInterfaceMTU = getValueAsInt ("TAPInterfaceMTU");
                    if ((iTAPInterfaceMTU > NetProxyApplicationParameters::TAP_INTERFACE_MIN_MTU) &&
                        (iTAPInterfaceMTU <= NetProxyApplicationParameters::TAP_INTERFACE_MAX_MTU)) {
                        NetProxyApplicationParameters::TAP_INTERFACE_MTU = (uint16) iTAPInterfaceMTU;
                        checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                        "MTU of the TUN/TAP interface set to %hu bytes\n",
                                        NetProxyApplicationParameters::TAP_INTERFACE_MTU);
                    }
                    else {
                        checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_SevereError,
                                        "invalid value (%d bytes) specified as MTU of the TUN/TAP interface\n",
                                        iTAPInterfaceMTU);
                        return -1;
                    }
                }
                else {
                    // Using default MTU size
                    NetProxyApplicationParameters::TAP_INTERFACE_MTU = NetProxyApplicationParameters::TAP_INTERFACE_DEFAULT_MTU;
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                    "MTU of the TUN/TAP interface set to the default value of %hu bytes\n",
                                    NetProxyApplicationParameters::TAP_INTERFACE_MTU);
                }
            }
        }

        if (hasValue ("MocketsListenPort")) {
            // Mockets server port number
            uint32 ui32PortNumber = getValueAsUInt32 ("MocketsListenPort");
            if ((ui32PortNumber > 0) && (ui32PortNumber <= 65535U)) {
                NetProxyApplicationParameters::MOCKET_SERVER_PORT = ui32PortNumber;
            }
            else {
                NetProxyApplicationParameters::MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
            }
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "set Mockets server port to %hu for incoming Mockets connections\n",
                            NetProxyApplicationParameters::MOCKET_SERVER_PORT);
        }
        else {
            NetProxyApplicationParameters::MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "using default Mockets server port of %hu for incoming Mockets connections\n",
                            NetProxyApplicationParameters::MOCKET_SERVER_PORT);
        }

        if (hasValue ("MocketsTimeout")) {
            // Timeout value for mockets to drop a connection
            uint32 ui32MocketsTimeout = getValueAsUInt32 ("MocketsTimeout");
            NetProxyApplicationParameters::MOCKET_TIMEOUT = ui32MocketsTimeout;
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "set mocket timeout to %lu milliseconds\n",
                            NetProxyApplicationParameters::MOCKET_TIMEOUT);
        }
        else {
            NetProxyApplicationParameters::MOCKET_TIMEOUT = NetProxyApplicationParameters::DEFAULT_MOCKET_TIMEOUT;
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "using default mocket timeout of %lu milliseconds\n",
                            NetProxyApplicationParameters::MOCKET_TIMEOUT);
        }

        if (hasValue ("TCPListenPort")) {
            // TCP port number
            uint32 ui32PortNumber = getValueAsUInt32 ("TCPListenPort");
            if ((ui32PortNumber > 0) && (ui32PortNumber <= 65535U)) {
                NetProxyApplicationParameters::TCP_SERVER_PORT = ui32PortNumber;
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "listening on port %hu for new TCP connections\n",
                            NetProxyApplicationParameters::TCP_SERVER_PORT);
        }

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
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "listening on port %hu for any incoming UDP datagram\n",
                            NetProxyApplicationParameters::UDP_SERVER_PORT);
        }

        // Enabled connectors
        String enabledConnectors (getValue ("EnabledConnectors"));
        if (enabledConnectors.length() <= 0) {
            enabledConnectors = String (NetworkConfigurationSettings::DEFAULT_ENABLED_CONNECTORS);
        }
        StringTokenizer st (enabledConnectors, ',');

        while (const char * const pszToken = st.getNextToken()) {
            if (pszToken == S_MOCKETS_CONNECTOR) {
                _enabledConnectorsSet.put (CT_MOCKETS);
            }
            else if (pszToken == S_SOCKET_CONNECTOR) {
                _enabledConnectorsSet.put (CT_SOCKET);
            }
            else if (pszToken == S_UDP_CONNECTOR) {
                _enabledConnectorsSet.put (CT_UDP);
            }
            else if (pszToken == S_CSR_CONNECTOR) {
                _enabledConnectorsSet.put (CT_CSR);
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Warning,
                                "impossible to recognize connector %s; ignoring entry\n", pszToken);
            }
        }

        // GUI
        if (hasValue ("GenerateGUIMessages")) {
            // GUI enabled/diabled boolean
            bool bRunUpdateGUIThread = getValueAsBool ("GenerateGUIMessages");
            NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED = bRunUpdateGUIThread;
        }
        checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                        "UpdateGUI Thread is %s\n",
                        NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED ? "enabled" : "disabled");

        if (hasValue ("GUIListenPort")) {
            // GUI local port number
            uint32 ui32GUIPortNumber = getValueAsInt ("GUIListenPort");
            if ((ui32GUIPortNumber > 0) && (ui32GUIPortNumber <= 65535U)) {
                while ((ui32GUIPortNumber == NetProxyApplicationParameters::MOCKET_SERVER_PORT) || (ui32GUIPortNumber == NetProxyApplicationParameters::UDP_SERVER_PORT)) {
                    ui32GUIPortNumber++;
                }
                NetProxyApplicationParameters::GUI_LOCAL_PORT = ui32GUIPortNumber;
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "GUI update packets will be sent to localhost on port %hu\n",
                                NetProxyApplicationParameters::GUI_LOCAL_PORT);
            }
        }
        else {
            while ((NetProxyApplicationParameters::GUI_LOCAL_PORT == NetProxyApplicationParameters::MOCKET_SERVER_PORT) ||
                    (NetProxyApplicationParameters::GUI_LOCAL_PORT == NetProxyApplicationParameters::UDP_SERVER_PORT)) {
                NetProxyApplicationParameters::GUI_LOCAL_PORT++;
            }
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "GUI update packets will be sent to localhost on port %hu\n",
                            NetProxyApplicationParameters::GUI_LOCAL_PORT);
        }

        // Check if a maximum transmit packet size has been specified
        const char *pMaxMsgLen = getValue ("maxMsgLen");
        if (pMaxMsgLen) {
            int iMaxMsgLen = atoi (pMaxMsgLen);
            if (iMaxMsgLen <= 0) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "entered an invalid value (%d bytes) as maximum transmit packet size; "
                                "the standard value of %hu bytes will be used\n",
                                iMaxMsgLen, NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            }
            else if (iMaxMsgLen > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = NetProxyApplicationParameters::PROXY_MESSAGE_MTU;
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "entered a too high value (%d bytes) as maximum transmit packet size; "
                                "the maximum allowed value (%hu bytes) will be used\n",
                                iMaxMsgLen, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            }
            else {
                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = iMaxMsgLen;
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "standard maximum transmit packet size set to %hu bytes\n",
                                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "standard maximum transmit packet size of %hu bytes will be used\n",
                            NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
        }

        // Check if timeout for the UDP Nagle's Algorithm has been specified
        bool bUDPNagleAlgorithmUsed = false;
        const char *pUDPNagleAlgorithmTimeout = getValue ("UDPNagleAlgorithmTimeout");
        if (pUDPNagleAlgorithmTimeout) {
            int iUDPNagleAlgorithmTimeout = atoi (pUDPNagleAlgorithmTimeout);
            if (iUDPNagleAlgorithmTimeout < 0) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "entered an invalid value (%d milliseconds) as timeout for the UDP Nagle's algorithm; algorithm will be disabled\n",
                                iUDPNagleAlgorithmTimeout);
            }
            else if (iUDPNagleAlgorithmTimeout > NetProxyApplicationParameters::MAX_UDP_NAGLE_ALGORITHM_TIMEOUT) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "entered an invalid value (%d milliseconds) as timeout for the UDP Nagle's algorithm;"
                                " algorithm will use maximum value (%u milliseconds)\n",
                                iUDPNagleAlgorithmTimeout, NetProxyApplicationParameters::MAX_UDP_NAGLE_ALGORITHM_TIMEOUT);
            }
            else {
                bUDPNagleAlgorithmUsed = true;
                NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT = iUDPNagleAlgorithmTimeout;
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "timeout for the UDP Nagle's algorithm set to %u milliseconds\n",
                                NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "UDP Nagle's algorithm will be disabled\n");
        }

        // Check if a threshold for the building and sending of a multiple UDP packet has been specified
        if (bUDPNagleAlgorithmUsed) {
            const char *pUDPNagleAlgorithmThreshold = getValue ("UDPNagleAlgorithmThreshold");
            if (pUDPNagleAlgorithmThreshold) {
                int iUDPNagleAlgorithmThreshold = atoi (pUDPNagleAlgorithmThreshold);
                if (iUDPNagleAlgorithmThreshold < 0) {
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                    "entered an invalid value (%d bytes) as the UDP Nagle's algorithm threshold; algorithm will use standard value %hu\n",
                                    iUDPNagleAlgorithmThreshold, NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }
                else if (iUDPNagleAlgorithmThreshold > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                    "entered a too high value (%d bytes) as the UDP Nagle's algorithm threshold; "
                                    "max allowed value is %hu; algorithm will use standard value %hu\n",
                                    iUDPNagleAlgorithmThreshold, NetProxyApplicationParameters::PROXY_MESSAGE_MTU,
                                    NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }
                else {
                    NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD = iUDPNagleAlgorithmThreshold;
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                    "UDP Nagle's algorithm threshold set to %u bytes\n",
                                    NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "UDP Nagle's algorithm will use the standard threshold (%hu bytes)\n",
                                NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
            }
        }

        // Check if a maximum transmit rate for UDPConnection class has been specified
        const char *pMaxUDPTransmitRate = getValue ("maxUDPTransmitRate");
        if (pMaxUDPTransmitRate) {
            int iMaxUDPTransmitRate = atoi (pMaxUDPTransmitRate);
            if (iMaxUDPTransmitRate < 0) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "entered an invalid value (%d bytes per second) as maximum UDP transmission rate; no limit will be used\n",
                                iMaxUDPTransmitRate);
            }
            else {
                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS = iMaxUDPTransmitRate;
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "maximum UDP transmission rate set to %u Bps (%.2f KBps, %.2f MBps)\n",
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS,
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS / 1024.0,
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS / (1024.0 * 1024.0));
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "no UDP transmission rate limit will be used\n");
        }

        // Check if the buffer size for the UDPConnection class has been specified
        const char *pUDPConnectionBufferSize = getValue ("UDPConnectionBufferSize");
        if (pUDPConnectionBufferSize) {
            int iUDPConnectionBufferSize = atoi (pUDPConnectionBufferSize);
            if (iUDPConnectionBufferSize < 0) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "entered an invalid value (%d bytes) as maximum UDP Connection buffer size; default value (%u bytes) will be used\n",
                                iUDPConnectionBufferSize, NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_BUFFER_SIZE);
            }
            else if (iUDPConnectionBufferSize > NetProxyApplicationParameters::MAX_UDP_CONNECTION_BUFFER_SIZE) {
                NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetProxyApplicationParameters::MAX_UDP_CONNECTION_BUFFER_SIZE;
                checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                "entered a too high value (%d bytes) as UDP Connection buffer size; the maximum value (%u bytes) will be used\n",
                                iUDPConnectionBufferSize, NetProxyApplicationParameters::MAX_UDP_CONNECTION_BUFFER_SIZE);
            }
            else {
                if ((NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT > 0) &&
                    (iUDPConnectionBufferSize < NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD)) {
                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                    "specified value is too low (the UDPNagleAlgorithmThreshold value of %hu bytes is the lowest limit)"
                                    " the maximum UDP Connection buffer size is set to %u bytes\n",
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
                else {
                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = iUDPConnectionBufferSize;
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                                    "maximum UDP Connection buffer size set to %u bytes\n",
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFile", Logger::L_Info,
                            "maximum UDP Connection buffer size set with default value of %u bytes\n",
                            NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_BUFFER_SIZE);
        }
        
        // Disable console logging if specified in the config file
        if (!bConsoleLogging) {
            pLogger->disableScreenOutput();
        }

        return 0;
    }

    unsigned int NetProxyConfigManager::trimConfigLine (char *pConfigLine)
    {
        size_t len = strlen (pConfigLine);
        char *pcCommentSymbol;

        // Perform a dos2unix eol conversion if necessary
        if ((len >= 2) && (pConfigLine[len-2] == '\r') && (pConfigLine[len-1] == '\n')) {
            pConfigLine[len-2] = '\0';
            len -= 2;
        }

        if ((len >= 1) && (pConfigLine[len-1] == '\n')) {
            pConfigLine[len-1] = '\0';
            len--;
        }

        if (pcCommentSymbol = strchr (pConfigLine, '#')) {
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

    int NetProxyConfigManager::splitIPFromPort (String sIPPortPair, char *pcKeyIPAddress, uint16 *pui16KeyPortNumber, bool bAllowWildcards)
    {
        if (sIPPortPair.length() == 0) {
            return -1;
        }

        String pcTempStr;
        StringTokenizer st (sIPPortPair.c_str(), ':');
        if (NULL != (pcTempStr = st.getNextToken())) {
            if (bAllowWildcards && (pcTempStr == "*")) {
                strcpy (pcKeyIPAddress, "0.0.0.0");
            }
            else if ((INADDR_NONE != inet_addr (pcTempStr.c_str())) && (INADDR_ANY != inet_addr (pcTempStr.c_str()))) {
                strcpy (pcKeyIPAddress, pcTempStr);
            }
            else {
                return -2;
            }
        }
        else {
            return -3;
        }

        if (NULL != (pcTempStr = st.getNextToken())) {
            if (bAllowWildcards && (pcTempStr == "*")) {
                *pui16KeyPortNumber = 0;
            }
            else {
                int32 i32PortNumber = atoi (pcTempStr);
                if ((i32PortNumber < 0) || (i32PortNumber > 65535)) {
                    return -4;
                }
                *pui16KeyPortNumber = (uint16) i32PortNumber;
            }
        }
        else {
            return -5;
        }

        return 0;
    }

}
