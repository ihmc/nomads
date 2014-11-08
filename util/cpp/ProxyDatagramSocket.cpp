/*
 * ProxyDatagramSocket.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#include "ProxyDatagramSocket.h"

#include "CommHelper2.h"
#include "Logger.h"
#include "StringTokenizer.h"
#include "TCPSocket.h"
#include "net/NetUtils.h"

using namespace NOMADSUtil;

#if defined (WIN32)
    bool ProxyDatagramSocket::bWinsockInitialized;
#endif

#define checkAndLogMsg if (pLogger) pLogger->logMsg

ProxyDatagramSocket::ProxyDatagramSocket (void)
{
    _ui32LocalAddr = 0;
    _ui16LocalPort = 0;
    _pSocketToServer = NULL;
    _pchToServer = NULL;
    _pConnector = NULL;
    _bConnectedToServer = false;
    _bReadyToConnect = true;
    _i64LastReconnectAttemptTime = 0;
    _bClearToSend = true;
    _ui32Timeout = 0;
    #if defined (WIN32)
        if (!bWinsockInitialized) {
            WSADATA wsadata;
            WSAStartup (MAKEWORD(2,2), &wsadata);
            bWinsockInitialized = true;
        }
    #endif
    _ui32SentPacketCount = 0;
    _ui32ReceivedPacketCount = 0;
}

ProxyDatagramSocket::~ProxyDatagramSocket (void)
{
    close();
}

int ProxyDatagramSocket::init (const char *pszProxyServerAddr, uint16 ui16ProxyServerPort, uint16 ui16Port)
{
    _proxyServerAddr.setIPAddress (pszProxyServerAddr);
    _proxyServerAddr.setPort (ui16ProxyServerPort);
    _ui16LocalPort = ui16Port;
    _pConnector = new Connector (this);
    _pConnector->start();
    return 0;
}

int ProxyDatagramSocket::close (void)
{
    _m.lock();
    if ((_bConnectedToServer) && (_pchToServer)) {
        try {
            uint8 aui8Buf[1];
            aui8Buf[0] = 'C';
            _pchToServer->sendBlock (aui8Buf, 1);
            _pSocketToServer->disconnect();
        }
        catch (Exception e) {
            checkAndLogMsg ("ProxyDatagramSocket::close", Logger::L_Warning,
                            "exception when sending close message to proxy server; message = <%s>\n",
                            e.getMsg());
        }
    }
    _bReadyToConnect = false;
    _bConnectedToServer = false;
    sleepForMilliseconds (1000);   // So that any other thread calling send() or receive() will fail
    delete _pchToServer;
    _pchToServer = NULL;
    delete _pSocketToServer;
    _pSocketToServer = NULL;
    delete _pConnector;
    _pConnector = NULL;
    _bReadyToConnect = true;
    _m.unlock();
    return 0;
}

uint16 ProxyDatagramSocket::getLocalPort (void)
{
    return _ui16LocalPort;
}

InetAddr ProxyDatagramSocket::getLocalAddr (void)
{
    if ((_ui32LocalAddr == 0) && (!_bConnectedToServer)) {
        connectToProxyServer();
    }
    return InetAddr (_ui32LocalAddr);
}

int ProxyDatagramSocket::getLocalSocket (void)
{
    // TODO: implement this
    checkAndLogMsg ("ProxyDatagramSocket::getLocalSocket", Logger::L_SevereError,
                    "method not yet implemented\n");
    return -1;
}

uint16 ProxyDatagramSocket::getMTU (void)
{
    if (_bConnectedToServer) {
        return _ui16MTU;
    }
    else {
        return DEFAULT_MTU;
    }
}

int ProxyDatagramSocket::getReceiveBufferSize (void)
{
    // TODO: implement this
    checkAndLogMsg ("ProxyDatagramSocket::getReceiveBufferSize", Logger::L_SevereError,
                    "method not yet implemented\n");
    return -1;
}

int ProxyDatagramSocket::setReceiveBufferSize (int iSize)
{
    // TODO: implement this
    checkAndLogMsg ("ProxyDatagramSocket::setReceiveBufferSize", Logger::L_SevereError,
                    "method not yet implemented\n");
    return -1;
}

int ProxyDatagramSocket::getSendBufferSize (void)
{
    // TODO: implement this
    checkAndLogMsg ("ProxyDatagramSocket::getSendBufferSize", Logger::L_SevereError,
                    "method not yet implemented\n");
    return -1;
}

int ProxyDatagramSocket::setSendBufferSize (int iSize)
{
    // TODO: implement this
    checkAndLogMsg ("ProxyDatagramSocket::setSendBufferSize", Logger::L_SevereError,
                    "method not yet implemented\n");
    return -1;
}

int ProxyDatagramSocket::setTimeout (uint32 ui32TimeoutInMS)
{
    _ui32Timeout = ui32TimeoutInMS;
    return 0;
}

int ProxyDatagramSocket::bytesAvail (void)
{
    // TODO: implement this
    checkAndLogMsg ("ProxyDatagramSocket::bytesAvail", Logger::L_SevereError,
                    "method not yet implemented\n");
    return -1;
}

bool ProxyDatagramSocket::clearToSend (void)
{
    return _bClearToSend;
}

int ProxyDatagramSocket::sendTo (const char *pszIPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
    if (pszIPAddr == NULL) {
        return -1;
    }
    int rc;
    if ((rc = sendTo (inet_addr(pszIPAddr), ui16Port, pBuf, iBufSize, pszHints)) < 0) {
        return -2;
    }
    return rc;
}

int ProxyDatagramSocket::sendTo (uint32 ui32IPAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
    int rc;
    if (_bConnectedToServer) {
        if (iBufSize < 0) {
            checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_MildError,
                            "packet size cannot be negative\n");
            return -1;
        }
        if (iBufSize > (MAXIMUM_PROXY_PACKET_SIZE - 17)) {
            checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_MildError,
                            "packet too large to be sent - maximum size is <%d>\n",
                            (int) (MAXIMUM_PROXY_PACKET_SIZE - 17));
            return -2;
        }
        try {
            bool bExcludeAddr = false;
            bool bIncludeAddr = false;
            InetAddr excludeAddr;
            InetAddr includeAddr;
            if (pszHints != NULL) {
                // Check to see if we can handle the hints
                StringTokenizer st (pszHints, ';');
                const char *pszHint = NULL;
                while ((pszHint = st.getNextToken()) != NULL) {
                    StringTokenizer st2 (pszHint, '=');
                    const char *pszAttr = st2.getNextToken();
                    const char *pszValue = st2.getNextToken();
                    if ((pszAttr == NULL) || (pszValue == NULL)) {
                        checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_Warning,
                                        "could not parse hint <%s> - ignoring\n", pszHint);
                    }
                    // Check to see if an exclude address has been specified in the hints
                    else if (0 == stricmp (pszAttr, "exclude")) {
                        if (InetAddr::isIPv4Addr (pszValue)) {
                            excludeAddr.setIPAddress (pszValue);
                            bExcludeAddr = true;
                        }
                        else {
                            checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_Warning,
                                            "could not parse exclude address <%s> - ignoring\n", pszValue);
                        }
                    }
                    // Check to see if an include address has been specified in the hints
                    else if (0 == stricmp (pszAttr, "include")) {
                        if (InetAddr::isIPv4Addr (pszValue)) {
                            includeAddr.setIPAddress (pszValue);
                            bIncludeAddr = true;
                        }
                        else {
                            checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_Warning,
                                            "could not parse include address <%s> - ignoring\n", pszValue);
                        }
                    }
                    // Check to see if we are doing a forced unicast
                    else if (0 == stricmp (pszAttr, "unicast")) {
                        if (InetAddr::isIPv4Addr (pszValue)) {
                            InetAddr unicastAddr (pszValue);
                            ui32IPAddr = unicastAddr.getIPAddress();
                            checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_Info,
                                            "overwrote destination address with unicast address %s\n", unicastAddr.getIPAsString());
                        }
                    }
                    else {
                        checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_Warning,
                                        "unknown hint attribute <%s> - ignoring\n", pszAttr);
                    }
                }
            }
            /*!!*/ // Improve the efficiency of this by implementing a two-argument sendBlock() method
                   // so that a memcpy is not needed
            uint8 aui8Buf [MAXIMUM_PROXY_PACKET_SIZE];
            uint8 ui8Offset = 0;
            if (bIncludeAddr) {
                aui8Buf[ui8Offset] = 'I';
            }
            else if (bExcludeAddr) {
                aui8Buf[ui8Offset] = 'X';
            }
            else {
                aui8Buf[ui8Offset] = 'D';
            }
            ui8Offset += 1;
            (*((uint32*)(aui8Buf+ui8Offset))) = _ui32LocalAddr;
            ui8Offset += 4;
            (*((uint16*)(aui8Buf+ui8Offset))) = EndianHelper::htons (_ui16LocalPort);
            ui8Offset += 2;
            (*((uint32*)(aui8Buf+ui8Offset))) = ui32IPAddr;
            ui8Offset += 4;
            if (bIncludeAddr) {
                (*((uint32*)(aui8Buf+ui8Offset))) = includeAddr.getIPAddress();
                ui8Offset += 4;
            }
            else if (bExcludeAddr) {
                (*((uint32*)(aui8Buf+ui8Offset))) = excludeAddr.getIPAddress();
                ui8Offset += 4;
            }
            (*((uint16*)(aui8Buf+ui8Offset))) = EndianHelper::htons (ui16Port);
            ui8Offset += 2;
            memcpy (aui8Buf+ui8Offset, pBuf, iBufSize);
            _pchToServer->sendBlock (aui8Buf, iBufSize + ui8Offset);
            _ui32SentPacketCount++;
            return iBufSize;
        }
        catch (Exception e) {
            checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_MildError,
                            "exception occurred when sending data to proxy server; message = <%s>\n",
                            e.getMsg());
            close();
            rc = -3;
        }
    }
    else {
        checkAndLogMsg ("ProxyDatagramSocket::sendTo", Logger::L_MildError,
                        "cannot send - not connected to the proxy server\n");
        rc = -4;
    }
    connectToProxyServer();
    return rc;
}

