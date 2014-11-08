/*
 * UDPRawDatagramSocket.cpp
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

#include "UDPRawDatagramSocket.h"

#include "EndianHelper.h"

#include <errno.h>
#include <stdio.h>
#include <memory.h>

#if defined (UNIX)
    #include <sys/types.h>
    #include <sys/time.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <signal.h>
    #include <unistd.h>
    #if defined (SOLARIS)
        #include <stropts.h>
        #define FIONREAD I_NREAD
    #endif
#elif defined (WIN32)
    #include <WS2IpDef.h>
    #define ioctl ioctlsocket
    #define socklen_t int
    bool NOMADSUtil::UDPRawDatagramSocket::bWinsockInitialized;
#endif

#include "net/NetworkHeaders.h"

using namespace NOMADSUtil;

UDPRawDatagramSocket::UDPRawDatagramSocket (void)
{
    sockfd = -1;
    #if defined (WIN32)
        if (!bWinsockInitialized) {
            WSADATA wsadata;
            WSAStartup (MAKEWORD(2,2), &wsadata);
            bWinsockInitialized = true;
        }
    #endif
    #if defined (UNIX)
        _ui32TimeOut = 0;
    #endif
}

UDPRawDatagramSocket::~UDPRawDatagramSocket (void)
{
    close();
}

int UDPRawDatagramSocket::init (uint16 ui16Port, uint32 ui32ListenAddr)
{
    if (sockfd >= 0) {
        close ();
    }
    if ((sockfd = (int) socket (PF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
        return -1;
    }
    struct sockaddr_in local;
    memset (&local, 0, sizeof (local));
    local.sin_family = AF_INET;
    local.sin_port = EndianHelper::htons (ui16Port);
    local.sin_addr.s_addr = ui32ListenAddr;
    memset ((void*) local.sin_zero, 0, sizeof (local.sin_zero));

    int opt_val = 1;

    //enables local address reuse
    if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt_val, sizeof (opt_val)) < 0) {
        return -2;
    }

    #if defined (OSX)

        //enables duplicate address and port bindings
        if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt_val, sizeof (opt_val)) < 0) {
            return -3;
        }

    #endif

    //enables permission to transmit broadcast messages
    if (setsockopt (sockfd, SOL_SOCKET, SO_BROADCAST, (char*) &opt_val, sizeof (opt_val)) < 0) {
        return -4;
    }

    //bind the socket to a specified address (INADDR_ANY if not specified) and port
    if (bind (sockfd, (struct sockaddr *) &local, sizeof (local)) < 0) {
         return -5;
    }
    
    // Configure socket so that we generate the IP headers
    int iSockOpt = 1;
    #if defined (WIN32)
        if (setsockopt (sockfd, IPPROTO_IP, IP_HDRINCL, (char*)&iSockOpt, sizeof(iSockOpt)) < 0) {
    #else
        if (setsockopt (sockfd, IPPROTO_IP, IP_HDRINCL, &iSockOpt, sizeof(iSockOpt)) < 0) {
    #endif
        return -6;
    }

    return 0;
}

int UDPRawDatagramSocket::close (void)
{
    int rc;
    #if defined (WIN32)
        rc = closesocket (sockfd);
    #elif defined (UNIX)
        rc = ::close (sockfd);
    #endif
    sockfd = -1;
    return rc;
}

uint16 UDPRawDatagramSocket::getLocalPort (void)
{
    struct sockaddr_in saLocal;
    socklen_t saLen = sizeof (saLocal);
    if (getsockname (sockfd, (struct sockaddr *) &saLocal, &saLen)) {
        return 0;
    }
    return EndianHelper::ntohs (saLocal.sin_port);
}

InetAddr UDPRawDatagramSocket::getLocalAddr (void)
{
    InetAddr localAddr;
    struct sockaddr_in saLocal;
    socklen_t saLen = sizeof (saLocal);
    if (getsockname (sockfd, (struct sockaddr *) &saLocal, &saLen)) {
        return localAddr;
    }
    localAddr.setIPAddress (saLocal.sin_addr.s_addr);
    localAddr.setPort (saLocal.sin_port);
    return localAddr;
}

int UDPRawDatagramSocket::getLocalSocket()
{
    return sockfd;
}

uint16 UDPRawDatagramSocket::getMTU()
{
    // return default Ethernet MTU
    return 1500;
}

int UDPRawDatagramSocket::getReceiveBufferSize (void)
{
    int iBufSize = 0;
    socklen_t optLen = sizeof (iBufSize);
    if (getsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &iBufSize, &optLen)) {
        return -1;
    }
    if (optLen != sizeof (iBufSize)) {
        return -2;
    }
    return iBufSize;
}

int UDPRawDatagramSocket::setReceiveBufferSize (int iSize)
{
    socklen_t optLen = sizeof (iSize);
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &iSize, optLen)) {
        return -1;
    }
    return 0;
}

int UDPRawDatagramSocket::getSendBufferSize (void)
{
    int iBufSize = 0;
    socklen_t optLen = sizeof (iBufSize);
    if (getsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &iBufSize, &optLen)) {
        return -1;
    }
    if (optLen != sizeof (iBufSize)) {
        return -2;
    }
    return iBufSize;
}

int UDPRawDatagramSocket::setSendBufferSize (int iSize)
{
    socklen_t optLen = sizeof (iSize);
    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &iSize, optLen)) {
        return -1;
    }
    return 0;
}

int UDPRawDatagramSocket::setTimeout (uint32 ui32TimeoutInMS)
{
    #if defined (WIN32)
        int iTimeout = (int) ui32TimeoutInMS;
        socklen_t optLen = sizeof (iTimeout);
        if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*) &iTimeout, optLen)) {
            return -1;
        }
        if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*) &iTimeout, optLen)) {
            return -2;
        }
    #elif defined (UNIX)
        _ui32TimeOut = ui32TimeoutInMS;
    #endif

    return 0;
}

int UDPRawDatagramSocket::getTimeout (void)
{
    #if defined (WIN32)
        int iTimeout = 0;
        socklen_t optLen = sizeof (iTimeout);
        if (getsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*) &iTimeout, &optLen)) {
            return -1;
        }
        if (optLen != sizeof (iTimeout)) {
            return -2;
        }
        return iTimeout;
    #elif defined (UNIX)
        return _ui32TimeOut;
    #endif
}

int UDPRawDatagramSocket::bytesAvail (void)
{
    unsigned long ulBytesAvail;
    if (ioctl (sockfd, FIONREAD, &ulBytesAvail)) {
        return -1;
    }
    return (int) ulBytesAvail;
}

int UDPRawDatagramSocket::sendTo (uint32 ui32SrcIPAddr, uint16 ui16SrcUDPPort, uint32 ui32DestIPAddr, uint16 ui16DestUDPPort, const void *pBuf, int iBufSize)
{
    if ((pBuf == NULL) || (iBufSize < 0)) {
        return -1;
    }

    // Create a buffer and setup pointers to the headers and the data
    uint8 ui8PacketBuf[2048];
    uint16 ui16PacketSize = (uint16) (iBufSize + sizeof (IPHeader) + sizeof (UDPHeader));
    IPHeader *pIPHeader = (IPHeader*) ui8PacketBuf;
    UDPHeader *pUDPHeader = (UDPHeader*) (ui8PacketBuf + sizeof (IPHeader));
    uint8 *pui8DataBuf = ui8PacketBuf + sizeof (IPHeader) + sizeof (UDPHeader);

    // Copy the data into the right place
    memcpy (pui8DataBuf, pBuf, iBufSize);

    // Fill in the IP Header
    pIPHeader->ui8VerAndHdrLen = (4 << 4) | 5;
    pIPHeader->ui8TOS = 0;
    pIPHeader->ui16TLen = ui16PacketSize;
    pIPHeader->ui16Ident = 0;
    pIPHeader->ui16FlagsAndFragOff = 0;
    pIPHeader->ui8TTL = 5;
    pIPHeader->ui8Proto = IP_PROTO_UDP;
    pIPHeader->srcAddr.ui32Addr = ui32SrcIPAddr;
    pIPHeader->destAddr.ui32Addr = ui32DestIPAddr;
    IPHeader::computeChecksum (pIPHeader);

    // Fill in the UDP Header
    pUDPHeader->ui16SPort = ui16SrcUDPPort;
    pUDPHeader->ui16DPort = ui16DestUDPPort;
    pUDPHeader->ui16Len = (uint16) sizeof (UDPHeader) + iBufSize;
    pUDPHeader->ui16CRC = 0;
    pUDPHeader->hton();

    struct sockaddr_in remote;
    remote.sin_family = PF_INET;
    remote.sin_port = EndianHelper::htons (ui16DestUDPPort);
    remote.sin_addr.s_addr = ui32DestIPAddr;
    int rc;
    if ((rc = sendto (sockfd, (char*)ui8PacketBuf, ui16PacketSize, 0, (struct sockaddr*) &remote, sizeof (remote))) < 0) {
        return -2;
    }
    return rc;
}

int UDPRawDatagramSocket::sendTo (const char *pszSrcIPAddr, uint16 ui16SrcUDPPort, const char *pszDestIPAddr, uint16 ui16DestUDPPort, const void *pBuf, int iBufSize)
{
    if ((pszSrcIPAddr == NULL) || (pszDestIPAddr == NULL)) {
        return -1;
    }
    int rc;
    if ((rc = sendTo (inet_addr(pszSrcIPAddr), ui16SrcUDPPort, inet_addr(pszDestIPAddr), ui16DestUDPPort, pBuf, iBufSize)) < 0) {
        return -2;
    }
    return rc;
}

int UDPRawDatagramSocket::receive (void *pBuf, int iBufSize)
{
    return receive (pBuf, iBufSize, &lastPacketAddr);
}

#if defined (UNIX)

    int UDPRawDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
    {
        if ((pBuf == NULL) || (iBufSize <= 0) || (pRemoteAddr == NULL)) {
            return -1;
        }
        int rc;
        socklen_t saSourceLen = sizeof (pRemoteAddr->_sa);
        
        if (_ui32TimeOut > 0) {
            fd_set fdSet;
            FD_ZERO (&fdSet);
            FD_SET (sockfd, &fdSet);
            struct timeval tv;
            tv.tv_sec = _ui32TimeOut / 1000;
            tv.tv_usec = (_ui32TimeOut % 1000) * 1000;
            rc = select (sockfd+1, &fdSet, NULL, NULL, &tv);
            if (rc == 0) {
                // Select timed out
                return 0;
            }
            else if (rc < 0) {
                if (errno == EINTR) {
                    // Some signal interrupted this call - just return 0 for timeout
                    return 0;
                }
                else {
                    // Error in select
                    return -2;
                }
            }
            // Otherwise, there is some data to read, so just call recvfrom()
        }

        if ((rc = recvfrom (sockfd, (char*)pBuf, iBufSize, 0, (struct sockaddr*) &pRemoteAddr->_sa, &saSourceLen)) < 0) {
            pRemoteAddr->updateIPAddrString();
            if (errno == EINTR) {
                // Some signal interrupted this call - just return 0 for timeout
                return 0;
            }
            else {
                return -3;
            }
        }

        pRemoteAddr->updateIPAddrString();
        return rc;
    }

#elif defined (WIN32)

    int UDPRawDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
    {
        if ((pBuf == NULL) || (iBufSize <= 0) || (pRemoteAddr == NULL)) {
            return -1;
        }
        int rc;
        socklen_t saSourceLen = sizeof (pRemoteAddr->_sa);
        if ((rc = recvfrom (sockfd, (char*)pBuf, iBufSize, 0, (struct sockaddr*) &pRemoteAddr->_sa, &saSourceLen)) < 0) {
            int iLastError = WSAGetLastError();
            pRemoteAddr->updateIPAddrString();
            /*
             * Notes about the socket errors that we are ignoring:
             *
             * WSAETIMEDOUT: The connection has been dropped, becaui16e of a network failure 
             * or becaui16e the system on the other end went down without notice.
             *
             * WSAECONNRESET: On a UDP-datagram socket this error indicates a previous
             * send operation resulted in an ICMP Port Unreachable message.
             */
            if (iLastError == WSAETIMEDOUT || iLastError == WSAECONNRESET) {
                WSASetLastError (0);
                return 0;
            }
            return -2;
        }
        pRemoteAddr->updateIPAddrString();
        return rc;
    }

#endif

int UDPRawDatagramSocket::getLastError (void)
{
    #if defined (WIN32)
        int rc = WSAGetLastError();
        WSASetLastError(0);
        return rc;
    #elif defined (UNIX)
        return errno;
    #endif
}
