/*
 * TCPConnHandler.cpp
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
 * Created on April 14, 2014, 5:58 PM
 */

#include "TCPConnHandler.h"

#include <assert.h>
#include <stdlib.h>

#include "BufferWriter.h"
#include "NLFLib.h"
#include "SocketReader.h"
#include "TCPSocket.h"


using namespace IHMC_ACI;
using namespace NOMADSUtil;


TCPConnHandler::TCPConnHandler (const AdaptorProperties &adptProp, const char *pszRemotePeerId,
                                CommAdaptorListener *pListener, TCPSocket *pSocket, const char *pszLocalPeerAdd) :
    ConnHandler (adptProp, pszRemotePeerId, pSocket->getRemotePort(), pListener), _pSocket (pSocket),
    _localPeerAddr (pszLocalPeerAdd), _remotePeerAddr (pSocket->getRemoteHostAddr())
{
    assert (Socket::checkIfStringIsIPAddr (_localPeerAddr));
    assert (Socket::checkIfStringIsIPAddr (_remotePeerAddr));
}

int TCPConnHandler::init (void)
{
    String sThreadName ("IHMC_ACI::TCPConnHandler_");
    sThreadName += getRemotePeerNodeId();
    setName (sThreadName.c_str());

    return 0;
}

bool TCPConnHandler::isConnected (void)
{
    if (_pSocket != nullptr) {
        return _pSocket->isConnected() > 0;
    }
    return false;
}

void TCPConnHandler::requestTermination (void)
{
    if (_pSocket->isConnected()) {
        _pSocket->disconnect();
    }
    ManageableThread::requestTermination();
}

void TCPConnHandler::requestTerminationAndWait (void)
{
    if (_pSocket->isConnected()) {
        _pSocket->disconnect();
    }
    ManageableThread::requestTerminationAndWait();
}

void TCPConnHandler::abortConnHandler (void)
{
    if (_pSocket->isConnected()) {
        _pSocket->shutdown (true, true);
        _pSocket->disconnect();
    }
    ManageableThread::requestTerminationAndWait();
}

/* Note: do not move (and inline) this method to the header file,
 * unless the #include of TCPSocket.h is substituted for the
 * forward declarations of Socket and TCPSocket. */
int TCPConnHandler::send (const void *pBuf, uint32 ui32Len, uint8 ui8Priority)
{
    return send (_pSocket, pBuf, ui32Len, ui8Priority);
}

int TCPConnHandler::send (Socket *pSocket, const void *pBuf, uint32 ui32Len, uint8 ui8Priority)
{
    if (pSocket == nullptr) {
        return -1;
    }
    BufferWriter bw (1400, 1400);
    bw.write32 (&ui32Len);
    bw.writeBytes (pBuf, ui32Len);
    return pSocket->sendBytes (bw.getBuffer(), bw.getBufferLength());
}

int TCPConnHandler::receive (BufferWrapper &bw, char **ppszRemotePeerAddr)
{
    if (ppszRemotePeerAddr == nullptr || _pSocket == nullptr) {
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
    if (ui32Len > MAX_MESSAGE_SIZE) {
        void *pBuf = malloc (ui32Len);
        if (pBuf == nullptr) {
            return -4;
        }
        else if (sr.readBytes (pBuf, ui32Len) < 0) {
            free (pBuf);
            return -5;
        }
        bw.init (pBuf, ui32Len, true);
    }
    else {
        if (sr.readBytes (_buf, ui32Len) < 0) {
            return -6;
        }
        bw.init (_buf, ui32Len, false);
    }

    *ppszRemotePeerAddr = strDup (_pSocket->getRemoteHostAddr());

    return bw.getBufferLength();
}

