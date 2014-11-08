/*
 * CommandProcessor.cpp
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

#include "CommandProcessor.h"

#include "CommHelper2.h"
#include "TCPSocket.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

using namespace NOMADSUtil;

CommandProcessor::CommandProcessor (void)
{
    _prompt = "";
    _pNetworkServer = NULL;
}

CommandProcessor::~CommandProcessor (void)
{
}

void CommandProcessor::setPrompt (const char *pszPrompt)
{
    if (pszPrompt == NULL) {
        _prompt = "";
    }
    else {
        _prompt = pszPrompt;
    }
}

int CommandProcessor::enableNetworkAccess (uint16 ui16Port)
{
    if (_pNetworkServer != NULL) {
        return -1;
    }
    _pNetworkServer = new NetworkServer (this);
    if (0 != _pNetworkServer->init (ui16Port)) {
        _pNetworkServer->close();
        delete _pNetworkServer;
        _pNetworkServer = NULL;
        return -2;
    }
    _pNetworkServer->start();
    return 0;
}

int CommandProcessor::print (const void *pToken, const char *pszFmt, ...)
{
    int rc;
    char szBuf[65535];
    va_list vargs;
    va_start (vargs, pszFmt);
    rc = vsnprintf (szBuf, sizeof (szBuf), pszFmt, vargs);
    va_end (vargs);
    if (rc < 0) {
        return -1;
    }
    if (pToken == NULL) {
        printf ("%s", szBuf);
    }
    else {
        ConnHandler *pCH = (ConnHandler*) pToken;
        pCH->print (szBuf);
    }
    return 0;
}

void CommandProcessor::run (void)
{
    started();
    char szCmd[1024];
    while (true) {
        fprintf (stdout, "%s> ", (const char*) _prompt);
        fflush (stdout);
        fgets (szCmd, sizeof (szCmd)-1, stdin);
        szCmd [sizeof (szCmd) - 1] = '\0';
        trim (szCmd);
        int rc;
        if (szCmd[0] != '\0') {
            if ((rc = processCmd (NULL, szCmd)) < 0) {
                setTerminatingResultCode (rc);
                terminating();
                break;
            }
        }
    }
}

const char * CommandProcessor::getPrompt (void)
{
    return _prompt;
}

void CommandProcessor::trim (char *pszBuf)
{
    if (isspace (pszBuf[0])) {
        // Need to trim at the front of the string
        char *pszDest = pszBuf;
        char *pszSrc = pszBuf + 1;
        while ((*pszSrc != '\0') && (isspace (*pszSrc))) {
            pszSrc++;
        }
        while (*pszSrc != '\0') {
            *pszDest++ = *pszSrc++;
        }
        *pszDest = '\0';
    }
    // Check for space at the end of the string
    char *pszEnd = pszBuf + (strlen (pszBuf) - 1);
    while (pszEnd > pszBuf) {
        if (isspace (*pszEnd)) {
            *pszEnd-- = '\0';
        }
        else {
            break;
        }
    }
}

NetworkServer::NetworkServer (CommandProcessor *pCP)
{
    _pCP = pCP;
    _pServerSocket = NULL;
}

NetworkServer::~NetworkServer (void)
{
    if (_pServerSocket) {
        _pServerSocket->disableReceive();
    }
    requestTerminationAndWait();
    if (_pServerSocket) {
        delete _pServerSocket;
        _pServerSocket = NULL;
    }
    _pCP = NULL;
}

int NetworkServer::init (uint16 ui16Port)
{
    if (_pServerSocket != NULL) {
        return -1;
    }
    _pServerSocket = new TCPSocket();
    if (0 != _pServerSocket->setupToReceive (ui16Port)) {
        delete _pServerSocket;
        _pServerSocket = NULL;
        return -2;
    }
    return 0;
}

void NetworkServer::close (void)
{
    if (_pServerSocket != NULL) {
        _pServerSocket->disableReceive ();
    }
}

void NetworkServer::run (void)
{
    started();
    if (_pServerSocket == NULL) {
        setTerminatingResultCode (-1);
        terminating();
    }
    Socket *pSocket;
    while ((!terminationRequested()) && (NULL != (pSocket = _pServerSocket->accept()))) {
        ConnHandler *pConnHandler = new ConnHandler (pSocket, _pCP);
        pConnHandler->start();
    }
    terminating();
}

ConnHandler::ConnHandler (Socket *pSocket, CommandProcessor *pCP)
{
    _pSocket = pSocket;
    _pCP = pCP;
    _pCH = new CommHelper2();
    _pCH->init (_pSocket);
    //_pSocket->bufferingMode (0);
}

ConnHandler::~ConnHandler (void)
{
    if (_pSocket) {
        _pSocket->disconnect();
    }
    requestTerminationAndWait();
    if (_pCH) {
        delete _pCH;
        _pCH = NULL;
    }
    if (_pSocket) {
        delete _pSocket;
        _pSocket = NULL;
    }
}

void ConnHandler::run (void)
{
    while (!terminationRequested()) {
        try {
            _pCH->send (_pCP->getPrompt(), (unsigned long)strlen (_pCP->getPrompt()));
            _pCH->send ("> ", 2);
            const char *pszLine = _pCH->receiveLine();
            if (pszLine == NULL) {
                break;
            }
            char szCmd[1024];
            strncpy (szCmd, pszLine, sizeof(szCmd)-1);
            szCmd[sizeof (szCmd)-1] = '\0';
            CommandProcessor::trim (szCmd);
            int rc;
            if (szCmd[0] != '\0') {
                if ((rc = _pCP->processCmd (this, szCmd)) < 0) {
                    setTerminatingResultCode (rc);
                    break;
                }
            }
        }
        catch (Exception) {
            break;
        }
    }
    terminating();
    delete this;
}

void ConnHandler::print (const char *pszBuf)
{
    bool bCRSent = false;
    bool bLFSent = false;
    if ((_pCH != NULL) & (pszBuf != NULL)) {
        while (*pszBuf) {
            if ((bCRSent == true) && (*pszBuf != '\n')) {
                _pCH->send ("\n", 1);
                bCRSent = false;
            }
            else if ((bLFSent == true) && (*pszBuf != '\r')) {
                _pCH->send ("\r", 1);
                bLFSent = false;
            }
            else if (*pszBuf == '\r') {
                bCRSent = true;
            }
            else if (*pszBuf == '\n') {
                bLFSent = true;
            }
            _pCH->send (pszBuf, 1);
            pszBuf++;
        }
        if (bCRSent) {
            _pCH->send ("\n", 1);
        }
        else if (bLFSent) {
            _pCH->send ("\r", 1);
        }
    }
}
