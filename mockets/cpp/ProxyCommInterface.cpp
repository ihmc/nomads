/*
 * ProxyCommInterface.cpp
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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
#include "ProxyCommInterface.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

ProxyCommInterface::ProxyCommInterface (const char *pszProxyServerAddr, uint16 ui16ProxyServerPort)
{
    _proxyServerAddr = pszProxyServerAddr;
    _ui16ProxyServerPort = ui16ProxyServerPort;
    _pPDGS = nullptr;
}

ProxyCommInterface::~ProxyCommInterface (void)
{
    if (_pPDGS != nullptr) {
        delete _pPDGS;
        _pPDGS = nullptr;
    }
}

CommInterface * ProxyCommInterface::newInstance (void)
{
    return new ProxyCommInterface (_proxyServerAddr, _ui16ProxyServerPort);
}

int ProxyCommInterface::bind (uint16 ui16Port)
{
    if (_pPDGS != nullptr) {
        // Socket has already been initialized and bound
        return -1;
    }
    int rc;
    _pPDGS = new ProxyDatagramSocket();
    if (0 != (rc = _pPDGS->init (_proxyServerAddr, _ui16ProxyServerPort, ui16Port))) {
        return -2;
    }
    if (ui16Port == 0) {
        // Wait for upto 5 seconds to see if the ProxyDatagramSocket has connected succeeded and the port was bound
        // This is to prevent an issue if the caller calls getLocalPort() and the port is not yet available
        for (int i = 0; i < 50; i++) {
            if (_pPDGS->getLocalPort() != 0) {
                // Success
                return 0;
            }
            sleepForMilliseconds (100);
        }
        return -3;
    }
    return 0;
}

int ProxyCommInterface::bind (NOMADSUtil::InetAddr *pLocalAddr)
{
    // ProxyDatagramSocket currently does not have a mechanism to set the local address - just the port
    return bind (pLocalAddr->getPort());
}

InetAddr ProxyCommInterface::getLocalAddr (void)
{
    return _pPDGS->getLocalAddr();
}

int ProxyCommInterface::getLocalPort (void)
{
    return _pPDGS->getLocalPort();
}

int ProxyCommInterface::close (void)
{
    return _pPDGS->close();
}

int ProxyCommInterface::shutdown (bool bReadMode, bool bWriteMode)
{
    //need to check what should happen here
    return 0;
}

int ProxyCommInterface::setReceiveTimeout (uint32 ui32TimeoutInMS)
{
    return _pPDGS->setTimeout (ui32TimeoutInMS);
}

int ProxyCommInterface::setReceiveBufferSize (uint32 ui32BufferSize)
{
    return _pPDGS->setReceiveBufferSize ((int) ui32BufferSize);
}

int ProxyCommInterface::sendTo (InetAddr *pRemoteAddr, const void *pBuf, int iBufSize, const char *pszHints)
{
    return _pPDGS->sendTo (pRemoteAddr->getIPAddress(), pRemoteAddr->getPort(), pBuf, iBufSize, pszHints);
}

int ProxyCommInterface::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
{
    return _pPDGS->receive (pBuf, iBufSize, pRemoteAddr);
}

int ProxyCommInterface::getLastError (void)
{
    return _pPDGS->getLastError();
}

int ProxyCommInterface::isRecoverableSocketError (void)
{
    return 0;
}