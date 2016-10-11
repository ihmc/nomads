/*
 * FileWriter.cpp
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

#include "FileWriter.h"

using namespace NOMADSUtil;

FileWriter::FileWriter (FILE *file, bool bCloseWhenDone)
{
    fileOutput = file;
    _bHandleFD = bCloseWhenDone;
}

FileWriter::FileWriter (const char *pszFilename, const char *pszMode)
{
    fileOutput = fopen (pszFilename, pszMode);
    _bHandleFD = true;
}

FileWriter::~FileWriter (void)
{
    if (_bHandleFD && fileOutput != NULL) {
        if (ftell (fileOutput)>=0) { //file could be already closed
            fclose (fileOutput);
        }
    }
    fileOutput = NULL;
}

int FileWriter::writeBytes (const void *pBuf, unsigned long ulCount)
{
    if (fileOutput == NULL) {
        return -1;
    }
    if (ulCount != fwrite (pBuf, 1, ulCount, fileOutput)) {
        return -1;
    }
    return 0;
}

int FileWriter::flush (void)
{
    if (fileOutput == NULL) {
        return -1;
    }
    return fflush (fileOutput);
}

int FileWriter::close()
{
    if (fileOutput == NULL) {
        return -1;
    }
    if (ftell (fileOutput) >= 0) { //file could be already closed
        int rc = fclose (fileOutput);
        fileOutput = NULL;
        return rc;
    }
    else {
        return 0;
    }
}