int ProxyDatagramSocket::receive (void *pBuf, int iBufSize)
{
    return receive (pBuf, iBufSize, &lastPacketAddr);
}

int ProxyDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
{
    if ((pBuf == NULL) || (iBufSize <= 0) || (pRemoteAddr == NULL)) {
        return -1;
    }
    int rc;
    if (_bConnectedToServer) {
        try {
            if (_ui32Timeout > 0) {
                /*!!*/ // This is a hack to implement the timeout
                int64 i64StartTime = getTimeInMilliseconds();
                while (_pSocketToServer->bytesAvail() <= 0) {
                    checkAndLogMsg ("ProxyDatagramSocket::receive", Logger::L_HighDetailDebug,
                                    "no data to receive; LP=%d-RP=%d\n",
                                    _pSocketToServer->getLocalPort(), _pSocketToServer->getRemotePort());
                    sleepForMilliseconds (1000);
                    uint32 ui32ElapsedTime = (uint32) (getTimeInMilliseconds() - i64StartTime);
                    if (ui32ElapsedTime > _ui32Timeout) {
                        return 0;
                    }
                }
            }
            while (true) {
                uint8 aui8Buf [MAXIMUM_PROXY_PACKET_SIZE];
                uint32 ui32PacketSize = _pchToServer->receiveBlock (aui8Buf, MAXIMUM_PROXY_PACKET_SIZE);
                if (ui32PacketSize < 1) {
                    throw ProtocolException ("received a short packet from the server");
                }
                else if ((aui8Buf[0] == 'f') && (ui32PacketSize == 2)) {
                    // Received a flow control message from the server
                    _bClearToSend = (aui8Buf[1] != 0);
                    checkAndLogMsg ("ProxyDatagramSocket::receive", Logger::L_Info,
                                    "clear to send is %s\n", _bClearToSend ? "true" : "false");
                }
                else if ((aui8Buf[0] == 'd') && (ui32PacketSize >= 13)) {
                    uint32 ui32BufSize = (uint32) iBufSize;
                    uint32 ui32SourceAddr = *((uint32*)(aui8Buf+1));
                    uint16 ui16SourcePort = EndianHelper::ntohs (*((uint16*)(aui8Buf+5)));
                    uint32 ui32DestAddr = *((uint32*)(aui8Buf+7));
                    uint16 ui16DestPort = EndianHelper::ntohs (*((uint16*)(aui8Buf+11)));
                    if (ui32BufSize < (ui32PacketSize - 13)) {
                        checkAndLogMsg ("ProxyDatagramSocket::receive", Logger::L_MildError,
                                        "user buffer size <%lu> is too small to receive incoming message of size <%lu>\n",
                                        ui32BufSize, (ui32PacketSize - 13));
                        return -2;
                    }
                    memcpy (pBuf, aui8Buf + 13, ui32PacketSize - 13);
                    pRemoteAddr->setIPAddress (ui32SourceAddr);
                    pRemoteAddr->setPort (ui16SourcePort);
                    _ui32ReceivedPacketCount++;
                    checkAndLogMsg ("ProxyDatagramSocket::received", Logger::L_MediumDetailDebug,
                                    "received packet %u\n", _ui32ReceivedPacketCount);
                    return ui32PacketSize - 13;
                }
                else {
                    throw ProtocolException ("received an unknown packet or a short packet from server");
                }
            }
        }
        catch (Exception e) {
            checkAndLogMsg ("ProxyDatagramSocket::receive", Logger::L_MildError,
                            "exception occurred when trying to receive data from proxy server; message = <%s>\n",
                            e.getMsg());
            close();
            rc = -3;
        }
    }
    else {
        checkAndLogMsg ("ProxyDatagramSocket::receive", Logger::L_MildError,
                        "cannot receive - not connected to the proxy server\n");
        rc = -4;
    }
    connectToProxyServer();
    return rc;
}

