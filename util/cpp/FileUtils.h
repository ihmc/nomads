/*
 * FileUtils.h
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

#ifndef INCL_FILEUTILS_H
#define INCL_FILEUTILS_H

#include "FTypes.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class FileUtils
    {
        public:
            // note: parent directories are made as needed
            static bool createDirectory (const char *pszPath);

            // Deletes the contents of a directory recursively
            static bool deleteDirectory (const char *pszPath);

            // Checks if a directory exists
            static bool directoryExists (const char *pszPath);

            // Return a NULL-terminated list of the files in pszPath.
            // If bIncludeDirs is set on true the list will contain also the
            // subdirectories, if it is set on false, only regular files will
            // appear in the list
            static char ** listFilesInDirectory (const char *pszPath, bool bIncludeDirs);

            // Returns an array of strings containing the names
            // of the subdirectories of a specified directory. The 
            // array terminator is an empty string.
            //
            // NOTE: remember to call a delete[] on the
            // returned array when done using it
            static String * enumSubdirs (const char *pszPath);

            // Checks if a file exists
            static bool fileExists (const char *pszPath);

            // Returns the file size in bytes (or a negative value in case of error)
            static int64 fileSize (const char *pszPath);

            // Read the contents of the specified file into a newly allocated buffer and return the buffer
            // The size of the file is written into pui32FileSize (if it is not NULL)
            // Caller is responsible for deleting the buffer
            static void * readFile (const char *pszPath, int64 *pi64FileSize);

            // Transforms all instances of the path separator character between '\' (used by Windows) and '/' (used by Linux)
            static void transformPathSeparator (char *pszPath);

            // Create the directory structure
            // /*!!*/ Document this method better
            static void createDirStructure (const char *pszPath);

            // Removes a file from the file system. It returns 0 if the file exists and was delated, a positive number if
            // the file does not exists, a negative number if it exists but it was not deleted
            static int deleteFile (const char *pszPath);

            static int dumpBufferToFile (const void *pBuf, uint64 ui32BufLen, const char *pszPath);
    };
}

#endif  //INCL_FILEUTILS_H
