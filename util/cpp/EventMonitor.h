/*
 * EventMonitor.h
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

#ifndef INCL_EVENT_MONITOR_H
#define INCL_EVENT_MONITOR_H

#include <stdio.h>

#include "FTypes.h"
#include "StrClass.h"

namespace NOMADSUtil
{

    typedef int (*EventHandlerCallbackFnPtr) (void *pCallbackArg, const char *pszMsg);

    class UDPDatagramSocket;
    struct Event;

    class EventMonitor
    {
        public:
            EventMonitor (void);
            virtual ~EventMonitor (void);
            
            enum ports {
                DEFAULT_PORT = 23556
            };

            int initReceiveSocket (uint16 ui16StatPort = DEFAULT_PORT);
            
            // Initialize the GroupManagerStatusMonitor to send messages to a file
            // If the log file is null, the output is written to STDOUT
            int initFileOutput (FILE *fileLog = NULL);
            
            // Initialize the EventMonitor to pass messages to a handler function
            int initHandler (EventHandlerCallbackFnPtr pHandlerFn, void *pCallbackArg);
            
            int go (void);
            
        private:
            int handleEvent (Event *pEvent);
            void printHeader (void);
            int handleLog (void);

        private:
            EventHandlerCallbackFnPtr _pHandlerFn;
            void *_pCallbackArg;
            UDPDatagramSocket *_pDGSocket;
            int64 _i64StartTime;
            char _szBuf[1024];    // NOTE: This is the maximum size of an individual message that can be supported
            String _componentName;
            String _instanceName;
            FILE *_fileLog;
    };

}

#endif   // #ifndef INCL_EVENT_MONITOR_H