#if defined (UNIX)
    int ProxyDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr, int &iIncomingIfaceIdx)
    {
        // TODO: implement this
        checkAndLogMsg ("ProxyDatagramSocket::receive 2", Logger::L_Warning,
                        "incoming interface identification not applicable to ProxyDatagramSocket\n");
        return receive (pBuf, iBufSize, pRemoteAddr);
    }
#endif

int ProxyDatagramSocket::getLastError (void)
{
    // TODO: implement this
    checkAndLogMsg ("ProxyDatagramSocket::getLastError", Logger::L_SevereError,
                    "method not yet implemented\n");
    return -1;
}

int ProxyDatagramSocket::connectToProxyServer (void)
{
    _m.lock();
    if (!_bReadyToConnect) {
        checkAndLogMsg ("ProxyDatagramSocket::connectToProxyServer", Logger::L_Info,
                        "proxy not ready to connect to server yet\n");
        _m.unlock();
        return 0;
    }
    if (_pConnector != NULL) {
        if (_pConnector->hasTerminated()) {
            if (_bConnectedToServer) {
                // Nothing to do!
                _m.unlock();
                return 0;
            }
            else {
                checkAndLogMsg ("ProxyDatagramSocket::connectToProxyServer", Logger::L_MildError,
                                "*************** DELETING pConnector *****************\n");
                delete _pConnector;
               _pConnector = NULL;
            }
        }
        else {
            // Have to wait - connector is still running
            checkAndLogMsg ("ProxyDatagramSocket::connectToProxyServer", Logger::L_Info,
                            "connection to proxy server is still being established\n");
            _m.unlock();
            return -1;
        }
    }
    if (getTimeInMilliseconds() < (_i64LastReconnectAttemptTime + RECONNECT_INTERVAL)) {
        checkAndLogMsg ("ProxyDatagramSocket::connectToProxyServer", Logger::L_Info,
                        "not connected to proxy server - waiting for reconnect interval\n");
        _m.unlock();
        return -2;
    }
    _pConnector = new Connector (this);
    _pConnector->start();
    _i64LastReconnectAttemptTime = getTimeInMilliseconds();
    checkAndLogMsg ("ProxyDatagramSocket::connectToProxyServer", Logger::L_Info,
                    "not connected to proxy server - started background connection attempt\n");
    _m.unlock();
    return 0;
}

