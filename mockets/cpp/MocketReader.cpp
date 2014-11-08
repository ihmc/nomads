/*
 * MocketReader.cpp
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "MocketReader.h"

#include "Mocket.h"
#include "StreamMocket.h"


MocketReader::MocketReader (StreamMocket *pMocket, bool bDeleteWhenDone)
{
    _ui32TotalBytesRead = 0;
    _ulReadTimeout = 0;

    _pStreamMocket = pMocket;
    _bDeleteWhenDone = bDeleteWhenDone;
}

MocketReader::~MocketReader()
{
    if (_bDeleteWhenDone) {
        delete _pStreamMocket;
    }
    _pStreamMocket = NULL;
}

int MocketReader::read (void *pBuf, int iCount)
{
    int rc = _pStreamMocket->receive (pBuf, iCount, _ulReadTimeout);
    if (rc >= 0) {
        _ui32TotalBytesRead += rc;
    }

    return rc;
}

int MocketReader::readBytes (void *pBuf, unsigned long ulCount)
{
    int rc;
    unsigned long ulReadBytes = 0;

    while (ulReadBytes < ulCount) {
        rc = read ((char*)pBuf + ulReadBytes, ulCount - ulReadBytes);
        if (rc < 0) {
            return rc;
        }

        ulReadBytes += rc;
    }

    return 0;
}

uint32 MocketReader::getBytesAvailable()
{
    return _pStreamMocket->getCumulativeSizeOfAvailableMessages();
}

uint32 MocketReader::getTotalBytesRead()
{
    return _ui32TotalBytesRead;
}

int MocketReader::close()
{
    return _pStreamMocket->close();
}
