/*
 * ConditionVariable.h
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

#ifndef INCL_CONDITION_VARIABLE_H
#define INCL_CONDITION_VARIABLE_H

#include "FTypes.h"

#if defined (WIN32)
    typedef void * HANDLE;
#elif defined (UNIX)
    #include <pthread.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

namespace NOMADSUtil
{
    class Mutex;

    class ConditionVariable
    {
        public:
            ConditionVariable (Mutex *pMutex);
            ~ConditionVariable();
            // These functions use pthread_cond_wait() and pthread_cond_timedwait().
            // The pthread_cond_wait() and pthread_cond_timedwait() functions are used to block on a condition variable.
            // They are called with mutex locked by the calling thread or undefined behaviour will result.
            // These functions atomically release mutex and cause the calling thread to block on the condition variable.
            int wait (void);
            int wait (int64 i64Millisec);
            int wait (int64 i64Millisec, bool *timedOut);
            // When each thread unblocked as a result of a pthread_cond_broadcast() or pthread_cond_signal() returns from
            // its call to pthread_cond_wait() or pthread_cond_timedwait(), the thread shall own the mutex with which it
            // called pthread_cond_wait() or pthread_cond_timedwait().
            // The thread(s) that are unblocked shall contend for the mutex according to the scheduling policy (if applicable),
            // and as if each had called pthread_mutex_lock().
            int notify (void);
            int notifyAll (void);
        protected:
            Mutex *_pMutex;
            #if defined (WIN32)
                HANDLE _hEventObj;
                volatile unsigned long _ulCount;
            #elif defined (UNIX)
                pthread_cond_t _c;
            #else
                #error Must Define WIN32 or UNIX!
            #endif
    };
}

#endif   // #ifndef INCL_CONDITION_VARIABLE_H
