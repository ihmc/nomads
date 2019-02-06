/*
 * CommHelper.cpp
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

#include "CommHelper.h"

#include "NLFLib.h"
#include "Socket.h"

#include <stdio.h>

#if defined (WIN32)
    #if _MCS_VER<1900
        #define snprintf _snprintf
    #endif
#endif

using namespace NOMADSUtil;


CommHelper::CommHelper (void)
{
    _pSocket = NULL;
    _szErrorMsgBuf[0] = '\0';
    _pszLineBuf = NULL;
    _usLineBufSize = 0;
}

CommHelper::~CommHelper (void)
{
    delete[] _pszLineBuf;
    _pszLineBuf = NULL;
    _usLineBufSize = 0;
}

int CommHelper::init (Socket *pSocket, unsigned short usMaxLineSize)
{
    if (pSocket == NULL) {
        return -1;
    }
    if (usMaxLineSize > 0) {
        _pszLineBuf = new char[usMaxLineSize];
        _usLineBufSize = usMaxLineSize;
    }
    else {
        delete[] _pszLineBuf;
        _pszLineBuf = NULL;
        _usLineBufSize = 0;
    }
    _pSocket = pSocket;
    _pSocket->setLineTerminator ("\r\n");
    return 0;
}

unsigned long CommHelper::send (const void *pBuf, unsigned long ulSize) throw (CommException)
{
    int rc;
    if ((rc = _pSocket->send (pBuf, (int)ulSize)) <= 0) {
        sprintf (_szErrorMsgBuf, "CommHelper::send socket error %d:%d", rc, _pSocket->error());
        throw CommException (_szErrorMsgBuf);
    }
    return rc;
}

unsigned long CommHelper::receive (void *pBuf, unsigned long ulSize) throw (CommException)
{
    int rc;
    if ((rc = _pSocket->receive (pBuf, (int)ulSize)) <= 0) {
        sprintf (_szErrorMsgBuf, "CommHelper::receive socket error %d:%d", rc, _pSocket->error());
        throw CommException (_szErrorMsgBuf);
    }
    return rc;
}

void CommHelper::sendBlob (const void *pBuf, unsigned long ulSize) throw (CommException)
{
    int rc;
    if (0 != (rc = _pSocket->sendBytes (pBuf, (int)ulSize))) {
        sprintf (_szErrorMsgBuf, "CommHelper::sendBlob socket error %d:%d", rc, _pSocket->error());
        throw CommException (_szErrorMsgBuf);
    }
}

void CommHelper::receiveBlob (void *pBuf, unsigned long ulSize) throw (CommException)
{
    int rc;
    int iSize = (int) ulSize;
    if (iSize != (rc = _pSocket->receiveBytes (pBuf, iSize))) {
        sprintf (_szErrorMsgBuf, "CommHelper::receiveBlob socket error %d:%d", rc, _pSocket->error());
        throw CommException (_szErrorMsgBuf);
    }
}

void CommHelper::sendBlock (const void *pBuf, unsigned long ulBufSize) throw (CommException)
{
    unsigned long ulBlockSize = htonl (ulBufSize);
    sendBlob (&ulBlockSize, sizeof (ulBlockSize));
    sendBlob (pBuf, ulBufSize);
}

unsigned long CommHelper::receiveBlock (void *pBuf, unsigned long ulBufSize) throw (CommException, ProtocolException)
{
    unsigned long ulBlockSize;
    receiveBlob (&ulBlockSize, sizeof (ulBlockSize));
    ulBlockSize = ntohl (ulBlockSize);
    if (ulBlockSize > ulBufSize) {
        sprintf (_szErrorMsgBuf, "CommHelper::receiveBlock: receiver block size of %lu is not large enough to hold block of size %lu\n",
                 ulBufSize, ulBlockSize);
        throw ProtocolException (_szErrorMsgBuf);
    }
    receiveBlob (pBuf, ulBlockSize);
    return ulBlockSize;
}

void CommHelper::sendLine (const char *pszMsg, ...) throw (CommException)
{
    int rc;
    va_list args;
    va_start (args, pszMsg);
    vsprintf (_pszLineBuf, pszMsg, args);
    va_end (args);
    // Not calling sendLine here because of a delay in the send operation when
    //     Socket::sendLine() calls send() twice, once with the string and once
    //     with the line terminator
    strcat (_pszLineBuf, "\r\n");
    if (0 != (rc = (int) _pSocket->sendBytes (_pszLineBuf, (int) strlen (_pszLineBuf)))) {
        sprintf (_szErrorMsgBuf, "CommHelper::sendLine socket error %d:%d", rc, _pSocket->error());
        throw CommException (_szErrorMsgBuf);
    }
}

int CommHelper::receiveLine (char *pszBuf, int iBufSize) throw (CommException)
{
    int rc;
    if ((rc = _pSocket->receiveLine (pszBuf, iBufSize)) <= 0) {
        sprintf (_szErrorMsgBuf, "CommHelper::receiveLine socket error %d:%d", rc, _pSocket->error());
        throw CommException (_szErrorMsgBuf);
    }
    stripCRLF (pszBuf);
    return rc;
}

int CommHelper::receiveRemainingLine (char *pszBuf, int iBufSize, const char *pszMsg, ...) throw (CommException, ProtocolException)
{
    va_list args;
    va_start (args, pszMsg);
    return receiveRemainingLine (pszBuf, iBufSize, pszMsg, args);
}

void CommHelper::receiveMatch (const char *pszMsg, ...) throw (CommException, ProtocolException)
{
    char *pszMatchStr = new char [_usLineBufSize];

    // Get the initial string
    va_list args;
    va_start (args, pszMsg);
    vsprintf (pszMatchStr, pszMsg, args);
    va_end (args);

    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize);
    if (0 != strcmp (pszMatchStr, _pszLineBuf)) {
        snprintf (_szErrorMsgBuf, sizeof (_szErrorMsgBuf), "CommHelper::receive Match mismatch in expected (%s) and received (%s) strings", pszMatchStr, _pszLineBuf);
        _szErrorMsgBuf[sizeof(_szErrorMsgBuf)-1] = '\0';
        delete[] pszMatchStr;
        throw ProtocolException (_szErrorMsgBuf);
    }
}

int CommHelper::receiveMatchIndex (unsigned short usAltCount, ...) throw (CommException, ProtocolException)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize);

    // Compare received line with alternatives
    va_list args;
    va_start (args, usAltCount);
    for (unsigned short us = 0; us < usAltCount; us++) {
        const char *pszAlternative = va_arg (args, const char *);
        // A zero-length alternative string matches any input
        if (pszAlternative[0] == '\0') {
            va_end (args);
            return us;
        }
        else if (0 == strcmp (pszAlternative, _pszLineBuf)) {
            va_end (args);
            return us;
        }
    }
    va_end (args);

    // No match was found among the alternatives
    throw ProtocolException ("CommHelper::receiveMatchIndex received line does not match any alternatives");
}

int CommHelper::receiveMatchIndex (const char *apszAlternatives[]) throw (CommException, ProtocolException)
{
    unsigned short usCount = 0;
    while (apszAlternatives[usCount] != NULL) {
        usCount++;
    }
    return receiveMatchIndex (apszAlternatives, usCount);
}

int CommHelper::receiveMatchIndex (const char *apszAlternatives[], unsigned short usCount) throw (CommException, ProtocolException)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize);

    // Compare received line with alternatives
    for (unsigned short us = 0; us < usCount; us++) {
        // A zero-length alternative string matches any input
        if (apszAlternatives[us][0] == '\0') {
            return us;
        }
        else if (0 == strcmp (apszAlternatives[us], _pszLineBuf)) {
            return us;
        }
    }

    throw ProtocolException ("CommHelper::receiveMatchIndex received line does not match any alternatives");
}


int CommHelper::receiveRemainingMatchIndex (unsigned short usAltCount, ...) throw (CommException, ProtocolException)
{
    // Get a pointer to pszMsg first;
    const char *pszMsg;
    va_list altArgs, msgArgs;
    va_start (altArgs, usAltCount);
    unsigned short us;
    for (us = 0; us < usAltCount; us++) {
        va_arg (altArgs, const char*);
    }
    pszMsg = va_arg (altArgs, const char*);
    va_end (altArgs);
    va_start (msgArgs, pszMsg);

    // Get the remaining line next
    char *pszRemainingLine = new char [_usLineBufSize];

    try {
        receiveRemainingLine (pszRemainingLine, _usLineBufSize, pszMsg, msgArgs);
    }
    catch (Exception) {
        delete[] pszRemainingLine;
        throw;
    }

    // Compare received line with alternatives
    va_start (altArgs, usAltCount);
    for (us = 0; us < usAltCount; us++) {
        const char *pszAlternative = va_arg (altArgs, const char*);
        // A zero-length alternative string matches any input
        if (pszAlternative[0] == '\0') {
            va_end (altArgs);
            return us;
        }
        else if (0 == strcmp (pszAlternative, _pszLineBuf)) {
            va_end (altArgs);
            return us;
        }
    }
    va_end (altArgs);
    // No match was found among the alternatives
    throw ProtocolException ("CommHelper::receiveRemainingMatchIndex received line does not match any alternatives");
}


int CommHelper::receiveRemainingMatchIndex (const char *apszAlternatives[], const char *pszMsg, ...) throw (CommException, ProtocolException)
{
    char *pszRemainingLine = new char [_usLineBufSize];

    // Get the remaining line first
    va_list args;
    va_start (args, pszMsg);

    try {
        receiveRemainingLine (pszRemainingLine, _usLineBufSize, pszMsg, args);
    }
    catch (Exception) {
        delete[] pszRemainingLine;
        throw;
    }

    // Compare received line with alternatives
    for (unsigned short us = 0; apszAlternatives[us] != NULL; us++) {
        // A zero-length alternative string matches any input
        if (apszAlternatives[us][0] == '\0') {
            delete[] pszRemainingLine;
            return us;
        }
        else if (0 == strcmp (apszAlternatives[us], pszRemainingLine)) {
            delete[] pszRemainingLine;
            return us;
        }
    }
    delete[] pszRemainingLine;

    // No match was found among the alternatives
    throw ProtocolException ("CommHelper::receiveRemainingMatchIndex received line does not match any alternatives");
}

int CommHelper::receiveRemainingMatchIndex (const char *apszAlternatives[], unsigned short usCount, const char *pszMsg, ...) throw (CommException, ProtocolException)
{
    char *pszRemainingLine = new char [_usLineBufSize];

    // Get the remaining line first
    va_list args;
    va_start (args, pszMsg);
    try {
         receiveRemainingLine (pszRemainingLine, _usLineBufSize, pszMsg, args);
    }
    catch (Exception) {
         delete[] pszRemainingLine;
         throw;
     }

    // Compare received line with alternatives
    for (unsigned short us = 0; us < usCount; us++) {
        // A zero-length alternative string matches any input
        if (apszAlternatives[us][0] == '\0') {
            delete[] pszRemainingLine;
            return us;
        }
        else if (0 == strcmp (apszAlternatives[us], pszRemainingLine)) {
            delete[] pszRemainingLine;
            return us;
        }
    }
    delete[] pszRemainingLine;

    throw ProtocolException ("CommHelper::receiveRemainingMatchIndex received line does not match any alternatives");
}

const char ** CommHelper::receiveParsed (int* pCount) throw (CommException)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize);

    // Start extracting the tokens
    char *pszTemp;
    char *pszToken;
    int i = 0;
    pszToken = strtok_mt (_pszLineBuf, " \t", &pszTemp);
    while (pszToken) {
        _parsedTokens[i++] = pszToken;
        pszToken = strtok_mt (NULL, " \t", &pszTemp);
    }
    _parsedTokens[i] = NULL;
    if (NULL != pCount) {
        *pCount = i;
    }
    return (const char **) &_parsedTokens[0];
}

const char ** CommHelper::receiveParsedDelimited (const char* pszDelimiters, int* pCount) throw (CommException)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize);

    // Start extracting the tokens
    char *pszTemp;
    char *pszToken;
    int i = 0;
    pszToken = strtok_mt (_pszLineBuf, pszDelimiters, &pszTemp);
    while (pszToken) {
        _parsedTokens[i++] = pszToken;
        pszToken = strtok_mt (NULL, pszDelimiters, &pszTemp);
    }
    _parsedTokens[i] = NULL;
    if (NULL != pCount) {
        *pCount = i;
    }
    return (const char **) &_parsedTokens[0];
}


const char ** CommHelper::receiveParsedSpecific (const char *pszParseFmt) throw (CommException, ProtocolException)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize);

    // Copy the ParseFmt string to tokenize it
    char *pszParseFmtCopy = new char [strlen(pszParseFmt)+1];
    strcpy (pszParseFmtCopy, pszParseFmt);

    // Start extracting the tokens
    char *pszTokenStart = _pszLineBuf;
    char *pszTokenEnd = NULL;

    int iCount = 0;
    char *pszTemp;
    char *pszFmt = strtok_mt (pszParseFmtCopy, " ", &pszTemp);
    while (pszFmt) {
        bool bSkip = false;
        int iTokenCount = atoi (pszFmt);
        if (iTokenCount < 0) {
            bSkip = true;
            iTokenCount = -iTokenCount;
        }
        while ((*pszTokenStart == ' ') || (*pszTokenStart == '\t')) {
            pszTokenStart++;
        }
        if (*pszTokenStart == '\0') {
            delete pszParseFmtCopy;

            throw ProtocolException ("CommHelper::receiveParsedSpecific premature end of line");
        }
        pszTokenEnd = pszTokenStart;
        if (iTokenCount == 0) {
            // Need the rest of the line (excluding any trailing whitespace)
            int iEnd = (int) strlen (pszTokenEnd) - 1;
            while (iEnd >= 0) {
                if ((pszTokenEnd[iEnd] == ' ') || (pszTokenEnd[iEnd] == '\t')) {
                    pszTokenEnd[iEnd] = '\0';
                }
                else {
                    break;
                }
                iEnd--;
            }
            _parsedTokens[iCount++] = pszTokenStart;
            break;
        }
        else {
            while (iTokenCount > 1) {
                while ((*pszTokenEnd != ' ') && (*pszTokenEnd != '\t')) {
                    if (*pszTokenEnd == '\0') {
                        delete pszParseFmtCopy;
                        throw ProtocolException ("CommHelper::receiveParsedSpecific premature end of line");
                    }
                    pszTokenEnd++;
                }
                while ((*pszTokenEnd == ' ') || (*pszTokenEnd == '\t')) {
                    pszTokenEnd++;
                }
                iTokenCount--;
            }
            while ((*pszTokenEnd != ' ') && (*pszTokenEnd != '\t')) {
                if (*pszTokenEnd == '\0') {
                    break;
                }
                pszTokenEnd++;
            }
        }
        if (!bSkip) {
            _parsedTokens[iCount++] = pszTokenStart;
        }
        if (*pszTokenEnd == '\0') {
            pszTokenStart = pszTokenEnd;
        }
        else {
            *pszTokenEnd = '\0';
            pszTokenStart = pszTokenEnd+1;
        }
        pszFmt = strtok_mt (NULL, " ", &pszTemp);
    }
    _parsedTokens[iCount] = NULL;
    delete pszParseFmtCopy;
    return (const char **) &_parsedTokens[0];
}

int CommHelper::receiveRemainingLine (char *pszBuf, int iBufSize, const char *pszMsg, va_list args)
{
    char *pszInitStr = new char [_usLineBufSize];

    // Get the initial string
    vsprintf (pszInitStr, pszMsg, args);
    va_end (args);

    // Get the length of the remaining portion
    int iInitStrLen = (int) strlen (pszInitStr);
    int iReceivedLineLen = 0;
    int iMaxReceiveLineSize = iBufSize + iInitStrLen;
    if (_usLineBufSize < iMaxReceiveLineSize) {
        iMaxReceiveLineSize = _usLineBufSize;
    }

    // Receive a line into the internal buffer
    iReceivedLineLen = receiveLine (_pszLineBuf, iMaxReceiveLineSize);

    // Make sure that we have the initial string in the received string
    if (iReceivedLineLen < iInitStrLen) {
        delete[] pszInitStr;
        throw ProtocolException("CommHelper::receiveRemainingLine received line shorter than initial string");
    }
    if (0 != strncmp (pszInitStr, _pszLineBuf, iInitStrLen)) {
        delete[] pszInitStr;
        throw ProtocolException("CommHelper::receiveRemainingLine mismatch between receievd line and initial string");
    }
    delete[] pszInitStr;

    // Now copy the remaining data into pszBuf and return
    strcpy (pszBuf, &_pszLineBuf[iInitStrLen]);
    return iReceivedLineLen - iInitStrLen;
}

void CommHelper::stripCRLF (char *pszBuf)
{
    int i = (int) strlen (pszBuf) - 1;
    while (i >= 0) {
        if (pszBuf[i] == '\r') {
            pszBuf[i] = '\0';
        }
        else if (pszBuf[i] == '\n') {
            pszBuf[i] = '\0';
        }
        else {
            break;
        }
        i--;
    }
}

void CommHelper::closeConnection()
{
    if (_pSocket->disconnect() < 0) {
        throw ProtocolException("CommHelper::Unable to close the connection");
    }
}
