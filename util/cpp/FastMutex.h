/*
 * FastMutex.h
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

#ifndef INCL_FAST_MUTEX_H
#define INCL_FAST_MUTEX_H

#if defined (WIN32)
#elif defined (UNIX)
    #include <pthread.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

namespace NOMADSUtil
{
    class FastMutex
    {
        public:
            FastMutex (void);
            ~FastMutex (void);
            enum ReturnCode {
                RC_Ok = 0,
                RC_Error = -1,
                RC_TimedOut = 1,
                RC_Busy = 2
            };
            int lock (void);
            int tryLock (void);
            int unlock (void);
        private:
            #if defined (WIN32)
                void *pCritSec;
            #elif defined (UNIX)
                pthread_mutex_t _m;
            #endif
    };
}

#endif   // #ifndef INCL_FAST_MUTEX_H
