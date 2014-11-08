/*
 * File.cpp
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

#include "File.h"

#include "FileUtils.h"
#include "NLFLib.h"

#include <string.h>

#if defined (WIN32)
    #include <windows.h>
    #define PATH_MAX MAX_PATH
#endif

namespace NOMADSUtil
{   
    void split (const String &path, String &parentDir, String &fileName, char chSep)
    {
        int iPathLen = path.length();
        if (iPathLen <= 0) {
            parentDir = "";
            fileName = "";
            return;
        }

        char buf[PATH_MAX];
        strcpy (buf, path);

        // Remove separator if it's the last character
        if (buf[iPathLen-1] == chSep) {
            buf[iPathLen-1] = '\0';
            iPathLen--;
        }

        int i = path.lastIndexOf (chSep);
        if (i < 0) {
            parentDir = "";
            fileName = buf;
            return;
        }

        buf[i] = '\0';
        parentDir = buf;
        fileName = &(buf[i+1]);
    }

    String getparentdir (const String &path)
    {
        String parentDir, fileName;
        split (path, parentDir, fileName, getPathSepChar());
        return parentDir;
    }

    String getfilename (const String &path)
    {
        String parentDir, fileName;
        split (path, parentDir, fileName, getPathSepChar());
        return fileName;
    }
}

using namespace NOMADSUtil;

File::File (const String &path)
    : _parentDir (getparentdir (path)), _fileName (getfilename (path))
{
}

File::File (const String &parentDir, const String &fileName)
    : _parentDir (parentDir), _fileName (fileName)
{
}

File::~File (void)
{
}

bool File::exists (void) const
{
    return FileUtils::fileExists (getPath()) || FileUtils::directoryExists (getPath());
}

String File::getExtension (void) const
{
    String name, extension;
    split (getName(), name, extension, '.');
    return extension;
}

int64 File::getFileSize (void) const
{
    return FileUtils::fileSize (getPath());
}

String File::getName (bool bExcludeExtension) const
{
    if (bExcludeExtension) {
        String name, extension;
        split (getName(), name, extension, '.');
        return name;
    }
    return _fileName;
}

String File::getParent (void) const
{
    return _parentDir;
}

String File::getPath (void) const
{
    String path (_parentDir);
    if ((path.length() > 0) && (path.endsWith (getPathSepCharAsString()))) {
        path += getPathSepChar();
    }
    path += _fileName;
    return path;
}

