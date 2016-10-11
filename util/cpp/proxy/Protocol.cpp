/* 
 * Protocol.cpp
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 29, 2015, 4:43 PM
 */

#include "Protocol.h"

#include "Logger.h"

using namespace NOMADSUtil;

const String Protocol::REGISTER_PROXY = "RegisterProxy";
const String Protocol::REGISTER_PROXY_CALLBACK = "RegisterProxyCallback";

SimpleCommHelper2::Error Protocol::doHandshake (SimpleCommHelper2 *pCommHelper, const String &service, const String &version)
{
    const char *pszMethodName = "Protocol::doHandshake";
    SimpleCommHelper2::Error error = SimpleCommHelper2::None;
    pCommHelper->sendLine (error, "%s %s", service.c_str() , version.c_str());
    if (error != SimpleCommHelper2::None) {
        return error;
    }

    // Check whether the client is expecting to connect to this service, and that
    // the protocol is the same
    int iCount = 0;
    const char **ppszBuf = pCommHelper->receiveParsed (error, &iCount);
    if (error != SimpleCommHelper2::None) {
        return error;
    }
    if (iCount != 2) {
        return SimpleCommHelper2::ProtocolError;
    }

    if ((service != ppszBuf[0]) || (version != ppszBuf[1])) {
        if (pLogger != NULL) pLogger->logMsg (pszMethodName, Logger::L_Warning,
                        "service mismatch: received %s %s, while expecting %s %s.\n",
                        ppszBuf[0], ppszBuf[1], service.c_str() , version.c_str());
        return SimpleCommHelper2::ProtocolError;
    }
    return error;
}

