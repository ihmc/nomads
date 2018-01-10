/*
 * FileReader.cpp
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

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "FileReader.h"

using namespace NOMADSUtil;

FileReader::FileReader (const char *pszFilePath, const char *pszMode)
    : _bCloseFileWhenDone (true),
      _pfileInput (fopen (pszFilePath, pszMode))
{
}

FileReader::FileReader (FILE *pfile, bool bCloseFileWhenDone)
    : _bCloseFileWhenDone (bCloseFileWhenDone),
      _pfileInput (pfile)
{
}

FileReader::~FileReader (void)
{
    if ((_bCloseFileWhenDone) && (_pfileInput != NULL)) {
        fclose (_pfileInput);
    }
    _pfileInput = NULL;
}

int FileReader::read (void *pBuf, int iCount)
{
    int rc;
    if (_pfileInput == NULL) {
        return -1;
    }
    if ((rc = (int) fread (pBuf, 1, iCount, _pfileInput)) < 0) {
        return -2;
    }
    _ui32TotalBytesRead += rc;
    return rc;
}

int FileReader::readBytes (void *pBuf, uint32 ui32Count)
{
    if (_pfileInput == NULL) {
        return -1;
    }
    size_t rc = fread (pBuf, 1, ui32Count, _pfileInput);
    if (ui32Count != rc) {
        return -2;
    }
    _ui32TotalBytesRead += rc;
    return 0;
}

int FileReader::seek (long lPos)
{
    if (_pfileInput == NULL) {
        return -1;
    }
    if (fseek (_pfileInput, lPos, SEEK_SET) < 0) {
        return -2;
    }
    return 0;
}

