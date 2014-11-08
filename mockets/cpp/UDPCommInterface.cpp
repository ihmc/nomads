/*
 * UDPCommInterface.cpp
 * Author: nino
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

#include "UDPCommInterface.h"

using namespace NOMADSUtil;

UDPCommInterface::UDPCommInterface (UDPDatagramSocket *pDGSocket, bool bDeleteDGSocketWhenDone)
{
    _pDGSocket = pDGSocket;
    _bDeleteDGSocketWhenDone = bDeleteDGSocketWhenDone;
}

UDPCommInterface::~UDPCommInterface (void)
{
    if (_bDeleteDGSocketWhenDone) {
        delete _pDGSocket;
    }
    _pDGSocket = NULL;
}

CommInterface * UDPCommInterface::newInstance (void)
{
    // Cannot create a new instance since we do not know how to instantiate and initialize
    // the underlying UDPDatagramSocket
    return NULL;
}

int UDPCommInterface::bind (uint16 ui16Port)
{
    return _pDGSocket->init (ui16Port);
}

int UDPCommInterface::bind (NOMADSUtil::InetAddr *pLocalAddr)
{
    return _pDGSocket->init (pLocalAddr->getPort(), pLocalAddr->getIPAddress());
}

InetAddr UDPCommInterface::getLocalAddr (void)
{
    return _pDGSocket->getLocalAddr();
}

int UDPCommInterface::getLocalPort (void)
{
    return _pDGSocket->getLocalPort();
}

int UDPCommInterface::close (void)
{
    return _pDGSocket->close();
}

int UDPCommInterface::setReceiveTimeout (uint32 ui32TimeoutInMS)
{
    return _pDGSocket->setTimeout (ui32TimeoutInMS);
}

int UDPCommInterface::setReceiveBufferSize (uint32 ui32BufferSize)
{
    return _pDGSocket->setReceiveBufferSize ((int) ui32BufferSize);
}

int UDPCommInterface::sendTo (InetAddr *pRemoteAddr, const void *pBuf, int iBufSize, const char *pszHints)
{
    return _pDGSocket->sendTo (pRemoteAddr->getIPAddress(), pRemoteAddr->getPort(), pBuf, iBufSize, pszHints);
}

int UDPCommInterface::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
{
    return _pDGSocket->receive (pBuf, iBufSize, pRemoteAddr);
}

int UDPCommInterface::getLastError (void)
{
    return _pDGSocket->getLastError();
}
