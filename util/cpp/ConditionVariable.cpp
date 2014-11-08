/*
 * ConditionVariable.cpp
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

#include "ConditionVariable.h"

#include "Mutex.h"

#if defined (WIN32)
    #define _WIN32_WINNT 0x0400
    #include <windows.h>
#elif defined (UNIX)
    #include <pthread.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <errno.h>
#endif

using namespace NOMADSUtil;

ConditionVariable::ConditionVariable (Mutex *pMutex)
{
    _pMutex = pMutex;
    #if defined (WIN32)
        _hEventObj = CreateEvent (NULL, FALSE, FALSE, NULL);
        _ulCount = 0;
    #elif defined (UNIX)
        pthread_cond_init (&_c, NULL);
    #endif
}

ConditionVariable::~ConditionVariable (void)
{
    #if defined (WIN32)
        CloseHandle (_hEventObj);
    #elif defined (UNIX)
        pthread_cond_destroy (&_c);
    #endif
}

int ConditionVariable::wait ()
{
    if (_pMutex == NULL) {
        return -1;
    }
    #if defined (WIN32)
        _ulCount++;
        ResetEvent (_hEventObj);
        if (WAIT_OBJECT_0 != SignalObjectAndWait (*_pMutex, _hEventObj, INFINITE, FALSE)) {
            return -2;
        }
        if (_pMutex->lock()) {
            return -3;
        }
    #elif defined (UNIX)
        // The function pthread_cond_wait atomically releases mutex and causes the calling thread to block on the condition variable.
        if (pthread_cond_wait (&_c, *_pMutex)) {
            return -2;
        }
    #endif
    return 0;
}

int ConditionVariable::wait (int64 i64Millisec)
{
    if (_pMutex == NULL) {
        return -1;
    }
    #if defined (WIN32)
        _ulCount++;
        ResetEvent (_hEventObj);
        int rc = SignalObjectAndWait (*_pMutex, _hEventObj, (DWORD)i64Millisec, FALSE);
        if ((rc != WAIT_OBJECT_0) && (rc != WAIT_TIMEOUT)) {
            return -2;
        }
        if (_pMutex->lock()) {
            return -3;
        }
    #elif defined (UNIX)
        struct timeval tv;
        if (gettimeofday (&tv,0) < 0) {
            return -2;
        }
        struct timespec ts;
        ts.tv_nsec = (i64Millisec%1000UL) * 1000000UL + tv.tv_usec * 1000L;
        ts.tv_sec = i64Millisec/1000UL + tv.tv_sec + ts.tv_nsec/1000000000L;
        ts.tv_nsec %= 1000000000L;
        // The function pthread_cond_timedwait atomically releases mutex and causes the calling thread to block on the condition variable.
        if (pthread_cond_timedwait (&_c, *_pMutex, &ts) != ETIMEDOUT)
            return -3;
    #endif
    return 0;
}

int ConditionVariable::wait (int64 i64Millisec, bool *timedOut)
{
    int rc=0;

    if (_pMutex == NULL) {
        return -1;
    }

    #if defined (WIN32)

       return -1; // not implemented on Win32!

    #elif defined (UNIX)
        struct timeval tv;
        if (gettimeofday (&tv,0) < 0) {
            return -2;
        }
        struct timespec ts;
        ts.tv_nsec = (i64Millisec%1000UL) * 1000000UL + tv.tv_usec * 1000L;
        ts.tv_sec = i64Millisec/1000UL + tv.tv_sec + ts.tv_nsec/1000000000L;
        ts.tv_nsec %= 1000000000L;
        // The function pthread_cond_timedwait atomically releases mutex and causes the calling thread to block on the condition variable.
        rc = pthread_cond_timedwait (&_c, *_pMutex, &ts);
        if(rc == ETIMEDOUT)
        {
            *timedOut = true;
            rc = 0;
        }
        else
        {
            *timedOut = false;
            if(rc != 0)
                rc = -3;
        }

    #endif

    return rc;
}

int ConditionVariable::notify (void)
{
    if (_pMutex == NULL) {
        return -1;
    }
    #if defined (WIN32)
        if (_ulCount > 0) {
            _ulCount--;
            if (FALSE == SetEvent (_hEventObj)) {
                return -2;
            }
        }
    #elif defined (UNIX)
        if (pthread_cond_signal (&_c)) {
            return -2;
        }
    #endif
    return 0;
}

int ConditionVariable::notifyAll (void)
{
    if (_pMutex == NULL) {
        return -1;
    }
    #if defined(WIN32)
        while (_ulCount > 0) {
            _ulCount--;
            if (FALSE == SetEvent (_hEventObj)) {
                return -2;
            }
        }
    #elif defined(UNIX)
        if (pthread_cond_broadcast (&_c)) {
            return -2;
        }
    #endif
    return 0;
}
