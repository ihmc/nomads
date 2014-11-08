/*
 * Thread.cpp
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

#include "Thread.h"

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
    #define strdup _strdup
#endif

using namespace NOMADSUtil;

Thread::Thread (void)
{
    _bStarted = false;
    _pszName = NULL;
}

Thread::~Thread (void)
{
    if (_pszName) {
        free (_pszName);
        _pszName = NULL;
    }
}

void Thread::setName (const char *pszName)
{
    if (_pszName) {
        free (_pszName);
        _pszName = NULL;
    }
    if (pszName) {
        _pszName = strdup (pszName);
        _ost.setThreadName("%s", pszName);
    }
}

int Thread::start (bool bDetached)
{
    if (_bStarted) {
        return -1;
    }
    #if defined (WIN32)
        if (_ost.start (callRun, this)) {
            return -2;
        }
    #elif defined (UNIX)
        if (_ost.start (callRun, this, bDetached)) {
            return -2;
        }
    #endif
    _bStarted = true;
    return 0;
}

void Thread::callRun (void *pArg)
{
    OSThread::setThreadLocalData (pArg);
    ((Thread*)pArg)->run();
}
