/*
 * Socket.cpp
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
 * RCSInfo: $Header: /export/cvs/nomads.root/util/cpp/Socket.cpp,v 1.29 2016/06/09 20:02:45 gbenincasa Exp $
 * Revision: $Revision: 1.29 $
 */

#include "Socket.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

using namespace NOMADSUtil;

Socket::Socket (void)
{
    debug = false;
    iBlockingMode = 1;
    iConnected = 0;
    iSetupToReceive = 0;
    pszLineTerminator = (char*) malloc (strlen (LIN_TER_DEFAULT) + 1);
    strcpy (pszLineTerminator, LIN_TER_DEFAULT);
    pRecvBuf = NULL;
    iRecvBufSize = 0;
    ulTimeOutLimit = 0;
    #if defined(WIN16) || defined(WIN32)
        iNotifyOnReceive = 0;
    #endif
}

Socket::~Socket (void)
{
    if (pszLineTerminator) {
        free (pszLineTerminator);
        pszLineTerminator = NULL;
    }
    if (pRecvBuf) {
        free (pRecvBuf);
        pRecvBuf = NULL;
        iRecvBufSize = 0;
    }
    if (ulTimeOutLimit) {
        setTimeOut (0);
    }
}

int Socket::receiveBuffered (void *pBuf, int iSize)
{
    if (!iConnected) {
        return -1;
    }
    if (pRecvBuf) {
        if (iSize < iRecvBufSize) {
            memcpy (pBuf, pRecvBuf, iSize);
            char *pOldBuf = (char*) pRecvBuf;
            pRecvBuf = malloc (iRecvBufSize-iSize+1);
            memcpy (pRecvBuf, pOldBuf+iSize, iRecvBufSize-iSize);
            free (pOldBuf);
            iRecvBufSize -= iSize;
            ((char*)pRecvBuf) [iRecvBufSize] = '\0';
            return iSize;
        }
        else {
            iSize = iRecvBufSize;
            memcpy (pBuf, pRecvBuf, iRecvBufSize);
            free (pRecvBuf);
            pRecvBuf = NULL;
            iRecvBufSize = 0;
            return iSize;
        }
    }
    else {
        return receive (pBuf, iSize);
    }
}

int Socket::undoReceive (const void *pBuf, int iSize)
{
    if (pRecvBuf) {
        pRecvBuf = realloc (pRecvBuf, iRecvBufSize+iSize+1);         // +1 for \0 at the end
        memcpy (((char*)pRecvBuf)+iRecvBufSize, pBuf, iSize);
        iRecvBufSize += iSize;
        ((char*)pRecvBuf)[iRecvBufSize] = '\0';
    }
    else {
        pRecvBuf = malloc (iSize+1);
        memcpy (pRecvBuf, pBuf, iSize);
        iRecvBufSize = iSize;
        ((char*)pRecvBuf)[iRecvBufSize] = '\0';
    }
    return 0;
}

int Socket::send (const void *pBuf, int iNumBytes)
{
    return sendImpl (pBuf, iNumBytes);
}

int Socket::receive (void *pBuf, int iNumBytes)
{
    if (pRecvBuf) {
        // There is some data in the buffer - just return that
        return receiveBuffered (pBuf, iNumBytes);
    }
    else {
        return receiveImpl (pBuf, iNumBytes);
    }
}

int Socket::sendBytes (const void *pBuf, int iNumBytes)
{
    int i, iBytesSent = 0;
    if (iNumBytes == 0) {
        // Nothing to be done
        return 0;
    }
    do {
        if ((i = send (&((char*)pBuf)[iBytesSent], iNumBytes-iBytesSent)) <= 0) {
            if ((i == SE_DISCONNECT) || (i == SE_TIMEOUT)) {
                return i;
            }
            else if (i != SE_WOULDBLOCK) {
                return -2;
            }
        }
        else {
            iBytesSent += i;
        }
    } while (iBytesSent < iNumBytes);
    return 0;
}

