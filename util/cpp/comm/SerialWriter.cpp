/*
 * SerialWriter.cpp
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

#include "SerialWriter.h"

#include "Serial.h"

using namespace NOMADSUtil;

SerialWriter::SerialWriter (Serial *pSerial, bool bDeleteWhenDone)
{
    _pSerial = pSerial;
    _bDeleteWhenDone = bDeleteWhenDone;
}

SerialWriter::~SerialWriter (void)
{
    if (_bDeleteWhenDone) {
        delete _pSerial;
    }
    _pSerial = NULL;
}

int SerialWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    int rc = _pSerial->write (pBuf, ulCount);
    if (rc != (int) ulCount) {
        return -1;
    }
    return 0;
}
