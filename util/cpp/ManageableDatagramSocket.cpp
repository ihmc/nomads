/*
 * ManageableDatagramSocket.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on September 2, 2015, 2:00 PM
 */

#include "ManageableDatagramSocket.h"

#include "DArray2.h"
#include "Logger.h"
#include "SimpleCommHelper2.h"
#include "TCPSocket.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

namespace NOMADSUtil
{
    bool filterOut (StringHashset &addresses, const bool bExclusive, const char *pszAddr)
    {
        const bool bContained = addresses.containsKey (pszAddr);
        if (bExclusive) {
            return bContained;
        }
        return !bContained;
    }

    bool filterOut (StringHashset &addresses, const bool bExclusive, uint32 ui32IPv4Addr)
    {
        const InetAddr addr (ui32IPv4Addr);
        return filterOut (addresses, bExclusive, addr.getIPAsString());
    }
}

//-----------------------------------------------------------------------------
// ManageableDatagramSocket
//-----------------------------------------------------------------------------

ManageableDatagramSocket::ManageableDatagramSocket (DatagramSocket *pDGramSock)
    : _bExclusive (false),
    _pDGramSock (pDGramSock)
{
}

ManageableDatagramSocket::~ManageableDatagramSocket (void)
{
    _pDGramSock = NULL;
}

int ManageableDatagramSocket::receive (void *pBuf, int iBufSize)
{
    InetAddr remoteAddr;
    return receive (pBuf, iBufSize, &remoteAddr);
}

int ManageableDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
{
    int rc = _pDGramSock->receive (pBuf, iBufSize, pRemoteAddr);
    if (rc < 0) {
        return rc;
    }
    _m.lock();
    if (filterOut (_addresses, _bExclusive, pRemoteAddr->getIPAsString())) {
        _m.unlock();
        return 0;
    }
    _m.unlock();
    return rc;
}

#if defined (UNIX)
    int ManageableDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr, int &iIncomingIfaceIdx)
    {
        int rc = _pDGramSock->receive (pBuf, iBufSize, pRemoteAddr, iIncomingIfaceIdx);
        if (rc < 0) {
            return rc;
        }
        _m.lock ();
        if (filterOut (_addresses, _bExclusive, pRemoteAddr->getIPAsString ())) {
            _m.unlock ();
            return 0;
        }
        _m.unlock ();
        return rc;
    }
#endif

int ManageableDatagramSocket::sendTo (uint32 ui32IPv4Addr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
    _m.lock();
    if (filterOut (_addresses, _bExclusive, ui32IPv4Addr)) {
        _m.unlock();
        return -1;
    }
    _m.unlock();
    return _pDGramSock->sendTo (ui32IPv4Addr, ui16Port, pBuf, iBufSize, pszHints);
}

int ManageableDatagramSocket::sendTo (const char *pszAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
    _m.lock();
    if (filterOut (_addresses, _bExclusive, pszAddr)) {
        _m.unlock();
        return -1;
    }
    _m.unlock();
    return _pDGramSock->sendTo (pszAddr, ui16Port, pBuf, iBufSize, pszHints);
}

//-----------------------------------------------------------------------------
// ManageableDatagramSocketManager
//-----------------------------------------------------------------------------

ManageableDatagramSocketManager::ManageableDatagramSocketManager (void)
{
}

ManageableDatagramSocketManager::~ManageableDatagramSocketManager (void)
{
}

void ManageableDatagramSocketManager::run (void)
{
    const char *pszMethodName = "ManageableDatagramSocketManager::run";
    setName (pszMethodName);
    SimpleCommHelper2 ch;
    started();
    TCPSocket *pServerSock = new TCPSocket();
    if (pServerSock == NULL) {
        terminating();
        return;
    }
    uint16 ui16ProxyServerPortNum = 7777;
    int rc = pServerSock->setupToReceive (ui16ProxyServerPortNum);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to bind socket to port %d error num: %d\n",
                        static_cast<int>(ui16ProxyServerPortNum), rc);
        return;
    }

    while (!terminationRequested()) {
        // Get next connection, or abort if it fails
        TCPSocket *pClientSock = static_cast<TCPSocket*>(pServerSock->accept());
        if (NULL == pClientSock) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "failed to accept a connection\n");
        }
        else {
            if (pClientSock->setLingerOptions (1, 30)) {
                // Enable SO_LINGER and set the timeout to 30 seconds
                checkAndLogMsg (pszMethodName, Logger::L_Warning,
                                "failed to set the linger option for the socket\n");
            }
            pClientSock->bufferingMode (false);
            rc = ch.init (pClientSock);
            if (rc != 0) {
                checkAndLogMsg (pszMethodName, Logger::L_MildError,
                                "could not initialize CommHelper; rc = %d\n", rc);
            }
            while (true) {
                parse (ch.getReaderRef());
            }
        }
    }
    terminating();
}

ManageableDatagramSocket * ManageableDatagramSocketManager::getManageableDatagramSocket (DatagramSocket *pDGramSock)
{
    if (pDGramSock == NULL) {
        return NULL;
    }
    const InetAddr addr (pDGramSock->getLocalAddr());
    ManageableDatagramSocket *pMngblDgramSock = new ManageableDatagramSocket (pDGramSock);
    if (pMngblDgramSock == NULL) {
        return NULL;
    }
    _sockets.put (addr.getIPAsString(), pMngblDgramSock);
    return pMngblDgramSock;
}

void ManageableDatagramSocketManager::parse (Reader *pReader)
{
    // Parse message
    MessageHeader header;
    if (header.read (pReader) < 0) {
        return;
    }

    char localIfAddr[128];
    char *p = localIfAddr;
    if (pReader->readString (&p) < 0) {
        return;
    }

    uint16 ui16NAddr = 0;
    if (pReader->read16 (&ui16NAddr) < 0) {
        return;
    }
    char remoteIfAddr[128];
    DArray2<String> addresses (ui16NAddr);
    for (uint16 i = 0; i < ui16NAddr; i++) {
        p = remoteIfAddr;
        if (pReader->readString (&p) < 0) {
            return;
        }
        addresses[i] = remoteIfAddr;
    }

    // Change settings
    ManageableDatagramSocket *pMgblDgramSocket = _sockets.get (localIfAddr);
    if (pMgblDgramSocket == NULL) {
        return;
    }
    pMgblDgramSocket->_m.lock();
    if (header.bReset) {
        pMgblDgramSocket->_addresses.removeAll();
    }
    pMgblDgramSocket->_bExclusive = header.bExclusive;
    for (uint16 i = 0; i < addresses.size(); i++) {
        pMgblDgramSocket->_addresses.put (addresses[i]);
    }
    pMgblDgramSocket->_m.unlock();
}

//-----------------------------------------------------------------------------
// MessageHeader
//-----------------------------------------------------------------------------

int MessageHeader::read (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    uint8 ui8Header = 0x00;
    if (pReader->read8 (&ui8Header) < 0) {
        return -2;
    }
    bReset = (1 == ((ui8Header >> RESET_LIST) & 1));
    bExclusive = (1 == ((ui8Header >> EXCLUSIVE_LIST) & 1));
    return 0;
}

int MessageHeader::write (Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    uint8 ui8Header = 0x00;
    if (bReset) {
        ui8Header |= 1 << RESET_LIST;
    }
    if (bExclusive) {
        ui8Header |= 1 << EXCLUSIVE_LIST;
    }
    if (pWriter->write8 (&ui8Header) < 0) {
        return -2;
    }
    return 0;
}

