#ifndef INCL_REMOTE_PROXY_INFO_H
#define INCL_REMOTE_PROXY_INFO_H

/*
 * RemoteProxyInfo.h
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
 * Class that stores information related to remote NetProxies.
 */

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "InetAddr.h"

#include "ConfigurationParameters.h"
#include "Utilities.h"


namespace ACMNetProxy
{
    class ConnectionManager;


    class RemoteProxyInfo
    {
    public:
        RemoteProxyInfo (void);
        RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<NOMADSUtil::InetAddr> & remoteMocketsServerAddr,
                         const std::vector<NOMADSUtil::InetAddr> & remoteTCPServerAddr,
                         const std::vector<NOMADSUtil::InetAddr> & remoteUDPServerAddr, const char * const pszMocketsConfFileName);
        RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<uint32> & vui32InterfaceIPv4AddressList,
                         uint16 ui16RemoteMocketsPort = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT,
                         uint16 ui16RemoteTCPPort = NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT,
                         uint16 ui16RemoteUDPPort = NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT,
                         const char * const pszMocketsConfFileName = nullptr);
        RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<NOMADSUtil::InetAddr> & remoteHostAddrList,
                         const char * const pszMocketsConfFileName);
        RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<NOMADSUtil::InetAddr> & remoteHostAddrList,
                         const std::vector<std::string> & vsRemoteHostInterfaceNames, const char * const pszMocketsConfFileName);
        RemoteProxyInfo (const RemoteProxyInfo & rhsEntry);
        ~RemoteProxyInfo (void);

        const RemoteProxyInfo & operator = (const RemoteProxyInfo & rhsEntry);

        const uint32 getRemoteNetProxyID (void) const;
        const std::unordered_set<uint32> getRemoteProxyIPAddressList (void) const;
        const NOMADSUtil::InetAddr getIPAddressOfInterfaceWithLabel (const std::string & sInterfaceLabel) const;
        const NOMADSUtil::InetAddr & getRemoteProxyMainInetAddr (void) const;
        const NOMADSUtil::InetAddr & getRemoteServerInetAddrForIPv4AddressAndConnectorType (uint32 ui32IPv4Address, ConnectorType connectorType) const;
        const std::vector<NOMADSUtil::InetAddr> & getRemoteProxyInetAddrListForConnectorType (ConnectorType connectorType) const;
        char const * const getMocketsConfFileName (void) const;

        bool hasInterfaceIPv4Address (uint32 ui32InterfaceIPv4Address) const;
        bool isLocalProxyReachableFromRemote (void) const;
        bool isRemoteProxyReachableFromLocalHost (void) const;

        void setRemoteProxyID (uint32 ui32RemoteProxyID);
        void addRemoteServerAddr (const NOMADSUtil::InetAddr & iaInterfaceIPv4Address);
        void setRemoteServerAddr (std::vector<uint32> vui32RemoteHostIP, uint16 ui16RemoteMocketsPort = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT,
                                  uint16 ui16RemoteTCPPort = NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT,
                                  uint16 ui16RemoteUDPPort = NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT);
        void setRemoteServerPort (ConnectorType connectorType, uint16 ui16RemotePort);
        void setMocketsConfigFileName (char const * const pszMocketsConfFile);
        void setLocalProxyReachabilityFromRemote (const bool bLocalProxyReachabilityFromRemote);
        void setRemoteProxyReachabilityFromLocalHost (const bool bRemoteProxyReachabilityFromLocalHost);

        const std::string toString (void) const;


    private:
        // This is the only class that can update the UniqueID of a remote NetProxy
        template <typename InvalidT> RemoteProxyInfo (InvalidT invalidType1, InvalidT invalidType2) = delete;
        template <typename InvalidT1, typename InvalidT2> RemoteProxyInfo (InvalidT1 invalidType1, InvalidT2 invalidType2) = delete;

        std::vector<NOMADSUtil::InetAddr> & getRemoteProxyInetAddrListForConnectorType (ConnectorType connectorType);

        uint32 _ui32RemoteProxyID;
        std::vector<NOMADSUtil::InetAddr> _remoteMocketsServerAddr;
        std::vector<NOMADSUtil::InetAddr> _remoteTCPServerAddr;
        std::vector<NOMADSUtil::InetAddr> _remoteUDPServerAddr;
        std::unordered_map<std::string, uint32> _mRemoteInterfaceLabels;
        std::string _sMocketsConfFileName;
        bool _bIsLocalProxyReachableFromRemote;
        bool _bIsRemoteProxyReachableFromLocalHost;

        static std::vector<NOMADSUtil::InetAddr> SIA_INVALID_CONNECTOR;
    };


    inline RemoteProxyInfo::RemoteProxyInfo (void) :
        _ui32RemoteProxyID{0}, _bIsLocalProxyReachableFromRemote{true}, _bIsRemoteProxyReachableFromLocalHost{true}
    { }

    inline RemoteProxyInfo::RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<NOMADSUtil::InetAddr> & remoteMocketsServerAddr,
                                             const std::vector<NOMADSUtil::InetAddr> & remoteTCPServerAddr,
                                             const std::vector<NOMADSUtil::InetAddr> & remoteUDPServerAddr,
                                             const char * const pszMocketsConfFileName) :
        _ui32RemoteProxyID{ui32RemoteProxyID}, _remoteMocketsServerAddr{remoteMocketsServerAddr}, _remoteTCPServerAddr{remoteTCPServerAddr},
        _remoteUDPServerAddr{remoteUDPServerAddr}, _sMocketsConfFileName{pszMocketsConfFileName},
        _bIsLocalProxyReachableFromRemote{true}, _bIsRemoteProxyReachableFromLocalHost{true}
    { }

    inline RemoteProxyInfo::RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<uint32> & vui32InterfaceIPv4AddressList, uint16 ui16RemoteMocketsPort,
                                             uint16 ui16RemoteTCPPort, uint16 ui16RemoteUDPPort, const char * const pszMocketsConfFileName) :
        _ui32RemoteProxyID{ui32RemoteProxyID}, _sMocketsConfFileName{pszMocketsConfFileName},
        _bIsLocalProxyReachableFromRemote{true}, _bIsRemoteProxyReachableFromLocalHost{true}
    {
        for (const auto & ui32IPv4Address : vui32InterfaceIPv4AddressList) {
            _remoteMocketsServerAddr.emplace_back (ui32IPv4Address, ui16RemoteMocketsPort);
            _remoteTCPServerAddr.emplace_back (ui32IPv4Address, ui16RemoteTCPPort);
            _remoteUDPServerAddr.emplace_back (ui32IPv4Address, ui16RemoteUDPPort);
        }
    }

    inline RemoteProxyInfo::RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<NOMADSUtil::InetAddr> & remoteHostAddrList,
                                             const char * const pszMocketsConfFileName) :
        _ui32RemoteProxyID{ui32RemoteProxyID}, _sMocketsConfFileName{pszMocketsConfFileName},
        _bIsLocalProxyReachableFromRemote{true}, _bIsRemoteProxyReachableFromLocalHost{true}
    {
        for (const auto & inetAddr : remoteHostAddrList) {
            _remoteMocketsServerAddr.emplace_back (inetAddr.getIPAddress(), NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT);
            _remoteTCPServerAddr.emplace_back (inetAddr.getIPAddress(), NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT);
            _remoteUDPServerAddr.emplace_back (inetAddr.getIPAddress(), NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT);
        }
    }

    inline RemoteProxyInfo::RemoteProxyInfo (uint32 ui32RemoteProxyID, const std::vector<NOMADSUtil::InetAddr> & remoteHostAddrList,
                                             const std::vector<std::string> & vsRemoteHostInterfaceNames, const char * const pszMocketsConfFileName) :
        RemoteProxyInfo{ui32RemoteProxyID, remoteHostAddrList, pszMocketsConfFileName}
    {
        for (unsigned int i = 0; i < remoteHostAddrList.size(); ++i) {
            if (vsRemoteHostInterfaceNames[i].length() > 0) {
                _mRemoteInterfaceLabels[vsRemoteHostInterfaceNames[i]] = remoteHostAddrList[i].getIPAddress();
            }
        }
    }

    inline RemoteProxyInfo::RemoteProxyInfo (const RemoteProxyInfo & rhsEntry) :
        _ui32RemoteProxyID{rhsEntry._ui32RemoteProxyID}, _remoteMocketsServerAddr{rhsEntry._remoteMocketsServerAddr},
        _remoteTCPServerAddr{rhsEntry._remoteTCPServerAddr}, _remoteUDPServerAddr{rhsEntry._remoteUDPServerAddr},
        _mRemoteInterfaceLabels{rhsEntry._mRemoteInterfaceLabels}, _sMocketsConfFileName{rhsEntry._sMocketsConfFileName},
        _bIsLocalProxyReachableFromRemote{rhsEntry._bIsLocalProxyReachableFromRemote},
        _bIsRemoteProxyReachableFromLocalHost{rhsEntry._bIsRemoteProxyReachableFromLocalHost}
    { }

    inline RemoteProxyInfo::~RemoteProxyInfo (void) { }

    inline const uint32 RemoteProxyInfo::getRemoteNetProxyID (void) const
    {
        return _ui32RemoteProxyID;
    }

    inline const std::unordered_set<uint32> RemoteProxyInfo::getRemoteProxyIPAddressList (void) const
    {
        std::unordered_set<uint32> res;
        for (const auto & inetAddr : _remoteMocketsServerAddr) {
            res.insert (inetAddr.getIPAddress());
        }
        for (const auto & inetAddr : _remoteTCPServerAddr) {
            res.insert (inetAddr.getIPAddress());
        }
        for (const auto & inetAddr : _remoteUDPServerAddr) {
            res.insert (inetAddr.getIPAddress());
        }

        return res;
    }

    inline const NOMADSUtil::InetAddr RemoteProxyInfo::getIPAddressOfInterfaceWithLabel (const std::string & sInterfaceLabel) const
    {
        if (_mRemoteInterfaceLabels.count (sInterfaceLabel) == 1) {
            return _mRemoteInterfaceLabels.at (sInterfaceLabel);
        }

        return NetProxyApplicationParameters::IA_INVALID_ADDR;
    }

    inline const NOMADSUtil::InetAddr & RemoteProxyInfo::getRemoteProxyMainInetAddr (void) const
    {
        return (_remoteMocketsServerAddr.size() == 0) ?
            NetProxyApplicationParameters::IA_INVALID_ADDR : _remoteMocketsServerAddr[0];
    }

    inline const NOMADSUtil::InetAddr & RemoteProxyInfo::getRemoteServerInetAddrForIPv4AddressAndConnectorType (uint32 ui32IPv4Address,
                                                                                                                ConnectorType connectorType) const
    {
        const auto & vConnectorServerAddresses = getRemoteProxyInetAddrListForConnectorType (connectorType);
        auto it = std::find_if (vConnectorServerAddresses.begin(), vConnectorServerAddresses.end(),
                                [ui32IPv4Address] (const NOMADSUtil::InetAddr & iaServerAddress)
        {
            return iaServerAddress.getIPAddress() == ui32IPv4Address;
        });

        if (it != vConnectorServerAddresses.cend()) {
            return *it;
        }

        return NetProxyApplicationParameters::IA_INVALID_ADDR;
    }

    inline char const * const RemoteProxyInfo::getMocketsConfFileName (void) const
    {
        return _sMocketsConfFileName.c_str();
    }

    inline bool RemoteProxyInfo::isLocalProxyReachableFromRemote (void) const
    {
        return _bIsLocalProxyReachableFromRemote;
    }

    inline bool RemoteProxyInfo::isRemoteProxyReachableFromLocalHost (void) const
    {
        return _bIsRemoteProxyReachableFromLocalHost;
    }

    inline void RemoteProxyInfo::setRemoteProxyID (uint32 ui32RemoteProxyID)
    {
        _ui32RemoteProxyID = ui32RemoteProxyID;
    }

    inline void RemoteProxyInfo::setRemoteServerPort (ConnectorType connectorType, uint16 ui16RemotePort)
    {
        for (auto & remoteProxyInetAddr : getRemoteProxyInetAddrListForConnectorType (connectorType)) {
            remoteProxyInetAddr.setPort (ui16RemotePort);
        }
    }

    inline void RemoteProxyInfo::setMocketsConfigFileName (char const * const pszMocketsConfFile)
    {
        _sMocketsConfFileName = pszMocketsConfFile;
    }

    inline void RemoteProxyInfo::setLocalProxyReachabilityFromRemote (const bool bLocalProxyReachabilityFromRemote)
    {
        _bIsLocalProxyReachableFromRemote = bLocalProxyReachabilityFromRemote;
    }

    inline void RemoteProxyInfo::setRemoteProxyReachabilityFromLocalHost (const bool bRemoteProxyReachability)
    {
        _bIsRemoteProxyReachableFromLocalHost = bRemoteProxyReachability;
    }

}

#endif
