/*
 * OSThread.h
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

#ifndef INCL_OS_THREAD_H
#define INCL_OS_THREAD_H

#if defined (WIN32)
    typedef void * HANDLE;
    typedef unsigned long DWORD;
#elif defined (UNIX)
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/types.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "FTypes.h"

namespace NOMADSUtil
{

    #if defined (WIN32)
        class ThreadLocalStorage
        {
            public:
                ThreadLocalStorage (void);
                ~ThreadLocalStorage (void);
                DWORD dwTLSIndex;
        };
    #endif

    class OSThread
    {
        public:
            OSThread (void);
            ~OSThread (void);

            #if defined (WIN32)
                int start (void (*pFn) (void *pArg), void *pArg);
            #elif defined (UNIX)
                int start (void (*pFn) (void *pArg), void *pArg, bool bDetached = true);
            #endif

            // Creates an instance of OSThread that represents the current thread (i.e., the caller thread)
            // The caller must delete the OSThread when done
            static OSThread * createOSThreadForCurrentThread (void);

            static void yield();
            int suspend (void);
            int resume (void);

            // Sets the priority for this thread
            // For Win32, priority ranges from 1 to 10 (with 10 being the highest priority)
            int setPriority (int priority);

            // Returns the current priority for this thread
            int getPriority (void);

            // Returns true if the thread was started
            bool wasStarted (void);

            int terminate (void);

            static void setThreadLocalData (void *pData);
            static void * getThreadLocalData (void);

            // Wait for thread to terminate
            // The caller will be blocked until the this thread terminates or the timeout expires
            int waitForThreadToTerminate (uint64 ui64TimeOutInMillis = 0UL);

            // Check to see if the thread needs to be terminated (cancelled)
            // Should be called by threads when they do not call other
            //     cancellation functions
            // Currently important only for POSIX Threads
            static int checkForTermination (void);

            // Returns the percentage of CPU used by the current thread
            // The value returned may range from 0.0 to 100.0
            float getCPUUtilization (void);

            // Sets the thread name
            // Right now this only has an effect on Windows where it sets the
            // thread name shown in the visual studio debugger.
            void setThreadName (const char *format, ...);

        protected:
            struct ThreadArg {
                OSThread *pThis;
                void (*pFn) (void *pArg);
                void *pArg;
            };

            #if defined (UNIX)
                static void execUserFunc (void *pArg);
            #endif

            #if defined (LINUX)
                float getLinuxCPUUtilization (void);
            #endif

        protected:
            #if defined (WIN32)
                HANDLE _hThread;
                DWORD _dwThreadId;
                int64 _i64TimeWhenLastQueriedUtilization;
                int64 _i64LastQueriedCPUTime;
                static ThreadLocalStorage _tls;
            #elif defined (UNIX)
                pid_t _pid;       // Needed since threads map to different pids
                pthread_t _tid;
                bool _bIsDetachedMode;
            #endif
            bool _bStarted;
    };

    inline bool OSThread::wasStarted (void)
    {
        return _bStarted;
    }

    inline int OSThread::checkForTermination (void)
    {
        #if defined (ANDROID)
            //do nothing
        #elif defined (UNIX)
            pthread_testcancel();
        #endif
        return 0;
    }

}

#endif   // #ifndef INCL_OS_THREAD_H
