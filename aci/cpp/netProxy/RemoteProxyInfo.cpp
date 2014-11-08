/*
 * RemoteProxyInfo.cpp
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

#include "Logger.h"

#include "RemoteProxyInfo.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    InetAddr * const RemoteProxyInfo::getRemoteProxyInetAddr (ConnectorType connectorType) const
    {
        switch (connectorType) {
        case CT_MOCKETS:
            return const_cast<InetAddr * const> (&_remoteMocketsServerAddr);
        case CT_SOCKET:
            return const_cast<InetAddr * const> (&_remoteTCPServerAddr);
        case CT_UDP:
            return const_cast<InetAddr * const> (&_remoteUDPServerAddr);
        case CT_CSR:
            return const_cast<InetAddr * const> (&_remoteMocketsServerAddr);
        }

        return NULL;
    }

    const RemoteProxyInfo & RemoteProxyInfo::operator = (const RemoteProxyInfo &rhs)
    {
        _remoteMocketsServerAddr = rhs._remoteMocketsServerAddr;
        _remoteTCPServerAddr = rhs._remoteTCPServerAddr;
        _remoteUDPServerAddr = rhs._remoteUDPServerAddr;
        _mocketsConfFileName = rhs._mocketsConfFileName;

        _bIsLocalProxyReachableFromRemote = rhs._bIsLocalProxyReachableFromRemote;
        _bIsRemoteProxyReachableFromLocalHost = rhs._bIsRemoteProxyReachableFromLocalHost;

        return *this;
    }
}