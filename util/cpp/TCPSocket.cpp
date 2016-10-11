/*
 * TCPSocket.cpp
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
 * RCSInfo: $Header: /export/cvs/nomads.root/util/cpp/TCPSocket.cpp,v 1.61 2016/06/09 20:02:45 gbenincasa Exp $
 * Revision: $Revision: 1.61 $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "TCPSocket.h"

#if defined (WIN16)

    //#include "winsock.h"
    #define close closesocket
    #define ioctl ioctlsocket
    #define bzero(x,y) memset((x),'\0',(y))
    #define bcopy(x,y,z) memcpy((y),(x),(z))
    #define EINTR WSAEINTR
    #define EWOULDBLOCK WSAEWOULDBLOCK
    #ifndef MAKEWORD
        #define MAKEWORD(a, b)  ((WORD)(((BYTE)(a)) | (((WORD)((BYTE)(b))) << 8)))
    #endif

#elif defined (WIN32)
    #define close closesocket
    #define bzero(x,y) memset((x),'\0',(y))
    #define bcopy(x,y,z) memcpy((y),(x),(z))
//    #define EINTR WSAEINTR
    #define EWOULDBLOCK WSAEWOULDBLOCK
    typedef int socklen_t;

#elif defined (OS2)

    #define BSD_SELECT
    #include <types.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <nerrno.h>
    #include <utils.h>
    #define close soclose
    #ifndef EINTR
         #define EINTR SOCEINTR
    #endif
    #define EWOULDBLOCK SOCEWOULDBLOCK
    int TCPSocket::iSockInitCalled;
    unsigned short htons (unsigned short us) { return bswap(us); }
    unsigned short ntohs (unsigned short us) { return bswap(us); }

#elif defined (UNIX)

    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <errno.h>
    #include <sys/socket.h>
    //#include <sys/socketvar.h>
    #include <unistd.h>
    #include <sys/select.h>
    #if defined (accept)                     // For AIX 4.1
         #define DEFINED_ACCEPT naccept
         #undef accept
    #endif
    #if defined (__QNX__)
         #include <unix.h>
         #define FNONBLK O_NONBLOCK
    #else
         #include <strings.h>
    #endif

#endif

#if defined (WIN16) || defined (WIN32) || defined (OS2)
    #define fd_set struct fd_set
#endif

using namespace NOMADSUtil;

#if defined (WIN16) || defined (WIN32)
    void * TCPSocket::pUserData;
    TCPSocketIdleCallbackFuncType TCPSocket::pIdleFn;
#endif

TCPSocket::TCPSocket (void)
{
    iSocket = 0;
    bCleanupAllowed = false;//true;	//Set flag which allows WSACleanup on object destruction
    bzero ((char*)&lastConnectSockAddr, sizeof (lastConnectSockAddr));
    #if defined(UNIX)
        signal (SIGPIPE, SIG_IGN);
    #elif defined(OS2)
        if (!iSockInitCalled) {
            sock_init();
            iSockInitCalled = 1;
        }
    #elif defined(WIN16)
        pIdleFn = NULL;
        pUserData = NULL;
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(1,1);
        WSAStartup(wVersionRequested, &wsaData);
    #elif defined (WIN32)
        pIdleFn = NULL;
        pUserData = NULL;
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2,2);
        WSAStartup (wVersionRequested, &wsaData);
    #endif
}

TCPSocket::TCPSocket (int aSocket)
{
    iSocket = aSocket;
    iConnected = 1;
    bCleanupAllowed = false; //Set flag which allows WSACleanup on object destruction
    bzero ((char*)&lastConnectSockAddr, sizeof (lastConnectSockAddr));
    #if defined(UNIX)
        signal (SIGPIPE, SIG_IGN);
    #elif defined(OS2)
        if (!iSockInitCalled) {
            sock_init();
            iSockInitCalled = 1;
        }
    #elif defined(WIN16) || defined (WIN32)
        pIdleFn = NULL;
        pUserData = NULL;
    #endif
}

TCPSocket::~TCPSocket (void)
{
    if (iSocket) {
        close (iSocket);
    }
    #if defined(WIN16) || defined (WIN32)
        if (bCleanupAllowed) {
            WSACleanup ();
        }
    #endif
}

int TCPSocket::blockingMode (int iMode)
{
    if (iMode) {
        #if defined(WIN16) || defined (WIN32)
            if (WSAAsyncSelect (iSocket, hWnd, 0, 0)) {
                return -1;
            }
            unsigned long ulMode = 0;
            if (ioctlsocket (iSocket, FIONBIO, &ulMode)) {
                return -2;
            }
        #elif defined(OS2)
            // Do whatever OS/2 requires
        #else
            int iFlags;
            iFlags = fcntl (iSocket, F_GETFL);
            iFlags &= ~O_NONBLOCK;
            if (fcntl (iSocket, F_SETFL, iFlags)) {
                return -1;
            }
        #endif
    }
    else {
        #if defined(WIN16) || defined (WIN32)
            unsigned long ulMode = 0;
            if (ioctlsocket (iSocket, FIONBIO, &ulMode)) {
                return -1;
            }
        #elif defined (OS2)
            // Do whatever OS/2 requires
        #else
            int iFlags;
            iFlags = fcntl (iSocket, F_GETFL);
            iFlags |= O_NONBLOCK;
            if (fcntl (iSocket, F_SETFL, iFlags)) {
                return -1;
            }
        #endif
    }
    iBlockingMode = iMode;
    return 0;
}

int TCPSocket::bufferingMode (int iMode)
{
    #if defined (WIN32) || defined (UNIX)
        int optval;
        if (iMode) {
            optval = 0;
        }
        else {
            optval = 1;
        }
        if (setsockopt (iSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval))) {
            return -2;
        }
    #else
        return -1;
    #endif
    return 0;
}

int TCPSocket::getLingerMode (void)
{
    struct linger l;
    socklen_t optlen = sizeof (l);
    if (getsockopt (iSocket, SOL_SOCKET, SO_LINGER, (char*)&l, &optlen)) {
        return -1;
    }
    return l.l_onoff;
}

int TCPSocket::getLingerTime (void)
{
    struct linger l;
    socklen_t optlen = sizeof (l);
    if (getsockopt (iSocket, SOL_SOCKET, SO_LINGER, (char*)&l, &optlen)) {
        return -1;
    }
    return l.l_linger;
}

int TCPSocket::setLingerOptions (int iMode, int iTime)
{
    struct linger l;
    l.l_onoff = iMode;
    l.l_linger = iTime;
    if (setsockopt (iSocket, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof (l))) {
        return -1;
    }
    return 0;
}

const char * TCPSocket::getLocalHostName (void)
{
    if (localHostName.length() > 0) {
        return localHostName;
    }
    localHostName.setSize (256);
    if (gethostname ((char*)localHostName, 255)) {
        return NULL;
    }
    if (strchr ((char*)localHostName, '.') == NULL) { //assume it's not FQDN
        hostent *hostinfo;
        if((hostinfo = gethostbyname ((char*)localHostName)) != NULL) {
            if (strlen (hostinfo->h_name) > strlen ((char*)localHostName)) {
                localHostName = hostinfo->h_name;
            }
        }
    }
    return localHostName;
}

int TCPSocket::connect (const char *pszHostName, unsigned short usPortNum)
{
    struct sockaddr_in remote;
    struct hostent *hp;

    if ((iConnected) || (iSetupToReceive)) {
        return -1;
    }

    // Check if the hostname is actually an IP address
    if (checkIfStringIsIPAddr (pszHostName)) {
        unsigned long ulAddr = inet_addr (((char*)pszHostName));
        bcopy ((char*) &ulAddr, (char*) &lastConnectSockAddr.sin_addr, sizeof (unsigned long));
        lastConnectSockAddr.sin_family = AF_INET;
    }
    else {
        if (NULL == (hp = gethostbyname (((char*)pszHostName)))) {
            return -2;
        }
        bcopy (hp->h_addr, (char*) &lastConnectSockAddr.sin_addr, hp->h_length);
        lastConnectSockAddr.sin_family = hp->h_addrtype;
    }

    if ((iSocket = (int) socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        return -3;
    }

    remote = lastConnectSockAddr;
    remote.sin_port = htons (usPortNum);

    int iResult;
/*    
    #if defined (UNIX)
    if (ulTimeOutLimit) {
            alarm (ulTimeOutLimit);
        }
    #endif

*/
    iResult = ::connect (iSocket, (struct sockaddr *) &remote, sizeof (remote));
