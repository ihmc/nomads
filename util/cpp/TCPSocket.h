/*
 * TCPSocket.h
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
 * TCP/IP Stream Socket Class header file
 *
 * Written by Niranjan Suri
 * RCSInfo: $Header: /export/cvs/nomads.root/util/cpp/TCPSocket.h,v 1.34 2016/06/09 20:02:45 gbenincasa Exp $
 * Revision: $Revision: 1.34 $
 */

#ifndef INCL_TCPSOCKET_H
#define INCL_TCPSOCKET_H

#include "Socket.h"

#if !defined (WIN16) && !defined (WIN32) && !defined (OS2) && !defined (DOS) && !defined (UNIX)
    #define UNIX
#endif

#if defined (WIN16)
    #pragma hdrstop
#elif defined (WIN32)
#elif defined (OS2)
    #define BSD_SELECT
    #include <netinet/in.h>
    #include "utils.h"
    #undef htons
    #undef ntohs
    unsigned short ntohs (unsigned short);
    unsigned short htons (unsigned short);
#elif defined (UNIX)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <signal.h>
    #if defined (accept)
         #undef accept
    #endif
    #if defined(SOLARIS)
        #include <stropts.h>
        #define FIONREAD I_NREAD
    #endif
#endif

#include <stdio.h>
#include <string.h>

#include "StrClass.h"

namespace NOMADSUtil
{

    #if defined (WIN16) || defined (WIN32)
        typedef int (*TCPSocketIdleCallbackFuncType) (void *pUserData);
    #endif

    class TCPSocket : public Socket
    {
        public:
            TCPSocket (void);
            TCPSocket(int aSocket);
            virtual ~TCPSocket (void);
            virtual int blockingMode (int iMode);

            // Enable or disable buffering (which, for TCP, basically controls the Nagle algorithm)
            // Passing a zero indicates no buffering, which implies that the Nagle algorithm is disabled (TCP_NODELAY is set to 1)
            // Passing a non-zero value indicates that buffering should be enabled (default with TCP)
            virtual int bufferingMode (int iMode);

            virtual int getLingerMode (void);
            virtual int getLingerTime (void);
            virtual int setLingerOptions (int iMode, int iTime);
            virtual const char * getLocalHostName (void);
            virtual int connect (const char *pszHostName, unsigned short usPortNum);
            virtual int reconnect (void);
            virtual int disconnect (void);
            virtual int shutdown (bool bReadMode, bool bWriteMode);
            virtual int flushAndClose (void);
            virtual int setupToReceive (unsigned short usPortNum, int iQueueSize = 5, unsigned long ulIPAddr = INADDR_ANY);
            virtual int disableReceive (void);
            virtual Socket * accept (void);
            virtual const char * getRemoteHostAddr (void);
            virtual const char * getRemoteHostName (void);
            virtual unsigned short getLocalPort (void);
            virtual unsigned short getRemotePort (void);
            virtual Socket * dup (void);
            virtual void setTimeOut (unsigned long ulTimeOut);
            virtual int error (void);
            virtual int bytesAvail (void);
            virtual int getFileDescriptor (void);
            virtual int getAndDisassociateFileDescriptor(void);
            virtual int isConnected(void);
            #if defined (WIN16) || defined (WIN32)
                virtual int setupReceiveNotification (HWND hWnd, unsigned int wMsg);
                virtual int disableReceiveNotification (void);
                virtual int setIdleCallbackFunc (TCPSocketIdleCallbackFuncType pFn, void *pUserData);
                                                   // NOTE: pUserData will be shared by all instances of TCPSocket!
            #endif

        protected:
            virtual int sendImpl (const void *pBuf, int iSize);
            virtual int receiveImpl (void *pBuf, int iSize);
            #if defined (WIN16) || defined (WIN32)
                int suspendReceiveNotification (void);
                int resumeReceiveNotification (void);
                static BOOL idleCallback (void);
            #endif
            #if defined (OS2)
                virtual unsigned short htons (unsigned short us) {return ::htons(us);}
                virtual unsigned short ntohs (unsigned short us) {return ::ntohs(us);}
            #endif
        protected:
            int iSocket;
            bool bCleanupAllowed;
            String localHostName;
            struct sockaddr_in lastConnectSockAddr;
            struct sockaddr_in remoteSockAddr;
            String remoteHostAddr;
            String remoteHostName;
            #if defined (OS2)
                static int iSockInitCalled;
            #elif defined (WIN16) || defined (WIN32)
                static TCPSocketIdleCallbackFuncType pIdleFn;
                static void *pUserData;
            #endif
    };

    extern "C" void sigHandler (int signal);

}

#endif   // #ifdef INCL_TCPSOCKET_H