int Socket::receiveBytes (void *pBuf, int iNumBytes)
{
    int i,iBytesRcvd = 0;
    do {
        if ((i = receive (&((char*)pBuf)[iBytesRcvd], iNumBytes-iBytesRcvd)) <= 0) {
            if ((i == SE_DISCONNECT) || (i == SE_TIMEOUT)) {
                return i;
            }
            else if (i != SE_WOULDBLOCK) {
                return -1;
            }
        }
        else {
            iBytesRcvd += i;
        }
    } while (iBytesRcvd < iNumBytes);
    return iBytesRcvd;
}

int Socket::sendBlock (const void *pBuf, int iSize)
{
    if (debug) {
        fprintf (stdout, "Socket::sendBlock...sending size: %d\n", iSize);
    }

    int rc;
    unsigned long ulSize = htonl (iSize);
    char *pszBuf = (char*) &ulSize;

    if ((rc = sendBytes (pszBuf, sizeof (ulSize))) < 0) {
        if ((rc == SE_DISCONNECT) || (rc == SE_TIMEOUT)) {
            return rc;
        }
        else {
            return -1;
        }
    }

    if (iSize > 0) {
        if ((rc = sendBytes (pBuf, iSize)) < 0) {
            if ((rc == SE_DISCONNECT) || (rc == SE_TIMEOUT)) {
                return rc;
            }
            else {
                return -2;
            }
        }
    }
    return 0;
}

int Socket::receiveBlock (void *pBuf, int iMaxSize)
{
    unsigned long ulSize;
    char *pszBuf = (char*) &ulSize;
    int rc;
    
    if ((rc = receiveBytes (pszBuf, sizeof (ulSize))) <= 0) {
        if ((rc == SE_DISCONNECT) || (rc == SE_TIMEOUT)) {
            return rc;
        }
        else {
            return -1;
        }
    }
    ulSize = ntohl (ulSize);
    if (debug) {
        fprintf (stdout, "Socket::receiveBlock...received size: %d\n", (int) ulSize);
    }
    if (ulSize > (unsigned long)iMaxSize) {   // choy - jan 4 2007
        return (int) ulSize;
    }
    if (ulSize == 0) {
        return 0;
    }
    if ((rc = receiveBytes (pBuf, (int) ulSize)) <= 0) {
        if ((rc == SE_DISCONNECT) || (rc == SE_TIMEOUT)) {
            return rc;
        }
        else {
            return -2;
        }
    }
    return rc;
}

void * Socket::receiveBlock (void)
{
    unsigned long ulSize;
    char *pszBuf = (char*) &ulSize;
    int rc;
    
    if ((rc = receiveBytes (pszBuf, sizeof (ulSize))) <= 0) {
        return NULL;
    }
    ulSize = ntohl (ulSize);
    if (NULL == (pszBuf = (char*) malloc (ulSize))) {
        return NULL;
    }
    if (ulSize == 0) {
        return NULL;
    }
    if ((rc = receiveBytes (pszBuf, (int) ulSize)) <= 0) {
        free (pszBuf);
        return NULL;
    }
    return pszBuf;
}

int Socket::sendLine (const char *pszLine)
{
    int rc;
    if ((rc = sendBytes (pszLine, (int) strlen (pszLine))) < 0) {
        if ((rc == SE_DISCONNECT) || (rc == SE_TIMEOUT)) {
            return rc;
        }
        else {
            return -1;
        }
    }
    if ((pszLineTerminator) && (strlen (pszLineTerminator))) {
        if ((rc = sendBytes (pszLineTerminator, (int) strlen (pszLineTerminator))) < 0) {
            if ((rc == SE_DISCONNECT) || (rc == SE_TIMEOUT)) {
                return rc;
            }
            else {
                return -2;
            }
        }
    }
    if (debug) {
        fprintf (stdout, "Socket::sendLine...sent: %s, %d bytes\n", pszLine, rc);
    }
    return 0;
}