Connector::Connector (ProxyDatagramSocket *pPDS)
{
    _pPDS = pPDS;
}

void Connector::run (void)
{
    int rc;
    started();
    _pPDS->_bConnectedToServer = false;
    Socket *pSocketToServer = new TCPSocket();
    if (0 != (rc = ((TCPSocket*)pSocketToServer)->connect (_pPDS->_proxyServerAddr.getIPAsString(), _pPDS->_proxyServerAddr.getPort()))) {
        checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_Warning,
                        "failed to connect to proxy server at %s:%d\n",
                        _pPDS->_proxyServerAddr.getIPAsString(), (int) _pPDS->_proxyServerAddr.getPort());
        delete pSocketToServer;
        setTerminatingResultCode (-1);
        terminating();
        return;
    }
    checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_Info,
                    "connected to proxy server; LP:%d-RP:%d\n", pSocketToServer->getLocalPort(), pSocketToServer->getRemotePort());
    pSocketToServer->bufferingMode (0);
    CommHelper2 *pchToServer = new CommHelper2();
    if (0 != (rc = pchToServer->init (pSocketToServer))) {
        checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_MildError,
                        "failed to initialize CommHelper; rc = %d\n", rc);
        delete pchToServer;
        delete pSocketToServer;
        setTerminatingResultCode (-2);
        terminating();
        return;
    }
    InetAddr localAddr;
    uint16 ui16AssignedPort;
    uint16 ui16MTU;
    try {
        uint8 aui8Buf [ProxyDatagramSocket::MAXIMUM_PROXY_PACKET_SIZE];

        // Receive the Welcome message
        uint32 ui32PacketSize = pchToServer->receiveBlock (aui8Buf, ProxyDatagramSocket::MAXIMUM_PROXY_PACKET_SIZE);
        if ((ui32PacketSize < 1) || (aui8Buf[0] != 'w')) {
            throw ProtocolException ("expecting but did not receive a welcome message from server");
        }
        checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_MediumDetailDebug,
                        "connected to proxy server at %s:%d\n",
                        _pPDS->_proxyServerAddr.getIPAsString(), (int) _pPDS->_proxyServerAddr.getPort());
        if (ui32PacketSize > 2) {
            aui8Buf[ui32PacketSize] = '\0';      // Just for safety
            checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_MediumDetailDebug,
                            "proxy server identification string is <%s>\n",
                            (char*) aui8Buf+2);
        }

        // Receive the Address message
        ui32PacketSize = pchToServer->receiveBlock (aui8Buf, ProxyDatagramSocket::MAXIMUM_PROXY_PACKET_SIZE);
        if ((ui32PacketSize < 6) || (aui8Buf[0] != 'a')) {
            throw ProtocolException ("expecting but did not receive an address message from server");
        }
        if (aui8Buf[1] != 0) {
            if (aui8Buf[1] == 4) {
                // Received a 4-byte address - store it as the local address
                localAddr = InetAddr (*((uint32*)(aui8Buf+2)));
                checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_LowDetailDebug,
                                "set local address to %s (%x)\n",
                                localAddr.getIPAsString(), localAddr.getIPAddress());
            }
            else {
                checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_Warning,
                                "received a local address of length %d - ignoring since it is not a 4 byte address\n",
                                (int) aui8Buf[1]);
            }
        }

        // Receive the MTU message
        ui32PacketSize = pchToServer->receiveBlock (aui8Buf, ProxyDatagramSocket::MAXIMUM_PROXY_PACKET_SIZE);
        if ((ui32PacketSize < 3) || (aui8Buf[0] != 'm')) {
            throw ProtocolException ("expecting but did not receive an MTU message from server");
        }
        ui16MTU = EndianHelper::ntohs (*((uint16*)(aui8Buf+1)));
        checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_LowDetailDebug,
                        "set MTU to %d\n", (int) ui16MTU);

        // Send the bind message
        checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_LowDetailDebug,
                        "sending a bind message; local port = %d\n", _pPDS->_ui16LocalPort);
        aui8Buf[0] = 'B';
        (*((uint16*)(aui8Buf+1))) = EndianHelper::htons (_pPDS->_ui16LocalPort);
        pchToServer->sendBlock (aui8Buf, 3);

        // Receive the reply
        ui32PacketSize = pchToServer->receiveBlock (aui8Buf, ProxyDatagramSocket::MAXIMUM_PROXY_PACKET_SIZE);
        if ((ui32PacketSize < 3) || (aui8Buf[0] != 'b')) {
            throw ProtocolException ("expecting but did not receive a bind reply message from server");
        }
        ui16AssignedPort = EndianHelper::ntohs (*((uint16*)(aui8Buf+1)));

        if (ui16AssignedPort == 0) {
            throw ProtocolException ("server failed to assign a port");
        }

        _pPDS->_ui16LocalPort = ui16AssignedPort;
        checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_MediumDetailDebug,
                        "bound socket to local port <%d>\n",
                        (int) _pPDS->_ui16LocalPort);
    }
    catch (Exception e) {
        checkAndLogMsg ("[ProxyDatagramSocket]Connector::run", Logger::L_MildError,
                        "exception when establishing connection to proxy server; message = <%s>\n",
                        e.getMsg());
        delete pchToServer;
        delete pSocketToServer;
        this->setTerminatingResultCode (-3);
        terminating();
        return;
    }
    _pPDS->_pchToServer = pchToServer;
    _pPDS->_pSocketToServer = pSocketToServer;
    _pPDS->_ui16MTU = ui16MTU;
    _pPDS->_ui32LocalAddr = localAddr.getIPAddress();
    _pPDS->_ui16LocalPort = ui16AssignedPort;
    _pPDS->_bConnectedToServer = true;
    setTerminatingResultCode (0);
    terminating();
    return;
}
