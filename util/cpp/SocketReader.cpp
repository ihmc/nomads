/*
 * SocketReader.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include "SocketReader.h"

#include "Socket.h"

using namespace NOMADSUtil;

SocketReader::SocketReader (Socket *pSocket, bool bDeleteWhenDone)
{
    _pSocket = pSocket;
    _bDeleteSocketWhenDone = bDeleteWhenDone;
}

SocketReader::~SocketReader (void)
{
    if (_bDeleteSocketWhenDone) {
        delete _pSocket;
    }
    _pSocket = NULL;
}

int SocketReader::read (void *pBuf, int iCount)
{
    int rc;
    if ((rc = _pSocket->receive (pBuf, iCount)) < 0) {
        return -1;
    }
    _ui32TotalBytesRead += rc;
    return rc;
}

int SocketReader::readBytes (void *pBuf, uint32 ui32Count)
{
    int iCount = (int) ui32Count;
    if (iCount != _pSocket->receiveBytes (pBuf, iCount)) {
        return -1;
    }
    _ui32TotalBytesRead += ui32Count;
    return 0;
}

int SocketReader::close()
{
    _pSocket->disconnect();
    return 0;
}
