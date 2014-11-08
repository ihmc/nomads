/*
 * SerialReader.cpp
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

#include "SerialReader.h"

#include "Serial.h"

#include <stdio.h>

using namespace NOMADSUtil;

SerialReader::SerialReader (Serial *pSerial, bool bDeleteWhenDone)
{
    _pSerial = pSerial;
    _bDeleteWhenDone = bDeleteWhenDone;
}

SerialReader::~SerialReader (void)
{
    if (_bDeleteWhenDone) {
        delete _pSerial;
        _pSerial = NULL;
    }    
}

int SerialReader::read (void *pBuf, int iCount)
{
    uint32 ui32Count = (uint32) iCount;
    uint32 ui32BytesAvail = _pSerial->getBytesAvailable();
    if (ui32BytesAvail > ui32Count) {
        return _pSerial->read (pBuf, ui32Count);
    }
    else if (ui32BytesAvail > 0) {
        return _pSerial->read (pBuf, ui32BytesAvail);
    }
    else {
        // There are no bytes available - wait until at least one byte is available
        int rc = _pSerial->read (pBuf, 1);
        if (rc <= 0) {
            return rc;
        }
        // Now check if there happen to be more bytes, if so, read those also
        ui32Count -= 1;
        ui32BytesAvail = _pSerial->getBytesAvailable();
        uint32 ui32BytesToRead = ui32BytesAvail;
        if (ui32BytesAvail > ui32Count) {
            ui32BytesToRead = ui32Count;
        }
        if ((rc = _pSerial->read (((uint8*)pBuf)+1, ui32BytesToRead)) >= 0) {
            return rc + 1;   // + 1 for the earlier one byte read
        }
        else {
            // The second read returned a failure, but we still have successfully read one byte, so return that
            // Presumably, the next call to read will still be a failure, and it will be reported then
            return 1;
        }
    }
}

uint32 SerialReader::getBytesAvailable (void)
{
    return _pSerial->getBytesAvailable();
}
