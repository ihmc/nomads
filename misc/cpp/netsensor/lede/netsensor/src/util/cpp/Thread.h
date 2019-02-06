/*
 * Thread.h
 *
 * Defines a Thread base class with the same semantics of java.lang.Thread in Java.
 * A subclass must override the run() method.
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

#ifndef INCL_THREAD_H
#define INCL_THREAD_H

#include "OSThread.h"

namespace NOMADSUtil
{
    class Thread
    {
        public:
            Thread (void);
            virtual ~Thread (void);

            void setName (const char *pszName);
            const char * getName (void);

            // Sets the priority for this thread
            // For Win32, priority ranges from 1 to 10 (with 10 being the highest priority)
            int setPriority (int priority);
            int getPriority (void);

            int start (bool bDetached = true);
            static void yield (void);
            int join (uint64 ui64TimeOutInMillis = 0);

            static Thread * currentThread (void);

            virtual void run (void) = 0;

            static void callRun (void *pArg);

        protected:
            OSThread _ost;
            bool _bStarted;
            char *_pszName;
    };

    inline const char * Thread::getName (void)
    {
        return _pszName;
    }

    inline int Thread::setPriority (int priority)
    {
        return _ost.setPriority (priority);
    }

    inline int Thread::getPriority (void)
    {
        return _ost.getPriority();
    }

    inline Thread * Thread::currentThread (void)
    {
        return (Thread*) OSThread::getThreadLocalData();
    }

    inline int Thread::join (uint64 ui64TimeOutInMillis)
    {
        return _ost.waitForThreadToTerminate (ui64TimeOutInMillis);
    }

    inline void Thread::yield (void)
    {
        OSThread::yield();
    }
}

#endif   // #ifndef INCL_THREAD_H
