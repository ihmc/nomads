/*
 * main.cpp
 *
 * This file is part of the IHMC NORM Socket Library.
 * Copyright (c) 2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "Nocket.h"

#include "InetAddr.h"
#include "Logger.h"
#include "StrClass.h"

#include <stdio.h>
#include <string.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;
using namespace IHMC_MISC;

static bool bReceiver = false;
static bool bSender = false;
static InetAddr nocketAddr ("224.1.2.3", 6003);

void printUsage (const char *pszExecName)
{
    printf ("%s [--receiver||--send]\n", pszExecName);
}

int parseArgs (int argc, char *argv[])
{
    String addr;
    for (int i = 0; i < argc; i++) {
        const String opt (argv[i]);
        if ((opt ^= "-r") || (opt ^= "--receive")) {
            bReceiver = true;
        }
        else if ((opt ^= "-s") || (opt ^= "--send")) {
            bSender = true;
        } 
        else if ((opt ^= "-h") || (opt ^= "--help")) {
            printUsage (argv[0]);
            return 0;
        }
    }
    if (bReceiver == bSender) {
        return -1;
    }
    return 0;
}

void receive (Nocket &nock)
{
    for (int rc; true;) {
        char buf[1024];
        rc = nock.receive(buf, 1024);
        if (rc < 0) {
            checkAndLogMsg ("main::receive", Logger::L_Warning, "error receiving message\n");
        }
        else {
            checkAndLogMsg ("main::receive", Logger::L_Info, "received message\n");
        }
    }
}

void send (Nocket &nock)
{
    static const unsigned int LEN = 2014;
    char data[LEN];
    memset(data, 'g', LEN);
    nock.sendTo(nocketAddr.getIPAddress(), nocketAddr.getPort(), data, LEN);
}

int main (int argc, char *argv[])
{
    Logger *pLogger = new Logger();
    if (pLogger) {
        pLogger->enableScreenOutput();
        pLogger->setDebugLevel (Logger::L_HighDetailDebug);
    }
    int rc = parseArgs (argc, argv);
    if (rc < 0) {
        printf ("Error! return code: %d.\n", rc);
        printUsage (argv[0]);
        return -1;
    }

    Nocket nock;
    nock.init (nocketAddr.getPort(), nocketAddr.getIPAddress());
    if ((rc = nock.init()) < 0) {
        printf ("Error! return code: %d.\n", rc);
        return -2;
    }

    if (bSender) {
        send (nock);
    }
    else {
        receive (nock);
    }

    return 0;
}


