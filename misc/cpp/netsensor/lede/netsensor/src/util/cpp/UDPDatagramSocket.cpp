/*
 * UDPDatagramSocket.cpp
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

#include "UDPDatagramSocket.h"

#include "EndianHelper.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <memory.h>
#include <string>

#if defined (UNIX)
    #include <sys/types.h>
    #include <sys/time.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <sys/param.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <signal.h>
    #include <unistd.h>
    #if defined (SOLARIS)
        #include <stropts.h>
        #define FIONREAD I_NREAD
    #endif

    #if defined IP_PKTINFO
        # define DSTADDR_SOCKOPT IP_PKTINFO
        # define DSTADDR_DATASIZE (CMSG_SPACE(sizeof(struct in_pktinfo)))

        # define dstaddr(x) (&(((struct in_pktinfo *)(CMSG_DATA(x)))->ipi_addr))
        # define incomingIface(x) (((struct in_pktinfo *)(CMSG_DATA(x)))->ipi_ifindex)
    #else
        # error "can't determine IP_PKTINFO socket option"
    #endif

#elif defined (WIN32)
    #define ioctl ioctlsocket
    #define socklen_t int

    #include <windows.h>
    #include <Mswsock.h>
    #include <Ws2ipdef.h>

    bool NOMADSUtil::UDPDatagramSocket::bWinsockInitialized;
#endif

using namespace NOMADSUtil;

#if defined (UNIX)
    ssize_t ifaceInfoReceiveFrom (int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen, int &iIncomingIfaceIdx);
#endif

#if defined UNIX
    UDPDatagramSocket::UDPDatagramSocket (bool bPktInfoEnabled)
        : DatagramSocket (bPktInfoEnabled)
#else
    UDPDatagramSocket::UDPDatagramSocket (void)
#endif
{
    sockfd = -1;
    _ui16Port = 0;
    _ui32ListenAddr = 0;
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

UDPDatagramSocket::~UDPDatagramSocket (void)
{
    close();
}

int UDPDatagramSocket::init (uint16 ui16Port, uint32 ui32ListenAddr)
{
    if (sockfd >= 0) {
        close ();
    }
    if ((sockfd = (int) socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        return -1;
    }

    int rc, opt_val = 1;

    #if defined (DSTADDR_SOCKOPT)
        if (pktInfoEnabled()) {
            // DSTADDR_SOCKOPT option should be set right after socket creation before any other use of the socket
            if (setsockopt (sockfd, IPPROTO_IP, DSTADDR_SOCKOPT, (char*)&opt_val, sizeof (opt_val)) == -1) {
                return -2;
            }
        }
    #endif

    struct sockaddr_in local;
    memset (&local, 0, sizeof (local));
    local.sin_family = AF_INET;
    local.sin_port = EndianHelper::htons (ui16Port);
    local.sin_addr.s_addr = ui32ListenAddr;
    memset ((void*) local.sin_zero, 0, sizeof (local.sin_zero));

    // Enables local address reuse
    if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt_val, sizeof (opt_val)) < 0) {
        return -3;
    }

    #if defined (OSX)
        // Enables duplicate address and port bindings
        if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt_val, sizeof (opt_val)) < 0) {
            return -4;
        }
    #endif

    // Enables permission to transmit broadcast messages
    if (setsockopt (sockfd, SOL_SOCKET, SO_BROADCAST, (char*) &opt_val, sizeof (opt_val)) < 0) {
        return -5;
    }

    // Enable retrieval of the IP destination address from the IP header of a received
    if (setsockopt (sockfd, IPPROTO_IP, IP_PKTINFO, (char*) &opt_val, sizeof (opt_val)) < 0) {
        return -6;
    }

    #if defined (WIN32)
        #if defined (NTDDI_VERSION) && defined (NTDDI_WINXP) && (NTDDI_VERSION >= NTDDI_WINXP)
        rc = WSAIoctl (sockfd, SIO_GET_EXTENSION_FUNCTION_POINTER, &WSARecvMsg_GUID,
                       sizeof (WSARecvMsg_GUID), &WSARecvMsg, sizeof (WSARecvMsg),
                       &numberOfBytes, NULL, NULL);
        if (rc == SOCKET_ERROR) {
            //m_ErrorCode = WSAGetLastError();
            WSARecvMsg = NULL;
            return -2;
        }
        #endif
    #endif

    // Bind the socket to a specified address (INADDR_ANY if not specified) and port
    if (bind (sockfd, (struct sockaddr *) &local, sizeof (local)) < 0) {
         return -8;
    }
    _ui16Port       = ui16Port;             // Store the port that was used
    _ui32ListenAddr = ui32ListenAddr;       // Store the listen addr that was used

    return 0;
}

int UDPDatagramSocket::close (void)
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

int UDPDatagramSocket::shutdown (bool bReadMode, bool bWriteMode)
{
    if (!bReadMode && !bWriteMode) {
        return 0;
    }

    int iHow = -1;
#if defined (WIN32)
    if (bReadMode && bWriteMode) {
        iHow = SD_BOTH;
    }
    else if (bReadMode) {
        iHow = SD_RECEIVE;
    }
    else {
        iHow = SD_SEND;
    }
#elif defined (LINUX)
    if (bReadMode && bWriteMode) {
        iHow = SHUT_RDWR;
    }
    else if (bReadMode) {
        iHow = SHUT_RD;
    }
    else {
        iHow = SHUT_WR;
    }
#endif

    return ::shutdown (sockfd, iHow);
}

uint16 UDPDatagramSocket::getLocalPort (void)
{
    struct sockaddr_in saLocal;
    socklen_t saLen = sizeof (saLocal);
    if (getsockname (sockfd, (struct sockaddr *) &saLocal, &saLen)) {
        return 0;
    }
    return EndianHelper::ntohs (saLocal.sin_port);
}

InetAddr UDPDatagramSocket::getLocalAddr (void)
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

int UDPDatagramSocket::getLocalSocket()
{
    return sockfd;
}

uint16 UDPDatagramSocket::getMTU()
{
    // return default Ethernet MTU
    return 1500;
}

int UDPDatagramSocket::getReceiveBufferSize (void)
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

int UDPDatagramSocket::setReceiveBufferSize (int iSize)
{
    socklen_t optLen = sizeof (iSize);
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &iSize, optLen)) {
        return -1;
    }
    return 0;
}

int UDPDatagramSocket::getSendBufferSize (void)
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

int UDPDatagramSocket::setSendBufferSize (int iSize)
{
    socklen_t optLen = sizeof (iSize);
    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &iSize, optLen)) {
        return -1;
    }
    return 0;
}

int UDPDatagramSocket::setTimeout (uint32 ui32TimeoutInMS)
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

int UDPDatagramSocket::getTimeout (void)
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

int UDPDatagramSocket::bytesAvail (void)
{
    unsigned long ulBytesAvail;
    if (ioctl (sockfd, FIONREAD, &ulBytesAvail)) {
        return -1;
    }
    return (int) ulBytesAvail;
}

bool UDPDatagramSocket::clearToSend (void)
{
    /*!!*/ // Flow control not implemented for UDP
    return true;
}

