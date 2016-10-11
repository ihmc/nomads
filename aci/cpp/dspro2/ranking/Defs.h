/* 
 * Defs.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on February 14, 2011, 2:31 PM
 */

#ifndef INCL_DEFS_H
#define	INCL_DEFS_H
    #include "FTypes.h"

    #define AdaptorId unsigned int
    #define checkAndLogMsg if (pLogger) pLogger->logMsg
    #define logTopology if (pTopoLog) pTopoLog->logMsg
    #define memoryExhausted Logger::L_Warning, "Memory exhausted.\n"
    #define invalidCoordinates Logger::L_Warning, "Invalid coordinates. %f %f %f %f %f %f %f %f\n"
    #define listerRegistrationFailed Logger::L_SevereError, "%s could not be registered as a %s. Quitting.\n"
    #define caseUnhandled Logger::L_SevereError, "this case should never happen and it is not handled.\n"
    #define dataDeserializationError Logger::L_MildError, "could not read message. Message ID = <%s>\n"
    #define ctrlDeserializationError Logger::L_MildError, "could not read %s message from %s.\n"
    #define dbInsertError Logger::L_Warning, "Could not insert the metadata arrived into the DB. Message ID = <%s>\n"
    #define coreComponentInitFailed Logger::L_SevereError, "component could not be initialized. Quitting.\n"

    #if defined (ANDROID)
        #define USE_LOGGING_MUTEX
    #endif

    #if defined (USE_LOGGING_MUTEX)
        #define DSP_MUTEX LoggingMutex
        #define logMutexId(className, mutexName, mutexId) if (pLogger) pLogger->logMsg (className, Logger::L_Info, "created mutex %s with log %u\n", mutexName, mutexId);
        #define LOG_MUTEX true
    #else
        #define DSP_MUTEX Mutex
        #define logMutexId(className, mutexName, mutexId)
        #define LOG_MUTEX false
    #endif

    struct MutexId
    {
        static const uint16 Controller_m                            = 100;
        static const uint16 InformationStore_m                      = 101;
        static const uint16 MetadataRanker_mWeights                 = 102;
        static const uint16 NodeContextManager_mLocal               = 103;
        static const uint16 NodeContextManager_mPeer                = 104;
        static const uint16 Profiler_m                              = 105;
        static const uint16 Scheduler_m                             = 106;
        static const uint16 Scheduler_mQueues                       = 107;
        static const uint16 SchedulerPeerQueue_m                    = 108;
        static const uint16 SchedulerGeneratedMetadata_m            = 109;
        static const uint16 Scheduler_mRequests                     = 110;
        static const uint16 Scheduler_mMessageIdWrappers            = 111;
        static const uint16 UserRequests_m                          = 112;
        static const uint16 DSPro_m                                 = 113;
        static const uint16 DataStore_m                             = 114;
        static const uint16 DSProCback_m                            = 115;
        static const uint16 CommAdaptoManager_m                     = 116;
    };

    #ifdef WIN32
        #define snprintf _snprintf
        #define stringcasecmp _stricmp
    #else
        #define stringcasecmp strcasecmp
    #endif

#endif	// INCL_DEFS_H

