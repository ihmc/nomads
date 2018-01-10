/*
 * SemClass.cpp
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

#include "SemClass.h"

#if defined (WIN32)
	#define NOMINMAX
	#include <winsock2.h>
    #include <windows.h>
#elif defined (UNIX)
    #include <semaphore.h>
#endif

using namespace NOMADSUtil;

Semaphore::Semaphore (int iInitValue)
{
    #if defined (WIN32)
        hSem = CreateSemaphore (NULL, iInitValue, 65535, NULL);
    #elif defined (UNIX)
        sem_init (&_sem, 0, iInitValue);
    #endif
}

Semaphore::~Semaphore (void)
{
    #if defined (WIN32)
        CloseHandle (hSem);
    #elif defined (UNIX)
        sem_destroy (&_sem);
    #endif
}

int Semaphore::up (void)
{
    #if defined (WIN32)
        if (0 == ReleaseSemaphore (hSem, 1, NULL)) {
            return -1;
        }
    #elif defined (UNIX)
        if (sem_post (&_sem)) {
            return -1;
        }
    #endif
    return 0;
}

int Semaphore::down (void)
{
    #if defined (WIN32)
        if (WAIT_OBJECT_0 != WaitForSingleObject (hSem, INFINITE)) {
            return -1;
        }
    #elif defined (UNIX)
        if (sem_wait (&_sem)) {
            return -1;
        }
    #endif
    return 0;
}
