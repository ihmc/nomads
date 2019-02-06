/*
 * ManageableThread.cpp
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
 *
 * Written by Niranjan Suri (nsuri@ihmc.us)
 */

#include "ManageableThread.h"

#include <stddef.h>

using namespace NOMADSUtil;

ManageableThread::ManageableThread (void)
    : _cv (&_m)
{
    _bRunning = false;
    _bTerminateRequested = false;
    _iTerminatingResultCode = 0;
    _bTerminated = false;
    _pTerminationCallbackFn = NULL;
    _pTerminationCallbackArg = NULL;
}

ManageableThread::~ManageableThread (void)
{
}

void ManageableThread::requestTerminationAndWait (void)
{
    _m.lock();
    if (_bRunning) {
        _bTerminateRequested = true;
        while (!_bTerminated) {
            _cv.wait (1000);
        }
        _ost.waitForThreadToTerminate();
    }
    _m.unlock();
}
