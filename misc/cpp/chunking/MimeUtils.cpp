/*
 * MimeUtils.cpp
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

#include "MimeUtils.h"

#include "Defs.h"
#include "FileUtils.h"
#include "Logger.h"
#include "NLFLib.h"

#include <stddef.h>
#include <string.h>

#ifdef WIN32
    #define stringcasecmp _stricmp
#else
    #define stringcasecmp strcasecmp
#endif

using namespace IHMC_MISC;
using namespace NOMADSUtil;

const char * MimeUtils::DEFAULT_MIME_TYPE = "application/octet-stream";

char * MimeUtils::getMimeType (const char *pszFileName)
{
    if (pszFileName == NULL) {
        checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Warning,
                        "File name is null\n");
        return NULL;
    }
    if (!FileUtils::fileExists (pszFileName)) {
        checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Warning,
                        "File %s not found\n", pszFileName);
        return NULL;
    }

    /*magic_t magicMimePredictor = getPredictor();
    if (magicMimePredictor == NULL) {
        return strDup (DEFAULT_MIME_TYPE);
    }

    char *pszMime = strDup (magic_file (magicMimePredictor, pszFileName));
    releasePredictor (magicMimePredictor);

    checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Info,
                    "File %s has MIME type: %s\n", pszFileName, pszMime);
    return pszMime;*/
    return NULL;
}

char * MimeUtils::getMimeType (FILE *pFile)
{
    // TODO: implement this
    return strDup (DEFAULT_MIME_TYPE);
}

char * MimeUtils::getMimeType (const void *pBuf, uint64 ui64Len)
{
    /*magic_t magicMimePredictor = getPredictor();
    if (magicMimePredictor == NULL) {
        return strDup (DEFAULT_MIME_TYPE);
    }

    char *pszMime = strDup (magic_buffer (magicMimePredictor, pBuf, ui64Len));
    releasePredictor (magicMimePredictor);

    checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Info,
                    "Buffer has MIME type: %s\n", pszMime);
    return pszMime;*/
    return NULL;
}

Chunker::Type MimeUtils::mimeTypeToFragmentType (const char *pszMimeType)
{
    if (pszMimeType == NULL) {
        return Chunker::UNSUPPORTED;
    }
    if ((0 == stringcasecmp (pszMimeType, "image/x-ms-bmp")) ||
        (0 == stringcasecmp (pszMimeType, "image/x-bmp"))) {    
        return Chunker::BMP;
    }
    else if (0 == stringcasecmp (pszMimeType, "image/jpeg")) {
        return Chunker::JPEG;
    }
    else if (0 == stringcasecmp (pszMimeType, "image/jp2")) {
        return Chunker::JPEG2000;
    }
    else if (0 == stringcasecmp (pszMimeType, "video/mpeg")) {
        return Chunker::V_MPEG;                     
    }
    else if (0 == stringcasecmp (pszMimeType, "audio/mpeg")) {
        return Chunker::A_MPEG;
    }
    else if (0 == stringcasecmp (pszMimeType, "image/png")) {
        return Chunker::PNG;
    }
    else {
        return Chunker::UNSUPPORTED;
    }
}

/*magic_t MimeUtils::getPredictor()
{
    magic_t magicMimePredictor = magic_open (MAGIC_MIME_TYPE);
    if (magicMimePredictor == NULL) {
        checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Warning,
                        "libmagic: Unable to initialize magic library\n");
        return NULL;
    }
    if (magic_load (magicMimePredictor, 0)) {
        checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Warning,
                        "libmagic: can't load magic database - %s\n",
                        magic_error (magicMimePredictor));
        magic_close (magicMimePredictor);
        return NULL;
    }
    return magicMimePredictor;
}

void MimeUtils::releasePredictor (magic_t predictor)
{
    magic_close (predictor);
}*/

