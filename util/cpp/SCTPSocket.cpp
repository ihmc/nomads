/*
 * SCTPSocket.cpp
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
 * RCSInfo: $Header$
 * Revision: $Revision$
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "SCTPSocket.h"

#if defined (UNIX)

    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netinet/sctp.h>
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

using namespace NOMADSUtil;

SCTPSocket::SCTPSocket (void)
{
    iSocket = 0;
    bCleanupAllowed = false;//true;	//Set flag which allows WSACleanup on object destruction
    bzero ((char*)&lastConnectSockAddr, sizeof (lastConnectSockAddr));
    #if defined(UNIX)
        signal (SIGPIPE, SIG_IGN);
    #elif defined (WIN32)
        #error Currently not supported on Win32
    #endif
}

SCTPSocket::SCTPSocket (int aSocket)
{
    iSocket = aSocket;
    iConnected = 1;
    bCleanupAllowed = false; //Set flag which allows WSACleanup on object destruction
    bzero ((char*)&lastConnectSockAddr, sizeof (lastConnectSockAddr));
    #if defined(UNIX)
        signal (SIGPIPE, SIG_IGN);
    #elif defined (WIN32)
        #error Currently not supported on Win32
    #endif
}

SCTPSocket::~SCTPSocket (void)
{
    if (iSocket) {
        close (iSocket);
    }
}

int SCTPSocket::blockingMode (int iMode)
{
    iBlockingMode = iMode;
    return 0;
}

int SCTPSocket::bufferingMode (int iMode)
{
    #if defined (UNIX)
        int optval;
        if (iMode) {
            optval = 0;
        }
        else {
            optval = 1;
        }
        if (setsockopt (iSocket, IPPROTO_SCTP, SCTP_NODELAY, (char*)&optval, sizeof(optval))) {
            return -2;
        }
    #else
        return -1;
    #endif
    return 0;
}

const char * SCTPSocket::getLocalHostName (void)
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

int SCTPSocket::connect (const char *pszHostName, unsigned short usPortNum)
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

    remote = lastConnectSockAddr;
    bzero((void*)& remote, sizeof (remote));
    remote.sin_port = htons (usPortNum);
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr (pszHostName);
    memset (&(remote.sin_zero), '\0', sizeof (remote.sin_zero));

    printf("*****pszHostName: %s\n", pszHostName);

    if ((iSocket = (int) socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
	printf("*****unable to create the SCTP socket");
        return -3;
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
    //printf("*****SCTPSocket iResult: %d\n", iResult);

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
        printf("*****SCTPSocket Unable to connect: %d\n", iResult);
        return -4;
    }
    
    iConnected = 1;

    remoteSockAddr = remote;
    return 0;
}

int SCTPSocket::reconnect (void)
{
    if ((iConnected) || (iSetupToReceive)) {
        return -1;
    }

    struct sockaddr_in remote = lastConnectSockAddr;

    if (iSocket) {
        close (iSocket);
        iSocket = 0;
    }

    if ((iSocket = (int) socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
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


int SCTPSocket::shutdown (bool bReadMode, bool bWriteMode)
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

int SCTPSocket::disconnect (void)
{
    if (!(iConnected) || (iSetupToReceive)) {
        return -1;
    }

    close (iSocket);
    iSocket = 0;
    iConnected = 0;

    return 0;
}

int SCTPSocket::flushAndClose (void)
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

        struct sctp_sndrcvinfo sndrcvinfo;
        struct timeval ti;
        ti.tv_sec=2;
        ti.tv_usec=0;
        FD_ZERO(&t);
        FD_SET(iSocket,&t);
        int iResult;
        char pBuf[512];
        int iSize = 512;

        struct msghdr msghdr;
        struct iovec iov;

        /* Prepare header for receiving message */
	memset (&msghdr, 0, sizeof(msghdr));
	msghdr.msg_iov    = &iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control    = &pBuf;
	msghdr.msg_controllen = sizeof (pBuf);

        while (select(iSocket+1, &t, NULL, NULL, &ti) > 0){
            if (FD_ISSET (iSocket, &t)){
                if ((iResult = ::recvmsg (iSocket, &msghdr, 0)) <= 0){
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

int SCTPSocket::setupToReceive (unsigned short usPortNum, int iQueueSize, unsigned long ulIPAddr)
{
    struct sockaddr_in local;
    struct sctp_initmsg initmsg;

    if ((iConnected) || (iSetupToReceive)) {
        return -1;
    }

    if ((iSocket = (int) socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
        printf ("*****Unable to create the server socket!!");
        return -2;
    }

    int opt_val = 1;
    if (setsockopt (iSocket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0) {
        printf ("*****Unable to to do the setsockopt!!");
        return -3;
    }

    bzero ((char*) &local, sizeof (local));
    local.sin_family = AF_INET;
    local.sin_port = htons (usPortNum);
    local.sin_addr.s_addr = ulIPAddr;     //INADDR_ANY
    memset (&(local.sin_zero), '\0', sizeof (local.sin_zero));

    if (bind (iSocket, (struct sockaddr *) &local, sizeof (local)) < 0) {
        printf ("*****Unable to bind the socket!!\n");
        return -5;
    }

    if (listen (iSocket, iQueueSize) < 0) {
        printf ("failed to call listen\n");
    }

    iSetupToReceive = 1;
    return 0;
}

int SCTPSocket::disableReceive (void)
{
    if ((iConnected) || !(iSetupToReceive)) {
        return -1;
    }

    close (iSocket);
    iSocket = 0;
    iSetupToReceive = 0;

    return 0;
}

SCTPSocket * SCTPSocket::accept (void)
{
    if ((iConnected) || !(iSetupToReceive)) {
        return NULL;
    }

    struct sockaddr_in remote;
    socklen_t sockAddrLen = sizeof (remote);
    int iNewSocket; 

    #if defined (UNIX)
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
	        printf ("*****No incoming connection\n");
                return NULL;
            }
            else if (rc < 0) {
                // Error occurred in select();
	        printf ("*****Error occurred in select()\n");
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

    SCTPSocket *pNewSocket = new SCTPSocket;
    pNewSocket->iSocket = iNewSocket;
    pNewSocket->iConnected = 1;
    pNewSocket->remoteSockAddr = remote;

    return pNewSocket;
}

const char * SCTPSocket::getRemoteHostAddr (void)
{
    if (remoteHostAddr.length() > 0) {
        return remoteHostAddr;
    }
    remoteHostAddr = inet_ntoa (remoteSockAddr.sin_addr);
    return remoteHostAddr;
}

const char * SCTPSocket::getRemoteHostName (void)
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

unsigned short SCTPSocket::getLocalPort (void)
{
    struct sockaddr_in localAddr;
    socklen_t sockAddrLen = sizeof (localAddr);
    if (getsockname (iSocket,  (struct sockaddr *) &localAddr, &sockAddrLen)) {
        return 0;
    }
    return ntohs (localAddr.sin_port);
}

unsigned short SCTPSocket::getRemotePort (void)
{
    return ntohs (remoteSockAddr.sin_port);
}

SCTPSocket * SCTPSocket::dup (void)
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
        SCTPSocket *pDupSocket = new SCTPSocket(fd);
        return pDupSocket;
    #else
        if ((fd = ::dup (iSocket)) < 0) {
            return NULL;
        }
        SCTPSocket *pDupSocket = new SCTPSocket(fd);
        return pDupSocket;
    #endif
}

int SCTPSocket::sendImpl (const void *pBuf, int iSize)
{
    if (!iConnected) {
        return -1;
    }
    if (pBuf == NULL) {
        return -2;
    }
    if (iSize <= 0) {
        return -3;
    }
    
    struct msghdr mhdr;
    struct iovec  iov;
    
    memset (&mhdr, 0, sizeof (mhdr));
    memset (&iov,  0, sizeof (iov));

    iov.iov_base = (void*) pBuf;
    iov.iov_len  = iSize;
    mhdr.msg_iov = &iov;
    mhdr.msg_iovlen = 1;

    #if defined (UNIX)
        if (ulTimeOutLimit) {
            alarm (ulTimeOutLimit);
        }
    #endif

    int iResult = ::sendmsg (iSocket, &mhdr, 0);
    //printf ("**send***iResult: %d\n", iResult);

    #if defined (UNIX)
        alarm (0);
    #endif

    if (iResult < 0) {
        #if !defined (WIN16) && !defined (WIN32)
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
    //printf ("**send***returning iResult: %d\n", iResult);
    return iResult;
}

int SCTPSocket::receiveImpl (void *pBuf, int iSize)
{
    if (!iConnected) {
        return -1;
    }
    if (pBuf == NULL) {
        return -2;
    }
    if (iSize <= 0) {
        return -3;
    }

    struct sockaddr saddr;
    socklen_t saddr_len = 0;
    struct sctp_sndrcvinfo sndrcvinfo;
    int flags = 0;
    struct msghdr mhdr;
    struct iovec iov;

    // Prepare header for receiving message
    memset (&mhdr, 0, sizeof (mhdr));
    memset (&iov,  0, sizeof (iov));
    iov.iov_base = (void*) pBuf;
    iov.iov_len  = iSize;
    mhdr.msg_iov    = &iov;
    mhdr.msg_iovlen = 1;

    int iResult;

    if (ulTimeOutLimit == 0UL) {
        iResult = ::recvmsg (iSocket, &mhdr, 0);
        //printf ("***receivemsg**iResult 1: %d\n", iResult);
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
            printf ("***receivemsg::some error occurred\n");
            return -2;
        }
        else if (iResult > 0) {
            // There is some data to read
            iResult = ::recvmsg (iSocket, &mhdr, 0);
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
    //printf ("***receivemsg**iResult returning: %d\n", iResult);
    return iResult;
}

void SCTPSocket::setTimeOut (unsigned long ulTimeOut)
{
    ulTimeOutLimit = ulTimeOut;
}

int SCTPSocket::error (void)
{
    #if defined (OS2)
        return sock_errno();
    #elif defined (WIN16) || defined (WIN32)
        return WSAGetLastError();
    #else
        return errno;
    #endif
}

int SCTPSocket::bytesAvail (void)
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
int SCTPSocket::getFileDescriptor (void)
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
int SCTPSocket::getAndDisassociateFileDescriptor (void)
{
    int i = iSocket;
    iSocket = -1;
    return i;
}

int SCTPSocket::isConnected(void)
{
    return iConnected;
}

#if defined (WIN16) || defined (WIN32)
int SCTPSocket::setupReceiveNotification (HWND hWnd, unsigned int wMsg)
{
    if (WSAAsyncSelect (iSocket, hWnd, wMsg, FD_READ)) {
        return -1;
    }
    iNotifyOnReceive = 1;
    this->hWnd = hWnd;
    this->wMsg = wMsg;
    return 0;
}

int SCTPSocket::disableReceiveNotification (void)
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

int SCTPSocket::setIdleCallbackFunc (SCTPSocketIdleCallbackFuncType pFn, void *pUserData)
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

int SCTPSocket::suspendReceiveNotification (void)
{
    return -1;
}

int SCTPSocket::resumeReceiveNotification (void)
{
    return -1;
}

BOOL SCTPSocket::idleCallback (void)
{
    if (pIdleFn) {
        (*pIdleFn) (pUserData);
    }
    return FALSE;
}

#endif

/**void sigHandler (int signal)
{
    #if defined (DEBUG_SIGNALS)
        printf ("In sigHandler: signal = %d\n", signal);
        fflush (stdout);
    #endif
}**/
