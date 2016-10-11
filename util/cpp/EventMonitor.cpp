/*
 * EventMonitor.cpp
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

#include "EventMonitor.h"

#include "EventNotifier.h" //defines the Event class

#include "InetAddr.h"
#include "Logger.h"
#include "StrClass.h"
#include "UDPDatagramSocket.h"

#include <stdio.h>
#include <string.h>

#if defined (WIN32)
    #if _MCS_VER<1900
        #define snprintf _snprintf    
    #endif
#endif

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

EventMonitor::EventMonitor (void)
{
    _pDGSocket = NULL;
    _fileLog = NULL;
    _pHandlerFn = NULL;
    _pCallbackArg = NULL;
}

EventMonitor::~EventMonitor (void)
{
    delete _pDGSocket;
    _pDGSocket = NULL;
}

int EventMonitor::initFileOutput (FILE *fileLog)
{
    if (fileLog == NULL) {
        _fileLog = stdout;
    }
    else {
        _fileLog = fileLog;
    }
    return 0;
}

int EventMonitor::initReceiveSocket (uint16 ui16StatPort)
{
    const char * const pszMethodName = "EventMonitor::initReceiveSocket";
    int rc;
    _pDGSocket = new UDPDatagramSocket();
    InetAddr localAddr ("127.0.0.1");
    if (0 != (rc = _pDGSocket->init (ui16StatPort, localAddr.getIPAddress()))) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "failed to initialize DatagramSocket on port %d; rc = %d\n", 
                        (int) ui16StatPort, rc);
        return -1;
    }
    return 0;
}

int EventMonitor::initHandler (EventHandlerCallbackFnPtr pHandlerFn, void *pCallbackArg)
{
    _pHandlerFn = pHandlerFn;
    _pCallbackArg = pCallbackArg;
    return 0;
}

int EventMonitor::go (void)
{
    const char * const pszMethodName = "EventMonitor::go";
    int rc;
    char buf[65535];
    InetAddr senderAddr;
    _i64StartTime = getTimeInMilliseconds();
    
    while (true) {
        rc = _pDGSocket->receive (buf, sizeof (buf), &senderAddr);
        if (rc <= 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError,
                            "receive on DatagramSocket failed; rc = %d\n", rc);
            return -1;
        }
        
        Event *pEvent = (Event*) buf;
        handleEvent (pEvent);
    }
}

int EventMonitor::handleEvent (Event *pEvent)
{
    if (!pEvent) {
        return -1;
    }
    
    //pEvent->pchEventLog[pEvent->ui16EventLogLen] = '\0';   // Just in case
    int64 i64CurrTime = getTimeInMilliseconds();

    snprintf (_szBuf, sizeof(_szBuf)-1, 
            #if defined (WIN32)
                  "%I64d, %I64d, %s, %lu, %s, %lu, %s\n",
            #else
                  "%lld, %lld, %s, %lu, %s, %lu, %s\n",
            #endif
            i64CurrTime,
            (i64CurrTime - _i64StartTime),
            (const char*) pEvent->componentName, 
            pEvent->ui32PID,
            (const char*) pEvent->instanceName,
            pEvent->ui16EventID,
            pEvent->pchEventLog);
    handleLog();
    return 0;
}


void EventMonitor::printHeader (void)
{
    snprintf (_szBuf, sizeof (_szBuf)-1, "Time, DTime, Component, PID, Instance, EventID, LogMsg\n");
    handleLog();
}

int EventMonitor::handleLog (void)
{
    _szBuf[sizeof(_szBuf)-1] = '\0';   // Just in case
    if (_fileLog) {
        fprintf (_fileLog, _szBuf);
        fflush (_fileLog);
    }
    
    if (_pHandlerFn) {
        int64 i64CurrTime = getTimeInMilliseconds();
        (*_pHandlerFn) (_pCallbackArg, _szBuf);
    }

	return 0;
}
