/*
 * SessionId.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Created on June 21, 2018, 1:02 PM
 */

#include "SessionId.h"

#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

const String SessionId::MIME_TYPE ("x-dspro/x-soi-reset");

SessionId *SessionId::INSTANCE = new SessionId();

SessionId::SessionId (void)
    : _i64Timestamp (-1)
{
}

SessionId::~SessionId (void)
{
}

void SessionId::setSessionId (const char *pszSessionId, int64 i64Timestamp)
{
    const char *pszMethodName = "SessionId::setSessionId";
    if (i64Timestamp <= _i64Timestamp) {
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "Obsolete session id received <%s> (%lld < %lld).\n",
                        pszSessionId, i64Timestamp, _i64Timestamp);
        return;
    }
    const String newSessionId (pszSessionId == NULL ? "" : pszSessionId);
    std::lock_guard<std::mutex> lock (_m);
    static bool bInitialized = false;
    bool bChanged = (INSTANCE->_sessionId != pszSessionId);
    if (bChanged || (!bInitialized)) {
        _i64Timestamp = i64Timestamp;
    }
    INSTANCE->_sessionId = pszSessionId;
    checkAndLogMsg (pszMethodName, Logger::L_Info, "session set to <%s>.\n", pszSessionId);
    if (bInitialized) {
        if (bChanged) {
            for (SessionIdListener *pListener : _listeners) {
                pListener->sessionIdChanged();
            }
        }
    }
    else {
        bInitialized = true;
    }
}

void SessionId::registerSessionIdListener (SessionIdListener *pListener)
{
    if (pListener != NULL) {
        _listeners.push_back (pListener);
    }
}

String SessionId::getSessionId (NOMADSUtil::ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return String();
    }
    return String (pCfgMgr->getValue("aci.dspro.sessionKey"));
}

