/*
 * LineOrientedReader.cpp
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

#include "LineOrientedReader.h"

#include <stdio.h>
#include <string.h>

using namespace NOMADSUtil;

LineOrientedReader::LineOrientedReader (Reader *pr, bool bDeleteWhenDone)
{
    _pReader = pr;
    _bDeleteReaderWhenDone = bDeleteWhenDone;
    _chBuffer = 0;
    _bByteInBuffer = false;
}

LineOrientedReader::~LineOrientedReader (void)
{
    if (_bDeleteReaderWhenDone) {
        delete _pReader;
    }
    _pReader = NULL;
    _chBuffer = 0;
    _bByteInBuffer = false;
}

int LineOrientedReader::read (void *pBuf, int iCount)
{
    if (_bByteInBuffer) {
        if (iCount < 0) {
            return -1;
        }
        else if (iCount == 0) {
            return 0;
        }
        *((char*)pBuf) = _chBuffer;
        _bByteInBuffer = false;
        int rc = _pReader->read (((char*)pBuf)+1, iCount-1);
        if (rc >= 0) {
            return rc+1;
        }
        else {
            return 1;
        }
    }
    else {
        return _pReader->read (pBuf, iCount);
    }
}

int LineOrientedReader::readBytes (void *pBuf, uint32 ui32Count)
{
    if (_bByteInBuffer) {
        if (ui32Count == 0) {
            return 0;
        }
        *((char*)pBuf) = _chBuffer;
        _bByteInBuffer = false;
        return _pReader->readBytes (((char*)pBuf)+1, ui32Count-1);
    }
    else {
        return _pReader->readBytes (pBuf, ui32Count);
    }
}

int LineOrientedReader::readLine (char *pszBuf, int iBufSize)
{
    int iBytesRead = 0;
    bool bCRFound = false;
    while (true) {
        char ch;
        if (read (&ch, 1) <= 0) {
            return -1;
        }
        if (ch == '\r') {
            bCRFound = true;
        }
        else if (ch == '\n') {
            // End of the line
            if (iBufSize <= 0) {
                return -2;
            }
            *pszBuf = '\0';
            return iBytesRead;
        }
        else if (bCRFound) {
            // Found a CR, but this is not an LF, so put this back and return
            _chBuffer = ch;
            _bByteInBuffer = true;
            return iBytesRead;
        }
        else {
            if (iBufSize <= 0) {
                return -2;
            }
            // Just add this byte and continue
            iBytesRead++;
            *pszBuf++ = ch;
            iBufSize--;
        }
    }
}
