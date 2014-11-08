/*
 * Utils.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2014 IHMC.
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

#include "Utils.h"

#include "BufferWriter.h"
#include "Defs.h"
#include "MD5.h"

#include "FileWriter.h"
#include "Logger.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_MISC;
using namespace NOMADSUtil;

unsigned int Utils::getPadding (unsigned int uiLength, uint8 ui8NChunks)
{
    unsigned int uiReminder = uiLength % ui8NChunks;
    unsigned int padding = 0;
    if (uiReminder > 0) {
        padding = ui8NChunks - uiReminder;
    }
    return padding;
}
        
char * Utils::getMD5Checksum (Reader *pReader)
{
    uint32 ui32Len = pReader->getBytesAvailable();
    void *pBuf = malloc (ui32Len);
    pReader->readBytes (pBuf, ui32Len);
    return getMD5Checksum ((const void*)pBuf, ui32Len);
}

char * Utils::getMD5Checksum (BufferWriter *pWriter)
{
    return getMD5Checksum (pWriter->getBuffer(), pWriter->getBufferLength());
}

char * Utils::getMD5Checksum (const void *pBuf, uint32 ui32Len)
{
    void *pBufCpy = malloc (ui32Len);
    memcpy (pBufCpy, pBuf, ui32Len);
    char *pszChecksum = getMD5Checksum (pBufCpy, ui32Len);
    free (pBufCpy);
    return pszChecksum;
}

char * Utils::getMD5Checksum (void *pBuf, uint32 ui32Len)
{
    MD5 md5;
    md5.init();
    md5.update (pBuf, ui32Len);
    return md5.getChecksumAsString();
}

void Utils::dump (const char *pszOutFileName, const void *pBuf, uint64 ui64Len)
{
    FILE *pFile = fopen (pszOutFileName, "wb");
    if (pFile == NULL) {
        checkAndLogMsg ("dump", Logger::L_SevereError,
                        "could not open dump file %s\n", pszOutFileName);
        return;
    }
    FileWriter fw (pFile);
    fw.writeBytes (pBuf, ui64Len);
}

