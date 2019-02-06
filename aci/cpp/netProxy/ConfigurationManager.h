#ifndef INCL_NETPROXY_CONFIG_MANAGER_H
#define INCL_NETPROXY_CONFIG_MANAGER_H

/*
 * ConfigurationManager.h
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
 * Classes to parse the configuration files and store the acquired settings.
 */

#include <string>
#include <tuple>
#include <utility>

#include "FTypes.h"
#include "UInt32Hashset.h"
#include "ConfigManager.h"

#include "ConfigurationParameters.h"
#include "NetworkAddressRange.h"
#include "RemoteProxyInfo.h"
#include "ProtocolSetting.h"
#include "measure.pb.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class ARPCache;
    class ARPTableMissCache;
    class ConnectionManager;
    class StatisticsManager;

    class ConfigurationManager : public NOMADSUtil::ConfigManager
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
            const CompressionSettings & getCompressionSetting (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;
            EncryptionType getEncryptionType (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;


        private:
            static const uint8 readPriorityConfEntry (const char * const pcPriority);
            static const EncryptionType readEncryptionValue (const ci_string & sValue);
            static const CompressionSettings readCompressionConfEntry (const char * const pcCompressionAlg);


            ProtocolSetting _psICMPProtocolSetting;
            ProtocolSetting _psTCPProtocolSetting;
            ProtocolSetting _psUDPProtocolSetting;
        };


    private:
        class NetworkInterfaceDescriptionReader : public NOMADSUtil::ConfigManager
        {
        public:
            NetworkInterfaceDescriptionReader (const std::string & sInterfaceName);

            int init (const std::string & sConfigFilePath);
            NetworkInterfaceDescriptor parseConfigFile (const std::vector<std::string> & vComplementaryNetworkInterfacesList);


        private:
            const std::string _sInterfaceName;
        };


        class AddressMappingConfigFileReader
        {
        public:
            AddressMappingConfigFileReader (ConnectionManager & rConnectionManager, StatisticsManager & rStatisticsManager);
            ~AddressMappingConfigFileReader (void);

            int parseAddressMappingConfigFile (const char * const pszPathToConfigFile);
            void addAllAddressMappingEntriesToConnectionManager (void) const;

            std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>,
                                  std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr>>>
                getRemoteHostMappingList (void);
            std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>,
                                  std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr>>>
                getMulticastMappingList (void);

            static void buildMeasure (measure::Measure & m, const NetworkAddressRangeDescriptor & nardSource,
                                      const NetworkAddressRangeDescriptor & nardDestination, uint32 ui32RemoteProxyUID,
                                      const NOMADSUtil::InetAddr & iaRemoteInterfaceIPv4Address);


        private:
            int parseAndAddEntry (const char *pszEntry);
            int addAddressMappingToList (const NetworkAddressRange & sourceAddressRangeDescriptor,
                                         const NetworkAddressRange & destinationAddressRangeDescriptor,
                                         uint32 ui32RemoteProxyID, const NOMADSUtil::InetAddr & iaLocalInterfaceIPv4Address,
                                         const NOMADSUtil::InetAddr & iaRemoteInterfaceIPv4Address);

            int enforceAddressMappingConsistency (void);
            int enforceAddressMappingConsistencyImpl (void);

            static bool addressMappingIsAValidSpecializationOfMapping (const NetworkAddressRange & ardSourceGeneral,
                                                                       const NetworkAddressRange & ardDestinationGeneral,
                                                                       const NetworkAddressRange & ardSourceSpecialization,
                                                                       const NetworkAddressRange & ardDestinationSpecialization);


            std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr>>>
                _remoteHostAddressMappings;                                                                 // A list of all configured address mappings
            std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr>>>
                _multiBroadCastAddressMappings;                                                             // A list of all mappings for multi/broad-cast addresses

            ConnectionManager & _rConnectionManager;
            StatisticsManager & _rStatisticsManager;

            static const int MAX_LINE_LENGTH = 2048;
            static const std::string DEFAULT_SOURCE_ADDRESS_MAPPING;
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
                bool matches (const NetworkAddressRange & ardSource, const NetworkAddressRange & ardDestination) const;
                bool matchesAsSource (uint32 ui32SourceIPAddr, uint16 ui16SourcePort = 0) const;
                bool matchesAsSource (const NetworkAddressRange & ardSource) const;
                bool matchesAsDestination (uint32 ui32DestIPAddr, uint16 ui16DestPort = 0) const;
                bool matchesAsDestination (const NetworkAddressRange & ardDestination) const;

                const NetworkAddressRangeDescriptor & getNARDSource (void) const;
                const NetworkAddressRangeDescriptor & getNARDDestination (void) const;


            private:
                NetworkAddressRangeDescriptor _nardSource;
                NetworkAddressRangeDescriptor _nardDestination;
                EndpointConfigParameters _configParams;
            };


        public:
            EndpointConfigFileReader (StatisticsManager & rStatisticsManager);
            virtual ~EndpointConfigFileReader (void);

            int parseEndpointConfigFile (const char * const pszPathToConfigFile);

            const std::vector <EndpointConfigEntry> & getEndpointConfigTable (void) const;
            const EndpointConfigParameters * const getConfigParams (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                                                                    uint16 ui16SourcePort = 0, uint16 ui16DestPort = 0) const;

            static void buildMeasure (measure::Measure & m, const EndpointConfigEntry & entry);


        private:
            // To prevent erroneous method calls due to automatic type promotion
            template <typename InvalidT> const EndpointConfigParameters * const getConfigParams (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                                                                                                 InvalidT ui16SourcePort = 0, InvalidT ui16DestPort = 0) = delete;
            template <typename InvalidT> const EndpointConfigParameters * const getConfigParams (uint32 ui32SourceIPAddr, InvalidT ui32DestIPAddr,
                                                                                                 uint32 ui16SourcePort = 0, InvalidT ui16DestPort = 0) = delete;


            std::vector <EndpointConfigEntry> _vEndpointConfigTable;

            StatisticsManager & _rStatisticsManager;

            static const int MAX_LINE_LENGTH = 2048;
        };


        class UniqueIDsConfigFileReader
        {
        public:
            UniqueIDsConfigFileReader (ConnectionManager & rConnectionManager);
            ~UniqueIDsConfigFileReader (void);

            int parseUniqueIDsConfigFile (const char * const pszPathToConfigFile);


        private:
            int parseAndAddEntry (const char * pszEntry);
            int updateInfoWithKeyValuePair (RemoteProxyInfo & remoteProxyInfo, const std::string & sKey, const std::string & sValue);


            ConnectionManager & _rConnectionManager;

            static const int MAX_LINE_LENGTH = 2048;
            static const std::string ACTIVE_CONNECTIVITY_CONFIG_PARAMETER;
            static const std::string PASSIVE_CONNECTIVITY_CONFIG_PARAMETER;
            static const std::string BIDIRECTIONAL_CONNECTIVITY_CONFIG_PARAMETER;
        };


        class StaticARPTableConfigFileReader
        {
        public:
            StaticARPTableConfigFileReader (ARPCache & rAC);
            ~StaticARPTableConfigFileReader (void);

            int parseStaticARPTableConfigFile (const char * const pszPathToConfigFile);


        private:
            int parseAndAddEntry (const char * pszEntry);


            ARPCache & _rAC;

            static const int MAX_LINE_LENGTH = 2048;
        };


    public:
        ConfigurationManager (ARPCache & rAC, ARPTableMissCache & rATMC, ConnectionManager & rConnectionManager,
                              StatisticsManager & rStatisticsManager);
        explicit ConfigurationManager (const ConfigurationManager & rNetProxyConfigManager) = delete;
        ~ConfigurationManager (void);

        int init (const std::string & sHomeDir, const std::string & sConfigFile);
        int processConfigFiles (void);

        NOMADSUtil::UInt32Hashset * const getEnabledConnectorsSet (void);

        const ProtocolSetting * const mapAddrToProtocol (uint32 ulSrcIP, uint32 ulDstIP, NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;
        const ProtocolSetting * const mapAddrToProtocol (uint32 ulSrcIP, uint16 uhSrcPort, uint32 ulDstIP, uint16 uhDstPort,
                                                         NOMADSUtil::IP_PROTO_TYPE networkProtocol) const;


    private:
        int processMainConfigFile (void);
        NetworkInterfaceDescriptor parseNetworkInterfaceDetails (const std::string & sInterfaceName,
                                                                 const std::vector<std::string> & vComplementaryNetworkInterfacesList);
        void consolidateConfigurationSettings (void);

        static unsigned int trimConfigLine (char * const pConfigLine);


        std::string _sHomeDir;
        std::string _sConfigDir;
        NOMADSUtil::UInt32Hashset _enabledConnectorsSet;

        AddressMappingConfigFileReader _amcfr;
        EndpointConfigFileReader _ecfr;
        UniqueIDsConfigFileReader _uicfr;
        StaticARPTableConfigFileReader _satcfr;

        ARPCache & _rAC;
        ARPTableMissCache & _rATMC;
        ConnectionManager & _rConnectionManager;
        StatisticsManager & _rStatisticsManager;

        static const std::string S_TCP_CONNECTOR;
        static const std::string S_UDP_CONNECTOR;
        static const std::string S_MOCKETS_CONNECTOR;
        static const std::string S_CSR_CONNECTOR;
    };


    inline ConfigurationManager::EndpointConfigParameters::EndpointConfigParameters (void) :
        _psICMPProtocolSetting{*ProtocolSetting::getDefaultICMPProtocolSetting()}, _psTCPProtocolSetting{*ProtocolSetting::getDefaultTCPProtocolSetting()},
        _psUDPProtocolSetting{*ProtocolSetting::getDefaultUDPProtocolSetting()}
    { }

    inline ConfigurationManager::EndpointConfigParameters::~EndpointConfigParameters (void) { }

    inline const ProtocolSetting & ConfigurationManager::EndpointConfigParameters::getProtocolSetting (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
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

    inline const CompressionSettings & ConfigurationManager::EndpointConfigParameters::getCompressionSetting (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        return getProtocolSetting (networkProtocol).getCompressionSetting();
    }

    inline EncryptionType ConfigurationManager::EndpointConfigParameters::getEncryptionType (NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        return getProtocolSetting (networkProtocol).getEncryptionType();
    }

    inline const EncryptionType ConfigurationManager::EndpointConfigParameters::readEncryptionValue (const ci_string & sValue)
    {
        if (sValue == "dtls") {
            return ET_DTLS;
        }
        if (sValue == "plain") {
            return ET_PLAIN;
        }

        return ET_UNDEF;
    }

    inline ConfigurationManager::NetworkInterfaceDescriptionReader::NetworkInterfaceDescriptionReader (const std::string & sInterfaceName) :
        _sInterfaceName{sInterfaceName}
    { }

    inline ConfigurationManager::AddressMappingConfigFileReader::AddressMappingConfigFileReader (ConnectionManager & rConnectionManager,
                                                                                                 StatisticsManager & rStatisticsManager) :
        _rConnectionManager{rConnectionManager}, _rStatisticsManager{rStatisticsManager}
    { }

    inline ConfigurationManager::AddressMappingConfigFileReader::~AddressMappingConfigFileReader (void) { }

    inline std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr>>>
        ConfigurationManager::AddressMappingConfigFileReader::getRemoteHostMappingList (void)
    {
        return _remoteHostAddressMappings;
    }

    inline std::vector<std::pair<std::pair<NetworkAddressRange, NetworkAddressRange>, std::tuple<uint32, NOMADSUtil::InetAddr, NOMADSUtil::InetAddr>>>
        ConfigurationManager::AddressMappingConfigFileReader::getMulticastMappingList (void)
    {
        return _multiBroadCastAddressMappings;
    }

    inline int ConfigurationManager::AddressMappingConfigFileReader::enforceAddressMappingConsistency (void)
    {
        int rc;
        while ((rc = enforceAddressMappingConsistencyImpl()) > 0);

        return rc;
    }

    inline ConfigurationManager::UniqueIDsConfigFileReader::UniqueIDsConfigFileReader (ConnectionManager & rConnectionManager) :
        _rConnectionManager{rConnectionManager}
    { }

    inline ConfigurationManager::UniqueIDsConfigFileReader::~UniqueIDsConfigFileReader (void) { }

    inline ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::EndpointConfigEntry (void) { }

    inline ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::~EndpointConfigEntry (void) { }

    inline const ConfigurationManager::EndpointConfigParameters * const
        ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::getConfigParams (void) const
    {
        return &_configParams;
    }

    inline bool ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::matches (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                                                                                              uint16 ui16SourcePort, uint16 ui16DestPort) const
    {
        return matchesAsSource (ui32SourceIPAddr, ui16SourcePort) && matchesAsDestination (ui32DestIPAddr, ui16DestPort);
    }

    inline bool ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::matches (const NetworkAddressRange & ardSource,
                                                                                               const NetworkAddressRange & ardDestination) const
    {
        return matchesAsSource (ardSource) && matchesAsDestination (ardDestination);
    }

    inline bool ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::matchesAsSource (uint32 ui32SourceIPAddr, uint16 ui16SourcePort) const
    {
        return _nardSource.matches (ui32SourceIPAddr, ui16SourcePort);
    }

    inline bool ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::matchesAsDestination (uint32 ui32DestIPAddr, uint16 ui16DestPort) const
    {
        return _nardDestination.matches (ui32DestIPAddr, ui16DestPort);
    }

    inline bool ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::matchesAsSource (const NetworkAddressRange & ardSource) const
    {
        return ardSource.overlaps (_nardSource);
    }

    inline bool ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::matchesAsDestination (const NetworkAddressRange & ardDestination) const
    {
        return ardDestination.overlaps (_nardDestination);
    }

    inline const NetworkAddressRangeDescriptor & ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::getNARDSource (void) const
    {
        return _nardSource;
    }

    inline const NetworkAddressRangeDescriptor & ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry::getNARDDestination (void) const
    {
        return _nardDestination;
    }

    inline ConfigurationManager::EndpointConfigFileReader::EndpointConfigFileReader (StatisticsManager & rStatisticsManager) :
        _rStatisticsManager{rStatisticsManager}
    { }

    inline ConfigurationManager::EndpointConfigFileReader::~EndpointConfigFileReader (void) { }

    inline const ConfigurationManager::EndpointConfigParameters * const
        ConfigurationManager::EndpointConfigFileReader::getConfigParams (uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                                                                          uint16 ui16SourcePort, uint16 ui16DestPort) const
    {
        for (const auto & endpointConfigEntry : _vEndpointConfigTable) {
            if (endpointConfigEntry.matches (ui32SourceIPAddr, ui32DestIPAddr, ui16SourcePort, ui16DestPort)) {
                return endpointConfigEntry.getConfigParams();
            }
        }

        return nullptr;
    }

    inline const std::vector<ConfigurationManager::EndpointConfigFileReader::EndpointConfigEntry> &
        ConfigurationManager::EndpointConfigFileReader::getEndpointConfigTable (void) const
    {
        return _vEndpointConfigTable;
    }

    inline ConfigurationManager::StaticARPTableConfigFileReader::StaticARPTableConfigFileReader (ARPCache & rAC) :
        _rAC{rAC}
    { }

    inline ConfigurationManager::StaticARPTableConfigFileReader::~StaticARPTableConfigFileReader (void) { }

    inline ConfigurationManager::ConfigurationManager (ARPCache & rAC, ARPTableMissCache & rATMC, ConnectionManager & rConnectionManager,
                                                       StatisticsManager & rStatisticsManager) :
        _rConnectionManager{rConnectionManager}, _rStatisticsManager{rStatisticsManager}, _rAC{rAC},
        _rATMC{rATMC}, _amcfr{rConnectionManager, rStatisticsManager}, _ecfr{rStatisticsManager},
        _uicfr{rConnectionManager}, _satcfr{rAC}
    { }

    inline ConfigurationManager::~ConfigurationManager (void) { }

    inline NOMADSUtil::UInt32Hashset * const ConfigurationManager::getEnabledConnectorsSet (void)
    {
        return &_enabledConnectorsSet;
    }

    inline const ProtocolSetting * const ConfigurationManager::mapAddrToProtocol (uint32 ulSrcIP, uint32 ulDstIP, NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        const EndpointConfigParameters * const pEndpointConfigParameters = _ecfr.getConfigParams (ulSrcIP, ulDstIP);
        if (pEndpointConfigParameters == nullptr) {
            return nullptr;
        }

        return &(pEndpointConfigParameters->getProtocolSetting (networkProtocol));
    }

    inline const ProtocolSetting * const ConfigurationManager::mapAddrToProtocol (uint32 ulSrcIP, uint16 uhSrcPort, uint32 ulDstIP, uint16 uhDstPort,
                                                                                   NOMADSUtil::IP_PROTO_TYPE networkProtocol) const
    {
        const EndpointConfigParameters * const pEndpointConfigParameters = _ecfr.getConfigParams (ulSrcIP, ulDstIP, uhSrcPort, uhDstPort);
        if (pEndpointConfigParameters == nullptr) {
            return nullptr;
        }

        return &(pEndpointConfigParameters->getProtocolSetting (networkProtocol));
    }

    const NetworkInterfaceDescriptor & findNIDWithName (const std::string & sInterfaceName);
    const NetworkInterfaceDescriptor & findNIDWithIPv4Address (uint32 ui32IPv4Address);
    const NetworkInterfaceDescriptor & findNIDWithDefaultGatewayIPv4Address (uint32 ui32IPv4DefaultGatewayAddress);

}

#endif   // #ifndef INCL_NETPROXY_CONFIG_MANAGER_H
