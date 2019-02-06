/*
 * RemoteProxyInfo.cpp
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

#include <vector>
#include <sstream>
#include <algorithm>

#include "Logger.h"

#include "RemoteProxyInfo.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    const std::vector<NOMADSUtil::InetAddr> & RemoteProxyInfo::getRemoteProxyInetAddrListForConnectorType (ConnectorType connectorType) const
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return _remoteTCPServerAddr;
        case CT_UDPSOCKET:
            return _remoteUDPServerAddr;
        case CT_MOCKETS:
        case CT_CSR:
            return _remoteMocketsServerAddr;
        }

        return SIA_INVALID_CONNECTOR;
    }

    const RemoteProxyInfo & RemoteProxyInfo::operator = (const RemoteProxyInfo &rhs)
    {
        _remoteMocketsServerAddr = rhs._remoteMocketsServerAddr;
        _remoteTCPServerAddr = rhs._remoteTCPServerAddr;
        _remoteUDPServerAddr = rhs._remoteUDPServerAddr;
        _sMocketsConfFileName = rhs._sMocketsConfFileName;

        _bIsLocalProxyReachableFromRemote = rhs._bIsLocalProxyReachableFromRemote;
        _bIsRemoteProxyReachableFromLocalHost = rhs._bIsRemoteProxyReachableFromLocalHost;

        return *this;
    }

    bool RemoteProxyInfo::hasInterfaceIPv4Address (uint32 ui32InterfaceIPv4Address) const
    {
        for (const auto & inetAddr : _remoteMocketsServerAddr) {
            if (inetAddr.getIPAddress() == ui32InterfaceIPv4Address) {
                return true;
            }
        }
        for (const auto & inetAddr : _remoteTCPServerAddr) {
            if (inetAddr.getIPAddress() == ui32InterfaceIPv4Address) {
                return true;
            }
        }
        for (const auto & inetAddr : _remoteUDPServerAddr) {
            if (inetAddr.getIPAddress() == ui32InterfaceIPv4Address) {
                return true;
            }
        }

        return false;
    }

    void RemoteProxyInfo::addRemoteServerAddr (const NOMADSUtil::InetAddr & iaInterfaceIPv4Address)
    {
        const auto findPredicate = [iaInterfaceIPv4Address](const NOMADSUtil::InetAddr & iaServerIPv4Address)
            { return iaInterfaceIPv4Address.getIPAddress() == iaServerIPv4Address.getIPAddress(); };

        if (std::find_if (_remoteMocketsServerAddr.cbegin(), _remoteMocketsServerAddr.cend(),
                          findPredicate) == _remoteMocketsServerAddr.cend()) {
            _remoteMocketsServerAddr.emplace_back (iaInterfaceIPv4Address.getIPAddress(), (iaInterfaceIPv4Address.getPort() == 0) ?
                                                   NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT : iaInterfaceIPv4Address.getPort());
        }
        if (std::find_if (_remoteTCPServerAddr.cbegin(), _remoteTCPServerAddr.cend(),
                          findPredicate) == _remoteTCPServerAddr.cend()) {
            _remoteTCPServerAddr.emplace_back (iaInterfaceIPv4Address.getIPAddress(), (iaInterfaceIPv4Address.getPort() == 0) ?
                                               NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT : iaInterfaceIPv4Address.getPort());
        }
        if (std::find_if (_remoteUDPServerAddr.cbegin(), _remoteUDPServerAddr.cend(),
                          findPredicate) == _remoteUDPServerAddr.cend()) {
            _remoteUDPServerAddr.emplace_back (iaInterfaceIPv4Address.getIPAddress(), (iaInterfaceIPv4Address.getPort() == 0) ?
                                               NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT : iaInterfaceIPv4Address.getPort());
        }
    }

    void RemoteProxyInfo::setRemoteServerAddr (std::vector<uint32> vui32RemoteHostIP, uint16 ui16RemoteMocketsPort,
                                                      uint16 ui16RemoteTCPPort, uint16 ui16RemoteUDPPort)
    {
        _remoteMocketsServerAddr.clear();
        _remoteTCPServerAddr.clear();
        _remoteUDPServerAddr.clear();

        for (const auto & ui32IP : vui32RemoteHostIP) {
            _remoteMocketsServerAddr.emplace_back (ui32IP, ui16RemoteMocketsPort);
            _remoteTCPServerAddr.emplace_back (ui32IP, ui16RemoteTCPPort);
            _remoteUDPServerAddr.emplace_back (ui32IP, ui16RemoteUDPPort);
        }
    }

    const std::string RemoteProxyInfo::toString (void) const
    {
        std::ostringstream oss;
        oss << "{ NetProxyUniqueID=" << _ui32RemoteProxyID << "; MocketsServerAddresses=";
        for (const auto & inetAddr : _remoteMocketsServerAddr) {
            oss << inetAddr.getIPAsString() << ':' << inetAddr.getPort() << ',';
        }
        oss.seekp (-1, oss.cur); oss << "; TCPServerAddresses=";
        for (const auto & inetAddr : _remoteTCPServerAddr) {
            oss << inetAddr.getIPAsString() << ':' << inetAddr.getPort() << ',';
        }
        oss.seekp (-1, oss.cur); oss << "; UDPServerAddresses=";
        for (const auto & inetAddr : _remoteUDPServerAddr) {
            oss << inetAddr.getIPAsString() << ':' << inetAddr.getPort() << ',';
        }
        oss.seekp (-1, oss.cur);
        oss << "; MocketsConfigFile=" << _sMocketsConfFileName << "; reachability=";
        if (_bIsLocalProxyReachableFromRemote && _bIsRemoteProxyReachableFromLocalHost) {
            oss << "BIDIRECTIONAL; }";
        }
        else if (_bIsLocalProxyReachableFromRemote) {
            oss << "PASSIVE; }";
        }
        else if (_bIsRemoteProxyReachableFromLocalHost) {
            oss << "ACTIVE; }";
        }
        else {
            oss << "NONE; }";
        }

        return oss.str();
    }

    std::vector<NOMADSUtil::InetAddr> & RemoteProxyInfo::getRemoteProxyInetAddrListForConnectorType (ConnectorType connectorType)
    {
        switch (connectorType) {
        case CT_TCPSOCKET:
            return _remoteTCPServerAddr;
        case CT_UDPSOCKET:
            return _remoteUDPServerAddr;
        case CT_MOCKETS:
        case CT_CSR:
            return _remoteMocketsServerAddr;
        }

        return SIA_INVALID_CONNECTOR;
    }


    std::vector<NOMADSUtil::InetAddr> RemoteProxyInfo::SIA_INVALID_CONNECTOR;
}
