/*
 * Mutex.cpp
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

#include "Mutex.h"

#if defined (WIN32)
#define NOMINMAX
#include <winsock2.h>
    #include <windows.h>
#elif defined (UNIX)
    #include <errno.h>
#endif

using namespace NOMADSUtil;

Mutex::Mutex (void)
{
    #if defined (WIN32)
        _hMutex = CreateMutex (NULL, false, NULL);
    #elif defined (LINUX)
        pthread_mutexattr_t attr;
        pthread_mutexattr_init (&attr);
        pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init (&_m, &attr);
        pthread_mutexattr_destroy (&attr);
    #elif defined (UNIX)
        pthread_mutexattr_t attr;
        pthread_mutexattr_init (&attr);
        pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init (&_m, &attr);
        pthread_mutexattr_destroy (&attr);
    #endif
}

Mutex::~Mutex (void)
{
    #if defined (WIN32)
        CloseHandle (_hMutex);
    #elif defined (UNIX)
        pthread_mutex_destroy (&_m);
    #endif
}

int Mutex::lock (void)
{
    #if defined (WIN32)
        if (WAIT_OBJECT_0 != WaitForSingleObject (_hMutex, INFINITE)) {
            return RC_Error;
        }
        return RC_Ok;
    #elif defined (UNIX)
        //printf("Thread ID: %u acquired the lock\n", (unsigned int)pthread_self());
        if (pthread_mutex_lock (&_m)) {
            return RC_Error;
        }
        //printf("Thread ID: %u acquired the lock\n", (unsigned int)pthread_self());
        return RC_Ok;
    #endif
}

int Mutex::tryLock (void)
{
    #if defined (WIN32)
        DWORD rc = WaitForSingleObject (_hMutex, 0);
        if (rc == WAIT_OBJECT_0) {
            return RC_Ok;
        }
        else if (rc == WAIT_TIMEOUT) {
            return RC_Busy;
        }
        return RC_Error;
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
//unlock

int Mutex::unlock (void)
{
    #if defined (WIN32)
        if (0 == ReleaseMutex (_hMutex)) {
            return RC_Error;
        }
        return RC_Ok;
    #elif defined (UNIX)
        if (pthread_mutex_unlock (&_m)) {
            return RC_Error;
        }
        //printf("Thread ID: %u released the lock\n", (unsigned int)pthread_self());
        return RC_Ok;
    #endif
}


