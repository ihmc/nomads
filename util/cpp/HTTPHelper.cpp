/*
 * HTTPHelper.cpp
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

#include "HTTPHelper.h"

#include "CommHelper2.h"
#include "StringStringHashtable.h"

#define BUFF_LENGTH 1024

using namespace NOMADSUtil;

HTTPHelper::HTTPHelper (void)
{
    _bExpectCode = false;
    _ui16ExpectedHTTPCode = 0;
    _pContent = NULL;
    _ui32ContentLength = 0;
    _ui16LastResponseCode = 0;
    _bDeleteCommHelper = false;

    _requestMethod = NULL;
    _resourceName = NULL;

    _pHeaderFieldsHT = new StringStringHashtable();
}

HTTPHelper::~HTTPHelper (void)
{
    if (_bDeleteCommHelper && _pch) {
        delete _pch;
    }
    
    delete _pHeaderFieldsHT;
}

int HTTPHelper::init (Socket *pSock)
{
    _pch = new CommHelper2();
    _pch->init (pSock);
    _bDeleteCommHelper = true;
    return 0;
}

int HTTPHelper::init (CommHelper2 *pch)
{
    _pch = pch;
    return 0;
}

int HTTPHelper::init (Reader *pReader, Writer *pWriter)
{
    _pch = new CommHelper2();
    _pch->init (pReader, pWriter);
    _bDeleteCommHelper = true;
    return 0;
}

int HTTPHelper::sendField (const char* pszFieldName, const char* pszFieldValue)
{
    char buf[BUFF_LENGTH];
    strcpy (buf, pszFieldName);
    strcat (buf, ": ");
    strcat (buf, pszFieldValue);

    try {
        _pch->sendLine (buf);
    }
    catch (Exception) {
        return -1;
    }

    return 0;
}

int HTTPHelper::sendHeader (const char *pszMethodName, const char * pszResourceName, StringStringHashtable &headerFields)
{
    char buf[BUFF_LENGTH];
    strcpy (buf, pszMethodName);
    strcat (buf, " ");
    strcat (buf, pszResourceName);
    strcat (buf, " HTTP/1.1");

    try {
        _pch->sendLine (buf);
    
        for (StringStringHashtable::Iterator i = headerFields.getAllElements(); !i.end(); i.nextElement()) {
            strcpy (buf, i.getKey());
            strcat (buf, ": ");
            strcat (buf, i.getValue());
            _pch->sendLine (buf);
        }
        if (_bExpectCode) {
            strcpy (buf, "Expect: ");
            strcat (buf, getCodeMessage (_ui16ExpectedHTTPCode));
            _pch->sendLine (buf);
        }
        sendBlankLine();
    }
    catch (CommException) {
        return -1;
    }

    return 0;
}

int HTTPHelper::expectResponseCode (uint16 ui16ResponseCode)
{
    char buf[BUFF_LENGTH];
    
    try {
        _pch->receiveLine (buf, BUFF_LENGTH);
    }
    catch (CommException) {
        return -1;
    }
    
    char *pszAux, *pszCode;
    strtok_mt (buf, " ", &pszAux);
    pszCode = strtok_mt (NULL, " ", &pszAux);

    itoa (buf, ui16ResponseCode);
    if (strcmp (pszCode, buf) != 0) {
        _ui16LastResponseCode = atoi (pszCode);
        return -2;
    }

    try {
        int rc = _pch->receiveLine (buf, BUFF_LENGTH);
        if (strcmp ("", buf) != 0) {
            return -3;
        }
    }
    catch (CommException) {
        return -4;
    }

    return 0;
}

int HTTPHelper::checkAndSendResponseCode (uint16 ui16ResponseCode)
{
    if (_bExpectCode && (ui16ResponseCode == _ui16ExpectedHTTPCode)) {
        return sendResponseCode (_ui16ExpectedHTTPCode); //usually 100
    }
    return -1; // no response code was expected
}

uint16 HTTPHelper::getExpectedResponseCode (void)
{
    return _ui16ExpectedHTTPCode; // = 0 if no response code is expected
}

int HTTPHelper::setExpectedResponseCode (uint16 ui16ResponseCode)
{
    _bExpectCode = true;
    _ui16ExpectedHTTPCode = ui16ResponseCode;
    _pHeaderFieldsHT->put ("Expect", (char*) getCodeMessage(ui16ResponseCode));
    return 0;
}

int HTTPHelper::sendContent (void *pContent, uint32 ui32ContentLength)
{
    try {
        _pch->sendBlob (pContent, ui32ContentLength);
    }
    catch (CommException) {
        return -1;
    }
    return 0;
}

int HTTPHelper::getLastResponseCode()
{
    return _ui16LastResponseCode;
}

int HTTPHelper::receiveHeader (const char* pszProtoLine)
{
    char * pszTmp;

    _requestMethod = strtok_mt (pszProtoLine, " ", &pszTmp);
    _resourceName  = strtok_mt (NULL, " ", &pszTmp);
    
    return receiveHeader();
}

int HTTPHelper::receiveHeader()
{
    char buf[BUFF_LENGTH];
    _pHeaderFieldsHT->removeAll();
    _bExpectCode = false;

    try {
        while (true) {
            _pch->receiveLine (buf, BUFF_LENGTH);
            //printf ("HTTPHelper:: line rcvd: [%s]\n", buf);
            if (strcmp (buf, "") == 0) {
                //end of header
                break;
            }
            else {
                // parse the line and add it to the hashtable
                char *pszKey, *pszValue, *pszAux;
                pszKey = strtok_mt (buf, " :", &pszAux);
                pszValue = strtok_mt (NULL, " :", &pszAux);
                if (strcmp (pszKey, "Content-Length") == 0) {
                    _ui32ContentLength = atoi (pszValue);
                }
                else if (strcmp (pszKey, "Expect") == 0) {
                    pszValue = strtok_mt (pszValue, "- ", &pszAux);
                    _ui16ExpectedHTTPCode = atoi (pszValue);
                    _bExpectCode = true;
                }
                _pHeaderFieldsHT->put (pszKey, pszValue);
            }
        }
    }
    catch (CommException) {
        return -1;
    }
    return 0;
}

bool HTTPHelper::hasHeaderFieldValue (const char *pszKey)
{
    return (_pHeaderFieldsHT->get (pszKey) != NULL);
}

String HTTPHelper::getHeaderFieldValue (const char *pszKey)
{
    return _pHeaderFieldsHT->get (pszKey);
}

String HTTPHelper::getResourceName()
{
    return _resourceName;
}

String HTTPHelper::getRequestMethod()
{
    return _requestMethod;
}

void * HTTPHelper::receiveContent (void)
{
    if (_ui32ContentLength > 0) {
        _pContent = malloc (_ui32ContentLength);
        try {
            _pch->receiveBlob (_pContent, _ui32ContentLength);
        }
        catch (CommException) {
            return NULL;
        }
        return _pContent;
    }
    return NULL;
}

uint32 HTTPHelper::getContentLength()
{
    return _ui32ContentLength;
}

int HTTPHelper::receiveContent (void *pContent, uint32 ui32ContentLength)
{
    if (_ui32ContentLength > 0) {
        try {
            _pch->receiveBlob (pContent, ui32ContentLength);
        }
        catch (CommException) {
            return -2;
        }
        return 0;
    }
    return -1;
}

int HTTPHelper::sendResponseCode (uint16 ui16ResponseCode)
{
    try {
        _pch->sendLine (getResponseCodeMessage (ui16ResponseCode));
        _pch->sendLine ("");
    }
    catch (CommException) {
        return -1;
    }
    return 0;
}

String HTTPHelper::getResponseCodeMessage (uint16 ui16CodeNum)
{
    String s = "HTTP/1.1 ";
    s += getCodeMessage (ui16CodeNum);
    return s;
}

int HTTPHelper::sendBlankLine (void)
{
    try {
        _pch->sendLine("");
    }
    catch (CommException) {
        return -1;
    }
    return 0;
}

int HTTPHelper::flush()
{
    try {
        return _pch->getWriterRef()->flush();
    }
    catch (CommException) {
        return -1;
    }
}

/////// Private Methods ///////

const char * HTTPHelper::getCodeMessage (uint16 ui16Code)
{
    String codeMessage;
    static int shortcut[6] = {0, LEVEL_200, LEVEL_300, LEVEL_400, LEVEL_500, RESPONSE_CODES};
    int i, pos;

    // Below 100 is illegal for HTTP status
    if (ui16Code < 100) {
        return http_status_string[LEVEL_500];
    }

    for (i = 0; i < 5; i++) {
        ui16Code -= 100;
        if (ui16Code < 100) {
            pos = (ui16Code + shortcut[i]);
            if (pos < shortcut[i + 1]) {
                return http_status_string[pos];
            }
            else {
                // status unknown
                return http_status_string[LEVEL_500];
            }
        }
    }
    // 600 or above is also illegal

    return http_status_string[LEVEL_500];
}