/*
    #if defined (UNIX)
        alarm (0);
    #endif
*/
    if (iResult < 0) {
        #if !defined(WIN16) && !defined (WIN32)
            if (errno == EINTR) {
                return SE_TIMEOUT;
            }
        #endif
        return -4;
    }
    
    iConnected = 1;

    remoteSockAddr = remote;

    return 0;
}

int TCPSocket::reconnect (void)
{
    if ((iConnected) || (iSetupToReceive)) {
        return -1;
    }

    struct sockaddr_in remote = lastConnectSockAddr;

    if (iSocket) {
        close (iSocket);
        iSocket = 0;
    }

    if ((iSocket = (int) socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        return -4;
    }

    int iResult;
/*
    #if defined (UNIX)
        if (ulTimeOutLimit) {
            alarm (ulTimeOutLimit);
        }
    #endif
*/
    iResult = ::connect (iSocket, (struct sockaddr *) &remote, sizeof (remote));
/*
    #if defined (UNIX)
        alarm (0);
    #endif
*/
    if (iResult < 0) {
        #if !defined(WIN16) && !defined (WIN32)
            if (errno == EINTR) {
                return SE_TIMEOUT;
            }
        #endif
        return -5;
    }

    iConnected = 1;

    return 0;
}


int TCPSocket::shutdown (bool bReadMode, bool bWriteMode)
{
    if (!bReadMode && !bWriteMode) {
        return 0;
    }

    int iHow, iRes;
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

    if (iRes = ::shutdown (iSocket, iHow)) {
        return iRes;
    }

    if (bReadMode) {
        iSetupToReceive = 0;
        if (bWriteMode) {
            iConnected = 0;
        }
    }

    return 0;
}

int TCPSocket::disconnect (void)
{
    if (!(iConnected) || (iSetupToReceive)) {
        return -1;
    }

    close (iSocket);
    iSocket = 0;
    iConnected = 0;

    return 0;
}

int TCPSocket::flushAndClose (void)
{
    if (!(iConnected) || (iSetupToReceive)) {
        return -1;
    }
    #if defined (WIN32)
        WSAEVENT closeEvent = WSACreateEvent();
        if (closeEvent == WSA_INVALID_EVENT) {
            return -2;
        }
        if (WSAEventSelect (iSocket, closeEvent, FD_CLOSE)) {
            WSACloseEvent (closeEvent);
            return -3;
        }
        if (::shutdown (iSocket, SD_SEND)) {
            WSACloseEvent (closeEvent);
            return -4;
        }
        if (0 != WSAWaitForMultipleEvents (1, &closeEvent, TRUE, WSA_INFINITE, TRUE)) {
            WSACloseEvent (closeEvent);
            return -5;
        }
        WSACloseEvent (closeEvent);
        closesocket (iSocket);
        iSocket = 0;
        iConnected = 0;
        return 0;
    #else
        
        //shutdown
        if (shutdown(iSocket,SHUT_WR) < 0) {
            return disconnect();
        }
        
        //read from the client until we get EOF or 2 seconds are passed
        fd_set t;
        struct timeval ti;
        ti.tv_sec=2;
        ti.tv_usec=0;
        FD_ZERO(&t);
        FD_SET(iSocket,&t);
        int iResult;
        char pBuf[512];
        int iSize = 512;
        
        while (select(iSocket+1, &t, NULL, NULL, &ti) > 0){
            if (FD_ISSET (iSocket, &t)){
                if ((iResult = ::recv (iSocket, pBuf, iSize, 0)) <= 0){
                    break;
                }
            }
            ti.tv_sec=2;
            ti.tv_usec=0;
            FD_ZERO(&t);
            FD_SET(iSocket,&t);
        }
        
        return disconnect();
    #endif
}

int TCPSocket::setupToReceive (unsigned short usPortNum, int iQueueSize, unsigned long ulIPAddr)
{
    struct sockaddr_in local;

    if ((iConnected) || (iSetupToReceive)) {
        return -1;
    }

    if ((iSocket = (int) socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        return -2;
    }

    int opt_val = 1;
    if (setsockopt (iSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt_val, sizeof(opt_val)) < 0) {
        return -3;
    }

    #if defined (OSX)
        opt_val = 1;
        //enables duplicate address and port bindings
        if (setsockopt (iSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&opt_val, sizeof(opt_val)) < 0) {
            return -4;
        }
    #endif

    bzero ((char*) &local, sizeof (local));
    local.sin_family = AF_INET;
    local.sin_port = htons (usPortNum);
    local.sin_addr.s_addr = ulIPAddr;           //INADDR_ANY

    if (bind (iSocket, (struct sockaddr *) &local, sizeof (local)) < 0) {
        return -5;
    }

    listen (iSocket, iQueueSize);
    iSetupToReceive = 1;

    return 0;
}

int TCPSocket::disableReceive (void)
{
    if ((iConnected) || !(iSetupToReceive)) {
        return -1;
    }

    close (iSocket);
    iSocket = 0;
    iSetupToReceive = 0;

    return 0;
}

Socket * TCPSocket::accept (void)
{
    if ((iConnected) || !(iSetupToReceive)) {
        return NULL;
    }

    struct sockaddr_in remote;
    socklen_t sockAddrLen = sizeof (remote);
    int iNewSocket; 

    #if defined (DEFINED_ACCEPT)
        if ((iNewSocket = ::DEFINED_ACCEPT (iSocket, (struct sockaddr *) &remote, &iSockAddrLen)) < 0) {
            return NULL;
        }
    #else
        #if defined (WIN16) || defined (WIN32) || defined (OS2)
            if (ulTimeOutLimit > 0) {
                struct timeval tv;
                tv.tv_sec = ulTimeOutLimit / 1000;
                tv.tv_usec = (ulTimeOutLimit % 1000) * 1000;
                fd_set fds;
                FD_ZERO (&fds);
                FD_SET (iSocket, &fds);
                int rc;
                if (0 == (rc = select (iSocket+1, &fds, NULL, NULL, &tv))) {
                    // No incoming connection
                    return NULL;
                }
                else if (rc < 0) {
                    // Error occurred in select();
                    return NULL;
                }
                else {
                    // There must be a connection to accept
                    if ((iNewSocket = (int) ::accept (iSocket, (struct sockaddr *) &remote, &sockAddrLen)) < 0) {
                        return NULL;
                    }
                }
            }
            else {
                if ((iNewSocket = (int) ::accept (iSocket, (struct sockaddr *) &remote, &sockAddrLen)) < 0) {
                     return NULL;
                }
            }
        #else
/*
            #if defined (UNIX)
                if (ulTimeOutLimit) {
                    alarm (ulTimeOutLimit);
                }
            #endif
            iNewSocket = ::accept (iSocket, (struct sockaddr *) &remote, &sockAddrLen);
            #if defined (UNIX)
                alarm (0);
            #endif
            if (iNewSocket < 0) {
                return NULL;
            }
*/
        // ---- marguedas ----

        if (ulTimeOutLimit > 0) {
            struct timeval tv;
            tv.tv_sec = ulTimeOutLimit / 1000;
            tv.tv_usec = (ulTimeOutLimit % 1000) * 1000;
            fd_set fds;
            FD_ZERO (&fds);
            FD_SET (iSocket, &fds);
            int rc;
            if (0 == (rc = select (iSocket+1, &fds, NULL, NULL, &tv))) {
                    // No incoming connection
                return NULL;
            }
            else if (rc < 0) {
                    // Error occurred in select();
                return NULL;
            }
            else {
                    // There must be a connection to accept
                if ((iNewSocket = (int) ::accept (iSocket, (struct sockaddr *) &remote, &sockAddrLen)) < 0) {
                    return NULL;
                }
            }
        }
        else {
            if ((iNewSocket = (int) ::accept (iSocket, (struct sockaddr *) &remote, &sockAddrLen)) < 0) {
                return NULL;
            }
        }
        #endif
    #endif

    TCPSocket *pNewSocket = new TCPSocket;
    pNewSocket->iSocket = iNewSocket;
    pNewSocket->iConnected = 1;
    pNewSocket->remoteSockAddr = remote;

    return pNewSocket;
}

const char * TCPSocket::getRemoteHostAddr (void)
{
    if (remoteHostAddr.length() > 0) {
        return remoteHostAddr;
    }
    remoteHostAddr = inet_ntoa (remoteSockAddr.sin_addr);
    return remoteHostAddr;
}

const char * TCPSocket::getRemoteHostName (void)
{
    if (remoteHostName.length() > 0) {
        return remoteHostName;
    }
    struct hostent *hp;
    if (NULL != (hp = gethostbyaddr ((char*) &remoteSockAddr.sin_addr, sizeof (in_addr), AF_INET))) {
        remoteHostName = hp->h_name;
        return remoteHostName;
    }
    return NULL;
}

unsigned short TCPSocket::getLocalPort (void)
{
    struct sockaddr_in localAddr;
    socklen_t sockAddrLen = sizeof (localAddr);
    if (getsockname (iSocket,  (struct sockaddr *) &localAddr, &sockAddrLen)) {
        return 0;
    }
    return ntohs (localAddr.sin_port);
}

unsigned short TCPSocket::getRemotePort (void)
{
    return ntohs (remoteSockAddr.sin_port);
}

Socket * TCPSocket::dup (void)
{
    if (!iConnected) {
        return NULL;
    }
    int fd;
    #if defined (WIN32)
        WSAPROTOCOL_INFO protocolInfo;
        if (WSADuplicateSocket(iSocket, GetCurrentProcessId(), &protocolInfo)) {
            return NULL;
        }
        if ((fd = (int) WSASocket (FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, &protocolInfo, 0, 0)) == INVALID_SOCKET) {
            return NULL;
        }
        TCPSocket *pDupSocket = new TCPSocket(fd);
        return pDupSocket;
    #else
        if ((fd = ::dup (iSocket)) < 0) {
            return NULL;
        }
        TCPSocket *pDupSocket = new TCPSocket(fd);
        return pDupSocket;
    #endif
}

int TCPSocket::sendImpl (const void *pBuf, int iSize)
{
    if (!iConnected) {
        return -1;
    }
    if (iSize == 0) {
        return 0;
    }

    #if defined (UNIX)
        if (ulTimeOutLimit) {
            alarm (ulTimeOutLimit);
        }
    #endif

    int iResult;
    #if defined (WIN16) || defined (WIN32) || defined (OS2)
        if ((ulTimeOutLimit == 0UL) || (!iBlockingMode)) {
            // Either the timeout is 0 (i.e., there is no timeout) or non-blocking
            // mode has been selected for the socket. In either case, just call send
            iResult = ::send (iSocket, (char *) pBuf, iSize, 0);
        }
        else {
            int iBytesSent = 0;
            // Set the socket into non-blocking mode
            if (blockingMode (0)) {
                // Setting non-blocking mode failed; try to set it back to blocking mode and return an error
                blockingMode (1);
                return -2;
            }
            while (iBytesSent < iSize) {
                struct timeval tv;
                tv.tv_sec = ulTimeOutLimit / 1000;
                tv.tv_usec = (ulTimeOutLimit % 1000) * 1000;
                fd_set fds;
                FD_ZERO (&fds);
                FD_SET (iSocket, &fds);
                if (0 == (iResult = select (iSocket+1, NULL, &fds, NULL, &tv))) {
                    // Connection timed out
                    // However, we might have sent a few bytes so check for that
                    blockingMode (1);
                    if (iBytesSent) {
                        return iBytesSent;
                    }
                    else {
                        return SE_TIMEOUT;
                    }
                }
                else if (iResult < 0) {
                    blockingMode (1);
                    return -3;
                }
                else {
                    // Write as many bytes as possible
                    iResult = ::send (iSocket, ((char*)pBuf)+iBytesSent, iSize-iBytesSent, 0);
                    if (iResult <= 0) {
                        blockingMode (1);
                        if (iBytesSent) {
                            return iBytesSent;
                        }
                        else {
                            return SE_TIMEOUT;
                        }
                    }
                    else {
                        iBytesSent += iResult;
                    }
                }
            }
            iResult = iBytesSent;
            blockingMode (1);
        }
    #else
        iResult = ::send (iSocket, (char *) pBuf, iSize, 0);
    #endif
    #if defined (UNIX)
        alarm (0);
    #endif
    if (iResult < 0)
    {
        #if !defined (WIN16) && !defined (WIN32)
            if (errno == EINTR) {
                return SE_TIMEOUT;
            }
        #endif
        if (errno == EWOULDBLOCK) {
            return SE_WOULDBLOCK;
        }
        else {
            return -2;
        }
    }
    return iResult;
}

int TCPSocket::receiveImpl (void *pBuf, int iSize)
{
    if (!iConnected) {
        return -1;
    }

    int iResult;

    if (ulTimeOutLimit == 0UL) {
        iResult = ::recv (iSocket, (char*) pBuf, iSize, 0);
    }
    else {
        struct timeval tv;
        tv.tv_sec = ulTimeOutLimit / 1000;
        tv.tv_usec = (ulTimeOutLimit % 1000) * 1000;
        fd_set fds;
        FD_ZERO (&fds);
        FD_SET (iSocket, &fds);
        if (0 == (iResult = select (iSocket+1, &fds, NULL, NULL, &tv))) {
                // No incoming data
            return SE_TIMEOUT;
        }
        else if (iResult < 0) {
                // Some other error occurred
            return -2;
        }
        else if (iResult > 0) {
                // There is some data to read
            iResult = ::recv (iSocket, (char*) pBuf, iSize, 0);
        }
    }
    
    if (0 == iResult) {
        return SE_DISCONNECT;
    }
        
    if (iResult < 0) {
        #if defined (UNIX)
            if (errno == EINTR) {
                return SE_TIMEOUT;
            }
        #endif
        if (errno == EWOULDBLOCK) {
            return SE_WOULDBLOCK;
        }
        else {
            return -3;
        }
    }

    return iResult;
}

void TCPSocket::setTimeOut (unsigned long ulTimeOut)
{
    ulTimeOutLimit = ulTimeOut;
}

int TCPSocket::error (void)
{
    #if defined (OS2)
        return sock_errno();
    #elif defined (WIN16) || defined (WIN32)
        return WSAGetLastError();
    #else
        return errno;
    #endif
}

int TCPSocket::bytesAvail (void)
{
    #if defined (WIN16) || defined (WIN32)
        unsigned long ulBytesAvail;
        if (ioctlsocket (iSocket, FIONREAD, &ulBytesAvail)) {
            return -1;
        }
        return (int) ulBytesAvail;
    #elif defined (OS2)
        int iBytesAvail;
        if (ioctl (iSocket, FIONREAD, (char*) &iBytesAvail, sizeof (iBytesAvail))) {
            return -1;
        }
        return iBytesAvail;
    #else
        int iBytesAvail;
        if (ioctl (iSocket, FIONREAD, &iBytesAvail)) {
            return -1;
        }
        return iBytesAvail;
    #endif
}

// getFileDescriptor 
// returns the file descriptor of the current socket.
int TCPSocket::getFileDescriptor (void)
{
    return iSocket;
}

// This method "getAndDisassociateFileDescriptor" will return
// the file descriptor (int) currently in use by this socket and will
// dissociate it from the socket. From this point on, the socket will
// no longer have control over the file descriptor. All Read/Write 
// operations on the socket will fail and closing or destroing the 
// socket will not affect the file descritpor (IT IS YOUR RESPONSABILITY
// TO CLOSE IT, WHEN YOU'RE DONE) (mc - Feb 2002).
int TCPSocket::getAndDisassociateFileDescriptor (void)
{
    int i = iSocket;
    iSocket = -1;
    return i;
}

int TCPSocket::isConnected(void)
{
    return iConnected;
}

#if defined (WIN16) || defined (WIN32)
int TCPSocket::setupReceiveNotification (HWND hWnd, unsigned int wMsg)
{
    if (WSAAsyncSelect (iSocket, hWnd, wMsg, FD_READ)) {
        return -1;
    }
    iNotifyOnReceive = 1;
    this->hWnd = hWnd;
    this->wMsg = wMsg;
    return 0;
}

int TCPSocket::disableReceiveNotification (void)
{
    if (WSAAsyncSelect (iSocket, hWnd, 0, 0)) {
        return -1;
    }
    iNotifyOnReceive = 0;
    if (iBlockingMode) {
        unsigned long ulMode = iBlockingMode;
        if (ioctlsocket (iSocket, FIONBIO, &ulMode)) {
            return -2;
        }
    }
    return 0;
}

int TCPSocket::setIdleCallbackFunc (TCPSocketIdleCallbackFuncType pFn, void *pUserData)
{
    if (pFn) {
        this->pIdleFn = pFn;
        this->pUserData = pUserData;
        if (WSASetBlockingHook ((FARPROC)idleCallback)) {
            return 0;
        }
        else {
            return -1;
        }
    }
    else {
         this->pIdleFn = NULL;
         this->pUserData = NULL;
         return WSAUnhookBlockingHook ();
    }
}

int TCPSocket::suspendReceiveNotification (void)
{
    return -1;
}

int TCPSocket::resumeReceiveNotification (void)
{
    return -1;
}

BOOL TCPSocket::idleCallback (void)
{
    if (pIdleFn) {
        (*pIdleFn) (pUserData);
    }
    return FALSE;
}

#endif

void sigHandler (int signal)
{
    #if defined (DEBUG_SIGNALS)
        printf ("In sigHandler: signal = %d\n", signal);
        fflush (stdout);
    #endif
}
