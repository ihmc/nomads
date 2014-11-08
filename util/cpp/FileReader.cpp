/*
 * FileReader.cpp
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

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "FileReader.h"

using namespace NOMADSUtil;

FileReader::FileReader (const char *pszFilePath, const char *pszMode)
{
    _pfileInput = fopen (pszFilePath, pszMode);
    _bCloseFileWhenDone = true;
}

FileReader::FileReader (FILE *pfile, bool bCloseFileWhenDone)
{
    _pfileInput = pfile;
    _bCloseFileWhenDone = bCloseFileWhenDone;
}

FileReader::~FileReader (void)
{
    if (_bCloseFileWhenDone) {
        fclose (_pfileInput);
    }
    _pfileInput = NULL;
}

int FileReader::read (void *pBuf, int iCount)
{
    int rc;
    if ((rc = (int) fread (pBuf, 1, iCount, _pfileInput)) < 0) {
        return -1;
    }
    return rc;
}

int FileReader::readBytes (void *pBuf, uint32 ui32Count)
{
    if (ui32Count != fread (pBuf, 1, ui32Count, _pfileInput)) {
        return -1;
    }
    return 0;
}
