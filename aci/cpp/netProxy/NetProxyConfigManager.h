#ifndef INCL_NETPROXY_CONFIG_MANAGER_H
#define INCL_NETPROXY_CONFIG_MANAGER_H

/*
 * NetProxyConfigManager.h
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
 * Classes to parse the configuration files and store the acquired settings.
 */

#include <utility>

#include "FTypes.h"
#include "StrClass.h"
#include "UInt32Hashtable.h"
#include "ConfigManager.h"

#include "ConfigurationParameters.h"
#include "NPDArray2.h"
#include "AddressRangeDescriptor.h"
#include "RemoteProxyInfo.h"
#include "Connector.h"


namespace ACMNetProxy
{
    class NetProxyConfigManager : public NOMADSUtil::ConfigManager
    {
    public:
        class EndpointConfigParameters
        {
        public:
            EndpointConfigParameters (void);
            ~EndpointConfigParameters (void);

            // Parses the config parameters. They must be in the form <parameterName>=<value>;
            int parse (char *pszParamsEntry);

            const ProtocolSetting & getProtocolSetting (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;
            const CompressionSetting & getCompressionSetting (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;


        private:
			static const int readPriorityConfEntry(const char * const pcPriority);
            static const CompressionSetting readCompressionConfEntry (const char * const pcCompressionAlg);

            ProtocolSetting _psICMPProtocolSetting;
            ProtocolSetting _psTCPProtocolSetting;
            ProtocolSetting _psUDPProtocolSetting;
        };


    private:
        class AddressMappingConfigFileReader
        {
        public:
            AddressMappingConfigFileReader (void);
            ~AddressMappingConfigFileReader (void);

            int parseAddressMappingConfigFile (const char * const pszPathToConfigFile);


        private:
            friend class NetProxyConfigManager;

            int parseAndAddEntry (const char *pszEntry);

            static const int MAX_LINE_LENGTH = 2048;
        };


        class EndpointConfigFileReader
        {
            class EndpointConfigEntry
            {
            public:
                EndpointConfigEntry (void);
                ~EndpointConfigEntry (void);

                // Parses an entry in the form of a <SourceAddressDescriptor> <DestinationAddressDescriptor> <ConfigParameters>
                // Returns 0 if successful, or a negative value in case of error
                int parseLine (const char *pszEntry);

                const EndpointConfigParameters * const getConfigParams (void) const;
                bool matches (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr, uint16 ui16SourcePort = 0, uint16 ui16DestPort = 0) const;


            private:
                AddressRangeDescriptor _source;
                AddressRangeDescriptor _destination;
                EndpointConfigParameters _configParams;
            };


        public:
            EndpointConfigFileReader (void);
            virtual ~EndpointConfigFileReader (void);

            int parseEndpointConfigFile (const char * const pszPathToConfigFile);
            const EndpointConfigParameters * const getConfigParams (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr, uint16 ui16SourcePort = 0, uint16 ui16DestPort = 0) const;


        private:
            friend class NetProxyConfigManager;

            // To prevent erroneous method calls due to automatic type promotion
            template <typename InvalidT> const EndpointConfigParameters * const getConfigParams (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                                                                                                 InvalidT ui16SourcePort = 0, InvalidT ui16DestPort = 0);
            template <typename InvalidT> const EndpointConfigParameters * const getConfigParams (uint32 ui32SourceIPAddr, InvalidT ui32DestIPAddr,
                                                                                                 uint32 ui16SourcePort = 0, InvalidT ui16DestPort = 0);

            NPDArray2 <EndpointConfigEntry> _endpointConfigTable;

            static const int MAX_LINE_LENGTH = 2048;
        };


        class UniqueIDsConfigFileReader
        {
        public:
            UniqueIDsConfigFileReader(void);
            ~UniqueIDsConfigFileReader(void);

            int parseUniqueIDsConfigFile(const char * const pszPathToConfigFile);


        private:
            int parseAndAddEntry(const char *pszEntry);
            int updateInfoWithKeyValuePair(RemoteProxyInfo & remoteProxyInfo, const NOMADSUtil::String & sKey, const NOMADSUtil::String & sValue);
            int updateInfoWithMocketsConfigFilePath(RemoteProxyInfo & remoteProxyInfo, const NOMADSUtil::String & sMocketsConfigFilePath);

            static const int MAX_LINE_LENGTH = 2048;
            static const NOMADSUtil::String ACTIVE_CONNECTIVITY_CONFIG_PARAMETER;
            static const NOMADSUtil::String PASSIVE_CONNECTIVITY_CONFIG_PARAMETER;
            static const NOMADSUtil::String BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER;
        };


        class StaticARPTableConfigFileReader
        {
        public:
            StaticARPTableConfigFileReader (void);
            ~StaticARPTableConfigFileReader (void);

            int parseStaticARPTableConfigFile (const char * const pszPathToConfigFile);


        private:
            friend class NetProxyConfigManager;

            int parseAndAddEntry (const char *pszEntry) const;
            int parseEtherMACAddress (const char * const pcEtherMACAddressString, NOMADSUtil::EtherMACAddr * const pEtherMACAddress) const;

            static const int MAX_LINE_LENGTH = 2048;
        };


    public:
        static NetProxyConfigManager * const getNetProxyConfigManager (void);

        int init (const char *pszHomeDir, const char *pszConfigFile);
        int processConfigFiles (void);

        NOMADSUtil::UInt32Hashset * const getEnabledConnectorsSet (void);

        const ProtocolSetting * const mapAddrToProtocol (uint32 ulSrcIP, uint32 ulDstIP, NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;
        const ProtocolSetting * const mapAddrToProtocol (uint32 ulSrcIP, uint16 uhSrcPort, uint32 ulDstIP, uint16 uhDstPort, NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;


    private:
        friend class AddressRangeDescriptor;
        friend class AddressMappingConfigFileReader;
        friend class EndpointConfigFileReader;
        friend class StaticARPTableConfigFileReader;
        friend class PacketRouter;

        NetProxyConfigManager (void);
        explicit NetProxyConfigManager (const NetProxyConfigManager &rNetProxyConfigManager);       // Disallow object copy
        ~NetProxyConfigManager (void);

        int processNetProxyConfigFile (void);

        static unsigned int trimConfigLine (char *pConfigLine);
        static int splitIPFromPort (NOMADSUtil::String sIPPortPair, char *pcKeyIPAddress, uint16 *pui16KeyPortNumber, bool bAllowWildcards = true);

        NOMADSUtil::String _homeDir;
        NOMADSUtil::UInt32Hashset _enabledConnectorsSet;

        AddressMappingConfigFileReader _pamc;
        EndpointConfigFileReader _ecfr;
        UniqueIDsConfigFileReader _uicfr;
        StaticARPTableConfigFileReader _satcfr;

        static const NOMADSUtil::String S_MOCKETS_CONNECTOR;
        static const NOMADSUtil::String S_SOCKET_CONNECTOR;
        static const NOMADSUtil::String S_UDP_CONNECTOR;
        static const NOMADSUtil::String S_CSR_CONNECTOR;

        static ConnectionManager * const P_CONNECTION_MANAGER;
    };


    inline NetProxyConfigManager::EndpointConfigParameters::EndpointConfigParameters (void) :
        _psICMPProtocolSetting (*ProtocolSetting::getDefaultICMPProtocolSetting()), _psTCPProtocolSetting (*ProtocolSetting::getDefaultTCPProtocolSetting()),
        _psUDPProtocolSetting (*ProtocolSetting::getDefaultUDPProtocolSetting()) { }

    inline NetProxyConfigManager::EndpointConfigParameters::~EndpointConfigParameters (void) { }

    inline const ProtocolSetting & NetProxyConfigManager::EndpointConfigParameters::getProtocolSetting (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        switch (networkProtocol) {
        case NOMADSUtil::IP_PROTO_ICMP:
            return _psICMPProtocolSetting;
        case NOMADSUtil::IP_PROTO_UDP:
            return _psUDPProtocolSetting;
        case NOMADSUtil::IP_PROTO_TCP:
            return _psTCPProtocolSetting;
        default:
            break;
        }

        return ProtocolSetting::getInvalidProtocolSetting();
    }

    inline const CompressionSetting & NetProxyConfigManager::EndpointConfigParameters::getCompressionSetting (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        return getProtocolSetting (networkProtocol).getCompressionSetting();
    }

    inline NetProxyConfigManager::AddressMappingConfigFileReader::AddressMappingConfigFileReader (void) {}

    inline NetProxyConfigManager::AddressMappingConfigFileReader::~AddressMappingConfigFileReader (void) {}

    inline NetProxyConfigManager::UniqueIDsConfigFileReader::UniqueIDsConfigFileReader (void) { }

    inline NetProxyConfigManager::UniqueIDsConfigFileReader::~UniqueIDsConfigFileReader (void) { }

    inline NetProxyConfigManager::EndpointConfigFileReader::EndpointConfigEntry::EndpointConfigEntry (void) {}

    inline NetProxyConfigManager::EndpointConfigFileReader::EndpointConfigEntry::~EndpointConfigEntry (void) {}

    inline const NetProxyConfigManager::EndpointConfigParameters * const
        NetProxyConfigManager::EndpointConfigFileReader::EndpointConfigEntry::getConfigParams (void) const
    {
        return &_configParams;
    }

    inline bool NetProxyConfigManager::EndpointConfigFileReader::EndpointConfigEntry::matches (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                                                                                               uint16 ui16SourcePort, uint16 ui16DestPort) const
    {
        return (_source.matches (ui32SourceIPAddr, ui16SourcePort) && _destination.matches (ui32DestIPAddr, ui16DestPort));
    }

    inline NetProxyConfigManager::EndpointConfigFileReader::EndpointConfigFileReader (void) {}

    inline NetProxyConfigManager::EndpointConfigFileReader::~EndpointConfigFileReader (void) {}

    inline const NetProxyConfigManager::EndpointConfigParameters * const
        NetProxyConfigManager::EndpointConfigFileReader::getConfigParams (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr, uint16 ui16SourcePort, uint16 ui16DestPort) const
    {
        const EndpointConfigEntry *pEndpointConfigEntry = NULL;
        for (long i = 0; i <= _endpointConfigTable.getHighestIndex(); i++) {
            pEndpointConfigEntry = &_endpointConfigTable.get (i);
            if (pEndpointConfigEntry->matches (ui32SourceIPAddr, ui32DestIPAddr, ui16SourcePort, ui16DestPort)) {
                return pEndpointConfigEntry->getConfigParams();
            }
        }

        return NULL;
    }

    inline NetProxyConfigManager::StaticARPTableConfigFileReader::StaticARPTableConfigFileReader (void) {}

    inline NetProxyConfigManager::StaticARPTableConfigFileReader::~StaticARPTableConfigFileReader (void) {}

    inline NOMADSUtil::UInt32Hashset * const NetProxyConfigManager::getEnabledConnectorsSet (void)
    {
        return &_enabledConnectorsSet;
    }

    inline const ProtocolSetting * const NetProxyConfigManager::mapAddrToProtocol (uint32 ulSrcIP, uint32 ulDstIP, NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        const EndpointConfigParameters * const pEndpointConfigParameters = _ecfr.getConfigParams (ulSrcIP, ulDstIP);
        if (pEndpointConfigParameters == NULL) {
            return NULL;
        }

        return &(pEndpointConfigParameters->getProtocolSetting (networkProtocol));
    }

    inline const ProtocolSetting * const NetProxyConfigManager::mapAddrToProtocol (uint32 ulSrcIP, uint16 uhSrcPort, uint32 ulDstIP, uint16 uhDstPort,
                                                                                   NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        const EndpointConfigParameters * const pEndpointConfigParameters = _ecfr.getConfigParams (ulSrcIP, ulDstIP, uhSrcPort, uhDstPort);
        if (pEndpointConfigParameters == NULL) {
            return NULL;
        }

        return &(pEndpointConfigParameters->getProtocolSetting (networkProtocol));
    }

    inline NetProxyConfigManager::NetProxyConfigManager (void) { }

    inline NetProxyConfigManager * const NetProxyConfigManager::getNetProxyConfigManager (void)
    {
        static NetProxyConfigManager netProxyConfigManager;

        return &netProxyConfigManager;
    }

    inline NetProxyConfigManager::~NetProxyConfigManager (void) { }

}

#endif   // #ifndef INCL_NETPROXY_CONFIG_MANAGER_H
