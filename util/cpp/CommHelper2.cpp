/*
 * CommHelper2.cpp
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

#include "CommHelper2.h"

#include "NLFLib.h"
#include "Socket.h"
#include "SocketReader.h"
#include "SocketWriter.h"

#include <stdio.h>

#if defined (WIN32)
    #if _MCS_VER<1900
        #define snprintf _snprintf
    #endif
#endif

using namespace NOMADSUtil;



CommHelper2::CommHelper2 (void)
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

CommHelper2::~CommHelper2 (void)
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

int CommHelper2::init (Socket *pSocket, unsigned short usMaxLineSize)
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

int CommHelper2::init (Reader *pReader, Writer *pWriter, unsigned short usMaxLineSize)
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

uint32 CommHelper2::send (const void *pBuf, uint32 ui32Size)
{
    int rc;
    if (_pWriter == NULL) {
        throw CommException ("CommHelper2::send: CommHelper not initialized for writing");
    }
    if ((rc = _pWriter->writeBytes (pBuf, ui32Size)) < 0) {
        if (_pSocket) {
            sprintf (_szErrorMsgBuf, "CommHelper2::send: socket error %d:%d", rc, _pSocket->error());
        }
        else {
            sprintf (_szErrorMsgBuf, "CommHelper2::send: write error; rc = %d", rc);
        }

        throw CommException (_szErrorMsgBuf);
    }
    return ui32Size;
}

uint32 CommHelper2::receive (void *pBuf, uint32 ui32Size)
{
    int rc;
    if (_pLOReader == NULL) {
        throw CommException ("CommHelper2::send: CommHelper not initialized for writing");
    }
    if ((rc = _pLOReader->read (pBuf, (int)ui32Size)) <= 0) {
        if (_pSocket) {
            sprintf (_szErrorMsgBuf, "CommHelper2::receive: socket error %d:%d", rc, _pSocket->error());
        }
        else {
            sprintf (_szErrorMsgBuf, "CommHelper2::receive: reader error %d", rc);
        }

        throw CommException (_szErrorMsgBuf);
    }
    return rc;
}

void CommHelper2::sendBlob (const void *pBuf, uint32 ui32Size)
{
    // With a writer, there is not difference between send and sendBlob because the
    // writer will treat a partial write as a failure
    send (pBuf, ui32Size);
}

void CommHelper2::receiveBlob (void *pBuf, uint32 ui32Size)
{
    int rc;
    if (_pLOReader == NULL) {
        throw CommException ("CommHelper2:receiveBlob: CommHelper not initialized for reading");
    }
    if (0 != (rc = _pLOReader->readBytes (pBuf, ui32Size))) {
        if (_pSocket) {
            sprintf (_szErrorMsgBuf, "CommHelper2::receiveBlob: socket error %d:%d", rc, _pSocket->error());
        }
        else {
            sprintf (_szErrorMsgBuf, "CommHelper2::receiveBlob: reader error %d", rc);
        }
        throw CommException (_szErrorMsgBuf);
    }
}

void CommHelper2::sendBlock (const void *pBuf, uint32 ui32BufSize)
{
    if (pBuf == NULL) {
        ui32BufSize = 0;
    }
    uint32 ui32BlockSize = htonl (ui32BufSize);
    sendBlob (&ui32BlockSize, sizeof (ui32BlockSize));
    if (ui32BufSize > 0) {
        sendBlob (pBuf, ui32BufSize);
    }
}

void CommHelper2::sendBlock (const void *pBuf1, uint32 ui32Buf1Size, const void *pBuf2, uint32 ui32Buf2Size)
{
    uint32 ui32BlockSize = htonl (ui32Buf1Size + ui32Buf2Size);
    sendBlob (&ui32BlockSize, sizeof (ui32BlockSize));
    sendBlob (pBuf1, ui32Buf1Size);
    sendBlob (pBuf2, ui32Buf2Size);
}

void CommHelper2::sendStringBlock (const char *pszBuf)
{
    uint32 ui32Len = (pszBuf == NULL ? 0 : (uint32)strlen (pszBuf));
    if (ui32Len == 0) {
        _pWriter->write32 (&ui32Len);
    }
    else {
        sendBlock (pszBuf, ui32Len);
    }
}

uint32 CommHelper2::receiveBlock (void *pBuf, uint32 ui32BufSize)
{
    uint32 ui32BlockSize;
    receiveBlob (&ui32BlockSize, sizeof (ui32BlockSize));
    ui32BlockSize = ntohl (ui32BlockSize);
    if (ui32BlockSize > ui32BufSize) {
        sprintf (_szErrorMsgBuf, "CommHelper::receiveBlock: receiver block size of %lu is not large enough to hold block of size %lu\n",
                 ui32BufSize, ui32BlockSize);
        throw ProtocolException (_szErrorMsgBuf);
    }
    receiveBlob (pBuf, ui32BlockSize);
    return ui32BlockSize;
}

void CommHelper2::sendLine (const char *pszMsg, ...)
{
    if (_pWriter == NULL) {
        throw CommException ("CommHelper2::sendLine: CommHelper not initialized for writing");
    }
    va_list args;
    va_start (args, pszMsg);
    vsprintf (_pszLineBuf, pszMsg, args);
    va_end (args);
    strcat (_pszLineBuf, "\r\n");
    send (_pszLineBuf, (uint32) strlen (_pszLineBuf));
}

int CommHelper2::receiveLine (char *pszBuf, int iBufSize)
{
    int rc;
    if (_pLOReader == NULL) {
        throw CommException ("CommHelper2::receiveLine: CommHelper not initialized for reading");
    }
    if ((rc = _pLOReader->readLine (pszBuf, iBufSize)) < 0) {
        if (_pSocket != NULL) {
            sprintf (_szErrorMsgBuf, "CommHelper2::receiveLine: socket error %d:%d", rc, _pSocket->error());
        }
        else {
            sprintf (_szErrorMsgBuf, "CommHelper2::receiveLine: error reading line from underlying reader: %d", rc);
        }
        throw CommException (_szErrorMsgBuf);
    }
    //stripCRLF (pszBuf);
    return rc;
}

const char * CommHelper2::receiveLine (void)
{
    receiveLine (_pszLineBuf, _usLineBufSize);
    return _pszLineBuf;
}

int CommHelper2::receiveRemainingLine (char *pszBuf, int iBufSize, const char *pszMsg, ...)
{
    va_list args;
    va_start (args, pszMsg);
    return receiveRemainingLine (pszBuf, iBufSize, pszMsg, args);
}

void CommHelper2::receiveMatch (const char *pszMsg, ...)
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
        snprintf (_szErrorMsgBuf, sizeof (_szErrorMsgBuf), "CommHelper2::receiveMatch: mismatch in expected (%s) and received (%s) strings", pszMatchStr, _pszLineBuf);
        _szErrorMsgBuf[sizeof(_szErrorMsgBuf)-1] = '\0';
        delete[] pszMatchStr;
        throw ProtocolException (_szErrorMsgBuf);
    }
    delete[] pszMatchStr;
}

int CommHelper2::receiveMatchIndex (unsigned short usAltCount, ...)
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
    throw ProtocolException ("CommHelper2::receiveMatchIndex: received line does not match any alternatives");
}

int CommHelper2::receiveMatchIndex (const char *apszAlternatives[])
{
    unsigned short usCount = 0;
    while (apszAlternatives[usCount] != NULL) {
        usCount++;
    }
    return receiveMatchIndex (apszAlternatives, usCount);
}

int CommHelper2::receiveMatchIndex (const char *apszAlternatives[], unsigned short usCount)
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

    // No match was found among the alternatives
    throw ProtocolException ("CommHelper2::receiveMatchIndex: received line does not match any alternatives");
}

int CommHelper2::receiveRemainingMatchIndex (unsigned short usAltCount, ...)
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

    throw ProtocolException ("CommHelper2::receiveMatchIndex: received line does not match any alternatives");
}

int CommHelper2::receiveRemainingMatchIndex (const char *apszAlternatives[], const char *pszMsg, ...)
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
    throw ProtocolException ("CommHelper2::receiveMatchIndex: received line does not match any alternatives");
}

int CommHelper2::receiveRemainingMatchIndex (const char *apszAlternatives[], unsigned short usCount, const char *pszMsg, ...)
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

    // No match was found among the alternatives
    throw ProtocolException ("CommHelper2::receiveMatchIndex: received line does not match any alternatives");
}

const char ** CommHelper2::receiveParsed (int* pCount)
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
    return (const char **) &_parsedTokens[0];    // NOTE: This makes the assumption that the elements in the array in DArray are contiguously allocated!
}

const char ** CommHelper2::receiveParsedDelimited (const char* pszDelimiters, int* pCount)
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


const char ** CommHelper2::receiveParsedSpecific (const char *pszParseFmt)
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
            delete[] pszParseFmtCopy;
            throw ProtocolException ("CommHelper2::receiveParsedSpecific: premature end of line");
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
                        throw ProtocolException ("CommHelper2::receiveParsedSpecific: premature end of line");
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

int CommHelper2::receiveRemainingLine (char *pszBuf, int iBufSize, const char *pszMsg, va_list args)
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
        throw ProtocolException ("CommHelper2::receiveRemainingLine: received line shorter than initial string");
    }
    if (0 != strncmp (pszInitStr, _pszLineBuf, iInitStrLen)) {
        delete[] pszInitStr;
        throw ProtocolException ("CommHelper2::receiveRemainingLine: mismatch between receievd line and initial string");
    }
    delete[] pszInitStr;

    // Now copy the remaining data into pszBuf and return
    strcpy (pszBuf, &_pszLineBuf[iInitStrLen]);
    return iReceivedLineLen - iInitStrLen;
}

void CommHelper2::stripCRLF (char *pszBuf)
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

Reader * CommHelper2::getReaderRef()
{
    return _pLOReader;
}

Writer * CommHelper2::getWriterRef()
{
    return _pWriter;
}

void CommHelper2::setDeleteUnderlyingSocket (bool bDelete)
{
    _bDeleteSocket = bDelete;
}

void CommHelper2::setDeleteUnderlyingReader (bool bDelete)
{
    // NOTE: The setDeleteUnderlyingReader() is different from setDeleteUnderlyingSocket() and setDeleteUnderlyingWriter()
    //       because in this case, the pLOReader is instantiated by the CommHelper and it must always be deleted
    //       The deletion of the pReader that was passed to the CommHelper can still be controlled - except it is done by the pLOReader
    if (_pLOReader != NULL) {
        _pLOReader->setDeleteReaderWhenDone (bDelete);
    }
}

void CommHelper2::setDeleteUnderlyingWriter (bool bDelete)
{
    _bDeleteWriter = bDelete;
}

void CommHelper2::closeConnection()
{
    if (_pSocket == NULL) {
        throw new ProtocolException ("CommHelper2::closeConnection: CommHelper not initialized with a socket");
    }
    if (_pSocket->disconnect() < 0) {
        throw ProtocolException ("CommHelper2::closeConnection: unable to close the connection");
    }
}


