/*
 * EventNotifier.h
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

#ifndef INCL_EVENT_NOTIFIER_H
#define INCL_EVENT_NOTIFIER_H

#include <stdarg.h>

#include "BufferWriter.h"
#include "FTypes.h"
#include "InetAddr.h"
#include "StrClass.h"
#include "UDPDatagramSocket.h"

#include <stdio.h>

#define MAX_STR_LEN 256
#define MAX_LOG_LEN 1024

namespace NOMADSUtil
{

    struct Event
    {
        char componentName[MAX_STR_LEN];
        char instanceName[MAX_STR_LEN];
        uint32 ui32PID;
        uint16 ui16EventID;
        char pchEventLog[MAX_LOG_LEN];
        uint16 ui16EventLogLen;
    };

    /*
     * The EventNotifier class sends out event information information 
     * on a UDP port, which may be monitored by the EventMonitor or 
     * some other component
     */
    class EventNotifier
    {
        public:
            EventNotifier (const char *pszComponentName, const char *pszInstanceName = NULL);
            virtual ~EventNotifier (void);

            enum ports {
                DEFAULT_PORT = 23556
            };
        
            void setInstanceName (const char *pszInstanceName);
            String getInstanceName (void);
            String getComponentName (void);

            int init (uint16 ui16EventMonitorPort = DEFAULT_PORT);
            int logEvent (uint16 ui16EventID, 
                          const char *pchEventLog, 
                          uint16 ui16EventLogLen, 
                          const char *pszComponentName = NULL, 
                          const char *pszInstanceName = NULL);

        private:
            BufferWriter _bufWriter;
            InetAddr _localHostAddr;
            uint16 _ui16EventMonitorPort;
            UDPDatagramSocket _logSocket;
            String _componentName;
            String _instanceName;
    };

    inline void EventNotifier::setInstanceName (const char *pszInstanceName)
    {
        _instanceName = pszInstanceName;
    }

    inline String EventNotifier::getInstanceName (void)
    {
        return _instanceName;
    }
     
    inline String EventNotifier::getComponentName (void)
    {
        return _componentName;
    }

}

#endif   // #ifndef INCL_EVENT_NOTIFIER_H

