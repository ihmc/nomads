/*
 * PushbackLineOrientedReader.cpp
 *
 *This file is part of the IHMC Util Library
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

#include "PushbackLineOrientedReader.h"

#include <string.h>

using namespace NOMADSUtil;

PushbackLineOrientedReader::PushbackLineOrientedReader (Reader *pr, bool bDeleteWhenDone)
    : LineOrientedReader (pr, bDeleteWhenDone)
{
    _bBufferEmpty = true;
}

PushbackLineOrientedReader::~PushbackLineOrientedReader (void)
{
}

int PushbackLineOrientedReader::pushbackLine (const char *pszLine)
{
    if (pszLine == NULL) {
        return -1;
    }
    if (!_bBufferEmpty) {
        return -2;
    }
    _bufferedLine = pszLine;
    _bBufferEmpty = false;
    return 0;
}

int PushbackLineOrientedReader::readLine (char *pszBuf, int iBufSize)
{
    if (_bBufferEmpty) {
        return LineOrientedReader::readLine (pszBuf, iBufSize);
    }
    else {
        int iLineLength = _bufferedLine.length();
        if (iBufSize <= iLineLength) {
            return -2;  // -2 imples buffer was too small - see readLine in LineOrientedReader.h
        }
        strcpy (pszBuf, _bufferedLine);
        _bBufferEmpty = true;
        return iLineLength;
    }
}
