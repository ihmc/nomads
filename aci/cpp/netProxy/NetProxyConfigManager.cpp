/*
 * NetProxyConfigManager.cpp
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
 */

#if defined (WIN32)
    #include <io.h>
#else
    #if !defined (ANDROID)
        #include <sys/io.h>
    #endif
    #include <limits.h>
#endif

#include <cstdlib>

#include "NLFLib.h"
#include "StringTokenizer.h"
#include "InetAddr.h"
#include "NetworkHeaders.h"
#include "Logger.h"

#include "NetProxyConfigManager.h"
#include "ConnectionManager.h"
#include "PacketRouter.h"

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
			/*
			else if (sKey == "tcppriority") {
				_psTCPProtocolSetting.setPrioritySetting(readPriorityConfEntry(sValue));
				checkAndLogMsg("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::readPriorityConfEntry", Logger::L_LowDetailDebug,
					"priority level %d will be used (only for TCP)\n",
					_psTCPProtocolSetting.getPrioritySetting());

			}
            else {
                checkAndLogMsg ("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::parse", Logger::L_Warning,
                                "key value %s not recognized\n", sKey.c_str());
            }
			*/
        }

        return 0;
    }

	const int NetProxyConfigManager::EndpointConfigParameters::readPriorityConfEntry(const char * const pcPriority)
	{
		// Format is <priority>
		String sPriority;
		StringTokenizer st(pcPriority, ':');
		sPriority = st.getNextToken();
		int iPriorityLevel = 0;
		if (sPriority) {
			iPriorityLevel = atoi(sPriority);
			if (iPriorityLevel < 0) {
				iPriorityLevel = 0;
			}
			else if (iPriorityLevel > 9) {
				iPriorityLevel = 9;
			}
		}
		checkAndLogMsg("NetProxyConfigManager::EndpointConfigEntry::EndpointConfigParameters::readPriorityConfEntry", Logger::L_MediumDetailDebug,
			"Entry's priority set up as:%d\n", iPriorityLevel);
		return iPriorityLevel;
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
                                Logger::L_Warning, "failed to parse line <%s>; rc = %d\n", szLineBuf, rc);
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
                                "failed to parse line <%s>; rc = %d\n", szLineBuf, rc);
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
        RemoteProxyInfo remoteProxyInfo (ui32UniqueID, static_cast<uint32> (remoteProxyIP.getIPAddress()));
        remoteProxyInfo.setMocketsConfFileName (NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE);        // Set the default Mockets Configuration file
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
                // Set a specific Mockets Configuration file for the remote host
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
                                    "impossible to set UDP as the protocol over which performing the connection to the remote NetProxy; ignoring entry\n");
                    return 0;
                }
                if (!remoteProxyInfo.isRemoteProxyReachableFromLocalHost()) {
                    checkAndLogMsg ("NetProxyConfigManager::UniqueIDsConfigFileReader::updateInfoWithKeyValuePair", Logger::L_Warning,
                                    "impossible to add an autoConnection entry for the remote NetProxy with address %s because its connectivity is set to PASSIVE\n",
                                    pAutoConnectionEntry->getRemoteProxyInetAddress()->getIPAsString());
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
        else if (sKey == "connectivity") {
            if (sValue == ACTIVE_CONNECTIVITY_CONFIG_PARAMETER) {
                remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (true);
                remoteProxyInfo.setLocalProxyReachabilityFromRemote (false);
            }
            else if (sValue == PASSIVE_CONNECTIVITY_CONFIG_PARAMETER) {
                remoteProxyInfo.setRemoteProxyReachabilityFromLocalHost (false);
                remoteProxyInfo.setLocalProxyReachabilityFromRemote (true);
                
                AutoConnectionEntry * const pAutoConnectionEntry = P_CONNECTION_MANAGER->getAutoConnectionEntryForProxyWithID (remoteProxyInfo.getRemoteProxyID());
                if (pAutoConnectionEntry) {
                    pAutoConnectionEntry->setInvalid();
                    checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::processKeyValuePairInConfigFileEntry", Logger::L_Warning,
                                    "autoConnection to the remote NetProxy with address %s is impossible because the specified connectivity is PASSIVE\n",
                                    remoteProxyInfo.getRemoteProxyIPAddressAsString());
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

                checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::processKeyValuePairInConfigFileEntry", Logger::L_Warning,
                                "could not interpret the connectivity value specified for remote NetProxy with address %s; specified value was %s, "
                                "admissible values are: %s, %s, %s; assuming %s\n", remoteProxyInfo.getRemoteProxyIPAddressAsString(), sValue.c_str(),
                                ACTIVE_CONNECTIVITY_CONFIG_PARAMETER.c_str(), PASSIVE_CONNECTIVITY_CONFIG_PARAMETER.c_str(),
                                BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER.c_str(), BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER.c_str());
                return 0;
            }

            checkAndLogMsg ("NetProxyConfigManager::AddressMappingConfigFileReader::processKeyValuePairInConfigFileEntry", Logger::L_Info,
                            "connectivity of local NetProxy to the remote one at address %s set to %s\n",
                            remoteProxyInfo.getRemoteProxyIPAddressAsString(), sValue.c_str());
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

    int NetProxyConfigManager::StaticARPTableConfigFileReader::parseStaticARPTableConfigFile (const char * const pszPathToConfigFile)
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
            len = NetProxyConfigManager::trimConfigLine (szLineBuf);
            if (len == 0) {
                continue;
            }

            // Have a line that needs to be parsed
            if (0 != (rc = parseAndAddEntry (szLineBuf))) {
                checkAndLogMsg ("NetProxyConfigManager::StaticARPTableConfigFileReader::parseStaticARPTableConfigFile",
                                Logger::L_Warning, "failed to parse line <%s>; rc = %d\n", szLineBuf, rc);
            }
        }

        return 0;
    }

    int NetProxyConfigManager::StaticARPTableConfigFileReader::parseAndAddEntry (const char * const pszEntry) const
    {
        if (pszEntry == NULL) {
            return -1;
        }

        // Entries in the format <IP MAC> (without '<' and '>' symbols)
        StringTokenizer st(pszEntry, ' ');
        // Get IP address
        const char * const pLocalIPAddress = st.getNextToken();
        if (pLocalIPAddress == NULL) {
            return -2;
        }
        // Get Ethernet MAC address
        const char * const pLocalEtherMACAddress = st.getNextToken();
        if (pLocalEtherMACAddress == NULL) {
            return -3;
        }

        InetAddr ipV4Address(pLocalIPAddress);
        EtherMACAddr etherMACAddress;
        if (parseEtherMACAddress (pLocalEtherMACAddress, &etherMACAddress) < 0) {
            checkAndLogMsg ("NetProxyConfigManager::StaticARPTableConfigFileReader::parseAndAddEntry", Logger::L_Warning,
                            "error parsing the Ethernet MAC address in line %s; ignoring entry\n", pszEntry);
            return -4;
        }

        PacketRouter::addEntryToARPTable (EndianHelper::ntohl (static_cast<uint32> (ipV4Address.getIPAddress())), etherMACAddress);

        return 0;
    }

    int NetProxyConfigManager::StaticARPTableConfigFileReader::parseEtherMACAddress(const char * const pcEtherMACAddressString, EtherMACAddr * const pEtherMACAddress) const
    {
        static const unsigned int MAC_ADDRESS_SIZE = 6;
        static const char AC_SEPARATOR[] = ":";
        const char *pszMACAddressOctets[MAC_ADDRESS_SIZE];
        char *pszTemp;

        if (!checkEtherMACAddressFormat (pcEtherMACAddressString)) {
            return -1;
        }
        // Parse Ethernet MAC address in the format A:B:C:D:E:F
        unsigned int i = 0;
        if ((pszMACAddressOctets[i++] = strtok_mt (pcEtherMACAddressString, AC_SEPARATOR, &pszTemp)) == NULL) {
            return -2;
        }
        for (; i < MAC_ADDRESS_SIZE; ++i) {
            pszMACAddressOctets[i] = strtok_mt (NULL, AC_SEPARATOR, &pszTemp);
            if (pszMACAddressOctets[i] == NULL) {
                return -2;
            }
        }

        uint8 *pui8EtherMACAddress = reinterpret_cast<uint8*> (pEtherMACAddress);
        for (i = 0; i < MAC_ADDRESS_SIZE; i += 2) {
            pui8EtherMACAddress[i] = static_cast<uint8> (std::strtoul(pszMACAddressOctets[i + 1], 0, 16));
            pui8EtherMACAddress[i + 1] = static_cast<uint8> (std::strtoul(pszMACAddressOctets[i], 0, 16));
        }

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

        // Determine if there is an Endpoint config file specified
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
                                "failed to read the proxy Endpoint config file at path <%s>; "
                                "rc = %d\n", szFile, rc);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                            "no path was specified for the proxy Endpoint config file\n");
            return -2;
        }

        // Determine if there is a NetProxy UniqueID config file specified
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
                                "failed to read the proxy Unique ID config file at path <%s>; "
                                "rc = %d\n", szFile, rc);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                            "no path was specified for the proxy Unique ID config file\n");
            return -3;
        }

        // Determine if there is an Address Mapping config file specified
        const char *pszAddressMappingConfigFile = getValue ("ProxyAddrMappingConfigFile");
        if (pszAddressMappingConfigFile != NULL) {
            char szFile[PATH_MAX];
            if (_homeDir.length() > 0) {
                strcpy (szFile, _homeDir);
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, "conf");
                strcat (szFile, getPathSepCharAsString());
                strcat (szFile, pszAddressMappingConfigFile);
            }
            else {
                strcpy (szFile, pszAddressMappingConfigFile);
            }
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Info,
                            "using <%s> as the proxy Address Mapping config file\n", szFile);
            if (0 != (rc = _pamc.parseAddressMappingConfigFile (szFile))) {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Warning,
                                "failed to read the proxy Address Mapping config file at path <%s>; "
                                "rc = %d\n", szFile, rc);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                            "no path was specified for the proxy Address Mapping config file\n");
            return -4;
        }

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            // Determine if there is a Static ARP Table config file specified
            const char *pszStaticARPTableConfigFile = getValue ("StaticARPTableConfigFile");
            if (pszStaticARPTableConfigFile != NULL) {
                char szFile[PATH_MAX];
                if (_homeDir.length() > 0) {
                    strcpy (szFile, _homeDir);
                    strcat (szFile, getPathSepCharAsString());
                    strcat (szFile, "conf");
                    strcat (szFile, getPathSepCharAsString());
                    strcat (szFile, pszStaticARPTableConfigFile);
                }
                else {
                    strcpy (szFile, pszStaticARPTableConfigFile);
                }
                checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Info,
                                "using <%s> as the proxy Static ARP Table config file\n", szFile);
                if (0 != (rc = _satcfr.parseStaticARPTableConfigFile (szFile))) {
                    checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_Warning,
                                    "failed to read the proxy Static ARP Table config file at path <%s>; "
                                    "rc = %d\n", szFile, rc);
                }
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processConfigFiles", Logger::L_MildError,
                                "no path was specified for the proxy Static ARP Table config file\n");
                return -4;
            }
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
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "Logging to console disabled in the options\n");
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
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
                    if (_homeDir.length() > 0) {
                        strcpy (szLogFilePath, _homeDir);
                        strcat (szLogFilePath, getPathSepCharAsString());
                        strcat (szLogFilePath, NetProxyApplicationParameters::LOGS_DIR);
                        strcat (szLogFilePath, getPathSepCharAsString());
                        strcat (szLogFilePath, szLogFileName);
                    }
					else {
						strcpy(szLogFilePath, "../log/");
						strcat(szLogFilePath, szLogFileName);
					}
                }
                else {
                    strcpy (szLogFilePath, pszLogFileName);
                }
                if (0 != (rc = pLogger->initLogFile (szLogFilePath, false))) {
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
                                    "failed to initialize logging to file %s\n", szLogFilePath);
                    pLogger->disableFileOutput();
                }
                else {
                    pLogger->enableFileOutput();
                }
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
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
        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                        "using <%s> as the home directory\n", (const char*) _homeDir);
        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                        "using <%s> as the config file\n", _pszConfigFile);

        // Check if a Unique ID has been specified
        if (hasValue ("NetProxyUniqueID")) {
            NetProxyApplicationParameters::NETPROXY_UNIQUE_ID = getValueAsUInt32 ("NetProxyUniqueID");
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "the NetProxy UniqueID is set to be %u\n",
                            NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
        }

        // Check whether NetProxy is running in Gateway Mode
        if (hasValue ("GatewayMode")) {
            NetProxyApplicationParameters::GATEWAY_MODE = getValueAsBool ("GatewayMode");
        }
        else {
            NetProxyApplicationParameters::GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_GATEWAY_MODE;
        }
        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile",Logger::L_Info,
                        NetProxyApplicationParameters::GATEWAY_MODE ? "configured to run in gateway mode\n" : "configured to run in host mode with the TUN/TAP interface\n");

        if (NetProxyApplicationParameters::GATEWAY_MODE) {
            // Use default MTU size for now - ideally, should query from adapter
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "MFS of the INTERNAL network interface set to the default value of %hu bytes\n",
                            NetProxyApplicationParameters::ETHERNET_DEFAULT_MFS);
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "MFS of the EXTERNAL network interface set to the default value of %hu bytes\n",
                            NetProxyApplicationParameters::ETHERNET_DEFAULT_MFS);

            if (hasValue("transparentGatewayMode")) {
                // Option to run NetProxy in transparent or non-transparent Gateway Mode
                bool bTransparentGatewayMode = getValueAsBool ("transparentGatewayMode");
                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE = bTransparentGatewayMode;

                checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                               "Transparent Gateway Mode set to %s: the TTL field of IP packets forwarded from one network to the other will %s\n",
                               NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? "TRUE" : "FALSE",
                               NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? "not be decremented" : "be decremented by one");
            }
            else {
                NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE = NetProxyApplicationParameters::DEFAULT_TRANSPARENT_GATEWAY_MODE;

                checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                               "The Transparent Gateway Mode option was not specified; using the default value %s: "
                               "the TTL field of IP packets forwarded from one network to the other will %s\n",
                               NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? "TRUE" : "FALSE",
                               NetProxyApplicationParameters::TRANSPARENT_GATEWAY_MODE ? "not be decremented" : "be decremented by one");
            }

            if (hasValue ("IPAddress")) {
                // IP Address of the external interface
                String sExternalInterfaceIPAddress = getValue ("IPAddress");
                if (sExternalInterfaceIPAddress.length() > 0) {
                    InetAddr externalInterfaceInetAddr (sExternalInterfaceIPAddress);
                    NetProxyApplicationParameters::NETPROXY_IP_ADDR = externalInterfaceInetAddr.getIPAddress();
                    NetProxyApplicationParameters::NETPROXY_EXTERNAL_IP_ADDR = NetProxyApplicationParameters::NETPROXY_IP_ADDR;
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "IP Address of the external interface set to %s\n",
                                    externalInterfaceInetAddr.getIPAsString());
                }
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
                                "Impossible to set any value for the IP address of the external network interface; "
                                "NetProxy will try and query it from the adapter.\n");
            }

            if (hasValue ("Netmask")) {
                NetProxyApplicationParameters::NETPROXY_NETWORK_NETMASK = InetAddr(getValue ("Netmask")).getIPAddress();
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "IP address of the network netmask set to %s\n",
                                InetAddr(NetProxyApplicationParameters::NETPROXY_NETWORK_NETMASK).getIPAsString());
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
                                "Impossible to set any value for the external network netmask; "
                                "NetProxy will try and query it from the adapter.\n");
            }

            // Use configured value for the gateway
            if (hasValue ("GatewayAddress")) {
                NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR = InetAddr(getValue ("GatewayAddress")).getIPAddress();
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "IP address of the default network gateway set to %s\n",
                                InetAddr(NetProxyApplicationParameters::NETWORK_GATEWAY_NODE_IP_ADDR).getIPAsString());
            }
            else {
                // implement query from the adapter
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
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
                        InetAddr tapInterfaceInetAddr (sTAPInterfaceIPAddress);
                        NetProxyApplicationParameters::NETPROXY_IP_ADDR = tapInterfaceInetAddr.getIPAddress();
                        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                        "IP Address of the TUN/TAP interface set to %s\n",
                                        tapInterfaceInetAddr.getIPAsString());
                    }
                }
                else {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
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
                        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                        "MTU of the TUN/TAP interface set to %hu bytes\n",
                                        NetProxyApplicationParameters::TAP_INTERFACE_MTU);
                    }
                    else {
                        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_SevereError,
                                        "invalid value (%d bytes) specified as MTU of the TUN/TAP interface\n",
                                        iTAPInterfaceMTU);
                        return -1;
                    }
                }
                else {
                    // Using default MTU size
                    NetProxyApplicationParameters::TAP_INTERFACE_MTU = NetProxyApplicationParameters::TAP_INTERFACE_DEFAULT_MTU;
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
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
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "set Mockets server port to %hu for incoming Mockets connections\n",
                            NetProxyApplicationParameters::MOCKET_SERVER_PORT);
        }
        else {
            NetProxyApplicationParameters::MOCKET_SERVER_PORT = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT;
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "using default Mockets server port of %hu for incoming Mockets connections\n",
                            NetProxyApplicationParameters::MOCKET_SERVER_PORT);
        }

        if (hasValue ("MocketsTimeout")) {
            // Timeout value for mockets to drop a connection
            uint32 ui32MocketsTimeout = getValueAsUInt32 ("MocketsTimeout");
            NetProxyApplicationParameters::MOCKET_TIMEOUT = ui32MocketsTimeout;
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "set Mockets timeout to %lu milliseconds\n",
                            NetProxyApplicationParameters::MOCKET_TIMEOUT);
        }
        else {
            NetProxyApplicationParameters::MOCKET_TIMEOUT = NetProxyApplicationParameters::DEFAULT_MOCKET_TIMEOUT;
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "using default Mockets timeout of %lu milliseconds\n",
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
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
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
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "listening on port %hu for any incoming UDP datagram\n",
                            NetProxyApplicationParameters::UDP_SERVER_PORT);
        }

        if (hasValue ("DefaultMocketsConfigFile")) {
            // Read default path for the Mockets configuration file
            const String sDefaultMocketsConfigFile (getValue ("DefaultMocketsConfigFile"));
            if (sDefaultMocketsConfigFile.length() > 0) {
                NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE = sDefaultMocketsConfigFile;
            }
        }
        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                        "default Mockets parameter settings will be loaded from file %s\n",
                        NetProxyApplicationParameters::DEFAULT_MOCKETS_CONFIG_FILE.c_str());

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
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
                                "impossible to recognize connector %s; ignoring entry\n", pszToken);
            }
        }

        // Forwarding of packets with a Multicast/Broadcast MAC destination address on the external interface
        if (hasValue ("forwardMulticastPacketsOnExternalInterface")) {
            bool bMCastPacketForwarding = getValueAsBool ("forwardMulticastPacketsOnExternalInterface");
            NetProxyApplicationParameters::MULTICAST_PACKETS_FORWARDING_ON_EXTERNAL_INTERFACE = bMCastPacketForwarding;
        }
        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                        "Forwarding of Multicast MAC Packets on the external interface is %s\n",
                        NetProxyApplicationParameters::MULTICAST_PACKETS_FORWARDING_ON_EXTERNAL_INTERFACE ? "enabled" : "disabled");

        if (hasValue ("forwardBroadcastPacketsOnExternalInterface")) {
            bool bBCastPacketForwarding = getValueAsBool ("forwardBroadcastPacketsOnExternalInterface");
            NetProxyApplicationParameters::BROADCAST_PACKETS_FORWARDING_ON_EXTERNAL_INTERFACE = bBCastPacketForwarding;
        }
        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                        "Forwarding of Broadcast MAC Packets on the external interface is %s\n",
                        NetProxyApplicationParameters::BROADCAST_PACKETS_FORWARDING_ON_EXTERNAL_INTERFACE ? "enabled" : "disabled");

        // GUI
        if (hasValue ("GenerateGUIMessages")) {
            // GUI enabled/diabled boolean
            bool bRunUpdateGUIThread = getValueAsBool ("GenerateGUIMessages");
            NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED = bRunUpdateGUIThread;
        }
        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                        "UpdateGUI Thread is %s\n",
                        NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED ? "enabled" : "disabled");

        if (NetProxyApplicationParameters::UPDATE_GUI_THREAD_ENABLED) {
            // Parse addresses to which status updates need to be sent
            String sCleanAddressList, sAddressList (getValue ("StatusNotificationAddresses", "127.0.0.1"));
            StringTokenizer stAddressList (sAddressList, ',');

            while (const char * const pszPairToken = stAddressList.getNextToken()) {
                StringTokenizer stPair (pszPairToken, ':');
                String sIP = stPair.getNextToken();
                String sPort = stPair.getNextToken();
                uint32 ui32GUIPortNumber = 0;
                char pszGUIPortNumber[6];
                memset (pszGUIPortNumber, 0, 6);

                if (sIP.length() <= 0) {
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
                                    "found an empty entry in the StatusNotificationAddresses list\n");
                    continue;
                }
                if (!InetAddr::isIPv4Addr (sIP)) {
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
                                    "entry <%s> in the StatusNotificationAddresses list does not contain a valid IP address\n",
                                    pszPairToken);
                    continue;
                }
                if (sPort.length() <= 0) {
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "port not specified for address %s in the StatusNotificationAddresses list; using the default port %hu\n",
                                    sIP.c_str(), NetProxyApplicationParameters::GUI_LOCAL_PORT);
                    ui32GUIPortNumber = NetProxyApplicationParameters::GUI_LOCAL_PORT;
                }
                else {
                    // Parsing port number
                    ui32GUIPortNumber = atoi (sPort);
                    if (ui32GUIPortNumber > 65535U) {
                        checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Warning,
                                        "%u is not an acceptable port number; the default value %hu will be used, instead, for notifications sent to %s\n",
                                        ui32GUIPortNumber, NetProxyApplicationParameters::GUI_LOCAL_PORT, sIP.c_str());
                        ui32GUIPortNumber = NetProxyApplicationParameters::GUI_LOCAL_PORT;
                    }
                }
                sPort = itoa (pszGUIPortNumber, ui32GUIPortNumber);

                sCleanAddressList += ((sCleanAddressList.length() > 0) ? "," : "") + sIP + ":" + sPort;
            }

            // Change set value with the clean one
            setValue ("StatusNotificationAddresses", sCleanAddressList);
        }

        // Check if a maximum transmit packet size has been specified
        const char *pMaxMsgLen = getValue ("maxMsgLen");
        if (pMaxMsgLen) {
            int iMaxMsgLen = atoi (pMaxMsgLen);
            if (iMaxMsgLen <= 0) {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "entered an invalid value (%d bytes) as maximum transmit packet size; "
                                "the default value of %hu bytes will be used\n", iMaxMsgLen,
                                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            }
            else if (iMaxMsgLen > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = NetProxyApplicationParameters::PROXY_MESSAGE_MTU;
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "entered a too high value (%d bytes) as maximum transmit packet size; "
                                "the maximum allowed value (%hu bytes) will be used\n", iMaxMsgLen,
                                NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
            }
            else {
                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE = iMaxMsgLen;
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "default maximum transmit packet size set to %hu bytes\n",
                                NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "default maximum transmit packet size of %hu bytes will be used\n",
                            NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
        }

        // Check if a maximum transmit packet size has been specified
        const char *pVrtConnEstTimeout = getValue("virtualConnectionEstablishmentTimeout");
        if (pVrtConnEstTimeout) {
            int iVrtConnEstTimeout = atoi(pVrtConnEstTimeout);
            if (iVrtConnEstTimeout <= 0) {
                checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                               "entered an invalid value (%dms) as timeout for virtual connection establishment; "
                               "the default value of %u bytes will be used\n", iVrtConnEstTimeout,
                               NetProxyApplicationParameters::DEFAULT_VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT);
            }
            else {
                NetProxyApplicationParameters::VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT = iVrtConnEstTimeout;
                checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                               "timeout for virtual connection establishment set to %ums\n",
                               NetProxyApplicationParameters::VIRTUAL_CONN_ESTABLISHMENT_TIMEOUT);
            }
        }
        else {
            checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                           "no virtual connection establishment timeout specified; "
                           "the default value of %ums will be used\n",
                           NetworkConfigurationSettings::MAX_TCP_DATA_PROXY_MESSAGE_PAYLOAD_SIZE);
        }

        // Check if timeout for the UDP Nagle's Algorithm has been specified
        bool bUDPNagleAlgorithmUsed = false;
        const char *pcUDPNagleAlgorithmTimeout = getValue ("UDPNagleAlgorithmTimeout");
        if (pcUDPNagleAlgorithmTimeout) {
            int iUDPNagleAlgorithmTimeout = atoi (pcUDPNagleAlgorithmTimeout);
            if (iUDPNagleAlgorithmTimeout < 0) {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "entered an invalid value (%d milliseconds) as the timeout for the UDP Nagle's algorithm; "
                                "the algorithm will be disabled\n", iUDPNagleAlgorithmTimeout);
            }

            const uint32 ui32UDPNagleAlgorithmTimeout = static_cast<uint32> (iUDPNagleAlgorithmTimeout);
            if (ui32UDPNagleAlgorithmTimeout > NetProxyApplicationParameters::MAX_UDP_NAGLE_ALGORITHM_TIMEOUT) {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "entered a value for the timeout of the UDP Nagle's algorithm that is too high (%u milliseconds); "
                                "the proxy will use the maximum value allowed (%u milliseconds)\n", ui32UDPNagleAlgorithmTimeout,
                                NetProxyApplicationParameters::MAX_UDP_NAGLE_ALGORITHM_TIMEOUT);
            }
            else {
                bUDPNagleAlgorithmUsed = true;
                NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT = iUDPNagleAlgorithmTimeout;
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "timeout for the UDP Nagle's algorithm set to %u milliseconds\n",
                                NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT);
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "UDP Nagle's algorithm disabled\n");
        }

        // Check if a threshold for the building and sending of a multiple UDP packet has been specified
        if (bUDPNagleAlgorithmUsed) {
            const char *pUDPNagleAlgorithmThreshold = getValue ("UDPNagleAlgorithmThreshold");
            if (pUDPNagleAlgorithmThreshold) {
                int iUDPNagleAlgorithmThreshold = atoi (pUDPNagleAlgorithmThreshold);
                if (iUDPNagleAlgorithmThreshold < 0) {
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "entered an invalid value (%d bytes) as the UDP Nagle's algorithm threshold; "
                                    "algorithm will use the default value of %hu bytes\n", iUDPNagleAlgorithmThreshold,
                                    NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }

                const uint32 ui32UDPNagleAlgorithmThreshold = static_cast<uint32> (iUDPNagleAlgorithmThreshold);
                if (ui32UDPNagleAlgorithmThreshold > NetProxyApplicationParameters::PROXY_MESSAGE_MTU) {
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "entered a value for the UDP Nagle's algorithm threshold that is too high (%u bytes); "
                                    "max allowed value is %hu; algorithm will use the default value of %hu bytes\n",
                                    ui32UDPNagleAlgorithmThreshold, NetProxyApplicationParameters::PROXY_MESSAGE_MTU,
                                    NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }
                else {
                    NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD = static_cast<uint16> (ui32UDPNagleAlgorithmThreshold);
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "UDP Nagle's algorithm threshold set to %u bytes\n",
                                    NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
                }
            }
            else {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "UDP Nagle's algorithm will use the default threshold value of %hu bytes\n",
                                NetworkConfigurationSettings::DEFAULT_MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD);
            }
        }

		if (hasValue("NETSENSOR_CONFIG_FILE")) {
			strcpy (NetProxyApplicationParameters::NETSENSOR_CONFIG_FILE, getValue ("NETSENSOR_CONFIG_FILE"));

			checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
				            "NetSensor cfg file path specified to: %s\n",
                            NetProxyApplicationParameters::NETSENSOR_CONFIG_FILE);
		}
		else {
			checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
				            "NetSensor configuration file path not specified, using default value: %s\n",
                            NetProxyApplicationParameters::NETSENSOR_CONFIG_FILE);

		}

		if (hasValue("activate_netsensor")) {
			NetProxyApplicationParameters::ACTIVATE_NETSENSOR = getValueAsBool("activate_netsensor");
			if (NetProxyApplicationParameters::ACTIVATE_NETSENSOR) {
				checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
					"NetSensor active\n");
			}
			else {
				checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
					"NetSensor deactivated from netproxy cfg file\n");
			}
		}
		else {
			checkAndLogMsg("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
				"activate_netsensor field not present, netproxy will activate netsensor anyway.\n");
			NetProxyApplicationParameters::ACTIVATE_NETSENSOR = true;
		}


        // Check if a maximum transmit rate for UDPConnection class has been specified
        const char *pMaxUDPTransmitRate = getValue ("maxUDPTransmitRate");
        if (pMaxUDPTransmitRate) {
            int iMaxUDPTransmitRate = atoi (pMaxUDPTransmitRate);
            if (iMaxUDPTransmitRate < 0) {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "entered an invalid value (%d bytes per second) as maximum UDP transmission rate; no limit will be used\n",
                                iMaxUDPTransmitRate);
            }
            else {
                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS = iMaxUDPTransmitRate;
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "maximum UDP transmission rate set to %u Bps (%.2f KBps, %.2f MBps)\n",
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS,
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS / 1024.0,
                                NetworkConfigurationSettings::UDP_CONNECTION_THROUGHPUT_LIMIT_IN_BPS / (1024.0 * 1024.0));
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "no UDP transmission rate limit will be used\n");
        }

        // Check if the buffer size for the UDPConnection class has been specified
        const char *pcUDPConnectionBufferSize = getValue ("UDPConnectionBufferSize");
        if (pcUDPConnectionBufferSize) {
            int iUDPConnectionBufferSize = atoi (pcUDPConnectionBufferSize);
            if (iUDPConnectionBufferSize < 0) {
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "entered an invalid value (%d bytes) as the maximum buffer size for UDP connections; ",
                                "default value (%u bytes) will be used\n", iUDPConnectionBufferSize,
                                NetworkConfigurationSettings::DEFAULT_UDP_CONNECTION_BUFFER_SIZE);
            }

            const uint32 ui32UDPConnectionBufferSize = static_cast<uint32> (iUDPConnectionBufferSize);
            if (ui32UDPConnectionBufferSize > NetProxyApplicationParameters::MAX_UDP_CONNECTION_BUFFER_SIZE) {
                NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetProxyApplicationParameters::MAX_UDP_CONNECTION_BUFFER_SIZE;
                checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                "entered a value for the buffer size of UDP connections that is too high (%u bytes); "
                                "the maximum value (%u bytes) will be used instead\n", ui32UDPConnectionBufferSize,
                                NetProxyApplicationParameters::MAX_UDP_CONNECTION_BUFFER_SIZE);
            }
            else {
                if ((NetworkConfigurationSettings::UDP_NAGLE_ALGORITHM_TIMEOUT > 0) &&
                    (ui32UDPConnectionBufferSize < NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD)) {
                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = NetworkConfigurationSettings::MULTIPLE_UDP_DATAGRAMS_PACKET_THRESHOLD;
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "the specified buffer size for UDP connections is lower than the UDPNagleAlgorithmThreshold value; "
                                    "the proxy will use the same value (%u bytes) also for the buffer size of UDP connections\n",
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
                else if (ui32UDPConnectionBufferSize == 0) {
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "entered an invalid value (%u bytes) as the maximum buffer size for UDP connections; "
                                    "the default value of %u bytes will be used instead\n", ui32UDPConnectionBufferSize,
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
                else {
                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE = ui32UDPConnectionBufferSize;
                    checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                                    "maximum UDP connection buffer size set to %u bytes\n",
                                    NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE);
                }
            }
        }
        else {
            checkAndLogMsg ("NetProxyConfigManager::processNetProxyConfigFile", Logger::L_Info,
                            "maximum UDP connection buffer size set to the default value (%u bytes)\n",
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

    int NetProxyConfigManager::splitIPFromPort (String sIPPortPair, char *pcKeyIPAddress, uint16 *pui16KeyPortNumber, bool bAllowWildcards)
    {
        if (sIPPortPair.length() == 0) {
            return -1;
        }

        String pcTempStr;
        StringTokenizer st (sIPPortPair, ':');
        if (NULL != (pcTempStr = st.getNextToken())) {
            if (bAllowWildcards && (pcTempStr == "*")) {
                strcpy (pcKeyIPAddress, "0.0.0.0");
            }
            else if ((INADDR_NONE != inet_addr (pcTempStr)) && (INADDR_ANY != inet_addr (pcTempStr))) {
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
