/*
 * Mutex.h
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

#ifndef INCL_MUTEX_H
#define INCL_MUTEX_H

#if defined (WIN32)
    typedef void * HANDLE;
#elif defined (UNIX)
    #ifndef _XOPEN_SOURCE
        #define _XOPEN_SOURCE 600 // Enables UNIX98 Compatibility
    #endif
    #include <pthread.h>
    #include <stdio.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

namespace NOMADSUtil
{
    class Mutex
    {
        public:
            Mutex (void);
            virtual ~Mutex (void);
            enum ReturnCode {
                RC_Ok = 0,
                RC_Error = -1,
                RC_TimedOut = 1,
                RC_Busy = 2
            };
            int lock (void);

            // Returns RC_Busy if mutex is already locked and RC_Ok if lock was obtained
            int tryLock (void);

            int unlock (void);
            #if defined (WIN32)
                operator HANDLE (void);
            #elif defined (UNIX)
                operator pthread_mutex_t * (void);
            #endif
        private:
            #if defined (WIN32)
                HANDLE _hMutex;
            #elif defined (UNIX)
                pthread_mutex_t _m;
            #else
                #error Must Define WIN32 or UNIX!
            #endif
    };

    #if defined (WIN32)
        inline Mutex::operator HANDLE (void)
        {
            return _hMutex;
        }
    #elif defined (UNIX)
        inline Mutex::operator pthread_mutex_t * (void)
        {
            return &_m;
        }
    #endif
}

#endif   // #ifndef INCL_MUTEX_H