int Socket::receiveLine (char *pszLine, int iMaxSize)
{
    // Check whether a terminator has been set
    if ((pszLineTerminator) && (pszLineTerminator[0])) {
        // Check whether a line with the terminator is already in the buffer
        char *pszTemp;
        if ((pRecvBuf) && (NULL != (pszTemp = strstr (((char*)pRecvBuf), pszLineTerminator)))) {
            int iLen = (int)(pszTemp-((char*)pRecvBuf)) + (int) strlen (pszLineTerminator);  // choy - jan 4 2007
            if (iLen < iMaxSize) {
                // Required line is already in the buffer - no need to
                // check result of receiveBuffered
                receiveBuffered (pszLine, iLen);
                pszLine [iLen] = '\0';
                if (debug) printf ("Socket::receiveLine...recv'd: %s\n", pszLine);
                return iLen;
            }
            else {
                // Again, data being received is already in the buffer
                // no need to check result of receiveBuffered
                receiveBuffered (pszLine, iMaxSize-1);
                pszLine [iMaxSize] = '\0';
                // Now discard remaining line including terminator
                // int junkBytes = strlen (pszLineTerminator) + iLen - iMaxSize;
                // char *pTempBuf = (char *) malloc (junkBytes);
                // receiveBuffered (pTempBuf, junkBytes);
                // free (pTempBuf);
                return iMaxSize;
            }
        }
        else {
            // Line Terminator is not in the buffer - read more bytes
            char *pNewBuf = (char *) malloc (81);
            pNewBuf [80] = '\0';
            do {
                int iResult;
                if ((iResult = receiveImpl (pNewBuf, 80)) > 0) {
                    // Now put the newly read data also into the buffer
                    undoReceive (pNewBuf, iResult);
                }
                else {
                    // Receive failed before getting a line terminator
                    // Abort function
                    free (pNewBuf);
                    return iResult;
                }
            } while (!(strstr ((char*)pRecvBuf, pszLineTerminator)));
            // Line Terminator is now in the buffer - recurse
            free (pNewBuf);
            return receiveLine (pszLine, iMaxSize);
        }
    }
    else {
        // No line terminator has been specified - simply read as many
        // characters as will fit
        int iResult;
        if ((iResult = receiveBuffered (pszLine, iMaxSize-1)) > 0) {
            pszLine [iResult] = '\0';
        }
        return iResult;
    }
}

void Socket::setTimeOut (unsigned long ulTimeOut)
{
    ulTimeOutLimit = ulTimeOut;
}

void Socket::setLineTerminator (const char *pszLineTerminator)
{
    if (this->pszLineTerminator) {
        free (this->pszLineTerminator);
        this->pszLineTerminator = NULL;
    }
    if ((pszLineTerminator) && (strlen (pszLineTerminator))) {
        this->pszLineTerminator = (char*) malloc (strlen (pszLineTerminator) + 1);
        strcpy (this->pszLineTerminator, pszLineTerminator);
    }
}

bool Socket::isNumber (const char *pszNum)
{
    int i = 0;
    if (pszNum == NULL) {
        return false;
    }
    while (*(pszNum + i) != '\0') {
        if (!isdigit (*(pszNum + i++))) {
            return false;
        }
    }
    return true;
}

bool Socket::checkIfStringIsIPAddr (const char *szIPAddr)
{
    char *pszIPAddrCopy = new char[strlen (szIPAddr) + 1];
    strcpy (pszIPAddrCopy, szIPAddr);
    int octet = 0;
    char *tk = strtok (pszIPAddrCopy, ".");
    int i;
    for (i = 4; i > 0 && tk != NULL && isNumber (tk);i--) {
        octet = atoi (tk);
        if ((octet < 0) || (octet > 255)) {
            delete[] pszIPAddrCopy;
            return false;
        }
        tk = strtok (NULL, ".");
    }
    if (i == 0 && tk == NULL) { //found only four octets, all within desired range
        delete[] pszIPAddrCopy;
        return true;
    }
    delete[] pszIPAddrCopy;
    return false;
}
