#ifndef INCL_REMOTE_PROXY_INFO_H
#define INCL_REMOTE_PROXY_INFO_H

/*
 * RemoteProxyInfo.h
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
 * Class that stores information related to remote NetProxies.
 */

#include "InetAddr.h"
#include "StrClass.h"

#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    class ConnectionManager;

    class RemoteProxyInfo
    {
    public:
        RemoteProxyInfo (void);
        RemoteProxyInfo (uint32 ui32RemoteProxyID, NOMADSUtil::InetAddr remoteMocketsServerAddr,
                         NOMADSUtil::InetAddr remoteTCPServerAddr, NOMADSUtil::InetAddr remoteUDPServerAddr,
                         const char *pszMocketsConfFileName);
        RemoteProxyInfo (uint32 ui32RemoteProxyID, uint32 remoteHostAddr,
                         uint16 ui16RemoteMocketsPort = NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT,
                         uint16 ui16RemoteTCPPort = NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT,
                         uint16 ui16RemoteUDPPort = NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT,
                         const char *pszMocketsConfFileName = nullptr);
        RemoteProxyInfo (uint32 ui32RemoteProxyID, uint32 remoteHostAddr, const char *pszMocketsConfFileName);
        RemoteProxyInfo (const RemoteProxyInfo & rhsEntry);
        ~RemoteProxyInfo (void);

        const RemoteProxyInfo & operator = (const RemoteProxyInfo & rhsEntry);

        const uint32 getRemoteProxyID (void) const;
        const uint32 getRemoteProxyIPAddress (void) const;
        const char * const getRemoteProxyIPAddressAsString (void) const;
        NOMADSUtil::InetAddr * const getRemoteProxyInetAddr (ConnectorType connectorType) const;
        char const * const getMocketsConfFileName (void) const;
        const bool isLocalProxyReachableFromRemote (void) const;
        const bool isRemoteProxyReachableFromLocalHost (void) const;

        void setRemoteIPAddr (uint32 ui32RemoteHostIP);
        void setRemoteServerPort (ConnectorType connectorType, uint16 ui16RemotePort);
        char const * const setMocketsConfigFileName (char const * const pszMocketsConfFile);
        void setLocalProxyReachabilityFromRemote (const bool bLocalProxyReachabilityFromRemote);
        void setRemoteProxyReachabilityFromLocalHost (const bool bRemoteProxyReachabilityFromLocalHost);

    private:
        // This is the only class that can update the UniqueID of a remote NetProxy
        friend class ConnectionManager;

        template <typename InvalidT> RemoteProxyInfo (InvalidT invalidType1, InvalidT invalidType2);
        template <typename InvalidT1, typename InvalidT2> RemoteProxyInfo (InvalidT1 invalidType1, InvalidT2 invalidType2);

        const uint32 setRemoteProxyID (uint32 ui32RemoteProxyID);

        uint32 _ui32RemoteProxyID;
        NOMADSUtil::InetAddr _remoteTCPServerAddr;
        NOMADSUtil::InetAddr _remoteUDPServerAddr;
        NOMADSUtil::InetAddr _remoteMocketsServerAddr;
        NOMADSUtil::String _mocketsConfFileName;
        bool _bIsLocalProxyReachableFromRemote;
        bool _bIsRemoteProxyReachableFromLocalHost;
    };


    inline RemoteProxyInfo::RemoteProxyInfo (void) : _ui32RemoteProxyID (0), _bIsLocalProxyReachableFromRemote (true), _bIsRemoteProxyReachableFromLocalHost (true) { }

    inline RemoteProxyInfo::RemoteProxyInfo (uint32 ui32RemoteProxyID, NOMADSUtil::InetAddr remoteMocketsServerAddr, NOMADSUtil::InetAddr remoteTCPServerAddr,
                                             NOMADSUtil::InetAddr remoteUDPServerAddr, const char *pszMocketsConfFileName) :
                                             _ui32RemoteProxyID (ui32RemoteProxyID), _remoteMocketsServerAddr (remoteMocketsServerAddr), _remoteTCPServerAddr (remoteTCPServerAddr),
                                             _remoteUDPServerAddr (remoteUDPServerAddr), _mocketsConfFileName (pszMocketsConfFileName),
                                             _bIsLocalProxyReachableFromRemote (true), _bIsRemoteProxyReachableFromLocalHost (true) { }

    inline RemoteProxyInfo::RemoteProxyInfo (uint32 ui32RemoteProxyID, uint32 remoteHostAddr, uint16 ui16RemoteMocketsPort, uint16 ui16RemoteTCPPort,
                                             uint16 ui16RemoteUDPPort, const char *pszMocketsConfFileName) :
                                             _ui32RemoteProxyID (ui32RemoteProxyID), _remoteMocketsServerAddr (remoteHostAddr, ui16RemoteMocketsPort),
                                             _remoteTCPServerAddr (remoteHostAddr, ui16RemoteTCPPort), _remoteUDPServerAddr (remoteHostAddr, ui16RemoteUDPPort),
                                             _mocketsConfFileName (pszMocketsConfFileName), _bIsLocalProxyReachableFromRemote (true), _bIsRemoteProxyReachableFromLocalHost (true) { }

    inline RemoteProxyInfo::RemoteProxyInfo (uint32 ui32RemoteProxyID, uint32 remoteHostAddr, const char *pszMocketsConfFileName) :
        _ui32RemoteProxyID (ui32RemoteProxyID), _remoteMocketsServerAddr (remoteHostAddr, NetProxyApplicationParameters::DEFAULT_MOCKET_SERVER_PORT),
        _remoteTCPServerAddr (remoteHostAddr, NetProxyApplicationParameters::DEFAULT_TCP_SERVER_PORT),
        _remoteUDPServerAddr (remoteHostAddr, NetProxyApplicationParameters::DEFAULT_UDP_SERVER_PORT), _mocketsConfFileName (pszMocketsConfFileName),
        _bIsLocalProxyReachableFromRemote (true), _bIsRemoteProxyReachableFromLocalHost (true) { }

    inline RemoteProxyInfo::RemoteProxyInfo (const RemoteProxyInfo & rhsEntry) :
        _ui32RemoteProxyID (rhsEntry._ui32RemoteProxyID), _remoteMocketsServerAddr (rhsEntry._remoteMocketsServerAddr), _remoteTCPServerAddr (rhsEntry._remoteTCPServerAddr),
        _remoteUDPServerAddr (rhsEntry._remoteUDPServerAddr), _mocketsConfFileName (rhsEntry._mocketsConfFileName),
        _bIsLocalProxyReachableFromRemote (rhsEntry._bIsLocalProxyReachableFromRemote), _bIsRemoteProxyReachableFromLocalHost (rhsEntry._bIsRemoteProxyReachableFromLocalHost) { }

    inline RemoteProxyInfo::~RemoteProxyInfo (void) {}

    inline const uint32 RemoteProxyInfo::getRemoteProxyID (void) const
    {
        return _ui32RemoteProxyID;
    }

    inline const uint32 RemoteProxyInfo::getRemoteProxyIPAddress (void) const
    {
        return _remoteMocketsServerAddr.getIPAddress();
    }

    inline const char * const RemoteProxyInfo::getRemoteProxyIPAddressAsString (void) const
    {
        return _remoteMocketsServerAddr.getIPAsString();
    }

    inline char const * const RemoteProxyInfo::getMocketsConfFileName (void) const
    {
        return _mocketsConfFileName;
    }

    inline const bool RemoteProxyInfo::isLocalProxyReachableFromRemote (void) const
    {
        return _bIsLocalProxyReachableFromRemote;
    }

    inline const bool RemoteProxyInfo::isRemoteProxyReachableFromLocalHost (void) const
    {
        return _bIsRemoteProxyReachableFromLocalHost;
    }

    inline const uint32 RemoteProxyInfo::setRemoteProxyID (uint32 ui32RemoteProxyID)
    {
        const uint32 oldID = _ui32RemoteProxyID;
        _ui32RemoteProxyID = ui32RemoteProxyID;

        return oldID;
    }

    inline void RemoteProxyInfo::setRemoteIPAddr (uint32 ui32RemoteHostIP)
    {
        _remoteMocketsServerAddr.setIPAddress (ui32RemoteHostIP);
        _remoteTCPServerAddr.setIPAddress (ui32RemoteHostIP);
        _remoteUDPServerAddr.setIPAddress (ui32RemoteHostIP);
    }

    inline void RemoteProxyInfo::setRemoteServerPort (ConnectorType connectorType, uint16 ui16RemotePort)
    {
        getRemoteProxyInetAddr (connectorType)->setPort (ui16RemotePort);
    }

    inline char const * const RemoteProxyInfo::setMocketsConfigFileName (char const * const pszMocketsConfFile)
    {
        char const * const pszPreviousMocketsConfFile = _mocketsConfFileName;
        _mocketsConfFileName = pszMocketsConfFile;

        return pszPreviousMocketsConfFile;
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
