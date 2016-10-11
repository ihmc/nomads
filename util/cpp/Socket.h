/*
 * Socket.h
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
 * RCSInfo: $Header: /export/cvs/nomads.root/util/cpp/Socket.h,v 1.26 2016/08/08 15:50:41 rfronteddu Exp $
 * Revision: $Revision: 1.26 $
 */

#ifndef INCL_SOCKET_H
#define INCL_SOCKET_H

#if !defined (WIN16) && !defined (WIN32) && !defined (OS2) && !defined (DOS) && !defined (__NLM__) && !defined (UNIX)
    #define UNIX
#endif

#define LIN_TER_CR       "\r"
#define LIN_TER_LF       "\n"
#define LIN_TER_CRLF     "\r\n"
#define LIN_TER_DEFAULT  LIN_TER_CRLF

#define SE_DISCONNECT -200
#define SE_TIMEOUT    -201
#define SE_WOULDBLOCK -202

#if defined (WIN16)
    #pragma hdrstop
#include <winsock2.h>
    #include <windows.h>
#elif defined (WIN32)
	#include <winsock2.h>
    #include <windows.h>
#elif defined (OS2)

#elif defined (UNIX)
    #include <signal.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <ctype.h>
#endif

#include <stdio.h>
#include <string.h>

namespace NOMADSUtil
{

    class Socket
    {
        public:
            Socket (void);
            virtual ~Socket (void);
            virtual int blockingMode (int iMode) = 0;
            virtual int bufferingMode (int iMode) = 0;
            virtual const char * getLocalHostName (void) = 0;
            virtual int reconnect (void) = 0;
            virtual int disconnect (void) = 0;
            virtual int disableReceive (void) = 0;
            virtual Socket * accept (void) = 0;
            virtual const char * getRemoteHostAddr (void) = 0;
            virtual const char * getRemoteHostName (void) = 0;
            virtual unsigned short getLocalPort (void) = 0;
            virtual unsigned short getRemotePort (void) = 0;
            virtual Socket * dup (void) = 0;

            // Sends the number of bytes specified by iSize from the buffer pointed to by pBuf
            // Returns the number of bytes successfully sent, or a negative value in case of error
            // In case or error, return value could be SE_TIMEOUT, SE_WOULDBLOCK, or some other negative value
            virtual int send (const void *pBuf, int iSize);

            virtual int receive (void *pBuf, int iSize);
            virtual int receiveBuffered (void *pBuf, int iSize);        // must be used if undoReceive() is called
            virtual int undoReceive (const void *pBuf, int iSize);      // must be used in conjunction with receiveBuffered()
            virtual int sendBytes (const void *pBuf, int iNumBytes);    // Will loop until all bytes are sent
            virtual int receiveBytes (void *pBuf, int iNumBytes);       // Will loop until all bytes are received
            virtual int sendBlock (const void *pBuf, int iSize);
            virtual int receiveBlock (void *pBuf, int iMaxSize);
            virtual void * receiveBlock (void);
            virtual int sendString (const char *pszString);
            virtual int receiveString (char *pszString, int iMaxSize);
            virtual char * receiveString (void);
            virtual int sendLine (const char *pszLine);
            virtual int receiveLine (char *pszLine, int iMaxSize);
            virtual void setLineTerminator (const char *pszLineTerminator);
            virtual void setTimeOut (unsigned long ulTimeOut);
            virtual int error (void) = 0;
            virtual int bytesAvail (void) = 0;
            virtual int getFileDescriptor (void) = 0;
            virtual int getAndDisassociateFileDescriptor (void) = 0;
            static bool checkIfStringIsIPAddr (const char* szIPAddr);
            #if defined (WIN16) || defined (WIN32)
                virtual int setupReceiveNotification (HWND hWnd, unsigned int wMsg) = 0;
                virtual int disableReceiveNotification (void) = 0;
            #endif
        protected:
            static bool isNumber (const char* szNum);  
            virtual int sendImpl (const void *pBuf, int iSize) = 0;
            virtual int receiveImpl (void *pBuf, int iSize) = 0;
            #if defined (OS2)
                virtual unsigned short htons(unsigned short us) = 0;
                virtual unsigned short ntohs(unsigned short us) = 0;
            #endif
            #if defined (WIN16) || defined (WIN32)
                virtual int suspendReceiveNotification (void) = 0;
                virtual int resumeReceiveNotification (void) = 0;
            #endif
        protected:
            bool debug;
            int iBlockingMode;
            int iConnected;
            int iSetupToReceive;
            char *pszLineTerminator;
            void *pRecvBuf;
            int iRecvBufSize;
            unsigned long ulTimeOutLimit;
            #if defined (WIN16) || defined (WIN32)
                HWND hWnd;
                unsigned int wMsg;
                int iNotifyOnReceive;
            #endif
    };

    inline int Socket::sendString (const char *pszString)
    {
        return sendBlock (pszString, (int) (strlen (pszString)+1));
    }

    inline int Socket::receiveString (char *pszString, int iMaxSize)
    {
        return receiveBlock (pszString, iMaxSize);
    }

    inline char * Socket::receiveString (void)
    {
        return (char*) receiveBlock ();
    }

}

#endif   // #ifdef INCL_SOCKET_H
