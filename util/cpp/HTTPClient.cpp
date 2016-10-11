/*
 * HTTPClient.cpp
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

#include "HTTPClient.h"
#include "CommHelper.h"
#include "StrClass.h"
#include "TCPSocket.h"
#include "URLParser.h"

#include <stdio.h>
#include <string.h>

using namespace NOMADSUtil;

// Opens a connection to a specified host and port name.
TCPSocket * HTTPClient::openConnection (const char *pszHost, unsigned short usPort)
{
    int rc;
    TCPSocket *pSocket = new TCPSocket();
    if (rc = pSocket->connect (pszHost, usPort) < 0) {
        return NULL;
    }
    return pSocket;
}

// Gets the data and returns a struct given the host, port and path name.
HTTPClient::HTTPData HTTPClient::getData (const char *pszHost, unsigned short usPort, const char *pszPath)
{
    HTTPData data;

    // Open a socket connection
    TCPSocket *pSocket;
    pSocket = openConnection (pszHost, usPort);
    if (pSocket == NULL) {
        data.metaData.rc = HTTPMetaData::RC_InvalidNetworkAddr;
        return data;
    }
    int rc;
    CommHelper ch;
    if (0 != (rc = ch.init (pSocket))) {
        data.metaData.rc = HTTPMetaData::RC_InvalidNetworkAddr;
        return data;
    }
    // Send a line containing the protocol.
    ch.sendLine ("GET %s HTTP/1.0\r\n", pszPath);
    try {
        const char **apszMsgReceived;
        apszMsgReceived = ch.receiveParsed();
        if (apszMsgReceived == NULL) {
            return data;
        }
        // If message received contains 404 Page not found, it returns null.
        if (0 == strcmp (apszMsgReceived[1], "404")) {
            data.metaData.rc = HTTPMetaData::RC_InvalidPath;
            return data;
        }
        else {
            // If the page is valid, it gets the data and put it into a struct.
            const char **apszHeader = NULL;
            unsigned long ulSize = 0;
            while (1) {
                apszHeader = ch.receiveParsedDelimited(":");
                if ((apszHeader == NULL) || (apszHeader[0] == NULL)) {
                    break;
                }
                if (strcmp (apszHeader[0], "Content-Length") == 0) {
                    ulSize = atol (apszHeader[1]);
                }
            }
            if (ulSize <= 0) {
                /*!!*/ // Should this not return an error?
                return data;
            }
            char *pBuf = new char [ulSize];
            ch.receiveBlob (pBuf, ulSize);
            if (pBuf == NULL) {
                data.metaData.rc = HTTPMetaData::RC_UnknownError;
                return data;
            }
            data.metaData.ui32Length = ulSize;
            data.pData = pBuf;
            return data;
        }
    }
    catch (Exception) {
        data.metaData.rc = HTTPMetaData::RC_UnknownError;
        return data;
    }
    delete pSocket;
    return data;
}

// Gets the data and returns a struct given an URL.
HTTPClient::HTTPData HTTPClient::getData (const char *pszURL)
{
    HTTPData data;
    URLParser parser;
    parser.parse (pszURL);
    const char *pszHost = parser.getHost();
    unsigned short usPort = parser.getPort();
    const char *pszPath = parser.getPath();
    if ((pszHost != NULL) && (usPort >0)) {
        return (getData (pszHost, usPort, pszPath));
    }
    else {
        data.metaData.rc = HTTPMetaData::RC_InvalidNetworkAddr;
        return data;
    }
}

// Writes the data to a file given the file name and the URL.
HTTPClient::HTTPMetaData HTTPClient::getDataIntoFile (const char *pszFile, const char *pszURL)
{
    HTTPData data;
    data = getData (pszURL);
    FILE *fileOut = fopen (pszFile, "wb");
    fwrite (data.pData, 1, data.metaData.ui32Length, fileOut);
    fclose (fileOut);
    free (data.pData);
    return data.metaData;
}

// Writes the data to a file given the file name and the host, port and path name.
HTTPClient::HTTPMetaData HTTPClient::getDataIntoFile (const char *pszFile, const char *pszHost, 
                                                      unsigned short usPort, const char *pszPath)
{
    HTTPData data;
    data = getData (pszHost, usPort, pszPath);
    FILE *fileOut = fopen (pszFile, "wb");
    fwrite (data.pData, 1, data.metaData.ui32Length, fileOut);
    fclose (fileOut);
    free (data.pData);
    return data.metaData;
}
