/*
 * TCPEndPoint.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 15, 2014, 12:40 AM
 */

#include "TCPEndPoint.h"

#include "TCPSocket.h"
#include "TCPConnHandler.h"

#include "SocketReader.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

TCPEndPoint::TCPEndPoint (TCPSocket *pSocket, int64 i64Timeout)
    : _pSocket (pSocket),
      _i64Timeout (i64Timeout)
{
}

TCPEndPoint::~TCPEndPoint (void)
{
    _pSocket = nullptr;
}

int TCPEndPoint::connect (const char *pszRemoteHost, uint16 ui16RemotePort)
{
    if (pszRemoteHost == nullptr) {
        return -1;
    }
    if (_pSocket == nullptr) {
        return -2;
    }
    return _pSocket->connect (pszRemoteHost, ui16RemotePort);
}

void TCPEndPoint::close (void)
{
    if (_pSocket != nullptr) {
        _pSocket->disconnect();
    }
}

String TCPEndPoint::getRemoteAddress (void)
{
    String addr;
    if (_pSocket != nullptr) {
        addr = _pSocket->getRemoteHostAddr();
    }
    return addr;
}

int TCPEndPoint::send (const void *pBuf, int iSize)
{
    if (_pSocket == nullptr) {
        return -1;
    }
    if (!_pSocket->isConnected()) {
        return -2;
    }
    return TCPConnHandler::send (_pSocket, pBuf, iSize);
}

int TCPEndPoint::receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout)
{
    if (_pSocket == nullptr) {
        return -1;
    }
    if (!_pSocket->isConnected()) {
        return -2;
    }
    uint32 ui32Len = 0;
    SocketReader sr (_pSocket, false);
    if (sr.read32 (&ui32Len) < 0) {
        return -2;
    }
    if (ui32Len == 0) {
        return -3;
    }
    if (ui32BufSize < ui32Len) {
        return -4;
    }
    if (_pSocket->receiveBytes (pBuf, ui32Len) < 0) {
        return -5;
    }
    return (int) ui32Len;
}

