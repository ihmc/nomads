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
#if defined UNIX
    #include <cerrno>
#endif
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
    _pDGSocket = nullptr;
}

CommInterface * UDPCommInterface::newInstance (void)
{
    // Cannot create a new instance since we do not know how to instantiate and initialize
    // the underlying UDPDatagramSocket
    return nullptr;
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

int UDPCommInterface::shutdown (bool bReadMode, bool bWriteMode)
{
    return _pDGSocket->shutdown (bReadMode, bWriteMode);
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

int UDPCommInterface::isRecoverableSocketError (void) 
{
    int error = getLastError();
    int rc = 0;
    switch (error) {
#if defined WIN32
        case WSANOTINITIALISED:
            rc = 0;
            break;
        case WSAENETDOWN:
            rc = -1;
            break;
        case WSAEACCES:
            rc = -1;
            break;
        case WSAEINVAL:
            rc = -1;
            break;
        case WSAEINTR:
            rc = -1;
            break;
        case WSAEINPROGRESS:
            rc = -1;
            break;
        case EFAULT:
            rc = -1;
            break;
        case WSAENETRESET:
            rc = -1;
            break;
        case WSAENOBUFS:
            rc = 0;
            break;
        case WSAENOTCONN:
            rc = -1;
            break;
        case WSAENOTSOCK:
            rc = -1;
            break;
        case WSAEOPNOTSUPP:
            rc = -1;
            break;
        case WSAESHUTDOWN:
            rc = -1;
            break;
        case WSAEWOULDBLOCK:
            rc = 0;
            break;
        case WSAEMSGSIZE:
            rc = 0;
            break;
        case WSAEHOSTUNREACH:
            rc = -1;
            break;
        case WSAECONNABORTED:
            rc = -1;
            break;
        case WSAECONNRESET:
            rc = -1;
            break;
        case WSAEADDRNOTAVAIL:
            rc = -1;
            break;
        case WSAEAFNOSUPPORT:
            rc = -1;
            break;
        case WSAEDESTADDRREQ:
            rc = -1;
            break;
        case WSAENETUNREACH:
            rc = -1;
            break;
        case WSAETIMEDOUT:
            rc = -1;
            break;
 #elif defined UNIX
        case EACCES:
            rc = -1;
            break;
        case EWOULDBLOCK:
            rc = 0;
            break;
        case EBADF:
            rc = -1;
            break;
        case ECONNRESET:
            rc = -1;
            break;
        case EDESTADDRREQ:
            rc = -1;
            break;
        case EFAULT:
            rc = -1;
            break;
        case EINTR:
            rc = -1;
            break;
        case EINVAL:
            rc = -1;
            break;
        case EISCONN:
            rc = -1;
            break;
        case EMSGSIZE:
            rc = 0;
            break;
        case ENOBUFS:
            rc = 0;
            break;
        case ENOMEM:
            rc = 0;
            break;
        case ENOTCONN:
            rc = -1;
            break;
        case ENOTSOCK:
            rc = -1;
            break;
        case EOPNOTSUPP:
            rc = -1;
            break;
        case EPIPE:
            rc = -1;
            break;
#endif
        default:
            rc = -1;
            break;
    }

    return rc;


}