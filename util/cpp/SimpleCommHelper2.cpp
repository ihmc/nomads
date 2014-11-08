/*
 * SimpleCommHelper2.cpp
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

#include "SimpleCommHelper2.h"

#include "NLFLib.h"
#include "Socket.h"
#include "SocketReader.h"
#include "SocketWriter.h"

#include <stdio.h>

#if defined (WIN32)
    #define snprintf _snprintf
#endif

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

SimpleCommHelper2::SimpleCommHelper2 (Logger::Level loggingLevel)
    : _loggingLevel (loggingLevel)
{
    _pSocket = NULL;
    _pLOReader = NULL;
    _pWriter = NULL;
    _bDeleteSocket = false;
    _bDeleteWriter = false;
    _szErrorMsgBuf[0] = '\0';
    _pszLineBuf = NULL;
    _usLineBufSize = 0;
}

SimpleCommHelper2::~SimpleCommHelper2 (void)
{
    if (_bDeleteSocket) {
        delete _pSocket;
    }
    _pSocket = NULL;
    delete _pLOReader;       // _pLOReader is instantiated by this class, so it must always be deleted
    _pLOReader = NULL;       // The deletion of the pReader contained by pLOReader can still be controlled - see setDeleteUnderlyingReader()
    if (_bDeleteWriter) {
        delete _pWriter;
    }
    _pWriter = NULL;
    delete[] _pszLineBuf;
    _pszLineBuf = NULL;
    _usLineBufSize = 0;
}

int SimpleCommHelper2::init (Socket *pSocket, unsigned short usMaxLineSize)
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
    _pLOReader = new LineOrientedReader (new SocketReader (pSocket, false), true);
    _pWriter = new SocketWriter (pSocket, false);
    _bDeleteWriter = true;
    return 0;
}

int SimpleCommHelper2::init (Reader *pReader, Writer *pWriter, unsigned short usMaxLineSize)
{
    if ((pReader == NULL) && (pWriter == NULL)) {
        return -1;
    }
    if (pReader) {
        _pLOReader = new LineOrientedReader (pReader, false);
    }
    _pWriter = pWriter;
    _bDeleteWriter = false;
    if (usMaxLineSize > 0) {
        _pszLineBuf = new char[usMaxLineSize];
        _usLineBufSize = usMaxLineSize;
    }
    else {
        delete[] _pszLineBuf;
        _pszLineBuf = NULL;
        _usLineBufSize = 0;
    }
    return 0;
}

uint32 SimpleCommHelper2::send (const void *pBuf, uint32 ui32Size, Error &error)
{
    int rc;
    if (_pWriter == NULL) {
        error = CommError;
        return -1;
    }
    if ((rc = _pWriter->writeBytes (pBuf, ui32Size)) < 0) {
        error = CommError;
        return -2;
    }
    error = None;
    return ui32Size;
}

uint32 SimpleCommHelper2::receive (void *pBuf, uint32 ui32Size, Error &error)
{
    int rc;
    if (_pLOReader == NULL) {
        error = CommError;
        return -1;
    }
    if ((rc = _pLOReader->read (pBuf, (int)ui32Size)) <= 0) {
        error = CommError;
        return -2;
    }
    error = None;
    return rc;
}

void SimpleCommHelper2::sendBlob (const void *pBuf, uint32 ui32Size, Error &error)
{
    // With a writer, there is not difference between send and sendBlob because the
    // writer will treat a partial write as a failure
    send (pBuf, ui32Size, error);
}

void SimpleCommHelper2::receiveBlob (void *pBuf, uint32 ui32Size, Error &error)
{
    int rc;
    if (_pLOReader == NULL) {
        error = CommError;
        return;
    }
    if (0 != (rc = _pLOReader->readBytes (pBuf, ui32Size))) {
        error = CommError;
        return;
    }
    error = None;
}

void SimpleCommHelper2::sendBlock (const void *pBuf, uint32 ui32BufSize, Error &error)
{
    if (pBuf == NULL) {
        ui32BufSize = 0;
    }
    uint32 ui32BlockSize = htonl (ui32BufSize);
    sendBlob (&ui32BlockSize, sizeof (ui32BlockSize), error);
    if ((error == None) && (ui32BufSize > 0)) {
        sendBlob (pBuf, ui32BufSize, error);
    }
}

void SimpleCommHelper2::sendBlock (const void *pBuf1, uint32 ui32Buf1Size, const void *pBuf2, uint32 ui32Buf2Size, Error &error)
{
    uint32 ui32BlockSize = htonl (ui32Buf1Size + ui32Buf2Size);
    sendBlob (&ui32BlockSize, sizeof (ui32BlockSize), error);
    if (error != None) {
        return;
    }
    sendBlob (pBuf1, ui32Buf1Size, error);
    if (error != None) {
        return;
    }
    sendBlob (pBuf2, ui32Buf2Size, error);
}

void SimpleCommHelper2::sendStringBlock (const char *pszBuf, Error &error)
{
    uint32 ui32Len = (pszBuf == NULL ? 0 : (uint32)strlen (pszBuf));
    if (ui32Len == 0) {
        if (0 != _pWriter->write32 (&ui32Len)) {
            error = CommError;
            return;
        }
        error = None;
    }
    else {
        sendBlock (pszBuf, ui32Len, error);
    }
}

uint32 SimpleCommHelper2::receiveBlock (void *pBuf, uint32 ui32BufSize, Error &error)
{
    uint32 ui32BlockSize;
    receiveBlob (&ui32BlockSize, sizeof (ui32BlockSize), error);
    if (error != None) {
        return -1;
    }
    ui32BlockSize = ntohl (ui32BlockSize);
    if (ui32BlockSize > ui32BufSize) {
        error = ProtocolError;
        return -2;
    }
    receiveBlob (pBuf, ui32BlockSize, error);
    if (error != None) {
        return -3;
    }
    return ui32BlockSize;
}

void SimpleCommHelper2::sendLine (Error &error, const char *pszMsg, ...)
{
    if (_pWriter == NULL) {
        error= CommError;
        return;
    }
    va_list args;
    va_start (args, pszMsg);
    vsprintf (_pszLineBuf, pszMsg, args);
    va_end (args);
    strcat (_pszLineBuf, "\r\n");
    send (_pszLineBuf, (uint32) strlen (_pszLineBuf), error);
}

int SimpleCommHelper2::receiveLine (char *pszBuf, int iBufSize, Error &error)
{
    int rc;
    if (_pLOReader == NULL) {
        error = CommError;
        return -1;
    }
    if ((rc = _pLOReader->readLine (pszBuf, iBufSize)) < 0) {
        error = CommError;
        return -2;
    }
    //stripCRLF (pszBuf);
    error = None;
    return rc;
}

const char * SimpleCommHelper2::receiveLine (Error &error)
{
    receiveLine (_pszLineBuf, _usLineBufSize, error);
    return _pszLineBuf;
}

int SimpleCommHelper2::receiveRemainingLine (Error &error, char *pszBuf, int iBufSize, const char *pszMsg, ...)
{
    va_list args;
    va_start (args, pszMsg);
    return receiveRemainingLine (pszBuf, iBufSize, pszMsg, error, args);
}

void SimpleCommHelper2::receiveMatch (Error &error, const char *pszMsg, ...)
{
    char *pszMatchStr = new char [_usLineBufSize];

    // Get the initial string
    va_list args;
    va_start (args, pszMsg);
    vsprintf (pszMatchStr, pszMsg, args);
    va_end (args);

    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize, error);
    if (error != None) {
        return;
    }
    if (0 != strcmp (pszMatchStr, _pszLineBuf)) {
        delete[] pszMatchStr;
        error = ProtocolError;
        return;
    }
    delete[] pszMatchStr;
}

int SimpleCommHelper2::receiveMatchIndex (Error &error, unsigned short usAltCount, ...)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize, error);
    if (error != None) {
        return -1;
    }

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
    error = ProtocolError;
    return -2;
}

int SimpleCommHelper2::receiveMatchIndex (const char *apszAlternatives[], Error &error)
{
    unsigned short usCount = 0;
    while (apszAlternatives[usCount] != NULL) {
        usCount++;
    }
    return receiveMatchIndex (apszAlternatives, usCount, error);
}

int SimpleCommHelper2::receiveMatchIndex (const char *apszAlternatives[], unsigned short usCount, Error &error)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize, error);

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

    // No match was found among the alternatives
    error = ProtocolError;
    return -2;
}

int SimpleCommHelper2::receiveRemainingMatchIndex (Error &error, unsigned short usAltCount, ...)
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
    receiveRemainingLine (pszRemainingLine, _usLineBufSize, pszMsg, error, msgArgs);
    if (error != None) {
        delete[] pszRemainingLine;
        return -1;
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

    error = ProtocolError;
    return -2;
}

int SimpleCommHelper2::receiveRemainingMatchIndex (Error &error, const char *apszAlternatives[], const char *pszMsg, ...)
{
    char *pszRemainingLine = new char [_usLineBufSize];

    // Get the remaining line first
    va_list args;
    va_start (args, pszMsg);

    receiveRemainingLine (pszRemainingLine, _usLineBufSize, pszMsg, error, args);
    if (error != None) {
        delete[] pszRemainingLine;
        return -1;
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
    error = ProtocolError;
    return -2;
}

int SimpleCommHelper2::receiveRemainingMatchIndex (Error &error, const char *apszAlternatives[], unsigned short usCount, const char *pszMsg, ...)
{
    char *pszRemainingLine = new char [_usLineBufSize];

    // Get the remaining line first
    va_list args;
    va_start (args, pszMsg);

    receiveRemainingLine (pszRemainingLine, _usLineBufSize, pszMsg, error, args);
    if (error != None) {
        delete[] pszRemainingLine;
        return -1;
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

    // No match was found among the alternatives
    error = ProtocolError;
    return -2;
}

const char ** SimpleCommHelper2::receiveParsed (SimpleCommHelper2::Error &error, int* pCount)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize, error);
    if (error != None) {
        return NULL;
    }

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
    return (const char **) &_parsedTokens[0];    // NOTE: This makes the assumption that the elements in the array in DArray are contiguously allocated!
}

const char ** SimpleCommHelper2::receiveParsedDelimited (Error &error, const char* pszDelimiters, int* pCount)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize, error);
    if (error != None) {
        return NULL;
    }

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


const char ** SimpleCommHelper2::receiveParsedSpecific (const char *pszParseFmt, Error &error)
{
    // Receive a line into the internal buffer
    receiveLine (_pszLineBuf, _usLineBufSize, error);
    if (error != None) {
        return NULL;
    }

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
            delete[] pszParseFmtCopy;
            error = ProtocolError;
            return NULL;
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
                        delete[] pszParseFmtCopy;
                        error = ProtocolError;
                        return NULL;
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
    delete[] pszParseFmtCopy;
    return (const char **) &_parsedTokens[0];
}

int SimpleCommHelper2::receiveRemainingLine (char *pszBuf, int iBufSize, const char *pszMsg, Error &error, va_list args)
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
    iReceivedLineLen = receiveLine (_pszLineBuf, iMaxReceiveLineSize, error);
    if (error != None) {
        return -1;
    }

    // Make sure that we have the initial string in the received string
    if (iReceivedLineLen < iInitStrLen) {
        delete[] pszInitStr;
        throw ProtocolException ("SimpleCommHelper2::receiveRemainingLine: received line shorter than initial string");
    }
    if (0 != strncmp (pszInitStr, _pszLineBuf, iInitStrLen)) {
        delete[] pszInitStr;
        error = ProtocolError;
        return -2;
    }
    delete[] pszInitStr;

    // Now copy the remaining data into pszBuf and return
    strcpy (pszBuf, &_pszLineBuf[iInitStrLen]);
    return iReceivedLineLen - iInitStrLen;
}

void SimpleCommHelper2::stripCRLF (char *pszBuf)
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

Reader * SimpleCommHelper2::getReaderRef()
{
    return _pLOReader;
}

Writer * SimpleCommHelper2::getWriterRef()
{
    return _pWriter;
}

void SimpleCommHelper2::setDeleteUnderlyingSocket (bool bDelete)
{
    _bDeleteSocket = bDelete;
}

void SimpleCommHelper2::setDeleteUnderlyingReader (bool bDelete)
{
    // NOTE: The setDeleteUnderlyingReader() is different from setDeleteUnderlyingSocket() and setDeleteUnderlyingWriter()
    //       because in this case, the pLOReader is instantiated by the CommHelper and it must always be deleted
    //       The deletion of the pReader that was passed to the CommHelper can still be controlled - except it is done by the pLOReader
    if (_pLOReader != NULL) {
        _pLOReader->setDeleteReaderWhenDone (bDelete);
    }
}

void SimpleCommHelper2::setDeleteUnderlyingWriter (bool bDelete)
{
    _bDeleteWriter = bDelete;
}

void SimpleCommHelper2::closeConnection (Error &error)
{
    if (_pSocket == NULL) {
        error = ProtocolError;
        return;
    }
    if (_pSocket->disconnect() < 0) {
        error = ProtocolError;
        return;
    }
}

