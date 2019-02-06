/*
 * SocketWriter.cpp
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

#include "SocketWriter.h"

#include "Socket.h"

using namespace NOMADSUtil;

SocketWriter::SocketWriter (Socket *pSocket, bool bDeleteWhenDone)
{
    _pSocket = pSocket;
    _bDeleteSocketWhenDone = bDeleteWhenDone;
    _ulBytesWritten = 0;
}

SocketWriter::~SocketWriter (void)
{
    if (_bDeleteSocketWhenDone) {
        delete _pSocket;
    }
    _pSocket = NULL;
}

int SocketWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    if (_pSocket->sendBytes (pBuf, ulCount)) {
        return -1;
    }
    _ulBytesWritten += ulCount;
    return 0;
}

int SocketWriter::close()
{
    _pSocket->disconnect();
    return 0;
}
