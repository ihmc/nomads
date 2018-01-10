/*
 * FastMutex.cpp
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

#include "FastMutex.h"

#if defined (WIN32)
    #define _WIN32_WINNT 0x0400
	#define NOMINMAX
	#include <winsock2.h>
    #include <windows.h>
#elif defined (UNIX)
    #include <errno.h>
    #include <pthread.h>
#endif

using namespace NOMADSUtil;

FastMutex::FastMutex (void)
{
    #if defined (WIN32)
        if (NULL == (pCritSec = new CRITICAL_SECTION)) {
            // Throw C++ exception here
        }
        InitializeCriticalSection ((CRITICAL_SECTION*) pCritSec);
    #elif defined (UNIX)
        pthread_mutex_init (&_m, NULL);
    #endif
}

FastMutex::~FastMutex (void)
{
    #if defined (WIN32)
        DeleteCriticalSection ((CRITICAL_SECTION*) pCritSec);
        delete (CRITICAL_SECTION*) pCritSec;
    #elif defined (UNIX)
        pthread_mutex_destroy (&_m);
    #endif
}

int FastMutex::lock (void)
{
    #if defined (WIN32)
        EnterCriticalSection ((CRITICAL_SECTION*) pCritSec);
        return RC_Ok;
    #elif defined (UNIX)
        if (pthread_mutex_lock (&_m)) {
            return RC_Error;
        }
        return RC_Ok;
    #endif
}

int FastMutex::tryLock (void)
{
    #if defined (WIN32)
        if (0 == TryEnterCriticalSection ((CRITICAL_SECTION*) pCritSec)) {
            return RC_Busy;
        }
        return RC_Ok;
    #elif defined (UNIX)
        int rc = pthread_mutex_trylock (&_m);
        if (rc == 0) {
            return RC_Ok;
        }
        else if (rc == EBUSY) {
            return RC_Busy;
        }
        return RC_Error;
    #endif
}

int FastMutex::unlock (void)
{
    #if defined (WIN32)
        LeaveCriticalSection ((CRITICAL_SECTION*) pCritSec);
        return RC_Ok;
    #elif defined (UNIX)
        if (pthread_mutex_unlock (&_m)) {
            return RC_Error;
        }
        return RC_Ok;
    #endif
}
