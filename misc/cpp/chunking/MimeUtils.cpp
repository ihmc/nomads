/*
 * MimeUtils.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2016 IHMC.
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
#include "File.h"
#include "FileUtils.h"
#include "Logger.h"
#include "NLFLib.h"

#include <string.h>

#ifdef WIN32
    #define stringcasecmp _stricmp
#else
    #define stringcasecmp strcasecmp
#endif

using namespace IHMC_MISC;
using namespace NOMADSUtil;

const char * MimeUtils::DEFAULT_MIME_TYPE = "application/octet-stream";

NOMADSUtil::String MimeUtils::getMimeType (const char *pszFileName)
{
    if (pszFileName == NULL) {
        checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Warning,
                        "File name is null\n");
        return String();
    }
    if (!FileUtils::fileExists (pszFileName)) {
        checkAndLogMsg ("MimeUtils::getMimeType", Logger::L_Warning,
                        "File %s not found\n", pszFileName);
        return String();
    }
    File file (pszFileName);
    return toType (file.getExtension());
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
    if (0 == stringcasecmp (pszMimeType, "image/jpeg")) {
        return Chunker::JPEG;
    }
    if (0 == stringcasecmp (pszMimeType, "image/jp2")) {
        return Chunker::JPEG2000;
    }
    if (0 == stringcasecmp (pszMimeType, "video/mpeg")) {
        return Chunker::V_MPEG;                     
    }
    if (0 == stringcasecmp (pszMimeType, "audio/mpeg")) {
        return Chunker::A_MPEG;
    }
    if (0 == stringcasecmp (pszMimeType, "image/png")) {
        return Chunker::PNG;
    }
    if ((0 == stringcasecmp (pszMimeType, "application/x-troff-msvideo")) ||
        (0 == stringcasecmp (pszMimeType, "video/avi")) ||
        (0 == stringcasecmp (pszMimeType, "video/msvideo")) ||
        (0 == stringcasecmp (pszMimeType, "video/x-msvideo"))) {
        return Chunker::AVI;
    }
    if (0 == stringcasecmp (pszMimeType, "video/quicktime")) {
        return Chunker::MOV;
    }
    if (0 == stringcasecmp (pszMimeType, "video/mpeg")) {
        return Chunker::V_MPEG;
    }

    return Chunker::UNSUPPORTED;

}

Chunker::Type MimeUtils::toType (const String &extension)
{
    if (extension ^= "bmp") {
        return Chunker::BMP;
    }
    if ((extension ^= "jpeg") || (extension == "jpg")) {
        return Chunker::JPEG;
    }
    if (extension ^= "jp2") {
        return Chunker::JPEG2000;
    }
    if (extension ^= "png") {
        return Chunker::PNG;
    }
    if (extension ^= "avi") {
        return Chunker::AVI;
    }
    if (extension ^= "mov") {
        return Chunker::MOV;
    }
    if (extension ^= "mp4") {
        return Chunker::V_MPEG;
    }
    if (extension ^= "mpg") {
        return Chunker::V_MPEG;
    }
    return Chunker::UNSUPPORTED;
}

String MimeUtils::toExtesion (Chunker::Type extension)
{
    switch (extension) {
        case Chunker::BMP: return String ("bmp");
        case Chunker::JPEG: return String ("jpg");
        case Chunker::JPEG2000: return String ("jp2");
        case Chunker::PNG: return String ("png");
        case Chunker::AVI: return String ("avi");
        case Chunker::MOV: return String ("mov");
        case Chunker::V_MPEG: return String ("mpeg");
        default: return String();
    }
}

String MimeUtils::toMimeType (Chunker::Type extension)
{
    switch (extension) {
        case Chunker::BMP: return String ("image /x-bmp");
        case Chunker::JPEG: return String ("image/jpeg");
        case Chunker::JPEG2000: return String ("image/jp2");
        case Chunker::PNG: return String ("image/png");
        case Chunker::AVI: return String ("video/avi");
        case Chunker::MOV: return String ("video/quicktime");
        case Chunker::V_MPEG: return String ("video/mpeg");
        default: return String();
    }
}

