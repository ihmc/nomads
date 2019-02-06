/*
 * EventNotifier.cpp
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

#include "EventNotifier.h"
#include "Logger.h"
#include "NLFLib.h" //defines getPID()

#include <string.h>

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

EventNotifier::EventNotifier (const char *pszComponentName, const char *pszInstanceName)
{
    _localHostAddr.setIPAddress ("127.0.0.1");
    _componentName = pszComponentName;
    _instanceName = pszInstanceName;
}

EventNotifier::~EventNotifier (void)
{
}

int EventNotifier::init (uint16 ui16EventMonitorPort)
{
    const char * const pszMethodName = "EventNotifier::init";
    int rc;

    if ((char*) _instanceName == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "need to set the instance name before calling init()\n");
        return -1;
    }

    if (0 != (rc = _logSocket.init())) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to initialize DatagramSocket on port %d; rc = %d\n",
                        (int) ui16EventMonitorPort, rc);
        return -2;
    }
    _ui16EventMonitorPort = ui16EventMonitorPort;
    return 0;
}

int EventNotifier::logEvent (uint16 ui16EventID, const char *pchEventLog, uint16 ui16EventLogLen,
                             const char *pszComponentName, const char *pszInstanceName)
{
    Event *pEvent = new Event();

    if (pszComponentName) {
        memcpy (pEvent->componentName, pszComponentName, strlen(pszComponentName)+1);
    }
    else {
        memcpy (pEvent->componentName, (char*) _componentName, _componentName.length()+1);
    }

    if (pszInstanceName) {
        memcpy (pEvent->instanceName, pszInstanceName, strlen(pszInstanceName)+1);
    }
    else {
        memcpy (pEvent->instanceName, (char*) _instanceName, _instanceName.length());
    }

    pEvent->ui32PID = getPID();
    pEvent->ui16EventID = ui16EventID;
    memcpy (pEvent->pchEventLog, (char*) pchEventLog, ui16EventLogLen);
    pEvent->ui16EventLogLen = ui16EventLogLen;

    _bufWriter.reset();
    _bufWriter.writeBytes (pEvent, sizeof(Event));

    if (0 != _logSocket.sendTo(_localHostAddr.getIPAddress(), _ui16EventMonitorPort, _bufWriter.getBuffer(), _bufWriter.getBufferLength())) {
        delete pEvent;
        pEvent = NULL;
        return -1;
    }

    delete pEvent;
    return 0;
}