int UDPDatagramSocket::sendTo (uint32 ui32IPv4Addr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
    if ((pBuf == NULL) || (iBufSize < 0)) {
        return -1;
    }
    struct sockaddr_in remote;
    remote.sin_family = PF_INET;
    remote.sin_port = EndianHelper::htons (ui16Port);
    remote.sin_addr.s_addr = ui32IPv4Addr;
    int rc;
    if ((rc = sendto (sockfd, (char*)pBuf, iBufSize, 0, (struct sockaddr*) &remote, sizeof (remote))) < 0) {
        return -2;
    }
    #ifdef DATAGRAM_SOCKET_INCLUDE_RATE_LIMIT_CODE
        enforceTransmitRateLimit (ui32IPv4Addr, iBufSize);
    #endif
    return rc;
}

int UDPDatagramSocket::sendTo (const char *pszAddr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
    if (pszAddr == NULL) {
        return -1;
    }
    if (!InetAddr::isIPv4Addr (pszAddr)) {
        return -2;
    }

    int rc;
    if ((rc = sendTo (inet_addr (pszAddr), ui16Port, pBuf, iBufSize)) < 0) {
        return -3;
    }

    return rc;
}

#if defined (UNIX)

    int UDPDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
    {
        int iIncomingIfaceIdx = 0;
        return receive (pBuf, iBufSize, pRemoteAddr, iIncomingIfaceIdx);
    }

    int UDPDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pLocalAddr, InetAddr *pRemoteAddr)
    {
        if ((pBuf == NULL) || (iBufSize <= 0) || (pLocalAddr == NULL) || (pRemoteAddr == NULL)) {
            return -1;
        }

        int rc;
        struct msghdr mh;

        memset (&mh, 0, sizeof (mh));
        mh.msg_name = &pRemoteAddr->_sa;
        mh.msg_namelen = sizeof (pRemoteAddr->_sa);
        mh.msg_control = msgCtrlBuf;
        mh.msg_controllen = sizeof (msgCtrlBuf);

        if ((rc = recvmsg (sockfd, &mh, 0)) < 0) {
            if (errno == EINTR) {
                // Some signal interrupted this call - just return 0 for timeout
                memset (&pRemoteAddr->_sa, 0, sizeof (pRemoteAddr->_sa));
                memset (&pLocalAddr->_sa, 0, sizeof (pLocalAddr->_sa));
                return 0;
            }

            memset (&pRemoteAddr->_sa, 0, sizeof (pRemoteAddr->_sa));
            memset (&pLocalAddr->_sa, 0, sizeof (pLocalAddr->_sa));
            return -2;
        }

        for (struct cmsghdr * cmsg = CMSG_FIRSTHDR (&mh); cmsg != NULL; cmsg = CMSG_NXTHDR (&mh, cmsg)) {
            // ignore the control headers that don't match what we want
            if (cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_PKTINFO) {
                continue;
            }

            struct in_pktinfo *pi = reinterpret_cast<in_pktinfo *> (CMSG_DATA (cmsg));
            // at this point, pRemoteAddr->_sa is the source sockaddr
            // pi->ipi_spec_dst is the destination in_addr
            // pi->ipi_addr is the receiving interface in_addr
            pLocalAddr->_sa.sin_addr = pi->ipi_spec_dst;
            break;
        }

        if ((rc = recv (sockfd, (char *) pBuf, iBufSize, 0)) < 0) {
            if (errno == EINTR) {
                // Some signal interrupted this call - just return 0 for timeout
                memset (&pRemoteAddr->_sa, 0, sizeof (pRemoteAddr->_sa));
                memset (&pLocalAddr->_sa, 0, sizeof (pLocalAddr->_sa));
                return 0;
            }

            memset (&pRemoteAddr->_sa, 0, sizeof (pRemoteAddr->_sa));
            memset (&pLocalAddr->_sa, 0, sizeof (pLocalAddr->_sa));
            return -3;
        }

        // Update string representation of Src and Dst IP addresses
        pRemoteAddr->updateIPAddrString();
        pLocalAddr->updateIPAddrString();

        return rc;
    }

    int UDPDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr, int &iIncomingIfaceIdx)
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

        if (pktInfoEnabled()) {
            rc = ifaceInfoReceiveFrom (sockfd, (char*)pBuf, iBufSize, 0, (struct sockaddr*) &pRemoteAddr->_sa, &saSourceLen, iIncomingIfaceIdx);
		}
		else {
            rc = recvfrom (sockfd, (char*)pBuf, iBufSize, 0, (struct sockaddr*) &pRemoteAddr->_sa, &saSourceLen);
		}
        if (rc < 0) {
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

    ssize_t ifaceInfoReceiveFrom (int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen, int &iIncomingIfaceIdx)
    {
        struct iovec iov;
        iov.iov_base = buf;
        iov.iov_len = len;

        struct msghdr msg;
        msg.msg_name = from;
        msg.msg_namelen = *fromlen;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_controllen = DSTADDR_DATASIZE;
        struct cmsghdr *pCmsg = (struct cmsghdr*) calloc (1, msg.msg_controllen);
        if (pCmsg == NULL) {
            return -1;
        }
        msg.msg_control = pCmsg;
        msg.msg_flags = 0;

        size_t rc = recvmsg (s, &msg, flags);
        if (rc < 0) {
            free (pCmsg);
            return -2;
        }

        *fromlen = msg.msg_namelen;

        /* and arrange for IP_RECVDSTADDR to be available */
        if (msg.msg_controllen > 0) {
            for (pCmsg = CMSG_FIRSTHDR (&msg); pCmsg != NULL; pCmsg = CMSG_NXTHDR (&msg, pCmsg)) {
                if (pCmsg->cmsg_level == IPPROTO_IP && pCmsg->cmsg_type == DSTADDR_SOCKOPT) {
                    #if defined DSTADDR_SOCKOPT
                        iIncomingIfaceIdx = incomingIface(pCmsg);
                    #endif
                    break;
                }
            }
        }

        free (pCmsg);
        return rc;
    }

#elif defined (WIN32)

    int UDPDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
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
             * WSAETIMEDOUT: The connection has been dropped, because of a network failure
             * or because the system on the other end went down without notice.
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

    #if defined (NTDDI_VERSION) && defined (NTDDI_WINXP) && (NTDDI_VERSION >= NTDDI_WINXP)
        int UDPDatagramSocket::receive (void *pBuf, int iBufSize, InetAddr *pLocalAddr, InetAddr *pRemoteAddr, int iFlags)
        {
            if ((pBuf == NULL) || (iBufSize <= 0) || (pLocalAddr == NULL) || (pRemoteAddr == NULL)) {
                return -1;
            }

            int rc;
            char ControlBuffer[1024];
            WSABUF WSABuf;
            WSAMSG Msg;
            Msg.name = reinterpret_cast<sockaddr *> (&(pRemoteAddr->_sa));
            Msg.namelen = sizeof (sockaddr_in);
            WSABuf.buf = (char *)pBuf;
            WSABuf.len = iBufSize;
            Msg.lpBuffers = &WSABuf;
            Msg.dwBufferCount = 1;
            Msg.Control.buf = ControlBuffer;
            Msg.Control.len = sizeof(ControlBuffer);
            Msg.dwFlags = iFlags;
            rc = WSARecvMsg (sockfd, &Msg, &numberOfBytes, NULL, NULL);
            if (rc == SOCKET_ERROR) {
                int iLastError = WSAGetLastError();
                pRemoteAddr->updateIPAddrString();
                /*
                * Notes about the socket errors that we are ignoring:
                *
                * WSAETIMEDOUT: The connection has been dropped, because of a network failure
                * or because the system on the other end went down without notice.
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

            // There can be multiple enties but the following will show only the first.
            WSACMSGHDR *pCMsgHdr = WSA_CMSG_FIRSTHDR (&Msg);
            if (pCMsgHdr) {
                switch (pCMsgHdr->cmsg_type) {
                case IP_PKTINFO:
                    {
                        IN_PKTINFO *pPktInfo;
                        pPktInfo = (IN_PKTINFO *) WSA_CMSG_DATA(pCMsgHdr);
                        pLocalAddr->setIPAddress (pPktInfo->ipi_addr.S_un.S_addr);
                    }
                    break;
                }
            }

            return numberOfBytes;
        }
    #endif  // NTDDI_VERSION >= NTDDI_WINXP

#endif

int UDPDatagramSocket::getLastError (void)
{
    #if defined (WIN32)
        int rc = WSAGetLastError();
        WSASetLastError(0);
        return rc;
    #elif defined (UNIX)
        return errno;
    #endif
}
