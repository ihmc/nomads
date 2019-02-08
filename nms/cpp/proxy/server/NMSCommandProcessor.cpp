/*
 * NMSCommandProcessor.cpp
 *
 * This file is part of the IHMC Network Message Service Library
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on March 1, 2015, 6:31 PM
 */

#include "NMSCommandProcessor.h"

#include "NetworkMessageService.h"

#include "StringTokenizer.h"

using namespace NOMADSUtil;

NMSCommandProcessor::NMSCommandProcessor (NetworkMessageService *pNMS)
    : _pNMS (pNMS)
{
}

NMSCommandProcessor::~NMSCommandProcessor (void)
{
}

int NMSCommandProcessor::processCmd (const void *pToken, char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    const char *pszCmd = st.getNextToken();
    if (pszCmd == NULL) {
        // Should not happen, but return 0 anyways
        return 0;
    }
    String cmd (pszCmd);
    cmd.trim();
    if (cmd ^= "ping") {
        displayPongMsg (pToken);
    }
    if ((cmd ^= "getencryptionkey") || (cmd ^= "getencryptionkeyhash") || (cmd ^= "getenckeyhash") || (cmd ^= "getenckey")) {
        getEncryptionKey (pToken);
    }
    else if ((cmd == "exit") || (cmd == "quit")) {
        return 0;
    }
    else {
        print (pToken, "unknown command - type help for a list of valid commands\n");
    }
    return 0;
}

void NMSCommandProcessor::displayPongMsg (const void *pToken)
{
    print (pToken, "pong\n");
}

void NMSCommandProcessor::getEncryptionKey (const void *pToken)
{
    const String encKeyHash (_pNMS->getEncryptionKeyHash());
    print (pToken, "Current encryption key hash: %s.\n", encKeyHash.c_str());
}
