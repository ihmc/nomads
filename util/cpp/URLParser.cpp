/*
 * URLParser.cpp
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

#include "URLParser.h"

#include <stdlib.h>
#include <string.h>

using namespace NOMADSUtil;

URLParser::URLParser (void)
{
    _pszBuf = NULL;
    _pszProtocol = NULL;
    _pszHost = NULL;
    _iPort = -1;
    _pszPath = NULL;
}

URLParser::URLParser (const char *pszURL) throw (FormatException)
{
    _pszBuf = NULL;
    _pszProtocol = NULL;
    _pszHost = NULL;
    _iPort = -1;
    _pszPath = NULL;
    parse (pszURL);
}

URLParser::~URLParser (void)
{
    delete[] _pszBuf;
    _pszProtocol = NULL;
    _pszHost = NULL;
    _iPort = -1;
    delete[] _pszPath;
}

void URLParser::parse (const char *pszURL) throw (FormatException)
{
    if (pszURL == NULL) {
        throw FormatException ("null argument");
    }

    // Construct a buffer to hold a working copy of the URL
    _pszBuf = new char [strlen(pszURL)+2];  // +2 to allow for a missing '/' and the '\0'
    strcpy (_pszBuf, pszURL);

    // Get the protocol
    _pszProtocol = _pszBuf;

    char *pszTmp = strstr (_pszProtocol, "://");
    if (pszTmp == NULL) {
        throw FormatException ("invalid format");
    }

    *pszTmp = '\0';     // Put a null char to terminate the protocol
    pszTmp += 3;        // Now point to the hostname

    if (*pszTmp == '\0') {
        // No hostname was specified!
        throw FormatException ("missing hostname");
    }

    _pszHost = pszTmp;

    // Look for the end of the hostname (and portnumber)
    while ((*pszTmp != '/') && (*pszTmp != '\0')) {
        pszTmp++;
    }

    // Copy the path (or assign an empty path)
    if (*pszTmp == '\0') {
        _pszPath = new char [2];
        strcpy (_pszPath, "/");
    }
    else {
        _pszPath = new char [strlen(pszTmp)+1];
        strcpy (_pszPath, pszTmp);
    }

    // Terminate the host (and portnumber) string
    *pszTmp = '\0';

    // Look for the port number
    pszTmp = _pszHost;
    while (*pszTmp != '\0') {
        if (*pszTmp == ':') {
            _iPort = (atoi (pszTmp+1));
            *pszTmp = '\0';
            break;
        }
        pszTmp++;
    }
}

//Returns the protocol for the URL
const char * URLParser::getProtocol (void) throw (InvalidStateException)
{
    return _pszProtocol;
}

//Returns the host for the URL
const char * URLParser::getHost (void) throw (InvalidStateException)
{
    return _pszHost;
}

//Returns the port, if any, for the URL
int URLParser::getPort (void) throw (InvalidStateException)
{
    if (_iPort == -1 ) {
        _iPort = 80;
    }
    return _iPort;
}

//Returns the path for the URL
const char * URLParser::getPath (void) throw (InvalidStateException)
{
    return _pszPath;
}

