/*
 * DataRelayer.cpp
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

#include "DataRelayer.h"

#include <stdio.h>

using namespace NOMADSUtil;

DataRelayer::DataRelayer (Reader *pReader, Writer *pWriter, bool bDeleteWhenDone)
    : _cv (&_m)
{
    _pReader = pReader;
    _pWriter = pWriter;
    _bDeleteWhenDone = bDeleteWhenDone;
    _bDone = false;
}

DataRelayer::~DataRelayer (void)
{
    if (_bDeleteWhenDone) {
        delete _pReader;
        _pReader = NULL;
        delete _pWriter;
        _pWriter = NULL;
    }
}

void DataRelayer::run (void)
{
    while (true) {
        char buf[1024];
        int rc = _pReader->read (buf, sizeof (buf));
        if (rc <= 0) {
            break;
        }
        else {
            if (_pWriter->writeBytes (buf, rc)) {
                break;
            }
        }
    }
    _m.lock();
    _bDone = true;
    _cv.notifyAll();
    _m.unlock();
}

void DataRelayer::blockUntilDone (void)
{
    _m.lock();
    while (_bDone == false) {
        _cv.wait();
    }
    _m.unlock();
}
