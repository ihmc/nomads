/*
 * Semaphore.h
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

#ifndef INCL_SEMAPHORE_H
#define INCL_SEMAPHORE_H

#if defined (WIN32)
    typedef void * HANDLE;
#elif defined (OSX)
    #include <sys/semaphore.h>
#elif defined (UNIX)
    #include <semaphore.h>
#endif

namespace NOMADSUtil
{
    class Semaphore
    {
        public:
            Semaphore (int iInitValue = 0);
            ~Semaphore (void);
            int up (void);
            int down (void);
        private:
            #if defined (WIN32)
                HANDLE hSem;
            #elif defined (UNIX)
                sem_t _sem;
            #endif
    };
}

#endif   // #ifndef INCL_SEMAPHORE_H
